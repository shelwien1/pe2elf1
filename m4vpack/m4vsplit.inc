// m4vsplit.inc -- mt.inc's M4vSplit: c -t's splitter, the stream in, its parts out.

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
    cx.init();
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
    cx.quit();
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
    if (sc_is_vol(chunk_code)) cx.parse_header_chunk(p, n);
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
    if (type && !vop_is_coded(cx, buf + i, len - i)) type = 0;
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
      if (!grow(&buf, &room, len + l + GB_PAD, IO_BUF)) no_memory();
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
