// t and r: a stream's syntax dumped to a TSV file, and the identical stream
// restored from it (the rows: tsv.inc's channel; their format: README).

namespace h264p {

const char kMagic[] = "h264p";  // TSV files
const int kVersion = 1;

// The trailer of a NAL unit from its "trail" and "czw" rows, if any, applied.
void write_trailer(RowReader& in, BitWriter& bw, int pad) {
  Trailer tr;
  const Row* r = in.peek();
  if (r && r->is("trail")) {
    const Row& t = in.take();
    if (t.size() != 3) t.fail("expected 2 values");
    int64_t drop = t.num(1);
    if (drop < 0 || (size_t)drop > bw.pos()) t.fail("bad bit count");
    const char* bits = t.field(2);
    if (strcmp(bits, "-") != 0)
      for (const char* p = bits; *p; p++) {
        if (*p != '0' && *p != '1') t.fail("bad bit string");
        tr.bits += *p;
      }
    if ((bw.pos() - (size_t)drop + tr.bits.size()) & 7) t.fail("does not end on a byte boundary");
    tr.irregular = true;
    tr.drop = drop;
  }
  r = in.peek();
  if (r && r->is("czw")) {
    const Row& t = in.take();
    if (t.size() != 2) t.fail("expected 1 value");
    int64_t n = t.num(1);
    if (n < 1 || n > (1 << 26)) t.fail("bad count");
    tr.czw = n;
  }
  apply_trailer(tr, bw, pad);
}

std::vector<uint8_t> write_nal(int type, int ref_idc, Context& ctx, RowReader& in) {
  BitWriter bw;
  TsvChan ch(&in);
  int pad = write_syntax(type, ref_idc, ctx, ch, bw);
  write_trailer(in, bw, pad);
  return bw.data();
}

// Converts the payload of one NAL unit to rows, or returns false when it
// has to be stored verbatim (why; the rows it wrote are gone). trail: it
// ends with a trail row. obs: see find_trailer.
bool convert_nal(const NalHead& h, Context& ctx, RowWriter& out, std::string& why, bool& trail, int& obs) {
  std::vector<uint8_t> rbsp;
  trail = false;
  obs = -1;
  if (!nal_rbsp(h, ctx, rbsp, why)) return false;
  size_t start = out.size();
  try {
    TsvChan ch(&out);
    read_nal(h.type, h.ref_idc, rbsp, ctx, ch);
  } catch (const ParseError& e) {
    why = e.what();
    out.truncate(start);
    return false;
  }
  std::string text = out.text().substr(start);
  RowReader in(text.data(), text.size());
  TsvChan ch(&in);
  Trailer t;
  if (!write_back(h, ctx, ch, rbsp, t, why, obs)) {
    out.truncate(start);
    return false;
  }
  if (t.irregular) {
    out.begin("trail");
    out.add(t.drop);
    out.add(t.bits.empty() ? std::string("-") : t.bits);
    out.end();
    trail = true;
  }
  if (t.czw) out.row("czw", t.czw);
  return true;
}

int tsv_dump(const char* in_name, const char* out_name, const Options& opt) {
  filehandle fh;
  Output o;
  if (!open_io(in_name, out_name, fh, o)) return 1;
  FILE* f = o.g.f;
  Context ctx;
  PadDetector det;
  Stats st;
  RowWriter out;
  out.begin(kMagic);
  out.add(kVersion);
  out.end();

  AnnexBReader rd(file_source(fh.f));
  std::vector<uint8_t> nal;
  size_t zeros;
  bool more = rd.next(zeros, nal);
  if (!rd.junk().empty()) {
    out.begin("junk");
    out.add(to_hex(rd.junk().data(), rd.junk().size()));
    out.end();
  }
  for (; more; more = rd.next(zeros, nal)) {
    st.nals++;
    if (headless(nal)) {
      out.begin("nalx");
      out.add((int64_t)zeros);
      if (!nal.empty()) out.add(to_hex(nal.data(), nal.size()));
      out.end();
      st.verbatim("empty or forbidden_zero_bit");
      continue;
    }
    NalHead h = nal_head(nal, det);
    size_t mark = out.size();
    std::string why;
    bool trail;
    auto convert = [&](int& obs) {
      out.begin("nal");
      out.add((int64_t)zeros);
      out.add(h.ref_idc);
      out.add(h.type);
      out.end();
      return convert_nal(h, ctx, out, why, trail, obs);
    };
    int obs;
    bool ok = convert(obs);
    if (ok && repad(det, obs, ctx.pad)) {
      // once more, after an x264_padding row that predicts the bit
      out.truncate(mark);
      out.begin("x264_padding");
      if (ctx.pad.on)
        out.add(ctx.pad.frame);
      else
        out.add_absent();
      out.end();
      st.pad++;
      ok = convert(obs);
    }
    ctx.done(h.type, h.payload, h.len);
    if (ok) {
      st.parsed++;
      st.trail += trail;
    } else {
      out.begin("hex");
      out.add(to_hex(h.payload, h.len));
      out.end();
      if (!st.verbatim(h.type, why, opt.verbose, opt.strict, nullptr)) return close_io(1, fh, o, false);
    }
    if (out.size() > (1 << 22)) {
      put(f, out.text());
      out.truncate(0);
    }
  }
  out.row("eof", (int64_t)rd.eof_zeros());
  put(f, out.text());
  if (close_io(0, fh, o, false)) return 1;
  if (opt.verbose) st.report("converted", "x264_padding rows", nullptr);
  return 0;
}

int tsv_restore(const char* in_name, const char* out_name, const Options& opt) {
  filehandle fh;
  Output o;
  if (!open_io(in_name, out_name, fh, o)) return 1;
  FILE* fo = o.g.f;
  RowReader in(fh.f);
  Context ctx;
  std::vector<uint8_t> buf;
  long nals = 0;
  uint64_t written = 0, good = 0;  // the bytes written, and restored whole before the current row
  auto flush = [&]() {
    if (!buf.empty()) fwrite(buf.data(), 1, buf.size(), fo);
    written += buf.size();
    buf.clear();
  };
  auto spill = [&]() {
    if (buf.size() > (1 << 22)) flush();
  };
  try {
    const Row* first = in.peek();
    if (!first || !first->is(kMagic) || first->size() != 2 || first->num(1) != kVersion)
      throw TsvError("not an h264p file (version " + std::to_string(kVersion) + ")");
    in.take();
    bool eof = false;
    while (!eof) {
      good = written + buf.size();
      const Row& r = in.take();
      if (r.is("junk")) {
        std::vector<uint8_t> b = from_hex(r, 1);
        buf.insert(buf.end(), b.begin(), b.end());
      } else if (r.is("nal") || r.is("nalx")) {
        nals++;
        int64_t zeros = r.num(1);
        if (zeros < 0 || zeros > ((int64_t)1 << 40)) r.fail("bad zero count");
        put_start(buf, (uint64_t)zeros, spill);
        if (r.is("nalx")) {
          std::vector<uint8_t> b = from_hex(r, 2);
          buf.insert(buf.end(), b.begin(), b.end());
        } else {
          if (r.size() != 4) r.fail("expected 3 values");
          int64_t ref_idc = r.num(2), type = r.num(3);
          if (ref_idc < 0 || ref_idc > 3 || type < 0 || type > 31) r.fail("bad NAL unit header");
          buf.push_back((uint8_t)(ref_idc << 5 | type));
          size_t start = buf.size();
          const Row* p = in.peek();
          if (p && p->is("hex")) {
            put_verbatim(buf, (int)type, from_hex(in.take(), 1), ctx);
          } else {
            std::vector<uint8_t> e = escape(write_nal((int)type, (int)ref_idc, ctx, in));
            buf.insert(buf.end(), e.begin(), e.end());
          }
          ctx.done((int)type, buf.data() + start, buf.size() - start);
        }
      } else if (r.is("x264_padding")) {
        if (r.size() != 2) r.fail("expected 1 value");
        if (r.absent(1)) {
          ctx.pad.on = false;
        } else {
          int64_t frame = r.num(1);
          if (frame < 0 || frame > INT32_MAX) r.fail("bad frame number");
          ctx.pad.set(frame);
        }
      } else if (r.is("eof")) {
        int64_t n = r.num(1);
        if (n < 0 || n > ((int64_t)1 << 40)) r.fail("bad zero count");
        put_zeros(buf, (uint64_t)n, spill);
        eof = true;
      } else {
        r.fail("unexpected row");
      }
      spill();
    }
    if (const Row* extra = in.peek()) extra->fail("rows after eof");
  } catch (const std::exception& e) {  // TsvError, or an inconsistency only found when writing
    fprintf(stderr, "h264p: %s: %s\n", in_name, e.what());
    // (what was restored before the row that failed stays)
    if (good >= written) buf.resize((size_t)(good - written));
    flush();
    return close_io(1, fh, o, written > 0);
  }
  flush();
  if (close_io(0, fh, o, false)) return 1;
  if (opt.verbose) fprintf(stderr, "%ld NAL units\n", nals);
  return 0;
}

}  // namespace h264p
