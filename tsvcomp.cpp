
/*  tsvcomp -- a balrogg record stream, compressed.

      tsvcomp c [options] input.tsv output.tc
      tsvcomp d [options] input.tc  restored.tsv

    tsvtrans rearranges a record stream and leaves the coding to somebody
    else; this one does the coding.  It walks the same structure -- the walk
    is tsvtrans's, and has to be, since what a record means is decided by
    records already read -- and instead of writing a value out as text hands
    it to a model and a range coder.

    The model is mp3c's, described in MP3C-ALGORITHM.md.  Every value is a
    cascade of binary decisions and every decision goes through the same
    parts: four counters on different contexts, an APM correcting the first,
    a logistic mixer weighing them and one thing mp3c had no equivalent of
    (below) under a context of its own, a second APM correcting the mix, and
    a carryless binary range coder.  The encoder and the decoder are the same
    code -- `tc_fam::code` either codes the value it is given or decodes one
    and both then run the identical update -- so a model change cannot
    desynchronise the two halves.

    Rev G's subject is what the *file* knows about itself.  A residue digit
    is one base-nv place of a Vorbis codebook entry, and that entry reached
    the file as a Huffman codeword: a complete Huffman code is a probability
    model, an entry of length L standing for 2^-L.  balrogg has to read those
    lengths to find where the next field starts, and tsvcomp read them and
    threw them away -- which left the one thing about a residue digit that no
    amount of coded history can supply on the floor, and at low bitrates,
    where a context has few visits to learn from, that is most of what there
    is.  tc_prior.inc builds the conditional form of it, per book: the digits
    of a vector are the places of one entry, so once a place is known the
    entry is confined to a subtree, and the cascade asks each prefix node not
    for P(place) but for the weight either side of "is it zero", "is it one",
    "is its bit length k".  It enters as a mixer input, not as a decision, so
    the mixer learns how far to trust it; `pq` (how sure it is that the digit
    is zero) is an axis, and the mixer context is where that axis pays most.
    The classword gets the same from the classbook, and a floor post gets it
    in a second shape: which subclass book coded a post is implied by the
    post's own value, so P(post) is P(subclass) from the classword times
    P(value) in that book.  On the residue this is worth 9%; on the floor
    almost nothing, because an envelope moves slowly and the model was
    already coding 03's floor at 2.79 bits against the 3.19 the codebooks
    alone give.

    Two things were coded that are not information.  The header packets' tag
    sequence is a function of values already coded -- both halves run
    vd_setup's readers, which ask for cb.dim, then cb.entries, then whatever
    cb.ordered turned out to be -- and the reader now drives the coding
    directly through a pair of tsv hooks, so a tag is never written down.
    That was 37% of the header stage.  And a floor post at index >= 2 is
    floor1's folded residual against the line between its two bracketing
    neighbours, both of which precede it in list order and neither of which
    was reachable in the ascending-X order balrogg writes; the posts are
    turned around and coded in list order, which puts the prediction, the
    room either side of it and last packet's reconstructed curve in reach.
    `ep` -- what floor1 would have coded for last packet's curve against this
    packet's prediction -- is the largest floor context since the column.

    Rev E1 (the 08.tsv work, -4.4% on that file) took three things from
    mp3c's ModelMDCT() that the first version had left out, and one Vorbis
    fact it had missed:

      * the position of a digit inside its codebook vector (`vpos`) -- for a
        type-2 residue that is the channel, and mag and angle are not the
        same alphabet; on its own this was the largest single gain (-1.8%);
      * the earlier pass's digit at the same slot (`p0s`, signed) -- the
        refinement pass leans against the coarse pass's rounding, which is
        mp3c's cross-granule history applied across passes rather than
        across time; it goes into the coarse counter, the APM and the sign;
      * the sign as a model of its own (`fam_sgn`, tc_make_sgn) rather than
        one counter: it is 28% of the digit bits and the little that predicts
        it -- the previous pass's sign, the coupled channel's sign, the
        magnitude just coded -- is worth mixing for (-1.4%);
      * mp3c's expected magnitude: a running mean per slot and a weighted
        sum of the temporal and spectral neighbours (`ax`, the dig_av*
        Numbers), log-quantised into one context axis for counter D.

    The temporal neighbours themselves (t1, t2, n1, w1) carry little here:
    at 350 kbit/s the residue is nearly white, and a KT estimate over the
    digit stream gives 2.347 bits given (cls, pass, vpos, q1) against the
    2.242 this now codes at.  Adding them to the rich counter A measured
    worse (dilution); they live in counter C where they cost nothing.

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

    The contexts themselves are declared, not written.  IDX/tsvcomp.idx names
    every variable, its thresholds and every tunable rate; IDX/idx2inc.pl
    turns that plus the template IDX/tsvcomp.inc into MOD/tsvcomp_h.inc and
    MOD/tsvcomp_p.inc, and ./mk.sh drives it.  Nothing in this file decides a
    context, and MOD/ is generated -- edit the .idx.

    The point of that arrangement is IDX/opt.pl, which does not parse or
    rebuild anything: the tuning build embeds each threshold pattern in the
    executable as a "!MAP!name!base\0pattern" string, and the optimizer flips
    those bits *in the binary*, re-runs the corpus and keeps what shrinks it.
    One build serves a whole hill-climb.  ./mk.sh release folds the same
    patterns to literals for shipping, and both builds must code identically
    -- ./mk.sh check is that test.

    Values the stream implies are not coded as they stand: the granule, the
    page sequence and the page serial go out as residuals against what the
    walk already knows, so the usual case is a run of zeros.  Nothing is
    dropped outright and no input is refused for having an unusual page
    header -- a residual that is not zero simply costs a few bits.

    The three header packets of a link -- identification, comment, setup --
    are coded as records rather than as text or as bytes.  Their tag sequence
    is not information: both sides run the same reader, which asks for the
    tags it needs in the order it needs them, so only the values are coded.
    See tc_header.

    A note on what the .idx is for, since it decides more than it looks like
    it does.  The shipped parameters had been hill-climbed on the mono half
    of the corpus, and the same model with the same code was 10% worse than
    balrogg on a 96 kbit/s stereo file and 3% better on a 350 kbit/s mono
    one -- because a variable that carries nothing on dense residue (zrun,
    say) carries 3% on sparse residue, and a pattern of all zeroes is how a
    variable that was tried and did not pay stays out of the way.  Tuning on
    one regime is tuning against the other.  IDX/opt.pl's header says
    "optimize on a corpus, not a file"; a corpus has to span the regimes as
    well as the files.  */

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
#include "sh_mapping.inc"  /*  IDX runtime: mapping, masking, pdesc  */

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

static INLINE i32 tc_abs(i32 v) { return v < 0 ? -v : v; }
static INLINE int tc_clamp(int v, int lo, int hi) {
  return v < lo ? lo : v > hi ? hi : v;
}

static sz tc_mem;                         /*  the per-link histories  */
static void * tc_alloc(sz n) {
  void * p = calloc(n ? n : 1, 1);
  if (!p) FATAL_CODE(BLR_EXIT_IO, "out of memory for %" PRIu64 " bytes", (u64) n);
  tc_mem += n;
  return p;
}
/*  Every model table's unlearned state is now all-bits-zero, so the shipping
    build -- whose tables are one static object -- needs no filling at all and
    faults in only the pages a stream reaches.  The tuning build allocates
    with new[], which does not zero, so there it has to be said out loud.  */
#if USE_NEW
  #define TC_WIPE(p, n) memset((p), 0, (n))
#else
  #define TC_WIPE(p, n) ((void) 0)
#endif

#include "tc_prior.inc"    /*  the codebook's own model of its entries  */

/*  One prior table per codebook (the digits of a residue vector) and one per
    residue (the classes of a classword).  A link's setup replaces both.  */
static tc_ptab tcp_book[VD_MAXBOOK];
static tc_ptab tcp_cls[VD_MAXRES];
static tc_ptab tcp_flr[VD_MAXFLOOR][VD_MAXCLASS];

static void tcp_free(void) {
  u32 i, j;
  for (i = 0; i < VD_MAXBOOK; i++) tcp_book[i].drop();
  for (i = 0; i < VD_MAXRES; i++) tcp_cls[i].drop();
  for (i = 0; i < VD_MAXFLOOR; i++)
    for (j = 0; j < VD_MAXCLASS; j++) tcp_flr[i][j].drop();
  tcp_mem = 0;
}

static void tcp_build(void) {
  i32 v[TCP_SYMMAX];
  u32 i, j;
  tcp_free();
  for (i = 0; i < su.nbk; i++) {
    vd_book * b = su.bk + i;
    if (b->lookup != 1 || !b->clen || !b->mval || !b->nv) continue;
    if (b->nv > TCP_SYMMAX) continue;
    for (j = 0; j < b->nv; j++) v[j] = (i32) b->mval[j] - (i32) b->off;
    tcp_book[i].build(b->clen, b->ent, b->nv, b->dim, 0, v);
  }
  for (i = 0; i < su.nrs; i++) {
    vd_res * r = su.rs + i;
    vd_book * cb = su.bk + r->cbook;
    if (!cb->clen || r->ncl > TCP_SYMMAX) continue;
    for (j = 0; j < r->ncl; j++) v[j] = (i32) j;
    tcp_cls[i].build(cb->clen, cb->ent, r->ncl, cb->dim, 1, v);
  }
  /*  A floor partition: which subclass book coded a post is implied by the
      post's own value, so the classword and the subclass books together give
      one distribution over that value.  */
  for (i = 0; i < su.nfl; i++) {
    vd_floor * f = su.fl + i;
    u32 c;
    for (c = 0; c < VD_MAXCLASS; c++) {
      const u8 * bl[8];
      u32 be[8], nb = 1u << f->csub[c], maxy = 1, k;
      const u8 * cl = nullptr;
      u32 cent = 0;
      if (!f->cdim[c] || f->csub[c] > 3) continue;
      for (k = 0; k < nb; k++) {
        i32 sb = f->csb[c][k];
        if (sb >= 0 && (u32) sb < su.nbk && su.bk[sb].clen) {
          bl[k] = su.bk[sb].clen;  be[k] = su.bk[sb].ent;
          if (be[k] > maxy) maxy = be[k];
        } else if (sb >= 0) { bl[k] = nullptr;  be[k] = 0;  maxy = 0;  break; }
        else { bl[k] = nullptr;  be[k] = 1; }
      }
      if (!maxy || maxy > TCP_SYMMAX) continue;
      if (f->csub[c] && f->cbk[c] >= 0 && (u32) f->cbk[c] < su.nbk)
        { cl = su.bk[f->cbk[c]].clen;  cent = su.bk[f->cbk[c]].ent; }
      tcp_flr[i][c].build_floor(cl, cent, f->csub[c], f->cdim[c], bl, be, maxy);
    }
  }
}

/*  ------------------------------------------------------------------
    The generated model.

    IDX-FORMAT.md \u00a712 warns about an include-order cycle and it is real: the
    generated header needs this file's types and helpers *before* it, and the
    model below needs the tables and Volumes it declares *after* it.  So
    everything MOD/tsvcomp_h.inc reads is above this line and everything that
    reads MOD/tsvcomp_h.inc is below it.  Merging the two halves is the
    obvious-looking simplification that cannot work.  */

typedef u16 word;                         /*  idx2inc.pl's wider bucket table  */
#ifndef __min
  #define __min(a, b) ((a) < (b) ? (a) : (b))
  #define __max(a, b) ((a) > (b) ? (a) : (b))
#endif

/*  Every generated `new[]` goes through this.  It bounds the count so the
    compiler can prove the byte product cannot overflow -- without it GCC 13+
    raises -Walloc-size-larger-than on its own overflow guard, once per table
    -- and it is where a pattern widened past what the machine has is caught,
    by name, instead of as a failed allocation.  The shipping build sizes its
    tables as fixed arrays and never calls it, hence `INLINE`: a plain static
    would draw -Wunused-function from exactly one of the two builds.  */
static INLINE u64 tbl_n(u64 n) {
  FATAL_UNLESS(n > 0 && n <= ((u64) 1 << 31),
               "IDX/tsvcomp.idx asks for a table of %" PRIu64 " entries", n);
  return n;
}

/*  The fields the page and packet headers hold, one model context apart.
    Declared here rather than with the rest of the walk because the sign
    table's size is F_N and the generated header needs it.  */
enum {
  F_MORE, F_PGTYPE, F_GRAN, F_SERIAL, F_SEQ, F_NPKT, F_PLEN, F_TAIL,
  F_SPILL, F_MODE, F_WPREV, F_WNEXT, F_USED, F_N
};
constexpr int TC_MAXTAG = 128;            /*  header tags one stream may use  */

/*  Sign counters.  A family that codes signs through `codes` gets a context
    of its own for them.  These are sizes rather than Volumes, which is what
    Table() takes when the shape does not come from an Index.

    fam_dig has none: a residue digit's sign has been fam_sgn's since it
    became a model rather than a counter, and the magnitude goes through
    `code`, which never looks at G.  The table and the context that addressed
    it were still being built.  */
constexpr int TC_SGN_AUX = F_N;
constexpr int TC_SGN_HDR = TC_MAXTAG;
/*  A floor post and a residue class are never negative, so their sign is one
    counter saturating towards "positive" rather than a context: spread over
    the family index and put through the mixer it takes longer to get there,
    which measured +40 bytes on the floor of 00000000 and nothing anywhere
    else.  A field that is genuinely signed pays for a context; one that is
    not pays for a counter.  */
constexpr int TC_SGN_FLR = 1;
constexpr int TC_SGN_CLS = 1;

#include "MOD/tsvcomp_h.inc"

static TC_T tcm;

/*  Everything a residue digit's contexts are built from.  */
struct tc_dv {
  int rno, pass, band, col, vpos, q1, q2, q1s, q2s, t1, t1s, t2, n1, w1,
      p0, p0s, pn, cls, zrun, blk, bkq, ax, mg, pq, sq, ps, chn, crun, cm;
};

/*  One family's five index rows for one value: the two counters, the APM, the
    mixer and the mantissa plane each get their own.  */
struct tcx { int a, b, c, d, s, f, m, t; };

#include "MOD/tsvcomp_p.inc"

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

/*  A family: the tables its five indices address, and the cascade that turns
    a number into bits.  The tables and the index builders are both generated;
    what is here is the wiring and the coding.  */
struct tc_fam {
  cm_cnt * A, * B, * C, * D, * G, * T;
  i16 * S, * S2;
  i32 * W;
  cm_apm ap, ap2;
  cm_mix<7> mx;
  /*  The mantissa's own two-input mix.  It stays out of the main mixer
      because what it weighs is different -- one counter against the
      codebook -- and because a bare counter is what a decision this cheap
      deserves when there is no codebook to weigh it against.  */
  cm_mix<3> mxm;
  tc_pcur pc;                             /*  the prior for the value in hand  */
  int prp;                                /*  P(this bit is zero) from it, or -1  */
  u32 ba, bb, bc, bd, bs, bf, bm, bt;
  int rA, rB, rC, rD, rS, lr, mw, bw, mb;

  void wire(cm_cnt * a, int va, cm_cnt * b, int vb, cm_cnt * c, int vc,
            cm_cnt * d, int vd, cm_cnt * t, int vt,
            i16 * s, u8 * sc, int vs, i16 * s2, u8 * sc2, int vf,
            i32 * w, int vm, cm_cnt * g, int vg, i32 * wm,
            u8 * wc, u8 * wmc,
            int ra, int rb, int rc_, int rd, int rs, int lrate,
            int mwt, int bwt, int mbt, int qqs, int qqf) {
    A = a;  B = b;  C = c;  D = d;  T = t;  S = s;  S2 = s2;  W = w;  G = g;
    /*  The shipping build's tables come zeroed from the loader, so TC_WIPE
        expands to nothing there and the sizes go unread.  */
    (void) va;  (void) vb;  (void) vc;  (void) vd;  (void) vt;  (void) vg;
    (void) sc;  (void) sc2;  (void) wc;  (void) wmc;
    TC_WIPE(A, (sz) va * TC_NODE * sizeof(cm_cnt));
    TC_WIPE(B, (sz) vb * TC_NODE * sizeof(cm_cnt));
    TC_WIPE(C, (sz) vc * TC_NODE * sizeof(cm_cnt));
    TC_WIPE(D, (sz) vd * TC_NODE * sizeof(cm_cnt));
    TC_WIPE(T, (sz) vt * TC_MNODE * sizeof(cm_cnt));
    if (G) TC_WIPE(G, (sz) vg * sizeof(cm_cnt));
    TC_WIPE(s, (sz) vs * TC_NODE * qqs * sizeof(i16));
    TC_WIPE(sc, (sz) vs * TC_NODE * qqs);
    TC_WIPE(s2, (sz) vf * TC_NODE * qqf * sizeof(i16));
    TC_WIPE(sc2, (sz) vf * TC_NODE * qqf);
    TC_WIPE(w, (sz) vm * TC_NODE * 7 * sizeof(i32));
    TC_WIPE(wc, (sz) vm * TC_NODE);
    TC_WIPE(wm, (sz) vt * TC_MNODE * 3 * sizeof(i32));
    TC_WIPE(wmc, (sz) vt * TC_MNODE);
    /*  Each APM sizes its own curve.  The two do different jobs on different
        contexts -- one corrects a single counter and goes to the mixer, the
        other corrects what the mixer made of all of them -- so how many
        points that correction is worth is two questions, not one.  */
    ap.init(S, sc, (u32) vs * TC_NODE, qqs);
    ap2.init(S2, sc2, (u32) vf * TC_NODE, qqf);
    mx.init(W, wc, (u32) vm * TC_NODE);
    mxm.init(wm, wmc, (u32) vt * TC_MNODE);
    /*  A pattern is a search space and the optimizer visits its ends, so
        anything used as a size, a shift or a limit is clamped at the point of
        use -- IDX/IDX-FORMAT.md §5 says the same, and means it.  */
    rA = tc_clamp(ra, 1, CM_TMAX);
    rB = tc_clamp(rb, 1, CM_TMAX);
    rC = tc_clamp(rc_, 1, CM_TMAX);
    rD = tc_clamp(rd, 1, CM_TMAX);
    rS = tc_clamp(rs, 1, 15);
    lr = tc_clamp(lrate, 1, 64);
    mw = tc_clamp(mwt * 64, 0, CM_CONE / 4);
    bw = tc_clamp(bwt, 0, 16);
    mb = tc_clamp(mbt, 0, 255);
    pc.off();  prp = -1;
  }

  INLINE void select(const tcx & x) {
    ba = (u32) x.a * TC_NODE;
    bb = (u32) x.b * TC_NODE;
    bc = (u32) x.c * TC_NODE;
    bd = (u32) x.d * TC_NODE;
    bs = (u32) x.s * TC_NODE;
    bf = (u32) x.f * TC_NODE;
    bm = (u32) x.m * TC_NODE;
    bt = (u32) x.t * TC_MNODE;
  }

  /*  mp3c's six lines, widened: refine counter A through the APM, stretch
      it and the three others, mix them under one learned weight set, code,
      then update everything.  */
  INLINE int bit(int node, int b) {
    cm_cnt & a = A[ba + node];
    cm_cnt & c = B[bb + node];
    cm_cnt & e = C[bc + node];
    cm_cnt & f = D[bd + node];
    int p1 = ap.pp(a.P(), bs + (u32) node);
    if (p1 > CM_PONE - 1) p1 = CM_PONE - 1;
    mx.add(cm_stretch(p1));
    mx.add(cm_stretch(c.P()));
    mx.add(cm_stretch(e.P()));
    mx.add(cm_stretch(f.P()));
    mx.add(cm_stretch(a.P()));
    /*  What the codebook says about this decision.  A family with no prior,
        or a decision the prior cannot speak to, contributes a zero, which is
        what a stretched even chance is: the input is there and says nothing,
        and the mixer's weight for it stays where the rest of the corpus put
        it rather than being trained on a different set of inputs.  */
    mx.add(prp >= 0 ? cm_stretch(prp) : 0);
    mx.add(256);
    { int pm = mx.mix(bm + (u32) node);
      int pf = ap2.pp(pm, bf + (u32) node);
      /*  How much of the answer the second APM is allowed to be.  It was a
          fixed three parts mixer to one, which is 12 here.  */
      pf = (bw * pm + (16 - bw) * pf + 8) >> 4;
      b = tc_bit(pf < 1 ? 1 : pf > CM_PONE - 1 ? CM_PONE - 1 : pf, b); }
    mx.upd(b, lr, mb);
    ap.upd(b, rS);
    ap2.upd(b, rS);
    a.upd(b, rA, mw);
    c.upd(b, rB, mw);
    e.upd(b, rC, mw);
    f.upd(b, rD, mw);
    return b;
  }

  /*  A mantissa bit: one counter, no mixing.  What is left after the length
      has been coded is close to uniform, and a stage that cannot pay for
      itself should not be run four million times.  */
  INLINE int bitm(int node, int b) {
    cm_cnt & t = T[bt + node];
    if (prp < 0) {                          /*  nothing to weigh it against  */
      b = tc_bit(t.P(), b);
      t.upd(b, rA, mw);
      return b;
    }
    mxm.add(cm_stretch(t.P()));
    mxm.add(cm_stretch(prp));
    mxm.add(256);
    { int pm = mxm.mix(bt + (u32) node);
      b = tc_bit(pm < 1 ? 1 : pm > CM_PONE - 1 ? CM_PONE - 1 : pm, b); }
    mxm.upd(b, lr, mb);
    t.upd(b, rA, mw);
    return b;
  }

  /*  A non-negative integer: three direct steps, then the bit length of what
      is left in unary, then its mantissa.  Small values -- which is nearly
      all of them -- cost one, two or three decisions, and nothing has to
      know a field's range in advance.  */
  /*  The three head steps and the unary length each have a shape the prior
      can answer for; the mantissa does not, and is the part that carries
      least, so it goes uninformed.  `prp` is set immediately before the bit
      it describes and cleared after, so a path that does not set it cannot
      hand the mixer a stale one.  */
  i64 code(const tcx & v, i64 x) {
    int k, p, nb = 0;
    u64 u;
    select(v);
    if (tc_enc) FATAL_UNLESS(x >= 0, "negative magnitude");
    prp = tcp_head(pc, 0);
    if (bit(0, tc_enc && x == 0)) { prp = -1;  return 0; }
    prp = tcp_head(pc, 1);
    if (bit(1, tc_enc && x == 1)) { prp = -1;  return 1; }
    prp = tcp_head(pc, 2);
    if (bit(2, tc_enc && x == 2)) { prp = -1;  return 2; }
    u = tc_enc ? (u64) (x - 3) + 1 : 1;
    if (tc_enc) { u64 t = u;  while (t) { nb++;  t >>= 1; } }
    for (k = 0; ; k++) {
      prp = tcp_len(pc, k);
      if (bit(tc_node_len(k), tc_enc && nb == k + 1)) { nb = k + 1;  break; }
      FATAL_UNLESS(k + 2 <= TC_NBMAX, "%s: a value of 2^%d or more",
                   tc_enc ? "this stream holds" : "coded stream", TC_NBMAX);
    }
    { u64 pref = 1;                         /*  what both sides have of `u`  */
      for (p = 0; p < nb - 1; p++) {
        int mb;
        prp = tcp_man(pc, nb, p, pref);
        mb = bitm(tc_node_man(nb, p),
                  tc_enc && ((u >> (nb - 2 - p)) & 1) != 0);
        pref = pref * 2 + (u64) mb;
      }
      prp = -1;
      if (!tc_enc) u = pref; }
    return (i64) (u - 1) + 3;
  }

  /*  The same, with a sign under its own small context: the sign of a
      residue digit correlates with the sign the same slot had a packet ago
      and with the digit before it, and with nothing the magnitude cascade is
      looking at.  */
  i64 codes(const tcx & v, i64 x, u32 sc) {
    i64 m = code(v, tc_enc ? (x < 0 ? -x : x) : 0);
    int s;
    if (!m) return 0;
    if (G) {
      cm_cnt & g = G[sc];
      s = tc_bit(g.P(), tc_enc && x < 0);
      g.upd(s, rA, mw);
    } else { prp = tcp_sign(pc, m);  s = bit(TC_SIGN, tc_enc && x < 0);  prp = -1; }
    return s ? -m : m;
  }
};

static tc_fam fam_dig, fam_sgn, fam_flr, fam_cls, fam_aux, fam_hdr;

/*  ------------------------------------------------------------------
    The fields that are not floor, class or digit.  */

static i64 tc_last[F_N], tc_last2[F_N];

/*  `c` is whatever the caller knows that the field's own history does not.  */
static i64 tc_auxc(int fld, i32 c, i64 x, int sgn) {
  tcx v;
  i64 r;
  tc_make_aux(fld, c, tc_qlog(tc_last[fld]), v);
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
static i16 * dg_hist2[VD_MAXRES];         /*  the same, one packet further back  */
/*  Running mean magnitude, 4.12 fixed.  u32 and not u16: the mean settles at
    about 241 times the digit, so a u16 wraps for a sustained magnitude over
    271 and `ax` -- the one axis counter D is built on -- becomes noise for
    that slot.  No book in the corpus here has a range that wide, which is why
    it had never shown; the clamp below bounds the digit at 4095, so u32 is
    what that clamp implies.  */
static u32 * dg_avg[VD_MAXRES];
static i16 * dg_pp[VD_MAXRES];            /*  [channel][slot]: the earlier pass's digit this packet  */
static i16 * dg_ps[VD_MAXRES];            /*  and everything the passes so far put there  */
static u8  * dg_pn[VD_MAXRES];            /*  [channel][slot]: passes that coded this slot so far  */
static u8  * cl_hist[VD_MAXRES];
static sz    dg_span[VD_MAXRES], cl_np[VD_MAXRES];
static i32 * dg_q1, * dg_q2, * dg_zr;     /*  [rno][pass][channel]  */
static i32   cl_last[VD_MAXRES], cl_last2[VD_MAXRES];
static i16 * fl_hist;                     /*  [blk][channel][post]  */
static i16 * fl_yhist;                    /*  the same, rebuilt into a curve  */
static u32   tc_blk;                      /*  the block this packet is  */
static u32   tc_nchan;

static void tc_hist_free(void) {
  u32 i;
  for (i = 0; i < VD_MAXRES; i++) {
    free(dg_hist[i]);  dg_hist[i] = nullptr;
    free(dg_hist2[i]); dg_hist2[i] = nullptr;
    free(dg_avg[i]);   dg_avg[i] = nullptr;
    free(dg_pp[i]);    dg_pp[i] = nullptr;
    free(dg_ps[i]);    dg_ps[i] = nullptr;
    free(dg_pn[i]);    dg_pn[i] = nullptr;
    free(cl_hist[i]);  cl_hist[i] = nullptr;
    dg_span[i] = cl_np[i] = 0;
  }
  free(dg_q1);  free(dg_q2);  free(dg_zr);  free(fl_hist);  free(fl_yhist);
  dg_q1 = dg_q2 = dg_zr = nullptr;  fl_hist = nullptr;  fl_yhist = nullptr;
}

/*  A link's setup has just been read; size the histories it implies.  A
    residue whose span is beyond what is worth holding simply loses its
    temporal context -- the other contexts still apply, so the stream stays
    codable rather than refused.  */
static void tc_setup_done(void) {
  u32 i;
  tc_hist_free();
  tcp_build();
  tc_nchan = su.ch;
  for (i = 0; i < su.nrs; i++) {
    vd_res * r = su.rs + i;
    sz span = r->end > r->beg ? r->end - r->beg : 0;
    sz need;
    if (span > TC_SPANMAX) span = 0;
    need = (sz) 2 * 8 * tc_nchan * span;
    if (need > TC_HISTMAX) span = 0;
    dg_span[i] = span;
    if (span) {
      dg_hist[i]  = (i16 *) tc_alloc((sz) 2 * 8 * tc_nchan * span * sizeof(i16));
      dg_hist2[i] = (i16 *) tc_alloc((sz) 2 * 8 * tc_nchan * span * sizeof(i16));
      dg_avg[i]   = (u32 *) tc_alloc((sz) 2 * 8 * tc_nchan * span * sizeof(u32));
      dg_pp[i]    = (i16 *) tc_alloc((sz) tc_nchan * span * sizeof(i16));
      dg_ps[i]    = (i16 *) tc_alloc((sz) tc_nchan * span * sizeof(i16));
      dg_pn[i]    = (u8 *)  tc_alloc((sz) tc_nchan * span);
    }
    cl_np[i] = span ? span / r->psz + 2 : 0;
    if (cl_np[i]) cl_hist[i] = (u8 *) tc_alloc((sz) 2 * tc_nchan * cl_np[i]);
    cl_last[i] = cl_last2[i] = 0;
  }
  dg_q1 = (i32 *) tc_alloc((sz) su.nrs * 8 * tc_nchan * sizeof(i32));
  dg_q2 = (i32 *) tc_alloc((sz) su.nrs * 8 * tc_nchan * sizeof(i32));
  dg_zr = (i32 *) tc_alloc((sz) su.nrs * 8 * tc_nchan * sizeof(i32));
  fl_hist = (i16 *) tc_alloc((sz) 2 * tc_nchan * VD_MAXPOST * sizeof(i16));
  fl_yhist = (i16 *) tc_alloc((sz) 2 * tc_nchan * VD_MAXPOST * sizeof(i16));
}

/*  libvorbis's render_point and the inverse of floor1's folding.  A post at
    index >= 2 is coded against the line between its two bracketing
    neighbours, and what balrogg writes down is that folded residual -- which
    is why the column is worth -22% and the previous post nothing.  Both
    neighbours precede the post in list order, so coding in list order (which
    is not the order the records are in) puts the prediction, the room either
    side of it and the reconstructed curve within reach.  */
static i32 vd_render(i32 x0, i32 x1, i32 y0, i32 y1, i32 X) {
  i32 dy = y1 - y0, adx = x1 - x0;
  i32 ady = dy < 0 ? -dy : dy;
  i32 off;
  if (adx <= 0) return y0;
  off = (i32) (((i64) ady * (X - x0)) / adx);
  return dy < 0 ? y0 - off : y0 + off;
}
/*  The other direction: what floor1 would have coded for a curve point.  Run
    on last packet's curve against this packet's prediction it says what the
    residual would be if the envelope had not moved -- which is the context
    the previous packet's *residual* was standing in for, and a much better
    one, since the prediction it was measured against has moved since.  */
static i32 vd_fold(i32 y, i32 pred, i32 quant) {
  i32 hiroom = quant - pred, lowroom = pred;
  i32 room = (hiroom < lowroom ? hiroom : lowroom) << 1;
  i32 d = y - pred;
  i32 a = d >= 0 ? 2 * d : -2 * d - 1;
  if (!d) return 0;
  if (a < room) return a;
  return hiroom > lowroom ? d + lowroom : hiroom - 1 - d;
}

/*  The coded value back into a curve point, exactly as floor1_inverse2 does
    it.  A stream may name anything; the result is clamped to the range the
    quantiser admits so a later render cannot run away.  */
static i32 vd_unfold(i32 val, i32 pred, i32 quant) {
  i32 hiroom = quant - pred, lowroom = pred;
  i32 room = (hiroom < lowroom ? hiroom : lowroom) << 1;
  i32 y;
  if (!val) y = pred;
  else if (val >= room)
    y = hiroom > lowroom ? val - lowroom + pred : pred - val + hiroom - 1;
  else if (val & 1) y = pred - ((val + 1) >> 1);
  else y = pred + (val >> 1);
  return y < 0 ? 0 : y > quant - 1 ? quant - 1 : y;
}

/*  ------------------------------------------------------------------
    The header packets.

    Both sides run the same reader over the same records, so what has to
    cross is the values and, once each, the spelling of a tag.  The tag of a
    record is nearly always the tag of the record before it -- balrogg emits
    runs -- so that is one bit, and a first sighting costs its letters.  */

/*  The capture buffer, which grows to whatever a link's header packets turn
    out to need.  It cannot be sized from the packet length: a codebook
    declares its entry count in three bytes and every entry is a record, so
    the records a setup packet yields are a property of the stream and not of
    its length.  */
/*  The tag a record carries is not information.  Both halves run vd_setup's
    readers over the same values, so both ask for the same tags in the same
    order; what has to cross is the values.  A tag still names a context --
    every field is a different kind of number -- so it gets an id, assigned in
    order of first sighting, which both sides arrive at identically because
    they see the same first sightings.  Nothing about it is coded.

    That is what this file's preamble has always claimed and what the program
    did not do: the previous form captured the records, coded a count, and
    then coded each tag as a same-flag, an index and, on a first sighting, its
    letters.  On a short stream that was 37% of the header stage.  */
static char tc_tagbuf[TC_MAXTAG * (TSV_TAGMAX + 1)];
static const char * tc_tag[TC_MAXTAG];
static u32 tc_ntag;
static int tc_lasttag = -1;
static i64 tc_tlast[TC_MAXTAG], tc_tlast2[TC_MAXTAG];
static i64 tc_runpos;

static int tc_tagid(const char * tag) {
  u32 i;
  int id = -1;
  for (i = 0; i < tc_ntag; i++)
    if (!strcmp(tc_tag[i], tag)) { id = (int) i;  break; }
  if (id < 0) {
    char * dst = tc_tagbuf + (sz) tc_ntag * (TSV_TAGMAX + 1);
    /*  hdr_a's tag axis is 64 wide and TC_MAXTAG is what the table holds; a
        reader that grew past either would index off the end of both, so it is
        refused here rather than found later.  */
    FATAL_UNLESS(tc_ntag < TC_MAXTAG, "the reader asks for more than %d tags",
                 TC_MAXTAG);
    snprintf(dst, TSV_TAGMAX + 1, "%s", tag);
    tc_tag[tc_ntag] = dst;
    id = (int) tc_ntag++;
  }
  tc_runpos = (id == tc_lasttag) ? tc_runpos + 1 : 0;
  tc_lasttag = id;
  return id;
}

static i64 tc_hdrval(int id, i64 x) {
  tcx v;
  tc_make_hdr(id, tc_qlog(tc_tlast[id]), tc_qlog(tc_tlast2[id]),
              tc_qlog(tc_runpos), (int) (tc_tlast[id] & 255), v);
  x = fam_hdr.codes(v, x, (u32) id);
  tc_tlast2[id] = tc_tlast[id];  tc_tlast[id] = x;
  return x;
}

/*  Both modes run the reader once.  In mode c it reads the real input and a
    tee hands every value to the model; in mode d it reads nothing -- each
    get() is answered by the model, and the value goes on to the restored
    stream in the order and under the tag it was asked for.  The two are the
    same walk over the same values, so they ask for the same tags.  */
static void tc_hdr_enc(void * ctx, const char * tag, i64 v) {
  (void) ctx;
  tc_hdrval(tc_tagid(tag), v);
}
static i64 tc_hdr_dec(void * ctx, const char * tag) {
  i64 v = tc_hdrval(tc_tagid(tag), 0);
  ((tsv *) ctx)->put(tag, v);
  return v;
}

static void tc_header(int which, sz plen, tsv & out) {
  tsv h;
  memset(&h, 0, sizeof h);
  tc_stage = STG_HDR;
  if (tc_enc) {
    h.create_hook(nullptr, tc_hdr_enc);
    tc_in->tee = &h;
    if (which == 0) su.ident(*tc_in);
    else if (which == 1) su.comment(*tc_in, plen);
    else { su_role = 0;  su_dst = nullptr;  su.setup(*tc_in);  su.have = 1; }
    tc_in->tee = nullptr;
  } else {
    h.open_hook(&out, tc_hdr_dec);
    if (which == 0) su.ident(h);
    else if (which == 1) su.comment(h, plen);
    else { su_role = 0;  su_dst = nullptr;  su.setup(h);  su.have = 1; }
  }
  if (which == 2) tc_setup_done();
}

/*  ------------------------------------------------------------------
    The audio packets.  */

#ifdef TC_DUMP
static i16 * tc_dumprec;  static FILE * tc_dumpf;
#endif
/*  One residue partition.  */
static INLINE i32 tc_sq(i32 v) {          /*  signed log-ish quantisation  */
  i32 q = tc_qlog(v);
  return v < 0 ? -q : q;
}
static void tc_part(u32 rno, u32 pss, u32 j, u32 pc, u32 psz, u32 cls,
                    i32 bkq, u32 dim, u32 bn, i32 clrun, int clmatch) {
  sz span = dg_span[rno];
  sz hb = ((sz) (tc_blk * 8 + pss) * tc_nchan + j) * span;
  i16 * hist  = span ? dg_hist[rno]  + hb : nullptr;
  i16 * hist2 = span ? dg_hist2[rno] + hb : nullptr;
  u32 * avg   = span ? dg_avg[rno]   + hb : nullptr;
  i16 * pp    = span ? dg_pp[rno] + (sz) j * span : nullptr;
  i16 * ps    = span ? dg_ps[rno] + (sz) j * span : nullptr;
  u8  * pn    = span ? dg_pn[rno] + (sz) j * span : nullptr;
  sz base = (sz) ((rno * 8 + pss) * tc_nchan + j);
  i32 * q1 = dg_q1 + base, * q2 = dg_q2 + base, * zr = dg_zr + base;
  vd_book * bk = su.bk + bn;
  const tc_ptab * pt = tcp_book[bn].ok ? &tcp_book[bn] : nullptr;
  u32 i;
  tc_stage = STG_DIGIT;
  fam_dig.pc.off();
  for (i = 0; i < psz; i++) {
    sz slot = (sz) pc * psz + i;
    int ok = hist && slot < span;
    tcx v;
    tc_dv d;
    i32 t1 = ok ? hist[slot] : 0;
    i32 t2 = ok ? hist2[slot] : 0;
    i32 n1 = ok && slot + dim < span ? hist[slot + dim] : 0;
    i32 w1 = ok && slot >= dim ? hist[slot - dim] : 0;
    i32 av = ok ? avg[slot] : 0;
    i32 p0 = ok ? pp[slot] : 0;
    i32 psum = ok ? ps[slot] : 0;
    i32 pnn = ok ? pn[slot] : 0;
    i64 dg;
    d.rno = (int) rno;  d.pass = (int) pss;  d.band = tc_qlog(pc);
    d.col = tc_qlog(i);  d.vpos = (int) (dim ? i % dim : 0);
    d.q1 = tc_qlog(*q1);  d.q2 = tc_qlog(*q2);
    d.q1s = tc_sq(*q1);  d.q2s = tc_sq(*q2);
    d.t1 = tc_qlog(t1);  d.t1s = tc_sq(t1);
    d.t2 = tc_qlog(t2);  d.n1 = tc_qlog(n1);  d.w1 = tc_qlog(w1);
    d.p0 = tc_qlog(p0);  d.p0s = tc_sq(p0);  d.pn = pnn;
    /*  What the passes before this one have already put at this slot,
        signed.  Once three passes have hit a slot the last one alone is a
        poor summary of where the value stands; the refinement leans against
        the running total, not against the last correction.  */
    d.ps = tc_sq(psum);
    /*  For a type-2 residue the vector interleaves the channels, so a slot's
        channel is its position modulo their number -- which is what vpos
        names only when the book's dimension happens to be even.  */
    d.chn = (int) (tc_nchan ? (((sz) pc * psz + i) % tc_nchan) : 0);
    d.crun = tc_qlog(clrun);  d.cm = clmatch;
    d.cls = (int) cls;  d.zrun = tc_qlog(*zr);  d.blk = (int) tc_blk;
    d.bkq = bkq;
    /*  A vector's digits are the places of one codebook entry, so the prior
        restarts with the vector and walks down one level per digit.  */
    if (dim && d.vpos == 0) fam_dig.pc.start(pt);
    d.pq = tcp_axis(fam_dig.pc);
    d.sq = 0;
    { /*  mp3c's exA: a weighted mean of the neighbourhood, log-quantised  */
      i32 ex = (av >> 4) * TC_dig_avA + tc_abs(t1) * TC_dig_avT1
             + tc_abs(t2) * TC_dig_avT2 + tc_abs(n1) * TC_dig_avN1
             + tc_abs(w1) * TC_dig_avW1 + tc_abs(*q1) * TC_dig_avQ1
             + tc_abs(*q2) * TC_dig_avQ2;
      d.ax = tc_qlog((ex * TC_dig_avC) >> 12);
    }
    tc_make_dig(d, v);
#ifdef TC_DUMP
    { static FILE * df;  static i16 rec[12];
      if (!df) df = fopen("dump.bin", "wb");
      rec[0] = (i16) cls;  rec[1] = (i16) pss;  rec[2] = (i16) d.vpos;
      rec[3] = (i16) *q1;  rec[4] = (i16) *q2;  rec[5] = (i16) t1;
      rec[6] = (i16) n1;   rec[7] = (i16) w1;   rec[8] = (i16) p0;
      rec[9] = (i16) t2;   rec[10] = (i16) pc;
      tc_dumprec = rec;  tc_dumpf = df; }
#endif
    dg = tc_enc ? tc_in->get("res.digit") : 0;
    { i64 mg = fam_dig.code(v, tc_enc ? (dg < 0 ? -dg : dg) : 0);
      if (mg) {
        tcx w;
        int sn;
        int spr = tcp_sign(fam_dig.pc, mg);
        d.mg = tc_qlog(mg);
        /*  The codebook says which of +m and -m its entries hold at this
            place, which for a coupled angle channel is most of what there is
            to know about a sign.  It goes to the sign model twice: as the
            mixer input it is, and as an axis, so the weight it is given can
            depend on how sure it is.  */
        d.sq = tcp_bucket(spr);
        tc_make_sgn(d, w);
        fam_sgn.select(w);
        /*  The sign is a different family but the same place in the same
            vector, so it reads the digit model's cursor.  */
        fam_sgn.prp = spr;
        sn = fam_sgn.bit(0, tc_enc && dg < 0);
        fam_sgn.prp = -1;
        dg = sn ? -mg : mg;
      } else dg = 0; }
    /*  Step past this place.  A digit the grid does not name puts the cursor
        out for the rest of the vector rather than mispositioning it.  */
    if (fam_dig.pc.on) {
      i64 gv = dg + (i64) bk->off;
      fam_dig.pc.step(bk->minv && gv >= 0 && gv < (i64) bk->base
                      ? bk->minv[gv] : -1);
    }
#ifdef TC_DUMP
    tc_dumprec[11] = (i16) dg;  fwrite(tc_dumprec, 2, 12, tc_dumpf);
#endif
    if (!tc_enc) tc_out->put("res.digit", dg);
    if (ok) {
      i16 c = (i16) (dg < -32768 ? -32768 : dg > 32767 ? 32767 : dg);
      i32 a = tc_abs(c);
      hist2[slot] = hist[slot];
      hist[slot] = c;
      avg[slot] = (((u32) avg[slot] * (u32) TC_dig_avD) >> 8)
                + (u32) (a > 4095 ? 4095 : a) * 16;
      pp[slot] = c;  pn[slot] = (u8) (pnn < 7 ? pnn + 1 : 7);
      { i32 t = psum + c;
        ps[slot] = (i16) (t < -32768 ? -32768 : t > 32767 ? 32767 : t); }
    }
    *q2 = *q1;  *q1 = (i32) dg;
    *zr = dg ? 0 : *zr + 1;
    if (tc_verbose) tc_syms[STG_DIGIT]++;
  }
}

static i32 cl_run[VD_MAXRES];             /*  how long the class has held  */
static u8 cl_same[1u << 20];              /*  and whether it held from last packet  */

static u32 tc_classify(u32 rno, u32 j, u32 slot) {
  tcx v;
  u8 * hist = cl_np[rno] ? cl_hist[rno] + ((sz) tc_blk * tc_nchan + j) * cl_np[rno]
                         : nullptr;
  i32 t1 = (hist && slot < cl_np[rno]) ? hist[slot] : 0;
  i32 tn = (hist && slot + 1 < cl_np[rno]) ? hist[slot + 1] : 0;
  i64 c;
  tc_stage = STG_CLASS;
  tc_make_cls((int) rno, tc_qlog(slot), cl_last[rno], t1, (int) tc_blk, tn,
              cl_last2[rno], tcp_axis(fam_cls.pc), v);
  c = tc_enc ? (i64) tc_in->get_u("res.class", 16) : 0;
  c = fam_cls.code(v, c);
  if (!tc_enc) tc_out->put("res.class", c);
  FATAL_UNLESS(c >= 0 && c < 16, "coded stream: residue class %" PRId64, c);
  cl_run[rno] = (c == cl_last[rno]) ? cl_run[rno] + 1 : 0;
  if (hist && slot < cl_np[rno]) hist[slot] = (u8) c;
  cl_last2[rno] = cl_last[rno];  cl_last[rno] = (i32) c;
  if (tc_verbose) tc_syms[STG_CLASS]++;
  return (u32) c;
}

/*  The residue walk of io.inc, over records rather than bits.  */
static u8 tc_cl[1u << 20];
static u8 cl_run_buf[1u << 20];           /*  the run and the match, per slot  */
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
  if (dg_span[rno]) {
    memset(dg_pp[rno], 0, (sz) tc_nchan * dg_span[rno] * sizeof(i16));
    memset(dg_ps[rno], 0, (sz) tc_nchan * dg_span[rno] * sizeof(i16));
    memset(dg_pn[rno], 0, (sz) tc_nchan * dg_span[rno]);
  }
  for (pss = 0; pss < 8; pss++) {
    pc = 0;
    while (pc < np) {
      if (!pss)
        for (j = 0; j < vch; j++) {
          /*  The classes of `pv` consecutive partitions are the places of one
              classword, so they get the classbook's prior the same way a
              vector's digits get the residue book's.  */
          fam_cls.pc.start(tcp_cls[rno].ok ? &tcp_cls[rno] : nullptr);
          for (k = 0; k < pv; k++) {
            u8 * ch = cl_np[rno] ? cl_hist[rno] + ((sz) tc_blk * tc_nchan + j) * cl_np[rno]
                                 : nullptr;
            i32 was = (ch && pc + k < cl_np[rno]) ? ch[pc + k] : -1;
            i32 run = cl_run[rno];
            u32 c = tc_classify(rno, j, pc + k);
            tc_cl[j * w + pc + k] = (u8) c;
            if (j * w + pc + k < sizeof cl_run_buf) {
              cl_run_buf[j * w + pc + k] = (u8) (run > 255 ? 255 : run);
              cl_same[j * w + pc + k] = (u8) (was == (i32) c);
            }
            fam_cls.pc.step((int) c);
          }
        }
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
            tc_part(rno, pss, j, pc, r->psz, c, tc_qlog(su.bk[bn].off),
                    su.bk[bn].dim, (u32) bn,
                    j * w + pc < sizeof cl_run_buf ? cl_run_buf[j * w + pc] : 0,
                    j * w + pc < sizeof cl_same ? cl_same[j * w + pc] : 0);
        }
        pc++;
      }
    }
  }
}

/*  Floor curves, then the residues, for one audio packet.  */
static u8 tc_used[VD_MAXCH];
static i16 fl_cur[VD_MAXPOST];            /*  this channel's coded residuals  */
static i16 fl_fy[VD_MAXPOST];             /*  and the curve they rebuild  */

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
    i16 * hy = fl_yhist + ((sz) tc_blk * tc_nchan + k) * VD_MAXPOST;
    u32 u, p;
    i32 quant = (i32) f->quant;
    tc_stage = STG_AUD;
    u = (u32) tc_auxc(F_USED, tc_used[k], tc_enc ? tc_in->get_u("flr.used", 2) : 0, 0);
    FATAL_UNLESS(u < 2, "coded stream: flr.used is %u", u);
    if (!tc_enc) tc_out->put("flr.used", u);
    tc_used[k] = nz[k] = (u8) u;
    if (!u) continue;
    tc_stage = STG_FLOOR;
    fam_flr.pc.off();
    /*  The records are in ascending X, which is the order the column context
        is about; the coding is in list order, which is the order the
        prediction is available in.  Both sides do the same swap, so what
        crosses is the same values in a different order.  */
    if (tc_enc)
      for (i = 0; i < f->posts; i++)
        fl_cur[f->srt[i]] = (i16) tc_in->get("flr.y");
    for (p = 0; p < f->posts; p++) {
      tcx v;
      i64 y;
      i32 pred = 0, room = 0, hl = 0, lov = 0, hiv = 0, ep = 0;
      i32 o1 = k ? hp[p - (sz) VD_MAXPOST] : 0;    /*  the channel before  */
      i32 od = k ? hy[p - (sz) VD_MAXPOST] - hy[p] : 0;
      if (p >= 2) {
        u32 l = f->lo[p - 2], h = f->hi[p - 2];
        i32 hiroom, lowroom;
        u32 c = f->pcl[p];
        if (!f->ppos[p])
          fam_flr.pc.start(tcp_flr[fno][c].ok ? &tcp_flr[fno][c] : nullptr);
        pred = vd_render((i32) f->x[l], (i32) f->x[h], fl_fy[l], fl_fy[h],
                         (i32) f->x[p]);
        if (pred < 0) pred = 0;
        if (pred > quant - 1) pred = quant - 1;
        hiroom = quant - pred;  lowroom = pred;
        room = (hiroom < lowroom ? hiroom : lowroom) << 1;
        hl = hiroom > lowroom ? 1 : hiroom < lowroom ? 2 : 0;
        lov = fl_cur[l];  hiv = fl_cur[h];
        ep = vd_fold(hy[p], pred, quant);
        if (ep < 0) ep = 0;
      }
      tc_make_flr((int) fno, (int) (f->rnk[p] < VD_MAXPOST ? f->rnk[p] : VD_MAXPOST - 1),
                  tc_qlog(hp[p]), tc_qlog(hy[p]), p < 2, (int) tc_blk,
                  tc_qlog(o1), tc_qlog(pred), tc_sq(od), (int) k,
                  tc_qlog(room), hl, tc_qlog(lov), tc_qlog(hiv),
                  tcp_axis(fam_flr.pc), tc_qlog(ep), v);
      y = tc_enc ? fl_cur[p] : 0;
      y = fam_flr.codes(v, y, 0);
      FATAL_UNLESS(y >= 0 && y < 32768, "coded stream: floor post %" PRId64, y);
      fl_cur[p] = (i16) y;
      fl_fy[p] = (i16) (p < 2 ? (y > quant - 1 ? quant - 1 : (i32) y)
                              : vd_unfold((i32) y, pred, quant));
      if (fam_flr.pc.on)
        fam_flr.pc.step(y >= 0 && y < (i64) fam_flr.pc.t->nsym ? (int) y : -1);
      if (tc_verbose) tc_syms[STG_FLOOR]++;
    }
    if (!tc_enc)
      for (i = 0; i < f->posts; i++)
        tc_out->put("flr.y", fl_cur[f->srt[i]]);
    for (p = 0; p < f->posts; p++) { hp[p] = fl_cur[p];  hy[p] = fl_fy[p]; }
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
  /*  A packet's length against the one before it.  The two axes F_PLEN had
      were both the previous length, one of them log-quantised twice; what it
      wanted was the difference, which within a page is small.  */
  { i64 prev = 0;
    for (i = 0; i < p.np; i++) {
      i64 v = tc_auxc(F_PLEN, tc_qlog(prev),
                      tc_enc ? (i64) p.plen[i] - prev : 0, 1) + prev;
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
    u8 h[2];
    in.open(inpath);
    tc_in = &in;
    bf = fopen(outpath, "wb");
    if (!bf) FATAL_CODE(BLR_EXIT_IO, "cannot create %s", outpath);
    h[0] = (u8) TC_VER;  h[1] = 0;
    if (fwrite(TC_MAGIC, 1, sizeof TC_MAGIC - 1, bf) != sizeof TC_MAGIC - 1 ||
        fwrite(h, 1, 2, bf) != 2)
      FATAL_CODE(BLR_EXIT_IO, "write error on %s", outpath);
    rcb.attach(bf, 1 << 16);
    rc.StartEncode(&rcb);
  } else {
    char m[sizeof TC_MAGIC - 1];
    u8 h[2];
    bf = fopen(inpath, "rb");
    if (!bf) FATAL_CODE(BLR_EXIT_IO, "cannot open %s", inpath);
    FATAL_UNLESS(fread(m, 1, sizeof m, bf) == sizeof m &&
                 !memcmp(m, TC_MAGIC, sizeof m),
                 "%s: not a tsvcomp stream -- `tsvcomp c` makes one", inpath);
    FATAL_UNLESS(fread(h, 1, 2, bf) == 2,
                 "%s: the stream ends in its header", inpath);
    FATAL_UNLESS(h[0] == TC_VER,
                 "%s: made by tsvcomp version %u, this is version %u",
                 inpath, h[0], TC_VER);
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

/*  Hand a family the tables the generator sized for it and the rates the
    .idx set.  Every name here is generated: `TC_dig_A` is the Table() line in
    IDX/tsvcomp.inc, `TC_dig_a_Volume` the product of the factor sizes in
    IDX/tsvcomp.idx, `TC_dig_rA` its Number.  */
#define TC_WIRE(f, F, g, ng)                                                  \
  (f).wire(tcm.TC_##F##_A, TC_##F##_a_Volume,                                 \
           tcm.TC_##F##_B, TC_##F##_b_Volume,                                 \
           tcm.TC_##F##_C, TC_##F##_c_Volume,                                 \
           tcm.TC_##F##_D, TC_##F##_d_Volume,                                 \
           tcm.TC_##F##_T, TC_##F##_t_Volume,                                 \
           tcm.TC_##F##_S, tcm.TC_##F##_SC, TC_##F##_s_Volume,                \
           tcm.TC_##F##_F, tcm.TC_##F##_FC, TC_##F##_f_Volume,                \
           tcm.TC_##F##_W, TC_##F##_m_Volume, (g), (ng),                      \
           tcm.TC_##F##_WM, tcm.TC_##F##_WC, tcm.TC_##F##_WMC,               \
           TC_##F##_rA, TC_##F##_rB, TC_##F##_rC, TC_##F##_rD,                \
           TC_##F##_rS, TC_##F##_lr, TC_##F##_mw, TC_##F##_bw,                \
           TC_##F##_mb, TC_##F##_qs, TC_##F##_qf)

static void tc_models(void) {
  cm_tables();
  tcm.TC_Init();
  TC_WIRE(fam_dig, dig, (cm_cnt *) nullptr, 0);
  TC_WIRE(fam_sgn, sgn, (cm_cnt *) nullptr, 0);
  TC_WIRE(fam_flr, flr, tcm.TC_flr_G, TC_SGN_FLR);
  TC_WIRE(fam_cls, cls, tcm.TC_cls_G, TC_SGN_CLS);
  TC_WIRE(fam_aux, aux, tcm.TC_aux_G, TC_SGN_AUX);
  TC_WIRE(fam_hdr, hdr, tcm.TC_hdr_G, TC_SGN_HDR);
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
            blr_prog, tot / 8, (u64) (tcm.TC_Size >> 20));
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
  tcp_free();
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
    "The contexts and the rates are declared in IDX/tsvcomp.idx and compiled\n"
    "in through MOD/; ./mk.sh regenerates them and IDX/opt.pl tunes them.\n"
    "They are part of the format -- a stream is decodable by a build with the\n"
    "same MOD/, and by no other.\n");
  return BLR_EXIT_USAGE;
}
