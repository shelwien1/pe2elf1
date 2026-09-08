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

/*  tsvcomp -- a balrogg record stream, compressed.

      tsvcomp c [options] input.tsv output.tc
      tsvcomp d [options] input.tc  restored.tsv

    tsvtrans rearranges a record stream and leaves the coding to somebody
    else; this one does the coding.  It walks the same structure -- the walk
    is tsvtrans's, and has to be, since what a record means is decided by
    records already read -- and instead of writing a value out as text hands
    it to a model and a range coder.

    The model is mp3c's, described in MP3C-ALGORITHM.md.  Every value is a
    cascade of binary decisions and every decision goes through the same four
    parts: two counters on different contexts, an APM correcting the first,
    a logistic mixer weighing them, and a carryless binary range coder.  The
    encoder and the decoder are the same code -- `tc_fam::code` either codes
    the value it is given or decodes one and both then run the identical
    update -- so a model change cannot desynchronise the two halves.

    What is different from mp3c is which variables the contexts are built
    from, and those are not guesses.  TSVTRANS.md measured three of them on a
    real stream, one coordinate at a time:

      * a residue digit is predicted by its partition and its cascade pass
        (-4.76%) and by the digit before it (-2.72%), and *not* by its column
        inside the partition (+0.27%, and worse in combination);
      * a floor post is predicted by its column and almost nothing else
        (-22.20% on a 19-post floor, where the previous post is +0.28%);
      * a residue class is predicted by the class before it (-15.13%).

    To those this adds the one context mp3c leans on hardest and tsvtrans had
    no way to test: the same slot one packet ago.  `ShiftMDCT` there keeps
    three granules of history per spectral line because a line does not change
    much between two granules; the same is true of a Vorbis residue digit, of
    a floor post, and of the class of a partition, so all three carry one
    packet of history here, kept separately for short and long blocks.  It is
    worth 2.33%.

    And it adds one that neither of them suggested and that turned out to be
    the largest single factor in the program: **the class of the partition the
    digit is in**, worth 4.65% against the four buckets of it this started
    with.  The class is the choice of ladder and the ladder decides what a
    digit can be -- which, once the model has it, leaves the frequency band
    that TSVTRANS.md measured contributing 0.05%.  The band had been standing
    in for the class.  TSVCOMP.md has that measurement and the ones that
    settled every other pattern.

    The contexts themselves are declared, not written -- see idx.inc.  The
    block of IDX text below is the whole of what this program knows about how
    a variable becomes a table row, and editing a threshold pattern in it is
    the entire edit.

    Values the stream implies are not coded as they stand: the granule, the
    page sequence and the page serial go out as residuals against what the
    walk already knows, so the usual case is a run of zeros.  Nothing is
    dropped outright and no input is refused for having an unusual page
    header -- a residual that is not zero simply costs a few bits.

    The three header packets of a link -- identification, comment, setup --
    are coded as records rather than as text or as bytes.  Their tag sequence
    is not information: both sides run the same reader, which asks for the
    tags it needs in the order it needs them, so only the values are coded.
    See tc_header.  */

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

#include "common.inc"      /*  types, diagnostics, loop macros  */
#include "tsv.inc"         /*  the record stream  */
#include "lift.inc"        /*  fixed point; fx() for the setup's floats  */
#include "floor_db.inc"    /*  the dB table, for imdct.inc's sake  */
#include "imdct.inc"       /*  not used here; MD_MAXBLK bounds the setup  */
#include "vd_setup.inc"    /*  codebooks, floors, residues, mappings, modes  */
#include "sh_v2f.inc"      /*  the range coder  */
#include "cm.inc"          /*  counter, APM, mixer  */
#include "idx.inc"         /*  contexts as declarations  */

/*  vd_setup calls this to read the codebook section.  tsv2wav substitutes a
    whole set from an index here; there is nothing to substitute in a stream
    that has to come back byte for byte, so this is the plain reading.  */
static void su_books(tsv & t, u32 nbk, vd_book * bk) {
  u32 i;
  for (i = 0; i < nbk; i++) bk[i].read(t);
}

constexpr int OGG_MAXSEG = 255;
constexpr u32 LK_MAX = 4096;

constexpr char TC_MAGIC[] = "tsvc\x1a";
constexpr int  TC_VER = 1;

static int tc_enc;                        /*  1 = c, 0 = d  */
static tsv * tc_in;                       /*  mode c: the stream being read  */
static tsv * tc_out;                      /*  mode d: the stream being written  */
static vd_setup su;
static Rangecoder rc;
static rc_buf rcb;
static idx_decl tc_idx;
static u16 tc_idxsum;                     /*  which declaration coded a stream  */

/*  ------------------------------------------------------------------
    The declaration.

    One `Index` per (family, role): `_a` is the rich counter, `_b` the coarse
    one, `_s` the APM's own grouping and `_m` the mixer's.  mp3c computes four
    different context indices over the same variables with different
    thresholds for exactly this reason -- each of the four stages wants a
    different grouping, and sharing one is a compromise none of them asked
    for.

    A factor whose pattern is all zeroes contributes one bucket and is
    dropped, which is how a variable that was tried and found not to pay
    stays visible in the source.  `col` in the digit model is the measured
    example: it is declared, at zero, because the natural thing to assume
    about it is wrong.  */
static const char TC_IDXTEXT[] =
"Prefix TC\n"
"Debug 1\n"
"Const 0\n"
"\n"
"#  Residue digits: 92% of the records in a stream, so this is the program.\n"
"#\n"
"#  `cls` is the biggest single factor here and the most surprising one: the\n"
"#  class is the choice of ladder, and the ladder decides what a digit can be,\n"
"#  so knowing it is worth more than knowing where in the spectrum the digit\n"
"#  sits.  Sixteen buckets of it beat four by 0.9%.\n"
"#\n"
"#  `bkq` is the same thought in a form that would generalise -- the book's\n"
"#  `off`, which is the half-width of its multiplicand range, in four buckets\n"
"#  instead of sixteen.  It is left at zero because it measured 0.9% *worse*\n"
"#  than the class number it was meant to replace: two classes with the same\n"
"#  range are still not the same choice.\n"
"#\n"
"#  `col`, the position inside the partition, is at zero because TSVTRANS.md\n"
"#  measured it at +0.27% on its own and worse in combination.  `band` is at\n"
"#  three buckets because the fourth onwards are worth 0.03% between them once\n"
"#  `cls` is in: a partition's identity was standing in for its class.\n"
"\n"
"Index dig_a\n"
" a_rno:  rno,   1!0\n"
" a_pass: pass,  1!1111111\n"
" a_band: band,  1!110000000000000\n"
" a_col:  col,   1!0000000\n"
" a_q1:   q1,    1!11111\n"
" a_q2:   q2,    1!1111\n"
" a_t1:   t1,    1!111\n"
" a_bkq:  bkq,   1!0000\n"
" a_cls:  cls,   1!111111111111111\n"
" a_zrun: zrun,  1!1111\n"
" a_blk:  blk,   1!0\n"
"\n"
"Index dig_b\n"
" b_rno:  rno,   1!0\n"
" b_pass: pass,  1!1111111\n"
" b_band: band,  1!000000000000000\n"
" b_col:  col,   1!0000000\n"
" b_q1:   q1,    1!11111\n"
" b_q2:   q2,    1!0000\n"
" b_t1:   t1,    1!111\n"
" b_bkq:  bkq,   1!0000\n"
" b_cls:  cls,   1!000\n"
" b_zrun: zrun,  1!111\n"
" b_blk:  blk,   1!1\n"
"\n"
"Index dig_s\n"
" s_pass: pass,  1!11\n"
" s_band: band,  1!111\n"
" s_q1:   q1,    1!111\n"
" s_t1:   t1,    1!000\n"
" s_bkq:  bkq,   1!000\n"
" s_cls:  cls,   1!111\n"
"\n"
"Index dig_m\n"
" m_pass: pass,  1!111\n"
" m_band: band,  1!111\n"
" m_q1:   q1,    1!000\n"
" m_t1:   t1,    1!1\n"
" m_bkq:  bkq,   1!000\n"
" m_cls:  cls,   1!000\n"
"\n"
"#  The mantissa of a digit that escaped the ladder: its own plane, on its own\n"
"#  and much coarser context.\n"
"\n"
"Index dig_t\n"
" t_pass: pass,  1!111\n"
" t_band: band,  1!111\n"
" t_q1:   q1,    1!11111\n"
" t_t1:   t1,    1!000\n"
" t_bkq:  bkq,   1!000\n"
" t_cls:  cls,   1!111111111111111\n"
"\n"
"Number dig_rA, 1, 0!11111111\n"
"Number dig_rB, 1, 0!11111111\n"
"Number dig_rS, 1, 0!00111\n"
"Number dig_lr, 1, 0!01000\n"
"\n"
"#  Floor posts.  The column is the whole story here -- post 0 and post 1 are\n"
"#  coded raw and land at the ends of the row, everything between them is a\n"
"#  folded residual at a frequency the column names -- so it gets 48 buckets\n"
"#  where the digit model gives its own position none.  `p1` is what this post\n"
"#  was one packet ago, and it is the second factor: a floor curve is the\n"
"#  spectral envelope, and an envelope moves slowly.\n"
"\n"
"Index flr_a\n"
" a_fno:  fno,   1!1\n"
" a_col:  col,   1!111111111111111111111111111111111111111111111111\n"
" a_p1:   p1,    1!1111111\n"
" a_n1:   n1,    1!111\n"
" a_raw:  raw,   1!0\n"
" a_blk:  blk,   1!0\n"
"\n"
"Index flr_b\n"
" b_fno:  fno,   1!0\n"
" b_col:  col,   1!0000000\n"
" b_p1:   p1,    1!11111111111111\n"
" b_n1:   n1,    1!1111111\n"
" b_raw:  raw,   1!1\n"
" b_blk:  blk,   1!1\n"
"\n"
"Index flr_s\n"
" s_col:  col,   1!1111111\n"
" s_p1:   p1,    1!111\n"
" s_n1:   n1,    1!000\n"
"\n"
"Index flr_m\n"
" m_col:  col,   1!111\n"
" m_p1:   p1,    1!111\n"
" m_n1:   n1,    1!000\n"
"\n"
"Index flr_t\n"
" t_col:  col,   1!111111111111111111111111111111111111111111111111\n"
" t_p1:   p1,    1!111\n"
" t_n1:   n1,    1!000\n"
"\n"
"Number flr_rA, 1, 0!11111111\n"
"Number flr_rB, 1, 0!11111111\n"
"Number flr_rS, 1, 0!00110\n"
"Number flr_lr, 1, 0!00110\n"
"\n"
"#  Residue classes: the class before it (-15.13% on its own, measured in\n"
"#  TSVTRANS.md) and the class the same partition had one packet ago.\n"
"\n"
"Index cls_a\n"
" a_rno:  rno,   1!1\n"
" a_band: band,  1!111111111111111\n"
" a_prev: prev,  1!1111111\n"
" a_t1:   t1,    1!1111111\n"
" a_blk:  blk,   1!1\n"
"\n"
"Index cls_b\n"
" b_rno:  rno,   1!0\n"
" b_band: band,  1!000000000000000\n"
" b_prev: prev,  1!111111111111111\n"
" b_t1:   t1,    1!111\n"
" b_blk:  blk,   1!0\n"
"\n"
"Index cls_s\n"
" s_band: band,  1!111\n"
" s_prev: prev,  1!111\n"
" s_t1:   t1,    1!000\n"
"\n"
"Index cls_m\n"
" m_band: band,  1!000\n"
" m_prev: prev,  1!111\n"
" m_t1:   t1,    1!1\n"
"\n"
"Index cls_t\n"
" t_prev: prev,  1!111\n"
" t_band: band,  1!111\n"
"\n"
"Number cls_rA, 1, 0!11111111\n"
"Number cls_rB, 1, 0!11111111\n"
"Number cls_rS, 1, 0!00110\n"
"Number cls_lr, 1, 0!00110\n"
"\n"
"#  Everything the page and packet headers hold, one field id apart.  Each\n"
"#  field is a few thousand records at most, so what these want is separation,\n"
"#  not resolution: `fld` is a dense factor and the rest is the field's own\n"
"#  recent history.\n"
"\n"
"Index aux_a\n"
"ADD 32: fld\n"
" a_p1:   p1,    1!111111111111111\n"
" a_p2:   p2,    1!1111111\n"
"\n"
"Index aux_b\n"
"ADD 32: fld\n"
" b_p1:   p1,    1!111\n"
" b_p2:   p2,    1!000\n"
"\n"
"Index aux_s\n"
"ADD 32: fld\n"
"\n"
"Index aux_m\n"
"ADD 32: fld\n"
"\n"
"Index aux_t\n"
"ADD 32: fld\n"
"\n"
"Number aux_rA, 1, 0!11111111\n"
"Number aux_rB, 1, 0!11111111\n"
"Number aux_rS, 1, 0!00110\n"
"Number aux_lr, 1, 0!00110\n"
"\n"
"#  The three header packets, coded record by record.  The tag is a dense\n"
"#  factor because the alphabet is small and every entry in it is a different\n"
"#  kind of number; p1r is the previous value under the same tag, whole, which\n"
"#  is what makes cb.len and cmt.byte an order-1 byte model.\n"
"\n"
"Index hdr_a\n"
"ADD 64: tag\n"
" a_p1:   p1,    1!111111111111111\n"
" a_pos:  pos,   1!111\n"
" a_p2:   p2,    1!0000000\n"
"\n"
"Index hdr_b\n"
" b_p1r:  p1r,   &11111111\n"
" b_pos:  pos,   1!1\n"
"\n"
"Index hdr_s\n"
"ADD 64: tag\n"
"\n"
"Index hdr_m\n"
"ADD 64: tag\n"
"\n"
"Index hdr_t\n"
"ADD 64: tag\n"
" t_p1r:  p1r,   &11111111\n"
"\n"
"Number hdr_rA, 1, 0!11111111\n"
"Number hdr_rB, 1, 0!11111111\n"
"Number hdr_rS, 1, 0!00110\n"
"Number hdr_lr, 1, 0!00110\n";

/*  The variable vectors.  A factor line above names one of these; a name that
    is not here is a startup error rather than a silently wrong context.  */
enum { DV_RNO, DV_PASS, DV_BAND, DV_COL, DV_Q1, DV_Q2, DV_T1, DV_CLS,
       DV_ZRUN, DV_BLK, DV_BKQ, DV_N };
static const char * const TC_VDIG[] =
  { "rno", "pass", "band", "col", "q1", "q2", "t1", "cls", "zrun", "blk",
    "bkq", nullptr };

enum { FV_FNO, FV_COL, FV_P1, FV_N1, FV_RAW, FV_BLK, FV_N };
static const char * const TC_VFLR[] =
  { "fno", "col", "p1", "n1", "raw", "blk", nullptr };

enum { CV_RNO, CV_BAND, CV_PREV, CV_T1, CV_BLK, CV_N };
static const char * const TC_VCLS[] =
  { "rno", "band", "prev", "t1", "blk", nullptr };

enum { AV_FLD, AV_P1, AV_P2, AV_N };
static const char * const TC_VAUX[] = { "fld", "p1", "p2", nullptr };

enum { HV_TAG, HV_P1, HV_P2, HV_POS, HV_P1R, HV_N };
static const char * const TC_VHDR[] =
  { "tag", "p1", "p2", "pos", "p1r", nullptr };

/*  ------------------------------------------------------------------
    One binary decision, and the tree of them that makes a number.  */

/*  Where in the cascade a bit sits.  A family's tables are this many rows deep
    per context, so the ladder step and the length bit are context in their own
    right without being declared as a variable.

    The mantissa is a plane of its own, on its own index and its own tables --
    mp3c's `Smant` is likewise a separate submodel.  Folded into the head the
    mantissa positions would be six times the depth, multiplying the whole of
    a rich context by a part of the cascade that carries the least and is
    reached the least: the head of a digit is asked 921,600 ways and its
    fourteenth mantissa bit does not need to be.  */
constexpr int TC_NODE  = 30;              /*  0-2 ladder, 3-28 length, 29 sign  */
constexpr int TC_MNODE = 169;             /*  13 lengths x 13 positions  */
constexpr int TC_SIGN  = 29;
constexpr int TC_NBMAX = 62;              /*  bit lengths the cascade admits  */

static INLINE int tc_node_len(int k) { return 3 + (k < 25 ? k : 25); }
static INLINE int tc_node_man(int nb, int p) {
  return (nb > 12 ? 12 : nb) * 13 + (p > 12 ? 12 : p);
}

/*  A quantised magnitude: exact to 3, then two buckets per octave.  Contexts
    take values through this, so a counter is not asked to tell 1013 from
    1014.  It is mp3c's `caMDCTj` idea with the thresholds fixed rather than
    swept, because the sweep is what an optimizer is for and there is none
    here yet.  */
static INLINE i32 tc_qlog(i64 x) {
  i32 n = 0;
  if (x < 0) x = -x;
  if (x < 4) return (i32) x;
  while (x >= 4) { x >>= 1;  n++; }
  return 2 + n * 2 + (i32) (x - 2);
}

static INLINE int tc_clamp(int v, int lo, int hi) {
  return v < lo ? lo : v > hi ? hi : v;
}

static sz tc_mem;                         /*  what has been allocated  */
static sz tc_tab;                         /*  of which the model tables  */
static void * tc_alloc(sz n) {
  void * p = calloc(n ? n : 1, 1);
  if (!p) FATAL_CODE(BLR_EXIT_IO, "out of memory for %" PRIu64 " bytes", (u64) n);
  tc_mem += n;
  return p;
}
static cm_cnt * tc_cnt(sz n) {
  cm_cnt * p = (cm_cnt *) tc_alloc(n * sizeof *p);
  sz i;
  for (i = 0; i < n; i++) p[i].init();
  return p;
}

/*  Where the bits went, for -v.  Accounting only; it changes no output, and
    is the same trick MP3C-ALGORITHM.md's per-stage table came from.  */
enum { STG_HDR, STG_PAGE, STG_AUD, STG_FLOOR, STG_CLASS, STG_DIGIT, STG_N };
static const char * const TC_STAGE[STG_N] =
  { "headers", "pages", "packets", "floor", "class", "digits" };
static double tc_bits[STG_N];
static u64 tc_syms[STG_N];
static int tc_stage, tc_verbose;

/*  Code one bit at probability `p` -- that it is zero, 12 bits.  */
static INLINE int tc_bit(int p, int bit) {
  uint b = (uint) bit;
  if (tc_verbose && tc_enc)
    tc_bits[tc_stage] -= log2(bit ? 1.0 - p / (double) CM_PONE
                                  : p / (double) CM_PONE);
  b = rc.rc_BProcess((uint) (p << 3), b);
  return (int) b;
}

/*  A family: four context indices, the tables they address, and the cascade
    that turns a number into bits.  */
struct tc_fam {
  const idx_index * ia, * ib, * is, * im, * it;
  cm_cnt * A, * B, * G, * T;
  u16 * S;
  i32 * W;
  cm_apm ap;
  cm_mix<2> mx;
  u32 ba, bb, bs, bm, bt;
  int rA, rB, rS, lr;

  void init(const char * fam, u32 nsign) {
    char nm[IDX_NAME];
    snprintf(nm, sizeof nm, "%s_a", fam);  ia = tc_idx.index(nm);
    snprintf(nm, sizeof nm, "%s_b", fam);  ib = tc_idx.index(nm);
    snprintf(nm, sizeof nm, "%s_s", fam);  is = tc_idx.index(nm);
    snprintf(nm, sizeof nm, "%s_m", fam);  im = tc_idx.index(nm);
    snprintf(nm, sizeof nm, "%s_t", fam);  it = tc_idx.index(nm);
    A = tc_cnt((sz) ia->vol * TC_NODE);
    B = tc_cnt((sz) ib->vol * TC_NODE);
    T = tc_cnt((sz) it->vol * TC_MNODE);
    G = nsign ? tc_cnt(nsign) : nullptr;
    S = (u16 *) tc_alloc((sz) is->vol * TC_NODE * 33 * sizeof *S);
    W = (i32 *) tc_alloc((sz) im->vol * TC_NODE * 2 * sizeof *W);
    ap.init(S, is->vol * TC_NODE);
    mx.init(W, im->vol * TC_NODE);
    /*  A pattern is a search space and its ends get visited, so anything
        used as a size, a shift or a limit is clamped at the point of use --
        IDX/IDX-FORMAT.md §5 says the same, and means it.  */
    snprintf(nm, sizeof nm, "%s_rA", fam);  rA = tc_clamp(tc_idx.number(nm), 1, CM_TMAX);
    snprintf(nm, sizeof nm, "%s_rB", fam);  rB = tc_clamp(tc_idx.number(nm), 1, CM_TMAX);
    snprintf(nm, sizeof nm, "%s_rS", fam);  rS = tc_clamp(tc_idx.number(nm), 1, 15);
    snprintf(nm, sizeof nm, "%s_lr", fam);  lr = tc_clamp(tc_idx.number(nm), 1, 64);
  }

  INLINE void select(const i32 * v) {
    ba = ia->build(v) * TC_NODE;
    bb = ib->build(v) * TC_NODE;
    bs = is->build(v) * TC_NODE;
    bm = im->build(v) * TC_NODE;
    bt = it->build(v) * TC_MNODE;
  }

  /*  mp3c's six lines, in order: refine counter A through the APM, stretch
      both, mix them under one learned weight, code, then update all four.  */
  INLINE int bit(int node, int b) {
    cm_cnt & a = A[ba + node];
    cm_cnt & c = B[bb + node];
    int p1 = ap.pp(a.P(), bs + (u32) node);
    mx.add(cm_stretch(p1));
    mx.add(cm_stretch(c.P()));
    b = tc_bit(mx.mix(bm + (u32) node), b);
    mx.upd(b, lr);
    ap.upd(b, rS);
    a.upd(b, rA);
    c.upd(b, rB);
    return b;
  }

  /*  A mantissa bit: one counter, no mixing.  What is left after the length
      has been coded is close to uniform, and a stage that cannot pay for
      itself should not be run four million times.  */
  INLINE int bitm(int node, int b) {
    cm_cnt & t = T[bt + node];
    b = tc_bit(t.P(), b);
    t.upd(b, rA);
    return b;
  }

  /*  A non-negative integer: three direct steps, then the bit length of what
      is left in unary, then its mantissa.  Small values -- which is nearly
      all of them -- cost one, two or three decisions, and nothing has to
      know a field's range in advance.  */
  i64 code(const i32 * v, i64 x) {
    int k, p, nb = 0;
    u64 u;
    select(v);
    if (tc_enc) FATAL_UNLESS(x >= 0, "negative magnitude");
    if (bit(0, tc_enc && x == 0)) return 0;
    if (bit(1, tc_enc && x == 1)) return 1;
    if (bit(2, tc_enc && x == 2)) return 2;
    u = tc_enc ? (u64) (x - 3) + 1 : 1;
    if (tc_enc) { u64 t = u;  while (t) { nb++;  t >>= 1; } }
    for (k = 0; ; k++) {
      if (bit(tc_node_len(k), tc_enc && nb == k + 1)) { nb = k + 1;  break; }
      FATAL_UNLESS(k + 2 <= TC_NBMAX, "%s: a value of 2^%d or more",
                   tc_enc ? "this stream holds" : "coded stream", TC_NBMAX);
    }
    if (!tc_enc) u = 1;
    for (p = 0; p < nb - 1; p++) {
      int mb = bitm(tc_node_man(nb, p),
                    tc_enc && ((u >> (nb - 2 - p)) & 1) != 0);
      if (!tc_enc) u = u * 2 + (u64) mb;
    }
    return (i64) (u - 1) + 3;
  }

  /*  The same, with a sign under its own small context: the sign of a
      residue digit correlates with the sign the same slot had a packet ago
      and with the digit before it, and with nothing the magnitude cascade is
      looking at.  */
  i64 codes(const i32 * v, i64 x, u32 sc) {
    i64 m = code(v, tc_enc ? (x < 0 ? -x : x) : 0);
    int s;
    if (!m) return 0;
    if (G) {
      cm_cnt & g = G[sc];
      s = tc_bit(g.P(), tc_enc && x < 0);
      g.upd(s, rA);
    } else s = bit(TC_SIGN, tc_enc && x < 0);
    return s ? -m : m;
  }
};

static tc_fam fam_dig, fam_flr, fam_cls, fam_aux, fam_hdr;

/*  ------------------------------------------------------------------
    The fields that are not floor, class or digit.  */

enum {
  F_MORE, F_PGTYPE, F_GRAN, F_SERIAL, F_SEQ, F_NPKT, F_PLEN, F_TAIL,
  F_SPILL, F_MODE, F_WPREV, F_WNEXT, F_USED, F_NHDR, F_TAGSAME, F_TAGIDX,
  F_TAGLEN, F_TAGCHR, F_N
};
static i64 tc_last[F_N], tc_last2[F_N];

/*  `c` is whatever the caller knows that the field's own history does not.  */
static i64 tc_auxc(int fld, i32 c, i64 x, int sgn) {
  i32 v[AV_N];
  i64 r;
  v[AV_FLD] = fld;
  v[AV_P1] = c;
  v[AV_P2] = tc_qlog(tc_last[fld]);
  r = sgn ? fam_aux.codes(v, x, (u32) fld) : fam_aux.code(v, x);
  tc_last2[fld] = tc_last[fld];  tc_last[fld] = r;
  return r;
}
static i64 tc_aux(int fld, i64 x) {
  return tc_auxc(fld, tc_qlog(tc_last2[fld]), x, 0);
}

/*  ------------------------------------------------------------------
    History: the same slot, one packet ago.  */

constexpr sz TC_SPANMAX = 1u << 17;       /*  digits one residue may hold  */
constexpr sz TC_HISTMAX = 1u << 23;       /*  and slots its history may take  */

static i16 * dg_hist[VD_MAXRES];
static u8  * cl_hist[VD_MAXRES];
static sz    dg_span[VD_MAXRES], cl_np[VD_MAXRES];
static i32 * dg_q1, * dg_q2, * dg_zr;     /*  [rno][pass][channel]  */
static i32   cl_last[VD_MAXRES];
static i16 * fl_hist;                     /*  [blk][channel][post]  */
static u32   tc_blk;                      /*  the block this packet is  */
static u32   tc_nchan;

static void tc_hist_free(void) {
  u32 i;
  for (i = 0; i < VD_MAXRES; i++) {
    free(dg_hist[i]);  dg_hist[i] = nullptr;
    free(cl_hist[i]);  cl_hist[i] = nullptr;
    dg_span[i] = cl_np[i] = 0;
  }
  free(dg_q1);  free(dg_q2);  free(dg_zr);  free(fl_hist);
  dg_q1 = dg_q2 = dg_zr = nullptr;  fl_hist = nullptr;
}

/*  A link's setup has just been read; size the histories it implies.  A
    residue whose span is beyond what is worth holding simply loses its
    temporal context -- the other contexts still apply, so the stream stays
    codable rather than refused.  */
static void tc_setup_done(void) {
  u32 i;
  tc_hist_free();
  tc_nchan = su.ch;
  for (i = 0; i < su.nrs; i++) {
    vd_res * r = su.rs + i;
    sz span = r->end > r->beg ? r->end - r->beg : 0;
    sz need;
    if (span > TC_SPANMAX) span = 0;
    need = (sz) 2 * 8 * tc_nchan * span;
    if (need > TC_HISTMAX) span = 0;
    dg_span[i] = span;
    if (span) dg_hist[i] = (i16 *) tc_alloc((sz) 2 * 8 * tc_nchan * span * sizeof(i16));
    cl_np[i] = span ? span / r->psz + 2 : 0;
    if (cl_np[i]) cl_hist[i] = (u8 *) tc_alloc((sz) 2 * tc_nchan * cl_np[i]);
    cl_last[i] = 0;
  }
  dg_q1 = (i32 *) tc_alloc((sz) su.nrs * 8 * tc_nchan * sizeof(i32));
  dg_q2 = (i32 *) tc_alloc((sz) su.nrs * 8 * tc_nchan * sizeof(i32));
  dg_zr = (i32 *) tc_alloc((sz) su.nrs * 8 * tc_nchan * sizeof(i32));
  fl_hist = (i16 *) tc_alloc((sz) 2 * tc_nchan * VD_MAXPOST * sizeof(i16));
}

/*  ------------------------------------------------------------------
    The header packets.

    Both sides run the same reader over the same records, so what has to
    cross is the values and, once each, the spelling of a tag.  The tag of a
    record is nearly always the tag of the record before it -- balrogg emits
    runs -- so that is one bit, and a first sighting costs its letters.  */

constexpr int TC_MAXTAG = 128;

/*  The capture buffer, which grows to whatever a link's header packets turn
    out to need.  It cannot be sized from the packet length: a codebook
    declares its entry count in three bytes and every entry is a record, so
    the records a setup packet yields are a property of the stream and not of
    its length.  */
static char * tc_hbuf;
static sz tc_hcap;
static char tc_tagbuf[TC_MAXTAG * (TSV_TAGMAX + 1)];
static const char * tc_tag[TC_MAXTAG];
static u32 tc_ntag;
static int tc_lasttag = -1;
static i64 tc_tlast[TC_MAXTAG], tc_tlast2[TC_MAXTAG];
static i64 tc_runpos;

static int tc_tagcode(const char * tag) {
  int id = -1, same;
  if (tc_enc) {
    u32 i;
    for (i = 0; i < tc_ntag; i++)
      if (!strcmp(tc_tag[i], tag)) { id = (int) i;  break; }
  }
  same = (int) tc_auxc(F_TAGSAME, tc_lasttag < 0 ? 0 : tc_lasttag,
                       tc_enc && id >= 0 && id == tc_lasttag, 0);
  if (same) {
    FATAL_UNLESS(tc_lasttag >= 0, "coded stream: a tag repeats nothing");
    id = tc_lasttag;
  } else {
    id = (int) tc_auxc(F_TAGIDX, tc_lasttag < 0 ? 0 : tc_lasttag,
                       tc_enc ? (id < 0 ? (i64) tc_ntag : id) : 0, 0);
    FATAL_UNLESS(id >= 0 && (u32) id <= tc_ntag && id < TC_MAXTAG,
                 "coded stream: tag %d is out of range", id);
    if ((u32) id == tc_ntag) {
      char * dst = tc_tagbuf + (sz) tc_ntag * (TSV_TAGMAX + 1);
      u32 L = (u32) tc_auxc(F_TAGLEN, 0, tc_enc ? (i64) strlen(tag) : 0, 0);
      u32 k;
      FATAL_UNLESS(L && L <= TSV_TAGMAX, "coded stream: tag length %u", L);
      for (k = 0; k < L; k++)
        dst[k] = (char) tc_auxc(F_TAGCHR, k ? (u8) dst[k - 1] : 0,
                                tc_enc ? (u8) tag[k] : 0, 0);
      dst[L] = 0;
      tc_tag[tc_ntag++] = dst;
    }
  }
  tc_runpos = (id == tc_lasttag) ? tc_runpos + 1 : 0;
  tc_lasttag = id;
  return id;
}

static i64 tc_hdrval(int id, i64 x) {
  i32 v[HV_N];
  v[HV_TAG] = id;
  v[HV_P1] = tc_qlog(tc_tlast[id]);
  v[HV_P2] = tc_qlog(tc_tlast2[id]);
  v[HV_POS] = tc_qlog(tc_runpos);
  v[HV_P1R] = (i32) (tc_tlast[id] & 255);
  x = fam_hdr.codes(v, x, (u32) id);
  tc_tlast2[id] = tc_tlast[id];  tc_tlast[id] = x;
  return x;
}

/*  Mode c: the packet is read from the input while a memory stream captures
    it, and the capture is then re-read record by record and coded.  Mode d:
    the records are decoded into a memory stream, which the same reader then
    parses -- with a tee to the output, so they reach the restored file in
    the order and under the tags they were read with.  */
static void tc_header(int which, sz plen, tsv & out) {
  tsv mem;
  memset(&mem, 0, sizeof mem);
  tc_stage = STG_HDR;
  if (tc_enc) {
    tsv r;
    u32 n = 0;
    char tg[TSV_TAGMAX + 1];
    mem.create_mem(tc_hbuf, tc_hcap, 1);
    tc_in->tee = &mem;
    if (which == 0) su.ident(*tc_in);
    else if (which == 1) su.comment(*tc_in, plen);
    else { su_role = 0;  su_dst = nullptr;  su.setup(*tc_in);  su.have = 1; }
    tc_in->tee = nullptr;
    mem.close();
    tc_hbuf = mem.mem;  tc_hcap = mem.memcap;
    memset(&r, 0, sizeof r);
    r.open_mem(tc_hbuf, mem.memlen);
    while (*r.peek()) {
      snprintf(tg, sizeof tg, "%s", r.peek());
      r.get(tg);  n++;
    }
    tc_aux(F_NHDR, n);
    memset(&r, 0, sizeof r);
    r.open_mem(tc_hbuf, mem.memlen);
    while (*r.peek()) {
      i64 val;
      snprintf(tg, sizeof tg, "%s", r.peek());
      val = r.get(tg);
      tc_hdrval(tc_tagcode(tg), val);
    }
    if (which == 2) tc_setup_done();
  } else {
    tsv w;
    u32 n = (u32) tc_aux(F_NHDR, 0), i;
    memset(&w, 0, sizeof w);
    w.create_mem(tc_hbuf, tc_hcap, 1);
    for (i = 0; i < n; i++) {
      int id = tc_tagcode(nullptr);
      w.put(tc_tag[id], tc_hdrval(id, 0));
    }
    w.close();
    tc_hbuf = w.mem;  tc_hcap = w.memcap;
    mem.open_mem(tc_hbuf, w.memlen);
    mem.tee = &out;
    if (which == 0) su.ident(mem);
    else if (which == 1) su.comment(mem, plen);
    else { su_role = 0;  su_dst = nullptr;  su.setup(mem);  su.have = 1; }
    mem.tee = nullptr;
    FATAL_UNLESS(!*mem.peek(), "coded stream: the header packet has records "
                 "the reader did not want");
    if (which == 2) tc_setup_done();
  }
}

/*  ------------------------------------------------------------------
    The audio packets.  */

/*  One residue partition.  */
static void tc_part(u32 rno, u32 pss, u32 j, u32 pc, u32 psz, u32 cls,
                    i32 bkq) {
  sz span = dg_span[rno];
  i16 * hist = span ? dg_hist[rno]
                    + ((sz) (tc_blk * 8 + pss) * tc_nchan + j) * span
                    : nullptr;
  sz base = (sz) ((rno * 8 + pss) * tc_nchan + j);
  i32 * q1 = dg_q1 + base, * q2 = dg_q2 + base, * zr = dg_zr + base;
  u32 i;
  tc_stage = STG_DIGIT;
  for (i = 0; i < psz; i++) {
    sz slot = (sz) pc * psz + i;
    i32 v[DV_N];
    i32 t1 = (hist && slot < span) ? hist[slot] : 0;
    i64 d;
    u32 sc;
    v[DV_RNO] = (i32) rno;
    v[DV_PASS] = (i32) pss;
    v[DV_BAND] = tc_qlog(pc);
    v[DV_COL] = tc_qlog(i);
    v[DV_Q1] = tc_qlog(*q1);
    v[DV_Q2] = tc_qlog(*q2);
    v[DV_T1] = tc_qlog(t1);
    v[DV_CLS] = (i32) cls;
    v[DV_ZRUN] = tc_qlog(*zr);
    v[DV_BLK] = (i32) tc_blk;
    v[DV_BKQ] = bkq;
    sc = (u32) ((((rno & 3) * 8 + pss) * 3 + (*q1 < 0 ? 0 : *q1 > 0 ? 2 : 1)) * 3
                + (t1 < 0 ? 0 : t1 > 0 ? 2 : 1));
    d = tc_enc ? tc_in->get("res.digit") : 0;
    d = fam_dig.codes(v, d, sc);
    if (!tc_enc) tc_out->put("res.digit", d);
    if (hist && slot < span)
      hist[slot] = (i16) (d < -32768 ? -32768 : d > 32767 ? 32767 : d);
    *q2 = *q1;  *q1 = (i32) d;
    *zr = d ? 0 : *zr + 1;
    if (tc_verbose) tc_syms[STG_DIGIT]++;
  }
}

static u32 tc_classify(u32 rno, u32 j, u32 slot) {
  i32 v[CV_N];
  u8 * hist = cl_np[rno] ? cl_hist[rno] + ((sz) tc_blk * tc_nchan + j) * cl_np[rno]
                         : nullptr;
  i32 t1 = (hist && slot < cl_np[rno]) ? hist[slot] : 0;
  i64 c;
  tc_stage = STG_CLASS;
  v[CV_RNO] = (i32) rno;
  v[CV_BAND] = tc_qlog(slot);
  v[CV_PREV] = cl_last[rno];
  v[CV_T1] = t1;
  v[CV_BLK] = (i32) tc_blk;
  c = tc_enc ? (i64) tc_in->get_u("res.class", 16) : 0;
  c = fam_cls.code(v, c);
  if (!tc_enc) tc_out->put("res.class", c);
  FATAL_UNLESS(c >= 0 && c < 16, "coded stream: residue class %" PRId64, c);
  if (hist && slot < cl_np[rno]) hist[slot] = (u8) c;
  cl_last[rno] = (i32) c;
  if (tc_verbose) tc_syms[STG_CLASS]++;
  return (u32) c;
}

/*  The residue walk of io.inc, over records rather than bits.  */
static u8 tc_cl[1u << 20];
static void tc_residue(u32 rno, const u8 * nz, u32 nch, u32 n) {
  vd_res * r = su.rs + rno;
  vd_book * cb = su.bk + r->cbook;
  u32 vch, end, np, pv, pss, pc, i, j, k, w, nc = 0;
  for (i = 0; i < nch; i++) if (nz[i]) nc++;
  if (!nc) return;
  if (r->type == 2) { vch = 1;  end = blr_min(r->end, n * nch); }
  else { vch = nc;  end = blr_min(r->end, n); }
  if (end <= r->beg) return;
  np = (end - r->beg) / r->psz;
  if (!np) return;
  pv = cb->dim;
  w = np + pv;
  FATAL_UNLESS((sz) vch * w <= sizeof tc_cl,
               "residue needs %" PRIu64 " classification slots, limit %" PRIu64,
               (u64) ((sz) vch * w), (u64) sizeof tc_cl);
  for (pss = 0; pss < 8; pss++) {
    pc = 0;
    while (pc < np) {
      if (!pss)
        for (j = 0; j < vch; j++)
          for (k = 0; k < pv; k++)
            tc_cl[j * w + pc + k] = (u8) tc_classify(rno, j, pc + k);
      for (k = 0; k < pv; k++) {
        if (pc >= np) break;
        for (j = 0; j < vch; j++) {
          u32 c = tc_cl[j * w + pc];
          i32 bn;
          FATAL_UNLESS(c < r->ncl, "residue class out of range");
          bn = r->book[c][pss];
          /*  The class is the choice of ladder, and what the ladder decides
              is how wide the alphabet is: `off` is the centre of the book's
              multiplicand range, so a digit from it lies in about [-off,
              off].  Handing the model that rather than the class number is
              the same information in four buckets instead of sixteen, and it
              means the same thing in the next file.  */
          if (bn >= 0)
            tc_part(rno, pss, j, pc, r->psz, c, tc_qlog(su.bk[bn].off));
        }
        pc++;
      }
    }
  }
}

/*  Floor curves, then the residues, for one audio packet.  */
static u8 tc_used[VD_MAXCH];
static void tc_payload(u32 mode) {
  vd_map * mp = su.mp + su.mdmap[mode];
  u32 n = (su.blockflag[mode] ? su.bs1 : su.bs0) / 2;
  u32 ch = su.ch, k, i, j, m;
  u8 nz[VD_MAXCH], sub[VD_MAXCH];
  memset(nz, 0, sizeof nz);
  for (k = 0; k < ch; k++) {
    u32 fno = mp->fl[mp->mux[k]];
    vd_floor * f = su.fl + fno;
    i16 * hp = fl_hist + ((sz) tc_blk * tc_nchan + k) * VD_MAXPOST;
    i32 prev = 0;
    u32 u;
    tc_stage = STG_AUD;
    u = (u32) tc_auxc(F_USED, tc_used[k], tc_enc ? tc_in->get_u("flr.used", 2) : 0, 0);
    FATAL_UNLESS(u < 2, "coded stream: flr.used is %u", u);
    if (!tc_enc) tc_out->put("flr.used", u);
    tc_used[k] = nz[k] = (u8) u;
    if (!u) continue;
    tc_stage = STG_FLOOR;
    for (i = 0; i < f->posts; i++) {
      i32 v[FV_N];
      i64 y;
      v[FV_FNO] = (i32) fno;
      v[FV_COL] = (i32) (i < VD_MAXPOST ? i : VD_MAXPOST - 1);
      v[FV_P1] = tc_qlog(hp[i]);
      v[FV_N1] = tc_qlog(prev);
      v[FV_RAW] = i < 2;
      v[FV_BLK] = (i32) tc_blk;
      y = tc_enc ? tc_in->get("flr.y") : 0;
      y = fam_flr.codes(v, y, 0);
      if (!tc_enc) tc_out->put("flr.y", y);
      hp[i] = (i16) (y < -32768 ? -32768 : y > 32767 ? 32767 : y);
      prev = (i32) y;
      if (tc_verbose) tc_syms[STG_FLOOR]++;
    }
  }
  for (i = 0; i < mp->nstep; i++)
    if (nz[mp->mag[i]] || nz[mp->ang[i]])
      nz[mp->mag[i]] = nz[mp->ang[i]] = 1;
  for (i = 0; i < mp->sub; i++) {
    m = 0;
    for (j = 0; j < ch; j++) if (mp->mux[j] == i) sub[m++] = nz[j];
    if (m) tc_residue(mp->rs[i], sub, m, n);
  }
}

static u32 tc_prevW;                      /*  the block before this one  */
static u32 tc_prevmode;
static void tc_audio(void) {
  u32 md;
  tc_stage = STG_AUD;
  md = (u32) tc_auxc(F_MODE, (i32) tc_prevmode,
                     tc_enc ? tc_in->get_u("aud.mode", su.nmd) : 0, 0);
  FATAL_UNLESS(md < su.nmd, "coded stream: audio names mode %" PRIu64, (u64) md);
  if (!tc_enc) tc_out->put("aud.mode", md);
  tc_prevmode = md;
  tc_blk = su.blockflag[md];
  if (su.blockflag[md]) {
    /*  wprev names the block before this one, which both sides have already
        walked.  A stream that says otherwise is not wrong -- the decoder
        uses what it is told -- so it is coded against that expectation
        rather than derived from it, and disagreeing costs a few bits.  */
    u32 wp = (u32) tc_auxc(F_WPREV, (i32) tc_prevW,
                           tc_enc ? tc_in->get_u("aud.wprev", 2) : 0, 0);
    u32 wn;
    FATAL_UNLESS(wp < 2, "coded stream: aud.wprev is %u", wp);
    wn = (u32) tc_auxc(F_WNEXT, (i32) wp,
                       tc_enc ? tc_in->get_u("aud.wnext", 2) : 0, 0);
    FATAL_UNLESS(wn < 2, "coded stream: aud.wnext is %u", wn);
    if (!tc_enc) { tc_out->put("aud.wprev", wp);  tc_out->put("aud.wnext", wn); }
  }
  tc_prevW = su.blockflag[md];
  tc_payload(md);
}

/*  ------------------------------------------------------------------
    The page header.

    Three of its fields are not information in the ordinary case: the type
    follows from position, the sequence counts from zero within a link and
    the serial is one number per link.  tsvtrans drops them and refuses a
    stream that disagrees; here they are coded as residuals against the same
    expectations, which costs the same nothing when they hold and keeps the
    tool working when they do not.  */
struct tc_pg {
  u32 type, glo, ghi, serial, seq, np;
  u32 plen[OGG_MAXSEG];
  int tail;
};

static i64 pg_prev;
static u32 pg_prevtype, pg_prevser;

static void tc_page(tc_pg & p, u32 seq, int bos, int cont, tsv & out) {
  u32 i;
  i64 g;
  tc_stage = STG_PAGE;
  if (tc_enc) {
    p.type = tc_in->get_u("page.type", 0x100);
    p.glo = tc_in->get_u("page.granlo", (u64) 0x100000000);
    p.ghi = tc_in->get_u("page.granhi", (u64) 0x100000000);
    p.serial = tc_in->get_u("page.serial", (u64) 0x100000000);
    p.seq = tc_in->get_u("page.seq", (u64) 0x100000000);
    p.np = tc_in->get_u("page.npkt", OGG_MAXSEG + 1);
    for (i = 0; i < p.np; i++)
      p.plen[i] = tc_in->get_u("page.plen", (u64) OGG_MAXSEG * OGG_MAXSEG + 1);
    p.tail = 0;
    if (p.np && !(p.plen[p.np - 1] % OGG_MAXSEG))
      p.tail = (int) tc_in->get_u("page.tail", 2);
  }

  /*  the type against what position implies  */
  { u32 want = (u32) ((bos ? 2 : 0) | (cont ? 1 : 0));
    i64 d = tc_auxc(F_PGTYPE, (i32) (want * 4 + (pg_prevtype & 3)),
                    tc_enc ? (i64) p.type - (i64) want : 0, 1);
    if (!tc_enc) {
      FATAL_UNLESS(d + (i64) want >= 0 && d + (i64) want < 0x100,
                   "coded stream: page type %" PRId64, d + (i64) want);
      p.type = (u32) (d + (i64) want);
    }
    pg_prevtype = p.type; }

  g = tc_enc ? (i64) (((u64) p.ghi << 32) | p.glo) : 0;
  g = tc_auxc(F_GRAN, tc_qlog(tc_last[F_GRAN]), tc_enc ? g - pg_prev : 0, 1) + pg_prev;
  pg_prev = g;
  if (!tc_enc) { p.glo = (u32) ((u64) g & 0xFFFFFFFFu);  p.ghi = (u32) ((u64) g >> 32); }

  { i64 d = tc_auxc(F_SERIAL, bos, tc_enc ? (i64) p.serial - (i64) pg_prevser : 0, 1);
    if (!tc_enc) p.serial = (u32) ((i64) pg_prevser + d);
    pg_prevser = p.serial; }

  { i64 d = tc_auxc(F_SEQ, 0, tc_enc ? (i64) p.seq - (i64) seq : 0, 1);
    if (!tc_enc) p.seq = (u32) ((i64) seq + d); }

  p.np = (u32) tc_aux(F_NPKT, tc_enc ? p.np : 0);
  FATAL_UNLESS(p.np <= OGG_MAXSEG, "coded stream: page holds %" PRIu64 " packets",
               (u64) p.np);
  { i64 prev = 0;
    for (i = 0; i < p.np; i++) {
      i64 v = tc_auxc(F_PLEN, tc_qlog(prev), tc_enc ? p.plen[i] : 0, 0);
      FATAL_UNLESS(v >= 0 && v <= (i64) OGG_MAXSEG * OGG_MAXSEG,
                   "coded stream: packet length %" PRId64, v);
      p.plen[i] = (u32) v;
      prev = v;
    } }
  if (p.np && !(p.plen[p.np - 1] % OGG_MAXSEG)) {
    i64 v = tc_aux(F_TAIL, tc_enc ? p.tail : 0);
    FATAL_UNLESS(v >= 0 && v < 2, "coded stream: page tail %" PRId64, v);
    p.tail = (int) v;
  } else p.tail = 0;

  if (!tc_enc) {
    out.put("page.type", p.type);
    out.put("page.granlo", p.glo);   out.put("page.granhi", p.ghi);
    out.put("page.serial", p.serial);  out.put("page.seq", p.seq);
    out.put("page.npkt", p.np);
    for (i = 0; i < p.np; i++) out.put("page.plen", p.plen[i]);
    if (p.np && !(p.plen[p.np - 1] % OGG_MAXSEG)) out.put("page.tail", p.tail);
  }
}

/*  ------------------------------------------------------------------
    One pass over the stream.  The traversal is the same in both directions
    -- it has to be, since the shape of what follows is decided by records
    already read -- so the two modes differ only in where a value comes from
    and where it goes.  */
static void tc_walk(const char * inpath, const char * outpath) {
  tsv in, out;
  FILE * bf;
  tc_pg p;
  u32 link = 0;

  memset(&in, 0, sizeof in);
  memset(&out, 0, sizeof out);
  memset(&p, 0, sizeof p);

  if (tc_enc) {
    u8 h[4];
    in.open(inpath);
    tc_in = &in;
    bf = fopen(outpath, "wb");
    if (!bf) FATAL_CODE(BLR_EXIT_IO, "cannot create %s", outpath);
    h[0] = (u8) TC_VER;  h[1] = 0;
    h[2] = (u8) (tc_idxsum & 0xFF);  h[3] = (u8) (tc_idxsum >> 8);
    if (fwrite(TC_MAGIC, 1, sizeof TC_MAGIC - 1, bf) != sizeof TC_MAGIC - 1 ||
        fwrite(h, 1, 4, bf) != 4)
      FATAL_CODE(BLR_EXIT_IO, "write error on %s", outpath);
    rcb.attach(bf, 1 << 16);
    rc.StartEncode(&rcb);
  } else {
    char m[sizeof TC_MAGIC - 1];
    u8 h[4];
    bf = fopen(inpath, "rb");
    if (!bf) FATAL_CODE(BLR_EXIT_IO, "cannot open %s", inpath);
    FATAL_UNLESS(fread(m, 1, sizeof m, bf) == sizeof m &&
                 !memcmp(m, TC_MAGIC, sizeof m),
                 "%s: not a tsvcomp stream -- `tsvcomp c` makes one", inpath);
    FATAL_UNLESS(fread(h, 1, 4, bf) == 4,
                 "%s: the stream ends in its header", inpath);
    FATAL_UNLESS(h[0] == TC_VER,
                 "%s: made by tsvcomp version %u, this is version %u",
                 inpath, h[0], TC_VER);
    FATAL_UNLESS(((u32) h[2] | ((u32) h[3] << 8)) == tc_idxsum,
                 "%s: coded against a different context declaration (%04x, "
                 "this one is %04x) -- set TSVCOMP_IDX to the file it was "
                 "made with", inpath, (u32) h[2] | ((u32) h[3] << 8),
                 (u32) tc_idxsum);
    rcb.attach(bf, 1 << 16);
    rc.StartDecode(&rcb);
    out.create(outpath);
    tc_out = &out;
  }

  for (;;) {
    int done = 0, cont = 0, w = 0;
    u32 seq = 0;
    int prevtail = 0;
    sz spill = 0;
    i64 more;
    tc_stage = STG_PAGE;
    more = tc_aux(F_MORE, tc_enc ? (i64) in.get_u("link.more", 2) : 0);
    FATAL_UNLESS(more >= 0 && more < 2, "coded stream: link.more is %" PRId64, more);
    if (!tc_enc) out.put("link.more", more);
    if (!more) break;

    pg_prev = 0;  tc_prevW = 0;  tc_prevmode = 0;  pg_prevtype = 0;
    pg_prevser = 0;
    memset(tc_used, 0, sizeof tc_used);
    su.have = 0;
    while (!done) {
      u32 j;
      tc_page(p, seq, seq == 0, prevtail, out);
      prevtail = p.tail;
      seq++;
      for (j = 0; j < p.np; j++) {
        sz pl = p.plen[j], len = pl;
        if (j == 0 && (p.type & 1) && cont) {
          FATAL_UNLESS(pl <= spill, "%s: page overruns a continued packet",
                       tc_enc ? inpath : outpath);
          spill -= pl;
          if (!(p.np == 1 && p.tail)) cont = 0;
          continue;
        }
        if (j == p.np - 1 && p.tail) {
          i64 ex;
          tc_stage = STG_PAGE;
          ex = tc_aux(F_SPILL, tc_enc ? (i64) in.get_u("page.spill",
                                                       (u64) 0x100000000) : 0);
          FATAL_UNLESS(ex >= 0, "coded stream: page spill %" PRId64, ex);
          if (!tc_enc) out.put("page.spill", ex);
          len = pl + (sz) ex;  spill = (sz) ex;  cont = 1;
        }
        if (w < 3) tc_header(w++, len, out);
        else {
          FATAL_UNLESS(su.have, "%s: audio before setup",
                       tc_enc ? inpath : outpath);
          tc_audio();
        }
      }
      if (p.type & 4) done = 1;
      FATAL_UNLESS(seq < (1u << 30), "%s: a link never ends",
                   tc_enc ? inpath : outpath);
    }
    link++;
    FATAL_UNLESS(link < LK_MAX, "%s: too many links", tc_enc ? inpath : outpath);
  }

  if (tc_enc) {
    rc.FinishEncode();
    rcb.flush();
    { int bad = ferror(bf);
      if (fclose(bf)) bad = 1;
      if (bad) FATAL_CODE(BLR_EXIT_IO, "write error on %s", outpath); }
    in.close();
  } else {
    out.close();
    fclose(bf);
  }
  rcb.free_();
}

/*  ------------------------------------------------------------------  */

/*  The declaration is compiled in, and TSVCOMP_IDX replaces it with the
    contents of a file: that is how the measurements in TSVCOMP.md were made,
    and it is what an optimizer would drive if one were pointed at this.

    A stream carries a checksum of the declaration that coded it, because the
    declaration *is* the format -- move one threshold and the same bytes mean
    something else.  Two builds that disagree meet a message naming both
    checksums rather than a decode that runs to the end and produces
    nonsense.  */
static char * tc_idxfile;

static const char * tc_declaration(void) {
  const char * env = getenv("TSVCOMP_IDX");
  const char * text = TC_IDXTEXT;
  const char * q;
  u32 h = 2166136261u;
  if (env) {
    FILE * f = fopen(env, "rb");
    sz n = 0, cap = 1 << 16;
    if (!f) FATAL_CODE(BLR_EXIT_IO, "cannot open %s", env);
    tc_idxfile = (char *) malloc(cap);
    if (!tc_idxfile) FATAL_CODE(BLR_EXIT_IO, "out of memory");
    for (;;) {
      sz got = fread(tc_idxfile + n, 1, cap - n - 1, f);
      n += got;
      if (n + 1 < cap) break;
      cap *= 2;
      { char * t = (char *) realloc(tc_idxfile, cap);
        if (!t) FATAL_CODE(BLR_EXIT_IO, "out of memory");
        tc_idxfile = t; }
    }
    fclose(f);
    tc_idxfile[n] = 0;
    text = tc_idxfile;
  }
  for (q = text; *q; q++) { h ^= (u8) *q;  h *= 16777619u; }
  tc_idxsum = (u16) (h ^ (h >> 16));
  return text;
}

static void tc_models(void) {
  const char * text = tc_declaration();
  cm_tables();
  tc_idx.init();
  tc_idx.family("dig", TC_VDIG);
  tc_idx.family("flr", TC_VFLR);
  tc_idx.family("cls", TC_VCLS);
  tc_idx.family("aux", TC_VAUX);
  tc_idx.family("hdr", TC_VHDR);
  tc_idx.parse(text);
  fam_dig.init("dig", 4 * 8 * 3 * 3);
  fam_flr.init("flr", 1);
  fam_cls.init("cls", 1);
  fam_aux.init("aux", F_N);
  fam_hdr.init("hdr", TC_MAXTAG);
  tc_tab = tc_mem;                        /*  the histories come later, per link  */
}

int main(int argc, char ** argv) {
  int a = 1;
  const char * mode;
  if (argc < 4) goto usage;
  mode = argv[a++];
  if ((mode[0] != 'c' && mode[0] != 'd') || mode[1]) goto usage;
  for (; a < argc && argv[a][0] == '-' && argv[a][1]; a++) {
    const char * o = argv[a] + 1;
    for (; *o; o++)
      switch (*o) {
        case 'v': tc_verbose = 1;  break;
        default: goto usage;
      }
  }
  if (argc - a != 2) goto usage;
  blr_set_prog(argv[0]);
  blr_paths_distinct(argv + a, 2);
  tc_enc = mode[0] == 'c';
  tc_models();
  tc_walk(argv[a], argv[a + 1]);
  if (tc_verbose) {
    int i;
    double tot = 0;
    for (i = 0; i < STG_N; i++) tot += tc_bits[i];
    fprintf(stderr, "%s: %.0f bytes of model, %" PRIu64 " MB of tables\n",
            blr_prog, tot / 8, (u64) (tc_tab >> 20));
    if (tc_enc)
      for (i = 0; i < STG_N; i++) {
        fprintf(stderr, "  %-8s %12.0f bytes  %5.2f%%", TC_STAGE[i],
                tc_bits[i] / 8, tot > 0 ? 100.0 * tc_bits[i] / tot : 0.0);
        if (tc_syms[i])
          fprintf(stderr, "  %" PRIu64 " values, %.3f bits each",
                  tc_syms[i], tc_bits[i] / (double) tc_syms[i]);
        fputc('\n', stderr);
      }
  }
  tc_hist_free();
  return BLR_EXIT_OK;
usage:
  fprintf(stderr,
    "usage: tsvcomp c [options] input.tsv output.tc\n"
    "       tsvcomp d [options] input.tc  restored.tsv\n"
    "\n"
    "Lossless compression of a balrogg record stream.  The walk is\n"
    "tsvtrans's; the model is mp3c's, with the contexts declared in the IDX\n"
    "block at the top of tsvcomp.cpp.\n"
    "\n"
    "  -v  say where the bits went, by stage\n"
    "\n"
    "TSVCOMP_IDX names a file to take the context declaration from, in place\n"
    "of the one compiled in.  A stream records which declaration coded it and\n"
    "mode d refuses one it does not have.\n");
  return BLR_EXIT_USAGE;
}
