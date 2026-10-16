// h264p: dumps the syntax of an H.264 Annex B stream to a TSV file with the
// entropy coding removed (c), and restores the identical stream from it (d).
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "bits.h"
#include "rows.h"
#include "slice.h"
#include "syntax.h"

using namespace h264p;

namespace {

const char kMagic[] = "h264p";
const int kVersion = 1;

struct Options {
  bool verbose = false;
  bool strict = false;
};

struct Stats {
  long nals = 0, parsed = 0, raw = 0, trail = 0;
  std::map<int, long> raw_by_type;
  std::map<std::string, long> reasons;
};

// State shared by all NAL units of a stream.
struct Context {
  ParamSets ps;
  MbStore mbs;
};

std::vector<uint8_t> unescape(const uint8_t* p, size_t n) {
  std::vector<uint8_t> r;
  r.reserve(n);
  int zeros = 0;
  for (size_t i = 0; i < n; i++) {
    if (zeros >= 2 && p[i] == 3) {
      zeros = 0;
      continue;
    }
    r.push_back(p[i]);
    zeros = p[i] ? 0 : zeros + 1;
  }
  return r;
}

std::vector<uint8_t> escape(const std::vector<uint8_t>& r) {
  std::vector<uint8_t> e;
  e.reserve(r.size() + r.size() / 64 + 2);
  int zeros = 0;
  for (uint8_t b : r) {
    if (zeros >= 2 && b <= 3) {
      e.push_back(3);
      zeros = 0;
    }
    e.push_back(b);
    zeros = b ? 0 : zeros + 1;
  }
  if (!r.empty() && r.back() == 0) e.push_back(3);
  return e;
}

bool parsed_type(int type) { return type == 1 || type == 5 || type == 6 || type == 7 || type == 8 || type == 9 || type == 10 || type == 11; }

// Parses the RBSP of a NAL unit into rows; parameter sets are stored in the
// context. Throws ParseError.
void read_nal(int type, int ref_idc, const std::vector<uint8_t>& rbsp, Context& ctx, RowWriter& out) {
  BitReader br(rbsp.data(), rbsp.size());
  Syn s(&br, &out);
  switch (type) {
    case 7: {
      auto sps = std::make_shared<Sps>();
      code_sps(s, *sps);
      ctx.ps.sps[sps->id] = sps;
      break;
    }
    case 8: {
      auto pps = std::make_shared<Pps>();
      code_pps(s, *pps, ctx.ps);
      ctx.ps.pps[pps->id] = pps;
      break;
    }
    case 6:
      code_sei(s);
      break;
    case 9:
      code_aud(s);
      break;
    case 10:
    case 11:
      if (!rbsp.empty()) throw ParseError("end of sequence/stream NAL unit with data");
      break;
    case 1:
    case 5: {
      SliceHeader h;
      h.nal_unit_type = type;
      h.nal_ref_idc = ref_idc;
      code_slice_header(s, h, ctx.ps);
      read_slice_data(h, ctx.mbs, br, out);
      break;
    }
    default:
      throw ParseError("unsupported NAL unit type");
  }
}

// Writes the syntax of a NAL unit from rows, up to and including the
// rbsp_stop_one_bit. Throws TsvError.
void write_syntax(int type, int ref_idc, Context& ctx, RowReader& in, BitWriter& bw) {
  Syn s(&in, &bw);
  bool trailing = true;
  switch (type) {
    case 7: {
      auto sps = std::make_shared<Sps>();
      code_sps(s, *sps);
      ctx.ps.sps[sps->id] = sps;
      break;
    }
    case 8: {
      auto pps = std::make_shared<Pps>();
      code_pps(s, *pps, ctx.ps);
      ctx.ps.pps[pps->id] = pps;
      break;
    }
    case 6:
      code_sei(s);
      break;
    case 9:
      code_aud(s);
      break;
    case 10:
    case 11:
      trailing = false;
      break;
    case 1:
    case 5: {
      SliceHeader h;
      h.nal_unit_type = type;
      h.nal_ref_idc = ref_idc;
      code_slice_header(s, h, ctx.ps);
      write_slice_data(h, ctx.mbs, in, bw);
      trailing = false;  // written with the slice data
      break;
    }
    default:
      throw TsvError("NAL unit type " + std::to_string(type) + " needs a hex row");
  }
  if (trailing) bw.bit(1);  // rbsp_stop_one_bit
}

// The rest of the RBSP: alignment zero bits, or what a "trail" row says
// instead, and the cabac_zero_words of a "czw" row.
void write_trailer(RowReader& in, BitWriter& bw) {
  const Row* r = in.peek();
  if (r && r->is("trail")) {
    const Row& t = in.take();
    if (t.size() != 3) t.fail("expected 2 values");
    int64_t drop = t.num(1);
    if (drop < 0 || (size_t)drop > bw.pos()) t.fail("bad bit count");
    bw.truncate(bw.pos() - (size_t)drop);
    const char* bits = t.field(2);
    if (strcmp(bits, "-") != 0)
      for (const char* p = bits; *p; p++) {
        if (*p != '0' && *p != '1') t.fail("bad bit string");
        bw.bit(*p - '0');
      }
    if (bw.pos() & 7) t.fail("does not end on a byte boundary");
  } else {
    bw.align_zero();
  }
  r = in.peek();
  if (r && r->is("czw")) {
    const Row& t = in.take();
    if (t.size() != 2) t.fail("expected 1 value");
    int64_t n = t.num(1);
    if (n < 1 || n > (1 << 26)) t.fail("bad count");
    bw.append(2 * (size_t)n, 0);
  }
}

std::vector<uint8_t> write_nal(int type, int ref_idc, Context& ctx, RowReader& in) {
  BitWriter bw;
  write_syntax(type, ref_idc, ctx, in, bw);
  write_trailer(in, bw);
  return bw.data();
}

// SPS and PPS stored verbatim still define parameter sets.
void update_params(int type, const std::vector<uint8_t>& rbsp, Context& ctx) {
  if (type != 7 && type != 8) return;
  try {
    RowWriter dummy;
    read_nal(type, 0, rbsp, ctx, dummy);
  } catch (const ParseError&) {
  }
}

int get_bit(const std::vector<uint8_t>& v, size_t i) { return v[i >> 3] >> (7 - (i & 7)) & 1; }

// Converts the payload of one NAL unit (after the header byte) to rows, or
// returns false when it has to be stored verbatim.
bool convert_nal(int type, int ref_idc, const uint8_t* p, size_t n, Context& ctx, RowWriter& out,
                 std::string& why, bool& trail) {
  std::vector<uint8_t> rbsp = unescape(p, n);
  trail = false;
  if (!parsed_type(type)) {
    why = "NAL unit type " + std::to_string(type);
    return false;
  }
  if (escape(rbsp) != std::vector<uint8_t>(p, p + n)) {
    why = "irregular emulation prevention";
    update_params(type, rbsp, ctx);
    return false;
  }
  size_t start = out.size();
  try {
    read_nal(type, ref_idc, rbsp, ctx, out);
  } catch (const ParseError& e) {
    why = e.what();
    out.truncate(start);
    return false;
  }
  try {
    std::string text = out.text().substr(start);
    RowReader in(text.data(), text.size());
    BitWriter bw;
    write_syntax(type, ref_idc, ctx, in, bw);
    if (!in.at_end()) throw TsvError("rows left over");
    BitWriter o = bw;
    o.align_zero();
    if (o.data() == rbsp) return true;
    // the end differs: cabac_zero_words, or trailing bits that are not
    // the standard ones (e.g. from another CABAC flushing procedure)
    size_t zeros = 0;
    while (zeros < rbsp.size() && rbsp[rbsp.size() - 1 - zeros] == 0) zeros++;
    size_t czw = zeros / 2;
    std::vector<uint8_t> r(rbsp.begin(), rbsp.end() - 2 * czw);
    RowWriter tail;
    if (o.data() != r) {
      // compare our bits up to and including the stop bit
      size_t pbits = bw.pos(), rbits = r.size() * 8, d = 0;
      while (d < pbits && d < rbits && bw.at(d) == get_bit(r, d)) d++;
      if (pbits - d > 64 || rbits - d > 4096) {
        why = "re-encoding differs at bit " + std::to_string(d) + " of " + std::to_string(rbits);
        out.truncate(start);
        return false;
      }
      std::string bits;
      for (size_t i = d; i < rbits; i++) bits += (char)('0' + get_bit(r, i));
      tail.begin("trail");
      tail.add((int64_t)(pbits - d));
      tail.add(bits.empty() ? std::string("-") : bits);
      tail.end();
      trail = true;
    }
    if (czw) tail.row("czw", (int64_t)czw);
    // check the rows as the restoring will use them
    RowReader in2(tail.text().data(), tail.text().size());
    write_trailer(in2, bw);
    if (bw.data() != rbsp || !in2.at_end()) {
      why = "re-encoding differs";
      out.truncate(start);
      return false;
    }
    out.text() += tail.text();
    return true;
  } catch (const std::exception& e) {
    why = std::string("re-encoding failed: ") + e.what();
    out.truncate(start);
    return false;
  }
}

// Splits an Annex B byte stream into NAL units while reading it.
class AnnexBReader {
 public:
  explicit AnnexBReader(FILE* f) : f_(f) {}

  // The next NAL unit: the 0x00 bytes before its start code (zero_byte, and
  // the trailing_zero_8bits of the previous NAL unit), and its bytes up to
  // the zero bytes before the next start code. False at the end.
  bool next(size_t& zeros, std::vector<uint8_t>& nal) {
    if (done_) return false;
    size_t k;
    if (!started_) {
      if (!find(k)) {  // no start code at all
        junk_.swap(buf_);
        done_ = true;
        return false;
      }
      size_t j = k;
      while (j > 0 && buf_[j - 1] == 0) j--;
      junk_.assign(buf_.begin(), buf_.begin() + j);
      zeros_ = k - j;
      started_ = true;
      head_ = scan_ = k + 3;
    }
    bool last = !find(k);
    if (last) k = buf_.size();
    size_t e = k;
    while (e > head_ && buf_[e - 1] == 0) e--;
    nal.assign(buf_.begin() + head_, buf_.begin() + e);
    zeros = zeros_;
    if (last) {
      eof_zeros_ = k - e;
      done_ = true;
    } else {
      zeros_ = k - e;
      head_ = scan_ = k + 3;
    }
    return true;
  }
  // bytes before the first start code (all bytes if there is none)
  const std::vector<uint8_t>& junk() const { return junk_; }
  // 0x00 bytes after the last NAL unit
  size_t eof_zeros() const { return eof_zeros_; }
  bool error() const { return ferror(f_) != 0; }

 private:
  // the next start code at or after scan_, reading as much as needed
  bool find(size_t& k) {
    for (;;) {
      for (size_t i = scan_; i + 2 < buf_.size(); i++)
        if (buf_[i + 2] == 1 && !buf_[i + 1] && !buf_[i]) {
          k = i;
          return true;
        }
      scan_ = buf_.size() > 2 ? buf_.size() - 2 : 0;
      if (scan_ < head_) scan_ = head_;
      // drop what was returned before reading more
      buf_.erase(buf_.begin(), buf_.begin() + head_);
      scan_ -= head_;
      head_ = 0;
      size_t old = buf_.size();
      buf_.resize(old + (1 << 20));
      size_t n = fread(&buf_[old], 1, 1 << 20, f_);
      buf_.resize(old + n);
      if (!n) return false;
    }
  }

  FILE* f_;
  std::vector<uint8_t> buf_, junk_;
  size_t head_ = 0;  // start of the current NAL unit in buf_
  size_t scan_ = 0;  // where to look for the next start code
  size_t zeros_ = 0, eof_zeros_ = 0;
  bool started_ = false, done_ = false;
};

FILE* open_out(const char* name) {
  FILE* f = strcmp(name, "-") ? fopen(name, "wb") : stdout;
  if (!f) {
    fprintf(stderr, "h264p: cannot create %s: %s\n", name, strerror(errno));
    exit(1);
  }
  return f;
}

void put(FILE* f, const std::string& s) { fwrite(s.data(), 1, s.size(), f); }

int compress(const char* in_name, const char* out_name, const Options& opt) {
  FILE* fi = strcmp(in_name, "-") ? fopen(in_name, "rb") : stdin;
  if (!fi) {
    fprintf(stderr, "h264p: cannot open %s: %s\n", in_name, strerror(errno));
    return 1;
  }
  FILE* f = open_out(out_name);
  Context ctx;
  Stats st;
  RowWriter out;
  out.begin(kMagic);
  out.add(kVersion);
  out.end();

  AnnexBReader rd(fi);
  std::vector<uint8_t> nal;
  size_t zeros;
  bool first = true;
  while (rd.next(zeros, nal)) {
    if (first && !rd.junk().empty()) {
      out.begin("junk");
      out.add(to_hex(rd.junk().data(), rd.junk().size()));
      out.end();
    }
    first = false;
    const uint8_t* data = nal.data();
    size_t p = 0, e = nal.size();
    st.nals++;
    if (e == p || (data[p] & 0x80)) {  // empty, or forbidden_zero_bit set
      out.begin("nalx");
      out.add((int64_t)zeros);
      if (e > p) out.add(to_hex(&data[p], e - p));
      out.end();
      st.raw++;
      st.reasons["empty or forbidden_zero_bit"]++;
      continue;
    }
    int ref_idc = data[p] >> 5 & 3, type = data[p] & 31;
    out.begin("nal");
    out.add((int64_t)zeros);
    out.add(ref_idc);
    out.add(type);
    out.end();
    std::string why;
    bool trail;
    if (convert_nal(type, ref_idc, &data[p + 1], e - p - 1, ctx, out, why, trail)) {
      st.parsed++;
      st.trail += trail;
    } else {
      out.begin("hex");
      out.add(to_hex(&data[p + 1], e - p - 1));
      out.end();
      st.raw++;
      st.raw_by_type[type]++;
      st.reasons[why]++;
      if (opt.verbose && parsed_type(type))
        fprintf(stderr, "h264p: NAL unit %ld (type %d) stored verbatim: %s\n", st.nals - 1, type, why.c_str());
      if (opt.strict && parsed_type(type)) {
        fprintf(stderr, "h264p: NAL unit %ld (type %d) cannot be converted: %s\n", st.nals - 1, type, why.c_str());
        return 1;
      }
    }
    if (out.size() > (1 << 22)) {
      put(f, out.text());
      out.truncate(0);
    }
  }
  if (first && !rd.junk().empty()) {  // no start code
    out.begin("junk");
    out.add(to_hex(rd.junk().data(), rd.junk().size()));
    out.end();
  }
  if (rd.error()) {
    fprintf(stderr, "h264p: read error\n");
    return 1;
  }
  if (fi != stdin) fclose(fi);
  out.row("eof", (int64_t)rd.eof_zeros());
  put(f, out.text());
  if (ferror(f) || (f != stdout && fclose(f))) {
    fprintf(stderr, "h264p: write error\n");
    return 1;
  }
  if (opt.verbose) {
    fprintf(stderr, "%ld NAL units: %ld converted (%ld with irregular trailing bits), %ld stored verbatim\n",
            st.nals, st.parsed, st.trail, st.raw);
    for (auto& r : st.reasons) fprintf(stderr, "  verbatim: %ld x %s\n", r.second, r.first.c_str());
  }
  return 0;
}

int decompress(const char* in_name, const char* out_name, const Options& opt) {
  FILE* fi = strcmp(in_name, "-") ? fopen(in_name, "rb") : stdin;
  if (!fi) {
    fprintf(stderr, "h264p: cannot open %s: %s\n", in_name, strerror(errno));
    return 1;
  }
  FILE* fo = open_out(out_name);
  RowReader in(fi);
  Context ctx;
  std::vector<uint8_t> buf;
  long nals = 0;
  auto flush = [&]() {
    fwrite(buf.data(), 1, buf.size(), fo);
    buf.clear();
  };
  try {
    const Row* first = in.peek();
    if (!first || !first->is(kMagic) || first->size() != 2 || first->num(1) != kVersion)
      throw TsvError("not an h264p file (version " + std::to_string(kVersion) + ")");
    in.take();
    bool eof = false;
    while (!eof) {
      const Row& r = in.take();
      if (r.is("junk")) {
        std::vector<uint8_t> b = from_hex(r, 1);
        buf.insert(buf.end(), b.begin(), b.end());
      } else if (r.is("nal") || r.is("nalx")) {
        nals++;
        int64_t zeros = r.num(1);
        if (zeros < 0 || zeros > (1 << 30)) r.fail("bad zero count");
        buf.insert(buf.end(), (size_t)zeros, 0);
        buf.push_back(0);
        buf.push_back(0);
        buf.push_back(1);
        if (r.is("nalx")) {
          std::vector<uint8_t> b = from_hex(r, 2);
          buf.insert(buf.end(), b.begin(), b.end());
        } else {
          if (r.size() != 4) r.fail("expected 3 values");
          int64_t ref_idc = r.num(2), type = r.num(3);
          if (ref_idc < 0 || ref_idc > 3 || type < 0 || type > 31) r.fail("bad NAL unit header");
          buf.push_back((uint8_t)(ref_idc << 5 | type));
          const Row* p = in.peek();
          if (p && p->is("hex")) {
            std::vector<uint8_t> b = from_hex(in.take(), 1);
            buf.insert(buf.end(), b.begin(), b.end());
            update_params((int)type, unescape(b.data(), b.size()), ctx);
          } else {
            std::vector<uint8_t> e = escape(write_nal((int)type, (int)ref_idc, ctx, in));
            buf.insert(buf.end(), e.begin(), e.end());
          }
        }
      } else if (r.is("eof")) {
        int64_t n = r.num(1);
        if (n < 0 || n > (1 << 30)) r.fail("bad zero count");
        buf.insert(buf.end(), (size_t)n, 0);
        eof = true;
      } else {
        r.fail("unexpected row");
      }
      if (buf.size() > (1 << 22)) flush();
    }
    if (const Row* extra = in.peek()) extra->fail("rows after eof");
  } catch (const TsvError& e) {
    fprintf(stderr, "h264p: %s: %s\n", in_name, e.what());
    return 1;
  }
  flush();
  if (fi != stdin) fclose(fi);
  if (ferror(fo) || (fo != stdout && fclose(fo))) {
    fprintf(stderr, "h264p: write error\n");
    return 1;
  }
  if (opt.verbose) fprintf(stderr, "%ld NAL units\n", nals);
  return 0;
}

void usage() {
  fprintf(stderr,
          "usage: h264p c|d [options] input output\n"
          "  c  dump an H.264 Annex B stream to a TSV file\n"
          "  d  restore the stream from the TSV file\n"
          "options:\n"
          "  -v  print statistics\n"
          "  -s  strict: fail if a NAL unit of a supported type has to be stored verbatim\n"
          "input and output can be - for stdin and stdout\n");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2 || (strcmp(argv[1], "c") && strcmp(argv[1], "d"))) {
    usage();
    return 2;
  }
#ifdef _WIN32
  _setmode(_fileno(stdin), _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  Options opt;
  std::vector<const char*> files;
  for (int i = 2; i < argc; i++) {
    if (argv[i][0] == '-' && argv[i][1]) {
      for (const char* p = argv[i] + 1; *p; p++) {
        if (*p == 'v')
          opt.verbose = true;
        else if (*p == 's')
          opt.strict = true;
        else {
          usage();
          return 2;
        }
      }
    } else {
      files.push_back(argv[i]);
    }
  }
  if (files.size() != 2) {
    usage();
    return 2;
  }
  return argv[1][0] == 'c' ? compress(files[0], files[1], opt) : decompress(files[0], files[1], opt);
}
