// h264p: a lossless compressor of H.264 Annex B streams (c, d): the syntax
// of the stream, with the entropy coding removed, is coded with a context
// mixing model (model.inc) and coded back by the decompressor; both run as
// coroutines, streaming (coro.inc). The same syntax can be dumped to a TSV
// file (t) and the identical stream restored from it (r). This is the only
// module: the other sources are included here.
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cinttypes>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
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
#include <windows.h>
#define PSAPI_VERSION 2  // (GetProcessMemoryInfo from kernel32)
#include <psapi.h>
#else
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>
#endif
#if defined(__AVX2__)
#include <immintrin.h>
#endif

// The stack-copying coroutines of Lib3 (unchanged from mtwrap's), which
// h264p's compressor and decompressor are (coro.inc), and the library's
// drivers of them: CoroFileProc on files, CoroutinePair for two stages.
// Before h264p's own sources, which see its INLINE and NOINLINE, and after
// the system headers, which its setjmp and longjmp macros must not reach.
#ifndef FILE_API_STD
#define FILE_API_STD  // (its stdio file API on Windows too, as h264p had)
#endif
#include "Lib3/common.inc"
#include "Lib3/coro3b.inc"
#include "Lib3/file_api.inc"
#include "Lib3/coro_fhp2.inc"
#include "Lib3/coro2pair.inc"
// MTwrap (unchanged from mtwrap's, as is Lib3's win_shim.inc it includes:
// build with -I Lib3, and with threads, -pthread), which runs coroutines on
// threads, and h264split's splitter (rzwrap_h264's), which cuts a stream
// into the parts it deals to them (mt.inc)
#include "MTwrap_v3/MTwrap.inc"
#include "h264split.inc"

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
#include "famvars.inc"
#include "recon.inc"
#include "model.inc"

// (h264p's names stay in its namespace: Lib3's global byte, uint and qword,
// and whatever more of the library a program that includes this file has
// after it, MTwrap's included, see only theirs)
namespace h264p {
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
#if H264P_LV(7)
#define H264P_L7(X) X(7)
#else
#define H264P_L7(X)
#endif
#if H264P_LV(5)
#define H264P_L5(X) X(5)
#else
#define H264P_L5(X)
#endif
#if H264P_LV(3)
#define H264P_L3(X) X(3)
#else
#define H264P_L3(X)
#endif
#if H264P_LV(1)
#define H264P_L1(X) X(1)
#else
#define H264P_L1(X)
#endif
#define H264P_LEVELS(X) H264P_L9(X) H264P_L7(X) H264P_L5(X) H264P_L3(X) H264P_L1(X)
#define H264P_FIRST(n) n,
const int kLevels[] = {H264P_LEVELS(H264P_FIRST)};

// Progress on stderr, as c, d and x go: a line put over itself every half
// second -- the bytes of the stream (raw) and of the compressed stream
// (packed) done so far, how far through its input the call is, the speed
// (of the stream) and the memory (the most the process has had) -- and at
// the end, how long it all took. When stderr is a terminal, or with -p.
// what() gives the counts; poll() puts a line if one is due, from any
// thread (with -t, the workers', as they code: the main thread waits on
// them), and messages put a newline after the line first: cut(), or for
// an error, which ends the run, stop() (no more lines).
struct Progress {
  typedef std::chrono::steady_clock clk;
  const char* tag = nullptr;  // enc, dec, x; none: no progress
  uint64_t total = 0;         // the input's size (0: not known, no percentage)
  std::function<void(uint64_t& raw, uint64_t& packed, uint64_t& pos)> what;  // pos of total: raw or packed
  clk::time_point t0;
  std::atomic<int64_t> last{0};  // the last line, ns after t0
  std::atomic<int> len{0};       // its length (a shorter one is padded over it)
  std::atomic<bool> up{false};   // a line is up, the cursor after it
  std::atomic<bool> halted{false};  // an error was reported: no more lines

  void start(const char* t, uint64_t size) {
    tag = t;
    total = size;
    t0 = clk::now();
  }
  // a line, if half a second has gone by since the last (one thread puts it)
  void poll() {
    if (!tag || !what || halted) return;
    int64_t t = std::chrono::duration_cast<std::chrono::nanoseconds>(clk::now() - t0).count();
    int64_t l = last.load(std::memory_order_relaxed);
    if (t - l < 500000000 || !last.compare_exchange_strong(l, t)) return;
    uint64_t raw, packed, pos;
    what(raw, packed, pos);
    double mbs = raw / (t * 1e-9) / (1 << 20);
    char b[192];
    int n = total ? snprintf(b, sizeof b, "[%s] %" PRIu64 " > %" PRIu64 "  %5.1f%%  %5.2f MB/s  %5.2fM RAM", tag, raw,
                             packed, std::min(100.0, 100.0 * pos / total), mbs, ram())
                  : snprintf(b, sizeof b, "[%s] %" PRIu64 " > %" PRIu64 "  %5.2f MB/s  %5.2fM RAM", tag, raw, packed,
                             mbs, ram());
    show(b, n, "");
  }
  // the end, done (the main thread, no worker left)
  void done(uint64_t raw, uint64_t packed) {
    if (!tag) return;
    double s = std::chrono::duration<double>(clk::now() - t0).count();
    char b[192];
    int n = snprintf(b, sizeof b, "[%s] %" PRIu64 " > %" PRIu64 "  100.0%%  done in %.2fs  %5.2fM RAM", tag, raw, packed,
                     s, ram());
    show(b, n, "\n");
    up = false;
  }
  void cut() {
    if (up.exchange(false)) fputc('\n', stderr);
  }
  void stop() {
    halted = true;
    cut();
  }
  void show(const char* b, int n, const char* end) {
    int l = up ? len.load() : 0;
    fprintf(stderr, "\r%s%*s%s", b, l > n ? l - n : 0, "", end);
    fflush(stderr);
    len = n;
    up = true;
  }
  // the most memory the process has had (its resident set), in MB
  static double ram() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS m;
    return GetProcessMemoryInfo(GetCurrentProcess(), &m, sizeof m) ? m.PeakWorkingSetSize / 1048576.0 : 0.0;
#else
    struct rusage u;
    if (getrusage(RUSAGE_SELF, &u)) return 0;
#ifdef __APPLE__
    return u.ru_maxrss / 1048576.0;  // (bytes)
#else
    return u.ru_maxrss / 1024.0;  // (KB)
#endif
#endif
  }
};
void progress_cut(Progress* p) {
  if (p) p->cut();
}
void progress_stop(Progress* p) {
  if (p) p->stop();
}
void progress_poll(Progress* p) {
  if (p) p->poll();
}

// The level of a compressed stream from its first bytes, n of them read
// (the magic, the version, the level), or -1 (reported) if they are not
// h264p's
int cm_level(const uint8_t* head, size_t n, const char* name, Progress* meter = nullptr) {
  if (n < sizeof kCmMagic || memcmp(head, kCmMagic, sizeof kCmMagic - 1)) {
    progress_stop(meter);
    fprintf(stderr, "h264p: %s: not a stream h264p compressed\n", name);
    return -1;
  }
  if (head[sizeof kCmMagic - 1] != kCmVersion) {
    progress_stop(meter);
    fprintf(stderr, "h264p: %s: compressed by version %d, this is version %d\n", name, (int)head[sizeof kCmMagic - 1],
            kCmVersion);
    return -1;
  }
  if (n == sizeof kCmMagic) {
    progress_stop(meter);
    fprintf(stderr, "h264p: %s: the compressed stream is cut short\n", name);
    return -1;
  }
  return head[sizeof kCmMagic];
}

struct Options {
  bool verbose = false;
  bool strict = false;
  bool progress = false;    // -p: progress even where stderr is not a terminal
  int level = kLevels[0];   // c's model: -L
  int threads = 1;          // c's threads: -t (more than one: the stream in parts, mt.inc)
  uint32_t part = 4 << 20;  // and the size of a part, at least: -c
  Progress* meter = nullptr;  // the call's progress line, which messages cut
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
// sets are stored in the context. Returns x264's padding bit for the end of
// CABAC slice data, else -1, as write_syntax(). Throws ParseError.
int read_nal(int type, int ref_idc, const std::vector<uint8_t>& rbsp, Context& ctx, Chan& ch) {
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
      return h.pps->entropy_coding_mode_flag ? h.pad : -1;
    }
    default:
      throw ParseError("unsupported NAL unit type");
  }
  return -1;
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

// The first nbits bits of an RBSP, as written.
BitWriter rbsp_bits(const std::vector<uint8_t>& rbsp, size_t nbits) {
  BitWriter bw;
  bw.buf_.assign(rbsp.begin(), rbsp.begin() + nbits / 8);
  if (nbits & 7) bw.cur_ = (uint8_t)(rbsp[nbits / 8] >> (8 - (nbits & 7)));
  bw.n_ = nbits;
  return bw;
}

// The compressor's dry run of the payload of a NAL unit: parses it into the
// log, writes it back from the log and finds how it ends. False when it has
// to be stored verbatim. obs: see find_trailer.
//
// With rec, a slice is not written back: it would come out as it is up to
// its stop bit, unless the walk writes other values, bits or bins than it
// reads, which the model pass checks (Chan::rec, checked_bits) when this
// returns true with checked set; rbsp is then the RBSP. With CABAC, the
// bins of the parse go to rec, and each run of CABAC data must end in a 1
// bit (as written, the stop bit; its bits are then what an encoder writes
// for the bins).
bool dry_run(int type, int ref_idc, const uint8_t* p, size_t n, Context& ctx, LogChan& log, Trailer& t,
             std::string& why, int& obs, BinRec* rec, bool& checked, std::vector<uint8_t>& rbsp) {
  rbsp = unescape(p, n);
  obs = -1;
  checked = false;
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
  if (rec) rec->clear();
  log.rec = rec;
  int pad;
  try {
    pad = read_nal(type, ref_idc, rbsp, ctx, log);
  } catch (const ParseError& e) {
    why = e.what();
    return false;
  }
  log.rec = nullptr;
  if (rec && rec->slice && rec->stops) {
    BitWriter bw = rbsp_bits(rbsp, rec->end);
    if (find_trailer(bw, pad, rbsp, t, why, obs)) {
      checked = true;
      return true;
    }
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

// The model pass of a NAL unit that dry_run() did not write back (rec):
// whether what it wrote (bw) is the RBSP up to its stop bit. Without its
// CABAC bins (nobins), it wrote the bits outside the runs of CABAC data, and
// checked the bins against those recorded.
bool checked_bits(const BitWriter& bw, bool nobins, const BinRec& rec, const std::vector<uint8_t>& rbsp) {
  if (rec.bad || !rec.slice || (rec.segs.size() & 1)) return false;
  if (!nobins || rec.segs.empty()) {
    BitWriter r = rbsp_bits(rbsp, rec.end);
    return bw.n_ == r.n_ && bw.cur_ == r.cur_ && bw.buf_ == r.buf_;
  }
  size_t k = 0, from = 0;
  for (size_t i = 0; i < rec.segs.size(); i += 2) {
    for (size_t b = from; b < rec.segs[i]; b++, k++)
      if (k >= bw.pos() || bw.at(k) != get_bit(rbsp, b)) return false;
    from = rec.segs[i + 1];
  }
  return k == bw.pos();
}

// Splits an Annex B byte stream into NAL units while reading it.
struct AnnexBReader {
  explicit AnnexBReader(ByteSource src) : src_(src) {}

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
      buf_.resize(old + (1 << 16));
      size_t n = src_.read(&buf_[old], 1 << 16);
      buf_.resize(old + n);
      if (!n) return false;
    }
  }

  ByteSource src_;
  std::vector<uint8_t> buf_, junk_;
  size_t head_ = 0;  // start of the current NAL unit in buf_
  size_t scan_ = 0;  // where to look for the next start code
  size_t zeros_ = 0, eof_zeros_ = 0;
  bool started_ = false, done_ = false;
};

// a file as a ByteSource (t's input)
size_t file_read(void* f, uint8_t* p, size_t n) { return fread(p, 1, n, (FILE*)f); }
ByteSource file_source(FILE* f) {
  ByteSource s;
  s.fn = file_read;
  s.ctx = f;
  return s;
}

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

  AnnexBReader rd(file_source(fi));
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
  if (ferror(fi)) {
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

void cm_report(const tc_rc<0>& e, u64 in_bytes, u64 out_bytes, u64 tables) {
  double tot = 0;
  for (int i = 0; i < STG_N; i++) tot += e.tc_bits[i];
  fprintf(stderr, "%" PRIu64 " -> %" PRIu64 " bytes (%.2f%%), %" PRIu64 " MB of model tables\n", in_bytes, out_bytes,
          in_bytes ? 100.0 * out_bytes / in_bytes : 0.0, tables >> 20);
  for (int i = 0; i < STG_N; i++) {
    if (!e.tc_syms[i]) continue;
    fprintf(stderr, "  %-13s %11.0f bytes %6.2f%%  %10" PRIu64 " decisions or values, %.3f bits each\n", kStage[i],
            e.tc_bits[i] / 8, tot > 0 ? 100.0 * e.tc_bits[i] / tot : 0.0, e.tc_syms[i],
            e.tc_bits[i] / (double)e.tc_syms[i]);
  }
}

// The compressed stream: the magic, the version and the model's level (c's
// -L; d takes the instance of the model for it), then one range coded run of
// decisions. The bytes before the first start code (junk), then for each
// NAL unit: that there is one (F_MORE), its type, nal_ref_idc and the zero
// bytes before its start code, a change of x264's padding bits (slices),
// whether it is stored verbatim (a type the walk knows); then its syntax
// through the model and the trailer, or its length and bytes. At the end,
// the zero bytes after the last NAL unit and the CRC-32 of the stream.
//
// check: the dry runs leave the CABAC slices they can to the model pass to
// check (dry_run); if one of them does not come out as it is, the model
// has coded it, and compress_stream() returns kRestart.
const int kRestart = -1;

// (thrown by the model pass of a NAL unit that does not come out as it is)
struct Restart {
  long nal;
};

// A NAL unit's work that needs no model (its parsing, and writing it back
// to see that it comes out the same) runs before the model's, on a context
// of its own (cf); the model's context (cb) takes the same parameter sets
// and padding as it reaches each NAL unit
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
  BinRec rec;
  bool checked = false;  // the model pass checks it (dry_run)
  std::vector<uint8_t> rbsp;
  // (one item for every NAL unit: its buffers only grow)
  void reset() {
    bad = slice = newpic = ok = padchg = padon = checked = false;
    type = ref_idc = 0;
    padframe = 0;
    why.clear();
  }
};

// What a compression keeps from one NAL unit to the next besides its
// model, on the heap: the stack of its coroutine (coro.inc), which each
// yield copies, stays small, and Quit() frees all of it wherever the
// coroutine was left
struct CompressState {
  Context cf, cb;
  PadDetector det;
  Crc32 crc;
  Stats st;
  NalItem it;
  std::vector<uint8_t> nal;
  AnnexBReader rd;  // the input
  explicit CompressState(ByteSource in) : rd(in) {}
};

// and a decompression
struct DecompressState {
  Context ctx;
  Crc32 crc;
  std::vector<uint8_t> buf;  // the stream decoded, not yet written
};

// Compresses the stream s.rd reads with the model m, a new one, into its
// output (m is the range coder too, and the coroutine whose output it is,
// coro.inc): 0, 1 after an error (reported), or kRestart (check: a slice
// did not come out as it is; what the output has is to be thrown away)
template <class LV>
int compress_stream(Model<0, LV>& m, CompressState& s, const Options& opt, bool check, const char* name) {
  Stats& st = s.st;
  Context &cf = s.cf, &cb = s.cb;
  PadDetector& det = s.det;
  Crc32& crc = s.crc;
  AnnexBReader& rd = s.rd;
  const uint8_t head[2] = {(uint8_t)kCmVersion, (uint8_t)LV::id};
  m.write((const uint8_t*)kCmMagic, sizeof kCmMagic - 1);
  m.write(head, 2);
  u64 in_bytes = 0;
  try {
    m.init();
    m.tc_verbose = opt.verbose;
    m.rc_Init();
    std::vector<uint8_t>& nal = s.nal;
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
      BinRec* rec = check ? &it.rec : nullptr;
      it.ok = dry_run(it.type, it.ref_idc, payload, len, cf, it.log, it.t, it.why, obs, rec, it.checked, it.rbsp);
      if (it.ok && obs >= 0 && det.observe(obs)) {
        // once more, with padding bits that predict this one
        it.padchg = true;
        int64_t frame = det.frame();
        if (frame < 0)
          cf.pad.on = false;
        else
          cf.pad.set(frame);
        it.ok = dry_run(it.type, it.ref_idc, payload, len, cf, it.log, it.t, it.why, obs, rec, it.checked, it.rbsp);
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
        if (it.checked) {
          // (whatever fails, the NAL unit would not come out as it is)
          ch.rec = &it.rec;
          bool same = false;
          try {
            write_syntax(type, ref_idc, cb, ch, bw);
            same = it.log.at_end() && checked_bits(bw, ch.nobins, it.rec, it.rbsp);
          } catch (const std::exception&) {
          }
          if (!same) throw Restart{st.nals - 1};
        } else {
          write_syntax(type, ref_idc, cb, ch, bw);
          if (!it.log.at_end()) cm_fail("internal: values left over");
        }
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
        if (opt.verbose && parsed_type(type)) {
          progress_cut(opt.meter);
          fprintf(stderr, "h264p: NAL unit %ld (type %d) stored verbatim: %s\n", st.nals - 1, type, it.why.c_str());
        }
        if (opt.strict && parsed_type(type)) {
          progress_stop(opt.meter);
          fprintf(stderr, "h264p: NAL unit %ld (type %d) cannot be converted: %s\n", st.nals - 1, type, it.why.c_str());
          return false;
        }
      }
      if (it.slice) cb.pad.next_slice(it.newpic);
      return true;
    };
    NalItem& it = s.it;
    for (; more; more = rd.next(zeros, nal)) {
      it.reset();
      it.zeros = zeros;
      it.nal.swap(nal);
      fill(it);
      if (!use(it)) return 1;
    }
    in_bytes += in_nals;
    m.nal(F_MORE, 0, 0);
    m.nal(F_EOFZ, 0, (i64)rd.eof_zeros());
    crc.zeros(rd.eof_zeros());
    in_bytes += rd.eof_zeros();
    m.nal(F_CRC, 0, crc.value() ^ kCrcXor);
    m.rc_Quit();
  } catch (const std::exception& e) {  // CmError, or no memory
    progress_stop(opt.meter);
    fprintf(stderr, "h264p: %s: %s\n", name, e.what());
    return 1;
  } catch (const Restart& r) {
    if (opt.verbose) {
      progress_cut(opt.meter);
      fprintf(stderr, "h264p: NAL unit %ld would not come out as it is; compressing again, writing back each\n", r.nal);
    }
    return kRestart;
  }
  if (opt.verbose) {
    progress_cut(opt.meter);
    fprintf(stderr, "%ld NAL units: %ld coded (%ld with irregular trailing bits), %ld stored verbatim\n", st.nals,
            st.parsed, st.trail, st.raw);
    if (st.pad) fprintf(stderr, "%ld changes of x264's padding bits\n", st.pad);
    for (auto& r : st.reasons) fprintf(stderr, "  verbatim: %ld x %s\n", r.second, r.first.c_str());
    cm_report(m, in_bytes, m.n_out, m.table_bytes());
  }
  return 0;
}

// a count coded in a compressed stream
size_t cm_count(i64 v, i64 lim) {
  if (v < 0 || v > lim) cm_fail("corrupt stream: a count of %" PRId64, v);
  return (size_t)v;
}

// Decompresses with the model m, a new one, what is left of its input
// after the magic, the version and the level, into its output (m is the
// range decoder too, and the coroutine whose input it reads, coro.inc): 0,
// or 1 after an error (reported; the output has what was decoded)
template <class LV>
int decompress_stream(Model<1, LV>& m, DecompressState& s, const Options& opt, const char* name) {
  std::vector<uint8_t>& buf = s.buf;
  Crc32& crc = s.crc;
  long nals = 0;
  auto flush = [&]() {
    crc.add(buf);
    m.write(buf.data(), buf.size());
    buf.clear();
  };
  auto put_zeros = [&](size_t n) {
    for (; n; n -= std::min(n, (size_t)1 << 20)) {
      buf.insert(buf.end(), std::min(n, (size_t)1 << 20), 0);
      if (buf.size() > (1 << 16)) flush();
    }
  };
  bool failed = false;  // (reported after the handler: flush() may yield, which none may)
  std::string why;
  try {
    m.init();
    m.tc_verbose = opt.verbose;
    m.rc_Init();
    Context& ctx = s.ctx;
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
      if (buf.size() > (1 << 16)) flush();
    }
    put_zeros(cm_count(m.nal(F_EOFZ, 0, 0), (i64)1 << 40));
    flush();
    if (m.nal(F_CRC, 0, 0) != (crc.value() ^ kCrcXor)) cm_fail("CRC mismatch: the stream did not decompress correctly");
    // the coder's flush leaves off the last bytes, so the decoder always
    // reads past the end of a whole stream
    if (!m.over) cm_fail("corrupt stream: data after its end");
  } catch (const std::exception& e) {  // CmError, TsvError
    failed = true;
    why = e.what();
  }
  if (failed) {
    flush();  // what was decoded
    progress_stop(opt.meter);
    fprintf(stderr, "h264p: %s: %s\n", name, why.c_str());
    return 1;
  }
  if (opt.verbose) {
    progress_cut(opt.meter);
    fprintf(stderr, "%ld NAL units\n", nals);
  }
  return 0;
}

#include "coro.inc"

// The coroutine of a call of c, d or x (or of a part, mt.inc), constructed
// on pages of its own -- zeroed, and aligned as CoroutinePair's members
// ask -- and destroyed with them
template <class T>
struct CoroObj {
  T* p;
  CoroObj() : p(new (tc_map(sizeof(T), "a coroutine")) T()) {}
  ~CoroObj() {
    p->~T();
    tc_unmap(p, sizeof(T));
  }
  CoroObj(const CoroObj&) = delete;
  void operator=(const CoroObj&) = delete;
  T* get() const { return p; }
  T* operator->() const { return p; }
};

#include "mt.inc"

// c, d and x on files, or stdin and stdout for "-" (Lib3's stdio file API),
// their coroutines run by CoroFileProc

// CoroFileProc::processfile() (Lib3's coro_fhp2.inc), and a look at the
// progress line after each buffer it reads or writes: the input from f
// (what was read of it already, [pre, pre_end), first), the output into g
template <class C>
uint process_file(C& c, filehandle f, filehandle g, Progress& pr, byte* pre = nullptr, byte* pre_end = nullptr) {
  uint r;
  c.coro_init();
  c.addout(c.outbuf, C::outbufsize);
  if (pre) c.addinp(pre, (uint)(pre_end - pre));
  for (;;) {
    r = c.coro_call(&c);
    if (r == 1) {
      uint l = f.sread(c.inpbuf, C::inpbufsize);
      if (!l) c.f_quit = 1;
      c.addinp(c.inpbuf, l);
    } else {
      uint l = c.getoutsize();
      if (l) g.writ(c.outbeg, l);
      if (r != 2) break;
      c.addout(c.outbuf, C::outbufsize);
    }
    pr.poll();
  }
  return r;
}

// the size of the file f reads, 0 if it is not a regular file (a pipe)
uint64_t file_size(FILE* f) {
#ifdef _WIN32
  struct _stati64 s;
  return _fstati64(_fileno(f), &s) == 0 && (s.st_mode & _S_IFMT) == _S_IFREG ? (uint64_t)s.st_size : 0;
#else
  struct stat s;
  return fstat(fileno(f), &s) == 0 && S_ISREG(s.st_mode) ? (uint64_t)s.st_size : 0;
#endif
}

// A call's progress line, tagged tag, through the input f: shown with -p,
// or where stderr is a terminal
void progress_start(Progress& pr, const Options& opt, const char* tag, filehandle& f) {
#ifdef _WIN32
  bool tty = _isatty(_fileno(stderr)) != 0;
#else
  bool tty = isatty(fileno(stderr)) != 0;
#endif
  if (opt.progress || tty) pr.start(tag, file_size(f.f));
}

bool open_files(const char* in_name, const char* out_name, filehandle& f, filehandle& g) {
  f.open(in_name);
  if (!f.f) {
    fprintf(stderr, "h264p: cannot open %s: %s\n", in_name, strerror(errno));
    return false;
  }
  g.make(out_name);
  if (!g.f) {
    fprintf(stderr, "h264p: cannot create %s: %s\n", out_name, strerror(errno));
    if (f.f != stdin) fclose(f.f);
    return false;
  }
  return true;
}

// The exit status of c, d or x after their coroutines' status r, the files
// closed: a failed c or x leaves no output file behind, a failed d none it
// wrote nothing to (only a regular file: the output may be /dev/null)
int close_files(int r, filehandle& f, filehandle& g, const char* out_name, bool keep) {
  if (!r && ferror(f.f)) {
    fprintf(stderr, "h264p: read error\n");
    r = 1;
  }
  if (f.f != stdin) fclose(f.f);
  bool werr = false;
  if (g.f) {  // (none after a failed rewind_files)
    werr = ferror(g.f) != 0;
    if (fclose(g.f)) werr = true;
  }
  if (!r && werr) {
    fprintf(stderr, "h264p: write error\n");
    r = 1;
  }
  struct stat st;
  if (r && !keep && strcmp(out_name, "-") && stat(out_name, &st) == 0 && S_ISREG(st.st_mode)) remove(out_name);
  return r ? 1 : 0;
}

// (the model pass checks the dry runs where the input can be read again and
// the output written again: regular files)
bool can_check(const char* in_name, const char* out_name) {
  struct stat si, so;
  return strcmp(in_name, "-") && strcmp(out_name, "-") && stat(in_name, &si) == 0 && S_ISREG(si.st_mode) &&
         (stat(out_name, &so) != 0 || S_ISREG(so.st_mode));
}

// after kRestart: the input from its start, the output empty
bool rewind_files(const char* out_name, filehandle& f, filehandle& g) {
  fclose(g.f);
  g.make(out_name);
  if (!g.f || fseek(f.f, 0, SEEK_SET)) {
    fprintf(stderr, "h264p: cannot start again\n");
    return false;
  }
  return true;
}

bool has_level(int level) {
  bool r = false;
#define H264P_K(n) r = r || level == n;
  H264P_LEVELS(H264P_K)
#undef H264P_K
  if (!r) fprintf(stderr, "h264p: no level -%d in this build\n", level);
  return r;
}

// c: one model, the level's (H264pCoro<0, LV>), on all of the stream
template <class LV>
int compress_lv(const char* in_name, const char* out_name, const Options& opt, filehandle& f, filehandle& g) {
  Progress pr;
  progress_start(pr, opt, "enc", f);
  H264pArgs a = {opt.level, opt.verbose, opt.strict, can_check(in_name, out_name), in_name, &pr};
  for (;;) {
    int r;
    uint64_t raw, packed;
    {
      CoroObj<CoroFileProc<H264pCoro<0, LV>>> c;  // (a new model each time)
      pr.what = [&](uint64_t& raw, uint64_t& packed, uint64_t& pos) { pos = raw = c->n_in, packed = c->n_out; };
      c->Init(&a);
      process_file(*c.get(), f, g, pr);
      pr.what = nullptr;
      r = c->status;
      raw = c->n_in, packed = c->n_out;
    }
    if (r != kRestart) {
      if (r)
        pr.cut();
      else
        pr.done(raw, packed);
      return close_files(r, f, g, out_name, false);
    }
    // (a slice did not come out as it is: again, the dry run writing back each)
    a.check = false;
    if (!rewind_files(out_name, f, g)) return close_files(1, f, g, out_name, false);
  }
}

// c -t N: the stream in parts on N threads (mt.inc), after h264p's header
int compress_mt(const char* in_name, const char* out_name, const Options& opt, filehandle& f, filehandle& g) {
  Progress pr;
  progress_start(pr, opt, "enc", f);
  H264pArgs a = {opt.level, false, opt.strict, true, in_name, &pr};
  CoroObj<CoroFileProc<MtEnc>> c;
  pr.what = [&](uint64_t& raw, uint64_t& packed, uint64_t& pos) {
    mt_done(c->M2, raw, packed);
    pos = raw = pr.total ? std::min(raw, pr.total) : raw;  // (parts have their parameter sets copied in)
    packed += kMtHead;
  };
  if (c->M1.Init(opt.part, 0) || c->M2.Init(opt.threads, opt.part, &a, 1)) {
    fprintf(stderr, "h264p: cannot start %d threads\n", opt.threads);
    return close_files(1, f, g, out_name, false);
  }
  uint8_t head[kMtHead];
  memcpy(head, kMtMagic, sizeof kMtMagic - 1);
  head[sizeof kMtMagic - 1] = (uint8_t)kCmVersion;
  head[sizeof kMtMagic] = (uint8_t)opt.threads;
  put_le(head + sizeof kMtMagic + 1, opt.part >> 10, 4);
  fwrite(head, 1, sizeof head, g.f);
  int r = process_file(*c.get(), f, g, pr) ? 1 : 0;
  bool said = mt_failed(c->M2);
  uint64_t in_bytes = c->M1.st->total, out_bytes = sizeof head + c->M2.c_out, parts = c->M1.st->parts;
  c->M2.Quit();  // (the threads joined)
  c->M1.Quit();
  if (r)
    pr.cut();
  else
    pr.done(in_bytes, out_bytes);
  if (r && !said) fprintf(stderr, "h264p: %s: internal: a thread stopped early\n", in_name);
  if (!r && opt.verbose)
    fprintf(stderr, "%" PRIu64 " -> %" PRIu64 " bytes (%.2f%%), %" PRIu64 " parts on %d threads\n", in_bytes, out_bytes,
            in_bytes ? 100.0 * out_bytes / in_bytes : 0.0, parts, opt.threads);
  return close_files(r, f, g, out_name, false);
}

int compress(const char* in_name, const char* out_name, const Options& opt) {
  if (!has_level(opt.level)) return 2;
  filehandle f, g;
  if (!open_files(in_name, out_name, f, g)) return 1;
  if (opt.threads > 1) return compress_mt(in_name, out_name, opt, f, g);
  switch (opt.level) {
#define H264P_C(n) \
  case n:          \
    return compress_lv<Lv##n>(in_name, out_name, opt, f, g);
    H264P_LEVELS(H264P_C)
#undef H264P_C
  }
  return close_files(1, f, g, out_name, false);
}

// d of one model's stream, at its level: the bytes of the input read
// already (head, got of them) first
template <class LV>
int decompress_lv(const char* in_name, const char* out_name, const Options& opt, filehandle& f, filehandle& g,
                  uint8_t* head, size_t got) {
  Progress pr;
  progress_start(pr, opt, "dec", f);
  H264pArgs a = {0, opt.verbose, false, false, in_name, &pr};
  int r;
  uint64_t raw, packed;
  {
    CoroObj<CoroFileProc<H264pCoro<1, LV>>> d;
    pr.what = [&](uint64_t& raw, uint64_t& packed, uint64_t& pos) { raw = d->n_out, pos = packed = d->n_in; };
    d->Init(&a);
    process_file(*d.get(), f, g, pr, head, head + got);
    pr.what = nullptr;
    r = d->status;
    raw = d->n_out, packed = d->n_in;
  }
  if (r)
    pr.cut();
  else
    pr.done(raw, packed);
  return close_files(r, f, g, out_name, raw > 0);  // (what was decoded before an error)
}

// d of the parts on threads: as many threads as compressed them
int decompress_mt(const char* in_name, const char* out_name, const Options& opt, filehandle& f, filehandle& g,
                  const uint8_t* head, size_t got) {
  int threads = got == (size_t)kMtHead ? head[sizeof kMtMagic] : 0;
  uint32_t part = (uint32_t)std::min<uint64_t>(get_le(head + sizeof kMtMagic + 1, 4) << 10, 4095u << 20);
  if (got > sizeof kMtMagic - 1 && head[sizeof kMtMagic - 1] != kCmVersion) {
    fprintf(stderr, "h264p: %s: compressed by version %d, this is version %d\n", in_name, (int)head[sizeof kMtMagic - 1],
            kCmVersion);
    return close_files(1, f, g, out_name, false);
  }
  if (threads < 1 || threads > 64) {
    fprintf(stderr, "h264p: %s: a broken or truncated stream\n", in_name);
    return close_files(1, f, g, out_name, false);
  }
  Progress pr;
  progress_start(pr, opt, "dec", f);
  H264pArgs a = {0, false, false, false, in_name, &pr};
  CoroObj<CoroFileProc<MtDec>> d;
  pr.what = [&](uint64_t& raw, uint64_t& packed, uint64_t& pos) {
    mt_done(*d.get(), packed, raw);
    pos = packed += kMtHead;
  };
  if (d->Init(threads, part, &a, 1)) {
    fprintf(stderr, "h264p: cannot start %d threads\n", threads);
    return close_files(1, f, g, out_name, false);
  }
  int r = process_file(*d.get(), f, g, pr) ? 1 : 0;
  bool said = mt_failed(*d.get());
  uint64_t out_bytes = d->c_out, in_bytes = kMtHead + d->c_inp, parts = mt_parts(*d.get());
  d->Quit();
  if (r)
    pr.cut();
  else
    pr.done(out_bytes, in_bytes);
  if (r && !said) fprintf(stderr, "h264p: %s: a broken or truncated stream\n", in_name);
  if (!r && opt.verbose) fprintf(stderr, "%" PRIu64 " bytes, %" PRIu64 " parts on %d threads\n", out_bytes, parts, threads);
  return close_files(r, f, g, out_name, out_bytes > 0);
}

// d: the first bytes say which it is, a stream of one model (and its level)
// or of parts on threads (kMtMagic)
int decompress(const char* in_name, const char* out_name, const Options& opt) {
  filehandle f, g;
  if (!open_files(in_name, out_name, f, g)) return 1;
  uint8_t head[kMtHead];
  size_t got = fread(head, 1, sizeof head, f.f);
  if (got >= sizeof kMtMagic - 1 && !memcmp(head, kMtMagic, sizeof kMtMagic - 1))
    return decompress_mt(in_name, out_name, opt, f, g, head, got);
  int level = cm_level(head, got, in_name);
  switch (level) {
#define H264P_D(n) \
  case n:          \
    return decompress_lv<Lv##n>(in_name, out_name, opt, f, g, head, got);
    H264P_LEVELS(H264P_D)
#undef H264P_D
  }
  if (level >= 0) fprintf(stderr, "h264p: %s: compressed at level -%d, which this build has not\n", in_name, level);
  return close_files(1, f, g, out_name, false);
}

// x: c and d at once, the compressed stream going from the one coroutine to
// the other in a CoroutinePair; the output is the stream restored
template <class LV>
int roundtrip_lv(const char* in_name, const char* out_name, const Options& opt, filehandle& f, filehandle& g) {
  Progress pr;
  progress_start(pr, opt, "x", f);
  H264pArgs a = {opt.level, opt.verbose, opt.strict, can_check(in_name, out_name), in_name, &pr};
  H264pArgs b = {0, opt.verbose, false, false, "the compressed stream", &pr};
  for (;;) {
    int r;
    uint64_t raw, packed;
    {
      CoroObj<CoroFileProc<CoroutinePair<H264pCoro<0, LV>, H264pCoro<1, LV>>>> x;
      pr.what = [&](uint64_t& raw, uint64_t& packed, uint64_t& pos) { pos = raw = x->M1.n_in, packed = x->M1.n_out; };
      x->M1.Init(&a);
      x->M2.Init(&b);
      process_file(*x.get(), f, g, pr);
      pr.what = nullptr;
      r = x->M1.status ? x->M1.status : x->M2.status;
      raw = x->M1.n_in, packed = x->M1.n_out;
    }
    if (r != kRestart) {
      if (r)
        pr.cut();
      else
        pr.done(raw, packed);
      return close_files(r, f, g, out_name, false);
    }
    a.check = false;
    if (!rewind_files(out_name, f, g)) return close_files(1, f, g, out_name, false);
  }
}

// x -t N: the parts on N threads into MTwrap's decoder on N more
int roundtrip_mt(const char* in_name, const char* out_name, const Options& opt, filehandle& f, filehandle& g) {
  Progress pr;
  progress_start(pr, opt, "x", f);
  H264pArgs a = {opt.level, false, opt.strict, true, in_name, &pr};
  H264pArgs b = {0, false, false, false, "the compressed stream", &pr};
  CoroObj<CoroFileProc<CoroutinePair<MtEnc, MtDec>>> x;
  pr.what = [&](uint64_t& raw, uint64_t& packed, uint64_t& pos) {
    mt_done(x->M1.M2, raw, packed);
    pos = raw = pr.total ? std::min(raw, pr.total) : raw;
    packed += kMtHead;
  };
  if (x->M1.M1.Init(opt.part, 0) || x->M1.M2.Init(opt.threads, opt.part, &a, 1) ||
      x->M2.Init(opt.threads, opt.part, &b, 1)) {
    fprintf(stderr, "h264p: cannot start %d threads\n", 2 * opt.threads);
    return close_files(1, f, g, out_name, false);
  }
  int r = process_file(*x.get(), f, g, pr) ? 1 : 0;
  bool said = mt_failed(x->M1.M2) || mt_failed(x->M2);
  uint64_t in_bytes = x->M1.M1.st->total, mid_bytes = kMtHead + x->M1.M2.c_out, parts = x->M1.M1.st->parts;
  x->M2.Quit();
  x->M1.M2.Quit();
  x->M1.M1.Quit();
  if (r)
    pr.cut();
  else
    pr.done(in_bytes, mid_bytes);
  if (r && !said) fprintf(stderr, "h264p: %s: internal: a thread stopped early\n", in_name);
  if (!r && opt.verbose)
    fprintf(stderr, "%" PRIu64 " -> %" PRIu64 " bytes (%.2f%%), %" PRIu64 " parts on %d threads\n", in_bytes, mid_bytes,
            in_bytes ? 100.0 * mid_bytes / in_bytes : 0.0, parts, opt.threads);
  return close_files(r, f, g, out_name, false);
}

int roundtrip(const char* in_name, const char* out_name, const Options& opt) {
  if (!has_level(opt.level)) return 2;
  filehandle f, g;
  if (!open_files(in_name, out_name, f, g)) return 1;
  if (opt.threads > 1) return roundtrip_mt(in_name, out_name, opt, f, g);
  switch (opt.level) {
#define H264P_X(n) \
  case n:          \
    return roundtrip_lv<Lv##n>(in_name, out_name, opt, f, g);
    H264P_LEVELS(H264P_X)
#undef H264P_X
  }
  return close_files(1, f, g, out_name, false);
}

void usage() {
  fprintf(stderr,
          "usage: h264p c|d|t|r|x [options] input output\n"
          "  c  compress an H.264 Annex B stream\n"
          "  d  decompress it\n"
          "  t  dump the stream's syntax to a TSV file\n"
          "  r  restore the stream from the TSV file\n"
          "  x  compress and decompress at once (the compressor's output the\n"
          "     decompressor's input, two coroutines): writes the stream restored\n"
          "options:\n"
          "  -v    print statistics\n"
          "  -s    strict (c, t, x): fail if a NAL unit of a supported type has to be stored verbatim\n"
          "  -p    (c, d, x) progress on stderr even when it is not a terminal (where it is, always)\n"
          "  -L    (c, x) the model's level, 1 (fastest) to 9 (strongest, the default)\n"
          "  -t N  (c, x) compress on N threads, 1 to 64 (default 1): the stream in parts that start\n"
          "        at IDR access units, a model for each; d takes N from the stream\n"
          "  -c N  (c, x with -t) the size of a part, at least: N MB, or N KB with a k (default 4)\n"
          "input and output can be - for stdin and stdout\n");
}

}  // namespace
}  // namespace h264p

// (H264P_NO_MAIN: a program of its own includes this file, for the
// coroutines of coro.inc)
#ifndef H264P_NO_MAIN
int main(int argc, char** argv) {
  using namespace h264p;
  if (argc < 2 || !argv[1][0] || argv[1][1] || !strchr("cdtrx", argv[1][0])) {
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
        else if (*p == 'p')
          opt.progress = true;
        else if (*p >= '1' && *p <= '9')
          opt.level = *p - '0';
        else if ((*p == 't' || *p == 'c') && (p[1] || i + 1 < argc)) {
          // -t N, -c N (or -tN, -cN): the rest of the argument, or the next
          const char* v = p[1] ? p + 1 : argv[++i];
          char* e;
          unsigned long n = strtoul(v, &e, 10);
          if (*p == 't') {
            if (*e || e == v || n < 1 || n > 64) {
              fprintf(stderr, "h264p: -t takes 1 to 64 threads\n");
              return 2;
            }
            opt.threads = (int)n;
          } else {
            unsigned long unit = 1 << 20;
            if (*e == 'k' || *e == 'K') unit = 1 << 10, e++;
            if (*e || e == v || n < 1 || n > (4095ul << 20) / unit) {
              fprintf(stderr, "h264p: -c takes a part size of 1 to 4095 MB, or in KB with a k\n");
              return 2;
            }
            opt.part = (uint32_t)(n * unit);
          }
          break;
        } else {
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
      case 'x':
        return roundtrip(files[0], files[1], opt);
      default:
        return tsv_restore(files[0], files[1], opt);
    }
  } catch (const std::exception& e) {  // no memory
    fprintf(stderr, "h264p: %s\n", e.what());
    return 1;
  }
}
#endif
