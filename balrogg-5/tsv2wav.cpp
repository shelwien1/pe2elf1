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

/*  tsv2wav -- PCM synthesis from a balrogg record stream, and back.

      tsv2wav c input.tsv output.wav output.meta
      tsv2wav d output.wav restored.tsv output.meta

    Mode c decodes the record stream to 16-bit PCM, matching libvorbis to
    within one LSB, and writes a meta stream holding everything the PCM cannot
    give back.  Mode d rebuilds the exact record stream from the two.

    The bulk of a stream is res.digit, 92% of its records; those come back out
    of the PCM by running the synthesis backwards -- windowed MDCT, divide by
    the floor, undo the coupling, then walk the cascade ladder subtracting each
    digit's worth as it is read.  res.class is predicted from the recovered
    values, since the class is the choice of ladder.  aud.wprev is the previous
    block's flag.  What is left -- page framing, headers, the floor posts, and
    the corrections where the above got it wrong -- is the meta, which is
    itself a TSV.

    The floor has to stay: the recovery divides by it before it can read any
    digit.  */

#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #define DEV_NULL "nul"
#else
  #define DEV_NULL "/dev/null"
#endif

#ifdef BLR_VORBIS               /*  see vorbislib/patch.py  */
#include "psy.c"
#include "floor1.c"
#include "floor0.c"
#include "mdct.c"
#include "smallft.c"
#include "lookup.c"
#include "lpc.c"
#include "lsp.c"
#include "sharedbook.c"
#include "codebook.c"
#include "registry.c"
#include "info.c"
#include "block.c"
#include "envelope.c"
#include "bitrate.c"
#include "analysis.c"
#include "synthesis.c"
#include "window.c"
#include "res0.c"
#include "mapping0.c"
#include "vorbisenc.c"
#include "bitwise.c"
#include "framing.c"




/*  libvorbis's table and ours are identical entry for entry  */
#define FLOOR_DB FLOOR1_fromdB_LOOKUP
#endif

#ifndef BLR_VORBIS
#define VF_QREC(d)   ((void) 0)
#define VF_QGET(st_)  ((void) 0)
#endif

/*  A run of L zeros goes out as -L, which costs three bytes; a single zero
    written plainly costs two.  So runs start at length two.  Getting this
    wrong taxes every isolated zero a byte, and it taxes a predictor that
    scatters its zeros more than one that clusters them -- which is enough to
    decide the wrong predictor.  */
#define VF_RUNMIN 2

#include "common.inc"      /*  types, diagnostics, loop macros  */
#include "tsv.inc"         /*  the record stream  */


#include "lift.inc"        /*  fixed point, lifting rotations, DCT-IV  */
#ifndef BLR_VORBIS
#include "floor_db.inc"    /*  floor1_inverse_dB_table  */
#endif
#include "imdct.inc"       /*  the window and the inverse transform  */

#include "vd_setup.inc"    /*  codebooks, floors, residues, mappings, modes  */
#include "wav.inc"         /*  the PCM sink  */
#include "vd_dec.inc"      /*  one packet, and the lap between packets  */
#include "vd_ana.inc"      /*  PCM back to the spectrum  */
#include "vd_rec.inc"      /*  the residue walk, run backwards  */

/*  Signed values go out zigzagged -- 0, -1, 1, -2 becomes 0, 1, 2, 3 -- so a
    minus sign never costs a byte of its own and a run marker is free to use
    the sign instead.  */
static long long vf_zig(i32 d) { return d < 0 ? -2LL * d - 1 : 2LL * d; }
static i32 vf_unzig(long long z) { return (z & 1) ? (i32) (-(z + 1) / 2) : (i32) (z / 2); }
#ifdef BLR_VORBIS
#include "vfloor.inc"      /*  the floor, refitted by libvorbis  */
#include "vbooks.inc"      /*  codebooks, reproduced from an index  */
#endif

constexpr int OGG_MAXSEG = 255;
constexpr u32 LK_MAX = 4096;              /*  links in one file  */

/*  Just enough of a page to find the packet boundaries.

    Three of the header fields are not information.  The type flags follow from
    position -- BOS on a link's first page, EOS on its last, continued when the
    page before ended mid-packet.  The sequence number counts from zero within
    a link.  The serial is one number per link, not per page.  The meta carries
    the serial once and derives the other two, and mode c checks the derivation
    rather than trusting it.  */
struct pg {
  u32 type, glo, ghi, serial, seq, np;
  u32 plen[OGG_MAXSEG];
  int tail;

  /*  Read from the record stream, as balrogg wrote it.  */
  void get(tsv & t) {
    int i;
    type = t.get_u("page.type", 0x100);
    glo = t.get_u("page.granlo", 0x100000000ULL);
    ghi = t.get_u("page.granhi", 0x100000000ULL);
    serial = t.get_u("page.serial", 0x100000000ULL);
    seq = t.get_u("page.seq", 0x100000000ULL);
    np = t.get_u("page.npkt", OGG_MAXSEG + 1);
    Fi((int) np, plen[i] = t.get_u("page.plen",
                                   (unsigned long long) OGG_MAXSEG * OGG_MAXSEG + 1));
    tail = 0;
    if (np && !(plen[np - 1] % OGG_MAXSEG)) tail = (int) t.get_u("page.tail", 2);
  }

  /*  Read from the meta, which omits what the walk can work out.  */
  void get_meta(tsv & t, u32 ser, u32 sq, int bos, int cont) {
    int i;
    type = (u32) ((bos ? 2 : 0) | (cont ? 1 : 0));
    glo = t.get_u("page.granlo", 0x100000000ULL);
    ghi = t.get_u("page.granhi", 0x100000000ULL);
    serial = ser;  seq = sq;
    np = t.get_u("page.npkt", OGG_MAXSEG + 1);
    Fi((int) np, plen[i] = t.get_u("page.plen",
                                   (unsigned long long) OGG_MAXSEG * OGG_MAXSEG + 1));
    tail = 0;
    if (np && !(plen[np - 1] % OGG_MAXSEG)) tail = (int) t.get_u("page.tail", 2);
    if (!strcmp(t.peek(), "page.eos")) { t.get("page.eos");  type |= 4; }
  }

  /*  Write in balrogg's order and form.  */
  void put(tsv & t) const {
    int i;
    t.put("page.type", type);
    t.put("page.granlo", glo);   t.put("page.granhi", ghi);
    t.put("page.serial", serial);  t.put("page.seq", seq);
    t.put("page.npkt", np);
    Fi((int) np, t.put("page.plen", plen[i]));
    if (np && !(plen[np - 1] % OGG_MAXSEG)) t.put("page.tail", tail);
  }

  /*  Write to the meta, omitting type, serial and seq.  */
  void put_meta(tsv & t) const {
    int i;
    t.put("page.granlo", glo);   t.put("page.granhi", ghi);
    t.put("page.npkt", np);
    Fi((int) np, t.put("page.plen", plen[i]));
    if (np && !(plen[np - 1] % OGG_MAXSEG)) t.put("page.tail", tail);
    if (type & 4) t.put("page.eos", 1);
  }
};

static vd_dec dec;
static wav sink;
static vd_ana ana;
static vd_rec rec;
static pcmwin apw;
static i64 lk_frames[LK_MAX];             /*  output frames per link  */
static u32 lk_n;

/*  Analysis state across a link.  */
static i64 an_base, an_T, tot_before;
static int an_covered;
static long meta_bytes;
static u32 dec_ch, dec_rate;              /*  from the identification header  */
/*  Where to split between two output integers was a knob once: nearest, then
    +-0.25 and +-0.125 tilting the split.  Every tilt moves a few samples one
    step further from what libvorbis would have produced and none of them paid,
    so nearest is what the code does and the table it swept is gone.  */
static u32 an_Mprev;
static int an_first, an_prevW;

/*  Recover the spectrum for the block that is about to be walked, and turn it
    into the residue the decoder started from.  */
/*  Advance to this block and report where it sits.  Split out of recover()
    because the floor prediction needs the same span, and runs before the
    floor records have been read.  */
static i64 an_span;
static int block_span(u32 n) {
  if (an_first) { an_T = 0;  an_first = 0; } else an_T += (i64) (an_Mprev + n) / 2;
  an_Mprev = n;
  an_span = an_base + an_T - (i64) n;
  return !(an_span < an_base ||
           an_span + 2 * (i64) n > an_base + lk_frames[lk_n - 1]);
}

static int recover(u32 n, int W, int wp, int wn, u32 ch, vd_map * mp) {
  u32 c, k;
  i64 first = an_span;
  if (!an_covered) {
    for (c = 0; c < ch; c++) memset(aw_r[c], 0, n * sizeof *aw_r[c]);
    return 0;
  }
  ana.window(n, wp, W, wn);
  {
    const i16 * seg = apw.seg(first, 2 * n);
    for (c = 0; c < ch; c++) ana.spectrum(seg, c, n, aw_X[c]);
  }
  for (c = 0; c < ch; c++)
    for (k = 0; k < n; k++) aw_fl[c][k] = (double) vd_flc[c][k];
  ana.divide(ch, n);
  ana.uncouple_all(mp, n);

  return 1;
}

/*  Correction blocks.

    Three things keep these small.  The indices and the values go out as two
    runs rather than interleaved pairs, so the TSV row grouping pays for each
    tag once per packet instead of once per correction.  The indices rise
    within a packet, so only the first is absolute.  And a packet with nothing
    to correct writes nothing at all -- the reader peeks for the count tag
    instead of reading a zero.

    A block the PCM does not span carries every value as a raw run with no
    indices; both roles know which form to use because both compute the same
    `covered`.  */
/*  What each form costs, counted in values rather than guessed at.

    The raw form carries every digit of the block, but its zeros go out as runs
    -- exactly what the floor differences already do -- so a block whose
    corrections cluster spends one value on each stretch that needed none.  The
    sparse form spends a gap-coded index on every correction and then the
    correction itself, so it never costs less than 2n.  The old rule, `2n >
    total`, compared the sparse cost against a raw form that had no runs at all
    and so almost always lost; with runs, raw wins whenever the digits needing
    no correction fall into fewer than n stretches, which on coupled stereo is
    most of the time.  */
/*  What a value costs on a row: its digits, and the separator before it.  The
    forms below are close enough in count that the digits decide between them
    -- a gap and a run length and a digit are all one value and rarely the
    same width -- so they are compared in bytes rather than in values.  */
static u32 dcost(long long v) {
  u32 d = 1;
  unsigned long long a = v < 0 ? 0ULL - (unsigned long long) v : (unsigned long long) v;
  if (v < 0) d++;
  while (a >= 10) { a /= 10;  d++; }
  return d + 1;
}

/*  The raw form: every digit of the packet, zeros run-coded.  */
static u32 keep_run_cost(const i32 * v, u32 total) {
  u32 i, c = 0;
  for (i = 0; i < total; ) {
    if (v[i]) { c += dcost(vf_zig(v[i]));  i++;  continue; }
    { u32 e = i;
      while (e < total && !v[e]) e++;
      if (e - i < VF_RUNMIN) { c += dcost(0);  i++; }
      else { c += dcost(-(long long) (e - i));  i = e; } }
  }
  return c;
}

/*  The sparse form: the gap to each correction, then the corrections.  */
static u32 keep_gap_cost(const u32 * ix, u32 n, const i32 * all) {
  u32 i, c = 0;
  long long prev = -1;
  for (i = 0; i < n; ) {
    long long g = (long long) ix[i] - prev - 1;
    if (g) { c += dcost(g);  prev = ix[i];  i++;  continue; }
    { u32 e = i;  long long p2 = prev;
      while (e < n && (long long) ix[e] - p2 - 1 == 0) { p2 = ix[e];  e++; }
      if (e - i < VF_RUNMIN) { c += dcost(0);  prev = ix[i];  i++; }
      else { c += dcost(-(long long) (e - i));  prev = p2;  i = e; } }
  }
  for (i = 0; i < n; i++) c += dcost(vf_zig(all[ix[i]]));
  return c;
}

/*  The marked form: the whole packet as one run, a digit the walk got right
    contributing a zero and a digit it got wrong its own value.  It carries
    the same corrections as the sparse form and none of the indices -- what
    separates two corrections is a run marker rather than a gap, and two
    corrections side by side need nothing between them at all, which is where
    it wins: corrections cluster.  Values go out as zig(v) + 1 so that they
    are never zero and never collide with the runs.  */
static u32 keep_mark_cost(const u32 * ix, u32 n, const i32 * all, u32 total) {
  u32 i = 0, k = 0, c = 0;
  while (i < total) {
    if (k < n && ix[k] == i) { c += dcost(vf_zig(all[i]) + 1);  k++;  i++;  continue; }
    { u32 e = i, k2 = k;
      while (e < total && !(k2 < n && ix[k2] == e)) e++;
      if (e - i < VF_RUNMIN) { c += dcost(0);  i++; }
      else { c += dcost(-(long long) (e - i));  i = e; } }
  }
  return c;
}

/*  How the count goes out, and with it which form follows:

        0            no corrections
        n > 0        sparse: n corrections, gaps then values
        -(2t)        raw: the packet's t digits, zeros run-coded
        -(2t + 1)    marked: the same t positions, corrections in place

    The two block forms both need the digit count, and the count is a number
    that is going out anyway, so the form rides in its low bit rather than in
    a record of its own.  Doubling it costs a digit only when it crosses a
    power of ten; a record of its own would cost two bytes every time, which
    measured over four files is 1.7 points of the 7.5 this form is worth.

    Each form is charged for its own count, since the three do not write the
    same number: a block form's is the packet's whole digit count where the
    sparse form's is only how many were wrong, which is often two digits
    fewer.  And the sparse form is charged KEEP_TAGC for the row of indices
    the other two do not write at all -- "kd.i", a tab and a newline.  Rows
    merge across packets when the tags run on, so that row sometimes costs
    less; charging it in full measured best.  */
#define KEEP_TAGC 6               /*  the tag, its tab and its newline  */
static long long keep_count(u32 n, u32 total, const u32 * ix, const i32 * all) {
  u32 sparse, raw, mark;
  if (!n) return 0;
  sparse = keep_gap_cost(ix, n, all) + KEEP_TAGC + dcost((long long) n);
  raw = keep_run_cost(all, total) + dcost(-2 * (long long) total);
  mark = keep_mark_cost(ix, n, all, total) + dcost(-(2 * (long long) total + 1));
  if (sparse <= raw && sparse <= mark) return (long long) n;
  return raw <= mark ? -2 * (long long) total : -(2 * (long long) total + 1);
}

static void write_keeps(tsv & dst, const char * ti, const char * tv,
                        long long c, u32 n, const u32 * ix, const i32 * vl,
                        const i32 * all, u32 total) {
  u32 i;
  if (!c) return;
  if (c < 0 && !((-c) & 1)) {             /*  raw: every digit of the packet  */
    /*  Zigzagged, so a value is never negative and the run marker keeps the
        sign to itself, the same arrangement the floor differences use.  */
    for (i = 0; i < total; ) {
      long long z = vf_zig(all[i]);
      if (z) { dst.put(tv, z);  i++;  continue; }
      { u32 e = i;
        while (e < total && !all[e]) e++;
        if (e - i < VF_RUNMIN) { dst.put(tv, 0);  i++; }
        else { dst.put(tv, -(long long) (e - i));  i = e; } }
    }
    return;
  }
  if (c < 0) {                            /*  marked: corrections in place  */
    u32 k = 0;
    for (i = 0; i < total; ) {
      if (k < n && ix[k] == i) { dst.put(tv, vf_zig(vl[k]) + 1);  k++;  i++;  continue; }
      { u32 e = i, k2 = k;
        while (e < total && !(k2 < n && ix[k2] == e)) e++;
        if (e - i < VF_RUNMIN) { dst.put(tv, 0);  i++; }
        else { dst.put(tv, -(long long) (e - i));  i = e; } }
    }
    return;
  }
  /*  Indices rise, so what goes out is the gap since the last one, less the
      one that is always there.  Corrections often sit next to each other --
      the median gap is one -- so a stretch of touching corrections becomes a
      stretch of zero gaps, and the same run marker that serves the floor
      posts serves here.  */
  { long long prev = -1;                  /*  so a first index of zero gaps by zero  */
    for (i = 0; i < n; ) {
      long long g = (long long) ix[i] - prev - 1;
      if (g) { dst.put(ti, g);  prev = ix[i];  i++;  continue; }
      { u32 e = i;  long long p2 = prev;
        while (e < n && (long long) ix[e] - p2 - 1 == 0) { p2 = ix[e];  e++; }
        if (e - i < VF_RUNMIN) { dst.put(ti, 0);  prev = ix[i];  i++; }
        else { dst.put(ti, -(long long) (e - i));  prev = p2;  i = e; } }
    } }
  for (i = 0; i < n; i++) dst.put(tv, vf_zig(vl[i]));
}

static void read_keeps(tsv & src, const char * ti, const char * tv,
                       long long c, u32 & n, u32 * ix, i32 * vl, int & raw) {
  u32 i;
  n = 0;  raw = 0;
  if (!c) return;
  if (c < 0 && !((-c) & 1)) {
    raw = 1;  n = (u32) ((-c) >> 1);
    FATAL_UNLESS(n <= KEEP_MAX, "%s: correction block is too large", tv);
    for (i = 0; i < n; ) {
      long long z = src.get(tv);
      if (z > 0) { vl[i++] = vf_unzig(z);  continue; }
      if (!z)    { vl[i++] = 0;  continue; }
      FATAL_UNLESS((u32) -z <= n - i, "%s: correction run overruns the block", tv);
      { long long r = -z;  while (r--) vl[i++] = 0; }
    }
    return;
  }
  if (c < 0) {
    /*  The marked form carries the same list the sparse form does, so it is
        unpacked into that and the walk downstream never learns the
        difference.  */
    u32 total = (u32) ((-c) >> 1), at = 0;
    FATAL_UNLESS(total <= KEEP_MAX, "%s: correction block is too large", tv);
    while (at < total) {
      long long z = src.get(tv);
      if (z > 0) {
        FATAL_UNLESS(n < KEEP_MAX, "%s: correction block is too large", tv);
        ix[n] = at;  vl[n] = vf_unzig(z - 1);  n++;  at++;  continue;
      }
      if (!z) { at++;  continue; }
      FATAL_UNLESS((u32) -z <= total - at, "%s: correction run overruns the block", tv);
      at += (u32) -z;
    }
    return;
  }
  {
    n = (u32) c;
    FATAL_UNLESS(n <= KEEP_MAX, "%s: correction block is too large", ti);
    long long prev = -1;
    for (i = 0; i < n; ) {
      long long g = src.get(ti);
      if (g > 0) { prev += g + 1;  ix[i++] = (u32) prev;  continue; }
      if (!g)    { prev += 1;      ix[i++] = (u32) prev;  continue; }
      FATAL_UNLESS((u32) -g <= n - i, "%s: correction run overruns the block", ti);
      { long long r = -g;
        while (r--) { prev += 1;  ix[i++] = (u32) prev; } }
    }
  }
  FATAL_UNLESS(n <= KEEP_MAX, "%s: correction block is too large", tv);
  for (i = 0; i < n; i++) vl[i] = vf_unzig(src.get(tv));
}

#ifdef BLR_VORBIS
/*  Buffers for the codebook comparison: the stream's section, and one
    generated from libvorbis at a candidate quality.  */
constexpr sz CB_CAP = 1u << 23;
static char cb_a[CB_CAP], cb_b[CB_CAP];
static vbooks vb_gen;

static int cb_q = -1;                     /*  the setting that reproduces them  */

/*  Emit every codebook of a setting into a buffer, in balrogg's own form.  */
static sz cb_render(vbooks & g, char * buf) {
  tsv d;  int i;
  d.create_mem(buf, CB_CAP);
  for (i = 0; i < g.count(); i++) g.emit(i, d, 1);
  d.flush_row();
  return d.memlen;
}

static vfloor vf;
#define VF_QREC(d)   do { (d).put("vf.q", vf_on ? vf_q : -1); \
                          if (vf_on) { (d).put("vf.q", vf_pm); \
                                       (d).put("vf.q", vf_g);  vf.reset_amp(); } \
                        } while (0)
#define VF_QGET(st_)  do { long long q2_ = (st_).get("vf.q"); \
                          vf_on = q2_ >= 0 && vf.open(dec.s.ch, dec.s.rate, (int) q2_); \
                          if (vf_on) { vf_pm = (int) (st_).get("vf.q"); \
                                       vf_g = (int) (st_).get("vf.q"); \
                                       vf.gain = vf_g * 0.1f; \
                                       vf.reset_amp(); } } while (0)
static int vf_on;                         /*  a usable encoder was found  */
static i32 vf_pred[VD_MAXPOST];
static int vf_sweep;                      /*  scoring qualities, not predicting  */
static long vf_hit[21], vf_seen[21];      /*  posts reproduced, per quality  */
static int vf_pm;                         /*  which predictor is in use  */
static int vf_g;                          /*  the fitted offset, in tenth-dB  */
static double vf_cd[21][2], vf_cy;        /*  cost of each variant, and of the posts  */

/*  Refine libvorbis's refitted posts before differencing.

    The refit's error drifts slowly across a block -- a level offset from the
    encoder not being quite the one that made the stream -- so the residual at
    a post correlates with the residual at its two neighbours (+0.75 measured).
    Those neighbours are always decoded before the post itself, so subtracting
    their mean costs nothing in the meta.  It helps where the encoder is
    mismatched and hurts where it is exact, so the sweep scores both and the
    choice travels as one number.  */
static void vf_refine(vd_floor * f, const i32 * pred, const i32 * res, u32 i,
                      i32 * out) {
  *out = pred[i];
  if (vf_pm == 1 && i >= 2) {
    u32 l = f->lo[i - 2], h = f->hi[i - 2];
    i32 s = res[l] + res[h];
    *out += (s >= 0 ? s + 1 : s - 1) / 2;
  }
}

/*  Differences are signed, and the run marker wants the sign to itself, so
    they travel zigzagged: 0, -1, 1, -2 ... becomes 0, 1, 2, 3 ...  A negative
    value is then free to mean a run of zeros, exactly as it does for the
    posts themselves.  Around half the differences are zero at the setting the
    sweep picks, and they cluster.  */
/*  What a value costs as a TSV field: its digits, a sign, a separator.  */
static double vf_width(i32 v) {
  double w = 2;
  u32 a = (u32) (v < 0 ? -v : v);
  if (v < 0) w += 1;
  while (a >= 10) { a /= 10;  w += 1; }
  return w;
}
static int vf_q;                          /*  the winner, in tenths  */
static int vf_dbg;
static i32 g_unw[VD_MAXCH][VD_MAXPOST];   /*  posts as the encoder fitted them  */

/*  The inverse of vd_dec's unwrap: absolute posts back to the residuals the
    record stream carries.  Zero means "the neighbour interpolation was
    right", which is how the encoder drops a post.  */
static void floor_wrap(vd_floor * f, const i32 * y, i32 * coded) {
  u32 i;
  coded[0] = y[0];  coded[1] = y[1];
  for (i = 2; i < f->posts; i++) {
    u32 l = f->lo[i - 2], h = f->hi[i - 2];
    i32 dy = y[h] - y[l], adx = (i32) f->x[h] - (i32) f->x[l];
    i32 ady = dy < 0 ? -dy : dy;
    i32 pred = ady * ((i32) f->x[i] - (i32) f->x[l]) / adx;
    i32 hiroom, loroom, room, v;
    pred = dy < 0 ? y[l] - pred : y[l] + pred;
    hiroom = (i32) f->quant - pred;  loroom = pred;
    room = (hiroom < loroom ? hiroom : loroom) << 1;
    v = y[i] - pred;
    if (!v) { coded[i] = 0;  continue; }
    /*  the exact inverse of vd_dec::draw's mapping back  */
    if (v < 0) {
      if (-v > loroom && hiroom <= loroom) coded[i] = -1 - v * 2;
      else if (-v >= (hiroom < loroom ? hiroom : loroom) && hiroom <= loroom)
        coded[i] = hiroom - v - 1;
      else coded[i] = -1 - v * 2;
    } else {
      if (v >= (hiroom < loroom ? hiroom : loroom) && hiroom > loroom)
        coded[i] = v + loroom;
      else coded[i] = v * 2;
    }
    if (coded[i] < room) {                /*  keep the two branches consistent  */
      i32 chk = coded[i];
      chk = (chk & 1) ? -((chk + 1) >> 1) : (chk >> 1);
      if (chk != v) coded[i] = (v < 0) ? -1 - v * 2 : v * 2;
    }
  }
}

#endif


/*  The codebook section, per role.

    Reading it is the ordinary case.  In META it is captured and matched
    against libvorbis's static tables; when a setting reproduces it, the meta
    carries the setting instead of the books.  In REST the setting is read back
    and the books regenerated, so the restored stream gets them without their
    ever having been stored.  */
static void su_books(tsv & t, u32 nbk, vd_book * bk) {
  u32 i;
#ifdef BLR_VORBIS
  if (su_role == 1) {                     /*  META: capture, match, decide  */
    static tsv cap;
    tsv * dst = su_dst;
    tsv * save = t.tee;
    sz alen;
    int qq;
    cap.create_mem(cb_a, CB_CAP);
    /*  both paths have to land in the capture: the plain records arrive via
        the tee, the regrouped sparse ones via su_dst  */
    t.tee = &cap;  su_dst = &cap;
    for (i = 0; i < nbk; i++) bk[i].read(t);
    cap.flush_row();  alen = cap.memlen;
    su_dst = dst;  t.tee = save;
    /*  Both libraries are asked, the compiled-in one first so a stream it
        wrote keeps the quality it has always been named by.  Library 1 is the
        generated table, which holds the sets of every released libvorbis --
        among them the ones a 2002-era stream needs, which are in no build of
        ours at all.  */
    /*  -1 is the meta's "no set fits", so library 0 is named by 0..10 only
        and its quality -0.1 is left to library 1, which has that row like any
        other.  */
    cb_q = -1;
    for (qq = 0; qq <= 10 && cb_q < 0; qq++) {
      if (!vb_gen.open(dec_ch, dec_rate, qq)) continue;
      if ((u32) vb_gen.count() != nbk) continue;
      { sz blen = cb_render(vb_gen, cb_b);
        if (blen == alen && !memcmp(cb_a, cb_b, alen)) cb_q = qq; }
    }
    for (int row = vbooks::gen_next(dec_ch, dec_rate, -1);
         row >= 0 && cb_q < 0; row = vbooks::gen_next(dec_ch, dec_rate, row)) {
      /*  Rendering a set is far from free and nearly every row is the wrong
          one, so the shape of the books is checked first: it comes straight
          out of the table and settles almost all of them.  */
      if ((u32) vbooks::gen_count(row) != nbk) continue;
      for (i = 0; i < nbk; i++) {
        const vg_book * g = vbooks::gen_book(row, (int) i);
        if ((u32) g->dim != bk[i].dim || (u32) g->entries != bk[i].ent) break;
      }
      if (i != nbk) continue;
      if (!vb_gen.open(dec_ch, dec_rate, VB_LIB1 + row)) continue;
      { sz blen = cb_render(vb_gen, cb_b);
        if (blen == alen && !memcmp(cb_a, cb_b, alen)) cb_q = VB_LIB1 + row; }
    }
    dst->put("cb.i", cb_q);
    if (cb_q < 0) { dst->flush_row();  dst->emit(cb_a, alen); }
    return;
  }
  if (su_role == 2) {                     /*  REST: regenerate or read  */
    tsv * dst = su_dst;
    tsv * save = t.tee;
    t.tee = nullptr;
    cb_q = (int) t.get("cb.i");
    t.tee = save;
    if (cb_q >= 0 && vb_gen.open(dec_ch, dec_rate, cb_q) &&
        (u32) vb_gen.count() == nbk) {
      for (i = 0; i < nbk; i++) { vb_gen.fill((int) i, bk[i]);
                                  vb_gen.emit((int) i, *dst, 0); }
      return;
    }
    FATAL_UNLESS(cb_q < 0, "meta names codebook set %d, which does not fit "
                 "this stream", cb_q);
  }
#endif
  for (i = 0; i < nbk; i++) bk[i].read(t);
}

/*  One audio packet in the META or REST role.  */
/*  The fitted classifier, written once per setup.  A few dozen integers per
    residue stand in for tens of thousands of class records.  */
static void models_put(tsv & dst, vd_setup & s) {
  u32 i, k;
  for (i = 0; i < s.nrs; i++) {
    clsfit & c = cfit[i];
    dst.put("cm.mode", c.mode);
    if (!c.mode) {
      for (k = 0; k < s.rs[i].ncl; k++) dst.put("cm.a", c.cm1[k]);
      for (k = 0; k < s.rs[i].ncl; k++) dst.put("cm.b", c.cm2[k]);
    } else {
      dst.put("cm.n", c.tn);
      for (k = 0; k < c.tn; k++) dst.put("cm.f", c.tf1[k]);
      for (k = 0; k < c.tn; k++) dst.put("cm.g", c.tf2[k]);
      for (k = 0; k < c.tn; k++) dst.put("cm.c", c.tcl[k]);
    }
  }
}

static void models_get(tsv & src, vd_setup & s) {
  u32 i, k;
  for (i = 0; i < s.nrs; i++) {
    clsfit & c = cfit[i];
    c.reset(s.rs[i].ncl);
    c.mode = (int) src.get_u("cm.mode", 2);
    if (!c.mode) {
      for (k = 0; k < s.rs[i].ncl; k++) c.cm1[k] = (i32) src.get("cm.a");
      for (k = 0; k < s.rs[i].ncl; k++) c.cm2[k] = (i32) src.get("cm.b");
    } else {
      c.tn = src.get_u("cm.n", CF_TAB + 1);
      for (k = 0; k < c.tn; k++) c.tf1[k] = (i32) src.get("cm.f");
      for (k = 0; k < c.tn; k++) c.tf2[k] = (i32) src.get("cm.g");
      for (k = 0; k < c.tn; k++) c.tcl[k] = (u8) src.get_u("cm.c", VD_MAXCLASS);
    }
  }
}

static void audio_rec(tsv & src, tsv & dst, int role) {
  u32 mo, n, i, j, k, ch = dec.s.ch;
  u32 um[(VD_MAXCH + 31) / 32];
  vd_map * mp;
  int W, wp, wn;

  /*  The mode, the next-window flag and the floor-use bits are one scalar
      each per packet, and a record apiece spends more on tag text than on
      the values.  They travel as a single `pk` row: mode, wnext, then the
      floor-use bits packed 32 to a word.  */
  src.tee = nullptr;
  wp = an_prevW;                          /*  derived: the previous block  */
  if (role != ROLE_REST) {                /*  META and FIT read the source  */
    mo = src.get_u("aud.mode", dec.s.nmd);
    W = dec.s.blockflag[mo];
    wn = 0;
    if (W) {
      int truep = (int) src.get_u("aud.wprev", 2);
      wn = (int) src.get_u("aud.wnext", 2);
      if (truep != wp) { dst.put("aud.wpfix", truep);  wp = truep; }
    }
  } else {
    if (!strcmp(src.peek(), "aud.wpfix")) wp = (int) src.get_u("aud.wpfix", 2);
    mo = src.get_u("pk", dec.s.nmd);
    W = dec.s.blockflag[mo];
    wn = (int) src.get_u("pk", 2);
    dst.put("aud.mode", mo);
    if (W) { dst.put("aud.wprev", wp);  dst.put("aud.wnext", wn); }
  }
  an_prevW = W;
  mp = dec.s.mp + dec.s.mdmap[mo];
  n = (W ? dec.s.bs1 : dec.s.bs0) / 2;
  an_covered = block_span(n);
#ifdef BLR_VORBIS
  /*  the floor prediction needs the same windowed span the recovery uses,
      and it runs before the floor records have been read  */
  if ((vf_on || vf_sweep) && an_covered) ana.window(n, wp, W, wn);
#endif

  /*  Floor-use bits, then the posts.  The record stream interleaves them per
      channel; the meta separates them so the posts of the whole packet form
      one run and pay for their tag once.  */
  memset(um, 0, sizeof um);
  if (role != ROLE_REST) {
    for (k = 0; k < ch; k++) {           /*  read in the source's order  */
      vd_floor * f = dec.s.fl + mp->fl[mp->mux[k]];
      vd_used[k] = vd_nz[k] = (u8) src.get_u("flr.used", 2);
      if (!vd_used[k]) continue;
      um[k >> 5] |= 1u << (k & 31);
      for (i = 0; i < f->posts; i++)
        vd_y[k][f->srt[i]] = (i32) src.get_u("flr.y", 0x8000);
    }
    dst.put("pk", mo);  dst.put("pk", wn);
    for (k = 0; k < (ch + 31) / 32; k++) dst.put("pk", um[k]);
    /*  Half the posts code as zero -- the value the neighbour prediction
        already got right -- and they cluster, so a run of n zeros goes out
        as -n.  Real posts are never negative, so the sign is free.  */
    for (k = 0; k < ch; k++) {
      vd_floor * f = dec.s.fl + mp->fl[mp->mux[k]];
      if (!vd_used[k]) continue;
#ifdef BLR_VORBIS
      /*  Where libvorbis can refit this block's floor, the meta carries the
          difference from its fit rather than the posts themselves.  During a
          sweep the same fit is only scored, so the pass can compare encoder
          settings without writing anything.  */
      if ((vf_on || vf_sweep) && an_covered) {
        dec.floor_only(k, vd_flc[k], f, n, g_unw[k]);
        if (vf.fit(apw.seg(an_span, 2 * n), ch, k, n, W, aw_win,
                   f->x, f->posts, f->mult, vf_pred)) {
          if (vf_sweep) {
            for (i = 0; i < f->posts; i++) {
              vf_seen[vf.quality]++;
              if (g_unw[k][i] == vf_pred[i]) vf_hit[vf.quality]++;
            }
            { int pm;
              i32 res[VD_MAXPOST], adj[VD_MAXPOST];
              int save = vf_pm;
              for (pm = 0; pm < 2; pm++) {
                vf_pm = pm;
                for (i = 0; i < f->posts; i++) {
                  vf_refine(f, vf_pred, res, i, adj + i);
                  res[i] = g_unw[k][i] - vf_pred[i];
                }
                for (i = 0; i < f->posts; ) {
                  i32 d = g_unw[k][i] - adj[i];
                  if (d) { vf_cd[vf.quality][pm] += vf_width((i32) vf_zig(d));
                           i++;  continue; }
                  { u32 e = i;
                    while (e < f->posts && g_unw[k][e] == adj[e]) e++;
                    if (e - i < VF_RUNMIN) { vf_cd[vf.quality][pm] += 2;  i++; }
                    else { vf_cd[vf.quality][pm] += vf_width(-(i32) (e - i));
                           i = e; } }
                }
              }
              vf_pm = save;
            }
            /*  and what the posts themselves would have cost, zero runs
                and all, so the sweep compares like with like  */
            for (i = 0; i < f->posts; ) {
              i32 y = vd_y[k][f->srt[i]];
              if (y) { vf_cy += vf_width(y);  i++;  continue; }
              { u32 e = i;
                while (e < f->posts && !vd_y[k][f->srt[e]]) e++;
                if (e - i < VF_RUNMIN) { vf_cy += 2;  i++; }
                else { vf_cy += vf_width(-(i32) (e - i));  i = e; } }
            }
            continue;
          }
          { i32 res[VD_MAXPOST], adj[VD_MAXPOST];
            for (i = 0; i < f->posts; i++) {
              vf_refine(f, vf_pred, res, i, adj + i);
              res[i] = g_unw[k][i] - vf_pred[i];
            }
            for (i = 0; i < f->posts; ) {
              long long z = vf_zig(g_unw[k][i] - adj[i]);
              if (z) { dst.put("flr.d", z);  i++;  continue; }
              { u32 e = i;
                while (e < f->posts && g_unw[k][e] == adj[e]) e++;
                if (e - i < VF_RUNMIN) { dst.put("flr.d", 0);  i++; }
                else { dst.put("flr.d", -(long long) (e - i));  i = e; } }
            }
          }
          continue;
        }
        if (vf_sweep) continue;
      }
#endif
      for (i = 0; i < f->posts; ) {
        i32 y = vd_y[k][f->srt[i]];
        if (y) { dst.put("flr.y", y);  i++;  continue; }
        { u32 e = i;
          while (e < f->posts && !vd_y[k][f->srt[e]]) e++;
          if (e - i < VF_RUNMIN) { dst.put("flr.y", 0);  i++; }
          else { dst.put("flr.y", -(long long) (e - i));  i = e; } }
      }
    }
  } else {
    for (k = 0; k < (ch + 31) / 32; k++) um[k] = src.get_u("pk", 0x100000000ULL);
    for (k = 0; k < ch; k++) {           /*  ... and back interleaved  */
      vd_floor * f = dec.s.fl + mp->fl[mp->mux[k]];
      vd_used[k] = vd_nz[k] = (u8) ((um[k >> 5] >> (k & 31)) & 1);
      dst.put("flr.used", vd_used[k]);
      if (!vd_used[k]) continue;
#ifdef BLR_VORBIS
      /*  the same refit mode c made, plus the stored difference  */
      if (vf_on && an_covered &&
          vf.fit(apw.seg(an_span, 2 * n), ch, k, n, W, aw_win,
                 f->x, f->posts, f->mult, vf_pred)) {
        i32 unw[VD_MAXPOST], cod[VD_MAXPOST], res[VD_MAXPOST], adj;
        for (i = 0; i < f->posts; ) {
          long long z = src.get("flr.d");
          if (z > 0) {
            vf_refine(f, vf_pred, res, i, &adj);
            unw[i] = adj + vf_unzig(z);
            res[i] = unw[i] - vf_pred[i];  i++;  continue;
          }
          FATAL_UNLESS(z < 0 ? (u32) -z <= f->posts - i : 1,
                       "%s: floor difference run overruns the post list",
                       src.path);
          { long long r = z ? -z : 1;
            while (r--) {
              vf_refine(f, vf_pred, res, i, &adj);
              unw[i] = adj;  res[i] = unw[i] - vf_pred[i];  i++;
            } }
        }
        floor_wrap(f, unw, cod);
        for (i = 0; i < f->posts; i++) {
          dst.put("flr.y", cod[f->srt[i]]);
          vd_y[k][f->srt[i]] = cod[f->srt[i]];
        }
        continue;
      }
#endif
      for (i = 0; i < f->posts; ) {
        long long y = src.get("flr.y");
        if (y > 0) { vd_y[k][f->srt[i++]] = (i32) y;  continue; }
        if (!y) { vd_y[k][f->srt[i++]] = 0;  continue; }
        FATAL_UNLESS(y < 0 && (u32) -y <= f->posts - i,
                     "%s: floor run of %lld overruns the post list",
                     src.path, y);
        { long long r = -y;
          while (r--) vd_y[k][f->srt[i++]] = 0; }
      }
      for (i = 0; i < f->posts; i++) dst.put("flr.y", vd_y[k][f->srt[i]]);
    }
  }
  for (k = 0; k < ch; k++) {
    if (!vd_used[k]) { memset(vd_flc[k], 0, n * sizeof *vd_flc[k]);  continue; }
    dec.floor_only(k, vd_flc[k], dec.s.fl + mp->fl[mp->mux[k]], n);
  }
  for (i = 0; i < mp->nstep; i++)
    if (vd_nz[mp->mag[i]] || vd_nz[mp->ang[i]])
      vd_nz[mp->mag[i]] = vd_nz[mp->ang[i]] = 1;

  rec.packet_begin();
  rec.covered = recover(n, W, wp, wn, ch, mp);

  if (role == ROLE_REST) {                /*  corrections arrive before use  */
    { long long a = 0, b = 0;
      if (!strcmp(src.peek(), "kn")) { a = src.get("kn");  b = src.get("kn"); }
      read_keeps(src, "kc.i", "kc.v", a, rec.kc_n, kc_i, kc_v, rec.kc_raw);
      read_keeps(src, "kd.i", "kd.v", b, rec.kd_n, kd_i, kd_v, rec.kd_raw); }
  }

  rec.role = role;  rec.src = &src;  rec.dst = &dst;
  for (i = 0; i < mp->sub; i++) {
    i64 * bundle[VD_MAXCH];
    u8 nzb[VD_MAXCH];
    u32 m = 0;
    for (j = 0; j < ch; j++)
      if (mp->mux[j] == i) { nzb[m] = vd_nz[j];  bundle[m++] = aw_r[j]; }
    if (m) rec.residue(mp->rs[i], bundle, nzb, m, n);
  }

  if (role == ROLE_META) {
    /*  The two correction counts are known together and are one small number
        each; a row apiece spends more on tag text than on the values, so they
        share a row.  */
    { long long a = keep_count(rec.kc_n, rec.ci, kc_i, kc_all),
                b = keep_count(rec.kd_n, rec.di, kd_i, kd_all);
      if (a || b) { dst.put("kn", a);  dst.put("kn", b); }
      write_keeps(dst, "kc.i", "kc.v", a, rec.kc_n, kc_i, kc_v, kc_all, rec.ci);
      write_keeps(dst, "kd.i", "kd.v", b, rec.kd_n, kd_i, kd_v, kd_all, rec.di); }
  }
  src.tee = &dst;                         /*  structural records flow again  */
}

/*  One pass over a record stream.  */
static void walk(int role, const char * srcpath, const char * dstpath,
                 const char * wavpath) {
  tsv src, dst;
  pg p;
  int open = 0;
  u32 link = 0;

  src.open(srcpath);
  if (role == ROLE_META || role == ROLE_REST) dst.create(dstpath);
  else if (role == ROLE_FIT) dst.create(DEV_NULL);
  dec.s.have = 0;  lk_n = 0;  tot_before = 0;

  if (role == ROLE_META || role == ROLE_REST) src.tee = &dst;
  while (src.get_u("link.more", 2)) {
    unsigned long long last = 0;
    int w = 0, done = 0, cont = 0;
    u32 serial = 0, seq = 0;
    int prevtail = 0;
    sz spill = 0;
    if (open) { dec.link();  an_first = 1;  an_prevW = 0; }
    if (role == ROLE_META) { src.tee = nullptr;
                             dst.put("link.frames", (long long) lk_frames[link]);
                             src.tee = &dst; }
    if (role == ROLE_REST) { src.tee = nullptr;
                             lk_frames[link] = src.get("link.frames");
                             src.tee = &dst; }
    lk_n = link + 1;
    if (link) an_base += lk_frames[link - 1];
    while (!done) {
      int j;
      src.tee = nullptr;
      if (role == ROLE_REST) {
        if (!seq) serial = src.get_u("link.serial", 0x100000000ULL);
        p.get_meta(src, serial, seq, seq == 0, prevtail);
        p.put(dst);
      } else {
        p.get(src);
        if (role == ROLE_META) {
          u32 want = (u32) ((seq == 0 ? 2 : 0) | (prevtail ? 1 : 0)) |
                     (u32) (p.type & 4);
          /*  Only the pages after the first are checked against the link
              serial; the first page is where `serial` is read from, just
              below, so there is nothing to compare it with yet.  */
          FATAL_UNLESS(p.type == want && p.seq == seq &&
                       (!seq || p.serial == serial),
                       "%s: page header is not in canonical form; this stream "
                       "needs page.type, page.seq or page.serial carried",
                       srcpath);
          if (!seq) { serial = p.serial;  dst.put("link.serial", serial); }
          p.put_meta(dst);
        }
      }
      src.tee = (role == ROLE_META || role == ROLE_REST) ? &dst : nullptr;
      prevtail = p.tail;
      seq++;
      if (p.np) last = ((unsigned long long) p.ghi << 32) | p.glo;
      Fj((int) p.np,
        sz pl = p.plen[j];
        sz len = pl;
        if (j == 0 && (p.type & 1) && cont) {
          FATAL_UNLESS(pl <= spill, "%s: page overruns a continued packet", srcpath);
          spill -= pl;
          if (!(p.np == 1 && p.tail)) cont = 0;
          continue;
        }
        if (j == (int) p.np - 1 && p.tail) {
          sz ex;
          src.tee = nullptr;
          ex = (sz) src.get_u("page.spill", 0x100000000ULL);
          if (role != ROLE_SYNTH) dst.put("page.spill", (long long) ex);
          src.tee = (role == ROLE_SYNTH) ? nullptr : &dst;
          len = pl + ex;  spill = ex;  cont = 1;
        }
        if (w < 3) {
          if (w == 0) { dec.s.ident(src);  dec_ch = dec.s.ch;  dec_rate = dec.s.rate; }
          else if (w == 1) dec.s.comment(src, len);
          else {
            su_dst = (role == ROLE_META || role == ROLE_REST) ? &dst : nullptr;
            su_role = (role == ROLE_META) ? 1 : (role == ROLE_REST ? 2 : 0);
            dec.s.setup(src);
            src.tee = nullptr;
            if (role == ROLE_FIT)
              for (u32 q = 0; q < dec.s.nrs; q++)
                if (!cfit[q].ncl) cfit[q].reset(dec.s.rs[q].ncl);
            if (role == ROLE_META) {
              models_put(dst, dec.s);
              VF_QREC(dst);
            }
            if (role == ROLE_REST) {
              models_get(src, dec.s);
              VF_QGET(src);
            }
            src.tee = (role == ROLE_META || role == ROLE_REST) ? &dst : nullptr;
            dec.md.setup(dec.s.bs0, dec.s.bs1);
            dec.lift.setup(dec.s.bs0 / 2, dec.s.bs1 / 2);
            ana.s = &dec.s;  ana.lift = &dec.lift;  ana.md = &dec.md;
            ana.pw = &apw;   rec.s = &dec.s;
            ana.reset_windows();  rec.build_reach();
            if (!open) {
              if (role == ROLE_SYNTH) { sink.create(wavpath, dec.s.rate, dec.s.ch);
                                        dec.start(&sink); }
              else apw.open(wavpath, dec.s.ch, dec.s.rate);
              an_base = 0;  an_first = 1;  an_prevW = 0;  open = 1;
            }
          }
          w++;
        } else if (role == ROLE_SYNTH) dec.audio(src);
        else audio_rec(src, dst, role));
      if (p.type & 4) {
        done = 1;
        if (role == ROLE_SYNTH) {
          dec.finish(last);
          lk_frames[link] = (i64) sink.frames - tot_before;
          tot_before = (i64) sink.frames;
        }
      }
    }
    link++;
    FATAL_UNLESS(link < LK_MAX, "%s: too many links", srcpath);
  }
  FATAL_UNLESS(open, "%s: no Vorbis stream", srcpath);
  src.tee = nullptr;
  /*  the terminating link.more was echoed by the tee already  */
  if (role == ROLE_META) meta_bytes = (long) dst.bytes;
  if (role != ROLE_SYNTH) dst.close();
  src.close();
  if (role == ROLE_SYNTH) sink.close();
  else apw.close();
}

/*  Write the WAV with the shaper currently set, then learn the encoder's
    classifier from it.  Both depend on the samples, so a new placement needs
    both again; the fit accumulates, so it is cleared rather than added to.  */
static void synth_and_fit(const char * src, const char * wav) {
  u32 q;
  for (q = 0; q < VD_MAXRES; q++) cfit[q].ncl = 0;
  walk(ROLE_SYNTH, src, nullptr, wav);
  walk(ROLE_FIT, src, nullptr, wav);
  for (q = 0; q < VD_MAXRES; q++) if (cfit[q].ncl) cfit[q].settle();
}

int main(int argc, char ** argv) {
  if (argc != 5 || (argv[1][0] != 'c' && argv[1][0] != 'd') || argv[1][1]) {
    fprintf(stderr,
      "usage: tsv2wav c input.tsv output.wav output.meta\n"
      "       tsv2wav d output.wav restored.tsv output.meta\n");
    return BLR_EXIT_USAGE;
  }
  blr_set_prog(argv[0]);
  blr_paths_distinct(argv + 2, 3);
#ifdef BLR_VORBIS
#ifdef BLR_VORBIS
  vf_dbg = getenv("TSV2WAV_SWEEP") != nullptr;
#endif
#endif
  if (argv[1][0] == 'c') {
    /*  How the synthesis lands its samples is swept below; these are the
        coefficients tried.  Over 37 files, choosing per file from the whole
        grid is worth 2.12% of the meta and choosing from these four is worth
        2.10%, so the rest of the grid is not worth the passes.  Zero has to
        be among them: on some files every amount of feedback costs.

        Two neighbouring knobs were measured and are not swept, because they
        do not pay.  The lifting's rounding rule -- nearest, floor, ceiling or
        truncate -- moves the meta by under 0.03%, its shears rounding at
        2^-32 against a sample step of 2^-16.  (An earlier note here said the
        four produce byte-identical WAVs.  They do not; the effect is real,
        just far too small to chase.)  And the split point is best at nearest:
        floor or ceiling cost about a percent, quarter steps either side 0.4
        to 0.7.  */
    static const double WAV_SHAPES[] = { 0.0, 0.3, 0.5, 0.8 };
    constexpr int NSHAPE = (int) (sizeof WAV_SHAPES / sizeof *WAV_SHAPES);
    wav_shape = WAV_SHAPES[0];
    synth_and_fit(argv[2], argv[3]);
#ifdef BLR_VORBIS
    /*  Sweep the encoder settings and keep whichever reproduces the most
        posts.  On a stream whose encoder is reproducible this has a sharp
        maximum at the setting actually used.  */
#ifndef BLR_NOSWEEP
    { int qq, best = -1, bestpm = 0;  double bs = 0;
      for (qq = 0; qq <= 10; qq++) {
        if (!vf.open(dec.s.ch, dec.s.rate, qq)) continue;
        if (vf_dbg) fprintf(stderr, "  sweep q=%.1f ", qq * 0.1);
        vf_sweep = 1;  vf_hit[qq] = vf_seen[qq] = 0;
        vf_cd[qq][0] = vf_cd[qq][1] = vf_cy = 0;
        walk(ROLE_META, argv[2], DEV_NULL, argv[3]);
        vf_sweep = 0;
        if (vf_dbg) fprintf(stderr, "posts %ld exact %.1f%%  cost %.0f vs %.0f\n",
                            (long) vf_seen[qq],
                            vf_seen[qq] ? 100.0 * (double) vf_hit[qq] / (double) vf_seen[qq] : 0.0,
                            vf_cd[qq][0], vf_cy);
        /*  keep the setting whose differences are cheapest, and only if
            they beat carrying the posts unchanged  */
        /*  Keep the best ratio even when it is below one: the level offset
            swept next can move it, and only the final figure decides whether
            the differences are carried at all.  */
        { int pm;
          for (pm = 0; pm < 2; pm++)
            if (vf_seen[qq] > 0 && vf_cd[qq][pm] > 0) {
              double s = vf_cy / vf_cd[qq][pm];
              if (s > bs) { bs = s;  best = qq;  bestpm = pm; }
            } }
      }
      if (best >= 0) {
        int gg, bestg = 0;
        /*  now the level offset, at the setting just chosen  */
        for (gg = -30; gg <= 30; gg += 3) {
          vf.open(dec.s.ch, dec.s.rate, best);
          vf.gain = gg * 0.1f;
          vf_sweep = 1;  vf_hit[best] = vf_seen[best] = 0;
          vf_cd[best][0] = vf_cd[best][1] = vf_cy = 0;
          walk(ROLE_META, argv[2], DEV_NULL, argv[3]);
          vf_sweep = 0;
          if (vf_seen[best] > 0 && vf_cd[best][bestpm] > 0) {
            double s = vf_cy / vf_cd[best][bestpm];
            if (s > bs) { bs = s;  bestg = gg; }
          }
        }
        if (vf_dbg) fprintf(stderr, "  best q=%.1f pm=%d gain=%+.1f dB, "
                            "ratio %.3f%s\n", best * 0.1, bestpm, bestg * 0.1,
                            bs, bs > 1.0 ? "" : " (posts are cheaper; not used)");
        if (bs > 1.0) {
          vf.open(dec.s.ch, dec.s.rate, best);
          vf.gain = bestg * 0.1f;
          vf_q = best;  vf_pm = bestpm;  vf_g = bestg;  vf_on = 1;
        }
      }
    }
#endif
#endif
    /*  Now the synthesis itself.  Feeding each sample's quantization error
        into the next shapes the error spectrum by 1 - h/z, draining it away
        from the low frequencies where most of the digits are; it is worth up
        to 12.9% of a file's meta and costs as much as that on another, so it
        is chosen per file rather than fixed.  Nothing has to be carried: mode d
        reads back whatever was written.

        The candidates are scored with the floor refit switched off.  What
        the refit writes does not depend on the samples' placement in any way
        the argument here turns on -- the corrections do -- and leaving it out
        makes a scoring pass twenty-five times cheaper than the sweep above,
        which is the difference between this costing a tenth of the run and
        half of it.  Measured over 37 files, scoring this way picks the same
        coefficient as scoring the whole meta.  */
    { int k, bestk = 0;  long bestb = -1;
#ifdef BLR_VORBIS
      int vf_save = vf_on;
#endif
      for (k = 0; k < NSHAPE; k++) {
        if (k) { wav_shape = WAV_SHAPES[k];  synth_and_fit(argv[2], argv[3]); }
#ifdef BLR_VORBIS
        vf_on = 0;
#endif
        walk(ROLE_META, argv[2], DEV_NULL, argv[3]);
#ifdef BLR_VORBIS
        vf_on = vf_save;
#endif
        if (bestb < 0 || meta_bytes < bestb) { bestb = meta_bytes;  bestk = k; }
      }
      /*  the WAV on disk is the last candidate's; put the winner's back  */
      if (bestk != NSHAPE - 1) {
        wav_shape = WAV_SHAPES[bestk];  synth_and_fit(argv[2], argv[3]);
      }
    }
    walk(ROLE_META, argv[2], argv[4], argv[3]);
  } else
    walk(ROLE_REST, argv[4], argv[3], argv[2]);
  return BLR_EXIT_OK;
}
