/*  Link and page walk.  A file is a sequence of links, a link a sequence of
    pages, and a page a sequence of packets; a packet that runs off the end of
    its page is gathered whole before it is taken apart.  Neither direction
    ever looks back, so both stream and neither holds the file.  */

static u8 pgbuf[PAGE_MAX];    /*  one rebuilt page, on the way out  */
static u8 body[MAXPAY];       /*  its payload, while the packets fill it  */

void vb_pack(const char * in, const char * out) {
  source src;
  vb_ctx v;
  tsv t;
  link_walk lw;
  ogg_page p;
  sz got, npage = 0;
  int eos = 1;

  src.open(in);
  t.create(out);
  blr_output(out);
  v.init();
  lw.start();

  for (;;) {
    src.want();
    if (!src.avail()) break;
    got = p.parse(src.cur(), src.avail());
    FATAL_UNLESS(got != 0, "%s: no Ogg page at %" PRIu64, in,
                 (u64) src.offset());
    /*  CRCs must be valid because decoding recomputes them.  */
    FATAL_UNLESS(ogg_page::crc_ok(src.cur(), got), "%s: the page at %" PRIu64 " has a bad CRC",
                 in, (u64) src.offset());
    /*  BOS starts a link only after EOS.  */
    if ((p.type & 2) && eos) {
      t.put("link.more", 1);  v.link();  lw.start();  eos = 0;
    }
    FATAL_UNLESS(!eos, "%s: page %" PRIu64 " %s", in, (u64) npage,
                 npage ? "follows end of stream" : "precedes any bitstream");
    lw.page(v, t, src, p, got);
    if (p.type & 4) {
      FATAL_UNLESS(!lw.cont, "%s: a continued packet is %" PRIu64 " bytes short", in,
                   (u64) lw.spill);
      eos = 1;
    }
    src.skip(got);  npage++;
  }
  FATAL_UNLESS(npage > 0, "%s: not an Ogg bitstream", in);
  FATAL_UNLESS(eos, "%s: final bitstream has no end-of-stream page", in);
  t.put("link.more", 0);

  t.close();  src.close();
  blr_output_kept();
}

void vb_unpack(const char * in, const char * out) {
  vb_ctx v;
  tsv t;
  FILE * o;

  t.open(in);
  v.init();
  o = fopen(out, "wb");
  if (!o) FATAL_CODE(BLR_EXIT_IO, "cannot create %s", out);
  blr_output(out);

  while (t.get_u("link.more", 2)) {
    ogg_page q;
    sz jn = 0, spill = 0;
    int w = 0, done = 0, cont = 0;

    v.link();
    while (!done) {
      sz at = 0, pgl;
      int j;
      q.get(t);
      FATAL_UNLESS(q.blen <= MAXPAY, "%s: a page claims %" PRIu64 " payload bytes", in,
                   (u64) q.blen);
      Fj(q.np,
        sz pl = q.plen[j];
        /*  Copy a fragment from the packet decoded on an earlier page.  */
        if (j == 0 && (q.type & 1) && cont) {
          FATAL_UNLESS(pl <= spill, "%s: page takes %" PRIu64 " of %" PRIu64 " remaining bytes",
                       in, (u64) pl, (u64) spill);
          if (pl) memcpy(body + at, jb + jn - spill, pl);
          spill -= pl;
          if (!(q.np == 1 && q.tail)) {
            FATAL_UNLESS(!spill, "%s: a page closes a packet %" PRIu64 " bytes early",
                         in, (u64) spill);
            cont = 0;
          }
          at += pl;  continue;
        }
        FATAL_UNLESS(pl > 0, "%s: a page claims an empty packet", in);
        if (j == q.np - 1 && q.tail) {
          sz ex = (sz) t.get_u("page.spill",
                               (u64) MAXPAY * OGG_MAXSEG + 1);
          sz tot = pl + ex;
          io z(v, t, 0, jb, tot);
          jn = tot;
          if (w < 3) { z.hdr(w);  w++; } else z.audio();
          memcpy(body + at, jb, pl);
          spill = ex;  cont = 1;  at += pl;  continue;
        }
        io z(v, t, 0, body + at, pl);
        if (w < 3) { z.hdr(w);  w++; } else z.audio();
        at += pl);
      pgl = q.emit(pgbuf, body);
      if (fwrite(pgbuf, 1, pgl, o) != pgl)
        FATAL_CODE(BLR_EXIT_IO, "write error on %s", out);
      if (q.type & 4) done = 1;
    }
    FATAL_UNLESS(!cont, "%s: a continued packet is %" PRIu64 " bytes short", in,
                 (u64) spill);
  }

  t.close();
  if (fclose(o)) FATAL_CODE(BLR_EXIT_IO, "write error on %s", out);
  blr_output_kept();
}
