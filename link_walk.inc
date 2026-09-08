
/*  A packet gathered across pages: its first fragment fills a page at most,
    and the rest is capped at MAXPAY * OGG_MAXSEG.  */
constexpr sz JB_MAX = MAXPAY * (OGG_MAXSEG + 1);

static u8 jb[JB_MAX];         /*  a straddling packet, gathered whole  */

struct link_walk {
  int w;                      /*  header packets seen  */
  int cont;                   /*  a packet is still being handed on  */
  sz spill;                   /*  bytes of it still owed  */

  void start() { w = 0;  cont = 0;  spill = 0; }

  void page(vb_ctx & v, tsv & t, source & s, const ogg_page & p,
                       sz got) {
    const u8 * pbody = s.cur() + OGG_HDRMIN + p.nseg;
    u64 where = (u64) s.offset();
    sz at = 0;
    int j;
    p.put(t);
    Fj(p.np,
      const u8 * pk = pbody + at;
      sz pl = p.plen[j];
      sz off = at;
      at += pl;
      /*  Skip fragments already coded where their packet started. `cont`
          also covers a zero-length closing fragment.  */
      if (j == 0 && (p.type & 1) && cont) {
        FATAL_UNLESS(pl <= spill, "the page at %" PRIu64 " takes %" PRIu64 " of %" PRIu64 " remaining "
                     "bytes", where, (u64) pl, (u64) spill);
        spill -= pl;
        if (!(p.np == 1 && p.tail)) {
          FATAL_UNLESS(!spill, "the page at %" PRIu64 " closes a packet %" PRIu64 " bytes early",
                       where, (u64) spill);
          cont = 0;
        }
        continue;
      }
      FATAL_UNLESS(pl > 0, "the page at %" PRIu64 " carries an empty packet", where);
      if (j == p.np - 1 && p.tail) {
        sz tot = s.join_len(got, p, pl);
        s.join_pkt(got, pbody + off, pl, tot, jb);
        t.put("page.spill", (i64) (tot - pl));
        spill = tot - pl;  cont = 1;  pk = jb;  pl = tot;
      }
      io z(v, t, 1, (u8 *) pk, pl);
      if (pk[0] & 1) {
        FATAL_UNLESS(w < 3, "a link has over three header packets");
        z.hdr(w);  w++;
      } else z.audio());
  }
};
