// mt.inc -- c -t N and d of its streams: the stream in parts, on threads (MTwrap).

// c -t N: the stream cut into parts by M4vSplit (on the main thread), each part a stream by itself,
// and the parts dealt in turn to N workers by MTwrap (MTwrap_v3, its parts mode), a model for each
// part (M4vPart runs an M4vCoro on it). A part ends:
//   - before the first I-VOP at least -c's part size after the part's start, or before the run of
//     headers (VOS, VO, VOL, user data, GOV) right before that I-VOP;
//   - where no I-VOP comes, before the first VOP but a B-VOP at least mt_cap() after its start (a
//     B-VOP's syntax depends on the VOP before it);
//   - where no VOP comes within half again of the cap, at any start code, and where none comes
//     within twice the cap, there.
// VOPs that aren't coded (vop_coded 0: the placeholders packed B-VOPs leave) don't count as I-VOPs
// or other VOPs here. The headers m4vpack's parser reads (the last VOL, the last DivX and XviD
// user data), as defined before a part, are copied in at its start (their first 32 KB, which is
// what the parser reads), unless the part defines them itself before its first VOP: so each part
// parses as it would in the whole stream, but where the parser guessed the VOP header's time
// increment size.
//
// A worker's stream is a record for each of its parts:
//   u64 g_size  the record's size, these MT_REC (48) bytes included
//   u64 plen    the size of the part as compressed (with the headers copied in)
//   u64 ins     where in it those are: [ins, ins + xlen), removed after decoding
//   u64 xlen
//   u64 part    its place in the stream, from 0: worker w's k-th part is w + kN
//   u32 crc     the CRC-32 of these 40 bytes; u32 0
//   g_size - 48 bytes: the part compressed, a stream of the single kind (magic, version, level,
//               chunks, CRC)
// and its end, a record of the 48 bytes alone, whose part is the place its next part would have
// had (little endian). So d finds a part lost, added or moved, or a worker's stream cut short.
// MTwrap's container holds the N streams, after m4vpack's header: mt_magic, the format version,
// the threads, the part size in KB (4 bytes). d needs as many threads as c had; each part ends a
// round of MTwrap's decoder (Y_PART), which takes the parts from the workers in turn, as dealt.

// the header: mt_magic, the version, the threads, the part size in KB (MT_PART_BYTES)
enum { MT_MAGIC = 5, MT_AT_VERSION = MT_MAGIC, MT_AT_THREADS, MT_AT_PART, MT_PART_BYTES = U32_BYTES,
       MT_HEAD = MT_AT_PART + MT_PART_BYTES };
static const U8 mt_magic[MT_MAGIC] = { 'm', '4', 'v', 'p', 'm' };
constexpr int HEAD_MAX = (int)MT_HEAD > (int)M4V_HEAD ? (int)MT_HEAD : (int)M4V_HEAD;  // either kind of stream's

constexpr int MAX_THREADS = 64;  // (-t)
// a worker's record: its fields' places, and its size
enum {
  MT_R_SIZE = 0,                        // u64 g_size
  MT_R_PLEN = MT_R_SIZE + U64_BYTES,    // u64 plen
  MT_R_INS = MT_R_PLEN + U64_BYTES,     // u64 ins
  MT_R_XLEN = MT_R_INS + U64_BYTES,     // u64 xlen
  MT_R_PART = MT_R_XLEN + U64_BYTES,    // u64 part
  MT_R_CRC = MT_R_PART + U64_BYTES,     // u32 crc, of the fields before it
  MT_R_ZERO = MT_R_CRC + U32_BYTES,     // u32 0
  MT_REC = MT_R_ZERO + U32_BYTES
};
// a part as the splitter frames it: u64 L, u64 ins, u64 xlen, then L - MT_F_PAST bytes (L counts
// what follows it)
enum {
  MT_F_LEN = 0,
  MT_F_INS = MT_F_LEN + U64_BYTES,
  MT_F_XLEN = MT_F_INS + U64_BYTES,
  MT_FRAME = MT_F_XLEN + U64_BYTES,
  MT_F_PAST = MT_FRAME - MT_F_INS
};
constexpr U32 MT_RING = 1 * MIB;       // MTwrap's buffers to and from each worker
constexpr U32 MT_COPY_MAX = HDR_PEEK;  // the bytes of a header copied in, at most (what the parser reads)
constexpr unsigned MTW_NO_THREAD = 7;  // MTwrap's Init(): a thread didn't start (1-6: no memory)

static void put_le(U8* p, U64 v, int n) { int i; for (i = 0; i < n; i++) p[i] = (U8)(v >> (BYTE_BITS * i)); }
static U64 get_le(const U8* p, int n) {
  U64 v = 0;
  int i;
  for (i = n - 1; i >= 0; i--) v = (v << BYTE_BITS) | p[i];
  return v;
}

// the headers copied in
enum { K_VOL, K_DIVX, K_XVID, NKEY };
// the size past which a part ends at any VOP but a B-VOP: MT_CAP_PARTS part sizes, to MT_CAP_MAX
constexpr U64 MT_CAP_PARTS = 4, MT_CAP_MAX = 1ull << 31;
static U64 mt_cap(U64 part) { return MT_CAP_PARTS * part < MT_CAP_MAX ? MT_CAP_PARTS * part : MT_CAP_MAX; }
// where no VOP comes, a part ends at any start code past half again of the cap (mt_soft()), and
// anywhere at twice the cap (mt_hard())
static U64 mt_soft(U64 cap) { return cap + cap / 2; }
static U64 mt_hard(U64 cap) { return 2 * cap; }
// The most a part can be, as the splitter cuts it: mt_hard(), and the headers copied in. d takes a
// record of a larger part to be broken, which bounds the memory a part's output takes.
static U64 mt_part_max(U64 part) { return mt_hard(mt_cap(part)) + NKEY * (U64)MT_COPY_MAX; }

static void mt_head(U8* h, int threads, U32 part) {
  memcpy(h, mt_magic, sizeof mt_magic);
  h[MT_AT_VERSION] = M4V_VERSION;
  h[MT_AT_THREADS] = (U8)threads;
  put_le(h + MT_AT_PART, part / KIB, MT_PART_BYTES);
}

static U32 mt_rec_crc(const U8* h) { return crc32_upd(0, h, MT_R_CRC); }
static void mt_rec(U8* h, U64 g_size, U64 plen, U64 ins, U64 xlen, U64 part) {
  put_le(h + MT_R_SIZE, g_size, U64_BYTES);
  put_le(h + MT_R_PLEN, plen, U64_BYTES);
  put_le(h + MT_R_INS, ins, U64_BYTES);
  put_le(h + MT_R_XLEN, xlen, U64_BYTES);
  put_le(h + MT_R_PART, part, U64_BYTES);
  put_le(h + MT_R_CRC, mt_rec_crc(h), U32_BYTES);
  put_le(h + MT_R_ZERO, 0, U32_BYTES);
}

// room for n bytes at *p (*room of them now): 0 if there is no memory
static int grow(U8** p, size_t* room, size_t n) {
  size_t r = *room ? *room : IO_BUF;
  U8* q;
  if (n <= *room) return 1;
  while (r < n) r = r < ((size_t)-1) / 2 ? 2 * r : n;
  if (!(q = (U8*)realloc(*p, r))) return 0;
  *p = q;
  *room = r;
  return 1;
}

// Whether the VOP at p (n bytes from its start code on) is coded (vop_coded), as the parser reads it
// with the VOL c has (and its guess of the time increment's length, kept in c); -1 if its header
// doesn't parse
constexpr size_t VOP_PEEK = 64;  // the bytes of a VOP's header (from its start code on) vop_is_coded() reads
static int vop_is_coded(Ctx* c, const U8* p, size_t n) {
  U8 b[VOP_PEEK + GB_PAD];
  GB gb;
  VopHdr h;
  const size_t k = n - SC_LEN < VOP_PEEK ? n - SC_LEN : VOP_PEEK;
  memset(b, 0, sizeof b);
  memcpy(b, p + SC_LEN, k);
  gb.buf = b;
  gb.size = (int)k * BYTE_BITS;
  gb.pos = 0;
  h.coded = -1;
  parse_vop_hdr(c, &gb, &h);  // (coded as far as it got: the rest of the header doesn't matter here)
  return h.coded;
}

// The splitter: the stream in, its parts out as frames for MTwrap's parts mode (u64 L, u64 ins,
// u64 xlen, the part with the headers copied in at ins), on the main thread. Plain data (a part of
// MtEnc, mapped by coro_new()): Init() sets it up, Quit() frees what it holds.
struct M4vSplit : Coroutine {
  U64 chunksize, cap;  // -c's part size; mt_cap() of it
  U8* buf;             // the stream from begin on: the part under way
  size_t len, room;
  U64 begin, scan;     // the part's start; where to look for start codes next (stream offsets)
  U64 total, parts;    // the stream's size (at its end), the parts
  int eof;
  // the chunk under way (from its start code at chunk_at), and the run of headers it may be in
  U64 chunk_at, run_at;
  int chunk_code, run;  // chunk_code: the start code's last byte, -1 (none, or the part started in it)
  // the headers to copy in: as defined before the run under way (copy) and in it (pend, keys in
  // pend_has); the copies of the part under way (extra, xlen bytes)
  U8 copy[NKEY][MT_COPY_MAX], pend[NKEY][MT_COPY_MAX];
  U32 copy_len[NKEY], pend_len[NKEY];
  int pend_has;
  U8 extra[NKEY * MT_COPY_MAX];
  U32 xlen;
  Ctx cx;  // the last VOL's (whether a VOP is coded)

  void Init(U64 part) {
    coro_init();
    ctx_init(&cx);
    chunksize = part;
    cap = mt_cap(part);
    buf = 0;
    len = room = 0;
    begin = scan = total = parts = 0;
    eof = 0;
    chunk_at = run_at = 0;
    chunk_code = -1;
    run = 0;
    memset(copy_len, 0, sizeof copy_len);
    memset(pend_len, 0, sizeof pend_len);
    pend_has = 0;
    xlen = 0;
  }
  void Quit(void) {
    free(buf);
    buf = 0;
    len = room = 0;
    ctx_quit(&cx);
  }

  void put(const U8* p, size_t n) {
    while (n) {
      uint l;
      chkout();
      l = (uint)(n < getoutleft() ? n : getoutleft());
      memcpy(outptr, p, l);
      outptr += l;
      p += l;
      n -= l;
    }
  }

  // the part under way, up to the stream offset x, as a frame; the next starts at x
  void emit(U64 x) {
    const size_t n = (size_t)(x - begin);
    U8 h[MT_FRAME];
    put_le(h + MT_F_LEN, MT_F_PAST + (U64)xlen + n, U64_BYTES);
    put_le(h + MT_F_INS, 0, U64_BYTES);
    put_le(h + MT_F_XLEN, xlen, U64_BYTES);
    put(h, MT_FRAME);
    put(extra, xlen);
    put(buf, n);
    memmove(buf, buf + n, len - n);
    len -= n;
    begin = x;
    parts++;
  }
  // a cut at x: the part before it out, and the copies for the one after it (the headers defined
  // before x, but those its run of headers so far defines)
  void cut(U64 x) {
    int k;
    emit(x);
    xlen = 0;
    for (k = 0; k < NKEY; k++)
      if (copy_len[k] && !(pend_has >> k & 1)) {
        memcpy(extra + xlen, copy[k], copy_len[k]);
        xlen += copy_len[k];
      }
    if (chunk_at < x) chunk_code = -1;  // (a cut inside a chunk: its start is gone)
    if (run_at < x) run_at = x;
  }
  // the run of headers ends: what it defined is defined before the next run
  void commit(void) {
    int k;
    for (k = 0; k < NKEY; k++)
      if (pend_has >> k & 1) {
        memcpy(copy[k], pend[k], pend_len[k]);
        copy_len[k] = pend_len[k];
      }
    pend_has = 0;
    run = 0;
  }
  static int is_header(int c) {
    return c == SC_VOS || c == SC_VISOBJ || c == SC_USER_DATA || c == SC_GOV || (c >= SC_VO && c < SC_VOL_END);
  }
  // user data (n bytes from the start code on) that starts with tag
  static int tagged(const U8* p, size_t n, const char* tag) {
    const size_t l = strlen(tag);
    return n >= SC_LEN + l && !memcmp(p + SC_LEN, tag, l);
  }
  // the chunk under way ends at the stream offset a: a header to copy into pend
  void end_chunk(U64 a) {
    const U8* p = buf + (chunk_at - begin);
    const size_t n = (size_t)(a - chunk_at);
    int k = -1;
    if (chunk_code < 0) return;
    if (sc_is_vol(chunk_code)) parse_header_chunk(&cx, p, n);
    if (sc_is_vol(chunk_code)) k = K_VOL;
    else if (chunk_code == SC_USER_DATA && tagged(p, n, "DivX")) k = K_DIVX;
    else if (chunk_code == SC_USER_DATA && tagged(p, n, "XviD")) k = K_XVID;
    if (k < 0) return;
    pend_len[k] = (U32)(n < MT_COPY_MAX ? n : MT_COPY_MAX);
    memcpy(pend[k], p, pend_len[k]);
    pend_has |= 1 << k;
  }
  // a start code at the stream offset a: the chunk before it ends, and a cut before this one (or
  // before its run of headers) if one is due. A VOP that isn't coded (as packed B-VOPs leave) is no
  // place to cut: the B-VOPs after it depend on the coded VOP before it.
  void start_code(U64 a) {
    const size_t i = (size_t)(a - begin);
    const int code = i + SC_PREFIX < len ? buf[i + SC_PREFIX] : -1;
    int type = code == SC_VOP && i + SC_LEN < len ? vop_type(buf + i) : 0;  // a VOP's (PT_I ...), 0 none
    U64 x;
    end_chunk(a);
    if (type && !vop_is_coded(&cx, buf + i, len - i)) type = 0;
    if (is_header(code) && !run) {
      run = 1;
      run_at = a;
    }
    x = run ? run_at : a;
    if (x > begin && ((type == PT_I && x - begin >= chunksize) || (type && type != PT_B && x - begin >= cap) ||
                      a - begin >= mt_soft(cap)))
      cut(x);
    if (!is_header(code)) commit();
    chunk_at = a;
    chunk_code = code;
  }

  void do_process(void) {
    for (;;) {
      uint l;
      chkinp();
      l = getinplen();
      if (!l) break;  // the end
      if (!grow(&buf, &room, len + l + GB_PAD)) no_memory();
      memcpy(buf + len, inpptr, l);
      inpptr += l;
      len += l;
      scan_input();
    }
    eof = 1;
    scan_input();
    total = begin + len;
    end_chunk(total);
    commit();
    if (total > begin) emit(total);
    yield(this, Y_END);
  }
  // the start codes in what came of the input, but one whose code (and VOP type) is to come; and
  // where a stretch without one reaches past twice the cap, a cut there
  void scan_input(void) {
    for (;;) {
      size_t i = (size_t)(scan - begin);
      const int found = find_sc(buf, &i, len);
      const U64 a = begin + i;  // the start code, or where to look on
      while (a - begin > mt_hard(cap)) cut(begin + mt_hard(cap));
      i = (size_t)(a - begin);
      if (!found || (i + VOP_PEEK > len && !eof)) {  // (a VOP's header to come)
        scan = a;
        return;
      }
      scan = a + SC_PREFIX;
      start_code(a);
    }
  }
};

typedef struct MtArgs {
  int threads, stats;
  int level;        // c: the model's
  U64 part_max;     // d: the largest part a record can have (mt_part_max())
  Progress* meter;  // the call's progress line, which the workers' runs tell and poll (0: none)
} MtArgs;

// A part's run at the level LV, by a worker W (M4vPart): c of the part in its in_ (plen bytes),
// its statistics added to the worker's
template <class LV>
struct PartC {
  template <class W>
  static int run(W& w, U64 plen) {
    M4vCoro<0, LV>* c = coro_new<M4vCoro<0, LV> >();
    U64 pos = 0;
    int r;
    if (!c) return w.fail(E_INTERNAL, "out of memory");
    c->Init(w.args.stats);
    r = w.run_part(c, [&](const U8** p) {  // in_, FEED_MAX at most at a time (addinp() takes 32 bits)
      constexpr U32 FEED_MAX = 1024 * MIB;
      const size_t l = plen - pos < FEED_MAX ? (size_t)(plen - pos) : FEED_MAX;
      *p = w.in_ + pos;
      pos += l;
      return l;
    }, ~0ull);
    if (!r) w.add_stats(c->st, c->co.cost, c->co.orig);
    coro_free(c);
    return r;
  }
};
// and d of a part's record, its stream's header (nh bytes at head) read already, the rest (left
// bytes) from the worker's input, IO_BUF at a time; plen its size decompressed
template <class LV>
struct PartD {
  template <class W>
  static int run(W& w, const U8* head, size_t nh, U64 plen, U64& left) {
    M4vCoro<1, LV>* c = coro_new<M4vCoro<1, LV> >();
    int first = 1, r;
    if (!c) return w.fail(E_INTERNAL, "out of memory");
    c->Init(0);
    r = w.run_part(c, [&](const U8** p) {
      size_t got;
      if (first) {
        first = 0;
        *p = head;
        return nh;
      }
      got = w.read(w.in_, left < IO_BUF ? (size_t)left : IO_BUF);
      left -= got;
      *p = w.in_;
      return got;
    }, plen);
    coro_free(c);
    return r;
  }
};

// MTwrap's Enc_class: a worker, coding the parts it is dealt one after the other, each by a model
// of its own (M4vCoro, a coroutine of the worker's, on the part in memory). Plain data, never
// constructed (MTwrap maps its workers): Init() sets it up, do_process() frees what it took as it
// ends. What a part's run fails with is the worker's error (its note).
template <int f_DEC>
struct M4vPart : CoIo {
  MtArgs args;
  U32 worker;      // its place among the workers, dealt the parts in turn
  U64 parts;       // the parts done
  Stats st;        // c: its parts', added up (-s: and the bits by kind of syntax element)
  double cost[NCAT], orig[NCAT];
  U8 *in_, *out_;  // c: a part, all of it; d: IO_BUF of its record at a time. The part's run's output
  size_t in_room, out_room, out_len;
  // its counts on the progress line: the stream's bytes, the compressed stream's and the VOPs of its
  // parts done (done_*, the worker's own), and with the part under way (prog_*: read from any
  // thread)
  U64 done_raw, done_packed, done_vops;
  U64 prog_raw, prog_packed, prog_vops;

  uint Init(const MtArgs* a, uint w) {
    coro_init();
    args = *a;
    worker = w;
    parts = 0;
    memset(&st, 0, sizeof st);
    memset(cost, 0, sizeof cost);
    memset(orig, 0, sizeof orig);
    in_ = out_ = 0;
    in_room = out_room = out_len = 0;
    done_raw = done_packed = done_vops = 0;
    prog_set(0, 0, 0);
    return 0;
  }
  void prog_set(U64 raw, U64 packed, U64 vops) {
    __atomic_store_n(&prog_raw, raw, __ATOMIC_RELAXED);
    __atomic_store_n(&prog_packed, packed, __ATOMIC_RELAXED);
    __atomic_store_n(&prog_vops, vops, __ATOMIC_RELAXED);
  }
  void prog_get(U64& raw, U64& packed, U64& vops) const {
    raw += __atomic_load_n(&prog_raw, __ATOMIC_RELAXED);
    packed += __atomic_load_n(&prog_packed, __ATOMIC_RELAXED);
    vops += __atomic_load_n(&prog_vops, __ATOMIC_RELAXED);
  }
  void Quit(void) {
    free(in_);
    free(out_);
    in_ = out_ = 0;
    in_room = out_room = 0;
  }

  void do_process(void) {
    int r;
    io_init();
    r = f_DEC ? run_d() : run_c();
    Quit();
    for (;;) yield(static_cast<Coroutine*>(this), r ? Y_FAIL : Y_END);
  }

  // the place of the next part it has (worker w's k-th: w + kN)
  U64 next_part(void) const { return worker + parts * (U64)args.threads; }

  // c: a part's statistics, added to the worker's
  void add_stats(const Stats& s, const double* c, const double* o) {
    int i;
    st.vops += s.vops;
    st.packed += s.packed;
    for (i = 0; i < PT_N; i++) st.by_type[i] += s.by_type[i];
    st.raw_unsup += s.raw_unsup;
    st.raw_long += s.raw_long;
    st.raw_parse += s.raw_parse;
    st.raw_verify += s.raw_verify;
    st.raw_bytes += s.raw_bytes;
    st.vop_bytes += s.vop_bytes;
    for (i = 0; i < NCAT; i++) cost[i] += c[i], orig[i] += o[i];
  }

  // A part's run: the coroutine c on the input feed() gives (feed(&p): the next piece at p, its size;
  // 0 at the end), its output into out_ (out_len bytes; most at most, one more fails the part); its
  // status (0, or the exit status of its error, its note copied to the worker's). As it goes, its
  // counts are the worker's (from done_* on), what it tells is shown, and a line of progress may be
  // due.
  template <class C, class Feed>
  int run_part(C* c, Feed feed, U64 most) {
    int r;
    uint y;
    out_len = 0;
    if (!grow(&out_, &out_room, IO_BUF)) return fail(E_INTERNAL, "out of memory");
    c->addout(out_, IO_BUF);
    for (;;) {
      y = c->coro_call(c);
      if (args.meter) {
        if (y == Y_VIDEO) args.meter->video(c->video);
        prog_set(done_raw + c->raw_done(), done_packed + c->packed_done(), done_vops + c->nvop);
        args.meter->poll();
      }
      if (y == Y_VIDEO) continue;
      if (y == Y_IN) {
        const U8* p = 0;
        const size_t l = feed(&p);
        if (!l) c->f_quit = 1;
        c->addinp((byte*)(l ? p : out_), (uint)l);
        continue;
      }
      out_len += c->getoutsize();
      if (y != Y_OUT) break;
      if (out_len > most) {  // (a part that comes out longer than its record says: d only)
        c->Quit();
        return fail(E_DATA, "damaged data: a part comes out longer than its record says");
      }
      if (!grow(&out_, &out_room, out_len + IO_BUF)) {
        c->Quit();
        return fail(E_INTERNAL, "out of memory");
      }
      c->addout(out_ + out_len, IO_BUF);
    }
    r = c->status;
    if (r) fail(r, "%s", c->note);
    done_raw += c->raw_done();
    done_packed += c->packed_done();
    done_vops += c->nvop;
    return r;
  }

  // c: each part as the splitter frames it (u64 L, u64 ins, u64 xlen, then the part, L - MT_F_PAST
  // bytes) into its record, and as the parts end, the record of the worker's end
  int run_c(void) {
    for (;;) {
      U8 h[MT_FRAME], rec[MT_REC];
      const size_t n = read(h, MT_FRAME);
      U64 L, ins, xlen, plen;
      int r;
      if (!n) {
        mt_rec(rec, MT_REC, 0, 0, 0, next_part());
        write(rec, MT_REC);
        return 0;
      }
      L = get_le(h + MT_F_LEN, U64_BYTES);
      ins = get_le(h + MT_F_INS, U64_BYTES);
      xlen = get_le(h + MT_F_XLEN, U64_BYTES);
      plen = L - MT_F_PAST;
      if (n < MT_FRAME || L < MT_F_PAST || ins > plen || xlen > plen - ins || plen != (size_t)plen)
        return fail(E_INTERNAL, "internal error: a part framed wrong");
      if (!grow(&in_, &in_room, (size_t)plen + 1)) return fail(E_INTERNAL, "out of memory");
      if (read(in_, (size_t)plen) != plen) return fail(E_INTERNAL, "internal error: a part cut short");
      r = with_level<PartC>(args.level, *this, plen);
      if (r == kNoLevel) return fail(E_INTERNAL, "no level -%d in this build", args.level);
      if (r) return r;
      mt_rec(rec, MT_REC + out_len, plen, ins, xlen, next_part());
      write(rec, MT_REC);
      write(out_, out_len);
      parts++;
      done_packed += MT_REC;
      done_raw -= xlen;  // (the headers copied in are not the stream's)
      // the record handed over now, its last buffer as far as it is filled (a round's end): so
      // MTwrap puts each record in the container as its part is done, and d, which takes the
      // parts in turn, finds the next within about a round of the others
      yield(static_cast<Coroutine*>(this), Y_PART);
    }
  }

  // d: each record into its part, the headers copied in removed, after which MTwrap takes the
  // next part from the next worker; to the record of the worker's end, after which nothing
  int run_d(void) {
    for (;;) {
      U8 h[MT_REC];
      U8 head[M4V_HEAD];
      U64 g_size, plen, ins, xlen, left;
      size_t nh;
      int r;
      if (read(h, MT_REC) < MT_REC) return fail(E_DATA, "damaged data: a worker's records cut short");
      g_size = get_le(h + MT_R_SIZE, U64_BYTES);
      plen = get_le(h + MT_R_PLEN, U64_BYTES);
      ins = get_le(h + MT_R_INS, U64_BYTES);
      xlen = get_le(h + MT_R_XLEN, U64_BYTES);
      if (get_le(h + MT_R_CRC, U32_BYTES) != mt_rec_crc(h) || get_le(h + MT_R_ZERO, U32_BYTES) ||
          get_le(h + MT_R_PART, U64_BYTES) != next_part())
        return fail(E_DATA, "damaged data: a part's record broken, or a part lost, added or moved");
      if (g_size == MT_REC) {  // the worker's end
        U8 x;
        if (plen || ins || xlen) return fail(E_DATA, "damaged data: a part's record broken");
        if (read(&x, 1)) return fail(E_DATA, "damaged data after a worker's end");
        return 0;
      }
      if (plen > args.part_max || ins > plen || xlen > plen - ins || g_size < MT_REC)
        return fail(E_DATA, "damaged data: a part's record broken");
      // the part compressed, read as its run asks for it, IO_BUF at a time: its header here (its
      // level), then the rest of g_size from the input
      left = g_size - MT_REC;
      nh = (size_t)(left < (U64)M4V_HEAD ? left : (U64)M4V_HEAD);
      if (read(head, nh) < nh || nh < M4V_HEAD) return fail(E_DATA, "damaged data: a part's record cut short");
      left -= nh;
      if (!grow(&in_, &in_room, IO_BUF)) return fail(E_INTERNAL, "out of memory");
      r = with_level<PartD>(head[M4V_AT_LEVEL], *this, (const U8*)head, nh, plen, left);
      if (r == kNoLevel)
        return fail(E_DATA, "a part compressed at level -%d, which this build has not", head[M4V_AT_LEVEL]);
      if (r) return r;
      if (left) return fail(E_DATA, "damaged data: a part's record broken");  // (its stream ended before it)
      if (out_len != plen) return fail(E_DATA, "damaged data: a part comes out shorter than its record says");
      write(out_, (size_t)ins);
      write(out_ + ins + xlen, (size_t)(plen - ins - xlen));
      parts++;
      done_packed += MT_REC;
      done_raw -= xlen;  // (the headers copied in are not the stream's)

      yield(static_cast<Coroutine*>(this), Y_PART);
    }
  }
};

// c: the splitter into MTwrap's coder; d: MTwrap's decoder
typedef CoroutinePair<M4vSplit, MTwrap<0, M4vPart<0>, const MtArgs*> > MtEnc;
typedef MTwrap<1, M4vPart<1>, const MtArgs*> MtDec;

// whether a worker failed: the first one's exit status, its note copied to note (the workers go
// with MTwrap's Quit())
template <class M>
int mt_failed(M& m, char* note, size_t n) {
  uint i;
  for (i = 0; i < m.th_N; i++)
    if (m.C1[i].f_fail) {
      snprintf(note, n, "%s", m.C1[i].note);
      return m.C1[i].code ? m.C1[i].code : E_INTERNAL;
    }
  return 0;
}
// whether d's workers had the parts as c deals them, in turn: of P in all, worker i the i-th of each
// round of N, one more where P % N is past it
static int mt_dealt(MtDec& d) {
  U64 P = 0, N = d.th_N;
  uint i;
  for (i = 0; i < d.th_N; i++) P += d.C1[i].parts;
  for (i = 0; i < d.th_N; i++)
    if (d.C1[i].parts != P / N + (i < P % N)) return 0;
  return 1;
}
