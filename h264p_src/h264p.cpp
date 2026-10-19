// h264p: a lossless compressor of H.264 Annex B streams (c, d): the syntax
// of the stream, with the entropy coding removed, is coded with a context
// mixing model (model.inc) and coded back by the decompressor. The same
// syntax can be dumped to a TSV file (t) and the identical stream restored
// from it (r). This is the only module: the other sources are included here.
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <sys/stat.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "bits.inc"
#include "rows.inc"
#include "tables.inc"
#include "cabac.inc"
#include "cavlc.inc"
#include "chan.inc"
#include "syntax.inc"
#include "slice.inc"
#include "tsv.inc"
#include "log.inc"
#include "cmbase.inc"
#include "rcio.inc"
#include "rc.inc"
#include "cm.inc"
#include "mapping.inc"
#include "fam.inc"
#include "recon.inc"
#include "model.inc"

using namespace h264p;

namespace {

const char kMagic[] = "h264p";  // TSV files
const int kVersion = 1;
const char kCmMagic[] = "h264c";  // compressed streams, followed by the version and level bytes
const int kCmVersion = 5;
// The CRC-32 at the end is coded XORed with this: a decoder fed past the
// end of its input decodes zeros, and 0 is the CRC-32 of an empty stream.
const uint32_t kCrcXor = 0x68323634;

// The levels of this build (h264p_levels.md section 4): every level, or the
// one a tuning build has (mk.sh); c without -L takes the first
#if H264P_LV(9)
#define H264P_L9(X) X(9)
#else
#define H264P_L9(X)
#endif
#if H264P_LV(1)
#define H264P_L1(X) X(1)
#else
#define H264P_L1(X)
#endif
#define H264P_LEVELS(X) H264P_L9(X) H264P_L1(X)
#define H264P_FIRST(n) n,
const int kLevels[] = {H264P_LEVELS(H264P_FIRST)};

struct Options {
  bool verbose = false;
  bool strict = false;
  int level = kLevels[0];  // c's model: -L
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
struct PadDetector {
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

// Parses the RBSP of a NAL unit, handing the values to a channel; parameter
// sets are stored in the context. Throws ParseError.
void read_nal(int type, int ref_idc, const std::vector<uint8_t>& rbsp, Context& ctx, Chan& ch) {
  BitReader br(rbsp.data(), rbsp.size());
  Syn s(&br, &ch);
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
      read_slice_data(h, ctx.mbs, br, ch);
      break;
    }
    default:
      throw ParseError("unsupported NAL unit type");
  }
}

// Writes the syntax of a NAL unit with the values of a channel, up to and
// including the rbsp_stop_one_bit. Returns x264's padding bit for the end
// of CABAC slice data, else -1. Throws what the channel throws.
int write_syntax(int type, int ref_idc, Context& ctx, Chan& ch, BitWriter& bw) {
  Syn s(&ch, &bw);
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
      write_slice_data(h, ctx.mbs, ch, bw);  // with the stop bit
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

// How the RBSP of a NAL unit ends after the rbsp_stop_one_bit, when not with
// the expected alignment bits (e.g. from yet another CABAC flushing
// procedure): the last `drop` bits written are replaced by `bits`. Then come
// the cabac_zero_words.
struct Trailer {
  bool irregular = false;
  int64_t drop = 0;
  std::string bits;  // of '0' and '1'
  int64_t czw = 0;
};

// The rest of the RBSP after the rbsp_stop_one_bit, as the trailer says.
void apply_trailer(const Trailer& t, BitWriter& bw, int pad) {
  if (t.irregular) {
    bw.truncate(bw.pos() - (size_t)t.drop);
    for (char c : t.bits) bw.bit(c - '0');
  } else {
    align(bw, pad);
  }
  bw.append(2 * (size_t)t.czw, 0);
}

int get_bit(const std::vector<uint8_t>& v, size_t i) { return v[i >> 3] >> (7 - (i & 7)) & 1; }

// Compares the RBSP written up to and including the stop bit (bw; pad as
// write_syntax returned it) with the original one, and finds the trailer
// that makes them equal. False when there is none. obs is the bit where x264
// puts its padding bit in a CABAC slice, -1 if there is none.
bool find_trailer(const BitWriter& bw, int pad, const std::vector<uint8_t>& rbsp, Trailer& t, std::string& why,
                  int& obs) {
  t = Trailer();
  size_t pbits = bw.pos();
  // the last bit of the byte with the stop bit, unless it is the stop bit
  obs = -1;
  if (pad >= 0 && (pbits & 7) && (pbits - 1) / 8 < rbsp.size()) obs = rbsp[(pbits - 1) / 8] & 1;
  BitWriter o = bw;
  align(o, pad);
  if (o.data() == rbsp) return true;
  // the end differs: cabac_zero_words, or trailing bits that are not the
  // expected ones
  size_t zeros = 0;
  while (zeros < rbsp.size() && rbsp[rbsp.size() - 1 - zeros] == 0) zeros++;
  t.czw = (int64_t)(zeros / 2);
  if (t.czw > (1 << 26)) {
    why = "too many cabac_zero_words";
    return false;
  }
  std::vector<uint8_t> r(rbsp.begin(), rbsp.end() - 2 * (size_t)t.czw);
  if (o.data() != r) {
    // compare our bits up to and including the stop bit
    size_t rbits = r.size() * 8, d = 0;
    while (d < pbits && d < rbits && bw.at(d) == get_bit(r, d)) d++;
    if (pbits - d > 64 || rbits - d > 4096) {
      why = "re-encoding differs at bit " + std::to_string(d) + " of " + std::to_string(rbits);
      return false;
    }
    t.irregular = true;
    t.drop = (int64_t)(pbits - d);
    for (size_t i = d; i < rbits; i++) t.bits += (char)('0' + get_bit(r, i));
  }
  BitWriter chk = bw;
  apply_trailer(t, chk, pad);
  if (chk.data() != rbsp) {
    why = "re-encoding differs";
    return false;
  }
  return true;
}

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

// SPS and PPS stored verbatim still define parameter sets.
void update_params(int type, const std::vector<uint8_t>& rbsp, Context& ctx) {
  if (type != 7 && type != 8) return;
  try {
    LogChan ch;
    read_nal(type, 0, rbsp, ctx, ch);
  } catch (const ParseError&) {
  }
}

// Converts the payload of one NAL unit (after the header byte) to rows, or
// returns false when it has to be stored verbatim. obs: see find_trailer.
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
    TsvChan ch(&out);
    read_nal(type, ref_idc, rbsp, ctx, ch);
  } catch (const ParseError& e) {
    why = e.what();
    out.truncate(start);
    return false;
  }
  try {
    std::string text = out.text().substr(start);
    RowReader in(text.data(), text.size());
    BitWriter bw;
    TsvChan ch(&in);
    int pad = write_syntax(type, ref_idc, ctx, ch, bw);
    if (!in.at_end()) throw TsvError("rows left over");
    Trailer t;
    if (!find_trailer(bw, pad, rbsp, t, why, obs)) {
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
  } catch (const std::exception& e) {
    why = std::string("re-encoding failed: ") + e.what();
    out.truncate(start);
    return false;
  }
}

// The compressor's dry run of the payload of a NAL unit: parses it into the
// log, writes it back from the log and finds how it ends. False when it has
// to be stored verbatim. obs: see find_trailer.
bool dry_run(int type, int ref_idc, const uint8_t* p, size_t n, Context& ctx, LogChan& log, Trailer& t,
             std::string& why, int& obs) {
  std::vector<uint8_t> rbsp = unescape(p, n);
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
  log.clear();
  try {
    read_nal(type, ref_idc, rbsp, ctx, log);
  } catch (const ParseError& e) {
    why = e.what();
    return false;
  }
  try {
    log.rewind();
    BitWriter bw;
    int pad = write_syntax(type, ref_idc, ctx, log, bw);
    if (!log.at_end()) throw TsvError("values left over");
    return find_trailer(bw, pad, rbsp, t, why, obs);
  } catch (const std::exception& e) {
    why = std::string("re-encoding failed: ") + e.what();
    return false;
  }
}

// Splits an Annex B byte stream into NAL units while reading it.
struct AnnexBReader {
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

// The exit status of a failed c or t, which leaves no output file behind
// (only a regular file: the output may be /dev/null).
int fail_out(FILE* f, const char* name) {
  if (f != stdout) {
    fclose(f);
    struct stat st;
    if (stat(name, &st) == 0 && S_ISREG(st.st_mode)) remove(name);
  }
  return 1;
}

int tsv_dump(const char* in_name, const char* out_name, const Options& opt) {
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
        return fail_out(f, out_name);
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
    return fail_out(f, out_name);
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

int tsv_restore(const char* in_name, const char* out_name, const Options& opt) {
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
    if (!buf.empty()) fwrite(buf.data(), 1, buf.size(), fo);
    buf.clear();
  };
  auto put_zeros = [&](int64_t n) {
    for (; n > 0; n -= 1 << 20) {
      buf.insert(buf.end(), (size_t)std::min<int64_t>(n, 1 << 20), 0);
      if (buf.size() > (1 << 22)) flush();
    }
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
        if (zeros < 0 || zeros > ((int64_t)1 << 40)) r.fail("bad zero count");
        put_zeros(zeros);
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
        if (n < 0 || n > ((int64_t)1 << 40)) r.fail("bad zero count");
        put_zeros(n);
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

// CRC-32 (IEEE) of the stream, coded at the end of a compressed one
struct Crc32 {
  Crc32() {
    for (uint32_t i = 0; i < 256; i++) {
      uint32_t x = i;
      for (int k = 0; k < 8; k++) x = x & 1 ? 0xEDB88320u ^ (x >> 1) : x >> 1;
      t[i] = x;
    }
  }
  void add(const uint8_t* p, size_t n) {
    for (size_t i = 0; i < n; i++) c = t[(c ^ p[i]) & 255] ^ (c >> 8);
  }
  void add(const std::vector<uint8_t>& v) { add(v.data(), v.size()); }
  void zeros(size_t n) {
    for (size_t i = 0; i < n; i++) c = t[c & 255] ^ (c >> 8);
  }
  uint32_t value() const { return ~c; }
  uint32_t t[256];
  uint32_t c = 0xFFFFFFFFu;
};

void cm_report(u64 in_bytes, u64 out_bytes, u64 tables) {
  double tot = 0;
  for (int i = 0; i < STG_N; i++) tot += tc_bits[i];
  fprintf(stderr, "%" PRIu64 " -> %" PRIu64 " bytes (%.2f%%), %" PRIu64 " MB of model tables\n", in_bytes, out_bytes,
          in_bytes ? 100.0 * out_bytes / in_bytes : 0.0, tables >> 20);
  for (int i = 0; i < STG_N; i++) {
    if (!tc_syms[i]) continue;
    fprintf(stderr, "  %-13s %11.0f bytes %6.2f%%  %10" PRIu64 " decisions or values, %.3f bits each\n", kStage[i],
            tc_bits[i] / 8, tot > 0 ? 100.0 * tc_bits[i] / tot : 0.0, tc_syms[i], tc_bits[i] / (double)tc_syms[i]);
  }
}

// The compressed stream: the magic and version, then one range coded run of
// decisions. The bytes before the first start code (junk), then for each
// NAL unit: that there is one (F_MORE), its type, nal_ref_idc and the zero
// bytes before its start code, a change of x264's padding bits (slices),
// whether it is stored verbatim (a type the walk knows); then its syntax
// through the model and the trailer, or its length and bytes. At the end,
// the zero bytes after the last NAL unit and the CRC-32 of the stream.
template <class LV>
int compress_lv(const char* in_name, const char* out_name, const Options& opt) {
  FILE* fi = strcmp(in_name, "-") ? fopen(in_name, "rb") : stdin;
  if (!fi) {
    fprintf(stderr, "h264p: cannot open %s: %s\n", in_name, strerror(errno));
    return 1;
  }
  FILE* fo = open_out(out_name);
  fwrite(kCmMagic, 1, sizeof kCmMagic - 1, fo);
  fputc(kCmVersion, fo);
  fputc(LV::id, fo);
  u64 in_bytes = 0, out_bytes = sizeof kCmMagic + 1;
  std::unique_ptr<Model<0, LV>> mp(new Model<0, LV>());
  Model<0, LV>& m = *mp;
  auto& rc = tc_rc<0>::rc;
  Stats st;
  try {
    m.init();
    tc_verbose = opt.verbose;
    rc.rc_Init();
    auto flush = [&]() {
      if (!rc.buf.empty()) fwrite(rc.buf.data(), 1, rc.buf.size(), fo);
      out_bytes += rc.buf.size();
      rc.buf.clear();
    };
    // A NAL unit's work that needs no model (its parsing, and writing it back
    // to see that it comes out the same) runs before the model's, on a
    // context of its own (cf); the model's context (cb) takes the same
    // parameter sets and padding as it reaches each NAL unit
    struct NalItem {
      size_t zeros = 0;
      std::vector<uint8_t> nal;
      bool bad = false;  // empty, or forbidden_zero_bit set
      int type = 0, ref_idc = 0;
      bool slice = false, newpic = false, ok = false, padchg = false, padon = false;
      int64_t padframe = 0;
      Trailer t;
      std::string why;
      LogChan log;
    };
    Context cf, cb;
    PadDetector det;
    Crc32 crc;
    AnnexBReader rd(fi);
    std::vector<uint8_t> nal;
    size_t zeros;
    bool more = rd.next(zeros, nal);
    const std::vector<uint8_t>& junk = rd.junk();
    m.nal(F_JUNK, 0, (i64)junk.size());
    for (uint8_t b : junk) m.raw(b);
    crc.add(junk);
    u64 in_nals = junk.size();  // (the bytes of the NAL units, counted ahead)
    auto fill = [&](NalItem& it) {
      static const uint8_t sc[3] = {0, 0, 1};
      crc.zeros(it.zeros);
      crc.add(sc, 3);
      crc.add(it.nal);
      in_nals += it.zeros + 3 + it.nal.size();
      it.bad = it.nal.empty() || (it.nal[0] & 0x80);
      if (it.bad) return;
      it.ref_idc = it.nal[0] >> 5 & 3, it.type = it.nal[0] & 31;
      const uint8_t* payload = it.nal.data() + 1;
      size_t len = it.nal.size() - 1;
      it.slice = it.type == 1 || it.type == 5;
      it.newpic = starts_picture(it.type, payload, len);
      if (it.slice) det.slice(it.newpic);
      if (it.type == 7 || (it.type == 6 && is_x264_sei(payload, len))) det.restart();
      int obs;
      it.ok = dry_run(it.type, it.ref_idc, payload, len, cf, it.log, it.t, it.why, obs);
      if (it.ok && obs >= 0 && det.observe(obs)) {
        // once more, with padding bits that predict this one
        it.padchg = true;
        int64_t frame = det.frame();
        if (frame < 0)
          cf.pad.on = false;
        else
          cf.pad.set(frame);
        it.ok = dry_run(it.type, it.ref_idc, payload, len, cf, it.log, it.t, it.why, obs);
      }
      it.padon = cf.pad.on, it.padframe = cf.pad.frame;
      if (it.slice) cf.pad.next_slice(it.newpic);
    };
    int prev_type = 0;
    // (false: a NAL unit -s would not have verbatim)
    auto use = [&](NalItem& it) {
      st.nals++;
      m.nal(F_MORE, 0, 1);
      if (it.bad) {
        m.nal(F_TYPE, prev_type, 32);
        m.nal(F_ZEROS, 32, (i64)it.zeros);
        m.nal(F_LEN, 32, (i64)it.nal.size());
        for (uint8_t b : it.nal) m.raw(b);
        st.raw++;
        st.reasons["empty or forbidden_zero_bit"]++;
        return true;
      }
      int type = it.type, ref_idc = it.ref_idc;
      m.nal(F_TYPE, prev_type, type);
      m.nal(F_REF, type, ref_idc);
      m.nal(F_ZEROS, type, (i64)it.zeros);
      prev_type = type;
      const uint8_t* payload = it.nal.data() + 1;
      size_t len = it.nal.size() - 1;
      if (it.padchg) {
        if (!it.padon)
          cb.pad.on = false;
        else
          cb.pad.set(it.padframe);
        st.pad++;
      }
      if (it.slice) {
        m.nal(F_PADCHG, 0, it.padchg);
        if (it.padchg) {
          m.nal(F_PADON, 0, cb.pad.on);
          if (cb.pad.on) m.nal(F_PADFRAME, 0, cb.pad.frame);
        }
      }
      if (parsed_type(type)) m.nal(F_VERB, type, !it.ok);
      if (it.ok) {
        const Trailer& t = it.t;
        it.log.rewind();
        ModelChan<0, LV> ch(m, &it.log);
        m.nal_begin(type, ref_idc);
        BitWriter bw;
        write_syntax(type, ref_idc, cb, ch, bw);
        if (!it.log.at_end()) cm_fail("internal: values left over");
        m.nal(F_TRAIL, type, t.irregular);
        if (t.irregular) {
          m.nal(F_DROP, 0, t.drop);
          m.nal(F_NBITS, 0, (i64)t.bits.size());
          for (char c : t.bits) m.nal(F_TBIT, 0, c - '0');
        }
        m.nal(F_CZW, type, t.czw);
        st.parsed++;
        st.trail += t.irregular;
      } else {
        // (an SPS or PPS stored verbatim may still define a parameter set,
        // as it did ahead)
        if (type == 7 || type == 8) update_params(type, unescape(payload, len), cb);
        m.nal(F_LEN, type, (i64)len);
        for (size_t i = 0; i < len; i++) m.raw(payload[i]);
        st.raw++;
        st.reasons[it.why]++;
        if (opt.verbose && parsed_type(type))
          fprintf(stderr, "h264p: NAL unit %ld (type %d) stored verbatim: %s\n", st.nals - 1, type, it.why.c_str());
        if (opt.strict && parsed_type(type)) {
          fprintf(stderr, "h264p: NAL unit %ld (type %d) cannot be converted: %s\n", st.nals - 1, type, it.why.c_str());
          return false;
        }
      }
      if (it.slice) cb.pad.next_slice(it.newpic);
      if (rc.buf.size() > (1 << 20)) flush();
      return true;
    };
    for (; more; more = rd.next(zeros, nal)) {
      NalItem it;
      it.zeros = zeros;
      it.nal.swap(nal);
      fill(it);
      if (!use(it)) return fail_out(fo, out_name);
    }
    in_bytes += in_nals;
    if (rd.error()) {
      fprintf(stderr, "h264p: read error\n");
      return fail_out(fo, out_name);
    }
    if (fi != stdin) fclose(fi);
    m.nal(F_MORE, 0, 0);
    m.nal(F_EOFZ, 0, (i64)rd.eof_zeros());
    crc.zeros(rd.eof_zeros());
    in_bytes += rd.eof_zeros();
    m.nal(F_CRC, 0, crc.value() ^ kCrcXor);
    rc.rc_Quit();
    flush();
  } catch (const std::exception& e) {  // CmError, or no memory for the model
    fprintf(stderr, "h264p: %s: %s\n", in_name, e.what());
    return fail_out(fo, out_name);
  }
  if (ferror(fo) || (fo != stdout && fclose(fo))) {
    fprintf(stderr, "h264p: write error\n");
    return 1;
  }
  if (opt.verbose) {
    fprintf(stderr, "%ld NAL units: %ld coded (%ld with irregular trailing bits), %ld stored verbatim\n", st.nals,
            st.parsed, st.trail, st.raw);
    if (st.pad) fprintf(stderr, "%ld changes of x264's padding bits\n", st.pad);
    for (auto& r : st.reasons) fprintf(stderr, "  verbatim: %ld x %s\n", r.second, r.first.c_str());
    cm_report(in_bytes, out_bytes, m.table_bytes());
  }
  return 0;
}

// a count coded in a compressed stream
size_t cm_count(i64 v, i64 lim) {
  if (v < 0 || v > lim) cm_fail("corrupt stream: a count of %" PRId64, v);
  return (size_t)v;
}

template <class LV>
int decompress_lv(FILE* fi, const char* in_name, const char* out_name, const Options& opt) {
  FILE* fo = open_out(out_name);
  std::unique_ptr<Model<1, LV>> mp(new Model<1, LV>());
  Model<1, LV>& m = *mp;
  auto& rc = tc_rc<1>::rc;
  std::vector<uint8_t> buf;
  Crc32 crc;
  long nals = 0;
  auto flush = [&]() {
    crc.add(buf);
    if (!buf.empty()) fwrite(buf.data(), 1, buf.size(), fo);
    buf.clear();
  };
  auto put_zeros = [&](size_t n) {
    for (; n; n -= std::min(n, (size_t)1 << 20)) {
      buf.insert(buf.end(), std::min(n, (size_t)1 << 20), 0);
      if (buf.size() > (1 << 22)) flush();
    }
  };
  try {
    m.init();
    tc_verbose = opt.verbose;
    rc.f = fi;
    rc.rc_Init();
    Context ctx;
    for (size_t n = cm_count(m.nal(F_JUNK, 0, 0), (i64)1 << 40); n; n--) buf.push_back((uint8_t)m.raw(0));
    int prev_type = 0;
    while (m.nal(F_MORE, 0, 0)) {
      nals++;
      int type = (int)cm_count(m.nal(F_TYPE, prev_type, 0), 32);
      if (type == 32) {
        put_zeros(cm_count(m.nal(F_ZEROS, 32, 0), (i64)1 << 40));
        buf.push_back(0);
        buf.push_back(0);
        buf.push_back(1);
        for (size_t n = cm_count(m.nal(F_LEN, 32, 0), (i64)1 << 40); n; n--) buf.push_back((uint8_t)m.raw(0));
        continue;
      }
      int ref_idc = (int)cm_count(m.nal(F_REF, type, 0), 3);
      put_zeros(cm_count(m.nal(F_ZEROS, type, 0), (i64)1 << 40));
      buf.push_back(0);
      buf.push_back(0);
      buf.push_back(1);
      buf.push_back((uint8_t)(ref_idc << 5 | type));
      prev_type = type;
      size_t start = buf.size();
      bool slice = type == 1 || type == 5;
      if (slice && m.nal(F_PADCHG, 0, 0)) {
        if (m.nal(F_PADON, 0, 0))
          ctx.pad.set((int64_t)cm_count(m.nal(F_PADFRAME, 0, 0), INT32_MAX));
        else
          ctx.pad.on = false;
      }
      if (!parsed_type(type) || m.nal(F_VERB, type, 0)) {
        std::vector<uint8_t> b;  // grown as the bytes come: the length may be corrupt
        for (size_t n = cm_count(m.nal(F_LEN, type, 0), (i64)1 << 40); n; n--) b.push_back((uint8_t)m.raw(0));
        buf.insert(buf.end(), b.begin(), b.end());
        update_params(type, unescape(b.data(), b.size()), ctx);
      } else {
        ModelChan<1, LV> ch(m, nullptr);
        m.nal_begin(type, ref_idc);
        BitWriter bw;
        int pad = write_syntax(type, ref_idc, ctx, ch, bw);
        Trailer t;
        t.irregular = m.nal(F_TRAIL, type, 0) != 0;
        if (t.irregular) {
          t.drop = (int64_t)cm_count(m.nal(F_DROP, 0, 0), (i64)bw.pos());
          t.bits.resize(cm_count(m.nal(F_NBITS, 0, 0), 1 << 16));
          for (auto& c : t.bits) c = (char)('0' + cm_count(m.nal(F_TBIT, 0, 0), 1));
          if ((bw.pos() - (size_t)t.drop + t.bits.size()) & 7) cm_fail("corrupt stream: trailing bits");
        }
        t.czw = (int64_t)cm_count(m.nal(F_CZW, type, 0), 1 << 26);
        apply_trailer(t, bw, pad);
        std::vector<uint8_t> e = escape(bw.data());
        buf.insert(buf.end(), e.begin(), e.end());
      }
      if (slice) ctx.pad.next_slice(starts_picture(type, buf.data() + start, buf.size() - start));
      if (buf.size() > (1 << 22)) flush();
    }
    put_zeros(cm_count(m.nal(F_EOFZ, 0, 0), (i64)1 << 40));
    flush();
    if (m.nal(F_CRC, 0, 0) != (crc.value() ^ kCrcXor)) cm_fail("CRC mismatch: the stream did not decompress correctly");
    // the coder's flush leaves off the last bytes, so the decoder always
    // reads past the end of a whole stream
    if (!rc.over) cm_fail("corrupt stream: data after its end");
  } catch (const std::exception& e) {  // CmError, TsvError
    flush();
    fprintf(stderr, "h264p: %s: %s\n", in_name, e.what());
    return 1;
  }
  if (fi != stdin) fclose(fi);
  if (ferror(fo) || (fo != stdout && fclose(fo))) {
    fprintf(stderr, "h264p: write error\n");
    return 1;
  }
  if (opt.verbose) fprintf(stderr, "%ld NAL units\n", nals);
  return 0;
}

int compress(const char* in_name, const char* out_name, const Options& opt) {
  switch (opt.level) {
#define H264P_C(n) \
  case n:          \
    return compress_lv<Lv##n>(in_name, out_name, opt);
    H264P_LEVELS(H264P_C)
#undef H264P_C
  }
  fprintf(stderr, "h264p: no level -%d in this build\n", opt.level);
  return 2;
}

int decompress(const char* in_name, const char* out_name, const Options& opt) {
  FILE* fi = strcmp(in_name, "-") ? fopen(in_name, "rb") : stdin;
  if (!fi) {
    fprintf(stderr, "h264p: cannot open %s: %s\n", in_name, strerror(errno));
    return 1;
  }
  char head[sizeof kCmMagic + 1];
  if (fread(head, 1, sizeof kCmMagic, fi) != sizeof kCmMagic || memcmp(head, kCmMagic, sizeof kCmMagic - 1)) {
    fprintf(stderr, "h264p: %s: not a stream h264p compressed\n", in_name);
    return 1;
  }
  if (head[sizeof kCmMagic - 1] != kCmVersion) {
    fprintf(stderr, "h264p: %s: compressed by version %d, this is version %d\n", in_name,
            (int)(uint8_t)head[sizeof kCmMagic - 1], kCmVersion);
    return 1;
  }
  int level = fgetc(fi);
  switch (level) {
#define H264P_D(n) \
  case n:          \
    return decompress_lv<Lv##n>(fi, in_name, out_name, opt);
    H264P_LEVELS(H264P_D)
#undef H264P_D
  }
  fprintf(stderr, "h264p: %s: compressed at level -%d, which this build has not\n", in_name, level);
  return 1;
}

void usage() {
  fprintf(stderr,
          "usage: h264p c|d|t|r [options] input output\n"
          "  c  compress an H.264 Annex B stream\n"
          "  d  decompress it\n"
          "  t  dump the stream's syntax to a TSV file\n"
          "  r  restore the stream from the TSV file\n"
          "options:\n"
          "  -v  print statistics\n"
          "  -s  strict (c, t): fail if a NAL unit of a supported type has to be stored verbatim\n"
          "  -L  (c) the model's level, 1 (fastest) to 9 (strongest, the default)\n"
          "input and output can be - for stdin and stdout\n");
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2 || !argv[1][0] || argv[1][1] || !strchr("cdtr", argv[1][0])) {
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
        else if (*p >= '1' && *p <= '9')
          opt.level = *p - '0';
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
  try {
    switch (argv[1][0]) {
      case 'c':
        return compress(files[0], files[1], opt);
      case 'd':
        return decompress(files[0], files[1], opt);
      case 't':
        return tsv_dump(files[0], files[1], opt);
      default:
        return tsv_restore(files[0], files[1], opt);
    }
  } catch (const std::exception& e) {  // no memory
    fprintf(stderr, "h264p: %s\n", e.what());
    return 1;
  }
}
