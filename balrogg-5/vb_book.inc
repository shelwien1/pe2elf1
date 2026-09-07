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

constexpr u32 VB_MAXBOOK = 256;   /*  the codebook count is 8 bits + 1  */

/*  Codebook bodies -- lengths, codewords, decode tree, multiplicands and the
    inverse multiplicand table -- vary in size with the setup, so they come
    from a bump arena that is reclaimed whole when the link ends.  */
constexpr sz VB_ARENA = (sz) 96 << 20;

/*  vb_alloc hands out u32* and i32* into this, and rounds every request up to
    8, so the base is the only thing that has to be aligned.  Without alignas
    the array's alignment is 1 by the standard -- gcc happens to place it well,
    which is what kept -Wcast-align=strict's two warnings here theoretical.  */
alignas(8) static u8 vb_arena[VB_ARENA];
static sz vb_used;

template <class T> static T * vb_alloc(sz n) {
  sz want = (n * sizeof(T) + 7) & ~(sz) 7;
  u8 * p = vb_arena + vb_used;
  FATAL_UNLESS(want <= VB_ARENA - vb_used,
               "vorbis: the setup needs more than the %" PRIu64 " MiB codebook arena",
               (u64) (VB_ARENA >> 20));
  vb_used += want;
  return (T *) p;
}

struct vb_book {
  u32 dim, ent;
  u8 look;                    /*  lookup type: 0 none, 1 the VQ grid  */
  u8 * len;                   /*  codeword length per entry, 0 = unused  */
  u32 * code;                 /*  canonical codeword, most significant first  */
  i32 * nd;                   /*  decode tree: two children per node, a
                                  negative child is -(entry + 1)  */
  u32 nv;                     /*  multiplicands, i.e. digits per dimension  */
  u32 * mult;                 /*  the multiplicand VALUES  */
  u32 base, off;              /*  max multiplicand + 1, and half of it  */
  /*  Multiplicand value to mult[] index, `base` entries wide.  */
  u32 * inv;

  /*  Vorbis `_make_words` codeword assignment, including sparse length lists.  */
  void words() {
    u32 mk[33], i, j, e, l;
    memset(mk, 0, sizeof mk);
    Fi(ent,
      l = len[i];
      if (!l) continue;
      e = mk[l];
      FATAL_UNLESS(l >= 32 || !(e >> l), "vorbis: over-populated codebook");
      code[i] = e;
      for (j = l; j > 0; j--) {
        if (mk[j] & 1) { mk[j] = j == 1 ? mk[1] + 1 : mk[j - 1] << 1;  break; }
        mk[j]++;
      }
      for (j = l + 1; j < 33; j++) {
        if ((mk[j] >> 1) != e) break;
        e = mk[j];  mk[j] = mk[j - 1] << 1;
      });
  }

  /*  ... and the tree that reads them back.  A child of 0 is absent, a
      negative child is the leaf -(entry + 1); a tree over `u` used entries
      has at most `u` interior nodes.  */
  void tree() {
    u32 i, k, n = 1, u = 0, idx, cap;
    i32 t;
    Fi(ent, if (len[i]) u++);
    cap = u + 2;
    nd = vb_alloc<i32>(2 * cap);
    memset(nd, 0, 2 * cap * sizeof *nd);
    Fi(ent,
      if (!len[i]) continue;
      idx = 0;
      for (k = len[i]; k > 1; k--) {
        u32 c = 2 * idx + ((code[i] >> (k - 1)) & 1);
        t = nd[c];
        if (!t) { FATAL_UNLESS(n < cap, "vorbis: codebook tree overflows");
                  t = (i32) n++;  nd[c] = t; }
        FATAL_UNLESS(t > 0, "vorbis: codebook is not prefix free");
        idx = (u32) t;
      }
      idx = 2 * idx + (code[i] & 1);
      FATAL_UNLESS(!nd[idx], "vorbis: codebook is not prefix free");
      nd[idx] = -(i32) (i + 1));
  }

  /*  A lookup-1 entry is a base-`nv` list of multiplicand indices. Code centered
      values and use `inv` to recover each index before recomposition.  */
  void grid() {
    u32 i, mx = 0, np;
    Fi(nv, if (mult[i] > mx) mx = mult[i]);
    base = mx + 1;  off = base >> 1;
    inv = vb_alloc<u32>(base);
    Fi(base, inv[i] = (u32) -1);
    /*  Require unique multiplicands and enough digit tuples for every entry.  */
    Fi(nv,
      FATAL_UNLESS(inv[mult[i]] == (u32) -1,
                   "vorbis: duplicate codebook multiplicand %" PRIu64,
                   (u64) mult[i]);
      inv[mult[i]] = i);
    /*  Count tuples, guarding the nv == 1 case.  */
    np = 1;
    if (nv > 1) Fi(dim, np *= nv);
    Fi(ent, if (len[i] && i >= np)
                 FATAL("vorbis: codebook entry %" PRIu64 " exceeds lookup grid %" PRIu64,
                       (u64) i,
                       (u64) np));
  }
};
