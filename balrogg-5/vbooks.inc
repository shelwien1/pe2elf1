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

/*  Codebooks from an index instead of from the stream.

    libvorbis does not build codebooks when it encodes.  lib/books holds
    several hundred static tables, generated offline once, and a setup template
    chosen by the channel count, the sample rate and the encoder setting simply
    points at them.  A stream it produced therefore carries codebooks that are
    exactly one of those sets, so the whole of the setup's largest section can
    be replaced by the number of the set.

    Which sets exist is a property of the release that wrote the stream, and
    they moved: the tables were revised for 1.1 and again for 1.3, aoTuV
    replaced the templates that select them, and the managed-bitrate path uses
    a different residue family from the quality path in every release.  A file
    in the wild could have come from any of them, and the one library compiled
    in here is only ever one.  So the sets of every released libvorbis and
    every aoTuV beta are carried as data in vbooks_gen.inc, generated once by
    psyfit/bookdump.sh; between them they are what an Ogg Vorbis file is
    likely to hold.

    ffmpeg's own encoder -- the one behind `-c:a vorbis`, not `-c:a libvorbis`
    -- is in there too, as a single row: it chooses from nothing, and writes
    the same 29 books into every file it makes.

    Nothing about the encoding has to be carried alongside: whether the length
    list went out ordered, sparse or plain is decided by vorbis_staticbook_pack
    from the lengths themselves, and that decision is reproduced here.  */

/*  A codebook set, as it appears in the generated table.  The two pointer
    fields stand in for what static_codebook wants, and open_gen() copies the
    row into one of those.  */
struct vg_book {
  long dim, entries;
  const char * lengthlist;
  int maptype;
  long q_min, q_delta;
  int q_quant, q_sequencep;
  const long * quantlist;
};
/*  One set, over the sample rates it is chosen for.  Both bounds are the
    exact rates libvorbis switches at, and `first` indexes vg_idx.  */
struct vg_set { int ch, rate_lo, rate_hi, n, first; };

#include "vbooks_gen.inc"

/*  Two libraries answer here.  Library 0 is the libvorbis compiled in, asked
    the way it asks itself, so a stream it wrote is named by the quality alone
    and keeps the number it always had.  Library 1 is the generated table: the
    same sets collected from every released libvorbis, which no code of ours
    could otherwise reach, and a set there is named by its row.

    One integer in the meta covers both: the quality for library 0, and
    VB_LIB1 + the row for library 1.  */
constexpr int VB_LIB1 = 100;

/*  The widest set any release produces is well under this; open_gen() checks
    rather than trusting it.  */
#define VB_MAXSET 128

struct vbooks {
  vorbis_info vi;
  int ready;                              /*  1 library 0 open, 2 library 1  */
  const vg_set * gset;
  static_codebook gbk[VB_MAXSET];         /*  materialised from the table  */

  vbooks() : ready(0), gset(nullptr) {}
  void close() { if (ready == 1) vorbis_info_clear(&vi);  ready = 0;  gset = nullptr; }

  static int nsets() { return (int) (sizeof vg_sets / sizeof *vg_sets); }

  /*  Rows that could describe a stream of this shape, in table order; -1 ends
      the walk.  Rows are sorted by (ch, rate_lo), so this could stop early,
      but the table is small enough that a scan is not worth the subtlety.  */
  static int gen_next(u32 ch, u32 rate, int prev) {
    int i;
    for (i = prev + 1; i < nsets(); i++) {
      const vg_set * g = vg_sets + i;
      if ((u32) g->ch == ch && rate >= (u32) g->rate_lo && rate <= (u32) g->rate_hi)
        return i;
    }
    return -1;
  }
  static int gen_count(int i) { return vg_sets[i].n; }
  /*  Book k of row i, without materialising the row -- the width and depth of
      a book are enough to reject nearly every row at no cost.  */
  static const vg_book * gen_book(int i, int k) {
    return vg_books + vg_idx[vg_sets[i].first + k];
  }

  int open_gen(u32 ch, u32 rate, int row) {
    if (row < 0 || row >= nsets()) return 0;
    { const vg_set * g = vg_sets + row;
      int k;
      /*  a meta naming a row is trusted no further than this  */
      if ((u32) g->ch != ch || rate < (u32) g->rate_lo || rate > (u32) g->rate_hi)
        return 0;
      if (g->n > VB_MAXSET) return 0;
      for (k = 0; k < g->n; k++) {
        const vg_book * b = vg_books + vg_idx[g->first + k];
        static_codebook & d = gbk[k];
        memset(&d, 0, sizeof d);
        d.dim = b->dim;  d.entries = b->entries;
        d.lengthlist = (char *) b->lengthlist;
        d.maptype = b->maptype;  d.q_min = b->q_min;  d.q_delta = b->q_delta;
        d.q_quant = b->q_quant;  d.q_sequencep = b->q_sequencep;
        d.quantlist = (long *) b->quantlist;
      }
      gset = g;  ready = 2;  return 1; }
  }

  int open(u32 ch, u32 rate, int q10) {
    close();
    if (q10 >= VB_LIB1) return open_gen(ch, rate, q10 - VB_LIB1);
    vorbis_info_init(&vi);
    if (vorbis_encode_init_vbr(&vi, (long) ch, (long) rate, q10 * 0.1f)) {
      vorbis_info_clear(&vi);  return 0;
    }
    ready = 1;  return 1;
  }

  int count() const {
    if (ready == 2) return gset->n;
    return ready ? ((codec_setup_info *) vi.codec_setup)->books : 0;
  }
  static_codebook * book(int i) const {
    if (ready == 2) return (static_codebook *) (gbk + i);
    return ((codec_setup_info *) vi.codec_setup)->book_param[i];
  }

  /*  Does this setting's book i match what the stream described?  */
  int same(int i, const vd_book & s, const u8 * len, const u32 * mult) const {
    static_codebook * b = book(i);
    long j;
    if ((u32) b->dim != s.dim || (u32) b->entries != s.ent) return 0;
    if ((b->maptype != 0) != (s.lookup != 0)) return 0;
    for (j = 0; j < b->entries; j++)
      if ((u8) b->lengthlist[j] != len[j]) return 0;
    if (b->maptype) {
      long n = _book_maptype1_quantvals(b);
      if (b->q_sequencep != s.seq) return 0;
      for (j = 0; j < n; j++)
        if ((u32) b->quantlist[j] != mult[j]) return 0;
    }
    return 1;
  }

  /*  Populate a decoder book from the table, so the restore side does not
      have to parse records it is about to write.  */
  void fill(int i, vd_book & s) const {
    static_codebook * b = book(i);
    s.dim = (u32) b->dim;  s.ent = (u32) b->entries;
    s.lookup = b->maptype;  s.nv = 0;  s.off = 0;  s.seq = 0;
    s.minval = s.delta = 0;  s.min_fx = s.delta_fx = 0;
    if (!b->maptype) return;
    s.minval = vd_f32((u32) ((b->q_min >> 31) & 1), (u32) ((b->q_min >> 21) & 0x3ff),
                      (u32) (b->q_min & 0x1fffff));
    s.delta  = vd_f32((u32) ((b->q_delta >> 31) & 1), (u32) ((b->q_delta >> 21) & 0x3ff),
                      (u32) (b->q_delta & 0x1fffff));
    s.seq = b->q_sequencep;
    s.nv = vd_lk1(s.ent, s.dim);
    { long j, n = _book_maptype1_quantvals(b), mx = 0;
      for (j = 0; j < n; j++) if (b->quantlist[j] > mx) mx = b->quantlist[j];
      s.off = (u32) ((mx + 1) >> 1); }
    s.min_fx = fx((double) s.minval);  s.delta_fx = fx((double) s.delta);
  }

  /*  Write book i in balrogg's record form.  `grouped` writes a sparse
      book's use flags and lengths as two runs, which is how the meta stores
      them; the restored stream wants them interleaved, as the bitstream had
      them.  */
  void emit(int i, tsv & dst, int grouped) const {
    static_codebook * b = book(i);
    long j, k, ent = b->entries;
    int ordered = 0, sparse = 0;
    dst.put("cb.dim", b->dim);
    dst.put("cb.entries", ent);
    /*  the same test vorbis_staticbook_pack makes  */
    for (j = 1; j < ent; j++)
      if (b->lengthlist[j - 1] == 0 || b->lengthlist[j] < b->lengthlist[j - 1]) break;
    if (j == ent) ordered = 1;
    dst.put("cb.ordered", ordered);
    if (ordered) {
      long count = 0;
      dst.put("cb.ordlen", b->lengthlist[0] - 1);
      for (j = 1; j < ent; j++) {
        int now = b->lengthlist[j], last = b->lengthlist[j - 1];
        if (now > last)
          for (k = last; k < now; k++) { dst.put("cb.ordrun", j - count);  count = j; }
      }
      dst.put("cb.ordrun", j - count);
    } else {
      for (j = 0; j < ent; j++) if (!b->lengthlist[j]) break;
      sparse = (j != ent);
      dst.put("cb.sparse", sparse);
      if (!sparse)
        for (j = 0; j < ent; j++) dst.put("cb.len", b->lengthlist[j] - 1);
      else if (grouped) {
        for (j = 0; j < ent; j++) dst.put("cb.used", b->lengthlist[j] ? 1 : 0);
        for (j = 0; j < ent; j++)
          if (b->lengthlist[j]) dst.put("cb.len", b->lengthlist[j] - 1);
      } else
        for (j = 0; j < ent; j++) {
          dst.put("cb.used", b->lengthlist[j] ? 1 : 0);
          if (b->lengthlist[j]) dst.put("cb.len", b->lengthlist[j] - 1);
        }
    }
    dst.put("cb.lookup", b->maptype);
    if (b->maptype) {
      long n = _book_maptype1_quantvals(b);
      dst.put("cb.min.sign", (b->q_min >> 31) & 1);
      dst.put("cb.min.exp",  (b->q_min >> 21) & 0x3ff);
      dst.put("cb.min.mant",  b->q_min        & 0x1fffff);
      dst.put("cb.dlt.sign", (b->q_delta >> 31) & 1);
      dst.put("cb.dlt.exp",  (b->q_delta >> 21) & 0x3ff);
      dst.put("cb.dlt.mant",  b->q_delta        & 0x1fffff);
      dst.put("cb.valbits", b->q_quant - 1);
      dst.put("cb.sequence", b->q_sequencep);
      for (j = 0; j < n; j++) dst.put("cb.mult", b->quantlist[j]);
    }
  }
};
