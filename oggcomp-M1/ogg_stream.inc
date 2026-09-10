constexpr sz IN_LOW = (sz)20 << 20;
constexpr sz IN_WIN = (sz)32 << 20;
typedef sz (*source_pull)(void *ctx, u8 *dst, sz n);
static u8 in_win[IN_WIN];
struct source {
  source_pull pull;
  void *pctx;
  const char *path;
  sz at, have;
  sz origin;
  int eof;
  void open(const char *pth, source_pull p, void *ctx) {
    pull = p;
    pctx = ctx;
    path = pth;
    at = have = origin = 0;
    eof = 0;
    want();
  }
  void close() { pull = nullptr; }
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
      got = pull(pctx, in_win + have, IN_WIN - have);
      if(!got) {
        eof = 1;
        break;
      }
      have += got;
    }
  }
  void skip(sz n) { at += n; }
  const u8 *cur() const { return in_win + at; }
  sz avail() const { return have - at; }
  sz offset() const { return origin + at; }
  sz join_len(sz got, const ogg_page &p, sz pl) const {
    const ogg_page *prev = &p;
    sz total = pl, k = at + got, n;
    ogg_page q;
    for(;;) {
      FATAL_UNLESS(!(prev->type & 4), "the packet at %" PRIu64 " exceeds its link", (u64)offset());
      n = q.parse(in_win + k, have - k);
      if(!n)
        FATAL("the packet at %" PRIu64 " has no continuation page", (u64)offset());
      FATAL_UNLESS(q.np > 0 && (q.type & 1), "the continuation of the packet at %" PRIu64 " is missing", (u64)offset());
      total += q.plen[0];
      FATAL_UNLESS(total - pl <= MAXPAY * (sz)OGG_MAXSEG, "the packet at %" PRIu64 " continues too far", (u64)offset());
      if(!(q.np == 1 && q.tail))
        return total;
      k += n;
      prev = &q;
    }
  }
  void join_pkt(sz got, const u8 *head, sz pl, sz total, u8 *dst) const {
    sz done = pl, k = at + got, n, take;
    ogg_page q;
    memcpy(dst, head, pl);
    while(done < total) {
      n = q.parse(in_win + k, have - k);
      if(!n)
        FATAL("the packet at %" PRIu64 " has no continuation page", (u64)offset());
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
struct link_walk {
  int w;
  int cont;
  sz spill;
  void start() {
    w = 0;
    cont = 0;
    spill = 0;
  }
  void page(vb_ctx &v, sink_t &t, source &s, ogg_page &p, sz got) {
    const u8 *pbody = s.cur() + OGG_HDRMIN + p.nseg;
    u64 where = (u64)s.offset();
    sz at = 0;
    int j;
    t.page(p, cont);
    Fj(
        p.np, const u8 *pk = pbody + at; sz pl = p.plen[j]; sz off = at; at += pl; if(j == 0 && (p.type & 1) && cont) {
          FATAL_UNLESS(pl <= spill,
                       "the page at %" PRIu64 " takes %" PRIu64 " of %" PRIu64 " remaining "
                       "bytes",
                       where, (u64)pl, (u64)spill);
          spill -= pl;
          if(!(p.np == 1 && p.tail)) {
            FATAL_UNLESS(!spill, "the page at %" PRIu64 " closes a packet %" PRIu64 " bytes early", where, (u64)spill);
            cont = 0;
          }
          continue;
        } FATAL_UNLESS(pl > 0, "the page at %" PRIu64 " carries an empty packet", where);
        if(j == p.np - 1 && p.tail) {
          sz tot = s.join_len(got, p, pl);
          s.join_pkt(got, pbody + off, pl, tot, jb);
          t.spill((u32)(tot - pl));
          spill = tot - pl;
          cont = 1;
          pk = jb;
          pl = tot;
        } io z(v, t, 1, (u8 *)pk, pl);
        if(pk[0] & 1) {
          FATAL_UNLESS(w < 3, "a link has over three header packets");
          z.hdr(w);
          w++;
        } else z.audio());
  }
};
static u8 pgbuf[PAGE_MAX];
static u8 body[MAXPAY];
typedef void (*page_writer)(void *ctx, const u8 *page, sz len);
void vb_pack(sink_t &t, source &src) {
  const char *in = src.path;
  vb_ctx v;
  link_walk lw;
  ogg_page p;
  sz got, npage = 0;
  int eos = 1;
  v.init();
  lw.start();
  for(;;) {
    src.want();
    if(!src.avail())
      break;
    got = p.parse(src.cur(), src.avail());
    FATAL_UNLESS(got != 0, "%s: no Ogg page at %" PRIu64, in, (u64)src.offset());
    FATAL_UNLESS(ogg_page::crc_ok(src.cur(), got), "%s: the page at %" PRIu64 " has a bad CRC", in, (u64)src.offset());
    if((p.type & 2) && eos) {
      t.link(1);
      v.link();
      lw.start();
      t.link_begin();
      eos = 0;
    }
    FATAL_UNLESS(!eos, "%s: page %" PRIu64 " %s", in, (u64)npage, npage ? "follows end of stream" : "precedes any bitstream");
    lw.page(v, t, src, p, got);
    if(p.type & 4) {
      FATAL_UNLESS(!lw.cont, "%s: a continued packet is %" PRIu64 " bytes short", in, (u64)lw.spill);
      eos = 1;
    }
    src.skip(got);
    npage++;
  }
  FATAL_UNLESS(npage > 0, "%s: not an Ogg bitstream", in);
  FATAL_UNLESS(eos, "%s: final bitstream has no end-of-stream page", in);
  t.link(0);
}
void vb_unpack(sink_t &t, const char *in, page_writer emit, void *ectx) {
  vb_ctx v;
  v.init();
  while(t.link(0)) {
    ogg_page q;
    sz jn = 0, spill = 0;
    int w = 0, done = 0, cont = 0;
    v.link();
    t.link_begin();
    while(!done) {
      sz at = 0, pgl;
      int j;
      t.page(q, cont);
      FATAL_UNLESS(q.blen <= MAXPAY, "%s: a page claims %" PRIu64 " payload bytes", in, (u64)q.blen);
      Fj(
          q.np, sz pl = q.plen[j]; if(j == 0 && (q.type & 1) && cont) {
            FATAL_UNLESS(pl <= spill, "%s: page takes %" PRIu64 " of %" PRIu64 " remaining bytes", in, (u64)pl, (u64)spill);
            if(pl)
              memcpy(body + at, jb + jn - spill, pl);
            spill -= pl;
            if(!(q.np == 1 && q.tail)) {
              FATAL_UNLESS(!spill, "%s: a page closes a packet %" PRIu64 " bytes early", in, (u64)spill);
              cont = 0;
            }
            at += pl;
            continue;
          } FATAL_UNLESS(pl > 0, "%s: a page claims an empty packet", in);
          if(j == q.np - 1 && q.tail) {
            sz ex = t.spill(0);
            sz tot = pl + ex;
            io z(v, t, 0, jb, tot);
            jn = tot;
            if(w < 3) {
              z.hdr(w);
              w++;
            } else
              z.audio();
            memcpy(body + at, jb, pl);
            spill = ex;
            cont = 1;
            at += pl;
            continue;
          } io z(v, t, 0, body + at, pl);
          if(w < 3) {
            z.hdr(w);
            w++;
          } else z.audio();
          at += pl);
      pgl = q.emit(pgbuf, body);
      emit(ectx, pgbuf, pgl);
      if(q.type & 4)
        done = 1;
    }
    FATAL_UNLESS(!cont, "%s: a continued packet is %" PRIu64 " bytes short", in, (u64)spill);
  }
}
