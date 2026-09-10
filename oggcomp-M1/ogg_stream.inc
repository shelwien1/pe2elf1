constexpr sz IN_LOW = (sz)20 << 20;
constexpr sz IN_WIN = (sz)32 << 20;
static u8 in_win[IN_WIN];
//  The input, as a window over the coroutine's input pin: what the walker
//  reads is in_win, refilled from the pin when it runs low.
struct source {
  coro3_pin *in;
  const char *path;
  sz at, have;
  sz origin;
  int eof;
  u32 crc;
  void open(const char *pth, coro3_pin *pin) {
    in = pin;
    path = pth;
    at = have = origin = 0;
    eof = 0;
    crc = 0;
    crcinit();
    want();
  }
  void close() { in = nullptr; }
  //  Up to n bytes from the pin into dst; zero once the file is done.
  sz fill(u8 *dst, sz n) {
    coro3_pin &p = *in;
    for(;;) {
      sz got = (sz)(p.end - p.ptr);
      if(got) {
        if(got > n)
          got = n;
        memcpy(dst, p.ptr, got);
        p.ptr += got;
        return got;
      }
      if(p.f_quit())
        return 0;
      p.yield_r();
    }
  }
  void want() {
    sz got;
    if(avail() >= IN_LOW || eof)
      return;
    if(at) {
      memmove(in_win, in_win + at, have - at);
      origin += at;
      have -= at;
      at = 0;
    }
    while(have < IN_WIN) {
      got = fill(in_win + have, IN_WIN - have);
      if(!got) {
        eof = 1;
        break;
      }
      crc = crcrun(crc, in_win + have, got);
      have += got;
    }
  }
  void skip(sz n) { at += n; }
  const u8 *cur() const { return in_win + at; }
  sz avail() const { return have - at; }
  sz offset() const { return origin + at; }
  //  The next page of the same logical stream after offset k in the window:
  //  pages of other streams -- a multiplexed file's -- are stepped over.
  //  Zero when there is none, and whatever is at k is not a page then, so a
  //  continuation through junk is refused rather than joined across it.
  sz next_of(u32 serial, sz &k, ogg_page &q) const {
    sz n;
    for(;;) {
      n = q.parse(in_win + k, have - k);
      if(!n || q.serial == serial)
        return n;
      k += n;
    }
  }
  sz join_len(sz got, const ogg_page &p, sz pl) const {
    int last_eos = (p.type & 4) != 0;
    sz total = pl, k = at + got, n;
    ogg_page q;
    for(;;) {
      VB_UNLESS(!last_eos, "the packet at %" PRIu64 " exceeds its stream", (u64)offset());
      n = next_of(p.serial, k, q);
      if(!n)
        VB_REFUSE("the packet at %" PRIu64 " has no continuation page", (u64)offset());
      VB_UNLESS(q.np > 0 && (q.type & 1), "the continuation of the packet at %" PRIu64 " is missing", (u64)offset());
      total += q.plen[0];
      VB_UNLESS(total - pl <= MAXPAY * (sz)OGG_MAXSEG, "the packet at %" PRIu64 " continues too far", (u64)offset());
      if(!(q.np == 1 && q.tail))
        return total;
      k += n;
      last_eos = (q.type & 4) != 0;
    }
  }
  void join_pkt(sz got, const ogg_page &p, const u8 *head, sz pl, sz total, u8 *dst) const {
    sz done = pl, k = at + got, n, take;
    ogg_page q;
    memcpy(dst, head, pl);
    while(done < total) {
      n = next_of(p.serial, k, q);
      if(!n)
        VB_REFUSE("the packet at %" PRIu64 " has no continuation page", (u64)offset());
      take = q.plen[0];
      if(take > total - done)
        take = total - done;
      memcpy(dst + done, in_win + k + OGG_HDRMIN + q.nseg, take);
      done += take;
      k += n;
    }
  }
};
constexpr sz JB_MAX = MAXPAY * (OGG_MAXSEG + 1);
static u8 jb[JB_MAX];

//  A group of pages: what the v3 stream calls a link.  It starts at a page
//  and runs to the next beginning-of-stream page that arrives after the
//  group has seen a page that was not one -- so the streams of a multiplexed
//  file, whose beginning-of-stream pages all come first, share a group, and
//  a chained file's streams get one each -- or to the end of the file.  The
//  end-of-stream bit decides nothing; a stream that never sets it just ends.
//
//  One stream in the group is coded as Vorbis: the first whose page parses.
//  Its serial is remembered so that a page of another stream between two of
//  its pages is coded as bytes without disturbing the continuation state,
//  and so that a page of its own that had to be coded as bytes can end it.
//  Nothing here is coded; both sides keep it from the page headers.
struct og_group {
  int have_vs;
  u32 vserial;
  int w;
  int cont;
  sz spill;
  int bos_phase;
  int lastraw, lasteos;
  void start() {
    have_vs = 0;
    vserial = 0;
    w = 0;
    cont = 0;
    spill = 0;
    bos_phase = 1;
    lastraw = 0;
    lasteos = 0;
  }
  void after(const ogg_page &p, int raw) {
    lastraw = raw;
    lasteos = (p.type & 4) != 0;
    bos_phase = bos_phase && (p.type & 2);
  }
};

//  Where in the window the next page starts: the first "OggS" at which a
//  page parses.  Everything before it is a run.  Short of the end of the
//  file the last PAGE_MAX bytes are not searched, so that a page which
//  starts inside the window also ends inside it; the caller codes the run
//  up to there, refills, and looks again.
static sz page_start(const u8 *b, sz n, int eof) {
  sz i = 0, lim = eof ? n : (n > PAGE_MAX ? n - PAGE_MAX : 0);
  ogg_page q;
  while(i < lim) {
    const u8 *m = (const u8 *)memchr(b + i, 'O', lim - i);
    if(!m)
      return lim;
    i = (sz)(m - b);
    if(i + 4 <= n && !memcmp(b + i, "OggS", 4) && q.parse(b + i, n - i))
      return i;
    i++;
  }
  return lim;
}

static u8 pgbuf[PAGE_MAX];
static u8 body[MAXPAY];
//  Where the decoder's bytes go -- the coroutine's output pin -- and the
//  CRC of everything that went.
struct out_sink {
  coro3_pin *out;
  u32 crc;
  void put(const u8 *b, sz n) {
    coro3_pin &p = *out;
    crc = crcrun(crc, b, n);
    while(n) {
      sz room = (sz)(p.end - p.ptr);
      if(!room) {
        p.yield_r();
        continue;
      }
      if(room > n)
        room = n;
      memcpy(p.ptr, b, room);
      p.ptr += room;
      b += room;
      n -= room;
    }
  }
};

//  The encoder's walk: the source in front, the group being coded, the
//  Vorbis context the parser fills, and the model everything is coded
//  into.  One page at a time, each first through the parser against a
//  sink that codes nothing, to learn whether it parses at all.
template <class M> struct og_packer {
  M &t;
  source &src;
  vb_ctx v;
  og_group g;
  og_packer(M &tt, source &ss) : t(tt), src(ss) {}
  //  One page's packets through the parser, against whichever sink.  The
  //  continuation state and the header count are the group's, but come in by
  //  pointer so that a dry run can hand copies and keep the group as it was.
  template <class S> void walk_page(S &sink, const ogg_page &p, sz got, int *cont, sz *spill, int *w) {
    const u8 *pbody = src.cur() + OGG_HDRMIN + p.nseg;
    u64 where = (u64)src.offset();
    sz at = 0;
    int j;
    for(j = 0; j < p.np; j++) {
      const u8 *pk = pbody + at;
      sz pl = p.plen[j], off = at;
      at += pl;
      if(j == 0 && (p.type & 1) && *cont) {
        VB_UNLESS(pl <= *spill, "the page at %" PRIu64 " takes %" PRIu64 " of %" PRIu64 " remaining bytes", where, (u64)pl, (u64)*spill);
        *spill -= pl;
        if(!(p.np == 1 && p.tail)) {
          VB_UNLESS(!*spill, "the page at %" PRIu64 " closes a packet %" PRIu64 " bytes early", where, (u64)*spill);
          *cont = 0;
        }
        continue;
      }
      VB_UNLESS(pl > 0, "the page at %" PRIu64 " carries an empty packet", where);
      if(j == p.np - 1 && p.tail) {
        sz tot = src.join_len(got, p, pl);
        src.join_pkt(got, p, pbody + off, pl, tot, jb);
        sink.spill((u32)(tot - pl));
        *spill = tot - pl;
        *cont = 1;
        pk = jb;
        pl = tot;
      }
      io_t<S> z(v, sink, 1, (u8 *)pk, pl);
      if(pk[0] & 1) {
        VB_UNLESS(*w < 3, "a stream has over three header packets");
        z.hdr(*w);
        (*w)++;
      } else
        z.audio();
    }
  }

  //  Does this page parse as Vorbis?  The parser answers by refusing or not,
  //  so run it with nothing listening and catch the refusal.  What the run
  //  touched is put back: the group's copies were copies, and the setup a
  //  header may have parsed into vb_su is forgotten by pointing v.cur away
  //  from it and returning the arena, so the real parse does it again.
  int page_fits(const ogg_page &p, sz got) {
    null_sink ns;
    vb_ctx vsave = v;
    sz usave = vb_ar.used;
    int cont = g.cont, w = g.w;
    sz spill = g.spill;
    int ok = 0;
    vb_dry = 1;
    try {
      walk_page(ns, p, got, &cont, &spill, &w);
      ok = 1;
    } catch(vb_dry_refusal &) {
      ok = 0;
    }
    vb_dry = 0;
    v = vsave;
    vb_ar.used = usave;
    return ok;
  }

  //  Code whatever is at the cursor that is not a page, as a run, and stop
  //  at a page or at the end: 1 with a page at the cursor, 0 at the end.
  int consume_junk() {
    ogg_page q;
    for(;;) {
      sz n, off, k, i;
      src.want();
      n = src.avail();
      if(!n) {
        t.rawlen(0);
        return 0;
      }
      if(q.parse(src.cur(), n)) {
        t.rawlen(0);
        return 1;
      }
      off = page_start(src.cur(), n, src.eof);
      if(!off)
        off = 1;
      k = off < MAXPAY ? off : MAXPAY;
      t.rawlen((u32)k);
      for(i = 0; i < k; i++)
        t.raw(src.cur()[i]);
      src.skip(k);
    }
  }

  //  Does the group go on past the page at the cursor's back?  Yes if the
  //  next page is near enough to be sure of and is not the start of a new
  //  group.  A run longer than the window ends the group: whatever follows
  //  it starts one, and if that is a page of the same stream, its packets
  //  are what they are and code as they can.
  int peek_more() const {
    ogg_page q;
    sz n, off;
    src.want();
    n = src.avail();
    if(!n)
      return 0;
    off = page_start(src.cur(), n, src.eof);
    if(off >= n)
      return 0;
    if(!q.parse(src.cur() + off, n - off))
      return 0;
    return !((q.type & 2) && !g.bos_phase);
  }

  //  One page.  The header is modelled whatever the payload is.  The payload
  //  goes through the parser if the page is the Vorbis stream's, or could
  //  be -- no stream has parsed yet -- and parses; otherwise it is bytes.
  void code_page(ogg_page &p, sz got) {
    const u8 *cur = src.cur();
    int crcok = ogg_page::crc_ok(cur, got);
    int cand = !g.have_vs || g.vserial == p.serial;
    int raw;
    sz i;
    t.page(p, g.cont, g.bos_phase);
    if(t.crcbad(!crcok)) {
      u32 c = rd32(cur + 22);
      for(i = 0; i < 4; i++)
        t.raw((c >> (8 * i)) & 255);
    }
    raw = cand ? !page_fits(p, got) : 1;
    t.pgraw((u32)raw, g.lastraw);
    if(!raw) {
      if(!g.have_vs) {
        g.have_vs = 1;
        g.vserial = p.serial;
      }
      walk_page(t, p, got, &g.cont, &g.spill, &g.w);
    } else {
      const u8 *pb = cur + OGG_HDRMIN + p.nseg;
      for(i = 0; i < p.blen; i++)
        t.raw(pb[i]);
      if(g.have_vs && g.vserial == p.serial) {
        g.cont = 0;
        g.spill = 0;
      }
    }
    g.after(p, raw);
  }

  void pack() {
    ogg_page p;
    sz got;
    v.init();
    for(;;) {
      if(!consume_junk()) {
        t.link(0);
        t.check(src.crc);
        return;
      }
      t.link(1);
      v.link();
      g.start();
      t.link_begin();
      for(;;) {
        t.more(1, g.lasteos);
        consume_junk();
        got = p.parse(src.cur(), src.avail());
        code_page(p, got);
        src.skip(got);
        if(!peek_more())
          break;
      }
      t.more(0, g.lasteos);
    }
  }
};

//  The decoder's walk: the mirror image, following the flags the encoder
//  wrote and putting the pages back together for the output pin.
template <class M> struct og_unpacker {
  M &t;
  out_sink o;
  vb_ctx v;
  og_group g;
  og_unpacker(M &tt, coro3_pin *pin) : t(tt) {
    o.out = pin;
    o.crc = 0;
  }
  void emit_junk() {
    u32 n, i;
    while((n = t.rawlen(0)) != 0) {
      for(i = 0; i < n; i++)
        body[i] = (u8)t.raw(0);
      o.put(body, n);
    }
  }
  void unpack(const char *in) {
    crcinit();
    v.init();
    for(;;) {
      ogg_page q;
      sz jn = 0;
      emit_junk();
      if(!t.link(0))
        break;
      v.link();
      g.start();
      t.link_begin();
      while(t.more(0, g.lasteos)) {
        sz at = 0, pgl, i;
        int j, crcbad, raw;
        u32 crc = 0;
        emit_junk();
        t.page(q, g.cont, g.bos_phase);
        FATAL_UNLESS(q.blen <= MAXPAY, "%s: a page claims %" PRIu64 " payload bytes", in, (u64)q.blen);
        crcbad = (int)t.crcbad(0);
        if(crcbad)
          for(i = 0; i < 4; i++)
            crc |= t.raw(0) << (8 * i);
        raw = (int)t.pgraw(0, g.lastraw);
        if(raw) {
          for(i = 0; i < q.blen; i++)
            body[i] = (u8)t.raw(0);
          if(g.have_vs && g.vserial == q.serial) {
            g.cont = 0;
            g.spill = 0;
          }
        } else {
          if(!g.have_vs) {
            g.have_vs = 1;
            g.vserial = q.serial;
          }
          for(j = 0; j < q.np; j++) {
            sz pl = q.plen[j];
            if(j == 0 && (q.type & 1) && g.cont) {
              FATAL_UNLESS(pl <= g.spill, "%s: page takes %" PRIu64 " of %" PRIu64 " remaining bytes", in, (u64)pl, (u64)g.spill);
              if(pl)
                memcpy(body + at, jb + jn - g.spill, pl);
              g.spill -= pl;
              if(!(q.np == 1 && q.tail)) {
                FATAL_UNLESS(!g.spill, "%s: a page closes a packet %" PRIu64 " bytes early", in, (u64)g.spill);
                g.cont = 0;
              }
              at += pl;
              continue;
            }
            FATAL_UNLESS(pl > 0, "%s: a page claims an empty packet", in);
            if(j == q.np - 1 && q.tail) {
              sz ex = t.spill(0);
              sz tot = pl + ex;
              io z(v, t, 0, jb, tot);
              jn = tot;
              if(g.w < 3) {
                z.hdr(g.w);
                g.w++;
              } else
                z.audio();
              memcpy(body + at, jb, pl);
              g.spill = ex;
              g.cont = 1;
              at += pl;
              continue;
            }
            {
              io z(v, t, 0, body + at, pl);
              if(g.w < 3) {
                z.hdr(g.w);
                g.w++;
              } else
                z.audio();
              at += pl;
            }
          }
        }
        pgl = q.emit(pgbuf, body);
        if(crcbad)
          wr32(pgbuf + 22, crc);
        o.put(pgbuf, pgl);
        g.after(q, raw);
      }
    }
    FATAL_UNLESS(t.check(0) == o.crc, "%s: the restored file does not check -- the stream is damaged", in);
  }
};
