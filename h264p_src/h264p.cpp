// h264p: dumps the syntax of an H.264 Annex B stream to a TSV file with the
// entropy coding removed (c), and restores the identical stream from it (d).
#include <algorithm>
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
  long nals = 0, parsed = 0, raw = 0, trail = 0, pad = 0;
  std::map<std::string, long> reasons;
};

// x264 ends its CABAC data in its own way: after the rbsp_stop_one_bit and
// the alignment zero bits, it sets the last bit of the byte to bit
// (frame & 31) of 0x35a4e4f5, frame being the number of the picture in
// coding order from the start of the encoding, and it does the same before
// the samples of I_PCM macroblocks. An "x264_padding N" row says that the
// slices that follow do that, the next one being in frame N; restoring then
// writes these bits by default.
struct X264Padding {
  bool on = false;
  bool hold = false;  // set by the row: the next slice is in frame `frame`
  int64_t frame = 0;  // the frame of the last slice

  // the padding bit of the next slice, which starts a picture or not
  int bit(bool newpic) const {
    if (!on) return 0;
    uint64_t f = (uint64_t)frame + (newpic && !hold);
    return 0x35a4e4f5u >> (f & 31) & 1;
  }
  void set(int64_t n) { on = true, hold = true, frame = n; }
  void next_slice(bool newpic) {
    frame += newpic && !hold;
    hold = false;
  }
};

// State shared by all NAL units of a stream.
struct Context {
  ParamSets ps;
  MbStore mbs;
  X264Padding pad;
};

// A slice with first_mb_in_slice 0, whose first bit is 1, starts a picture.
bool starts_picture(int type, const uint8_t* payload, size_t n) {
  return (type == 1 || type == 5) && n && (payload[0] & 0x80);
}

// Decides when compress writes x264_padding rows, from the padding bits it
// observes: keeps the current hypothesis (no padding, or x264's padding in
// one of the 32 phases of the frame number relative to the picture count)
// while it predicts the observed bits. Otherwise it switches to x264's
// padding starting with frame 0 at the last SPS or x264 SEI, or to no
// padding, if that predicted all bits since, else to the hypothesis that
// predicted the longest run of the latest bits.
class PadDetector {
 public:
  PadDetector() {
    for (int64_t& l : last_) l = -1;
  }
  // a slice NAL unit, which starts a picture or not
  void slice(bool newpic) { pic_ += newpic; }
  // an SPS or x264's version SEI: the next picture may start an encoding
  void restart() {
    pref_ = (int)(-(uint64_t)(pic_ + 1) & 31);
    since_ = n_;
  }
  // the padding bit observed in the current slice; true when the
  // hypothesis changes
  bool observe(int bit) {
    for (int p = 0; p < 32; p++)
      if (predict(p) != bit) last_[p] = n_;
    if (bit) last_[kOff] = n_;
    if (last_[cur_] != n_++) return false;
    int best = pref_;
    if (last_[pref_] >= since_) {
      if (last_[kOff] < since_) {
        best = kOff;
      } else {
        if (last_[kOff] < last_[best]) best = kOff;
        for (int p = 0; p < 32; p++)
          if (last_[p] < last_[best]) best = p;
      }
    }
    cur_ = best;
    return true;
  }
  // the value of the x264_padding row for the current slice, -1 for none
  int64_t frame() const {
    if (cur_ == kOff) return -1;
    int64_t f = pic_ + cur_;
    return f < 0 ? f + 32 : f;
  }

 private:
  static const int kOff = 32;
  int predict(int p) const { return 0x35a4e4f5u >> (((uint64_t)pic_ + p) & 31) & 1; }

  int64_t pic_ = -1;  // pictures so far, the first one being 0
  int64_t n_ = 0;     // observations so far
  int64_t last_[33];  // the last observation each hypothesis mispredicted
  int cur_ = kOff;
  int pref_ = 0;       // the phase that makes the picture after the restart frame 0
  int64_t since_ = 0;  // the first observation after the restart
};

// x264's version SEI: user_data_unregistered with its UUID
bool is_x264_sei(const uint8_t* p, size_t n) {
  static const uint8_t uuid[16] = {0xdc, 0x45, 0xe9, 0xbd, 0xe6, 0xd9, 0x48, 0xb7,
                                   0x96, 0x2c, 0xd8, 0x20, 0xd9, 0x23, 0xee, 0xef};
  return std::search(p, p + n, uuid, uuid + 16) != p + n;
}

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

bool parsed_type(int type) { return (type >= 5 && type <= 12) || type == 1; }

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
    case 12:
      code_filler(s);
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
      h.pad = ctx.pad.bit(h.first_mb == 0);
      read_slice_data(h, ctx.mbs, br, out);
      break;
    }
    default:
      throw ParseError("unsupported NAL unit type");
  }
}

// Writes the syntax of a NAL unit from rows, up to and including the
// rbsp_stop_one_bit. Returns x264's padding bit for the end of CABAC slice
// data, else -1. Throws TsvError.
int write_syntax(int type, int ref_idc, Context& ctx, RowReader& in, BitWriter& bw) {
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
    case 12:
      code_filler(s);
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
      h.pad = ctx.pad.bit(h.first_mb == 0);
      write_slice_data(h, ctx.mbs, in, bw);  // with the stop bit
      return h.pps->entropy_coding_mode_flag ? h.pad : -1;
    }
    default:
      throw TsvError("NAL unit type " + std::to_string(type) + " needs a hex row");
  }
  if (trailing) bw.bit(1);  // rbsp_stop_one_bit
  return -1;
}

// The alignment zero bits, the last one being x264's padding bit if pad is 1.
void align(BitWriter& bw, int pad) { bw.put(pad > 0, (8 - (int)(bw.pos() & 7)) & 7); }

// The rest of the RBSP: the alignment bits, or what a "trail" row says
// instead, and the cabac_zero_words of a "czw" row.
void write_trailer(RowReader& in, BitWriter& bw, int pad) {
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
    align(bw, pad);
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
  int pad = write_syntax(type, ref_idc, ctx, in, bw);
  write_trailer(in, bw, pad);
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
// returns false when it has to be stored verbatim. obs is the bit where x264
// puts its padding bit in a CABAC slice, -1 if there is none.
bool convert_nal(int type, int ref_idc, const uint8_t* p, size_t n, Context& ctx, RowWriter& out,
                 std::string& why, bool& trail, int& obs) {
  std::vector<uint8_t> rbsp = unescape(p, n);
  trail = false;
  obs = -1;
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
    int pad = write_syntax(type, ref_idc, ctx, in, bw);
    if (!in.at_end()) throw TsvError("rows left over");
    size_t pbits = bw.pos();
    // the last bit of the byte with the stop bit, unless it is the stop bit
    if (pad >= 0 && (pbits & 7) && (pbits - 1) / 8 < rbsp.size()) obs = rbsp[(pbits - 1) / 8] & 1;
    BitWriter o = bw;
    align(o, pad);
    if (o.data() == rbsp) return true;
    // the end differs: cabac_zero_words, or trailing bits that are not
    // the expected ones (e.g. from yet another CABAC flushing procedure)
    size_t zeros = 0;
    while (zeros < rbsp.size() && rbsp[rbsp.size() - 1 - zeros] == 0) zeros++;
    size_t czw = zeros / 2;
    std::vector<uint8_t> r(rbsp.begin(), rbsp.end() - 2 * czw);
    RowWriter tail;
    if (o.data() != r) {
      // compare our bits up to and including the stop bit
      size_t rbits = r.size() * 8, d = 0;
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
    write_trailer(in2, bw, pad);
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
  PadDetector det;
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
    const uint8_t* payload = &data[p + 1];
    size_t len = e - p - 1;
    bool newpic = starts_picture(type, payload, len);
    if (type == 1 || type == 5) det.slice(newpic);
    if (type == 7 || (type == 6 && is_x264_sei(payload, len))) det.restart();
    size_t mark = out.size();
    auto nal_row = [&]() {
      out.begin("nal");
      out.add((int64_t)zeros);
      out.add(ref_idc);
      out.add(type);
      out.end();
    };
    nal_row();
    std::string why;
    bool trail;
    int obs;
    bool ok = convert_nal(type, ref_idc, payload, len, ctx, out, why, trail, obs);
    if (ok && obs >= 0 && det.observe(obs)) {
      // once more, after an x264_padding row that predicts the bit
      out.truncate(mark);
      int64_t frame = det.frame();
      out.begin("x264_padding");
      if (frame < 0) {
        out.add_absent();
        ctx.pad.on = false;
      } else {
        out.add(frame);
        ctx.pad.set(frame);
      }
      out.end();
      st.pad++;
      nal_row();
      ok = convert_nal(type, ref_idc, payload, len, ctx, out, why, trail, obs);
    }
    if (type == 1 || type == 5) ctx.pad.next_slice(newpic);
    if (ok) {
      st.parsed++;
      st.trail += trail;
    } else {
      out.begin("hex");
      out.add(to_hex(payload, len));
      out.end();
      st.raw++;
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
    if (st.pad) fprintf(stderr, "%ld x264_padding rows\n", st.pad);
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
          size_t start = buf.size();
          const Row* p = in.peek();
          if (p && p->is("hex")) {
            std::vector<uint8_t> b = from_hex(in.take(), 1);
            buf.insert(buf.end(), b.begin(), b.end());
            update_params((int)type, unescape(b.data(), b.size()), ctx);
          } else {
            std::vector<uint8_t> e = escape(write_nal((int)type, (int)ref_idc, ctx, in));
            buf.insert(buf.end(), e.begin(), e.end());
          }
          if (type == 1 || type == 5)
            ctx.pad.next_slice(starts_picture((int)type, buf.data() + start, buf.size() - start));
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
        if (n < 0 || n > (1 << 30)) r.fail("bad zero count");
        buf.insert(buf.end(), (size_t)n, 0);
        eof = true;
      } else {
        r.fail("unexpected row");
      }
      if (buf.size() > (1 << 22)) flush();
    }
    if (const Row* extra = in.peek()) extra->fail("rows after eof");
  } catch (const std::exception& e) {  // TsvError, or an inconsistency only found when writing
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
