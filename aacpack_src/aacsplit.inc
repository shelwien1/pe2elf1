// aacsplit.inc -- the splitter of c -t (mt.inc): aacsplit's cut points, as
// a streaming coroutine for MTwrap's parts mode (h264p's h264split.inc).
//
// A part ends before the first unit (an ADTS or LOAS frame, a
// raw_data_block of ADIF or -asc, an access unit of AUS1) at least -c's part
// size after its start that parses the same at the start of a stream as in
// the middle of it; or, in a stream with none, before the first unit at
// least the cap after it (mt_cap(): 4 times the part size); or where no
// unit came within twice the cap (bytes that are not units: junk, a tag),
// there. A unit parses the same where, parsed from the state a stream
// starts with (-asc's config; ADIF's or AUS1's header), it parses to the
// values it has after the units before it (but for ADTS's fixed header,
// which a frame codes as the last one's or as its fields: the state after
// it has it), and leaves the same state, but for the SBR headers of the
// elements it has no SBR data of: the part's probe assumes those where its
// units need them (cmstream.inc). So a LOAS frame with its StreamMuxConfig,
// a unit whose SBR and PS data come with their headers; in AAC LC any unit.
// Every unit is parsed as c's probe parses it (walk_unit(), with the state
// of the units before it), and where a cut is due, again from a stream's
// state. A part of ADIF or AUS1 after the first that starts at one of their
// units has a copy of the stream's header put in before it, so that it is
// a stream of theirs too (as h264p puts the parameter sets in). Whatever a
// part holds, it is coded losslessly: a cut elsewhere costs compression,
// nothing else.
//
// Output: one frame per part, for MTwrap's parts mode (MTwrap.md section
// 11): u64 L, u64 ins, u64 xlen, u64 at, u64 xat, then the part, L - 32
// bytes, of which [ins, ins + xlen) are bytes the splitter put in, a copy
// of the input's at xat, which d takes out (the header: ins 0, xlen its
// size; none: 0, 0); at is where the part is in the input (so that c's
// messages give the input's positions).

namespace aacpack {

// n bytes of v, little-endian (MTwrap's parts mode reads a frame's size as
// a u64 of the machine's: x86's), and back
void put_le(uint8_t* p, uint64_t v, int n) {
  for (int i = 0; i < n; i++) p[i] = (uint8_t)(v >> 8 * i);
}
uint64_t get_le(const uint8_t* p, int n) {
  uint64_t v = 0;
  for (int i = 0; i < n; i++) v |= (uint64_t)p[i] << 8 * i;
  return v;
}

// The splitter's channel: the values of a unit as the log has them, but for
// the ADTS fixed header's (adts.same and its fields), which a frame codes
// against the frame before it
struct SplitChan : LogChan {
  int64_t val(int e, int n, int64_t v, int i) override {
    if (e >= E_ADTS_SAME && e <= E_ADTS_HOME) return v;
    return LogChan::val(e, n, v, i);
  }
};

struct AacSplit : Coroutine {
  // how a part ended (-v): before a unit that parses the same at a
  // stream's start, at a unit past the cap, inside bytes at twice the cap,
  // at the stream's end
  enum { CUT_CLEAN, CUT_CAP, CUT_BYTES, CUT_END, CUT_N };

  // The state, on the heap behind a pointer, Init() and Quit() its (the
  // object sits on mapped pages, never constructed: coro_new())
  struct State {
    Buf<uint8_t> buf;  // the stream from begin on: the part under way, and what was read past it
    uint64_t begin;    // the part under way: where it starts in the stream
    uint64_t pos;      // the next byte to scan
    uint64_t total;    // the bytes read
    bool eof;          // the input has ended
    bool insync;       // the last unit was a frame (scan_frames()'s)
    Walk walk;         // the units so far, parsed
    AdtsRegs rg;
    ValueLog log, log2;  // a unit's values: after the units before it, and from a stream's state
    SynState start;    // a stream's state (after its header, ADIF's or AUS1's)
    Buf<uint8_t> head;  // ADIF's or AUS1's header, put in before a part that starts at one of their units
    uint64_t head_at;  // where it is in the stream
    bool units;        // the scan is at those units: a cut there starts a part with the header
    bool headed;       // the part under way starts with it
    uint64_t parts, cuts[CUT_N];
    void Init(const AacArgs& a) {
      buf.Init();
      begin = pos = total = 0;
      eof = insync = false;
      walk.Init();
      rg.Init();
      log.Init();
      log2.Init();
      if (a.asc_n) asc_config(walk.st, a.asc, a.asc_n, true);
      start = walk.st;
      head.Init();
      head_at = 0;
      units = headed = false;
      parts = 0;
      memset(cuts, 0, sizeof cuts);
    }
    void Quit() {
      buf.Quit();
      walk.Quit();
      rg.Quit();
      log.Quit();
      log2.Quit();
      head.Quit();
    }
  };
  State* st;
  uint64_t chunksize, cap;
  const AacArgs* a;  // the container looked for, -asc

  uint Init(uint64_t _chunksize, uint64_t _cap, const AacArgs* _a) {
    coro_init();
    chunksize = _chunksize;
    cap = _cap;
    a = _a;
    st = (State*)mem_alloc(sizeof(State));
    st->Init(*a);
    return 0;
  }
  void Quit() {
    if (st) st->Quit(), mem_free(st);
    st = nullptr;
  }

  // the input from pos on, at least k bytes of it (fewer where it ends
  // first): how many there are, at most k
  size_t need(size_t k) {
    State& s = *st;
    const size_t off = (size_t)(s.pos - s.begin);
    while (s.buf.size() - off < k && !s.eof) {
      chkinp();
      uint l = getinplen();
      if (!l) {
        s.eof = true;
        break;
      }
      s.buf.append((const uint8_t*)inpptr, l);
      inpptr += l;
      s.total += l;
    }
    return Min(s.buf.size() - off, k);
  }
  const uint8_t* at() const { return st->buf.data() + (st->pos - st->begin); }

  void put_bytes(const uint8_t* p, size_t n) {
    while (n) {
      chkout();
      uint l = (uint)Min<size_t>(n, getoutleft());
      memcpy((byte*)outptr, p, l);
      outptr += l, p += l, n -= l;
    }
  }
  // the part under way, up to the stream's byte end, as a frame (cut there
  // for the reason why), with the header put in where it starts at a unit
  // of ADIF or AUS1 (the bytes [0, hl), which d takes out)
  void emit(uint64_t end, int why) {
    State& s = *st;
    const uint64_t n = end - s.begin;
    const size_t hl = s.headed ? s.head.size() : 0;
    uint8_t h[40];
    put_le(h, 32 + hl + n, 8);
    put_le(h + 8, 0, 8);
    put_le(h + 16, hl, 8);
    put_le(h + 24, s.begin, 8);
    put_le(h + 32, s.head_at, 8);
    put_bytes(h, sizeof h);
    put_bytes(s.head.data(), hl);
    put_bytes(s.buf.data(), (size_t)n);
    const size_t rest = s.buf.size() - (size_t)n;  // (what was read past it: little)
    memmove(s.buf.data(), s.buf.data() + n, rest);
    s.buf.resize(rest);
    s.begin = end;
    s.headed = s.units;
    s.parts++;
    s.cuts[why]++;
  }
  // a cut at pos where there is none at a unit within twice the cap
  void cut_bytes() {
    if (st->pos - st->begin >= 2 * cap) emit(st->pos, CUT_BYTES);
  }
  // n bytes that are not a unit (fewer where the stream ends first)
  void skip(uint64_t n) {
    while (n) {
      cut_bytes();
      size_t k = need((size_t)Min<uint64_t>(n, Min<uint64_t>(1 << 16, 2 * cap - (st->pos - st->begin))));
      if (!k) return;
      st->pos += k, n -= k;
    }
  }

  // The unit of the kind at pos (len bytes; 0 as long as it parses to be,
  // of the avail there are), parsed from the walk's state into log: its
  // length, 0 where it did not parse (the walk's state then as it was)
  size_t parse(int kind, size_t len, size_t avail, ValueLog& log) {
    Walk& c = st->walk;
    const SynState before = c.st;
    log.clear();
    SplitChan ch;
    ch.Init(log, false);
    c.start_read(&ch, at());
    c.rests = 0;
    size_t end = 0;
    ErrTrap t;
    if (setjmp(t.jb)) {
      c.st = before;
      return 0;
    }
    err_push(t);
    end = walk_unit(c, st->rg, kind, len, avail);
    err_pop(t);
    return end;
  }
  // whether the unit at pos, parsed after the units before it (end1 bytes,
  // rests1 rests, the walk's state after it as it is), parses the same from
  // a stream's state (above); the walk's state after it as it was
  bool same_fresh(int kind, size_t len, size_t avail, size_t end1, int rests1) {
    State& s = *st;
    Walk& c = s.walk;
    const SynState after = c.st;
    c.st = s.start;
    size_t end2 = parse(kind, len, avail, s.log2);
    // (of the state but the SBR headers, what the unit's container sets:
    // a stream that changes containers keeps the other's, which no unit of
    // this one reads -- ADTS's fixed header, LATM's StreamMuxConfig)
    SynState x = after, y = c.st;
    auto mine = [kind](SynState& z) {
      if (kind == U_ADTS) z.chan_config = z.latm_cfg = z.mux_version = z.num_subframes = z.other_present = z.other_bits = 0;
      if (kind == U_LOAS) z.have_fixed = 0, memset((void*)&z.fixed, 0, sizeof z.fixed);
    };
    mine(x);
    mine(y);
    bool same = end2 == end1 && c.rests == rests1 && s.log.v.size() == s.log2.v.size() &&
                !memcmp(s.log.v.data(), s.log2.v.data(), s.log.v.size() * sizeof(int64_t)) &&
                !memcmp(&x, &y, offsetof(SynState, sbr));
    // (an element's SBR state: the same where the unit set it up afresh;
    // one it has no SBR data of the part's probe assumes, cmstream.inc)
    for (int k = 0; k < 3 && same; k++)
      for (int t = 0; t < 16 && same; t++)
        if (memcmp(&c.st.sbr[k][t], &s.start.sbr[k][t], sizeof(SbrState)))
          same = !memcmp(&c.st.sbr[k][t], &after.sbr[k][t], sizeof(SbrState));
    c.st = after;
    return same;
  }
  // The unit of the kind at pos, len bytes (0: as long as it parses to
  // be), parsed, and a cut before it if one is due: its length (0 where it
  // did not parse and has no length of its own)
  size_t unit(int kind, size_t len, size_t avail) {
    State& s = *st;
    size_t end = parse(kind, len, avail, s.log);
    const int rests = s.walk.rests;
    const uint64_t n = s.pos - s.begin;
    if (n >= chunksize && n > 0) {
      if (end && same_fresh(kind, len, avail, end, rests))
        emit(s.pos, CUT_CLEAN);
      else if (n >= cap)
        emit(s.pos, CUT_CAP);
    }
    return end ? end : len;
  }

  // ADTS and LOAS frames, anything between them as bytes (cmstream.inc's
  // scan_frames())
  void scan_frames() {
    State& s = *st;
    for (;;) {
      cut_bytes();
      size_t avail = need(kLook);
      if (!avail) return;
      const uint8_t* p = at();
      uint64_t tl = id3v2_len(p, avail);
      if (tl) {
        skip(tl);
        s.insync = false;
        continue;
      }
      int kind = 0;
      size_t len = 0;
      for (int f = F_ADTS; f <= F_LOAS && !kind; f++) {
        if (a->format && a->format != f) continue;
        len = f == F_ADTS ? adts_frame_len(p, avail) : loas_frame_len(p, avail);
        if (!len || len > avail) continue;
        size_t nl = f == F_ADTS ? adts_frame_len(p + len, avail - len) : loas_frame_len(p + len, avail - len);
        if (s.insync || avail == len || nl) kind = f == F_ADTS ? U_ADTS : U_LOAS;
      }
      if (!kind) {
        s.pos++;
        s.insync = false;
        continue;
      }
      s.pos += unit(kind, len, avail);
      s.insync = true;
    }
  }
  // back-to-back raw_data_blocks (-asc's, ADIF's), to one that does not
  // parse (the rest bytes)
  void scan_blocks() {
    State& s = *st;
    for (;;) {
      cut_bytes();
      size_t avail = need(kBlockMax);
      if (!avail) return;
      size_t l = unit(U_BLOCK, 0, avail);
      if (!l) return;
      s.pos += l;
    }
  }

  // the header of ADIF or AUS1 at pos, end bytes, after which the scan is
  // at their units: its copy, and the state a part's stream has after it
  void header(size_t end) {
    State& s = *st;
    s.head.clear();
    s.head.append(at(), end);
    s.head_at = s.pos;
    s.pos += end;
    s.start = s.walk.st;
    s.units = true;
  }
  // ADIF (cmstream.inc's Scanner::adif()): its header, then its
  // raw_data_blocks if it has any it parses (false: the header does not
  // parse)
  bool adif() {
    State& s = *st;
    size_t end = parse(U_ADIF, 0, need(kAdifMax), s.log);
    if (!end) return false;
    header(end);
    if (s.walk.st.blocks) scan_blocks();
    s.units = false;
    return true;
  }
  // AUS1 (Scanner::aus1()): its header, then the access units, to one
  // longer than kAuMax or cut short (false: not AUS1)
  bool aus1() {
    State& s = *st;
    size_t avail = need(6);
    if (avail < 6 || memcmp(at(), "AUS1", 4)) return false;
    const size_t hl = 6 + get_le(at() + 4, 2);
    if (need(hl) < hl) return false;
    parse(U_AUS1, hl, hl, s.log);
    header(hl);
    for (;;) {
      cut_bytes();
      if (need(4) < 4) break;
      uint64_t n = get_le(at(), 4);
      if (n > kAuMax || need((size_t)n + 4) < (size_t)n + 4) break;
      s.pos += unit(U_AU, (size_t)n + 4, (size_t)n + 4);
    }
    s.units = false;
    return true;
  }

  void do_process() {
    State& s = *st;
    // the tags first, then as cmstream.inc's Scanner: raw access units
    // (-asc), ADIF, AUS1, or frames
    for (;;) {
      size_t avail = need(10);
      uint64_t tl = id3v2_len(at(), avail);
      if (!tl) break;
      skip(tl);
    }
    // (tags cut in bytes: the part of their rest ends with them, so that
    // the header of ADIF or AUS1 starts a part, where its scan looks for it)
    if (s.begin && s.pos > s.begin) emit(s.pos, CUT_BYTES);
    size_t avail = need(4);
    if (a->asc_n) {
      scan_blocks();
    } else if ((a->format == F_AUTO || a->format == F_ADIF) && avail >= 4 && !memcmp(at(), "ADIF", 4) && adif()) {
    } else if (a->format == F_AUTO && aus1()) {
    } else {
      scan_frames();
    }
    skip(~(uint64_t)0);
    if (s.total > s.begin) emit(s.total, CUT_END);
    yield(this, 0);
  }
};

}  // namespace aacpack
