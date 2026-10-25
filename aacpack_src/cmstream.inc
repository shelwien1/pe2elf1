// The compressed stream of one model (c, d): its header, then the stream
// through the model (model.inc), unit by unit, each parsed and written back
// from its values before it is coded, and a CRC-32 at the end (as h264p's
// cmstream.inc).
//
// The stream: the magic, the version and the model's level, then one range
// coded run of decisions, flushed once at its end: the AudioSpecificConfig
// of -asc, if any, and the SBR headers assumed for the elements whose first
// SBR payloads come before their first header; then each unit's kind (the
// end, bytes, an ADTS or LOAS frame, an ADIF header, a raw_data_block, the
// AUS1 header or one of its access units) and the unit: its syntax through
// the model, or (bytes) their count and the bytes; and the CRC-32 of the
// stream. What the compressor finds of no container (tags, junk, frames that
// do not parse or would not come out as they were) is bytes, in chunks of at
// most kChunk.

namespace aacpack {

const char kCmMagic[] = "aacpk";  // compressed streams, followed by the version and level bytes
const int kCmVersion = 6;
// The CRC-32 at the end is coded XORed with this: a decoder fed past the
// end of its input decodes zeros, and 0 is the CRC-32 of an empty stream.
const uint32_t kCrcXor = 0x61616370;

// whether v, a compressed stream's version byte, is this version (else why,
// a line, in msg, of mn bytes, for the caller to say)
bool cm_version_ok(int v, const char* name, char* msg, size_t mn) {
  if (v == kCmVersion) return true;
  snprintf(msg, mn, "aacpack: %s: compressed by version %d, this is version %d\n", name, v, kCmVersion);
  return false;
}
// The level of a compressed stream from its first bytes, n of them read
// (the magic, the version, the level), or -1 if they are not aacpack's
// (why in msg)
int cm_level(const uint8_t* head, size_t n, const char* name, char* msg, size_t mn) {
  const size_t m = sizeof kCmMagic - 1;
  if (!n || memcmp(head, kCmMagic, Min(n, m))) {
    snprintf(msg, mn, "aacpack: %s: not a stream aacpack compressed\n", name);
    return -1;
  }
  if (n > m && !cm_version_ok(head[m], name, msg, mn)) return -1;
  if (n < m + 2) {
    snprintf(msg, mn, "aacpack: %s: the compressed stream is cut short\n", name);
    return -1;
  }
  return head[m + 1];
}

// the containers c can be told to look for alone
enum { F_AUTO, F_ADTS, F_LOAS, F_ADIF };

// What a run of the compressor or the decompressor takes: Init() (coro.inc)
struct AacArgs {
  int level;       // compressing: the model's level
  bool verbose;    // statistics, and the messages they come with
  bool strict;     // compressing: fail where a frame would be stored as its bytes, or kept partly as bits
  int format;      // compressing: the container to look for (F_*; 0 any)
  const uint8_t* asc;  // compressing: -asc, the AudioSpecificConfig of raw access units (asc_n bytes; none 0)
  size_t asc_n;
  const char* name;  // the input's, for messages
  Progress* meter;   // the call's progress line, where its driver shows what the codec tells (coro.inc)

  void Init() {
    level = 0;
    verbose = strict = false;
    format = F_AUTO;
    asc = nullptr;
    asc_n = 0;
    name = "";
    meter = nullptr;
  }
};

// the kinds of unit
enum { U_END, U_RAW, U_ADTS, U_LOAS, U_ADIF, U_BLOCK, U_AUS1, U_AU, U_N };
static const char* const kUnitName[U_N] = {"end", "bytes", "ADTS frames", "LOAS frames", "ADIF headers",
                                           "raw_data_blocks", "AUS1 headers", "access units"};

const size_t kChunk = (size_t)1 << 20;     // bytes coded as they are, at most at once
const size_t kBlockMax = (size_t)1 << 20;  // a raw_data_block of ADIF or -asc, at most (a rest of one takes this much)
const size_t kAdifMax = (size_t)1 << 16;   // an ADIF header
const size_t kAuMax = (size_t)1 << 20;     // an AUS1 access unit (a longer one starts bytes to the end)
const size_t kLook = 8194 + 16;            // a LOAS or ADTS frame and the header after it
const uint64_t kProbeMax = (uint64_t)8 << 20;  // the probe for SBR headers reads at most this far
const uint64_t kProbeUnits = 1000;             // or this many units with no SBR payload before a header

// the most bits the walk may write of a unit of the kind
static size_t cap_of(int kind) {
  switch (kind) {
    case U_ADTS: return (size_t)(8191 + 64) * 8;
    case U_LOAS: return (size_t)(8194 + 64) * 8;
    case U_ADIF: return kAdifMax * 8;
    case U_BLOCK: return kBlockMax * 8;
    case U_AUS1: return (size_t)(65535 + 6 + 64) * 8;
    default: return (kAuMax + 4 + 64) * 8;  // U_AU
  }
}

// A unit of the kind, walked: parsing, the len bytes (0: as long as it
// parses to be, of the avail there are) at the walk's buffer, and its
// length then; writing, into the walk's fw
static size_t walk_unit(Walk& c, AdtsRegs& rg, int kind, size_t len, size_t avail) {
  switch (kind) {
    case U_ADTS: adts_frame(c, rg, 0, len); return len;
    case U_LOAS: loas_frame(c, 0, len); return len;
    case U_ADIF:
      if (c.R) c.br.Init(c.buf, 0, avail * 8);
      adif_header(c, 0);
      break;
    case U_BLOCK:
      if (c.R) c.br.Init(c.buf, 0, avail * 8);
      c.raw_data_block(avail * 8, true);
      break;
    case U_AUS1: aus1_header(c, 0, len - 6); return len;
    default: au_unit(c, 0, len - 4); return len;  // U_AU
  }
  if (!c.R) return 0;
  if (c.br.pos & 7) c.bad("a unit that ends inside a byte");
  return c.br.pos / 8;
}

// What the stream is, as the walk's state has it after a unit of the kind
static AudioInfo audio_of(const Walk& c, int kind) {
  AudioInfo a;
  memset((void*)&a, 0, sizeof a);
  a.container = kind == U_ADTS ? 1 : kind == U_LOAS ? 2 : kind == U_AU || kind == U_AUS1 ? 4 : kind == U_ADIF ? 3 : 5;
  a.aot = c.st.aot;
  a.sf_index = c.st.sf_index;
  a.chan_config = kind == U_ADTS ? c.st.fixed.chan_config : c.st.chan_config;
  a.frame960 = c.st.frame960;
  a.sbr = c.sbr_seen;
  a.ps = c.ps_seen;
  return a;
}
// told to the driver (kYieldAudio) where it adds to what was told; and the
// progress (kYieldTick), each unit
static void tell_unit(CoroIo& io, const Walk& c, int kind) {
  if (kind != U_RAW && kind != U_AUS1) {
    AudioInfo a = audio_of(c, kind);
    if (io.audio.add(a)) io.tell(kYieldAudio);
  }
  io.frames++;
  io.tell(kYieldTick);
}

// a count coded in a compressed stream
size_t cm_count(i64 v, i64 lim) {
  if (v < 0 || v > lim) err_raise("corrupt stream: a count of %" PRId64, v);
  return (size_t)v;
}

// what -v says of a compression
struct Stats {
  uint64_t units[U_N];  // coded, by kind (bytes: chunks)
  uint64_t raw_bytes;   // coded as bytes
  uint64_t bad, unverified, rests;  // units that did not parse, did not come out as they were, kept partly as bits
  uint64_t assumed;     // SBR headers assumed
  uint64_t ob[OB_N];    // the bits of the parts of the units coded, as they were (Walk::ob)

  void Init() { memset((void*)this, 0, sizeof *this); }
  void report(Progress* p) const {
    say(p, "units:");
    for (int k = U_RAW; k < U_N; k++)
      if (units[k]) say(p, " %" PRIu64 " %s", units[k], k == U_RAW ? "chunks of bytes" : kUnitName[k]);
    say(p, "\n%" PRIu64 " bytes stored as they are; %" PRIu64 " frames that did not parse, %" PRIu64
           " that would not come out as they were, %" PRIu64 " kept partly as bits; %" PRIu64 " SBR headers assumed\n",
        raw_bytes, bad, unverified, rests, assumed);
    const char* sep = "the parts, as they were:";
    for (int k = 0; k < OB_N; k++)
      if (ob[k]) say(p, "%s %s %.0f bytes", sep, kObName[k], ob[k] / 8.0), sep = ",";
    if (sep[0] == ',') say(p, " (the signs a bit each, part of the spectra)\n");
  }
};

// -v's statistics of a compression's coder (said after the progress line,
// meter's, if any), and where the bits of each element went
template <class M>
void cm_report(const M& e, u64 in_bytes, u64 out_bytes, Progress* meter) {
  double tot = 0;
  for (int i = 0; i < STG_N; i++) tot += e.tc_bits[i];
  say(meter, "%" PRIu64 " -> %" PRIu64 " bytes (%.2f%%), %" PRIu64 " MB of model tables\n", in_bytes, out_bytes,
      in_bytes ? 100.0 * out_bytes / in_bytes : 0.0, e.table_bytes() >> 20);
  for (int i = 0; i < STG_N; i++) {
    if (!e.tc_syms[i]) continue;
    say(meter, "  %-13s %11.0f bytes %6.2f%%  %10" PRIu64 " values or decisions, %.3f bits each\n", kStage[i],
        e.tc_bits[i] / 8, tot > 0 ? 100.0 * e.tc_bits[i] / tot : 0.0, e.tc_syms[i],
        e.tc_bits[i] / (double)e.tc_syms[i]);
  }
  say(meter, "  by element:\n");
  for (int k = 0; k < E_N; k++)
    if (e.esyms_[k] && e.ebits_[k] >= 8 * 16)
      say(meter, "    %-14s %11.0f bytes %6.2f%%  %10" PRIu64 " values, %.3f bits each\n", kElemName[k],
          e.ebits_[k] / 8, tot > 0 ? 100.0 * e.ebits_[k] / tot : 0.0, e.esyms_[k], e.ebits_[k] / (double)e.esyms_[k]);
}

// The input of a compression, read as it is needed: the bytes [base,
// base + b.n) of the stream, and kPad zeros after them (the bit reader
// reads past what it parses); its CRC-32 as it is read
struct InWindow {
  static const size_t kPad = 16, kRead = (size_t)1 << 16;
  ByteSource src;
  Buf<uint8_t> b;
  uint64_t base;
  bool eof;
  Crc32 crc;

  void Init(ByteSource s) {
    src = s;
    b.Init();
    b.reserve(kPad);
    memset(b.p, 0, kPad);
    base = 0;
    eof = false;
    crc.Init();
  }
  void Quit() { b.Quit(); }
  uint64_t end() const { return base + b.n; }
  size_t have(uint64_t p) const { return p < end() ? (size_t)(end() - p) : 0; }
  // k bytes from p on (p not before base), unless the stream ends first: how many there are
  size_t need(uint64_t p, size_t k) {
    while (have(p) < k && !eof) {
      size_t want = Max(kRead, k - have(p));
      b.room(b.n + want + kPad);
      size_t r = src.read(b.p + b.n, want);
      if (!r) {
        eof = true;
        break;
      }
      crc.add(b.p + b.n, r);
      b.n += r;
      memset(b.p + b.n, 0, kPad);
    }
    return Min(have(p), k);
  }
  const uint8_t* at(uint64_t p) const { return b.p + (p - base); }
  // the bytes before p done with (moved out now and then)
  void release(uint64_t p) {
    size_t k = (size_t)(p - base);
    if (k < ((size_t)1 << 20) || k < b.n / 2) return;
    b.erase_front(k);
    base = p;
    memset(b.p + b.n, 0, kPad);
  }
};

// The length of the ID3v2 tag at p (the header there: as long as it says), else 0
static uint64_t id3v2_len(const uint8_t* p, size_t avail) {
  if (avail < 10 || memcmp(p, "ID3", 3) || p[3] == 0xff || p[4] == 0xff || ((p[6] | p[7] | p[8] | p[9]) & 0x80))
    return 0;
  uint64_t len = ((uint64_t)p[6] << 21 | (uint64_t)p[7] << 14 | (uint64_t)p[8] << 7 | p[9]) + 10;
  if (p[5] & 0x10) len += 10;
  return len;
}

// What a compression keeps from one unit to the next besides its model, on
// the heap: the stack of its coroutine (coro.inc), which each yield
// copies, stays small, and Quit() frees all of it wherever the coroutine
// was left
struct CompressState {
  InWindow win;
  Walk walk;
  AdtsRegs rg;
  ValueLog log;
  SynState before, after;
  Stats st;

  void Init(ByteSource in) {
    win.Init(in);
    walk.Init();
    rg.Init();
    log.Init();
    before.reset();
    after.reset();
    st.Init();
  }
  void Quit() {
    win.Quit();
    walk.Quit();
    rg.Quit();
    log.Quit();
  }
};

// and a decompression
struct DecompressState {
  Walk walk;
  AdtsRegs rg;
  Crc32 crc;
  uint64_t units;     // the units decoded (-v)
  Buf<uint8_t> buf;   // the stream decoded, not yet written
  Buf<uint8_t> asc;   // -asc's AudioSpecificConfig

  void Init() {
    walk.Init();
    rg.Init();
    crc.Init();
    units = 0;
    buf.Init();
    asc.Init();
  }
  void Quit() {
    walk.Quit();
    rg.Quit();
    buf.Quit();
    asc.Quit();
  }
};

// The compressor's scan of its input (aac2tsv's Encoder): the containers
// it finds, and between them bytes; or, with no model (the probe), the
// same scan parsing only, for the SBR headers of the elements whose first
// payloads come without one
template <class LV>
struct Scanner {
  Model<0, LV>* m;  // (none: the probe)
  CompressState* s;
  const AacArgs* a;
  uint64_t pos;        // the next byte to scan
  uint64_t raw_start;  // the bytes [raw_start, pos) are to be coded as they are
  int last;            // the kind of the last unit coded
  bool stop;           // the probe is done
  uint64_t probe_units;

  void Init(Model<0, LV>* mm, CompressState* ss, const AacArgs* aa) {
    m = mm, s = ss, a = aa;
    pos = raw_start = 0;
    last = U_END;
    stop = false;
    probe_units = 0;
  }
  bool probe() const { return !m; }
  InWindow& win() { return s->win; }

  // the bytes [raw_start, pos), coded
  void flush_raw() {
    if (probe()) return;
    while (raw_start < pos) {
      size_t n = (size_t)Min<uint64_t>(pos - raw_start, kChunk);
      const uint8_t* p = win().at(raw_start);
      m->unit(UF_KIND, last, U_RAW);
      last = U_RAW;
      m->unit(UF_RAWN, 0, (i64)n);
      if (n >= kPlainMin && m->raw_plain(m->raw_random(p, n)))
        for (size_t j = 0; j < n; j++) m->raw_byte(p[j]);
      else
        for (size_t j = 0; j < n; j++) m->raw(p[j]);
      raw_start += n;
      s->st.units[U_RAW]++;
      s->st.raw_bytes += n;
      tell_unit(*m, s->walk, U_RAW);
    }
    win().release(raw_start);
  }
  // after bytes went by as bytes
  void after_raw() {
    if (probe()) {
      if (pos > kProbeMax) stop = true;
    } else if (pos - raw_start >= kChunk) {
      flush_raw();
    }
  }
  // n bytes as they are (fewer where the stream ends first)
  void skip_raw(uint64_t n) {
    while (n && !stop) {
      size_t k = win().need(pos, (size_t)Min<uint64_t>(n, kChunk));
      if (!k) break;
      pos += k;
      n -= k;
      after_raw();
    }
  }
  void rest_raw() { skip_raw(~(uint64_t)0); }

  // the probe: done when every element whose SBR payloads came before any
  // header of its has had one, or after a while with no such payloads
  void probe_check() {
    const Walk& c = s->walk;
    bool need = false, open = false;
    for (int k = 0; k < 3; k++)
      for (int t = 0; t < 16; t++)
        if (c.sbr_need[k][t]) {
          need = true;
          open |= !c.sbr_found[k][t];
        }
    probe_units++;
    stop = need ? !open : probe_units > kProbeUnits;
    if (pos > kProbeMax) stop = true;
  }

  // a frame that is stored as its bytes, where it was not to be (-s)
  void strict(const char* what, const char* why) {
    if (!a->strict || probe()) return;
    err_raise("a unit at byte %" PRIu64 " %s%s%s", pos, what, why && *why ? ": " : "", why ? why : "");
  }

  // whether the unit, written back from its values, is as it was (end
  // bytes at p), and its state the parse's
  bool writes_back(int kind, const uint8_t* p, size_t end) {
    Walk& c = s->walk;
    s->log.rewind();
    LogChan lw;
    lw.Init(s->log, true);
    ErrTrap t;
    if (setjmp(t.jb)) return false;
    err_push(t);
    c.start_write(&lw, cap_of(kind));
    walk_unit(c, s->rg, kind, 0, 0);
    err_pop(t);
    return lw.at_end() && c.fw.pos == end * 8 && !memcmp(c.fw.b.p, p, end) && c.st.same(s->after);
  }

  // The unit of the kind at pos, len bytes long (0: as long as it parses to
  // be): parsed, written back from its values to see that it comes out the
  // same, and coded (the probe: parsed). False where it did not parse and
  // its length is not known; else pos is past it (its bytes still to be
  // coded where it did not come out as it was).
  bool unit(int kind, size_t len) {
    Walk& c = s->walk;
    const size_t avail = win().need(pos, len ? len : kind == U_ADIF ? kAdifMax : kBlockMax);
    const uint8_t* const p = win().at(pos);
    SynState& before = s->before;
    before = c.st;
    s->log.clear();
    LogChan lc;
    lc.Init(s->log, false);
    c.start_read(&lc, p);
    c.rests = 0;
    size_t end = 0;
    ErrTrap t;
    if (setjmp(t.jb)) {  // (it did not parse)
      c.st = before;
      if (probe()) {
        if (len) pos += len;
        probe_check();
        return len != 0;
      }
      if (!len) return false;
      s->st.bad++;
      strict("does not parse", t.why);
      if (a->verbose) m->tell_note("aacpack: the unit at byte %" PRIu64 " does not parse, stored as its bytes: %s", pos, t.why);
      pos += len;
      after_raw();
      return true;
    }
    err_push(t);
    end = walk_unit(c, s->rg, kind, len, avail);
    err_pop(t);
    if (probe()) {
      pos += end;
      probe_check();
      return true;
    }
    s->after = c.st;
    const int rests = c.rests;
    c.st = before;
    if (!writes_back(kind, p, end)) {
      c.st = before;
      s->st.unverified++;
      strict("would not come out as it was", nullptr);
      if (a->verbose) m->tell_note("aacpack: the unit at byte %" PRIu64 " would not come out as it was, stored as its bytes", pos);
      pos += end;
      after_raw();
      return true;
    }
    if (rests) {
      s->st.rests++;
      strict("parses only in part", nullptr);
    }
    for (int k = 0; k < OB_N; k++) s->st.ob[k] += c.ob[k];
    flush_raw();
    c.st = before;
    m->unit(UF_KIND, last, kind);
    last = kind;
    s->log.rewind();
    LogChan lw;
    lw.Init(s->log, true);
    ModelChan<0, LV> mc;
    mc.Init(*m, &lw);
    c.start_write(&mc, cap_of(kind));
    walk_unit(c, s->rg, kind, 0, 0);
    if (!lw.at_end() || !c.st.same(s->after)) err_raise_s("internal: the model pass did not walk the unit as it was parsed");
    pos += end;
    raw_start = pos;
    s->st.units[kind]++;
    win().release(raw_start);
    tell_unit(*m, c, kind);
    return true;
  }

  // ADTS and LOAS frames, anything between them as bytes
  void scan_frames() {
    bool insync = false;
    while (!stop) {
      size_t avail = win().need(pos, kLook);
      if (!avail) return;
      const uint8_t* p = win().at(pos);
      uint64_t tl = id3v2_len(p, avail);
      if (tl) {
        skip_raw(tl);
        insync = false;
        continue;
      }
      int kind = 0;
      size_t len = 0;
      for (int f = F_ADTS; f <= F_LOAS && !kind; f++) {
        if (a->format && a->format != f) continue;
        len = f == F_ADTS ? adts_frame_len(p, avail) : loas_frame_len(p, avail);
        if (!len || len > avail) continue;
        // (out of sync: a frame header after this one too, or the stream's end)
        size_t nl = f == F_ADTS ? adts_frame_len(p + len, avail - len) : loas_frame_len(p + len, avail - len);
        if (insync || avail == len || nl) kind = f == F_ADTS ? U_ADTS : U_LOAS;
      }
      if (!kind) {
        pos++;
        insync = false;
        after_raw();
        continue;
      }
      unit(kind, len);
      insync = true;
    }
  }

  // back-to-back raw_data_block()s to the end
  void scan_blocks() {
    while (!stop && win().need(pos, 1))
      if (!unit(U_BLOCK, 0)) {
        rest_raw();
        return;
      }
  }

  bool adif() {
    if (!unit(U_ADIF, 0)) return false;
    if (s->walk.st.blocks)
      scan_blocks();
    else
      rest_raw();
    return true;
  }

  bool aus1() {
    size_t avail = win().need(pos, 6);
    const uint8_t* p = win().at(pos);
    if (avail < 6 || memcmp(p, "AUS1", 4)) return false;
    size_t alen = p[4] | (size_t)p[5] << 8;
    if (win().need(pos, 6 + alen) < 6 + alen) return false;
    unit(U_AUS1, 6 + alen);
    while (!stop) {
      if (win().need(pos, 4) < 4) break;
      p = win().at(pos);
      uint64_t n = p[0] | (uint64_t)p[1] << 8 | (uint64_t)p[2] << 16 | (uint64_t)p[3] << 24;
      if (n > kAuMax || win().need(pos, (size_t)n + 4) < (size_t)n + 4) break;
      unit(U_AU, (size_t)n + 4);
    }
    rest_raw();
    return true;
  }

  // the scan: tags first, then raw access units (-asc), ADIF, AUS1, or frames
  void run() {
    for (;;) {
      size_t avail = win().need(pos, 10);
      uint64_t tl = id3v2_len(win().at(pos), avail);
      if (!tl || stop) break;
      skip_raw(tl);
    }
    if (stop) return;
    size_t avail = win().need(pos, 4);
    if (a->asc_n) {
      scan_blocks();
    } else if ((a->format == F_AUTO || a->format == F_ADIF) && avail >= 4 && !memcmp(win().at(pos), "ADIF", 4) && adif()) {
    } else if (a->format == F_AUTO && aus1()) {
    } else {
      scan_frames();
    }
    rest_raw();
    flush_raw();
  }
};

// the SBR headers assumed (c: of the probe), and how they are coded
static const int kSbriFields = 9;
template <class M>
void code_assumed(M& m, Walk& c, int count_hint, const int (*v)[kSbriFields]) {
  int n = (int)cm_count(m.unit(UF_SBRN, 0, count_hint), 48);
  for (int i = 0; i < n; i++) {
    int f[kSbriFields];
    static const int kMax[kSbriFields] = {2, 15, 1, 15, 15, 7, 3, 1, 3};
    for (int j = 0; j < kSbriFields; j++) f[j] = (int)cm_count(m.unit(UF_SBRI, j, v ? v[i][j] : 0), kMax[j]);
    SbrSpectrum sp;
    sp.start_freq = f[3], sp.stop_freq = f[4], sp.xover_band = f[5];
    sp.freq_scale = f[6], sp.alter_scale = f[7], sp.noise_bands = f[8];
    Walk::sbr_assume(c.st.sbr[f[0]][f[1]], f[2], sp);
  }
}

// Compresses the stream s.win reads with the model m, a new one, into its
// output (m is the range coder too, and the coroutine whose output it is,
// coro.inc, which tells its driver what the stream is, each unit, and at the
// end that its statistics are ready): 0. An error ends the run at its trap
// instead (coro.inc).
template <class LV>
int compress_stream(Model<0, LV>& m, CompressState& s, const AacArgs& a) {
  const uint8_t head[2] = {(uint8_t)kCmVersion, (uint8_t)LV::id};
  m.write((const uint8_t*)kCmMagic, sizeof kCmMagic - 1);
  m.write(head, 2);
  m.start();
  m.tc_verbose = a.verbose;
  m.rc_Init();
  Walk& c = s.walk;
  if (a.asc_n) asc_config(c.st, a.asc, a.asc_n, true);
  // the probe, then the walk afresh with what it found
  {
    Scanner<LV> pr;
    pr.Init(nullptr, &s, &a);
    pr.run();
  }
  int assumed[48][kSbriFields], na = 0;
  for (int k = 0; k < 3; k++)
    for (int t = 0; t < 16; t++)
      if (c.sbr_need[k][t] && c.sbr_found[k][t]) {
        const SbrState& f = c.sbr_first[k][t];
        int* v = assumed[na++];
        v[0] = k, v[1] = t, v[2] = f.amp_res, v[3] = f.sp.start_freq, v[4] = f.sp.stop_freq, v[5] = f.sp.xover_band;
        v[6] = f.sp.freq_scale, v[7] = f.sp.alter_scale, v[8] = f.sp.noise_bands;
      }
  c.Quit();
  c.Init();
  if (a.asc_n) asc_config(c.st, a.asc, a.asc_n, true);
  // the stream's header: -asc's config, the assumed SBR headers
  m.unit(UF_ASCN, 0, (i64)a.asc_n);
  for (size_t i = 0; i < a.asc_n; i++) m.raw(a.asc[i]);
  code_assumed(m, c, na, assumed);
  s.st.assumed = (uint64_t)na;
  Scanner<LV> sc;
  sc.Init(&m, &s, &a);
  sc.run();
  m.unit(UF_KIND, sc.last, U_END);
  m.unit(UF_CRC, 0, s.win.crc.value() ^ kCrcXor);
  m.rc_Quit();
  m.tell(kYieldStats);
  return 0;
}

// Decompresses with the model m, a new one, what is left of its input
// after the magic, the version and the level, into its output (m is the
// range decoder too, and the coroutine whose input it reads, coro.inc): 0.
// An error ends the run at its trap (coro.inc), which writes what was
// decoded (s.buf), and the error is m's note.
template <class LV>
int decompress_stream(Model<1, LV>& m, DecompressState& s, const AacArgs& a) {
  Buf<uint8_t>& buf = s.buf;
  auto flush = [&]() {
    s.crc.add(buf.data(), buf.size());
    m.write(buf.data(), buf.size());
    buf.clear();
  };
  m.start();
  m.tc_verbose = a.verbose;
  m.rc_Init();
  Walk& c = s.walk;
  size_t an = cm_count(m.unit(UF_ASCN, 0, 0), 1 << 16);
  s.asc.clear();
  for (size_t i = 0; i < an; i++) s.asc.push_back((uint8_t)m.raw(0));
  if (an) {
    s.asc.append(16, 0);
    asc_config(c.st, s.asc.data(), an, true);
  }
  code_assumed<Model<1, LV>>(m, c, 0, nullptr);
  int last = U_END;
  for (;;) {
    int kind = (int)cm_count(m.unit(UF_KIND, last, 0), U_N - 1);
    if (kind == U_END) break;
    last = kind;
    s.units++;
    if (kind == U_RAW) {
      size_t n = cm_count(m.unit(UF_RAWN, 0, 0), (i64)kChunk);
      if (n >= kPlainMin && m.raw_plain(0))
        for (size_t j = 0; j < n; j++) buf.push_back((uint8_t)m.raw_byte(0));
      else
        for (size_t j = 0; j < n; j++) buf.push_back((uint8_t)m.raw(0));
    } else {
      ModelChan<1, LV> mc;
      mc.Init(m, nullptr);
      c.start_write(&mc, cap_of(kind));
      walk_unit(c, s.rg, kind, 0, 0);
      if (c.fw.pos & 7) err_raise_s("corrupt stream: a unit of other than whole bytes");
      buf.append(c.fw.b.p, c.fw.pos / 8);
    }
    if (buf.size() > ((size_t)1 << 16)) flush();
    tell_unit(m, c, kind);
  }
  flush();
  if (m.unit(UF_CRC, 0, 0) != (s.crc.value() ^ kCrcXor)) err_raise_s("CRC mismatch: the stream did not decompress correctly");
  if (!m.input_done()) err_raise_s("corrupt stream: data after its end");
  m.tell(kYieldStats);
  return 0;
}

}  // namespace aacpack
