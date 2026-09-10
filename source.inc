
/*  Both directions walk forward only, so the input is read through a window
    rather than held whole.  A continued packet reaches at most
    MAXPAY * OGG_MAXSEG payload bytes, and every page it passes through
    carries at least OGG_MAXSEG of them in at most OGG_HDRMIN + 1 + OGG_MAXSEG
    bytes, so no page's join can reach further than IN_LOW from its own start.
    The window is larger so that refilling it moves the tail rarely.  */
constexpr sz IN_LOW = (sz) 20 << 20;
constexpr sz IN_WIN = (sz) 32 << 20;

/*  Where the bytes come from when not a file: a pull, handing back what it
    put at dst, up to n bytes, and 0 once there is no more.  oggcomp's
    coroutine feeds the window from its input pin this way.  */
typedef sz (*source_pull)(void * ctx, u8 * dst, sz n);

/*  A forward-only window over the input.  */

static u8 in_win[IN_WIN];     /*  the input window  */

struct source {
  FILE * f;                   /*  one or the other  */
  source_pull pull;
  void * pctx;
  const char * path;          /*  the input's name, for messages  */
  sz at, have;                /*  cursor and valid bytes in the window  */
  sz origin;                  /*  file offset of the window's first byte  */
  int eof;

  void open(const char * pth) {
    f = fopen(pth, "rb");
    if (!f) FATAL_CODE(BLR_EXIT_IO, "cannot open %s", pth);
    pull = nullptr;  pctx = nullptr;
    start(pth);
  }

  void open(const char * pth, source_pull p, void * ctx) {
    f = nullptr;  pull = p;  pctx = ctx;
    start(pth);
  }

  void start(const char * pth) {
    path = pth;  at = have = origin = 0;  eof = 0;
    want();
  }

  /*  A pull's errors are its caller's to see; a file's are seen here.  */
  void close() {
    if (f) {
      int bad = ferror(f);
      if (fclose(f)) bad = 1;
      if (bad) FATAL_CODE(BLR_EXIT_IO, "read error on %s", path);
      f = nullptr;
    }
    pull = nullptr;
  }

  /*  Make a page and everything its joins can reach available at the cursor.  */

  void want() {
    sz got;
    if (avail() >= IN_LOW || eof) return;
    if (at) {
      memmove(in_win, in_win + at, have - at);
      origin += at;  have -= at;  at = 0;
    }
    while (have < IN_WIN) {
      got = f ? fread(in_win + have, 1, IN_WIN - have, f)
              : pull(pctx, in_win + have, IN_WIN - have);
      if (!got) { eof = 1;  break; }
      have += got;
    }
    if (f && ferror(f)) FATAL_CODE(BLR_EXIT_IO, "read error on %s", path);
  }

  void skip(sz n) { at += n; }

  const u8 * cur() const { return in_win + at; }

  sz avail() const { return have - at; }
  sz offset() const { return origin + at; }

  /*  Total length of the packet whose first `pl` bytes end the page at the
      cursor, which occupies `got` bytes.  */

  sz join_len(sz got, const ogg_page & p, sz pl) const {
    const ogg_page * prev = &p;
    sz total = pl, k = at + got, n;
    ogg_page q;
    for (;;) {
      FATAL_UNLESS(!(prev->type & 4), "the packet at %" PRIu64 " exceeds its link",
                   (u64) offset());
      n = q.parse(in_win + k, have - k);
      /*  FATAL rather than FATAL_UNLESS: the latter returns when its
          condition holds, so it cannot be NORETURN, and gcc therefore cannot
          see that q is assigned on every path reaching the memcpy below.  It
          says so as a -Wmaybe-uninitialized on q.nseg at -O1 and -O3 -- but
          not at -O2, the one level the Makefile ships, which is how it went
          unnoticed.  */
      if (!n) FATAL("the packet at %" PRIu64 " has no continuation page",
                    (u64) offset());
      FATAL_UNLESS(q.np > 0 && (q.type & 1),
                   "the continuation of the packet at %" PRIu64 " is missing",
                   (u64) offset());
      total += q.plen[0];
      FATAL_UNLESS(total - pl <= MAXPAY * (sz) OGG_MAXSEG,
                   "the packet at %" PRIu64 " continues too far",
                   (u64) offset());
      if (!(q.np == 1 && q.tail)) return total;
      k += n;  prev = &q;
    }
  }

  /*  Gather that packet into one buffer.  */

  void join_pkt(sz got, const u8 * head, sz pl, sz total, u8 * dst) const {
    sz done = pl, k = at + got, n, take;
    ogg_page q;
    memcpy(dst, head, pl);
    while (done < total) {
      n = q.parse(in_win + k, have - k);
      /*  FATAL rather than FATAL_UNLESS: the latter returns when its
          condition holds, so it cannot be NORETURN, and gcc therefore cannot
          see that q is assigned on every path reaching the memcpy below.  It
          says so as a -Wmaybe-uninitialized on q.nseg at -O1 and -O3 -- but
          not at -O2, the one level the Makefile ships, which is how it went
          unnoticed.  */
      if (!n) FATAL("the packet at %" PRIu64 " has no continuation page",
                    (u64) offset());
      take = q.plen[0];
      if (take > total - done) take = total - done;
      memcpy(dst + done, in_win + k + OGG_HDRMIN + q.nseg, take);
      done += take;  k += n;
    }
  }
};
