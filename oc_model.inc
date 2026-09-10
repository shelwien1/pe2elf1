/*  oc_model -- the tsvcomp model as a sink for balrogg's walk.

    This is tsvcomp.cpp with its walk removed.  Everything below the design
    notes is the model that file described -- the counters, the two APMs, the
    mixer, the codebook prior, the histories and the six families -- and
    what used to be tc_walk, tc_page, tc_header, tc_audio, tc_payload,
    tc_residue and tc_part is now the set of sink methods at the end of the
    file, called by io.inc at the site where each value is read from the
    packet, with the context that site has (OGGCOMP-PLAN.md sec.2).  The
    setup it reads is balrogg's vb_setup, through `oc_v`, rather than a
    second parse of the same records.

    The bodies are the ones tsvcomp had: every decision is made in the same
    order with the same context, so a stream coded here is byte for byte
    what tsvcomp coded from balrogg's record stream (sec.6, step 4).

    What follows, to the include line, is the design of the model as
    tsvcomp.cpp's preamble gave it.

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

    The contexts themselves are declared, not written.  IDX/tsvcomp-<family>.idx names
    every variable, its thresholds and every tunable rate; IDX/idx2inc.pl
    turns that plus the templates IDX/tsvcomp-*.inc into MOD/tsvcomp-*_h.inc and
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


/*  Set by the main program: 1 = c, 0 = d.  */
static int tc_enc;
/*  The coder, one per direction: rc.inc has the direction as a template
    parameter, so the choice is one branch at tc_bit rather than tests
    inside every step.  Only the one for the direction in hand is started.
    OC_CARRYLESS=1 builds the carryless twin, which loses a carry once in
    2^33 steps and cannot recover here (there is no block to code again),
    so the encoder refuses the stream when that happens; it is for
    measuring, not shipping.  */
#ifndef OC_CARRYLESS
#define OC_CARRYLESS 0
#endif
static Rangecoder<0, OC_CARRYLESS> rce;
static Rangecoder<1, OC_CARRYLESS> rcd;
/*  The link's setup, once its header has been read (setup_done).  */
static vb_ctx * oc_v;
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
/*  How many of these a family's tables actually hold is the family's own
    number -- see TC_dig_ND and its five siblings below.  These two are the
    most any family needs: the ladder and the unary length between them reach
    node 28, and the mantissa plane 168.

    There is no sign node.  Every family that codes a sign has a counter for
    it -- `G`, on a context of its own -- so the 30th row these tables used to
    carry was addressed by nothing, in every family, at 1/30th of the whole
    model's address space.  `codes` refuses rather than reaching for it.  */
/*  How many inputs the main mixer weighs -- see tc_fam::bit for what they
    are.  It is a constant and not a parameter because the tables are sized by
    it and the .idx may not resize them.  */
constexpr int TC_MIXN  = 7;
constexpr int TC_NODE  = 29;              /*  0-2 ladder, 3-28 length  */
constexpr int TC_MNODE = 169;             /*  13 lengths x 13 positions  */
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
/*  It was a loop, one shift per octave, and its exit was a branch on the
    data twenty times a digit; the bit length says how many shifts there
    would have been.  */
static INLINE i32 tc_qlog(i64 x) {
  u64 u = (u64) (x < 0 ? -x : x);
  int n = 62 - __builtin_clzll(u | 1);     /*  shifts to bring u under 4  */
  i32 q = 2 + n * 2 + (i32) ((u >> n) & 1);
  return u < 4 ? (i32) u : q;
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
/*  Model tables are mapped, not allocated, and this is the whole reason the
    contexts can be as wide as IDX/opt.pl wants them.

    Every table's unlearned state is all-bits-zero, and a fresh mapping is
    zero, so a table costs address space at once and memory only where a
    stream actually reaches it.  The tuned indices are enormously sparse:
    a set asking for 38.9 GB of tables codes 00000008.tsv in 304 MB resident
    and 00000007.tsv in 79 MB, because a few hundred thousand distinct
    contexts is what a link really visits.  What has to be avoided is anything
    that touches a page the coder never will -- a memset over the whole table,
    which is what `new[]` plus a wipe amounted to, or a commit charge against
    the whole reservation, which is what a multi-gigabyte .bss amounted to on
    Linux: the kernel's default overcommit heuristic refuses the mapping and
    the program dies before main().  MAP_NORESERVE says exactly what is meant.

    Windows has no equivalent -- a commit is charged against RAM plus the
    pagefile whatever one does -- so there a wide model is bounded by the
    pagefile, and VirtualAlloc failing is reported like any other refusal
    rather than left to become a fault.  */
/*  -H: ask for the tables on huge pages.  A 4 kB page walk is a TLB miss
    on most rows a large file reaches (OGGCOMP-SPEED.md section 5.1), and
    2 MB pages take 5% off its user time -- when the kernel has them to
    give.  When it has to compact memory to make them, the first process
    to ask pays seconds of system time, which is why this is an option and
    not the default.  Linux only; elsewhere it is accepted and does nothing.  */
static int tc_huge;

static void * tc_map(sz n) {
  void * p;
#ifdef _WIN32
  p = VirtualAlloc(nullptr, n, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
  p = mmap(nullptr, n, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (p == MAP_FAILED) p = nullptr;
#ifdef MADV_HUGEPAGE
  if (p && tc_huge) madvise(p, n, MADV_HUGEPAGE);
#endif
#endif
  if (!p) FATAL_CODE(BLR_EXIT_IO,
                     "cannot map %" PRIu64 " MB for a model table -- "
                     "IDX/tsvcomp-*.idx asks for more than this machine will map",
                     (u64) (n >> 20));
  return p;
}

/*  INLINE for the same reason tbl_n has it: the shipping build's tables are
    members of one object, so nothing there unmaps a table and exactly one of
    the two builds would draw -Wunused-function.  */
static INLINE void tc_unmap(void * p, sz n) {
  if (!p) return;
#ifdef _WIN32
  (void) n;  VirtualFree(p, 0, MEM_RELEASE);
#else
  munmap(p, n);
#endif
}

#include "tc_prior.inc"    /*  the codebook's own model of its entries  */
/*  One prior table per codebook (the digits of a residue vector) and one per
    residue (the classes of a classword).  A link's setup replaces both.  */
static tc_ptab tcp_book[VB_MAXBOOK];
static tc_ptab tcp_cls[VB_MAXRES];
static tc_ptab tcp_flr[VB_MAXFLOOR][VB_MAXCLASS];

static void tcp_free(void) {
  u32 i, j;
  for (i = 0; i < VB_MAXBOOK; i++) tcp_book[i].drop();
  for (i = 0; i < VB_MAXRES; i++) tcp_cls[i].drop();
  for (i = 0; i < VB_MAXFLOOR; i++)
    for (j = 0; j < VB_MAXCLASS; j++) tcp_flr[i][j].drop();
  tcp_mem = 0;
}

static void tcp_build(void) {
  i32 v[TCP_SYMMAX];
  u32 i, j;
  const vb_setup * s = oc_v->cur;
  tcp_free();
  for (i = 0; i < s->nbk; i++) {
    const vb_book * b = s->bk + i;
    /*  balrogg keeps no length list for an empty book, and neither did
        vd_setup; a book without a lookup grid has no digits to model.  */
    if (b->look != 1 || !b->ent || !b->len || !b->mult || !b->nv) continue;
    if (b->nv > TCP_SYMMAX) continue;
    for (j = 0; j < b->nv; j++) v[j] = (i32) b->mult[j] - (i32) b->off;
    tcp_book[i].build(b->len, b->ent, b->nv, b->dim, 0, v, 1);
  }
  for (i = 0; i < s->nrs; i++) {
    const vb_res * r = s->rs + i;
    const vb_book * cb = s->bk + r->cbook;
    if (!cb->ent || !cb->len || r->ncl > TCP_SYMMAX) continue;
    for (j = 0; j < r->ncl; j++) v[j] = (i32) j;
    tcp_cls[i].build(cb->len, cb->ent, r->ncl, cb->dim, 1, v, 0);
  }
  /*  A floor partition: which subclass book coded a post is implied by the
      post's own value, so the classword and the subclass books together give
      one distribution over that value.  */
  for (i = 0; i < s->nfl; i++) {
    const vb_floor * f = s->fl + i;
    u32 c;
    for (c = 0; c < VB_MAXCLASS; c++) {
      const u8 * bl[8];
      u32 be[8], nb = 1u << f->csub[c], maxy = 1, k;
      const u8 * cl = nullptr;
      u32 cent = 0;
      if (!f->cdim[c] || f->csub[c] > 3) continue;
      for (k = 0; k < nb; k++) {
        i32 sb = f->csb[c][k];
        if (sb >= 0 && (u32) sb < s->nbk && s->bk[sb].ent && s->bk[sb].len) {
          bl[k] = s->bk[sb].len;  be[k] = s->bk[sb].ent;
          if (be[k] > maxy) maxy = be[k];
        } else if (sb >= 0) { bl[k] = nullptr;  be[k] = 0;  maxy = 0;  break; }
        else { bl[k] = nullptr;  be[k] = 1; }
      }
      if (!maxy || maxy > TCP_SYMMAX) continue;
      if (f->csub[c] && f->cbook[c] >= 0 && (u32) f->cbook[c] < s->nbk
          && s->bk[f->cbook[c]].ent)
        { cl = s->bk[f->cbook[c]].len;  cent = s->bk[f->cbook[c]].ent; }
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
/*  A Volume is folded through this rather than with a bare multiply.  There
    are up to two dozen factors in one, and IDX/opt.pl widening several at
    once carries the product past what 64 bits hold; a product that wraps can
    land on a number that looks perfectly reasonable, and then the table is
    sized from it and the coder walks off the end of it in silence.
    Saturating means tbl_n and the memory cap see a number at least as large
    as what was asked for, so they refuse it.  */
constexpr long long TC_VOLMAX = (long long) 1 << 60;
constexpr long long tc_vmul(long long a, long long b) {
  return (a <= 0 || b <= 0 || a > TC_VOLMAX / b) ? TC_VOLMAX : a * b;
}

/*  IDX-FORMAT.md sec.9: this exists so the compiler can prove the byte product
    behind a table cannot overflow.  The bound is on entries and is deliberately
    far above anything a sane index reaches -- what actually bounds a model is
    TC_MEMCAP, below, which is about the whole of it rather than one table.  */
static INLINE u64 tbl_n(u64 n) {
  FATAL_UNLESS(n > 0 && n <= ((u64) 1 << 40),
               "IDX/tsvcomp-*.idx asks for a table of %" PRIu64 " entries", n);
  return n;
}

/*  A Volume as the index builder can address it.  idx2inc.pl accumulates
    every index in an int, so this is the one limit no component escapes.  */
static void tc_vfits(const char * what, sz vol, sz nodes) {
  FATAL_UNLESS(vol > 0 && vol < ((sz) 1 << 31) && nodes > 0,
               "IDX/tsvcomp-*.idx gives the %s an index of %" PRIu64 " rows -- "
               "idx2inc.pl builds indices in int, so a Volume must stay under "
               "2147483648", what, (u64) vol);
}

/*  And a Volume as the component itself can address it, for the components
    that reach a row by multiplying the index by a width and keeping the
    product in an int.  */
static void tc_ifits(const char * what, sz vol, sz mult) {
  tc_vfits(what, vol, mult);
  FATAL_UNLESS(vol * mult < ((sz) 1 << 31),
               "IDX/tsvcomp-*.idx gives the %s %" PRIu64 " rows of %" PRIu64
               " -- it addresses them with an int, so the product must stay "
               "under 2147483648", what, (u64) vol, (u64) mult);
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
/*  A floor post is never negative, so its sign is one counter saturating
    towards "positive" rather than a context: spread over the family index and
    put through the mixer it takes longer to get there, which measured +40
    bytes on the floor of 00000000 and nothing anywhere else.  A field that is
    genuinely signed pays for a context; one that is not pays for a counter.

    fam_cls has no sign at all.  A residue class is a small non-negative
    number and the family codes it through `code`, which never reaches a sign
    -- so the counter it used to declare was never read, and the rate and
    bound the .idx offered for it were two knobs the optimizer could spend
    moves on and never move anything.  */
constexpr int TC_SGN_FLR = 1;

/*  How deep each family's tables are, per context.  A family pays for the
    nodes it can reach and no more, and what it can reach is a property of
    which entry point it uses:

      dig, flr, cls, aux, hdr   code() or codes(): the ladder and the unary
                                length, so nodes 0..28, and a mantissa plane
      sgn                       bit(0) and nothing else -- a sign is one bit

    fam_sgn was the expensive one.  Laid out like the rest it took 199.9 MB
    for the 1/29th of it node 0 addresses, and, worse, put consecutive
    contexts' counters 116 bytes apart, so the one counter a sign reads
    arrived alone in its cache line.  It is asked once per non-zero digit.

    The mantissa depth is the same story one step further: sgn never calls
    bitm(), so its mantissa plane and mantissa mixer are one row each.  */
constexpr int TC_dig_ND = TC_NODE,  TC_dig_MND = TC_MNODE;
constexpr int TC_sgn_ND = 1,        TC_sgn_MND = 1;
constexpr int TC_flr_ND = TC_NODE,  TC_flr_MND = TC_MNODE;
constexpr int TC_cls_ND = TC_NODE,  TC_cls_MND = TC_MNODE;
constexpr int TC_aux_ND = TC_NODE,  TC_aux_MND = TC_MNODE;
constexpr int TC_hdr_ND = TC_NODE,  TC_hdr_MND = TC_MNODE;

/*  One IDX module per family -- IDX-FORMAT.md sec.13.  Each declares its own
    mapping objects, its own Volumes and its own table struct, all under the
    prefix TC_<family>, so every generated name is the one it always was and an
    export.!!! keyed by those names still imports.  */
#include "MOD/tsvcomp-dig_h.inc"
#include "MOD/tsvcomp-sgn_h.inc"
#include "MOD/tsvcomp-flr_h.inc"
#include "MOD/tsvcomp-cls_h.inc"
#include "MOD/tsvcomp-aux_h.inc"
#include "MOD/tsvcomp-hdr_h.inc"

/*  Mapped in tc_models(), not declared static -- see there.  */
static TC_dig_T * tcm_dig;
static TC_sgn_T * tcm_sgn;
static TC_flr_T * tcm_flr;
static TC_cls_T * tcm_cls;
static TC_aux_T * tcm_aux;
static TC_hdr_T * tcm_hdr;

/*  Everything a residue digit's contexts are built from.  */
struct tc_dv {
  int rno, pass, band, col, vpos, q1, q2, q1s, q2s, t1, t1s, t2, n1, w1,
      p0, p0s, pn, cls, zrun, blk, bkq, ax, mg, pq, sq, ps, chn, crun, cm,
      x1, x1s;
};

/*  One family's five index rows for one value: the two counters, the APM, the
    mixer and the mantissa plane each get their own.  */
struct tcx { int a, b, c, d, s, f, m, t, n, g; };

#include "MOD/tsvcomp-dig_p.inc"
#include "MOD/tsvcomp-sgn_p.inc"
#include "MOD/tsvcomp-flr_p.inc"
#include "MOD/tsvcomp-cls_p.inc"
#include "MOD/tsvcomp-aux_p.inc"
#include "MOD/tsvcomp-hdr_p.inc"

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
  if (tc_enc) rce.rc_Process((uint) (p << 3), b);
  else        rcd.rc_Process((uint) (p << 3), b);
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
  cm_mix<TC_MIXN> mx;
  /*  The mantissa's own two-input mix.  It stays out of the main mixer
      because what it weighs is different -- one counter against the
      codebook -- and because a bare counter is what a decision this cheap
      deserves when there is no codebook to weigh it against.  */
  cm_mix<3> mxm;
  tc_pcur pc;                             /*  the prior for the value in hand  */
  int prp;                                /*  P(this bit is zero) from it, or -1  */
  /*  The four counters' bases are 64-bit: a Volume just under the index
      builder's 2^31 limit times TC_NODE is past what 32 bits hold, and a
      base that wraps addresses another counter's row.  The APM, mixer and
      mantissa rows are bounded to an int by tc_ifits, so theirs need not be.  */
  sz  ba, bb, bc, bd;
  u32 bs, bf, bm, bt, bn, bg;
  /*  Ten predictors, ten sets of parameters.  What had been shared: the
      mantissa counter and the sign counter both updated at counter A's rate,
      the two APMs at one rate between them, the mantissa mixer at the main
      mixer's rate and boost, and all six counters at one bound.  A parameter
      two components share is a parameter neither of them owns -- the
      optimizer moves it and two things move, so it settles where the louder
      one wants it and the quieter one never gets an answer.  */
  int rA, rB, rC, rD, rT, rG;             /*  one rate per counter  */
  int mwA, mwB, mwC, mwD, mwT, mwG;       /*  one bound per counter  */
  int rS1, rS2;                           /*  one limit per APM, from its rate  */
  int lr, mb, lrm, mbm;                   /*  one rate and boost per mixer  */
  int bw;
  int ng;                                 /*  the sign counter's field span  */
  int nd, nm;                             /*  and how deep its tables are  */

  void wire(cm_cnt * a, sz va, cm_cnt * b, sz vb, cm_cnt * c, sz vc,
            cm_cnt * d, sz vd, cm_cnt * t, sz vt,
            i16 * s, u8 * sc, sz vs, i16 * s2, u8 * sc2, sz vf,
            i32 * w, sz vm, cm_cnt * g, sz vg, sz vgi, i32 * wm, sz vn,
            u8 * wc, u8 * wmc,
            int ra, int rb, int rc_, int rd, int rt, int rg,
            int mwa, int mwb, int mwc, int mwd, int mwtt, int mwg,
            int rs1, int rs2, int lrate, int mbt, int lrm2, int mbm2,
            int bwt, int qqs, int qqf, int ndp, int nmp) {
    A = a;  B = b;  C = c;  D = d;  T = t;  S = s;  S2 = s2;  W = w;  G = g;
    /*  A mapped table is already zero, which is every counter's, APM's and
        mixer's unlearned state, so there is nothing to fill in either build.

        What does have to be said is how wide an index each component can
        actually address, because that is not one number.  idx2inc.pl builds
        every index in `int`, so no Volume may reach 2^31 whatever the table
        behind it costs.  Past that the four counters are the generous ones --
        their row is reached through a 64-bit base, so a Volume near the limit
        is merely expensive.  The APM multiplies its row count by the curve
        width and keeps the product in an int; the mixers multiply theirs by
        the input count and do the same.  An index that passes the first check
        and fails one of these would wrap into its own table and code against
        the wrong row, quietly, so each is checked where its width is known.  */
    ng = (int) vg;
    nd = ndp;  nm = nmp;
    tc_vfits("counter A", va, nd);
    tc_vfits("counter B", vb, nd);
    tc_vfits("counter C", vc, nd);
    tc_vfits("counter D", vd, nd);
    tc_vfits("counter T", vt, nm);
    if (G) tc_vfits("counter G", vg, vgi);
    tc_ifits("APM 1", vs, (sz) nd * qqs);
    tc_ifits("APM 2", vf, (sz) nd * qqf);
    tc_ifits("mixer", vm, (sz) nd * TC_MIXN);
    tc_ifits("mantissa mixer", vn, (sz) nm * 3);
    (void) sc;  (void) sc2;  (void) wc;  (void) wmc;
    /*  Each APM sizes its own curve.  The two do different jobs on different
        contexts -- one corrects a single counter and goes to the mixer, the
        other corrects what the mixer made of all of them -- so how many
        points that correction is worth is two questions, not one.  */
    ap.init(S, sc, vs * nd, qqs);
    ap2.init(S2, sc2, vf * nd, qqf);
    mx.init(W, wc, vm * nd);
    mxm.init(wm, wmc, vn * nm);
    /*  A pattern is a search space and the optimizer visits its ends, so
        anything used as a size, a shift or a limit is clamped at the point of
        use -- IDX/IDX-FORMAT.md §5 says the same, and means it.  */
    rA = tc_clamp(ra, 1, CM_TMAX);
    rB = tc_clamp(rb, 1, CM_TMAX);
    rC = tc_clamp(rc_, 1, CM_TMAX);
    rD = tc_clamp(rd, 1, CM_TMAX);
    rT = tc_clamp(rt, 1, CM_TMAX);
    rG = tc_clamp(rg, 1, CM_TMAX);
    mwA = tc_clamp(mwa * 64, 0, CM_CONE / 4);
    mwB = tc_clamp(mwb * 64, 0, CM_CONE / 4);
    mwC = tc_clamp(mwc * 64, 0, CM_CONE / 4);
    mwD = tc_clamp(mwd * 64, 0, CM_CONE / 4);
    mwT = tc_clamp(mwtt * 64, 0, CM_CONE / 4);
    mwG = tc_clamp(mwg * 64, 0, CM_CONE / 4);
    rS1 = cm_apm::limit(tc_clamp(rs1, 1, 15));
    rS2 = cm_apm::limit(tc_clamp(rs2, 1, 15));
    lr = tc_clamp(lrate, 1, 64);
    mb = tc_clamp(mbt, 0, 255);
    lrm = tc_clamp(lrm2, 1, 64);
    mbm = tc_clamp(mbm2, 0, 255);
    bw = tc_clamp(bwt, 0, 16);
    pc.off();  prp = -1;
  }

  INLINE void select(const tcx & x) {
    ba = (sz) x.a * nd;
    bb = (sz) x.b * nd;
    bc = (sz) x.c * nd;
    bd = (sz) x.d * nd;
    bs = (u32) x.s * nd;
    bf = (u32) x.f * nd;
    bm = (u32) x.m * nd;
    bt = (u32) x.t * nm;
    bn = (u32) x.n * nm;
    bg = (u32) x.g;
    /*  Every row the value will touch is addressed now, before its first
        bit and before the prior's arithmetic for that bit, so the lines are
        on their way while that runs.  OGGCOMP-SPEED.md section 4.  */
    __builtin_prefetch(A + ba);  __builtin_prefetch(B + bb);
    __builtin_prefetch(C + bc);  __builtin_prefetch(D + bd);
    __builtin_prefetch(ap.t + (sz) bs * (u32) ap.q);
    __builtin_prefetch(ap.c + (sz) bs * (u32) ap.q);
    __builtin_prefetch(mx.w + (sz) bm * TC_MIXN);
    __builtin_prefetch(mx.c + bm);
    __builtin_prefetch(ap2.t + (sz) bf * (u32) ap2.q);
    __builtin_prefetch(ap2.c + (sz) bf * (u32) ap2.q);
  }

  /*  mp3c's six lines, widened: refine counter A through the APM, stretch
      it and the three others, mix them under one learned weight set, code,
      then update everything.  */
  INLINE int bit(int node, int b) {
    /*  Each counter is read once, as a copy, and written back once at the
        end.  Through a reference the row was loaded for P() and again for
        upd(), with the coder's call between the two to make the compiler
        doubt what it still held.  */
    cm_cnt a = A[ba + node];
    cm_cnt c = B[bb + node];
    cm_cnt e = C[bc + node];
    cm_cnt f = D[bd + node];
    int pa = a.P();
    int p1 = ap.pp(pa, bs + (u32) node);
    if (p1 > CM_PONE - 1) p1 = CM_PONE - 1;
    mx.set(0, cm_stretch(p1));
    mx.set(1, cm_stretch(c.P()));
    mx.set(2, cm_stretch(e.P()));
    mx.set(3, cm_stretch(f.P()));
    mx.set(4, cm_stretch(pa));
    /*  What the codebook says about this decision.  A family with no prior,
        or a decision the prior cannot speak to, contributes a zero, which is
        what a stretched even chance is: the input is there and says nothing,
        and the mixer's weight for it stays where the rest of the corpus put
        it rather than being trained on a different set of inputs.  */
    mx.set(5, prp >= 0 ? cm_stretch(prp) : 0);
    mx.set(6, 256);
    { int pm = mx.mix(bm + (u32) node);
      int pf = ap2.pp(pm, bf + (u32) node);
      /*  How much of the answer the second APM is allowed to be.  It was a
          fixed three parts mixer to one, which is 12 here.  */
      pf = (bw * pm + (16 - bw) * pf + 8) >> 4;
      b = tc_bit(pf < 1 ? 1 : pf > CM_PONE - 1 ? CM_PONE - 1 : pf, b); }
    mx.upd(b, lr, mb);
    ap.upd(b, rS1);
    ap2.upd(b, rS2);
    a.upd(b, rA, mwA);  A[ba + node] = a;
    c.upd(b, rB, mwB);  B[bb + node] = c;
    e.upd(b, rC, mwC);  C[bc + node] = e;
    f.upd(b, rD, mwD);  D[bd + node] = f;
    return b;
  }

  /*  A mantissa bit: one counter, no mixing.  What is left after the length
      has been coded is close to uniform, and a stage that cannot pay for
      itself should not be run four million times.  */
  INLINE int bitm(int node, int b) {
    cm_cnt t = T[bt + node];                /*  a copy, as in bit()  */
    if (prp < 0) {                          /*  nothing to weigh it against  */
      b = tc_bit(t.P(), b);
      t.upd(b, rT, mwT);  T[bt + node] = t;
      return b;
    }
    mxm.set(0, cm_stretch(t.P()));
    mxm.set(1, cm_stretch(prp));
    mxm.set(2, 256);
    { int pm = mxm.mix(bn + (u32) node);
      b = tc_bit(pm < 1 ? 1 : pm > CM_PONE - 1 ? CM_PONE - 1 : pm, b); }
    mxm.upd(b, lrm, mbm);
    t.upd(b, rT, mwT);  T[bt + node] = t;
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
      cm_cnt g = G[bg * (u32) ng + sc];
      s = tc_bit(g.P(), tc_enc && x < 0);
      g.upd(s, rG, mwG);  G[bg * (u32) ng + sc] = g;
    } else {
      /*  Unreachable: every family that codes a sign is wired with one, and
          the two that are not (dig, sgn) never call this.  The tables have no
          row for a sign, so this refuses rather than addressing a magnitude's.  */
      FATAL_CODE(BLR_EXIT_IO, "a family with no sign counter coded a sign");
    }
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

static i16 * dg_hist[VB_MAXRES];
static i16 * dg_hist2[VB_MAXRES];         /*  the same, one packet further back  */
/*  Running mean magnitude, 4.12 fixed.  u32 and not u16: the mean settles at
    about 241 times the digit, so a u16 wraps for a sustained magnitude over
    271 and `ax` -- the one axis counter D is built on -- becomes noise for
    that slot.  No book in the corpus here has a range that wide, which is why
    it had never shown; the clamp below bounds the digit at 4095, so u32 is
    what that clamp implies.  */
static u32 * dg_avg[VB_MAXRES];
static i16 * dg_pp[VB_MAXRES];            /*  [channel][slot]: the earlier pass's digit this packet  */
static i16 * dg_ps[VB_MAXRES];            /*  and everything the passes so far put there  */
static u8  * dg_pn[VB_MAXRES];            /*  [channel][slot]: passes that coded this slot so far  */
static u8  * cl_hist[VB_MAXRES];
static sz    dg_span[VB_MAXRES], cl_np[VB_MAXRES];
static i32 * dg_q1, * dg_q2, * dg_zr;     /*  [rno][pass][channel]  */
static i32   cl_last[VB_MAXRES][VB_MAXCH], cl_last2[VB_MAXRES][VB_MAXCH];
static i16 * fl_hist;                     /*  [blk][channel][post]  */
static i16 * fl_yhist;                    /*  the same, rebuilt into a curve  */
/*  Which floor configuration last wrote each of those rows, or -1 for none.
    A row is keyed by block size and channel, and which floor a channel uses
    is a property of the mode: two modes of the same block size may map the
    same channel onto different floors, whose posts are at different
    frequencies and whose count differs.  Read across that boundary the
    "previous packet" context is a post from someone else's layout.  */
static i32 * fl_who;
static u32   tc_blk;                      /*  the block this packet is  */
static u32   tc_nchan;

static void tc_hist_free(void) {
  u32 i;
  for (i = 0; i < VB_MAXRES; i++) {
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
  free(fl_who);
  dg_q1 = dg_q2 = dg_zr = nullptr;  fl_hist = nullptr;  fl_yhist = nullptr;
  fl_who = nullptr;
}

/*  What the floor model wants of a floor that balrogg's vb_floor does not
    keep: each post's class and place within its partition, its rank in
    ascending X, and the two posts either side of it among those that
    precede it in list order -- libvorbis's low and high neighbours, which
    are what a post >= 2 is coded against.  Derived once per setup.  */
struct oc_floor {
  u8 pcl[VB_MAXPOST], ppos[VB_MAXPOST];  /*  post >= 2: its class and place  */
  u8 rnk[VB_MAXPOST];                    /*  a post's place in ascending X  */
  u8 lo[VB_MAXPOST], hi[VB_MAXPOST];     /*  decode neighbours, for post >= 2  */
};
static oc_floor oc_fl[VB_MAXFLOOR];

static void oc_floor_tables(const vb_floor * f, oc_floor * d) {
  u32 i, j, k, posts = 2;
  memset(d, 0, sizeof *d);
  for (i = 0; i < f->parts; i++)
    for (j = 0; j < f->cdim[f->pcls[i]]; j++) {
      if (posts >= VB_MAXPOST) break;
      d->pcl[posts] = f->pcls[i];  d->ppos[posts] = (u8) j;
      posts++;
    }
  for (i = 0; i < f->posts; i++) d->rnk[f->srt[i]] = (u8) i;
  for (i = 0; i + 2 < f->posts; i++) {
    u32 l = 0, h = 1, lx = 0, hx = f->x[1], cx = f->x[i + 2];
    for (k = 0; k < i + 2; k++) {
      if (f->x[k] > lx && f->x[k] < cx) { l = k;  lx = f->x[k]; }
      if (f->x[k] < hx && f->x[k] > cx) { h = k;  hx = f->x[k]; }
    }
    d->lo[i] = (u8) l;  d->hi[i] = (u8) h;
  }
}

/*  A link's setup has just been read; size the histories it implies.  A
    residue whose span is beyond what is worth holding simply loses its
    temporal context -- the other contexts still apply, so the stream stays
    codable rather than refused.  */
static void tc_setup_done(void) {
  const vb_setup * s = oc_v->cur;
  u32 i;
  tc_hist_free();
  tcp_build();
  tc_nchan = oc_v->i.ch;
  for (i = 0; i < s->nrs; i++) {
    const vb_res * r = s->rs + i;
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
    memset(cl_last[i], 0, sizeof cl_last[i]);
    memset(cl_last2[i], 0, sizeof cl_last2[i]);
  }
  dg_q1 = (i32 *) tc_alloc((sz) s->nrs * 8 * tc_nchan * sizeof(i32));
  dg_q2 = (i32 *) tc_alloc((sz) s->nrs * 8 * tc_nchan * sizeof(i32));
  dg_zr = (i32 *) tc_alloc((sz) s->nrs * 8 * tc_nchan * sizeof(i32));
  fl_hist = (i16 *) tc_alloc((sz) 2 * tc_nchan * VB_MAXPOST * sizeof(i16));
  fl_yhist = (i16 *) tc_alloc((sz) 2 * tc_nchan * VB_MAXPOST * sizeof(i16));
  fl_who = (i32 *) tc_alloc((sz) 2 * tc_nchan * sizeof(i32));
  for (i = 0; i < 2 * tc_nchan; i++) fl_who[i] = -1;
  for (i = 0; i < s->nfl; i++) oc_floor_tables(s->fl + i, oc_fl + i);
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
constexpr sz OC_TAGMAX = 31;                /*  what a tag may be, as tsv.inc had it  */
static char tc_tagbuf[TC_MAXTAG * (OC_TAGMAX + 1)];
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
    char * dst = tc_tagbuf + (sz) tc_ntag * (OC_TAGMAX + 1);
    /*  TC_MAXTAG is what tc_tag, tc_tlast and the sign table hold, so a
        reader that wants more than that is refused here rather than found
        later.  The model's tag axis is 64 wide, and it is a threshold list
        now rather than a bare multiply, so a tag past it saturates into the
        last bucket -- with every other overflow tag, and away from tag 0's
        statistics -- instead of indexing off the end of the table the way
        `ADD 64: tag` did.  The corpus reaches 57 tags, so that was seven
        short of happening.  */
    FATAL_UNLESS(tc_ntag < TC_MAXTAG, "the reader asks for more than %d tags",
                 TC_MAXTAG);
    snprintf(dst, OC_TAGMAX + 1, "%s", tag);
    tc_tag[tc_ntag] = dst;
    id = (int) tc_ntag++;
  }
  tc_runpos = (id == tc_lasttag) ? tc_runpos + 1 : 0;
  tc_lasttag = id;
  return id;
}

/*  BALROGG-MODEL.md sec.7: what this tag held last time is used twice over.
    As a context it is quantised, which throws away exactly the low bits that
    say how much it moved; as a prediction it is not, so what the cascade sees
    is a difference that is usually small and often zero.  balrogg carries an
    `order` per field in mdl_cfg for the same reason and most of its 44 fields
    are order-1.

    Only the header packets.  The same difference on the page and packet
    fields costs 4653 bytes over the corpus where this saves 154: those are
    mostly flags and lengths that do not drift, and a difference turns a value
    the model had learned into one it has to learn again.  Deciding per field
    which of the two to use -- balrogg's table, or a score kept on both sides
    -- was tried and is worse than either fixed choice, because a context row
    that sees both regimes has learned neither.  */
static i64 tc_hdrval(int id, i64 x) {
  tcx v;
  i64 pr = tc_tlast[id];
  tc_make_hdr(id, tc_qlog(tc_tlast[id]), tc_qlog(tc_tlast2[id]),
              tc_qlog(tc_runpos), (int) (tc_tlast[id] & 255), v);
  x = fam_hdr.codes(v, tc_enc ? x - pr : 0, (u32) id) + pr;
  tc_tlast2[id] = tc_tlast[id];  tc_tlast[id] = x;
  return x;
}

static INLINE i32 tc_sq(i32 v) {          /*  signed log-ish quantisation  */
  i32 q = tc_qlog(v);
  return v < 0 ? -q : q;
}
/*  The least common multiple, for the neighbour stride below.  */
static INLINE u32 tc_gcd(u32 a, u32 b) { while (b) { u32 t = a % b;  a = b;  b = t; } return a; }

/*  ------------------------------------------------------------------
    The residue digits.  io::residue walks the passes, classwords and
    partitions and io::rs_sym the vectors; what is here is the state a
    partition's digits share and the coding of one digit, which is tc_part's
    loop body with the loop taken out.  */

static i32 cl_run[VB_MAXRES];             /*  how long the class has held  */
static u8 cl_same[1u << 20];              /*  and whether it held from last packet  */
static u8 cl_run_buf[1u << 20];           /*  the run and the match, per slot  */

/*  A residue being walked: what tc_residue computed on entry.  */
static struct {
  u32 vch, np, pv, w, ilv;
  u8 cidx[VB_MAXCH];                      /*  the channel each coded vector is  */
} R;

/*  A partition being coded: what tc_part computed on entry.  */
static struct {
  sz span;
  i16 * hist, * hist2;
  u32 * avg;
  i16 * pp, * ps;
  u8 * pn;
  i32 * q1, * q2, * zr;
  u32 str;
  const vb_book * bk;
  const tc_ptab * pt;
  u32 rno, pss, j, pc, psz, cls, dim, ilv, i;
  u32 vmod, smod;                         /*  i mod dim, slot mod ilv  */
  i32 bkq, clrun;
  int clmatch;
} P;

static void oc_part_begin(u32 rno, u32 pss, u32 jv, u32 pc, const vb_res * r,
                          const vb_book * bk, u32 bn, u32 cls) {
  u32 j = R.cidx[jv];                     /*  the channel, not the vector  */
  sz sl = (sz) jv * R.w + pc;
  sz span = dg_span[rno];
  sz hb = ((sz) (tc_blk * 8 + pss) * tc_nchan + j) * span;
  sz base = (sz) ((rno * 8 + pss) * tc_nchan + j);
  u32 dim = bk->dim, ilv = R.ilv;
  P.rno = rno;  P.pss = pss;  P.j = j;  P.pc = pc;  P.psz = r->psz;
  P.cls = cls;  P.dim = dim;  P.ilv = ilv;  P.i = 0;
  P.vmod = 0;  P.smod = ilv ? (u32) (((sz) pc * r->psz) % ilv) : 0;
  P.bkq = tc_qlog(bk->off);
  P.clrun = sl < sizeof cl_run_buf ? cl_run_buf[sl] : 0;
  P.clmatch = sl < sizeof cl_same ? cl_same[sl] : 0;
  P.span = span;
  P.hist  = span ? dg_hist[rno]  + hb : nullptr;
  P.hist2 = span ? dg_hist2[rno] + hb : nullptr;
  P.avg   = span ? dg_avg[rno]   + hb : nullptr;
  P.pp    = span ? dg_pp[rno] + (sz) j * span : nullptr;
  P.ps    = span ? dg_ps[rno] + (sz) j * span : nullptr;
  P.pn    = span ? dg_pn[rno] + (sz) j * span : nullptr;
  P.q1 = dg_q1 + base;  P.q2 = dg_q2 + base;  P.zr = dg_zr + base;
  /*  How far away "the same place, one vector along" is.  In an interleaved
      residue a vector holds one place of every channel, so stepping by the
      book's dimension lands on the same channel only when the dimension is a
      multiple of the channel count -- 00000007 codes a 3-dimensional book
      over 2 channels, where every step by 3 crosses to the other channel and
      the neighbourhood `ax` is built from is half someone else's.  The least
      common multiple is the nearest step that stays put, and is the dimension
      itself in the ordinary case.  */
  P.str = ilv && dim ? dim / tc_gcd(dim, ilv) * ilv : dim;
  P.bk = bk;
  P.pt = tcp_book[bn].ok ? &tcp_book[bn] : nullptr;
  tc_stage = STG_DIGIT;
  fam_dig.pc.off();
}

/*  One digit: tc_part's loop body.  `dg` is the digit in mode c and is
    ignored in mode d; what comes back is the digit either way.  */
static i64 oc_digit(i64 dg) {
  u32 i = P.i++;
  sz slot = (sz) P.pc * P.psz + i;
  int ok = P.hist && slot < P.span;
  tcx v;
  tc_dv d;
  i32 t1 = ok ? P.hist[slot] : 0;
  i32 t2 = ok ? P.hist2[slot] : 0;
  i32 n1 = ok && slot + P.str < P.span ? P.hist[slot + P.str] : 0;
  i32 w1 = ok && slot >= P.str ? P.hist[slot - P.str] : 0;
  /*  The channel beside this one, at this very place, a packet ago.  It is
      what the neighbour stride used to return by accident whenever a book
      was one-dimensional, and losing it cost more than the wrong stride
      did: `t1` already says what this channel held here, so a same-channel
      neighbour repeats it, where the coupled channel does not.  In a
      residue coded per channel there is nothing beside the slot and this
      is zero.  */
  sz xs = P.ilv > 1 ? slot - P.smod + (P.smod ? P.smod - 1 : P.ilv - 1) : 0;
  i32 x1 = ok && P.ilv > 1 && xs < P.span ? P.hist[xs] : 0;
  i32 av = ok ? P.avg[slot] : 0;
  i32 p0 = ok ? P.pp[slot] : 0;
  i32 psum = ok ? P.ps[slot] : 0;
  i32 pnn = ok ? P.pn[slot] : 0;
  d.rno = (int) P.rno;  d.pass = (int) P.pss;  d.band = tc_qlog(P.pc);
  d.col = tc_qlog(i);  d.vpos = (int) P.vmod;
  d.q1 = tc_qlog(*P.q1);  d.q2 = tc_qlog(*P.q2);
  d.q1s = tc_sq(*P.q1);  d.q2s = tc_sq(*P.q2);
  d.t1 = tc_qlog(t1);  d.t1s = tc_sq(t1);
  d.t2 = tc_qlog(t2);  d.n1 = tc_qlog(n1);  d.w1 = tc_qlog(w1);
  d.p0 = tc_qlog(p0);  d.p0s = tc_sq(p0);  d.pn = pnn;
  d.x1 = tc_qlog(x1);  d.x1s = tc_sq(x1);
  /*  What the passes before this one have already put at this slot,
      signed.  Once three passes have hit a slot the last one alone is a
      poor summary of where the value stands; the refinement leans against
      the running total, not against the last correction.  */
  d.ps = tc_sq(psum);
  /*  Which channel this digit belongs to.  An interleaved residue holds
      them all in one vector, so it is the slot's position modulo the number
      interleaved -- the number this submap interleaves, which is not always
      the number the stream has; anything else is the channel the vector is,
      which for a residue coded per channel is the whole answer.  */
  d.chn = (int) (P.ilv ? P.smod : P.j);
  d.crun = tc_qlog(P.clrun);  d.cm = P.clmatch;
  d.cls = (int) P.cls;  d.zrun = tc_qlog(*P.zr);  d.blk = (int) tc_blk;
  d.bkq = P.bkq;
  /*  The prior restarted with the vector, in oc_vec_begin; it walks down
      one level per digit.  */
  d.pq = tcp_axis(fam_dig.pc);
  d.sq = 0;
  { /*  mp3c's exA: a weighted mean of the neighbourhood, log-quantised  */
    i64 ex = (i64) (av >> 4) * TC_dig_avA + (i64) tc_abs(t1) * TC_dig_avT1
           + (i64) tc_abs(t2) * TC_dig_avT2 + (i64) tc_abs(n1) * TC_dig_avN1
           + (i64) tc_abs(w1) * TC_dig_avW1 + (i64) tc_abs(*P.q1) * TC_dig_avQ1
           + (i64) tc_abs(*P.q2) * TC_dig_avQ2 + (i64) tc_abs(x1) * TC_dig_avX1;
    /*  In 64 bits, because the weights are a search space: `avg` reaches a
        million, the digits are whatever the stream holds, and nine terms
        times a scale of 1023 leaves an int only as much room as the tuned
        values happen to use.  A context that wraps is noise for that slot,
        and it would wrap where the optimizer looks.  */
    d.ax = tc_qlog((ex * TC_dig_avC) >> 12);
  }
  tc_make_dig(d, v);
  if (!tc_enc) dg = 0;
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
    i64 gv = dg + (i64) P.bk->off;
    fam_dig.pc.step(P.bk->inv && gv >= 0 && gv < (i64) P.bk->base
                    && P.bk->inv[gv] != (u32) -1 ? (int) P.bk->inv[gv] : -1);
  }
  if (ok) {
    i16 c = (i16) (dg < -32768 ? -32768 : dg > 32767 ? 32767 : dg);
    i32 a = tc_abs(c);
    P.hist2[slot] = P.hist[slot];
    P.hist[slot] = c;
    P.avg[slot] = (((u32) P.avg[slot] * (u32) TC_dig_avD) >> 8)
                + (u32) (a > 4095 ? 4095 : a) * 16;
    P.pp[slot] = c;  P.pn[slot] = (u8) (pnn < 7 ? pnn + 1 : 7);
    { i32 t = psum + c;
      P.ps[slot] = (i16) (t < -32768 ? -32768 : t > 32767 ? 32767 : t); }
  }
  *P.q2 = *P.q1;  *P.q1 = (i32) dg;
  *P.zr = dg ? 0 : *P.zr + 1;
  /*  The two positions step on; each was a division per digit before.  */
  if (P.dim && ++P.vmod == P.dim) P.vmod = 0;
  if (P.ilv && ++P.smod == P.ilv) P.smod = 0;
  if (tc_verbose) tc_syms[STG_DIGIT]++;
  return dg;
}

/*  A residue class: tc_classify, with the value passed in rather than read.  */
static u32 tc_classify(u32 rno, u32 j, u32 slot, u32 cin) {
  tcx v;
  u8 * hist = cl_np[rno] ? cl_hist[rno] + ((sz) tc_blk * tc_nchan + j) * cl_np[rno]
                         : nullptr;
  i32 t1 = (hist && slot < cl_np[rno]) ? hist[slot] : 0;
  i32 tn = (hist && slot + 1 < cl_np[rno]) ? hist[slot + 1] : 0;
  i64 c;
  tc_stage = STG_CLASS;
  tc_make_cls((int) rno, tc_qlog(slot), cl_last[rno][j], t1, (int) tc_blk, tn,
              cl_last2[rno][j], tcp_axis(fam_cls.pc), v);
  c = tc_enc ? (i64) cin : 0;
  c = fam_cls.code(v, c);
  FATAL_UNLESS(c >= 0 && c < 16, "coded stream: residue class %" PRId64, c);
  cl_run[rno] = (c == cl_last[rno][j]) ? cl_run[rno] + 1 : 0;
  if (hist && slot < cl_np[rno]) hist[slot] = (u8) c;
  cl_last2[rno][j] = cl_last[rno][j];  cl_last[rno][j] = (i32) c;
  if (tc_verbose) tc_syms[STG_CLASS]++;
  return (u32) c;
}

/*  tc_residue's entry: which channel each coded vector belongs to, and the
    per-residue histories cleared.  io::residue has already returned for a
    residue with nothing to code, so this sees only the ones it walks.  */
static void oc_residue_begin(u32 rno, const vb_res * r, const u8 * nz, u32 nch, u32 n) {
  const vb_book * cb = oc_v->cur->bk + r->cbook;
  u32 end, i, k, nc = 0;
  /*  A residue coded per channel skips the channels whose floor was not
      used, so the v-th vector is the v-th *non-zero* channel and not the
      v-th channel -- and every history here is per channel, so keying them
      by the vector's position slides one channel's past into another's the
      first time a channel falls silent.  An interleaved residue is one
      vector over all of them, and is its own key.  */
  for (i = 0; i < nch; i++) if (nz[i]) nc++;
  if (r->type == 2) { R.vch = 1;  R.ilv = nch;  R.cidx[0] = 0;
                      end = blr_min(r->end, n * nch); }
  else { R.vch = nc;  R.ilv = 0;  end = blr_min(r->end, n);
         for (i = 0, k = 0; i < nch; i++) if (nz[i]) R.cidx[k++] = (u8) i; }
  R.np = end > r->beg ? (end - r->beg) / r->psz : 0;
  R.pv = cb->dim;
  R.w = R.np + R.pv;
  FATAL_UNLESS((sz) R.vch * R.w <= sizeof cl_run_buf,
               "residue needs %" PRIu64 " classification slots, limit %" PRIu64,
               (u64) ((sz) R.vch * R.w), (u64) sizeof cl_run_buf);
  memset(dg_q1 + (sz) rno * 8 * tc_nchan, 0, (sz) 8 * tc_nchan * sizeof(i32));
  memset(dg_q2 + (sz) rno * 8 * tc_nchan, 0, (sz) 8 * tc_nchan * sizeof(i32));
  memset(dg_zr + (sz) rno * 8 * tc_nchan, 0, (sz) 8 * tc_nchan * sizeof(i32));
  if (dg_span[rno]) {
    memset(dg_pp[rno], 0, (sz) tc_nchan * dg_span[rno] * sizeof(i16));
    memset(dg_ps[rno], 0, (sz) tc_nchan * dg_span[rno] * sizeof(i16));
    memset(dg_pn[rno], 0, (sz) tc_nchan * dg_span[rno]);
  }
}

/*  Place `k` of vector `jv`'s classword, at partition `pc`.  The classes of
    `pv` consecutive partitions are the places of one classword, so they get
    the classbook's prior the same way a vector's digits get the residue
    book's; the run and the match are kept per slot for the digits to read.  */
static u32 oc_cls(u32 rno, u32 jv, u32 pc, u32 k, u32 cin) {
  u32 j = R.cidx[jv];
  sz sl = (sz) jv * R.w + pc + k;
  u8 * ch = cl_np[rno] ? cl_hist[rno] + ((sz) tc_blk * tc_nchan + j) * cl_np[rno]
                       : nullptr;
  i32 was = (ch && pc + k < cl_np[rno]) ? ch[pc + k] : -1;
  i32 run = cl_run[rno];
  u32 c;
  if (k == 0) fam_cls.pc.start(tcp_cls[rno].ok ? &tcp_cls[rno] : nullptr);
  c = tc_classify(rno, j, pc + k, cin);
  if (sl < sizeof cl_run_buf) {
    cl_run_buf[sl] = (u8) (run > 255 ? 255 : run);
    cl_same[sl] = (u8) (was == (i32) c);
  }
  fam_cls.pc.step((int) c);
  return c;
}

/*  ------------------------------------------------------------------
    The floor of one channel: tc_payload's per-channel block.  */
static u8 tc_used[VB_MAXCH];
static u8 oc_fno[VB_MAXCH];               /*  the floor each channel used this packet  */
static i16 fl_cur[VB_MAXPOST];            /*  this channel's coded residuals  */
static i16 fl_fy[VB_MAXPOST];             /*  and the curve they rebuild  */

static void oc_flr(u32 k, u32 fno, const vb_floor * f, u32 * y, int & used) {
  const oc_floor * fd = oc_fl + fno;
  sz row = (sz) tc_blk * tc_nchan + k;
  i16 * hp = fl_hist + row * VB_MAXPOST;
  i16 * hy = fl_yhist + row * VB_MAXPOST;
  /*  This channel's own past, and the channel before's, count only if they
      are this floor's and, for the neighbour, from this packet: a channel
      whose floor was not used this packet still holds whatever it held
      when it last was, which is not "the channel beside this one, now".  */
  int mine = fl_who[row] == (i32) fno;
  int hasn = k && tc_used[k - 1] && fl_who[row - 1] == (i32) oc_fno[k - 1];
  u32 u, p;
  i32 quant = (i32) f->quant;
  oc_fno[k] = (u8) fno;
  tc_stage = STG_AUD;
  u = (u32) tc_auxc(F_USED, tc_used[k], tc_enc ? (i64) used : 0, 0);
  FATAL_UNLESS(u < 2, "coded stream: flr.used is %u", u);
  tc_used[k] = (u8) u;
  used = (int) u;
  if (!u) return;
  tc_stage = STG_FLOOR;
  fam_flr.pc.off();
  /*  The packet holds the posts in ascending X, which is the order the
      column context is about; the coding is in list order, which is the
      order the prediction is available in.  `y` is by post number, so the
      swap is only in which post is coded when.  */
  if (tc_enc) for (p = 0; p < f->posts; p++) fl_cur[p] = (i16) y[p];
  for (p = 0; p < f->posts; p++) {
    tcx v;
    i64 yy;
    i32 pred = 0, room = 0, hl = 0, lov = 0, hiv = 0, ep = 0;
    i32 o1 = hasn ? hp[p - (sz) VB_MAXPOST] : 0;  /*  the channel before  */
    i32 od = hasn && mine ? hy[p - (sz) VB_MAXPOST] - hy[p] : 0;
    if (p >= 2) {
      u32 l = fd->lo[p - 2], h = fd->hi[p - 2];
      i32 hiroom, lowroom;
      u32 c = fd->pcl[p];
      if (!fd->ppos[p])
        fam_flr.pc.start(tcp_flr[fno][c].ok ? &tcp_flr[fno][c] : nullptr);
      pred = vd_render((i32) f->x[l], (i32) f->x[h], fl_fy[l], fl_fy[h],
                       (i32) f->x[p]);
      if (pred < 0) pred = 0;
      if (pred > quant - 1) pred = quant - 1;
      hiroom = quant - pred;  lowroom = pred;
      room = (hiroom < lowroom ? hiroom : lowroom) << 1;
      hl = hiroom > lowroom ? 1 : hiroom < lowroom ? 2 : 0;
      lov = fl_cur[l];  hiv = fl_cur[h];
      ep = mine ? vd_fold(hy[p], pred, quant) : 0;
      if (ep < 0) ep = 0;
    }
    tc_make_flr((int) fno, (int) (fd->rnk[p] < VB_MAXPOST ? fd->rnk[p] : VB_MAXPOST - 1),
                tc_qlog(mine ? hp[p] : 0), tc_qlog(mine ? hy[p] : 0),
                p < 2, (int) tc_blk,
                tc_qlog(o1), tc_qlog(pred), tc_sq(od), (int) k,
                tc_qlog(room), hl, tc_qlog(lov), tc_qlog(hiv),
                tcp_axis(fam_flr.pc), tc_qlog(ep), v);
    yy = tc_enc ? fl_cur[p] : 0;
    yy = fam_flr.codes(v, yy, 0);
    FATAL_UNLESS(yy >= 0 && yy < 32768, "coded stream: floor post %" PRId64, yy);
    fl_cur[p] = (i16) yy;
    fl_fy[p] = (i16) (p < 2 ? (yy > quant - 1 ? quant - 1 : (i32) yy)
                            : vd_unfold((i32) yy, pred, quant));
    if (fam_flr.pc.on)
      fam_flr.pc.step(yy >= 0 && yy < (i64) fam_flr.pc.t->nsym ? (int) yy : -1);
    if (tc_verbose) tc_syms[STG_FLOOR]++;
  }
  if (!tc_enc) for (p = 0; p < f->posts; p++) y[p] = (u32) fl_cur[p];
  for (p = 0; p < f->posts; p++) { hp[p] = fl_cur[p];  hy[p] = fl_fy[p]; }
  fl_who[row] = (i32) fno;
}

/*  ------------------------------------------------------------------
    The audio packet's mode and window flags: tc_audio, in three pieces.  */
static u32 tc_prevW;                      /*  the block before this one  */
static u32 tc_prevmode;
static u32 oc_nextW, oc_wp;

static u32 oc_mode(u32 md, u32 nmd) {
  tc_stage = STG_AUD;
  md = (u32) tc_auxc(F_MODE, (i32) tc_prevmode, tc_enc ? (i64) md : 0, 0);
  FATAL_UNLESS(md < nmd, "coded stream: audio names mode %" PRIu64, (u64) md);
  tc_prevmode = md;
  tc_blk = oc_v->cur->blockflag[md];
  /*  tc_audio set tc_prevW to this packet's block flag after the window
      flags were coded against the old one.  Without window flags to wait
      for, it becomes this packet's now.  */
  oc_nextW = tc_blk;
  if (!tc_blk) tc_prevW = 0;
  return md;
}
/*  wprev names the block before this one, which both sides have already
    walked.  A stream that says otherwise is not wrong -- the decoder uses
    what it is told -- so it is coded against that expectation rather than
    derived from it, and disagreeing costs a few bits.  */
static u32 oc_wprev(u32 w) {
  w = (u32) tc_auxc(F_WPREV, (i32) tc_prevW, tc_enc ? (i64) w : 0, 0);
  FATAL_UNLESS(w < 2, "coded stream: aud.wprev is %u", w);
  oc_wp = w;
  return w;
}
static u32 oc_wnext(u32 w) {
  w = (u32) tc_auxc(F_WNEXT, (i32) oc_wp, tc_enc ? (i64) w : 0, 0);
  FATAL_UNLESS(w < 2, "coded stream: aud.wnext is %u", w);
  tc_prevW = oc_nextW;
  return w;
}

/*  ------------------------------------------------------------------
    The page header.

    Three of its fields are not information in the ordinary case: the type
    follows from position, the sequence counts from zero within a link and
    the serial is one number per link.  tsvtrans drops them and refuses a
    stream that disagrees; here they are coded as residuals against the same
    expectations, which costs the same nothing when they hold and keeps the
    tool working when they do not.  */
static i64 pg_prev;
static u32 pg_prevtype, pg_prevser;
static u32 oc_seq;                        /*  pages so far in this link  */

static void oc_page(ogg_page & p, int cont) {
  u32 i;
  i64 g;
  u32 seq = oc_seq++;
  int bos = seq == 0;
  tc_stage = STG_PAGE;

  /*  the type against what position implies  */
  { u32 want = (u32) ((bos ? 2 : 0) | (cont ? 1 : 0));
    i64 d = tc_auxc(F_PGTYPE, (i32) (want * 4 + (pg_prevtype & 3)),
                    tc_enc ? (i64) p.type - (i64) want : 0, 1);
    if (!tc_enc) {
      FATAL_UNLESS(d + (i64) want >= 0 && d + (i64) want < 0x100,
                   "coded stream: page type %" PRId64, d + (i64) want);
      p.type = (u8) (d + (i64) want);
    }
    pg_prevtype = p.type; }

  g = tc_enc ? (i64) (((u64) p.ghi << 32) | p.glo) : 0;
  /*  tc_auxc supplies the field's own last value as the second axis, so
      passing it again as the first gave F_GRAN one axis twice over.  What the
      other fields get there is the value before that, which is what tc_aux
      passes; a granule difference is worth predicting from the one before it.  */
  g = tc_auxc(F_GRAN, tc_qlog(tc_last2[F_GRAN]), tc_enc ? g - pg_prev : 0, 1) + pg_prev;
  pg_prev = g;
  if (!tc_enc) { p.glo = (u32) ((u64) g & 0xFFFFFFFFu);  p.ghi = (u32) ((u64) g >> 32); }

  { i64 d = tc_auxc(F_SERIAL, bos, tc_enc ? (i64) p.serial - (i64) pg_prevser : 0, 1);
    if (!tc_enc) p.serial = (u32) ((i64) pg_prevser + d);
    pg_prevser = p.serial; }

  { i64 d = tc_auxc(F_SEQ, 0, tc_enc ? (i64) p.seq - (i64) seq : 0, 1);
    if (!tc_enc) p.seq = (u32) ((i64) seq + d); }

  { i64 np = tc_aux(F_NPKT, tc_enc ? (i64) p.np : 0);
    FATAL_UNLESS(np >= 0 && np <= OGG_MAXSEG, "coded stream: page holds %" PRId64 " packets",
                 np);
    p.np = (int) np; }
  /*  A packet's length against the one before it.  The two axes F_PLEN had
      were both the previous length, one of them log-quantised twice; what it
      wanted was the difference, which within a page is small.  */
  { i64 prev = 0;
    for (i = 0; i < (u32) p.np; i++) {
      i64 v = tc_auxc(F_PLEN, tc_qlog(prev),
                      tc_enc ? (i64) p.plen[i] - prev : 0, 1) + prev;
      FATAL_UNLESS(v >= 0 && v <= (i64) OGG_MAXSEG * OGG_MAXSEG,
                   "coded stream: packet length %" PRId64, v);
      p.plen[i] = (u32) v;
      prev = v;
    } }
  if (p.np && !(p.plen[p.np - 1] % OGG_MAXSEG)) {
    i64 v = tc_aux(F_TAIL, tc_enc ? (i64) p.tail : 0);
    FATAL_UNLESS(v >= 0 && v < 2, "coded stream: page tail %" PRId64, v);
    p.tail = (int) v;
  } else p.tail = 0;
  /*  The decoder's page is rebuilt from its fields; what the encoder saw was
      parsed from bytes and needs nothing more.  */
  if (!tc_enc) p.pack();
}

/*  The link.  What tc_walk reset at the top of its link loop.  */
static u32 oc_link(u32 more) {
  i64 m;
  tc_stage = STG_PAGE;
  m = tc_aux(F_MORE, tc_enc ? (i64) more : 0);
  FATAL_UNLESS(m >= 0 && m < 2, "coded stream: link.more is %" PRId64, m);
  return (u32) m;
}
static void oc_link_begin(void) {
  pg_prev = 0;  tc_prevW = 0;  tc_prevmode = 0;  pg_prevtype = 0;
  pg_prevser = 0;  oc_seq = 0;
  memset(tc_used, 0, sizeof tc_used);
}
static u32 oc_spill(u32 n) {
  i64 ex;
  tc_stage = STG_PAGE;
  ex = tc_aux(F_SPILL, tc_enc ? (i64) n : 0);
  FATAL_UNLESS(ex >= 0 && ex <= (i64) MAXPAY * OGG_MAXSEG,
               "coded stream: page spill %" PRId64, ex);
  return (u32) ex;
}

/*  A header record.  The tag sequence is a function of values already coded
    -- both sides run the same reader -- so only the value is coded, under the
    tag as context (tc_hdrval).  */
static u32 oc_hdr(const char * tag, u32 x, u64 hi) {
  i64 v;
  tc_stage = STG_HDR;
  v = tc_hdrval(tc_tagid(tag), tc_enc ? (i64) x : 0);
  if (!tc_enc)
    FATAL_UNLESS(v >= 0 && (u64) v < hi, "coded stream: %s = %" PRId64 " is out of range",
                 tag, v);
  return (u32) v;
}

/*  Optimization mode: what the model's memory costs, in bytes of output.

    IDX/opt.pl's objective is the size of the file it produced and nothing
    else, so every context it can widen is free to it and the widest set
    always wins -- which is how a search arrives at a 38 GB model that codes
    one file 3 kB smaller.  The alternative to a threshold is a price: build
    with -DTC_MEMCOST and the encoder appends one byte of padding per
    TC_MEMCOST-th of a gigabyte the tables occupy, so widening a context has
    to pay for itself against the memory it takes and the search finds the
    trade rather than the corner.  There is no threshold to tune and no set is
    refused outright -- a set that is worth its memory still wins.

    TC_MEMCOST is bytes charged per gigabyte; the default 10000 makes the
    929 MB this ships with cost about 9 kB against a 11.4 MB corpus, which is
    roughly the size of the win a doubling of the model has been worth.  Halve
    it to make memory cheap, raise it to make it dear.

    What is charged is TC_Size, the address space the tables occupy, because
    that is what the .idx decides and what is known before a byte is coded.
    Resident memory is a property of the file as much as of the model -- the
    38 GB set above touches 304 MB -- so it is not something a parameter
    search can be given as an objective.

    This is a tuning build only: the padding is written, never read back, so a
    stream still decodes, but it is not a stream anyone should ship.  ./mk.sh
    and ./mk.sh check leave it off, so the two builds still agree.  */
#ifdef TC_MEMCOST
  /*  `-DTC_MEMCOST` is `-DTC_MEMCOST=1` to the compiler, and one byte per
      gigabyte rounds to nothing for any model worth charging for -- so the
      bare form means the default rather than silently costing zero.  */
  #if TC_MEMCOST + 0 <= 1
    #undef TC_MEMCOST
    #define TC_MEMCOST 10000
  #endif
#endif

/*  Hand a family the tables the generator sized for it and the rates the
    .idx set.  Every name here is generated: `TC_dig_A` is the Table() line in
    IDX/tsvcomp-dig.inc, `TC_dig_a_Volume` the product of the factor sizes in
    IDX/tsvcomp-*.idx, `TC_dig_rA` its Number.  */
/*  Two of the six families do not have every component, so the wiring says
    which parameters they are missing rather than IDX/tsvcomp-*.idx carrying a
    knob that cannot move:  fam_dig has no sign counter -- a digit's sign is
    fam_sgn's whole job -- and fam_sgn has neither that nor a mantissa, since
    it only ever calls bit() and a sign is one bit.  The four that code a
    signed value with a mantissa name all of it, through TC_WIRE_ALL.  */
#define TC_WIRE(f, F, g, ng, rt, mwt, rg, mwg, lrmt, mbmt)                    \
  (f).wire(tcm_##F->TC_##F##_A, TC_##F##_a_Volume,                                 \
           tcm_##F->TC_##F##_B, TC_##F##_b_Volume,                                 \
           tcm_##F->TC_##F##_C, TC_##F##_c_Volume,                                 \
           tcm_##F->TC_##F##_D, TC_##F##_d_Volume,                                 \
           tcm_##F->TC_##F##_MT, TC_##F##_t_Volume,                                 \
           tcm_##F->TC_##F##_S, tcm_##F->TC_##F##_SC, TC_##F##_s_Volume,                \
           tcm_##F->TC_##F##_F, tcm_##F->TC_##F##_FC, TC_##F##_f_Volume,                \
           tcm_##F->TC_##F##_W, TC_##F##_m_Volume, (g), (ng), TC_##F##_g_Volume,  \
           tcm_##F->TC_##F##_WM, TC_##F##_n_Volume,                                \
           tcm_##F->TC_##F##_WC, tcm_##F->TC_##F##_WMC,                                 \
           TC_##F##_rA, TC_##F##_rB, TC_##F##_rC, TC_##F##_rD,                \
           (rt), (rg),                                                        \
           TC_##F##_mwA, TC_##F##_mwB, TC_##F##_mwC, TC_##F##_mwD,            \
           (mwt), (mwg),                                                      \
           TC_##F##_rS1, TC_##F##_rS2,                                        \
           TC_##F##_lr, TC_##F##_mb, (lrmt), (mbmt),                          \
           TC_##F##_bw, TC_##F##_qs, TC_##F##_qf,                             \
           TC_##F##_ND, TC_##F##_MND)

#define TC_WIRE_ALL(f, F, g, ng)                                              \
  TC_WIRE(f, F, g, ng, TC_##F##_rT, TC_##F##_mwT, TC_##F##_rG, TC_##F##_mwG,  \
          TC_##F##_lrm, TC_##F##_mbm)

/*  What the model may ask for in tables -- address space, not memory, since
    tc_map hands back a reservation and the pages arrive as the coder reaches
    them.  A tuned set really does ask for tens of gigabytes and really does
    code in a few hundred megabytes; the cap is a sanity bound on a search
    that has no way of knowing how far it has wandered, not a statement about
    what the machine has.  Which is also why it is not the thing keeping
    IDX/opt.pl honest: a set whose *touched* pages exceed the machine still
    ends under the OOM killer, a signal nothing can catch, and what survives
    that is opt.pl reading the exit status rather than the size.

    Override at build time: -DTC_MEMCAP='((u64) 256 << 30)'.  */
#ifndef TC_MEMCAP
  #define TC_MEMCAP ((u64) 64 << 30)
#endif
/*  What the six modules ask for between them.  A constant expression in the
    shipping build, where every Volume is folded, and a sum of load-time values
    in the tuning build, where they are read from the mapping objects.  */
static INLINE u64 tc_tables(void) {
  return (u64) tcm_dig->TC_dig_Size + (u64) tcm_sgn->TC_sgn_Size
       + (u64) tcm_flr->TC_flr_Size + (u64) tcm_cls->TC_cls_Size
       + (u64) tcm_aux->TC_aux_Size + (u64) tcm_hdr->TC_hdr_Size;
}
#if !USE_NEW
/*  The shipping build folds every size, so an .idx too large for the cap is a
    build error rather than something to find out at run time.  */
static_assert(TC_dig_T::TC_dig_Size + TC_sgn_T::TC_sgn_Size
            + TC_flr_T::TC_flr_Size + TC_cls_T::TC_cls_Size
            + TC_aux_T::TC_aux_Size + TC_hdr_T::TC_hdr_Size <= TC_MEMCAP,
              "IDX/tsvcomp-*.idx ask for more model tables than TC_MEMCAP allows");
#endif

static void tc_models(void) {
  cm_tables();
  /*  In the shipping build this object is the tables -- tens of gigabytes of
      them, at fixed offsets -- so it is mapped rather than declared static:
      a .bss that size needs -mcmodel=large to address and, on Linux, is
      refused outright by the default overcommit heuristic, which kills the
      program before main() with no message at all.  A mapping is zero, which
      is what the loader would have given it.  */
  #define TC_MAP(F) (tcm_##F = (TC_##F##_T *) tc_map(sizeof(TC_##F##_T)),      \
                     tcm_##F->TC_##F##_Init())
  TC_MAP(dig);  TC_MAP(sgn);  TC_MAP(flr);
  TC_MAP(cls);  TC_MAP(aux);  TC_MAP(hdr);
  #undef TC_MAP
  FATAL_UNLESS(tc_tables() <= TC_MEMCAP,
               "IDX/tsvcomp-*.idx asks for %" PRIu64 " MB of model tables, past "
               "the %" PRIu64 " MB this build allows -- narrow a context, or "
               "rebuild with a larger TC_MEMCAP",
               (u64) (tc_tables() >> 20), (u64) (TC_MEMCAP >> 20));
  TC_WIRE(fam_dig, dig, (cm_cnt *) nullptr, 0,      /*  no sign counter  */
          TC_dig_rT, TC_dig_mwT, 0, 0, TC_dig_lrm, TC_dig_mbm);
  TC_WIRE(fam_sgn, sgn, (cm_cnt *) nullptr, 0,      /*  nor a mantissa  */
          0, 0, 0, 0, 0, 0);
  TC_WIRE_ALL(fam_flr, flr, tcm_flr->TC_flr_G, TC_SGN_FLR);
  TC_WIRE(fam_cls, cls, (cm_cnt *) nullptr, 0,       /*  nor a sign  */
          TC_cls_rT, TC_cls_mwT, 0, 0, TC_cls_lrm, TC_cls_mbm);
  TC_WIRE_ALL(fam_aux, aux, tcm_aux->TC_aux_G, TC_SGN_AUX);
  TC_WIRE_ALL(fam_hdr, hdr, tcm_hdr->TC_hdr_G, TC_SGN_HDR);
}

/*  The model tables are mapped and report their own failures; what is left on
    new[] is sh_mapping.inc's threshold tables, which are tiny.  Still worth a
    handler: bad_alloc unwinds to terminate, terminate aborts, and an abort
    skips the unlink and leaves a partial output behind -- the very thing that
    misleads a size-driven search.  There is nothing to fall back to, so the
    failure is reported and the file goes.  */
static void tc_nomem(void) {
  FATAL_CODE(BLR_EXIT_IO, "out of memory -- IDX/tsvcomp-*.idx asks for more "
             "than this machine has");
}

/*  ------------------------------------------------------------------
    The sink itself: oc_sink.inc's contract, over the functions above.  */
struct oc_model {
  u32 link(u32 more) { return oc_link(more); }
  void link_begin() { oc_link_begin(); }
  void page(ogg_page & p, int cont) { oc_page(p, cont); }
  u32 spill(u32 n) { return oc_spill(n); }
  u32 hdr(const char * tag, u32 x, u64 hi) { return oc_hdr(tag, x, hi); }
  void setup_done(vb_ctx & v) { oc_v = &v;  tc_setup_done(); }
  u32 mode(u32 md, u32 nmd) { return oc_mode(md, nmd); }
  u32 wprev(u32 w) { return oc_wprev(w); }
  u32 wnext(u32 w) { return oc_wnext(w); }
  void floor(u32 k, u32 fno, vb_floor * f, u32 * y, int & used) {
    oc_flr(k, fno, f, y, used);
  }
  void residue_begin(u32 rno, vb_res * r, const u8 * nz, u32 nch, u32 n) {
    oc_residue_begin(rno, r, nz, nch, n);
  }
  u32 cls(u32 rno, u32 j, u32 pc, u32 k, u32 c) { return oc_cls(rno, j, pc, k, c); }
  void part_begin(u32 rno, u32 pass, u32 j, u32 pc, vb_res * r, vb_book * bk,
                  u32 bn, u32 c) {
    oc_part_begin(rno, pass, j, pc, r, bk, bn, c);
  }
  /*  A vector's digits are the places of one codebook entry, so the prior
      restarts with the vector and walks down one level per digit.  */
  void vec_begin() { fam_dig.pc.start(P.pt); }
  i64 digit(i64 d) { return oc_digit(d); }
};
