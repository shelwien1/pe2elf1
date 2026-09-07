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

/*  Recovery: PCM back to residue digits.

    The synthesis chain is spectrum -> IMDCT -> window -> overlap-add.  With a
    Princen-Bradley window that chain is perfectly reconstructing, so the same
    window followed by the matched MDCT returns the spectrum exactly, and the
    MDCT is the same lifting DCT-IV read through the TDAC fold.  From there the
    steps invert one by one: divide out the floor, undo the coupling (a
    sign-conditional add, exactly invertible), and walk the cascade ladder from
    the coarsest pass down, rounding each digit and subtracting what it stands
    for.

    Two things are not invertible and need help: the s16 rounding of the PCM,
    which puts a noise floor under the division, and the block at each end of a
    link whose window runs off the PCM.  Both are covered by a keep-list.  */

constexpr i64 AW_FRAMES = 3 * (i64) MD_MAXBLK;   /*  the PCM working window  */

static i16 aw_buf[AW_FRAMES * VD_MAXCH];
static i16 aw_seg[2 * MD_MAXM * VD_MAXCH];
static i64 aw_z[2 * MD_MAXM], aw_v[MD_MAXM], aw_u[MD_MAXM];
static i64 aw_wcache[8][2 * MD_MAXM];     /*  one window per (lW, W, nW)  */
static u8  aw_wbuilt[8];
static i64 * aw_win;
static i64 aw_X[VD_MAXCH][MD_MAXM];       /*  recovered spectrum  */
static i64 aw_r[VD_MAXCH][MD_MAXM];       /*  ... and residue, after un-floor  */
static double aw_fl[VD_MAXCH][MD_MAXM];   /*  the floor curve  */
static u32 aw_sh;                         /*  the 2/M left in the spectrum  */

/*  A forward window over the WAV, in Vorbis channel order.  */
struct pcmwin {
  FILE * f;
  const char * path;
  u32 ch;
  i64 frames, lo, hi, data0;               /*  data0: byte offset of the samples  */
  u8 ord[VD_MAXCH];                        /*  WAV channel -> Vorbis channel  */

  static u32 rd32(const u8 * p) {
    return (u32) p[0] | ((u32) p[1] << 8) | ((u32) p[2] << 16) | ((u32) p[3] << 24);
  }
  static u32 rd16(const u8 * p) { return (u32) p[0] | ((u32) p[1] << 8); }

  /*  fseek's offset is a long: 64-bit on LP64 Linux, 32-bit on LLP64 Windows,
      where a WAV past 2 GiB -- which wav.inc's own WAV_MAXBYTES allows -- would
      seek to a wrapped, negative offset and read the wrong samples.  Offsets
      are computed as i64 and checked to survive the narrowing, so such a file
      is refused rather than silently misread.  */
  void seek_to(i64 off) {
    FATAL_UNLESS(off == (i64) (long) off,
                 "%s: offset %" PRId64 " needs 64-bit file offsets", path,
                 (i64) off);
    if (fseek(f, (long) off, SEEK_SET))
      FATAL_CODE(BLR_EXIT_IO, "seek failed on %s", path);
  }

  /*  Walk the RIFF chunks rather than assuming a bare 44-byte header.  Any
      writer may put LIST, fact or padding chunks before the samples, and
      guessing the offset would silently shift every block.  The fmt chunk
      is checked against the stream so a mismatched WAV fails here and not
      as a wrong digit ten thousand records later.  */
  void open(const char * pth, u32 nch, u32 rate) {
    u8 h[8], fmt[16];
    i64 at;
    f = fopen(pth, "rb");
    if (!f) FATAL_CODE(BLR_EXIT_IO, "cannot open %s", pth);
    path = pth;  ch = nch;  lo = hi = 0;  data0 = -1;  frames = 0;
    wav_order(nch, ord);
    FATAL_UNLESS(fread(h, 1, 4, f) == 4 && !memcmp(h, "RIFF", 4),
                 "%s: not a RIFF file", pth);
    FATAL_UNLESS(fseek(f, 8, SEEK_SET) == 0 && fread(h, 1, 4, f) == 4 &&
                 !memcmp(h, "WAVE", 4), "%s: not a WAVE file", pth);
    at = 12;
    for (;;) {
      u32 sz4;
      if (at != (i64) (long) at) break;          /*  past what fseek can reach  */
      if (fseek(f, (long) at, SEEK_SET) || fread(h, 1, 8, f) != 8) break;
      sz4 = rd32(h + 4);
      if (!memcmp(h, "fmt ", 4)) {
        FATAL_UNLESS(sz4 >= 16 && fread(fmt, 1, 16, f) == 16,
                     "%s: short fmt chunk", pth);
        FATAL_UNLESS(rd16(fmt) == 1, "%s: not uncompressed PCM", pth);
        FATAL_UNLESS(rd16(fmt + 2) == nch, "%s: %" PRIu64 " channels, the stream has %"
                     PRIu64, pth, (u64) rd16(fmt + 2),
                     (u64) nch);
        FATAL_UNLESS(rd32(fmt + 4) == rate, "%s: %" PRIu64 " Hz, the stream is %" PRIu64 " Hz",
                     pth, (u64) rd32(fmt + 4), (u64) rate);
        FATAL_UNLESS(rd16(fmt + 14) == 16, "%s: not 16-bit samples", pth);
      } else if (!memcmp(h, "data", 4)) {
        data0 = at + 8;  frames = (i64) sz4 / (nch * 2);
        break;
      }
      at += 8 + (i64) sz4 + (i64) (sz4 & 1);     /*  chunks pad to even  */
    }
    FATAL_UNLESS(data0 >= 0, "%s: no data chunk", pth);
  }
  void close() { if (f) fclose(f);  f = nullptr; }

  void fill(i64 first) {
    i64 want = AW_FRAMES;
    if (first < 0) first = 0;
    if (first + want > frames) want = frames - first;
    if (want < 0) want = 0;
    seek_to(data0 + first * (i64) ch * 2);
    if (want && fread(aw_buf, 2 * ch, (size_t) want, f) != (size_t) want)
      FATAL_CODE(BLR_EXIT_IO, "short read on %s", path);
    lo = first;  hi = first + want;
  }

  /*  2M frames starting at `first`, de-interleaved into Vorbis order and
      zero-filled outside the stream.  */
  const i16 * seg(i64 first, u32 n2) {
    i64 i;  u32 c;
    if (first < lo || first + (i64) n2 > hi) fill(first);
    memset(aw_seg, 0, (sz) n2 * ch * sizeof *aw_seg);
    for (i = 0; i < (i64) n2; i++) {
      i64 t = first + i;
      if (t < lo || t >= hi) continue;
      for (c = 0; c < ch; c++)
        aw_seg[i * ch + ord[c]] = aw_buf[(t - lo) * ch + c];
    }
    return aw_seg;
  }
  /*  Is the whole span inside the stream?  A block that is not cannot be
      analysed and its digits are carried verbatim.  */
  int covered(i64 first, u32 n2) const {
    return first >= 0 && first + (i64) n2 <= frames;
  }
};

static const i64 INV_S16 = fx(1.0 / 32767.0);

struct vd_ana {
  vd_setup * s;
  lifted_dct4 * lift;
  imdct * md;
  pcmwin * pw;

  /*  The window the synthesis effectively applied to this block.  There are
      only eight of them -- the three flags -- so they are built once each
      rather than per sample per block.  */
  void window(u32 M, int lW, int W, int nW) {
    u32 n = 2 * M, i;
    int slot = (lW << 2) | (W << 1) | nW;
    aw_win = aw_wcache[slot];
    if (aw_wbuilt[slot]) return;
    aw_wbuilt[slot] = 1;
    u32 ln = 2 * (u32) (lW ? s->bs1 / 2 : s->bs0 / 2);
    u32 rn = 2 * (u32) (nW ? s->bs1 / 2 : s->bs0 / 2);
    u32 lb, le, rb, re;
    if (!W) { ln = n;  rn = n; }
    lb = n / 4 - ln / 4;  le = lb + ln / 2;
    rb = n / 2 + n / 4 - rn / 4;  re = rb + rn / 2;
    for (i = 0; i < n; i++) aw_win[i] = 0;
    for (i = lb; i < le; i++) aw_win[i] = fx((double) md->win[ln == 2 * s->bs1 / 2 ? 1 : 0][i - lb]);
    for (i = le; i < rb; i++) aw_win[i] = (i64) 1 << F;
    for (i = rb; i < re; i++)
      aw_win[i] = fx((double) md->win[rn == 2 * s->bs1 / 2 ? 1 : 0][rn / 2 - 1 - (i - rb)]);
  }

  /*  Windowed MDCT of one channel: fold to M points, then the lifting DCT-IV,
      then the 2/M the perfect-reconstruction pair calls for.  */
  void spectrum(const i16 * seg, u32 c, u32 M, i64 * X) {
    u32 j, h = M / 2, sh = 0, t = M;
    while (t > 2) { t >>= 1;  sh++; }        /*  sh = log2(M) - 1  */
    /*  the synthesis wrote round(y * 32767), so that is the scale to undo  */
    for (j = 0; j < 2 * M; j++)
      aw_z[j] = pmul((i64) seg[j * pw->ch + c] * INV_S16, aw_win[j]);
    for (j = 0; j < h; j++) {
      aw_v[j] = -aw_z[3 * h - 1 - j] - aw_z[3 * h + j];
      aw_v[h + j] = aw_z[j] - aw_z[M - 1 - j];
    }
    lift->dct4(aw_v, aw_u, M);
    /*  The 2/M the transform pair calls for is a right shift by log2(M)-1,
        and applying it here throws away that many low bits of every
        coefficient -- ten of them at the long block size, enough to zero a
        coefficient sitting under a deep floor.  So the scale is carried
        alongside instead and folded into the floor divide, which already
        works in double.

        This buys nothing measurable, and the reason is worth keeping: the
        sample quantization puts 1.5e-5 of full scale under every coefficient,
        which is 436 times the lifting's own error, so the bits being
        discarded were already below the noise.  The change stays because it
        removes a precision cliff for free, not because it made anything
        smaller.  */
    aw_sh = sh;
    memcpy(X, aw_u, M * sizeof *X);
  }

  /*  The inverse of the decoder's coupling, which is the encoder's coupling.

      libvorbis builds a coupled pair in _vp_couple_quantize_normalize: the
      larger of the two channels becomes the magnitude, the angle is their
      signed difference, and a final step collapses the two tuples that would
      otherwise decode alike.  That function is an exact inverse of the
      decoder's, so recovering the residue is a matter of calling it rather
      than deriving a branch test by hand.

      The hand-derived version this replaces was right 99.59% of the time over
      the 14,641 tuples the encoder can emit, and wrong on exactly the
      collapsed ones -- where the angle sits at twice the magnitude.  Those had
      been written off as an inherent loss of the coupling; they are nothing of
      the sort, and calling the encoder settles them.

      It buys nothing here: the collapsed tuples do not occur anywhere in the
      test corpus, and the meta is byte-identical either way.  The change
      stays because it is provably right and the version it replaces was
      provably not.  */
  static void uncouple(i64 & Mg, i64 & An) {
    i64 A = Mg, B = An, iM, iA;
    i64 aA = A < 0 ? -A : A, aB = B < 0 ? -B : B;
    if (aA > aB) { iM = A;  iA = A > 0 ? A - B : B - A; }
    else         { iM = B;  iA = B > 0 ? A - B : B - A; }
    { i64 am = iM < 0 ? -iM : iM;
      if (iA >= am * 2) { iA = -iA;  iM = -iM; } }
    Mg = iM;  An = iA;
  }

  /*  Divide the floor out of every channel.  This is the one lossy step of
      the recovery besides the s16 rounding: where the floor is small the
      division amplifies the PCM noise.  */
  /*  A new setup invalidates the cache: the block sizes may differ.  */
  void reset_windows() { memset(aw_wbuilt, 0, sizeof aw_wbuilt); }

  void divide(u32 ch, u32 n) {
    u32 c, k;
    double sc = 1.0 / (double) ((i64) 1 << aw_sh);
    for (c = 0; c < ch; c++)
      for (k = 0; k < n; k++) {
        double fl = aw_fl[c][k] / sc, q;
        if (fl <= 0) { aw_r[c][k] = 0;  continue; }
        /*  The floor spans seven decades; at its low end it has only a few
            significant bits as fixed point, so the division is done in
            double and only the quotient is carried on as fixed point.  This
            step is lossy anyway and is not part of the reversible chain.  */
        q = (double) aw_X[c][k] / fl;
        aw_r[c][k] = (i64) (q >= 0 ? q + 0.5 : q - 0.5);
      }
  }

  /*  Undo the coupling steps in the order the decoder applied them (it walks
      them backwards, so the inverse walks forwards).  */
  void uncouple_all(vd_map * mp, u32 n) {
    u32 i, k;
    for (i = 0; i < mp->nstep; i++)
      for (k = 0; k < n; k++)
        uncouple(aw_r[mp->mag[i]][k], aw_r[mp->ang[i]][k]);
  }
};
