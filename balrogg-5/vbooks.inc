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
    chosen by quality and sample rate simply points at them.  A stream it
    produced therefore carries codebooks that are exactly one of those sets, so
    the whole of the setup's largest section can be replaced by the quality
    that selects it.

    Across the test corpus 21 of 26 files reproduce their entire codebook set
    this way, and codebooks are 18.6% of the meta.  The rest -- constant
    bitrate, managed bitrate, or another release -- carry theirs as before.

    Nothing about the encoding has to be carried alongside: whether the length
    list went out ordered, sparse or plain is decided by vorbis_staticbook_pack
    from the lengths themselves, and that decision is reproduced here.  */

struct vbooks {
  vorbis_info vi;
  int ready;

  vbooks() : ready(0) {}
  void close() { if (ready) vorbis_info_clear(&vi);  ready = 0; }

  int open(u32 ch, u32 rate, int q10) {
    close();
    vorbis_info_init(&vi);
    if (vorbis_encode_init_vbr(&vi, (long) ch, (long) rate, q10 * 0.1f)) {
      vorbis_info_clear(&vi);  return 0;
    }
    ready = 1;  return 1;
  }

  int count() const {
    return ready ? ((codec_setup_info *) vi.codec_setup)->books : 0;
  }
  static_codebook * book(int i) const {
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
