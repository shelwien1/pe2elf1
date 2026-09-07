/*  Copyright (C) 2026 Kamila Szewczyk

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.  */

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
    unsigned long where = (unsigned long) s.offset();
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
        FATAL_UNLESS(pl <= spill, "the page at %lu takes %lu of %lu remaining "
                     "bytes", where, (unsigned long) pl, (unsigned long) spill);
        spill -= pl;
        if (!(p.np == 1 && p.tail)) {
          FATAL_UNLESS(!spill, "the page at %lu closes a packet %lu bytes early",
                       where, (unsigned long) spill);
          cont = 0;
        }
        continue;
      }
      FATAL_UNLESS(pl > 0, "the page at %lu carries an empty packet", where);
      if (j == p.np - 1 && p.tail) {
        sz tot = s.join_len(got, p, pl);
        s.join_pkt(got, pbody + off, pl, tot, jb);
        t.put("page.spill", (long long) (tot - pl));
        spill = tot - pl;  cont = 1;  pk = jb;  pl = tot;
      }
      io z(v, t, 1, (u8 *) pk, pl);
      if (pk[0] & 1) {
        FATAL_UNLESS(w < 3, "a link has over three header packets");
        z.hdr(w);  w++;
      } else z.audio());
  }
};
