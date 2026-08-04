
#define PAQ_PREFETCH
#define PAQ_AVX2
#define PAQ_DOT2
#define PAQ_GETSIMD
#define PAQ_APMPF

#define PAQ_RESTRICT
// #define PAQ_PF2
// #define PAQ_RCMPF
#define PAQ_N16
#define PAQ_WXPF
#define PAQ_CXTFL_T


//#define PAQ_DECFAST
#define FIXED_LEVEL 7

#define PAQ_HOT
#define PAQ_LOGISTIC



#ifndef NDEBUG
#define NDEBUG
#endif

#ifndef DEFAULT_OPTION
#define DEFAULT_OPTION 8
#endif

#ifndef NOASM
#define NOASM
#endif

// ---------------------------------------------------------------------------
// Speed gates.  A build with none of these defined must be byte-for-byte the
// baseline paq8hp.hpp; every gate below is output-preserving on its own and in
// any combination.  See paq8hpc_speed_results.md for what each one measured.
//
//   PAQ_PREFETCH   ContextMap hashed-probe prefetch                      (3)
//   PAQ_APMPF      APM context hoist + line prefetch                     (3)
//   PAQ_PF2        speculative two-target probe prefetch at bpos 1/4     (3)
//   PAQ_RCMPF      hoist rcm/cm prefetch above the RunContextMap walks   (3)
//   PAQ_AVX2       256-bit dot_product/train (loadu, 128-bit tail)       (4)
//   PAQ_AVX512     512-bit dot_product/train                            (4)
//   PAQ_N16        raise Mixer::N to (n+15)&-16 so aligned loads are legal
//   PAQ_DOT2       two weight rows per dot_product/train pass           (10)
//   PAQ_WXPF       mixer weight-row prefetch in Mixer::set()             (7)
//   PAQ_CXTFL_T    template mix1/mix2 on cxtfl instead of the global    (10)
//   PAQ_GETSIMD    SIMD checksum compare in ContextMap::E::get()        (10)
//   PAQ_RESTRICT   __restrict on the SIMD kernel pointers               (10)
//
// Driver-side gates (PAQ_HUGEPAGES, PAQ_LAZY_ZERO, PAQ_DECFAST, FIXED_LEVEL)
// live in paq8hpc_speed.cpp.
// ---------------------------------------------------------------------------

// __builtin_prefetch(p,1) asks for the line in anticipation of a write, which
// is what these probes do (E::get writes chk/last/bh on a miss).  MSVC has no
// write hint; _MM_HINT_T0 is the closest.
#if defined(__GNUC__) || defined(__clang__)
  #define PAQ_PF(p) __builtin_prefetch((const void*)(p),1)
#elif defined(_MSC_VER)
  #include <intrin.h>
  #define PAQ_PF(p) _mm_prefetch((const char*)(p),_MM_HINT_T0)
#else
  #define PAQ_PF(p) ((void)0)
#endif

#if defined(PAQ_RESTRICT) && (defined(__GNUC__)||defined(__clang__)||defined(_MSC_VER))
  #define PAQ_RS __restrict
#else
  #define PAQ_RS
#endif

// The doc's section 10 item 4 lists `hot` alongside __restrict; only __restrict
// was tested there.  This marks the functions the instrumented counts in the
// doc's section 2 identify as the hot tier, which biases block layout and lets
// gcc/clang group them away from the cold ctor/table-init code.
#if defined(PAQ_HOT) && (defined(__GNUC__)||defined(__clang__))
  #define PAQ_HOTFN __attribute__((hot))
#else
  #define PAQ_HOTFN
#endif

//
// !! Do NOT hoist PAQ_BP into a local variable. !!
//
// It expands to a fresh read of the global `bpos` when BP<0, which is exactly
// what the baseline does, and that is load-bearing under gcc.  Caching it in a
// `const int` makes it provably loop-invariant, which enables gcc's loop
// unswitching on the four bpos tests in the loop below - and gcc 13.3 -O3
// generates WRONG CODE for the unswitched loop: book1[0:65536] at L0 goes
// 23173 -> 32656 bytes, diverging from bit 16 on.  It is a compiler bug, not a
// latent bug here: -O0/-O1/-O2 all agree, -fno-unswitch-loops at -O3 agrees,
// and clang 18 -O3 agrees on both forms.  Reading the global directly keeps the
// U8* stores in the loop as potential aliases of it, so gcc cannot hoist it and
// cannot unswitch.  With PAQ_BPOS_T the tests fold at compile time instead, so
// there is nothing left to unswitch either way.
#define PAQ_BP (BP<0 ? bpos : BP)

#if defined(PAQ_GETSIMD)
  #include <emmintrin.h>
  #if defined(__GNUC__) || defined(__clang__)
    #define PAQ_CTZ(x) __builtin_ctz(x)
  #else
    #include <intrin.h>
    static inline int PAQ_CTZ(unsigned x) { unsigned long r; _BitScanForward(&r,x); return int(r); }
  #endif
#endif

namespace paq8hp {

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;

// ---------------------------------------------------------------------------
// Named scales.  These were spelled as literals throughout, which hid the fact
// that the same numeral means different things in different places: 256 is the
// logistic scale in squash/stretch, the state count in StateMap, and the byte
// alphabet in the partial-byte arithmetic; 4096 is the probability scale in the
// coder but a page size in the allocator; 2048 is neutral probability in the
// APM but a mixer context count in ContextModel::p.  Only the probability and
// logistic families are named here -- the byte-alphabet and tuning literals are
// deliberately left alone, since a name that conflated them would be worse than
// the number.  constexpr rather than enum so each carries a real type.
//
// A prediction is a probability p in [0, P_SCALE) at P_BITS of precision; that
// is the coder's totFreq.  The mixer works in the logistic domain, where
// stretch(p) = ST_SCALE*ln(p/(P_SCALE-p)) clamped to [ST_MIN, ST_MAX], and
// squash is its inverse.  StateMap and APM hold probabilities at PR_BITS (U16)
// internally and shift by PR_SHIFT to trade with the P_BITS world.
// ---------------------------------------------------------------------------

#ifndef PAQ_P_BITS
  #define PAQ_P_BITS 12
#endif
#ifndef PAQ_ST_SCALE
  #define PAQ_ST_SCALE 256
#endif

constexpr int P_BITS  = PAQ_P_BITS;       // precision of a prediction
constexpr int P_SCALE = 1<<P_BITS;        // 4096, the coder's totFreq
constexpr int P_MAX   = P_SCALE-1;        // 4095, largest codable p
constexpr int P_MIN   = 1;                // smallest codable p; p==0 is undecodable
constexpr int P_HALF  = P_SCALE/2;        // 2048, "no information"

constexpr int ilog2c(int v) { return v<=1 ? 0 : 1+ilog2c(v>>1); }

// The logistic domain belongs to ST_SCALE, NOT to P_BITS.  stretch(p) =
// ST_SCALE*ln(p/(P_SCALE-p)) reaches +-ST_SCALE*ln(P_MAX) ~ +-8*ST_SCALE nats, so
// the clamp and the squash table are sized from ST_SCALE.  Tying them to P_HALF
// instead (which is what ST_MAX = P_HALF-1 did) couples two independent things and
// breaks both ends of a P_BITS sweep: below 12 the clamp eats the range (54% of it
// at P_BITS=9), above 12 the APM's 33-node grid spreads over +-P_HALF while
// stretch still only reaches +-2839, leaving 4 of 33 nodes reachable at P_BITS=16.
// Sized from ST_SCALE it is 2048 either way, identical to the shipped value.
constexpr int ST_SCALE = PAQ_ST_SCALE;    // stretch's logistic scale
constexpr int ST_BITS  = ilog2c(ST_SCALE);           // 8
constexpr int ST_LIMIT = 1<<(ST_BITS+3);  // 2048, i.e. +-8 nats of log-odds
constexpr int ST_MAX   = ST_LIMIT-1;      // 2047
constexpr int ST_MIN   = -ST_MAX;         // -2047

static_assert( (1<<ST_BITS)==ST_SCALE, "ST_SCALE must be a power of two" );

// squash interpolates the logistic on a SQ_STEP-wide grid of SQ_NODES points.
// APM used to reuse this grid; it has its own now, below.
//
// SQ_BITS is DERIVED from ST_SCALE, not fixed at 7 and not from P_BITS: the grid
// covers the LOGISTIC range [-ST_LIMIT,ST_LIMIT) in SQ_NODES points, so its step
// is a property of the logistic domain.  SQ_STEP = ST_LIMIT/16 = ST_SCALE/2.
constexpr int SQ_NODES = 33;              // interpolation nodes over the range
constexpr int SQ_BITS  = ST_BITS-1;       // 7 at ST_SCALE==256
constexpr int SQ_STEP  = 1<<SQ_BITS;      // 128
constexpr int SQ_MASK  = SQ_STEP-1;       // 127
constexpr int SQ_MID   = SQ_NODES/2;      // 16, the node at d==0

static_assert( 2*ST_LIMIT/SQ_STEP + 1 == SQ_NODES,
  "SQ_STEP must divide the logistic range into SQ_NODES-1 intervals" );
static_assert( SQ_BITS>=1, "ST_SCALE must be at least 4 for the squash grid" );

// APM used to reuse the squash grid outright -- 33 nodes at SQ_STEP spacing --
// which welded the shape of an adaptive table to the shape of a fixed function
// approximation.  They are unrelated: squash's grid is fixed by how well 33
// points approximate a logistic, APM's by how finely a correction table should
// partition its input, and the second is a tuning question.
//
// So APM now carries its own per-instance node count and addresses the stretch
// range as a fixed-point fraction of it, independent of the grid: the interval
// is (s*NIV)>>ST_RANGE_BITS and the remainder is the interpolation weight.  At
// NIV==32 that is exactly the old (s>>SQ_BITS, s&SQ_MASK) pair scaled by 32.
constexpr int ST_RANGE      = 2*ST_LIMIT;         // 4096, the stretch span
constexpr int ST_RANGE_BITS = ilog2c(ST_RANGE);   // 12
constexpr int APM_NH_MAX    = 32;                 // ceiling on the half-node count
constexpr int APM_CELLS_MAX = 2*APM_NH_MAX+1;     // 65

static_assert( (1<<ST_RANGE_BITS)==ST_RANGE, "ST_RANGE must be a power of two" );

constexpr int PR_BITS  = 16;              // StateMap/APM internal precision
constexpr int PR_ONE   = 1<<PR_BITS;      // 65536
constexpr int PR_MAX   = PR_ONE-1;        // 65535
constexpr int PR_HALF  = PR_ONE/2;        // 32768, "no information" at PR_BITS
constexpr int PR_SHIFT = PR_BITS-P_BITS;  // 4, PR_BITS <-> P_BITS

constexpr int N_STATES = 256;             // rows of State_table / StateMap::t

// ---- the adaptation-rate fixed point ------------------------------------
// Every adaptive counter moves a fraction of the way to its target.  That
// fraction used to be a shift, which meant the only rates expressible were the
// powers of two -- 1/128, 1/256, 1/512 and nothing between them.  It is a
// 16-bit multiplier now: step = (target-v)*mul/EMA_SCALE, with
// mul = EMA_SCALE>>shift reproducing any old shift exactly.
constexpr int EMA_BITS  = 16;             // fixed-point fraction of the rate
constexpr int EMA_SCALE = 1<<EMA_BITS;    // 65536 == a full step to the target
constexpr int EMA_MUL_MAX = EMA_SCALE/4;  // ceiling the .idx patterns can spell

// The two rounding rules the callers use, as biases on the product rather than
// as fudge terms folded into the target.  That is the change that makes the
// multiplier form work: the old `+ (1<<rate)-1` and `+ (1<<(rate-1))` are
// functions of the SHIFT, so keeping them would have meant an EMA_SCALE/mul
// division per call -- six per bit in APM alone.  As product biases they are
// constants, and they say what they mean.
constexpr int EMA_CEIL = EMA_SCALE-1;     // round away from v: never stall short
constexpr int EMA_HALF = EMA_SCALE/2;     // round to nearest, halves downward

// APM's interpolation sums two U16 entries weighted to ST_RANGE, so the widest
// product has to stay inside int.  Declared here rather than beside ST_RANGE
// because it is the one constraint that ties the two precisions together.
static_assert( PR_MAX*(long long)ST_RANGE*2 < 2147483647LL,
  "APM interpolation would overflow int at this PR_BITS/ST_RANGE" );

// ---------------------------------------------------------------------------
// The IDX bridge, part one: everything the generated headers need.
//
// IDX-FORMAT.md section 12 -- the include order is a cycle unless the prelude is
// split in two.  MOD/paq8-G0_h.inc computes its values from P_SCALE and clamps
// them with pclamp, so those have to exist above it; IDXP/IDXC are defined from
// USE_NEW, which only the generated header defines, so they have to come below.
// The two halves are not merge-able, which is the whole reason this comment is
// here rather than one #include further down.
//
// Note what did NOT move into IDX: P_BITS/P_SCALE/PR_BITS/ST_SCALE and their
// derivatives, above.  Those are the arithmetic's resolution -- fixed by the
// rangecoder's totFreq and by the width the counters are stored at -- not a
// tuning choice, and the static_asserts further down say what they may be.  IDX
// holds what an optimizer is allowed to move; that is a different set.
// ---------------------------------------------------------------------------

// A pattern in the .idx is a search space and opt.pl WILL visit the ends of it,
// so every value is range-checked at the point of use (section 5).  constexpr
// because the shipping build folds the whole chain to a literal.
constexpr int pclamp(int v, int lo, int hi) { return v<lo ? lo : v>hi ? hi : v; }

#define PEMA(x)   pclamp((x), 1, EMA_MUL_MAX) // an EMA_SCALE-unit adaptation rate
#define PPOS(x)   pclamp((x), 1, 0x7fffffff)  // an input position, in bytes
#define PWGT(x)   pclamp((x), 1, P_SCALE-1)   // a P_SCALE-unit mixing weight
#define PSMALL(x) pclamp((x), 1, 255)         // a small positive multiplier/cap

// A dimensionless ratio in P_SCALE units: PMUL(P_SCALE) is unity.  Where PWGT
// above is a fraction OF P_SCALE and so cannot reach it, this one is a
// multiplier BY it and is meant to pass it -- the seeded learning rate is 7.
// These replace what used to be _NUM/_DEN parameter pairs.
//
// The ceiling is what the .idx's twelve-bit patterns can already spell, so it
// is the contract of section 5 rather than a constraint that bites; what makes
// the whole declared range safe is that both consumers form the product at 64
// bits, which they must, because P_SCALE*PMUL_MAX is past int at every P_BITS.
constexpr int PMUL_MAX = 16*P_SCALE;
#define PMUL(x)   pclamp((x), 0, PMUL_MAX)   // a P_SCALE-unit ratio

// Assemble a 32-bit mask over a byte-per-byte shift register (x4, c4) from the
// four byte lanes IDX declares it as, most significant first.  IDX cannot spell
// a full-width mask -- mapping::value accumulates the pattern into a signed int
// and thirty-two bits overflow it -- and a lane is the better knob anyway,
// since it is the register's own field: opt.pl moves a byte of history rather
// than an arbitrary bit of one.  The & at the call site is unchanged.
#define LANES4(m3, m2, m1, m0) \
  ( (U32((m3)&255)<<24) | (U32((m2)&255)<<16) | (U32((m1)&255)<<8) | U32((m0)&255) )
#define LANES2(m1, m0)  ( (int((m1)&255)<<8) | int((m0)&255) )

// An IDX-declared shift amount.  Section 5's rule with no exceptions: a shift
// is one of the three things a swept pattern can turn into undefined behaviour,
// so every one of them is clamped where it is used rather than where it is
// declared.  32 is the width of everything shifted here.
#define PSH(x)    pclamp((x), 0, 31)

// sh_mapping.inc is the IDX runtime: mapping/masking/masking_b plus the
// pdesc/pmask/pmask2 macros that embed a pattern in the executable for opt.pl,
// and the mdesc/mmask/mmask2 that deliberately do not.  It needs NOINLINE and
// <string.h>; the driver includes <string.h> at global scope before this header,
// so the copy inside sh_mapping.inc is a no-op against its own include guard and
// nothing from libc lands in namespace paq8hp.
#ifndef NOINLINE
  #if defined(__GNUC__) || defined(__clang__)
    #define NOINLINE __attribute__((noinline))
  #else
    #define NOINLINE __declspec(noinline)
  #endif
#endif
#include "sh_mapping.inc"

#include "MOD/paq8-G0_h.inc"
#include "MOD/paq8-G0_p.inc"

// The model context sets.  G0 above holds the coder's numbers -- rates, mixing
// weights, mixer selectors; these hold the masks, multipliers, tags and
// compared-against bytes that decide what each model's contexts SEE.  One
// module per model, because the models are what a reader wants to look at one
// at a time, and because opt.pl's search space is easier to reason about when a
// module is a model.
//
// Each _h.inc restates USE_NEW; they agree because mk.sh generates every .idx
// in the tree with the same mode, which is the same coupling section 7
// describes for one module.
#include "MOD/paq8-S0_h.inc"
#include "MOD/paq8-S0_p.inc"
#include "MOD/paq8-R0_h.inc"
#include "MOD/paq8-R0_p.inc"
#include "MOD/paq8-W0_h.inc"
#include "MOD/paq8-W0_p.inc"
#include "MOD/paq8-C0_h.inc"
#include "MOD/paq8-C0_p.inc"
#include "MOD/paq8-T0_h.inc"
#include "MOD/paq8-T0_p.inc"
#include "MOD/paq8-T1_h.inc"
#include "MOD/paq8-T1_p.inc"
#include "MOD/paq8-N0_h.inc"
#include "MOD/paq8-N0_p.inc"

// ---------------------------------------------------------------------------
// The IDX bridge, part two: now that USE_NEW exists.
//
// In the shipping build every G0_* is a literal and can be a template argument
// like any other integer.  In the tuning build it is a read from a patchable
// object, which is not a constant expression -- so a non-type template parameter
// of REFERENCE type is what lets Mixer take its initial weight from IDX and
// still be spelled identically in both builds (section 8).  const int at
// namespace scope has internal linkage and static storage duration, which is
// exactly what const int& binds to.
// ---------------------------------------------------------------------------
#if USE_NEW
  #define IDXP(name)       const int& name
  #define IDXC(t, n, expr) const t n = (expr)
#else
  #define IDXP(name)       int name
  #define IDXC(t, n, expr) static constexpr t n = (expr)
#endif

// Instantiating on a static const int gives the instantiated type internal
// linkage, and GCC warns when a class then has a member of such a type.  The
// warning is right in general -- several translation units would each get a
// different type under one name -- and vacuous here, because this program is
// one translation unit.
#if defined(__GNUC__) && !defined(__clang__)
  #pragma GCC diagnostic ignored "-Wsubobject-linkage"
#endif

// The one-sided failure section 8 describes: if MOD/ says USE_NEW=0 while
// actually holding tuning-mode mapping objects, IDXP declares plain int
// parameters and the arguments below are not constant expressions.  The other
// mistake (literals bound to reference parameters) compiles and runs correctly,
// merely without folding, so one assertion catches the direction that matters.
#if !USE_NEW
static_assert( G0_MIX_W_INIT>=0 && G0_MIX2_W_INIT>=0 && G0_MX0_Volume>0,
  "MOD/ declares USE_NEW=0 but its parameters are not constant expressions -- "
  "regenerate it with ./mk.sh release" );
#endif

// ---- adaptation rates and schedules -------------------------------------
// Every adaptive counter here updates as `v += (target-v)>>rate`, which is a
// linear mix of the stored value and the target with weight 2^-rate.  So the
// rate IS the weight, written in log form -- the same uniformity pmix() gives
// the fixed-weight mixes, except that a shift is the natural spelling when the
// weight is a power of two and the code sits in the hot path.  ema() below is
// that one shape; each caller supplies a target with its rounding already
// folded in, which is what lets all three users share it.
//
// Rates are plain const, like the mixing weights, and each position schedule is
// named next to the rates it switches between, so a rate and the point it
// changes at are visible together.  Every one of them now comes from
// IDX/paq8-G0.idx through a clamp; the names and the uses are unchanged.

// StateMap: one shared rate, slowed twice as the counts settle.
const int SM_MUL_0  = PEMA(G0_SM_MUL_0);   // 512/65536 == 1/128 up to SM_SWITCH_1
const int SM_MUL_1  = PEMA(G0_SM_MUL_1);   // 256/65536 == 1/256
const int SM_MUL_2  = PEMA(G0_SM_MUL_2);   // 128/65536 == 1/512 from SM_SWITCH_2 on
const int SM_SWITCH_1 = PPOS(G0_SM_SW1);
const int SM_SWITCH_2 = PPOS(G0_SM_SW2);

// APM: six instances, six schedules.  These used to be one shared `rate`
// computed once per bit and handed to five of the six, which meant opt.pl could
// only ever move all five together -- and they are not one adaptive unit: they
// sit at different points of the correction chain and index on contexts from
// 256 to 131072 rows wide.  Seeds reproduce the shared arithmetic exactly
// (A2_MUL is half the others because a2's rate was base+1).  a1 keeps one rate
// because it never had a schedule to clone.
//
// PAPN clamps the half-node count: a pattern is a search space, and this one
// multiplies a table that is 131072 rows wide in a4's case.
#define PAPN(x) pclamp((x), 1, APM_NH_MAX)

const int A1_MUL  = PEMA(G0_A1_MUL);
const int A1_NH   = PAPN(G0_A1_NH);

const int A2_MUL  = PEMA(G0_A2_MUL);
const int A2_SW1  = PPOS(G0_A2_SW1);
const int A2_SW2  = PPOS(G0_A2_SW2);
const int A2_NH   = PAPN(G0_A2_NH);

const int A3_MUL  = PEMA(G0_A3_MUL);
const int A3_SW1  = PPOS(G0_A3_SW1);
const int A3_SW2  = PPOS(G0_A3_SW2);
const int A3_NH   = PAPN(G0_A3_NH);

const int A4_MUL  = PEMA(G0_A4_MUL);
const int A4_SW1  = PPOS(G0_A4_SW1);
const int A4_SW2  = PPOS(G0_A4_SW2);
const int A4_NH   = PAPN(G0_A4_NH);

const int A5_MUL  = PEMA(G0_A5_MUL);
const int A5_SW1  = PPOS(G0_A5_SW1);
const int A5_SW2  = PPOS(G0_A5_SW2);
const int A5_NH   = PAPN(G0_A5_NH);

const int A6_MUL  = PEMA(G0_A6_MUL);
const int A6_SW1  = PPOS(G0_A6_SW1);
const int A6_SW2  = PPOS(G0_A6_SW2);
const int A6_NH   = PAPN(G0_A6_NH);

// SmallStationaryContextMap: one step slower once the counts have settled.
const int SCM_MUL_EARLY = PEMA(G0_SCM_MUL_EARLY);
const int SCM_MUL_LATE  = PEMA(G0_SCM_MUL_LATE);
const int SCM_SWITCH_POS = PPOS(G0_SCM_SW);

// Mixer: the prediction error is scaled by the learning rate before train()
// applies it to the weights, and each context's dot product is scaled again on
// the way into squash().
//
// One P_SCALE-unit multiplier each, not the _NUM/_DEN pair these were.  Two
// integers spelling one ratio gave opt.pl a search space whose points were
// mostly duplicates of each other -- 7/1, 14/2 and 28/4 are the same rate --
// and a redundant coordinate is search time spent learning that it is
// redundant.  P_SCALE is the fixed-point denominator, so MIX_LR_MUL==P_SCALE is
// a rate of 1; the seeds spell the old 7/1 and 2/4 exactly.
const int MIX_LR_MUL  = PMUL(G0_MIX_LR_MUL);   // outer mixer
const int MIX2_LR_MUL = PMUL(G0_MIX2_LR_MUL);  // the nested submixer
// initial weight.  These two are template arguments to Mixer, which is what
// IDXP() above exists for.
const int MIX_W_INIT  = pclamp(G0_MIX_W_INIT, -32768, 32767);   // 2.0 in the 8-bit weight fraction
const int MIX2_W_INIT = pclamp(G0_MIX2_W_INIT, -32768, 32767);  // submixer starts saturated
// Logistic-domain output scalers.  Two mixers, two sets: MIX_DP_* is the main
// mixer's per-context scaling in the S>1 path, MIX2_* the submixer's in the
// S==1 path.  The MIX2_ prefix is load-bearing -- these three execute only
// inside the submixer's p(), and the old MIX_ names read as if both mixers used
// them.
const int MIX_DP_MUL    = PSMALL(G0_MIX_DP_MUL);
const int MIX_DP_SHIFT  = pclamp(G0_MIX_DP_SHIFT, 0, 24);
const int MIX2_Z_SHIFT  = pclamp(G0_MIX2_Z_SHIFT, 0, 24);
const int MIX2_ZB_MUL   = PSMALL(G0_MIX2_ZB_MUL);
const int MIX2_ZB_SHIFT = pclamp(G0_MIX2_ZB_SHIFT, 0, 24);
// Per-group rescaling of the ContextMap outputs on the way into the mixer.
// These were three numerators over one shared MIX_MUL_DEN, which is worse than
// redundant: the denominator coupled the three groups, so no group's gain could
// be moved without moving the other two.  As P_SCALE-unit multipliers they are
// three independent knobs, and 256/256 and 448/256 spell the old 4/4 and 7/4.
const int MIX_MUL_0 = PMUL(G0_MIX_MUL_0);
const int MIX_MUL_1 = PMUL(G0_MIX_MUL_1);
const int MIX_MUL_2 = PMUL(G0_MIX_MUL_2);

// The bit-history decay is per ContextMap instance now: CTR_DFROM/DBIAS/DSHIFT/
// DSTEP in statetable.inc, from IDX/paq8-N0.idx, alongside the state machine
// each one steps through.

// run-length counters saturate rather than wrap
const int RCM_RUN_MAX = PSMALL(G0_RCM_RUN_MAX);   // RunContextMap, steps by 1
const int CM_RUN_MAX  = PSMALL(G0_CM_RUN_MAX);    // ContextMap, steps by CM_RUN_STEP
const int CM_RUN_STEP = PSMALL(G0_CM_RUN_STEP);

// StateMap's initial estimate is a Laplace prior over the state's (n0,n1)
const int SM_PRIOR_ADD = pclamp(G0_SM_PRIOR_ADD, 0, 255);


// v += (target-v)*mul/EMA_SCALE: mix v toward target with weight mul/EMA_SCALE.
// `bias` is the rounding, applied to the PRODUCT -- EMA_CEIL to round away from
// v, EMA_HALF to round to nearest, 0 to floor.  The shift is the division: it
// is arithmetic, so it floors, and floor((d*mul + bias)/EMA_SCALE) with
// mul = EMA_SCALE>>r and bias = (1<<r)-1 scaled up is bit-for-bit the old
// `(target-v)>>r` with its fudge term folded into target.  test_ema.cpp checks
// that over the whole domain rather than asserting it here.
//
// int, not long long: PEMA caps mul at EMA_MUL_MAX and |target-v| cannot exceed
// PR_ONE, so the product is bounded well inside int -- see the static_assert
// below.  That matters because StateMap::p runs this a few hundred times a bit.
inline int ema( int v, int target, int mul, int bias ) {
  return v + (((target-v)*mul + bias)>>EMA_BITS);
}

static_assert( (long long)PR_ONE*EMA_MUL_MAX + EMA_CEIL < 2147483647LL,
  "ema's product must stay inside int: raise EMA_BITS' clamp or widen the "
  "multiply" );

// ---- reinterpreting the fitted constants -------------------------------
// Probability estimation here is fixed-point, so a constant like `*3/64` or a
// threshold like `pr>=1820` is not an opaque weight: it is a coefficient that was
// fitted against a value carrying P_REF_BITS fractional bits.  Change P_BITS and
// the same literal silently means something else -- `(p1-p0)*3/64` doubles its
// mixer-input gain at P_BITS=13, and `pr>=1820` becomes a 0.11 threshold instead
// of 0.44.
//
// So rather than rescaling every literal, bring the VALUE to the reference scale
// and leave the fitted coefficient alone.  p_to_ref is the identity when
// P_BITS == P_REF_BITS, so nothing changes at the shipped precision.
//
// This applies only to P-domain quantities.  Values already in the logistic
// domain (`st/4`, `st*(n1-n0)*3/16`, `st*9/32`, and the mixer's MIX_DP_MUL /
// MIX_DP_SHIFT / MIX_Z_SHIFT / MIX_ZB_MUL / MIX_ZB_SHIFT) are functions of
// ST_SCALE, not of P_BITS, and are NOT rescaled here -- the mixer's five are in
// IDX and the rest are still literals, but either way they follow ST_SCALE.
// The same treatment against an ST reference would make ST_SCALE a real knob,
// but that is entangled with the ST_MAX == P_HALF-1 coupling described in
// paq8hpc_speed_results.md section 12.
constexpr int P_REF_BITS = 12;            // the precision every coefficient was fitted at
constexpr int P_UP = 1<<(P_REF_BITS>P_BITS ? P_REF_BITS-P_BITS : 0);
constexpr int P_DN = 1<<(P_BITS>P_REF_BITS ? P_BITS-P_REF_BITS : 0);

// ---- uniform linear mixing in the probability domain --------------------
// Every weighted average of probabilities is written the same way: weights in
// P_SCALE units summing to P_SCALE, rounding +P_HALF.  So `(a+7*pr+4)>>3` becomes
// pmix(A1_w,a, P_SCALE-A1_w,pr) with A1_w = P_SCALE/8, and the weight reads
// as a fraction instead of having to be decoded from a shift and a handful of
// small integers.  Exactly equal to the shift forms at P_BITS==12 -- each old
// numerator is the new one divided by P_SCALE/2^k -- and now the weights follow
// P_SCALE instead of being welded to it.
//
// long long because one term reaches P_MAX*P_SCALE = 2^32 at P_BITS==16, past
// int.  These run once per coded bit against ~5500 madds, so the width is free.
inline int pmix( int w0, int v0, int w1, int v1 ) {
  return int(( (long long)w0*v0 + (long long)w1*v1 + P_HALF )/P_SCALE);
}

inline int pmix( int w0, int v0, int w1, int v1, int w2, int v2, int w3, int v3 ) {
  return int(( (long long)w0*v0 + (long long)w1*v1
             + (long long)w2*v2 + (long long)w3*v3 + P_HALF )/P_SCALE);
}

// Mixing weights.  Plain const, not constexpr, so they can be retuned here
// without the value being baked into unrelated constant expressions -- and now
// they come from IDX/paq8-G0.idx, declared in units of P_SCALE/256 so the
// arithmetic stays exact at any P_BITS >= 8 and the optimizer gets 1/256
// resolution instead of the 1/32 the original shifts had.
//
// Only three weights of each blend are declared; the fourth is the remainder.
// pmix requires the four to sum to P_SCALE, and a sum constraint the optimizer
// can violate is not a constraint -- nor can a static_assert even be written
// for it in the tuning build, where these are runtime reads.  Making the last
// weight the leftover makes the invariant structural, and it reproduces the
// shipped 14/32 and 11/32 exactly.
//
// a1's correction against the raw model output, 1/8 : 7/8.  Named with the
// instance it belongs to, like a1's rate and node count.
const int A1_w = PWGT(G0_A1_w);
// final blend of the four APM chains, used when fails&255 is set (6:1:11:14 /32)
const int MIXA_pt_w = PWGT(G0_MIXA_pt);
const int MIXA_pu_w = PWGT(G0_MIXA_pu);
const int MIXA_pv_w = PWGT(G0_MIXA_pv);
const int MIXA_pz_w = P_SCALE - MIXA_pt_w - MIXA_pu_w - MIXA_pv_w;
// and when it is clear (4:5:12:11 /32)
const int MIXB_pt_w = PWGT(G0_MIXB_pt);
const int MIXB_pu_w = PWGT(G0_MIXB_pu);
const int MIXB_pv_w = PWGT(G0_MIXB_pv);
const int MIXB_pz_w = P_SCALE - MIXB_pt_w - MIXB_pu_w - MIXB_pv_w;

// a P-domain value at the current precision -> the same value at P_REF_BITS.
// Truncates when P_BITS > P_REF_BITS; prefer the two forms below where possible.
inline int p_to_ref(int v) {
  return v*P_UP/P_DN;
}

// a constant fitted at P_REF_BITS -> the current scale.  Exact, so a threshold
// keeps its full precision instead of truncating the value being compared.
constexpr int p_from_ref(int v) {
  return v*P_DN/P_UP;
}

// v*num/den, evaluated as if v were at the reference scale.  The rescaling is
// folded into the one division the expression already had, so precision is lost
// once rather than twice -- which matters because train()'s _mm256_mulhi_epi16
// hardcodes a >>16, so the mixer error has to arrive pre-scaled and every bit
// dropped on the way is a bit of learning signal gone.
//
// This three-argument form is for the coefficients still spelled as literals in
// the models below -- 3/64, 1/16, 7/64 and the like, which are fitted constants
// rather than knobs.  Everything that comes from IDX uses the form under it.
inline int p_scaled(int v, int num, int den) {
  return v*num*P_UP/(den*P_DN);
}

// The same thing with the ratio arriving as a single P_SCALE-unit multiplier,
// which is what an IDX-declared ratio now is: mul==P_SCALE is unity.  Same
// shape -- one division at the end, so the truncation still happens once -- and
// exactly equal to the pair form wherever num*P_SCALE/den is exact, which is
// what lets the _NUM/_DEN parameters be folded into one without moving a bit of
// the output.
//
// long long because the product reaches P_SCALE*PMUL_MAX at the top of the
// declared range, past int at every P_BITS.  This runs once per mixer context
// per coded bit, against train()'s ~5500 madds, so the width is free -- the
// same trade pmix() above makes for the same reason.
inline int p_scaled(int v, int mul) {
  return int( (long long)v*mul*P_UP / ((long long)P_SCALE*P_DN) );
}

// v*mul/P_SCALE with no reference rescaling, for the logistic-domain ratios.
// Those follow ST_SCALE rather than P_BITS (see the P_REF_BITS note above), so
// P_SCALE appears here only as the fixed-point denominator -- it cancels
// against the P_SCALE/256 unit the .idx declares these in, and the ratio the
// multiplier expresses does not move when P_BITS does.
inline int st_scaled(int v, int mul) {
  return int( (long long)v*mul / P_SCALE );
}

// Predictor's two fail counters: `fails` counts predictions at or above
// FAIL_TH, `failz` the weaker FAILZ_TH, and the pair selects between the two
// APM blends above.  Both are P-domain thresholds fitted at P_REF_BITS, so they
// come out of IDX at that scale and are brought to the current one here rather
// than being declared pre-scaled.
const int FAIL_TH  = p_from_ref(pclamp(G0_FAIL_TH,  1, (1<<P_REF_BITS)-1));
const int FAILZ_TH = p_from_ref(pclamp(G0_FAILZ_TH, 1, (1<<P_REF_BITS)-1));

// ---- what P_BITS is actually allowed to be -------------------------------
// These are the constraints the code really imposes, as opposed to the ones the
// naming makes it look like it imposes.  See paq8hpc_speed_results.md section 12
// for the measured survey.
//
// PR_SHIFT = PR_BITS-P_BITS must be >= 0: StateMap and APM hold probabilities at
// PR_BITS and shift down to P_BITS, and a negative shift is nonsense.
// The lower bound is 8 rather than 1 because IDX/paq8-G0.idx declares the fixed
// mixing weights in units of P_SCALE/256, which is where they stay exact -- the
// whole measured survey runs 8..16 anyway.
static_assert( P_BITS>=8 && P_BITS<=PR_BITS,
  "P_BITS must be in [8,16]: StateMap/APM store probabilities at PR_BITS=16 and "
  "shift down by PR_SHIFT=PR_BITS-P_BITS, and the IDX mixing weights are "
  "declared in units of P_SCALE/256" );
// stretch() values are held in `short` (Stretch::t, Mixer::tx), and ST_MAX is
// P_HALF-1 because squash indexes sqt[d+P_HALF] and APM indexes
// (pr+P_HALF)>>SQ_BITS -- both need the stretch range to be exactly [-P_HALF,P_HALF).
static_assert( ST_MAX<=32767 && ST_MIN>=-32768,
  "stretch values must fit in short" );
// APM no longer needs one: the interval index is (s*NIV)>>ST_RANGE_BITS with
// s < ST_RANGE, so it is at most NIV-1 and the index+1 read lands on node NIV,
// the last of the NIV+1 the row holds.  That holds for every NIV by
// construction, which is the point of moving off the shared squash grid.
// The rangecoder needs totFreq well below the renormalisation floor (sTOP=2^24).
static_assert( P_SCALE < (1<<24), "P_SCALE must stay far below the coder's sTOP" );
// the computed squash table stores probabilities, so its element type must hold P_MAX
static_assert( P_MAX <= 65535, "squash table element (U16) must hold P_MAX" );

// The baseline (non-PAQ_LOGISTIC) squash carries 33 table VALUES that are
// literal 12-bit probabilities, so that path is welded to P_BITS==12.  With
// PAQ_LOGISTIC the tables are computed instead and P_BITS is free.
#if !defined(PAQ_LOGISTIC)
static_assert( P_BITS==12,
  "the baseline squash() table holds 33 hardcoded 12-bit probabilities, so "
  "P_BITS != 12 needs -DPAQ_LOGISTIC (which computes the tables instead)" );
#endif

// ilog's domain is every U16 value.  Numerically equal to PR_ONE and completely
// unrelated to it -- kept separate on purpose.
constexpr int ILOG_SIZE = 1<<16;          // 65536

#ifndef min
inline int min(int a, int b) {
  return a<b ? a : b;
}

inline int max(int a, int b) {
  return a<b ? b : a;
}

#endif

typedef enum { DEFAULT, JPEG, EXE, BINTEXT, TEXT } Filetype;

#define preprocFlag 1220

#define OPTION_UTF8 1
#define OPTION_USE_NGRAMS 2
#define OPTION_CAPITAL_CONVERSION 4
#define OPTION_WORD_SURROROUNDING_MODELING 8
#define OPTION_SPACE_AFTER_EOL 16
#define OPTION_EOL_CODING 32
#define OPTION_NORMAL_TEXT_FILTER 64
#define OPTION_USE_DICTIONARY 128
#define OPTION_RECORD_INTERLEAVING 256
#define OPTION_DNA_QUARTER_BYTE 512
#define OPTION_TRY_SHORTER_WORD 1024
#define OPTION_TO_LOWER_AFTER_PUNCTUATION 2048
#define OPTION_SPACELESS_WORDS 4096
#define OPTION_ADD_SYMBOLS_0_5 8192
#define OPTION_ADD_SYMBOLS_14_31 16384
#define OPTION_ADD_SYMBOLS_A_Z 32768
#define OPTION_ADD_SYMBOLS_MISC 65536
#define OPTION_SPACE_AFTER_CC_FLAG 131072
#define IF_OPTION(option) ((preprocFlag&option)!=0)

struct Random {
U32 table[64];
int i;

Random() {
  table[0] = 123456789;
  table[1] = 987654321;
  for( int j = 0; j<62; j++ )
    table[j+2] = table[j+1]*11+table[j]*23/16;
  i = 0;
}

U32 operator()() {
  return ++i, table[i&63] = table[(i-24)&63]^table[(i-55)&63];
}
} rnd;

int pos;

// SZ must be a power of two.  Statically sized now, so setsize() is gone; the
// size is the 1<<30 the PAQ8HP wrapper used to pass, not a function of level.
template <U32 SZ> struct Buf {
U8 b[SZ];

U8 &operator[](U32 i) {
  return b[i&(SZ-1)];
}

int operator()(U32 i) const {
  return b[(pos-i)&(SZ-1)];
}

U32 size() const {
  return SZ;
}
};

// was MEM() over the global int level.  The baseline computed 0x10000<<level in
// int, which is UB for level>=15; done in U64 here, same result for every level
// this build accepts.  Levels above 11 are rejected by the dispatcher because
// (U32)MEM()*16 - the size the two big ContextMaps are built from - wraps to 0
// at level 12 and the baseline would have indexed a null table.
constexpr unsigned long long MEM(int L) {
  return (0x10000ULL<<L)>0xFFFFFFFF ? 0xFFFFFFFF : (0x10000ULL<<L);
}

int y = 0;

int c0 = 1;
U32 b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0, b7 = 0, b8 = 0, tt = 0, c4 = 0, x4 = 0, x5 = 0, w4 = 0, w5 = 0, f4 = 0;
int order, bpos = 0, cxtfl = 3, sm_mul = SM_MUL_0;

// The y-dependent target and rounding StateMap and APM share.  Both are
// (constant & -y), set once per bit, because both use the same rule: round away
// from the stored value, so a counter can actually reach PR_MAX rather than
// stalling one step short of it.  These were `sm_add`/`sm_add_y`, and `sm_add`
// had to be recomputed whenever the rate changed because the rounding was a
// function of the shift.  It is not any more, so the pair is now genuinely
// constant and APM can read it instead of rebuilding its own copy per call.
int ema_tgt_y = 0, ema_bias_y = 0;

// level-independent, and read by every model, so it stays at file scope with
// the rest of the shared scalar state rather than moving into Predictor<L>.
//
// SIZING: this is the history window every model back-references through -- the
// order-N contexts via buf(i), WordModel's `above` via buf[nl1+col].  There is
// no MatchModel in this build, so this buffer IS the whole match/history window.
// It must hold the entire input, or references farther back than SZ silently
// alias earlier bytes (operator[] masks with SZ-1) and the model sees wrapped
// garbage rather than real history -- no crash, no warning, just worse
// compression.  The target is enwik9 at 10^9 bytes, so the window is 1 GiB:
// 2^30 = 1,073,741,824, which clears enwik9 by 73.7 MB.
#ifndef PAQ_BUFBITS
  #define PAQ_BUFBITS 30
#endif
Buf<1u<<PAQ_BUFBITS> buf;

// Machine-checked so it cannot regress quietly.  This is not hypothetical: one
// of the reviewed optimisation docs proposed shrinking this to 4 MiB and
// labelled it bit-exact (see paq8hpc_speed.md section 9(g)), which would have
// broken every reference past 4 MiB while still round-tripping.  Build with
// -DPAQ_ALLOW_SMALL_BUF to deliberately use a smaller window (test_window.cpp
// uses it to exercise the wrap path).
//
// MEASURED REACH, so whoever adds a match model knows what the window is
// currently earning.  The only reads of buf in this build are buf(2), buf(3) in
// ContextModel::p and buf[nl1+col] in WordModel -- the previous-but-one line at
// the same column.  On book1[0:131072] the longest (prev line + current line)
// span is 137 bytes, and empirically a **128-byte** window reproduces the 1 GiB
// output byte for byte; it only diverges at 64 bytes.  So for text the current
// model set uses ~10^2 bytes of this 2^30-byte window.
//
// The one case where the size genuinely matters today: with no newlines in the
// input, nl1 stays at its -2 initial value, so buf[nl1+col] indexes the very TOP
// of the buffer.  That slot is untouched (and therefore a stable zero) only while
// the input has not wrapped -- measured on a 128 KB newline-free file, a 1 MiB
// window matches 1 GiB but a 64 KiB one does not.  A 1 GiB window keeps that
// context deterministic for any input up to 2^30, enwik9 included.
constexpr unsigned long long PAQ_ENWIK9 = 1000000000ull;
#if !defined(PAQ_ALLOW_SMALL_BUF)
static_assert( (1ull<<PAQ_BUFBITS) >= PAQ_ENWIK9,
  "history window (PAQ_BUFBITS) must hold enwik9 (10^9 bytes) whole -- "
  "anything smaller makes back-references past the window alias earlier bytes" );
#endif
// SZ is a U32 template argument and operator[] masks with SZ-1, so 32 would wrap
// the size itself to 0.
static_assert( PAQ_BUFBITS>=1 && PAQ_BUFBITS<=31, "PAQ_BUFBITS must be in [1,31]" );

struct Ilog {
U8 t[ILOG_SIZE];

int operator()(U16 x) const {
  return t[x];
}

Ilog() {
  U32 x = 14155776;
  for( int i = 2; i<ILOG_SIZE; ++i ) {
    x += 774541002/(i*2-1);
    t[i] = x>>24;
  }
}
} ilog;

inline int llog(U32 x) {
  if( x>=0x1000000 )
    return 256+ilog(x>>16);
  else if( x>=0x10000 )
    return 128+ilog(x>>8);
  else
    return ilog(x);
}

// One counter per ContextMap instance.
//
// Every ContextMap used to share a single 256-state machine and a single decay
// schedule.  They are not the same kind of map: the core map holds orders
// 3..13 over 2 GiB, RecordModel's cq holds three low-order contexts in 128 KiB,
// and what a "long run" means -- how far a count should saturate, how hard to
// discount the majority when the other bit arrives, when to step a state back
// -- is not obviously the same answer for both.  Each now has its own seven
// numbers and its own decay, so it can be answered separately.
//
// The index is a template parameter of ContextMap, so in the shipping build
// every one of these folds: the table address is a constant and the four decay
// values are literals, exactly as they were when there was one set.
//
// This lives above the PAQ_STATE_TABLE_GEN switch because it is a fact about
// the model rather than about how the machine is built -- the tabulated branch
// has to name the same counters even though it cannot give them separate
// tables.
enum {
  CTR_X,      // ContextModel::cm   orders 3,4,5,6,8,13,0,1,2 over MEM*16
  CTR_W,      // WordModel::cm      46 contexts over MEM*16
  CTR_S,      // SparseModel::cn    5 contexts over MEM*2
  CTR_RM,     // RecordModel::cm    byte/bigram recency, 8 KiB
  CTR_RN,     // RecordModel::cn    16 KiB
  CTR_RO,     // RecordModel::co    32 KiB
  CTR_RP,     // RecordModel::cp    64 KiB
  CTR_RQ,     // RecordModel::cq    128 KiB
  NCTR
};

// The bit-history state machine.  Two sources, one switch.
//
// PAQ_STATE_TABLE_GEN=1 (default) derives the 256-state automaton from the
// seven numbers in IDX/paq8-N0.idx, using fxcmv1's generator -- the machine
// described by its construction rule -- and derives one PER COUNTER.  =0 uses
// the 1012 tabulated bytes of IDX/paq8-T0.idx (transitions) and paq8-T1.idx
// (counts), which is the machine paq8hpc was originally tuned against.
//
// The generated machine is now the better of the two on book1 (191336 against
// 191980), and it is the only one of the two that cloning can reach: the
// tabulated bytes are one table, so its NEX ignores the counter index and all
// eight counters share it.  That branch is kept as the reference the generator
// was checked against, not as a tuning target.
//
// Both produce a [256][4] of { next0, next1, n0, n1 } per counter, which is
// what NEX() wants; nothing downstream can tell them apart.
#ifndef PAQ_STATE_TABLE_GEN
  #define PAQ_STATE_TABLE_GEN 1
#endif

#if PAQ_STATE_TABLE_GEN

#include "statetable.inc"

#else

// The tabulated machine, from IDX/paq8-T0.idx (the transitions, columns [0]
// and [1]) and IDX/paq8-T1.idx (the counts, [2] and [3]).  Two modules because
// they are not the same kind of number: T0 says `Const 1` in its own source so
// it folds to literals in BOTH builds and carries no !MAP! marker, while T1 is
// a live pattern per value in the tuning build.  `./mk.sh` therefore gives
// Const 0 for the counts only, which is what the split is for.
//
// The U8 casts are load-bearing, not tidiness: in the tuning build the counts
// are reads from mapping objects rather than constant expressions, and a
// narrowing conversion in a braced initialiser is ill-formed unless the value
// is a constant expression that fits.
//
// 253 rows, as the literal held; 253..255 were never written and stay zero.
#define ST(n) { U8(T0_s##n##_n0), U8(T0_s##n##_n1), U8(T1_s##n##_c0), U8(T1_s##n##_c1) }
static const U8 State_table[256][4] = {
  ST(000), ST(001), ST(002), ST(003), ST(004), ST(005), ST(006), ST(007),
  ST(008), ST(009), ST(010), ST(011), ST(012), ST(013), ST(014), ST(015),
  ST(016), ST(017), ST(018), ST(019), ST(020), ST(021), ST(022), ST(023),
  ST(024), ST(025), ST(026), ST(027), ST(028), ST(029), ST(030), ST(031),
  ST(032), ST(033), ST(034), ST(035), ST(036), ST(037), ST(038), ST(039),
  ST(040), ST(041), ST(042), ST(043), ST(044), ST(045), ST(046), ST(047),
  ST(048), ST(049), ST(050), ST(051), ST(052), ST(053), ST(054), ST(055),
  ST(056), ST(057), ST(058), ST(059), ST(060), ST(061), ST(062), ST(063),
  ST(064), ST(065), ST(066), ST(067), ST(068), ST(069), ST(070), ST(071),
  ST(072), ST(073), ST(074), ST(075), ST(076), ST(077), ST(078), ST(079),
  ST(080), ST(081), ST(082), ST(083), ST(084), ST(085), ST(086), ST(087),
  ST(088), ST(089), ST(090), ST(091), ST(092), ST(093), ST(094), ST(095),
  ST(096), ST(097), ST(098), ST(099), ST(100), ST(101), ST(102), ST(103),
  ST(104), ST(105), ST(106), ST(107), ST(108), ST(109), ST(110), ST(111),
  ST(112), ST(113), ST(114), ST(115), ST(116), ST(117), ST(118), ST(119),
  ST(120), ST(121), ST(122), ST(123), ST(124), ST(125), ST(126), ST(127),
  ST(128), ST(129), ST(130), ST(131), ST(132), ST(133), ST(134), ST(135),
  ST(136), ST(137), ST(138), ST(139), ST(140), ST(141), ST(142), ST(143),
  ST(144), ST(145), ST(146), ST(147), ST(148), ST(149), ST(150), ST(151),
  ST(152), ST(153), ST(154), ST(155), ST(156), ST(157), ST(158), ST(159),
  ST(160), ST(161), ST(162), ST(163), ST(164), ST(165), ST(166), ST(167),
  ST(168), ST(169), ST(170), ST(171), ST(172), ST(173), ST(174), ST(175),
  ST(176), ST(177), ST(178), ST(179), ST(180), ST(181), ST(182), ST(183),
  ST(184), ST(185), ST(186), ST(187), ST(188), ST(189), ST(190), ST(191),
  ST(192), ST(193), ST(194), ST(195), ST(196), ST(197), ST(198), ST(199),
  ST(200), ST(201), ST(202), ST(203), ST(204), ST(205), ST(206), ST(207),
  ST(208), ST(209), ST(210), ST(211), ST(212), ST(213), ST(214), ST(215),
  ST(216), ST(217), ST(218), ST(219), ST(220), ST(221), ST(222), ST(223),
  ST(224), ST(225), ST(226), ST(227), ST(228), ST(229), ST(230), ST(231),
  ST(232), ST(233), ST(234), ST(235), ST(236), ST(237), ST(238), ST(239),
  ST(240), ST(241), ST(242), ST(243), ST(244), ST(245), ST(246), ST(247),
  ST(248), ST(249), ST(250), ST(251), ST(252),
};
#undef ST

// The tabulated machine predates cloning and is a single table, so NEX drops
// the counter index and every counter sees the same automaton.  The decay
// schedule has to come from somewhere; it comes from counter X's slot in
// IDX/paq8-N0.idx, which is where the single shared schedule used to live
// before it was cloned.  Tuning the other seven counters' parameters has no
// effect in this build -- that is the point of the switch, not a bug in it.
#define NEX(ST, state, sel) State_table[state][sel]
#define CTR_DFROM(ST)  pclamp(N0_X_DFROM,  0, N_STATES-1)
#define CTR_DBIAS(ST)  pclamp(N0_X_DBIAS,  0, 511)
#define CTR_DSHIFT(ST) pclamp(N0_X_DSHIFT, 0, 8)
#define CTR_DSTEP(ST)  pclamp(N0_X_DSTEP,  0, 64)

#endif

// ---------------------------------------------------------------------------
// PAQ_LOGISTIC -- the ONLY gate in this file that changes the bitstream.
//
// Ported from xad_logistic.inc.  The baseline squash() is a 33-node
// piecewise-linear approximation of the logistic on a 128-wide grid, and
// stretch() is built by inverting *that*, so it inherits the approximation
// error.  Linear interpolation across a 128-wide step of a curve whose second
// derivative peaks at 4096*0.0962/256^2 costs up to (128^2/8)*that ~ 12 units,
// and measured it is 13.27 at d=318.
//
// The ported pair computes stretch exactly -- stretch(p) = S*ln(p/(N-p)) =
// S*ln2*(log2 p - log2(N-p)), from an integer 65536*log2 table built by repeated
// squaring -- and then finds squash as its inverse by binary search plus one
// linear interpolation, so it rounds the real crossing.  Max deviation from the
// true logistic drops 13.27 -> 0.58.
//
// paq8hp's constants (P_ONE 4096, ST_SCALE 256, stretch range +-2047) coincide
// with xadpcm's shipped ones, so the arithmetic ports unchanged and the tables
// come out bit-identical to xadpcm's -- see the FNV check below and
// test_logistic.cpp, which asserts it against the published XAD_SQSTT_HASH.
//
// No floating point: the one irrational needed is ln2 as a Q48 literal, so the
// tables are a property of this source rather than of whatever libm the build
// links.  That matters here for the same reason it did there -- encoder and
// decoder must agree on them exactly.
//
// The clamp shape is deliberately unchanged: squash still saturates to 0 below
// -2047 and 4095 above +2047, so the coder's `if(p<1) p=1` guard stays live
// rather than silently becoming dead code.
// ---------------------------------------------------------------------------
#if defined(PAQ_LOGISTIC)

static const long long LN2_Q48 = 195103586505167LL;   // round(ln2 * 2^48)

// Q8: the binary search runs on a stretch held at Q8_BITS fractional bits, so
// the crossing can be interpolated between adjacent p.
//
// The >>24 / >>32 below are applied to values that can be negative.  That is
// implementation-defined rather than undefined pre-C++20 (and arithmetic on every
// compiler this targets), which is what makes the +(1<<n) bias reproduce
// floor(x+0.5) for negative x too.  UBSan does not flag it; it flagged only the
// left shift, which is now a multiply.
constexpr int Q8_BITS = 8;
constexpr int Q8_ONE  = 1<<Q8_BITS;       // 256
constexpr int Q8_HALF = Q8_ONE/2;         // 128

struct Logistic {
  // sqt holds probabilities in [0,P_MAX] so it must be UNSIGNED: at P_BITS==16,
  // P_MAX is 65535 and short(65535) is -1.  stt holds signed stretch values,
  // bounded by ST_MAX <= 32767, so short is right for it.
  U16   sqt[2*ST_LIMIT];// squash:  d+ST_LIMIT -> p at P_BITS
  short stt[P_SCALE];   // stretch: p        -> d

  Logistic() {
    // 65536*log2(i): square, renormalise below 2^32, count the shifts.  The +31
    // corrects the truncation bias renormalisation accumulates, since
    // i^(2^16) = w*2^k with w in [2^31,2^32) gives 2^16*log2(i) = k + log2(w).
    static U32 L[P_SCALE];
    for( U32 i = 0; i<U32(P_SCALE); i++ ) {
      unsigned long long w = i;
      U32 k = 0;
      for( int j = 0; j<16; j++ ) {
        w = w*w;
        k = k+k;
        while( w >= (1ULL<<32) ) { w = (w+1)>>1; k++; }
      }
      L[i] = k + (i>1 ? 31u : 0u);
    }
    // S*ln2*2^16.  Q48 * 2^8 stays well inside i64.
    const long long K = ((long long)ST_SCALE*LN2_Q48 + (1LL<<31))>>32;

    // stretch, rounded: the +2^31 then >>32 is floor(x+0.5) and stays correct
    // for negative x, which round-away-from-zero would not.
    for( int p = 0; p<P_SCALE; p++ ) {
      int pi = p<P_MIN ? P_MIN : p>P_MAX ? P_MAX : p;
      long long v = (((long long)L[pi] - (long long)L[P_SCALE-pi])*K + (1LL<<31))>>32;
      stt[p] = short(v<ST_MIN ? ST_MIN : v>ST_MAX ? ST_MAX : v);
    }

    // squash = inverse of the above, via a Q8 stretch and one interpolation
    for( int d = -ST_LIMIT; d<ST_LIMIT; d++ ) {
      // d*Q8_ONE, not d<<Q8_BITS: d is negative over half this range, and left
      // shift of a negative value is UB before C++20 (xad_logistic.inc relies on
      // being built as C++20; this file does not set -std, and gcc 13 defaults to
      // gnu++17).  The multiply is well-defined and compiles to the same shift.
      // Caught by -fsanitize=undefined; the non-PAQ_LOGISTIC path is clean.
      long long t = (long long)d*Q8_ONE;
      int lo = P_MIN, hi = P_MAX;
      while( lo<hi ) {
        int m = (lo+hi)>>1;
        long long q = (((long long)L[m] - (long long)L[P_SCALE-m])*K + (1LL<<23))>>24;
        if( q<t ) lo = m+1; else hi = m;
      }
      int best = lo;
      if( lo>P_MIN ) {
        long long s1 = (((long long)L[lo]   - (long long)L[P_SCALE-lo]  )*K + (1LL<<23))>>24;
        long long s0 = (((long long)L[lo-1] - (long long)L[P_SCALE-lo+1])*K + (1LL<<23))>>24;
        if( s1>t && s1!=s0 ) {
          long long frac = ((t-s0)*Q8_ONE + (s1-s0)/2)/(s1-s0);
          best = (frac>=Q8_HALF) ? lo : lo-1;
        }
      }
      sqt[d+ST_LIMIT] = U16(best<P_MIN ? P_MIN : best>P_MAX ? P_MAX : best);
    }

    // The tables are the format: a build whose pair differs cannot decode
    // another's archive, and this says so at startup instead of as garbage.
    // -DPAQ_LOGISTIC_HASH=0 to silence while deliberately changing them.
#if !defined(PAQ_LOGISTIC_HASH)
  #define PAQ_LOGISTIC_HASH 0x7BCBA716u
#endif
    // only meaningful at the shipped scales -- a sweep of P_BITS/ST_SCALE must be
    // free to produce different tables, which is the point of sweeping them
    if( PAQ_LOGISTIC_HASH && P_BITS==12 && ST_SCALE==256 ) {
      U32 h = 2166136261u;                        // FNV-1a over the two tables
      // interleaved (sqt[i],stt[i]) exactly as xad_logistic.inc hashes them --
      // splitting it into two loops changes the value, which is how the guard
      // below caught the restructuring.  Bounded by the shorter table so it can
      // never read out of range once the two sizes stop being equal.
      constexpr int HN = (2*ST_LIMIT < P_SCALE) ? 2*ST_LIMIT : P_SCALE;
      for( int i = 0; i<HN; i++ ) {
        h = (h ^ U32(sqt[i]))*16777619u;
        h = (h ^ U32((U16)stt[i]))*16777619u;
      }
      if( h!=U32(PAQ_LOGISTIC_HASH) ) {
        printf( "paq8hpc: squash/stretch checksum %08X, expected %08X -- the tables are "
                "pure integer arithmetic, so this means a corrupted or mismatched build\n",
                h, U32(PAQ_LOGISTIC_HASH) );
        exit(1);
      }
    }
  }
};

// must precede `stretch` below: same TU, so declaration order is init order
Logistic g_logistic;

int squash(int d) {
  if( d>ST_MAX )
    return P_MAX;
  if( d<ST_MIN )
    return 0;
  return g_logistic.sqt[d+ST_LIMIT];
}

struct Stretch {
int operator()(int p) const {
  return g_logistic.stt[p];
}
} stretch;

#else

int squash(int d) {
  static const int t[SQ_NODES] = {1, 2, 3, 6, 10, 16, 27, 45, 73, 120, 194, 310, 488, 747, 1101, 1546, 2047, 2549, 2994, 3348, 3607, 3785, 3901, 3975, 4022, 4050, 4068, 4079, 4085, 4089, 4092, 4093, 4094};
  if( d>ST_MAX )
    return P_MAX;
  if( d<ST_MIN )
    return 0;
  int w = d&SQ_MASK;
  d = (d>>SQ_BITS)+SQ_MID;
  return (t[d]*(SQ_STEP-w)+t[(d+1)]*w+SQ_STEP/2)>>SQ_BITS;
}

struct Stretch {
short t[P_SCALE];

Stretch() {
  int pi = 0;
  for( int x = ST_MIN; x<=ST_MAX; ++x ) {
    int i = squash(x);
    for( int j = pi; j<=i; ++j )
      t[j] = x;
    pi = i+1;
  }
  t[P_MAX] = ST_MAX;
}

int operator()(int p) const {
  return t[p];
}
} stretch;

#endif

#if !defined(__GNUC__)

#if (2==_M_IX86_FP)
#define __SSE2__
#elif (1==_M_IX86_FP)
#define __SSE__
#endif

#endif

static int dot_product(const short*const t, const short*const w, int n);

static void train(const short*const t, short*const w, int n, const int e);

// PAQ_N16 raises Mixer::N to a multiple of 16 (456 -> 464) and pads nx to 16 in
// Mixer::p(), which is what makes every 32-byte load in a wx row naturally
// aligned: the row stride becomes 464*2 = 928 B (a 32-byte multiple) and the
// array is 64-byte aligned, so row starts are 0 or 32 mod 64.  Without it the
// stride is 912 B and every odd row is 16-mod-32, so only loadu is legal - no
// amount of alignas on the base can fix a stride (doc 9c).
#if defined(PAQ_N16)
  #define PAQ_MIXALIGN 64
#else
  #define PAQ_MIXALIGN 16
#endif

// ---- AVX-512 --------------------------------------------------------------
// Row stride is 928 B even under PAQ_N16, i.e. 32 mod 64, so 512-bit loads must
// stay unaligned.  Built to settle the doc's claim that it is a wash; see the
// results file.
#if defined(PAQ_AVX512)
#include <immintrin.h>
#define OPTIMIZE "AVX512-"
#define PAQ_KERNEL "avx512"

static int dot_product(const short*const PAQ_RS t, const short*const PAQ_RS w, int n) {
  __m512i sum = _mm512_setzero_si512();
  while( (n -= 32)>=0 ) {
    __m512i tmp = _mm512_madd_epi16(_mm512_loadu_si512((const void*)&t[n]), _mm512_loadu_si512((const void*)&w[n]));
    tmp = _mm512_srai_epi32(tmp, 8);
    sum = _mm512_add_epi32(sum, tmp);
  }
  __m256i s2 = _mm256_add_epi32(_mm512_castsi512_si256(sum), _mm512_extracti64x4_epi64(sum, 1));
  if( (n += 16)>=0 ) {
    __m256i tmp = _mm256_madd_epi16(_mm256_loadu_si256((const __m256i*)&t[n]), _mm256_loadu_si256((const __m256i*)&w[n]));
    tmp = _mm256_srai_epi32(tmp, 8);
    s2 = _mm256_add_epi32(s2, tmp);
    n -= 16;
  }
  __m128i s = _mm_add_epi32(_mm256_castsi256_si128(s2), _mm256_extracti128_si256(s2, 1));
  if( (n += 8)>=0 ) {
    __m128i tmp = _mm_madd_epi16(_mm_loadu_si128((const __m128i*)&t[n]), _mm_loadu_si128((const __m128i*)&w[n]));
    tmp = _mm_srai_epi32(tmp, 8);
    s = _mm_add_epi32(s, tmp);
  }
  s = _mm_add_epi32(s, _mm_srli_si128(s, 8));
  s = _mm_add_epi32(s, _mm_srli_si128(s, 4));
  return _mm_cvtsi128_si32(s);
}

static void train(const short*const PAQ_RS t, short*const PAQ_RS w, int n, const int e) {
  if( e ) {
    const __m512i one = _mm512_set1_epi16(1);
    const __m512i err = _mm512_set1_epi16(short(e));
    while( (n -= 32)>=0 ) {
      __m512i tmp = _mm512_loadu_si512((const void*)&t[n]);
      tmp = _mm512_adds_epi16(tmp, tmp);
      tmp = _mm512_mulhi_epi16(tmp, err);
      tmp = _mm512_adds_epi16(tmp, one);
      tmp = _mm512_srai_epi16(tmp, 1);
      tmp = _mm512_adds_epi16(tmp, _mm512_loadu_si512((const void*)&w[n]));
      _mm512_storeu_si512((void*)&w[n], tmp);
    }
    const __m256i one2 = _mm256_set1_epi16(1);
    const __m256i err2 = _mm256_set1_epi16(short(e));
    if( (n += 16)>=0 ) {
      __m256i tmp = _mm256_loadu_si256((const __m256i*)&t[n]);
      tmp = _mm256_adds_epi16(tmp, tmp);
      tmp = _mm256_mulhi_epi16(tmp, err2);
      tmp = _mm256_adds_epi16(tmp, one2);
      tmp = _mm256_srai_epi16(tmp, 1);
      tmp = _mm256_adds_epi16(tmp, _mm256_loadu_si256((const __m256i*)&w[n]));
      _mm256_storeu_si256((__m256i*)&w[n], tmp);
      n -= 16;
    }
    if( (n += 8)>=0 ) {
      const __m128i one1 = _mm_set1_epi16(1);
      const __m128i err1 = _mm_set1_epi16(short(e));
      __m128i tmp = _mm_loadu_si128((const __m128i*)&t[n]);
      tmp = _mm_adds_epi16(tmp, tmp);
      tmp = _mm_mulhi_epi16(tmp, err1);
      tmp = _mm_adds_epi16(tmp, one1);
      tmp = _mm_srai_epi16(tmp, 1);
      tmp = _mm_adds_epi16(tmp, _mm_loadu_si128((const __m128i*)&w[n]));
      _mm_storeu_si128((__m128i*)&w[n], tmp);
    }
  }
}

// ---- AVX2 -----------------------------------------------------------------
// madd pairs the same adjacent shorts at any lane width, the >>8 is per-32-bit
// lane, and the reduction is a mod-2^32 sum of the same int32 terms, so the
// result is identical to the SSE2 kernel regardless of lane count.  train is
// elementwise saturating ops - lane width is invisible.
// Tail: nx is a multiple of 8, so one 128-bit step can remain (nx == 8 mod 16).
// It must NOT be removed by padding nx to 16 unless N grows too (PAQ_N16) -
// tx is short[456] exactly and wx rows are contiguous, so writing tx[456..463]
// or training w[456..463] would corrupt the next context's row (doc 9d).
#elif defined(PAQ_AVX2)
#include <immintrin.h>
#define OPTIMIZE "AVX2-"
#define PAQ_KERNEL "avx2"

#if defined(PAQ_N16)
  #define PAQ_LD256(p) _mm256_load_si256((const __m256i*)(p))
  #define PAQ_ST256(p,v) _mm256_store_si256((__m256i*)(p),(v))
#else
  #define PAQ_LD256(p) _mm256_loadu_si256((const __m256i*)(p))
  #define PAQ_ST256(p,v) _mm256_storeu_si256((__m256i*)(p),(v))
#endif

static int dot_product(const short*const PAQ_RS t, const short*const PAQ_RS w, int n) {
  __m256i sum = _mm256_setzero_si256();
  while( (n -= 16)>=0 ) {
    __m256i tmp = _mm256_madd_epi16(PAQ_LD256(&t[n]), PAQ_LD256(&w[n]));
    tmp = _mm256_srai_epi32(tmp, 8);
    sum = _mm256_add_epi32(sum, tmp);
  }
  __m128i s = _mm_add_epi32(_mm256_castsi256_si128(sum), _mm256_extracti128_si256(sum, 1));
  if( (n += 8)>=0 ) {
    __m128i tmp = _mm_madd_epi16(_mm_loadu_si128((const __m128i*)&t[n]), _mm_loadu_si128((const __m128i*)&w[n]));
    tmp = _mm_srai_epi32(tmp, 8);
    s = _mm_add_epi32(s, tmp);
  }
  s = _mm_add_epi32(s, _mm_srli_si128(s, 8));
  s = _mm_add_epi32(s, _mm_srli_si128(s, 4));
  return _mm_cvtsi128_si32(s);
}

static void train(const short*const PAQ_RS t, short*const PAQ_RS w, int n, const int e) {
  if( e ) {
    const __m256i one = _mm256_set1_epi16(1);
    const __m256i err = _mm256_set1_epi16(short(e));
    while( (n -= 16)>=0 ) {
      __m256i tmp = PAQ_LD256(&t[n]);
      tmp = _mm256_adds_epi16(tmp, tmp);
      tmp = _mm256_mulhi_epi16(tmp, err);
      tmp = _mm256_adds_epi16(tmp, one);
      tmp = _mm256_srai_epi16(tmp, 1);
      tmp = _mm256_adds_epi16(tmp, PAQ_LD256(&w[n]));
      PAQ_ST256(&w[n], tmp);
    }
    if( (n += 8)>=0 ) {
      const __m128i one1 = _mm_set1_epi16(1);
      const __m128i err1 = _mm_set1_epi16(short(e));
      __m128i tmp = _mm_loadu_si128((const __m128i*)&t[n]);
      tmp = _mm_adds_epi16(tmp, tmp);
      tmp = _mm_mulhi_epi16(tmp, err1);
      tmp = _mm_adds_epi16(tmp, one1);
      tmp = _mm_srai_epi16(tmp, 1);
      tmp = _mm_adds_epi16(tmp, _mm_loadu_si128((const __m128i*)&w[n]));
      _mm_storeu_si128((__m128i*)&w[n], tmp);
    }
  }
}

// two rows per pass: tx is loaded once and the two miss streams overlap.
// Independent accumulators, same per-lane ops -> both results are identical to
// two separate calls (doc 10.1).
#define PAQ_HAVE_DOT2 1
static void dot_product2(const short*const PAQ_RS t, const short*const PAQ_RS w0, const short*const PAQ_RS w1, int n, int&r0, int&r1) {
  __m256i s0 = _mm256_setzero_si256(), s1 = _mm256_setzero_si256();
  while( (n -= 16)>=0 ) {
    __m256i tv = PAQ_LD256(&t[n]);
    __m256i a = _mm256_madd_epi16(tv, PAQ_LD256(&w0[n]));
    __m256i b = _mm256_madd_epi16(tv, PAQ_LD256(&w1[n]));
    s0 = _mm256_add_epi32(s0, _mm256_srai_epi32(a, 8));
    s1 = _mm256_add_epi32(s1, _mm256_srai_epi32(b, 8));
  }
  __m128i q0 = _mm_add_epi32(_mm256_castsi256_si128(s0), _mm256_extracti128_si256(s0, 1));
  __m128i q1 = _mm_add_epi32(_mm256_castsi256_si128(s1), _mm256_extracti128_si256(s1, 1));
  if( (n += 8)>=0 ) {
    __m128i tv = _mm_loadu_si128((const __m128i*)&t[n]);
    q0 = _mm_add_epi32(q0, _mm_srai_epi32(_mm_madd_epi16(tv, _mm_loadu_si128((const __m128i*)&w0[n])), 8));
    q1 = _mm_add_epi32(q1, _mm_srai_epi32(_mm_madd_epi16(tv, _mm_loadu_si128((const __m128i*)&w1[n])), 8));
  }
  q0 = _mm_add_epi32(q0, _mm_srli_si128(q0, 8));
  q0 = _mm_add_epi32(q0, _mm_srli_si128(q0, 4));
  q1 = _mm_add_epi32(q1, _mm_srli_si128(q1, 8));
  q1 = _mm_add_epi32(q1, _mm_srli_si128(q1, 4));
  r0 = _mm_cvtsi128_si32(q0);
  r1 = _mm_cvtsi128_si32(q1);
}

static void train2(const short*const PAQ_RS t, short*const PAQ_RS w0, short*const PAQ_RS w1, int n, const int e0, const int e1) {
  if( (e0|e1)==0 )
    return;
  const __m256i one = _mm256_set1_epi16(1);
  const __m256i r0 = _mm256_set1_epi16(short(e0));
  const __m256i r1 = _mm256_set1_epi16(short(e1));
  int n0 = n;
  while( (n -= 16)>=0 ) {
    __m256i tv = _mm256_adds_epi16(PAQ_LD256(&t[n]), PAQ_LD256(&t[n]));
    __m256i a = _mm256_srai_epi16(_mm256_adds_epi16(_mm256_mulhi_epi16(tv, r0), one), 1);
    __m256i b = _mm256_srai_epi16(_mm256_adds_epi16(_mm256_mulhi_epi16(tv, r1), one), 1);
    PAQ_ST256(&w0[n], _mm256_adds_epi16(a, PAQ_LD256(&w0[n])));
    PAQ_ST256(&w1[n], _mm256_adds_epi16(b, PAQ_LD256(&w1[n])));
  }
  if( (n += 8)>=0 ) {
    const __m128i o1 = _mm_set1_epi16(1);
    const __m128i q0 = _mm_set1_epi16(short(e0));
    const __m128i q1 = _mm_set1_epi16(short(e1));
    __m128i tv = _mm_loadu_si128((const __m128i*)&t[n]);
    tv = _mm_adds_epi16(tv, tv);
    __m128i a = _mm_srai_epi16(_mm_adds_epi16(_mm_mulhi_epi16(tv, q0), o1), 1);
    __m128i b = _mm_srai_epi16(_mm_adds_epi16(_mm_mulhi_epi16(tv, q1), o1), 1);
    _mm_storeu_si128((__m128i*)&w0[n], _mm_adds_epi16(a, _mm_loadu_si128((const __m128i*)&w0[n])));
    _mm_storeu_si128((__m128i*)&w1[n], _mm_adds_epi16(b, _mm_loadu_si128((const __m128i*)&w1[n])));
  }
  (void)n0;
}

#elif defined(__SSE2__)
#include <emmintrin.h>
#define OPTIMIZE "SSE2-"

static int dot_product(const short*const t, const short*const w, int n) {
  __m128i sum = _mm_setzero_si128();
  while( (n -= 8)>=0 ) {
    __m128i tmp = _mm_madd_epi16(*(__m128i*)&t[n], *(__m128i*)&w[n]);
    tmp = _mm_srai_epi32(tmp, 8);
    sum = _mm_add_epi32(sum, tmp);
  }
  sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 8));
  sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 4));
  return _mm_cvtsi128_si32(sum);
}

static void train(const short*const t, short*const w, int n, const int e) {
  if( e ) {
    const __m128i one = _mm_set1_epi16(1);
    const __m128i err = _mm_set1_epi16(short(e));
    while( (n -= 8)>=0 ) {
      __m128i tmp = _mm_adds_epi16(*(__m128i*)&t[n], *(__m128i*)&t[n]);
      tmp = _mm_mulhi_epi16(tmp, err);
      tmp = _mm_adds_epi16(tmp, one);
      tmp = _mm_srai_epi16(tmp, 1);
      tmp = _mm_adds_epi16(tmp, *(__m128i*)&w[n]);
      *(__m128i*)&w[n] = tmp;
    }
  }
}

#elif defined(__SSE__)
#include <xmmintrin.h>
#define OPTIMIZE "SSE-"

static int dot_product(const short*const t, const short*const w, int n) {
  __m64 sum = _mm_setzero_si64();
  while( (n -= 8)>=0 ) {
    __m64 tmp = _mm_madd_pi16(*(__m64*)&t[n], *(__m64*)&w[n]);
    tmp = _mm_srai_pi32(tmp, 8);
    sum = _mm_add_pi32(sum, tmp);

    tmp = _mm_madd_pi16(*(__m64*)&t[n+4], *(__m64*)&w[n+4]);
    tmp = _mm_srai_pi32(tmp, 8);
    sum = _mm_add_pi32(sum, tmp);
  }
  sum = _mm_add_pi32(sum, _mm_srli_si64(sum, 32));
  const int retval = _mm_cvtsi64_si32(sum);
  _mm_empty();
  return retval;
}

static void train(const short*const t, short*const w, int n, const int e) {
  if( e ) {
    const __m64 one = _mm_set1_pi16(1);
    const __m64 err = _mm_set1_pi16(short(e));
    while( (n -= 8)>=0 ) {
      __m64 tmp = _mm_adds_pi16(*(__m64*)&t[n], *(__m64*)&t[n]);
      tmp = _mm_mulhi_pi16(tmp, err);
      tmp = _mm_adds_pi16(tmp, one);
      tmp = _mm_srai_pi16(tmp, 1);
      tmp = _mm_adds_pi16(tmp, *(__m64*)&w[n]);
      *(__m64*)&w[n] = tmp;

      tmp = _mm_adds_pi16(*(__m64*)&t[n+4], *(__m64*)&t[n+4]);
      tmp = _mm_mulhi_pi16(tmp, err);
      tmp = _mm_adds_pi16(tmp, one);
      tmp = _mm_srai_pi16(tmp, 1);
      tmp = _mm_adds_pi16(tmp, *(__m64*)&w[n+4]);
      *(__m64*)&w[n+4] = tmp;
    }
    _mm_empty();
  }
}

#else
int dot_product(short* t, short* w, int n) {
  int sum = 0;
  n = (n+15)& -16;
  for( int i = 0; i<n; i += 2 )
    sum += (t[i]*w[i]+t[i+1]*w[i+1])>>8;
  return sum;
}

void train(short* t, short* w, int n, int err) {
  n = (n+15)& -16;
  for( int i = 0; i<n; ++i ) {
    int wt = w[i]+(((t[i]*err*2>>16)+1)>>1);
    if( wt<-32768 )
      wt = -32768;
    if( wt>32767 )
      wt = 32767;
    w[i] = wt;
  }
}

#endif

#ifndef PAQ_KERNEL
  #define PAQ_KERNEL OPTIMIZE
#endif

// PAQ_DOT2 on an ISA without a fused kernel still has to compile and still has
// to be exact, so fall back to two independent calls.
#ifndef PAQ_HAVE_DOT2
static inline void dot_product2(const short* t, const short* w0, const short* w1, int n, int&r0, int&r1) {
  r0 = dot_product((short*)t, (short*)w0, n);   // the scalar path declares these non-const
  r1 = dot_product((short*)t, (short*)w1, n);
}

static inline void train2(const short* t, short* w0, short* w1, int n, const int e0, const int e1) {
  train((short*)t, w0, n, e0);
  train((short*)t, w1, n, e1);
}
#endif

// The nested mixer the baseline built with `new Mixer(S,1,1,0x7fff)` when S>1.
// It has to be held by value now, but a Mixer with S==1 must not contain one or
// the instantiation would never terminate, so the holder is specialised empty
// for S==1 and forwards the three calls the outer p() makes.
// `w` is an IDX value, so its declared type differs between the two builds and
// nothing else does -- see IDXP above.  The s and w defaults are gone with it:
// a reference parameter cannot take one, and both instantiations below have
// always passed all four arguments explicitly.
template <int n, int m, int s, IDXP(w)> struct Mixer;

template <int S> struct SubMixer {
Mixer<S, 1, 1, MIX2_W_INIT> mp;

void update2() {
  mp.update2();
}

// The submixer's layout is the six stage-1 selectors in order, so the slot is
// the selector index -- no cursor here either.
void put(int i, int x) {
  mp.put(i, x);
}

void seek(int i) {
  mp.seek(i);
}

int p() {
  return mp.p();
}
};

template <> struct SubMixer<1> {
void update2() {
}

void seek(int) {
}

void put(int, int) {
}

int p() {
  return 0;
}
};

template <int n, int m, int s, IDXP(w)> struct Mixer {
#if defined(PAQ_N16)
static constexpr int N = (n+15)& -16, M = m, S = s, NPAD = 15;
#else
static constexpr int N = (n+7)& -8, M = m, S = s, NPAD = 7;
#endif

// 16-byte alignment is load-bearing: dot_product/train do aligned __m128i
// accesses, and Array<short,16> used to supply it.  N is a multiple of 8, so
// every wx row start is 16-byte aligned too.
alignas(PAQ_MIXALIGN) short wx[N*M];
int cxt[S];
int ncxt;
int base;
int pr[S];
SubMixer<S> sub;

alignas(PAQ_MIXALIGN) short tx[N];
int nx;

Mixer() : ncxt(0), base(0), nx(0) {
  int i;
  for( i = 0; i<S; ++i )
    pr[i] = P_HALF;
  for( i = 0; i<N*M; ++i )
    wx[i] = w;
  for( i = 0; i<S; ++i )
    cxt[i] = 0;
  for( i = 0; i<N; ++i )
    tx[i] = 0;
}

void update() {
#if defined(PAQ_DOT2)
  int i = 0;
  for(; i+1<ncxt; i += 2 )
    train2(&tx[0], &wx[cxt[i]*N], &wx[cxt[i+1]*N], nx,
           p_scaled((y<<P_BITS)-pr[i],MIX_LR_MUL), p_scaled((y<<P_BITS)-pr[i+1],MIX_LR_MUL));
  for(; i<ncxt; ++i )
    train(&tx[0], &wx[cxt[i]*N], nx, p_scaled((y<<P_BITS)-pr[i],MIX_LR_MUL));
#else
  for( int i = 0; i<ncxt; ++i ) {
    int err = p_scaled((y<<P_BITS)-pr[i],MIX_LR_MUL);
    train(&tx[0], &wx[cxt[i]*N], nx, err);
  }
#endif
  nx = base = ncxt = 0;
}

void update2() {
  train(&tx[0], &wx[0], nx, p_scaled((y<<P_BITS)-base,MIX2_LR_MUL));
  nx = 0;
}

// Every input is written to the slot mix_layout.inc gives it.  There is no
// append and no cursor: `nx` is set once, to the length of the vector, and is
// only ever read as that length by p() and train().
//
// Two properties this rests on, both checked rather than assumed:
//
//  * A span must be written in FULL every bit.  An append left no stale slots
//    because the cursor restarted at 0; a slot write overwrites in place, so a
//    slot that is skipped keeps the previous bit's value.  mix_ctxcheck() is
//    the guard: a ContextMap handed fewer contexts than the layout reserved
//    slots for would leave the tail of its span stale, silently and only for
//    some contexts.
//  * A span that has never been written holds zero -- tx is zeroed at
//    construction -- and a zero input contributes zero to every dot product and
//    moves no weight in train().  That is what makes the first byte free (no
//    map has been given contexts yet) and what would make a submodel gated off
//    by level, or switched off deliberately, cost nothing but its slots.
void put(int i, int x) { tx[i] = x; }
void seek(int i) { nx = i; }

// x is a P_SCALE-unit multiplier, so x==P_SCALE leaves the input alone.  It
// used to be a numerator over the shared MIX_MUL_DEN; st_scaled is the same
// arithmetic with the denominator folded in, and these are logistic-domain
// values so no reference rescaling belongs here.
void mulat(int i, int x) {
  tx[i] = st_scaled(tx[i], x);
}

// The six m.set() calls in ContextModel::p() use disjoint base ranges, so
// cxt[0..5] are always distinct - which is what lets train2() above write two
// rows in one interleaved pass without aliasing.
void set(int cx, int range) {
  cxt[ncxt++] = base+cx;
  base += range;
#if USE_NEW
  // The tuning build's IDX Volumes are runtime reads, so the static_assert that
  // guards this in the shipping build cannot be written here (see MIXER_ROWS).
  // Widen a mask in the .idx and the ranges stop fitting wx; say so instead of
  // walking off the end of it.  Compiled out entirely once the Volumes fold.
  if( base>M ) {
    fprintf( stderr, "paq8hpc: mixer context ranges sum to %i, wx holds %i rows -- "
                     "IDX/paq8-G0.idx declares more than MIXER_ROWS\n", base, M );
    exit(1);
  }
#endif
#if defined(PAQ_WXPF)
  // pull the selected weight row in before p() dots it and update() trains it
  const char* r = (const char*)&wx[cxt[ncxt-1]*N];
  for( int o = 0; o<int(N*sizeof(short)); o += 64 )
    PAQ_PF(r+o);
#endif
}

int p() {
  while( nx&NPAD )
    tx[nx++] = 0;
  if( S>1 ) {
    sub.update2();
#if defined(PAQ_DOT2)
    int i = 0;
    for(; i+1<ncxt; i += 2 ) {
      int d0, d1;
      dot_product2(&tx[0], &wx[cxt[i]*N], &wx[cxt[i+1]*N], nx, d0, d1);
      d0 = (d0*MIX_DP_MUL)>>MIX_DP_SHIFT;
      pr[i] = squash(d0);
      sub.put(i, d0);
      d1 = (d1*MIX_DP_MUL)>>MIX_DP_SHIFT;
      pr[i+1] = squash(d1);
      sub.put(i+1, d1);
    }
    for(; i<ncxt; ++i ) {
      int dp = dot_product(&tx[0], &wx[cxt[i]*N], nx);
      dp = (dp*MIX_DP_MUL)>>MIX_DP_SHIFT;
      pr[i] = squash(dp);
      sub.put(i, dp);
    }
#else
    for( int i = 0; i<ncxt; ++i ) {
      int dp = dot_product(&tx[0], &wx[cxt[i]*N], nx);
      dp = (dp*MIX_DP_MUL)>>MIX_DP_SHIFT;
      pr[i] = squash(dp);
      sub.put(i, dp);
    }
#endif
    sub.seek(ncxt);
    return sub.p();
  } else {
    int z = dot_product(&tx[0], &wx[0], nx);
    base = squash((z*MIX2_ZB_MUL)>>MIX2_ZB_SHIFT);
    return squash(z>>MIX2_Z_SHIFT);
  }
}
};

// Mixer::wx is a fixed-size member, so its row count has to be a literal in
// BOTH builds -- it cannot be the sum of the IDX Volumes, because in the tuning
// build those are reads from mapping objects rather than constant expressions.
// So it stays written out, and the two checks keep it honest: a static_assert
// where the Volumes fold, and the bounds check in Mixer::set where they do not.
// The mixer's input vector, as compile-time constants: one enum per submodel
// naming the start of every block it contributes, a second laying the
// submodels end to end.  MIX_INPUTS is what MainMixer is instantiated on, and
// the context counts the maps are declared from live there too, so a context
// count and the layout that depends on it cannot drift apart.
#include "mix_layout.inc"

constexpr int MIXER_ROWS = 128*(18+18+16+12+18+16);   // 12544

// The two mixer context ranges that are not IDX Indexes -- see the comment at
// the six m.set() calls in ContextModel::p.
const int MIX_R3 = 1536;
const int MIX_R5 = 2048;

#if !USE_NEW
static_assert( G0_MX0_Volume + G0_MX1_Volume + G0_MX2_Volume
             + MIX_R3 + G0_MX4_Volume + MIX_R5 == MIXER_ROWS,
  "the six mixer context ranges must exactly fill Mixer::wx" );
#endif

// the one mixer the models are handed
typedef Mixer<MIX_INPUTS, MIXER_ROWS, 6, MIX_W_INIT> MainMixer;

// The invariant §10.5 says nothing checks, in the form it takes once every input
// is placed by slot.  The cursor is no longer a cursor, so "where did the writes
// end" is not the question any more; the question is whether a map was handed
// exactly as many contexts as mix_layout.inc reserved slots for it.  Give it
// fewer and the tail of its span keeps the previous bit's values -- silently,
// and only for some contexts, which is the worst shape a bug can have here.
//
// cn == 0 is the first byte, before any set() has run: see ContextMap::mix1t.
inline void mix_ctxcheck( int cn, int C ) {
  if_e0( cn!=C && cn!=0 ) {
    fprintf( stderr, "paq8hpc: mixer layout: a ContextMap was given %i contexts, "
                     "mix_layout.inc reserves %i\n", cn, C );
    exit(1);
  }
}

// n contexts, 2*NH intervals per context.  NH comes from IDX per instance, so
// it is a template parameter of reference type in the tuning build and a plain
// int in the shipping build -- see IDXP.
template <int n, IDXP(NH)> struct APM {
// intervals.  IDXC because NH is only a constant expression in the shipping
// build; there this is `static constexpr`, in the tuning build a const member
// initialised at construction.
IDXC(int, NIV, 2*(NH));

// Cells allocated per context.  The tuning build cannot size a member array
// from an IDX value, so it reserves the widest grid the search space allows and
// treats the live node count as the row's CONTENT rather than its extent; the
// shipping build sizes it exactly.  Rows stay disjoint either way, so the two
// builds differ in layout only and code identically -- which is what verify.sh
// checks.
#if USE_NEW
static constexpr int CELLS = APM_CELLS_MAX;
#else
static constexpr int CELLS = NIV+1;
#endif

int index;

U16 t[n*CELLS];

APM() : index(0) {
  for( int j = 0; j<=NIV; ++j )
    t[j] = squash(j*ST_RANGE/NIV - ST_LIMIT)<<PR_SHIFT;
  // only reachable when CELLS > NIV+1, i.e. in the tuning build: the reserve
  // beyond the live grid is never read, but replicating it below would be
  // reading uninitialised storage.
  for( int j = NIV+1; j<CELLS; ++j )
    t[j] = 0;
  for( int i = CELLS; i<n*CELLS; ++i )
    t[i] = t[i-CELLS];
}

// p() will touch t[cxt*CELLS + k] and +1 for some k in [0,NIV-1], i.e. the
// (NIV+1)*2 bytes at the head of the row.
void pf(int cxt) const {
  PAQ_PF(&t[cxt*CELLS]);
  PAQ_PF(&t[cxt*CELLS+NIV]);
}

int p(int pr, int cxt, int mul) {
  pr = stretch(pr);
  // same target and rounding as StateMap, hoisted to a per-bit global: both
  // round away from the stored value so the counter can reach PR_MAX.  This was
  // `(y<<PR_BITS)+(y<<rate)-y*2`, which spelled the same thing but as a function
  // of the shift -- and a shift is exactly what there no longer is.
  t[index]   = U16(ema(t[index],   ema_tgt_y, mul, ema_bias_y));
  t[index+1] = U16(ema(t[index+1], ema_tgt_y, mul, ema_bias_y));
  // stretch lands in [-ST_MAX,ST_MAX], so s is in [1,ST_RANGE-1] and s*NIV
  // splits into an interval index below NIV and an ST_RANGE-scale fraction --
  // no dependence on the interval width, which is what makes NIV free.
  const int sn = (pr+ST_LIMIT)*NIV;
  const int w  = sn&(ST_RANGE-1);
  index = (sn>>ST_RANGE_BITS)+cxt*CELLS;
  // U16 entries times weights summing to ST_RANGE, brought back to P_BITS
  return (t[index]*(ST_RANGE-w)+t[index+1]*w)>>(PR_BITS+ST_RANGE_BITS-P_BITS);
}
};

// Templated on its counter, so the state machine it reads its prior from is
// its owning ContextMap's rather than a single global one.
template <int ST> struct StateMap {
int cxt;
U16 t[N_STATES];

StateMap() : cxt(0) {
  for( int i = 0; i<N_STATES; ++i ) {
    int n0 = NEX(ST, i, 2);
    int n1 = NEX(ST, i, 3);
    if( n0==0 )
      n1 *= 128;
    if( n1==0 )
      n0 *= 128;
    // The one division the IDX-declared counts feed.  n0 and n1 are swept
    // six-bit patterns and SM_PRIOR_ADD is a knob too, so the denominator can
    // legitimately reach 0 -- three of the 256 states already have n0==n1==0.
    // Section 5: the clamp belongs here, not in the .idx.
    int den = n0+n1+2*SM_PRIOR_ADD;
    t[i] = PR_ONE*(n1+SM_PRIOR_ADD)/(den<1 ? 1 : den);
  }
}

int p(int cx) {
  int q = t[cxt];
  t[cxt] = U16(ema(q, ema_tgt_y, sm_mul, ema_bias_y));
  return t[cxt = cx]>>PR_SHIFT;
}
};

inline U32 hash(U32 a, U32 b, U32 c = 0xffffffff) {
  U32 h = a*110002499u+b*30005491u+c*50004239u;
  return h^h>>9^a>>3^b>>3^c>>4;
}

template <int B, int I> struct BH {
static constexpr U32 M = 7;

// PAD is not slack for its own sake.  operator[] indexes buckets [0,I-1] but
// then walks up to M-1 further slots from &t[i*B], so the last bucket reads
// p[2] and memmoves up to 23 bytes PAST t[I*B].  The baseline's
// Array<U8,64>(I*B) had that land in untouched heap behind the block; with the
// table as a member it would land on the next member instead, so the same
// amount of never-read zero space has to be here explicitly.  Measured: 31
// such accesses on book1[0:131072] at level 4.
static constexpr int PAD = 64;
alignas(64) U8 t[I*B+PAD];
static const U32 n = I-1;

BH() {
#if !defined(PAQ_LAZY_ZERO)
  memset(t, 0, sizeof(t));
#else
  if( !g_zeroed )
    memset(t, 0, sizeof(t));
#endif
}

// same bucket index operator[] computes, exposed so the walk can be started
// early.  Bucket span is M*B = 28 B, so one line covers it in the common case.
void prefetch(U32 i) const {
  PAQ_PF(&t[(i*M&n)*B]);
}

U8*operator[](U32 i) {
  int chk = (i>>16^i)&0xffff;
  i = i*M&n;
  U8* p = &t[i*B]-B;
  int j;
  for( j = 0; j<M; ++j ) {
    p += B;
    if( p[2]==0 ) {
      *(U16*)p = chk;
      break;
    }
    if( *(U16*)p==chk )
      break;
  }
  if( j==0 )
    return p;
  if( j==M ) {
    --j;
    if( p[2]>p[-2] )
      --j;
  } else
    chk = *(int*)p;
  p = &t[i*4];
#if defined(PAQ_BHMOVE)
  // j is 0..6 and the whole move is <= 24 B inside one bucket; the switch drops
  // the call and the variable-length machinery memmove carries.
  {
    int* q = (int*)p;
    switch( j ) {
      case 6: q[6] = q[5];
      case 5: q[5] = q[4];
      case 4: q[4] = q[3];
      case 3: q[3] = q[2];
      case 2: q[2] = q[1];
      case 1: q[1] = q[0];
      default: break;
    }
  }
#else
  memmove(p+4, p, j*4);
#endif
  *(int*)p = chk;
  return p;
}
};

// CF == -1 reads the global cxtfl (exactly the baseline); CF == 0 or 1 bakes it
// in, which is what PAQ_CXTFL_T uses to turn the per-context branch into two
// instantiations.  Same arithmetic either way.
template <int CF, int ST> inline PAQ_HOTFN int mix2t(MainMixer &m, int slot, int s, StateMap<ST> &sm) {
  int p1 = sm.p(s);
  int n0 = -!NEX(ST, s, 2);
  int n1 = -!NEX(ST, s, 3);
  int st = stretch(p1);
  if( CF<0 ? (cxtfl!=0) : (CF!=0) ) {
    m.put(slot+0, st/4);
    int p0 = P_MAX-p1;
    m.put(slot+1, p_scaled(p1-p0,3,64));
    m.put(slot+2, st*(n1-n0)*3/16);
    m.put(slot+3, p_scaled((p1&n0)-(p0&n1),1,16));
    m.put(slot+4, p_scaled((p0&n0)-(p1&n1),7,64));
    return s>0;
  }
  m.put(slot+0, st*9/32);
  m.put(slot+1, st*(n1-n0)*3/16);
  int p0 = P_MAX-p1;
  m.put(slot+2, p_scaled((p1&n0)-(p0&n1),1,16));
  m.put(slot+3, p_scaled((p0&n0)-(p1&n1),7,64));
  return s>0;
}

template <int ST> inline int mix2(MainMixer &m, int slot, int s, StateMap<ST> &sm) {
  return mix2t<-1,ST>(m, slot, s, sm);
}

template <int MSZ, int mulc> struct RunContextMap {
BH<4, MSZ/4> t;
U8* cp;

RunContextMap() {
  cp = t[0]+2;
}

void prefetch(U32 cx) const {
  t.prefetch(cx);
}

void set(U32 cx) {
  if( cp[0]==0||cp[1]!=b1 )
    cp[0] = 1, cp[1] = b1;
  else if( cp[0]<RCM_RUN_MAX )
    ++cp[0];
  cp = t[cx]+2;
}

// same bpos-constant-folding as ContextMap::mix1t, on a much smaller body:
// three calls per bit, each with two variable shifts.
template <int BP> int pt() {
  if( (cp[1]+256)>>(8-PAQ_BP)==c0 )
    return (((cp[1]>>(7-PAQ_BP))&1)*2-1)*ilog(cp[0]+1)*mulc;
  else
    return 0;
}

int p() {
  return pt<-1>();
}

// Slot-addressed: the caller passes the slot mix_layout.inc gives this map.
// A RunContextMap always contributes exactly one input, so there is no ramp-up
// case here -- unlike a ContextMap, it has no cn to be empty.
int mix(MainMixer &m, int slot) {
#if defined(PAQ_BPOS_T)
  switch( bpos ) {
    case 0:  m.put(slot, pt<0>()); break;
    case 1:  m.put(slot, pt<1>()); break;
    case 2:  m.put(slot, pt<2>()); break;
    case 3:  m.put(slot, pt<3>()); break;
    case 4:  m.put(slot, pt<4>()); break;
    case 5:  m.put(slot, pt<5>()); break;
    case 6:  m.put(slot, pt<6>()); break;
    default: m.put(slot, pt<7>()); break;
  }
#else
  m.put(slot, pt<-1>());
#endif
  return cp[0]!=0;
}
};

template <int MSZ, int mulc> struct SmallStationaryContextMap {
U16 t[MSZ/2];
int cxt;
U16* cp;

SmallStationaryContextMap() : cxt(0) {
  for( U32 i = 0; i<U32(MSZ/2); ++i )
    t[i] = PR_HALF;
  cp = &t[0];
}

void set(U32 cx) {
  cxt = (cx*256)&(U32(MSZ/2)-256);
}

// Slot-addressed, like RunContextMap: one input, always written.
void mix(MainMixer &m, int slot) {
  // An exponential moving average is a linear mix of the stored probability and
  // the target with weight 2^-rate, so the rate IS the weight and the rounding
  // term is half a step -- derived from the rate rather than restated.  Left in
  // shift form rather than pmix()'s P_SCALE weights because this runs ten times
  // per bit and the PR-domain product would need 64-bit arithmetic to hold
  // PR_MAX*PR_ONE.
  const int mul = (pos<SCM_SWITCH_POS) ? SCM_MUL_EARLY : SCM_MUL_LATE;
  // symmetric rounding here, unlike StateMap/APM: nearest rather than away-from-v,
  // and the target is PR_ONE rather than PR_MAX
  *cp = U16(ema(*cp, y<<PR_BITS, mul, EMA_HALF));
  cp = &t[cxt+c0];
  m.put(slot, stretch(*cp>>PR_SHIFT)*mulc/32);
}
};

// ST names this map's counter in IDX/paq8-N0.idx: its own state machine and
// its own decay schedule, folded to literals in the shipping build.
template <U32 MSZ, int NC, int ST> struct ContextMap {
static constexpr int C = NC;
static const int Sz = int((MSZ>>6)-1);

struct E {
U16 chk[7];
U8 last;

U8 bh[7][7];

PAQ_HOTFN U8*get(U16 ch, int j) {
  ch += j;
  if( chk[last&15]==ch )
    return &bh[last&15][0];
#if defined(PAQ_GETSIMD)
  // one 16-byte load covers chk[0..6] (lane 7 is `last`+bh[0][0], masked off);
  // the lowest matching lane is the same entry the scalar loop returns first.
  int mk = _mm_movemask_epi8(_mm_cmpeq_epi16(_mm_loadu_si128((const __m128i*)&chk[0]),
                                             _mm_set1_epi16((short)ch)))&0x3fff;
  if( mk ) {
    int i = PAQ_CTZ(mk)>>1;
    return last = last<<4|i, &bh[i][0];
  }
  int b = 0xffff, bi = 0;
  for( int i = 0; i<7; ++i ) {
    int pri = bh[i][0];
    if( (last&15)!=i&&last>>4!=i&&pri<b )
      b = pri, bi = i;
  }
#else
  int b = 0xffff, bi = 0;
  for( int i = 0; i<7; ++i ) {
    if( chk[i]==ch )
      return last = last<<4|i, &bh[i][0];
    int pri = bh[i][0];
    if( (last&15)!=i&&last>>4!=i&&pri<b )
      b = pri, bi = i;
  }
#endif
  return last = 0xf0|bi, chk[bi] = ch, (U8*)memset(&bh[bi][0], 0, 7);
}
};

alignas(64) E t[MSZ>>6];
U8* cp[NC];
U8* cp0[NC];
U32 cxt[NC];
U8* runp[NC];
StateMap<ST> sm[NC];
int cn;

// the probe address for every context is known before the walk starts, and E is
// 64 B at alignas(64) so one prefetch covers a whole entry.  Issuing all cn of
// them up front means context i's line arrives under the state updates and mix2
// work of contexts 0..i-1 - with 46 contexts in WordModel's map that is a long
// cover.  bpos 1/3/6 and 4/7 only re-slice cp0 inside the already-resident line,
// so only 0/2/5 need it.
void pfprobe(int cc) const {
  for( int i = 0; i<cn; ++i )
    PAQ_PF(&t[(cxt[i]+cc)&Sz]);
}

// speculative: at bpos 1/4 the next bit's probe is one of two addresses
// (cc' = 2*cc+bit), so fetch both and buy an entire bit of mixer work as cover
// at 2x the line traffic.
void pfprobe2(int cc) const {
  for( int i = 0; i<cn; ++i ) {
    PAQ_PF(&t[(cxt[i]+2*cc)&Sz]);
    PAQ_PF(&t[(cxt[i]+2*cc+1)&Sz]);
  }
}

// BP == -1 reads the global `bpos` (exactly the baseline); BP in 0..7 bakes it
// in, which is what PAQ_BPOS_T uses.  `bpos` is constant for the whole call but
// the loop below re-tests it up to five times per context and feeds it to two
// *variable* shifts, and the loop runs cn times - up to 46 for WordModel's map,
// 58 contexts per bit across the four owners.  Baking it in collapses the
// four-way chain to the one arm actually taken, turns `>>(8-bpos)` and
// `>>(7-bpos)` into constant shifts, and resolves the `bpos==7` reset and the
// prefetch guards at compile time.  Costs 8 instantiations of this body.
// `base` is this map's first slot from mix_layout.inc.  Each context owns a
// fixed stride from it -- the run input, then mix2t's terms -- so the write
// positions are known before the loop runs instead of being wherever the cursor
// happened to reach.  The stride follows cxtfl exactly as mix2t's branch does,
// and with PAQ_CXTFL_T it is a compile-time constant like everything else here.
template <int CF, int BP> PAQ_HOTFN int mix1t(MainMixer &m, int base, int cc, int c1, int y1) {
  const int W = (CF<0 ? (cxtfl!=0) : (CF!=0)) ? MIXIN_CM : MIXIN_CM0;

#if defined(PAQ_PREFETCH)
  if( PAQ_BP==0||PAQ_BP==2||PAQ_BP==5 )
    pfprobe(cc);
#endif
#if defined(PAQ_PF2)
  if( PAQ_BP==1||PAQ_BP==4 )
    pfprobe2(cc);
#endif

  int result = 0;
  for( int i = 0; i<cn; ++i ) {
    U8* cpi = cp[i];
    if( cpi ) {
      int ns = NEX(ST, *cpi, y1);
      // PSH is not decoration.  CM_DECAY_BIAS is a live nine-bit knob and
      // opt.pl visits both ends of it, so without a clamp any value below
      // CM_DECAY_FROM makes this shift count negative -- which is undefined,
      // and which the two builds resolved DIFFERENTLY: the shipping build
      // folds the constants, proves the branch undefined and deletes the decay
      // outright, while the tuning build cannot prove it and executes the shl
      // with x86's count masked to five bits.  That single line was the whole
      // reason the two builds did not agree.  Section 5, exactly: a pattern is
      // the search space and the clamp is the contract.
      if( ns>=CTR_DFROM(ST)&&(rnd()<<PSH((CTR_DBIAS(ST)-ns)>>CTR_DSHIFT(ST))) )
        ns -= CTR_DSTEP(ST);
      *cpi = ns;
    }

    if( PAQ_BP>1&&runp[i][0]==0 )
      cpi = 0;
    else if( PAQ_BP==1||PAQ_BP==3||PAQ_BP==6 )
      cpi = cp0[i]+1+(cc&1);
    else if( PAQ_BP==4||PAQ_BP==7 )
      cpi = cp0[i]+3+(cc&3);
    else {
      cp0[i] = cpi = t[(cxt[i]+cc)&Sz].get(cxt[i]>>16, i);

      if( PAQ_BP==0 ) {
        if( cpi[3]==2 ) {
          const int c = cpi[4]+256;
          U8* p = t[(cxt[i]+(c>>6))&Sz].get(cxt[i]>>16, i);
          p[0] = 1+((c>>5)&1);
          p[p[0]] = 1+((c>>4)&1);
          p[3+((c>>4)&3)] = 1+((c>>3)&1);
          p = t[(cxt[i]+(c>>3))&Sz].get(cxt[i]>>16, i);
          p[0] = 1+((c>>2)&1);
          p[p[0]] = 1+((c>>1)&1);
          p[3+((c>>1)&3)] = 1+(c&1);
          cpi[6] = 0;
        }

        U8 c0 = runp[i][0];

        if( c0==0 )
          c0 = 2, runp[i][1] = c1;
        else if( runp[i][1]!=c1 )
          c0 = 1, runp[i][1] = c1;
        else if( c0<CM_RUN_MAX )
          c0 += CM_RUN_STEP;
        runp[i][0] = c0;
        runp[i] = cpi+3;
      }
    }

    int rc = runp[i][0];
    if( (runp[i][1]+256)>>(8-PAQ_BP)==cc ) {
      int b = ((runp[i][1]>>(7-PAQ_BP))&1)*2-1;
      int c = ilog(rc+1);
      if( rc&1 )
        c = (c*15)/4;
      else
        c *= 13;
      m.put(base+i*W, b*c);
    } else
      m.put(base+i*W, 0);

    result += mix2t<CF,ST>(m, base+i*W+1, cpi ? *cpi : 0, sm[i]);
    cp[i] = cpi;
  }
  if( PAQ_BP==7 )
    cn = 0;
  return result;
}

int mix1(MainMixer &m, int base, int cc, int c1, int y1) {
  return mix1t<-1,-1>(m, base, cc, c1, y1);
}

ContextMap() : cn(0) {
#if !defined(PAQ_LAZY_ZERO)
  memset(t, 0, sizeof(t));
#else
  if( !g_zeroed )
    memset(t, 0, sizeof(t));
#endif
  for( int i = 0; i<C; ++i ) {
    cxt[i] = 0;
    cp0[i] = cp[i] = &t[0].bh[0][0];
    runp[i] = cp[i]+3;
  }
}

void set(U32 cx) {
  int i = cn++;
  cx = cx*123456791+i;
  cx = cx<<16|cx>>16;
  cxt[i] = cx*987654323+i;
}

#if defined(PAQ_CXTFL_T)
  static constexpr int CF3 = 1, CF0 = 0;   // cxtfl baked in per call site
#else
  static constexpr int CF3 = -1, CF0 = -1; // read the global, as the baseline does
#endif

// Dispatch on bpos once per call so the body below sees it as a constant.
// Without PAQ_BPOS_T this is a single call with BP == -1 and the global is read,
// bit for bit the baseline.
template <int CF> int mixbp(MainMixer &m, int base) {
  mix_ctxcheck(cn, C);
#if defined(PAQ_BPOS_T)
  switch( bpos ) {
    case 0:  return mix1t<CF,0>(m, base, c0, b1, y);
    case 1:  return mix1t<CF,1>(m, base, c0, b1, y);
    case 2:  return mix1t<CF,2>(m, base, c0, b1, y);
    case 3:  return mix1t<CF,3>(m, base, c0, b1, y);
    case 4:  return mix1t<CF,4>(m, base, c0, b1, y);
    case 5:  return mix1t<CF,5>(m, base, c0, b1, y);
    case 6:  return mix1t<CF,6>(m, base, c0, b1, y);
    default: return mix1t<CF,7>(m, base, c0, b1, y);
  }
#else
  return mix1t<CF,-1>(m, base, c0, b1, y);
#endif
}

int mix(MainMixer &m, int base) {
  return mixbp<CF3>(m, base);
}

// RecordModel's cm/cn/cq run with cxtfl == 0; under PAQ_CXTFL_T that becomes a
// second instantiation instead of a per-context branch.
int mix0(MainMixer &m, int base) {
  return mixbp<CF0>(m, base);
}

// hoist the whole probe set above whatever the caller does next
void pfset() const {
  pfprobe(c0);
}
};

// ---------------------------------------------------------------------------
// Character set.
//
// This model was fitted on WRT-preprocessed text, and WRT permutes the
// alphabet.  So the byte literals in the model below are not the characters
// they look like: the sentence-end set spelled {'.','O','M','!',')','R'} is
// really {'.','?','=','!',')','}'} -- period, question mark, the wiki heading
// marker, bang, close paren, close brace, which is what a sentence ends with in
// enwik.  Reading 'O' as the letter O is how §10.6 of the analysis happens.
//
// So every character the model tests is written in ASCII and wrapped in WL().
// WL('?') is 'O' in a WRT build and '?' in an ASCII one; the source says which
// character is meant and the build says how that character is spelled.
//
// The permutation is its own inverse, which is why one function serves for
// both directions and why WL(WL(c)) == c.
// ---------------------------------------------------------------------------

// PAQ_CHARSET_WRT=1 (default) -- the alphabet the model was tuned against, for
// input that has been through WRT.  =0 -- raw bytes, for feeding the tool text
// directly.  It changes the bitstream, so encoder and decoder must be built
// alike; they are, being one binary.
#ifndef PAQ_CHARSET_WRT
  #define PAQ_CHARSET_WRT 1
#endif

// constexpr so WL() folds to a literal at every use.
constexpr int wrt_remap( int c ) {
  if( c>='{'&&c<127 )                             c += 'P'-'{';
  else if( c>='P'&&c<'T' )                        c -= 'P'-'{';
  else if( (c>=':'&&c<='?')||(c>='J'&&c<='O') )   c ^= 0x70;
  if( c=='X'||c=='`' )                            c ^= 'X'^'`';
  return c;
}

#if PAQ_CHARSET_WRT
  #define WL(c) (wrt_remap(c))
#else
  #define WL(c) (c)
#endif

// The two sentence-end sets, in ASCII.  They differ, and that is not a typo:
// WordModel's word-stack reset uses three of them and the byte-state update
// uses six (§10.7 of the analysis).  Preserved rather than unified, because
// which is right is a measurement nobody has run.
#define PAQ_EOS_WORD(c) ((c)==WL('.')||(c)==WL('?')||(c)==WL('}'))
#define PAQ_EOS_BYTE(c) ((c)==WL('.')||(c)==WL('?')||(c)==WL('=')|| \
                         (c)==WL('!')||(c)==WL(')')||(c)==WL('}'))

// A byte's contribution to a word.  WRT carries capitalisation out of band, so
// an uppercase byte in a WRT stream is a transform artefact and correctly ends
// a word; in raw text it is the middle of a sentence, and leaving it out means
// "The" is a word break followed by a two-letter word, with no capitalised
// token ever sharing a hash with its lowercase form.
//
// The escapes are the same story.  6, 8, 12 and the >127 range are WRT
// dictionary codes; in raw text 6 and 8 are control characters and >127 is
// ordinary high-bit text, so only the last stays a word character there.
#if PAQ_CHARSET_WRT
  #define PAQ_WORDFOLD(c)      (c)
  #define PAQ_WORDEXTRA(c, p)  ((c)==8||(c)==6||((c)>127&&(p)!=12))
#else
  #define PAQ_WORDFOLD(c)      ((U32((c))-'A')<=U32('Z'-'A') ? (c)+('a'-'A') : (c))
  #define PAQ_WORDEXTRA(c, p)  ((c)>127)
#endif

static U32 col, frstchar = 0, spafdo = 0, spaces = 0, spacecount = 0, words = 0, wordcount = 0, fails = 0, failz = 0, failcount = 0;

// The original WordModel, kept for reference under #if 0.  The live class is
// in wordmodel.inc, rebuilt from WORDMODEL.md and verified to produce the same
// archive; its tunables are the same IDX/paq8-W0.idx module.  The wide-mask
// constants travel with the class, so they sit inside the #if 0 too.
#if 0
// WordModel's wide masks, reassembled from the byte lanes IDX/paq8-W0.idx
// declares them as.  The HA..HF group windows h, which is w4 -- two bits per
// byte -- brought up to byte alignment and then shifted twice more; TA..TD
// window the t1/t2 chains; XA..XD are the sparse views of x4 and c4.  A lane is
// the register's own field, and it is also as wide a pattern as IDX can spell,
// since mapping::value accumulates into a signed int.
const U32 W0_HA_MASK = LANES4(W0_HA_M3, W0_HA_M2, W0_HA_M1, W0_HA_M0);
const U32 W0_HB_MASK = LANES4(W0_HB_M3, W0_HB_M2, W0_HB_M1, W0_HB_M0);
const U32 W0_HC_MASK = LANES4(W0_HC_M3, W0_HC_M2, W0_HC_M1, W0_HC_M0);
const U32 W0_HD_MASK = LANES4(W0_HD_M3, W0_HD_M2, W0_HD_M1, W0_HD_M0);
const U32 W0_HE_MASK = LANES4(W0_HE_M3, W0_HE_M2, W0_HE_M1, W0_HE_M0);
const U32 W0_HF_MASK = LANES4(W0_HF_M3, W0_HF_M2, W0_HF_M1, W0_HF_M0);
const U32 W0_F_MASK  = LANES4(W0_F_M3,  W0_F_M2,  W0_F_M1,  W0_F_M0);
const U32 W0_TA_MASK = LANES4(W0_TA_M3, W0_TA_M2, W0_TA_M1, W0_TA_M0);
const U32 W0_TB_MASK = LANES4(W0_TB_M3, W0_TB_M2, W0_TB_M1, W0_TB_M0);
const U32 W0_TC_MASK = LANES4(W0_TC_M3, W0_TC_M2, W0_TC_M1, W0_TC_M0);
const U32 W0_TD_MASK = LANES4(W0_TD_M3, W0_TD_M2, W0_TD_M1, W0_TD_M0);
const U32 W0_XA_MASK = LANES4(W0_XA_M3, W0_XA_M2, W0_XA_M1, W0_XA_M0);
const U32 W0_XB_MASK = LANES4(W0_XB_M3, W0_XB_M2, W0_XB_M1, W0_XB_M0);
const U32 W0_XC_MASK = LANES4(W0_XC_M3, W0_XC_M2, W0_XC_M1, W0_XC_M0);
const U32 W0_XD_MASK = LANES4(W0_XD_M3, W0_XD_M2, W0_XD_M1, W0_XD_M0);
const U32 W0_D_MASK  = U32(LANES2(W0_D_M1, W0_D_M0));
const U32 W0_F4_MASK = U32(LANES2(W0_F4_M1, W0_F4_M0));

template <int L> struct WordModel {
U32 word0, word1, word2, word3, word4;
ContextMap<((U32)MEM(L)*16), W_CM_CTX, CTR_W> cm;
int nl1, nl;
U32 t1[256];
U16 t2[0x10000];

WordModel() : word0(0), word1(0), word2(0), word3(0), word4(0), nl1(-3), nl(-2) {
  memset(t1, 0, sizeof(t1));
  memset(t2, 0, sizeof(t2));
}

void mix(MainMixer &m) {  if( bpos==0 ) {
    U32 c = b1, f = 0;

    if( spaces&0x80000000 )
      --spacecount;
    if( words&0x80000000 )
      --wordcount;
    spaces = spaces*2;
    words = words*2;

    U32 wc = PAQ_WORDFOLD(c);
    if( (wc-U32(WL('a')))<=U32(WL('z')-WL('a'))||PAQ_WORDEXTRA(c, b2) ) {
      ++words, ++wordcount;
      word0 = word0*W0_WORD0_MUL+wc;
    } else {
      if( c==WL(' ')||c==WL('\n') ) {
        ++spaces, ++spacecount;
        if( c==WL('\n') )
          nl1 = nl, nl = pos-1;
      }
      if( word0 ) {
        word4 = word3*W0_WORD4_MUL;
        word3 = word2*W0_WORD3_MUL;
        word2 = word1*W0_WORD2_MUL;
        word1 = word0*W0_WORD1_MUL;
        word0 = 0;
        if( PAQ_EOS_WORD(c) )
          f = 1, spafdo = 0;
        else {
          ++spafdo;
          spafdo = min(U32(W0_SPAFDO_CAP), spafdo);
        }
      }
    }

    U32 h = word0*W0_H_MUL+c;
    cm.set(word0);
    cm.set(h+word1);
    cm.set(word0*W0_P0_W0+word1*W0_P0_W1);
    cm.set(h+word1*W0_P1_W1+word2*W0_P1_W2);

    cm.set(h+word2);
    cm.set(h+word3);
    cm.set(h+word4);
    cm.set(h+word1*W0_P2_W1+word3*W0_P2_W3);
    cm.set(h+word2*W0_P3_W2+word3*W0_P3_W3);

    if( f ) {
      word4 = word3*W0_FWORD4_MUL;
      word3 = word2*W0_FWORD3_MUL;
      word2 = word1*W0_FWORD2_MUL;
      word1 = '.';
    }

    cm.set(W0_MakeBb34(b4, b3));
    cm.set(spafdo*W0_SPAFDO_MUL*((w4&W0_SPAFDO_W4M)==U32(W0_SPAFDO_EQ)));

    col = min(W0_COL_CAP, pos-nl);
    if( col<=U32(W0_COL_TH) ) {
      if( col==U32(W0_COL_TH) )
        frstchar = min(c, U32(W0_FRST_CAP));
      else
        frstchar = 0;
    }
    if( frstchar==U32(WL('['))&&c==WL(' ') ) {
      if( b3==U32(WL(']'))||b4==U32(WL(']')) )
        frstchar = W0_FRST_CAP;
    }
    cm.set(frstchar<<PSH(W0_FRST_SH)|c);

    int above = buf[nl1+col];

    cm.set(W0_MakeCca(col, c, above));
    cm.set(W0_MakeCc(col, c));
    cm.set(col*(c==U32(W0_COLSP_EQ)));

    h = W0_MakeWsp(wordcount, spacecount);
    cm.set(spaces&W0_SPACES_M1);
    cm.set(frstchar<<PSH(W0_FRST_SH2));
    cm.set(spaces&W0_SPACES_M0);
    cm.set(W0_MakeCsp(c, spacecount>>PSH(W0_SC_SH)));
    cm.set((c<<PSH(W0_C_SH13))+h);
    cm.set(h);

    U32 d = c4&W0_D_MASK;
    h = w4<<PSH(W0_H_SH);
    cm.set(c+(h&W0_HA_MASK));
    cm.set(c+(h&W0_HB_MASK));
    cm.set(c+(h&W0_HC_MASK));
    h <<= PSH(W0_H_SH);
    cm.set(d+(h&W0_HD_MASK));
    cm.set(d+(h&W0_HE_MASK));
    h <<= PSH(W0_H_SH), f = c4&W0_F_MASK;
    cm.set(f+(h&W0_HF_MASK));

    U16 &r2 = t2[f>>8];
    r2 = r2<<8|c;
    U32 &r1 = t1[d>>8];
    r1 = r1<<8|c;
    U32 t = c|t1[c]<<PSH(W0_T1_SH);
    cm.set(t&W0_TA_MASK);
    cm.set(t&W0_TB_MASK);
    cm.set(t);
    cm.set(t&W0_TC_MASK);
    t = d|t2[d]<<PSH(W0_T2_SH);
    cm.set(t&W0_TD_MASK);
    cm.set(t);

    cm.set(x4&W0_XA_MASK);
    cm.set(x4&W0_XB_MASK);
    cm.set(x4&W0_XC_MASK);
    cm.set(c4&W0_XD_MASK);
    cm.set(W0_MakeBp5(b5, c)+(W0_TAG_A<<PSH(W0_TAG_SH)));
    cm.set(W0_MakeBp6(b6, c)+(W0_TAG_B<<PSH(W0_TAG_SH)));
    cm.set(W0_MakeBp8(b8, b4)+(W0_TAG_C<<PSH(W0_TAG_SH)));

    cm.set(d);
    cm.set(w4&W0_W4_M15);
    cm.set(f4);
    cm.set((w4&W0_W4_M63)*W0_W4_MUL+(W0_TAG_D<<PSH(W0_TAG_SH)));
    cm.set(d<<PSH(W0_D_SH9)|frstchar);
    cm.set((f4&W0_F4_MASK)<<PSH(W0_F4_SH11)|frstchar);
  }
  cm.mix(m, MIXB_W + W_cm);
}
};
#endif

#include "wordmodel.inc"

// RecordModel's two wide masks, reassembled from the byte lanes
// IDX/paq8-R0.idx declares them as.  d takes the whole previous byte and the
// high nibble of the one before; e takes the last three bytes of c4, with the
// fourth lane seeded to zero so opt.pl can bring it in a bit at a time.
const int R0_D_MASK = LANES2(R0_D_M1, R0_D_M0);
const U32 R0_E_MASK = LANES4(R0_E_M3, R0_E_M2, R0_E_M1, R0_E_M0);

// every size here is a literal, so this one needs no level parameter
struct RecordModel {
int cpos1[256];
int wpos1[0x10000];
ContextMap<32768/4, R_CM_CTX, CTR_RM> cm;
ContextMap<32768/2, R_CN_CTX, CTR_RN> cn;
ContextMap<32768, R_CO_CTX, CTR_RO> co;
ContextMap<32768*2, R_CP_CTX, CTR_RP> cp;
ContextMap<32768*4, R_CQ_CTX, CTR_RQ> cq;

RecordModel() {
  memset(cpos1, 0, sizeof(cpos1));
  memset(wpos1, 0, sizeof(wpos1));
}

void mix(MainMixer &m) {
  if( !bpos ) {
    int c = b1, w = (b2<<8)+c, d = w&R0_D_MASK, e = c4&R0_E_MASK;
    // The two gap buckets.  `/4` was a shift written as a division -- both
    // operands are non-negative here, cpos1/wpos1 only ever hold a past pos --
    // so it is spelled as the shift it is, and the shift is now the knob.
    cm.set(R0_MakeCm0(c, min(R0_CM0_CAP, pos-cpos1[c])>>PSH(R0_CM0_SH)));
    cm.set(R0_MakeCm1(w, llog(pos-wpos1[w])>>PSH(R0_CM1_SH)));
    cn.set(w);
    cn.set(d<<PSH(R0_CN1_SH));
    cn.set(c<<PSH(R0_CN2_SH));
    cn.set((f4&R0_CN3_MASK)<<PSH(R0_CN3_SH));
    // the record width the model is guessing at
    int col = pos&R0_CN4_MASK;
    cn.set(col|R0_CN4_TAG);

    co.set(c);
    co.set(w<<PSH(R0_CO1_SH));
    co.set(w5&R0_CO2_MASK);
    co.set(e<<PSH(R0_CO3_SH));

    cp.set(d);
    cp.set(c<<PSH(R0_CP1_SH));
    cp.set(w<<PSH(R0_CP2_SH));

    cq.set(w<<PSH(R0_CQ0_SH));
    cq.set(c<<PSH(R0_CQ1_SH));
    cq.set(e);

    cpos1[c] = pos;
    wpos1[w] = pos;
  }
  co.mix ( m, MIXB_R + R_co  );
  cp.mix ( m, MIXB_R + R_cp  );
  cxtfl = 0;
  cm.mix0( m, MIXB_R + R_cm0 );
  cn.mix0( m, MIXB_R + R_cn0 );
  cq.mix0( m, MIXB_R + R_cq0 );
  cxtfl = 3;
}
};

// SparseModel's two 32-bit x4 masks, reassembled from the four byte lanes
// IDX/paq8-S0.idx declares them as.  x4 is a byte-per-byte shift register, so a
// lane IS the register's field and moving one moves a byte of history; a
// full-width pattern is also not spellable, because mapping::value accumulates
// into a signed int and thirty-two bits overflow it.
const U32 S0_CN2_MASK = LANES4(S0_CN2_M3, S0_CN2_M2, S0_CN2_M1, S0_CN2_M0);
const U32 S0_CN4_MASK = LANES4(S0_CN4_M3, S0_CN4_M2, S0_CN4_M1, S0_CN4_M0);

template <int L> struct SparseModel {
ContextMap<((U32)(MEM(L)*2)), S_CN_CTX, CTR_S> cn;
SmallStationaryContextMap<0x20000, 17> scm1;
SmallStationaryContextMap<0x20000, 12> scm2;
SmallStationaryContextMap<0x20000, 12> scm3;
SmallStationaryContextMap<0x20000, 13> scm4;
SmallStationaryContextMap<0x10000, 12> scm5;
SmallStationaryContextMap<0x20000, 12> scm6;
SmallStationaryContextMap<0x2000, 12> scm7;
SmallStationaryContextMap<0x8000, 13> scm8;
SmallStationaryContextMap<0x4000, 12> scm9;
SmallStationaryContextMap<0x10000, 16> scma;

void mix(MainMixer &m) {
  if( bpos==0 ) {
    cn.set(words&S0_CN0_MASK);
    cn.set((f4&S0_CN1_MASK)*S0_CN1_MUL);
    cn.set((x4&S0_CN2_MASK)+S0_CN2_TAG);
    cn.set((tt&S0_CN3_MASK)*S0_CN3_MUL);
    cn.set((x4&S0_CN4_MASK)+S0_CN4_TAG);
    scm1.set(b1);
    scm2.set(b2);
    scm3.set(b3);
    scm4.set(b4);
    scm5.set(words&S0_SCM5_MASK);
    scm6.set(S0_MakeScm6(words, w4, b1));
    scm7.set(w4&S0_SCM7_MASK);
    scm8.set(spafdo*((w4&S0_SCM8_MASK)==U32(S0_SCM8_EQ)));
    scm9.set(col*(b1==U32(S0_SCM9_EQ)));
    scma.set(frstchar);
  }
  cn.mix(m, MIXB_S + S_cn);
  scm1.mix(m, MIXB_S+S_scm1);  scm2.mix(m, MIXB_S+S_scm2);
  scm3.mix(m, MIXB_S+S_scm3);  scm4.mix(m, MIXB_S+S_scm4);
  scm5.mix(m, MIXB_S+S_scm5);  scm6.mix(m, MIXB_S+S_scm6);
  scm7.mix(m, MIXB_S+S_scm7);  scm8.mix(m, MIXB_S+S_scm8);
  scm9.mix(m, MIXB_S+S_scm9);  scma.mix(m, MIXB_S+S_scma);
}
};

// The order-1..order-13 hash multipliers, from IDX/paq8-C0.idx.  primes[0] is
// never read -- the loops below run from 1 -- so it stays a literal zero.
int primes[] = {0,
  C0_PRIME1,  C0_PRIME2,  C0_PRIME3,  C0_PRIME4,  C0_PRIME5,  C0_PRIME6,
  C0_PRIME7,  C0_PRIME8,  C0_PRIME9,  C0_PRIME10, C0_PRIME11, C0_PRIME12,
  C0_PRIME13 };
static U32 WRT_mpw[16] = {3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0}, tri[4] = {0, 4, 3, 7}, trj[4] = {0, 6, 6, 12};
static U32 WRT_mtt[16] = {0, 0, 1, 2, 3, 4, 5, 5, 6, 6, 6, 6, 6, 7, 7, 7};

template <int L> struct ContextModel {
ContextMap<((U32)MEM(L)*16), X_CM_CTX, CTR_X> cm;
RunContextMap<(int)(MEM(L)/4), 14> rcm7;
RunContextMap<(int)(MEM(L)/4), 18> rcm9;
RunContextMap<(int)(MEM(L)/2), 20> rcm10;
MainMixer m;
U32 cxt[16];
int size;

WordModel<L> wordModel;
SparseModel<L> sparseModel;
RecordModel recordModel;

ContextModel() : size(0) {
  memset(cxt, 0, sizeof(cxt));
}

int p() {  if( bpos==0 ) {
    --size;
    if( size==-5 ) {
      size = c4;
    }
  }

  m.update();
  // The bias, then the three RunContextMaps, then the core map -- each into the
  // slot mix_layout.inc names for it.
  m.put( MIXB_X + X_bias, 64 );

  if( bpos==0 ) {
    int i = 0, f2 = buf(2);

    if( PAQ_EOS_BYTE(f2) ) {
      if( b1!=(unsigned int)f2&&buf(3)!=f2 )
        i = 13, x4 = x4*256+f2;
    }

    for(; i>0; --i )
      cxt[i] = cxt[i-1]*primes[i];

    for( i = 13; i>0; --i )
      cxt[i] = cxt[i-1]*primes[i]+b1;

    cm.set(cxt[3]);
    cm.set(cxt[4]);
    cm.set(cxt[5]);
    cm.set(cxt[6]);
    cm.set(cxt[8]);
    cm.set(cxt[13]);
    cm.set(0);
    // Orders 1 and 2 had no bit-history context here at all -- RecordModel's
    // small maps were the only ones covering them, and only at L>=4.  Appended
    // rather than inserted: the mul block below addresses contexts by position,
    // so anything put in front of cm.set(0) would silently rescale a different
    // set of them.
    cm.set(cxt[1]);
    cm.set(cxt[2]);

#if defined(PAQ_RCMPF)
    // the three RunContextMap::set() calls below are themselves hashed BH walks
    // into MEM/4..MEM/2 tables, and cm's own prefetch does not happen until
    // mix1 runs after them.  Issue all ten lines here instead, so the rcm walks
    // and cm's entries are in flight together.
    rcm7.prefetch(cxt[7]);
    rcm9.prefetch(cxt[9]);
    rcm10.prefetch(cxt[11]);
    cm.pfset();
#endif

    rcm7.set(cxt[7]);
    rcm9.set(cxt[9]);
    rcm10.set(cxt[11]);

    x4 = x4*256+b1;
  }
  rcm7.mix ( m, MIXB_X + X_rcm7  );
  rcm9.mix ( m, MIXB_X + X_rcm9  );
  rcm10.mix( m, MIXB_X + X_rcm10 );
  order = cm.mix(m, MIXB_X + X_cm)-1;
  if( order<0 )
    order = 0;
  // Rewind and rescale four of the core map's contexts in place: orders 6 and
  // 8 by MIX_MUL_0, order 13 by MIX_MUL_1, order 0 by MIX_MUL_2.  Addressed
  // through mix_layout.inc rather than by measuring m.nx -- the block used to
  // divide by a literal 7 and rely on 3+2+1+1 summing to the same 7 to land the
  // cursor back where cm.mix left it, both of which stopped being true when the
  // map went to nine contexts.  Now the start, the stride and the end are all
  // the layout's business and none of them is inferred.
  // Rescale four of the core map's contexts in place: orders 6 and 8 by
  // MIX_MUL_0, order 13 by MIX_MUL_1, order 0 by MIX_MUL_2.  Unconditional now
  // -- during the first byte the map has written nothing and those slots are
  // still the zeros tx was constructed with, and scaling zero gives zero.
  {
    int sl = MIXB_X + X_cm + 3*MIXIN_CM;
    for( int n = 2*MIXIN_CM; n; --n ) m.mulat(sl++, MIX_MUL_0);
    for( int n = 1*MIXIN_CM; n; --n ) m.mulat(sl++, MIX_MUL_1);
    for( int n = 1*MIXIN_CM; n; --n ) m.mulat(sl++, MIX_MUL_2);
  }

  if( L>=4 ) {
    wordModel.mix(m);
    sparseModel.mix(m);
    recordModel.mix(m);
  }

  U32 c1 = b1, c2 = b2, c;
  if( c1==WL('\t')||c1==WL('\n')||c1==WL(' ') )
    c1 = 16;
  if( c2==WL('\t')||c2==WL('\n')||c2==WL(' ') )
    c2 = 16;

  // The six mixer context selectors.  Four of them are exact packings of a few
  // small variables and are declared in IDX/paq8-G0.idx, so the packing and its
  // row count come from one place instead of being an expression here and a
  // literal range next to it.  MIX_R3 and MIX_R5 are the two that are not: one
  // is a two-branch expression over bpos, the other adds two fields that can
  // carry into each other, and an Index for either would misdescribe it.
  m.set(G0_MakeMX0(order, w4, c2), G0_MX0_Volume);

  m.set(G0_MakeMX1(order, w4, words>>1), G0_MX1_Volume);

  m.set(G0_MakeMX2(bpos, w4), G0_MX2_Volume);

  if( bpos ) {
    c = c0<<(8-bpos);
    if( bpos==1 )
      c += b3/2;
    c = (min(bpos, 5))*256+(tt&63)+(c&192);
  } else
    c = (words&12)*16+(tt&63);
  m.set(c, MIX_R3);

  c2 = (c0<<(8-bpos))|(c1>>bpos);
  m.set(G0_MakeMX4(order, c2, bpos), G0_MX4_Volume);

  c = bpos*256+((c0<<(8-bpos))&255);
  c1 = (words<<bpos)&255;
  m.set(c+(c1>>bpos), MIX_R5);

  // Every input is placed by slot now, so nx is not a cursor any more -- it is
  // just the length of the vector, and the vector is the whole layout.  Slots a
  // submodel has not written yet (the first byte, or a model gated off by level)
  // hold the zeros tx was constructed with, and a zero input contributes zero to
  // every dot product and moves no weight, so their presence is free.
  m.seek( MIX_INPUTS );

  return m.p();
}
};

template <int L> struct Predictor {
int pr;

ContextModel<L> contextModel;

APM<256,     A1_NH> a1;
APM<0x8000,  A2_NH> a2;
APM<0x8000,  A3_NH> a3;
APM<0x20000, A4_NH> a4;
APM<0x10000, A5_NH> a5;
APM<0x10000, A6_NH> a6;

Predictor() : pr(P_HALF) {
}

int p() const {
  return pr;
}

// was PAQ8HP::Perceive
void Perceive(int bit) {
  y = bit;
  ema_tgt_y  = PR_MAX   & (-bit);
  ema_bias_y = EMA_CEIL & (-bit);
  update();
}

void update() {
  c0 += c0+y;
  if( c0>=256 ) {
    buf[pos++] = c0;
    c0 -= 256;
    if( pos<=SM_SWITCH_2 ) {
      if( pos==SM_SWITCH_2 )
        sm_mul = SM_MUL_2;
      if( pos==SM_SWITCH_1 )
        sm_mul = SM_MUL_1;
      // the `sm_add_y = sm_add&(-y)` that used to close this block is gone:
      // it existed because sm_add carried the rate's rounding and so changed
      // with the rate.  ema_tgt_y/ema_bias_y do not depend on the rate at all,
      // and y has not moved since Perceive set them, so the recompute was a
      // no-op the moment the rounding became a product bias.
    }
    int i = WRT_mpw[c0>>4];
    w4 = w4*4+i;
    if( b1==12 )
      i = 2;
    w5 = w5*4+i;
    b8 = b7, b7 = b6, b6 = b5, b5 = b4, b4 = b3;
    b3 = b2;
    b2 = b1;
    b1 = c0;
    if( PAQ_EOS_BYTE(c0) ) {
      w5 = (w5<<8)|0x3ff, x5 = (x5<<8)+c0, f4 = (f4&0xfffffff0)+2;
      if( c0!=U32(WL('!'))&&c0!=U32(WL('?')) )
        w4 |= 12;
      if( c0!=U32(WL('!')) )
        b2 = WL('.'), tt = (tt&0xfffffff8)+1;
    }
    c4 = (c4<<8)+c0;
    x5 = (x5<<8)+c0;
    if( c0==U32(WL(' ')) )
      --c0;
    f4 = f4*16+(c0>>4);
    tt = tt*8+WRT_mtt[c0>>4];
    c0 = 1;
  }
  bpos = (bpos+1)&7;

  if( fails&0x00000080 )
    --failcount;
  fails = fails*2;
  failz = failz*2;
  if( y )
    pr ^= P_MAX;
  if( pr>=FAIL_TH )
    ++fails, ++failcount;
  if( pr>=FAILZ_TH )
    ++failz;

#if defined(PAQ_APMPF)
  // Every APM context here is a function of state that is already final at this
  // point (c0/b1/x5/w5/fails/failz/failcount are all updated above), so the six
  // hashes can be computed before contextModel.p() and their regions prefetched
  // under the whole model call.  Pure reordering of pure computation.
  // one schedule per APM now, instead of one shared `rate` for five of them
  const int m2 = A2_MUL>>((pos>A2_SW1)+(pos>A2_SW2));
  const int m3 = A3_MUL>>((pos>A3_SW1)+(pos>A3_SW2));
  const int m4 = A4_MUL>>((pos>A4_SW1)+(pos>A4_SW2));
  const int m5 = A5_MUL>>((pos>A5_SW1)+(pos>A5_SW2));
  const int m6 = A6_MUL>>((pos>A6_SW1)+(pos>A6_SW2));
  int pz = failcount+1;
  pz += tri[(fails>>5)&3];
  pz += trj[(fails>>3)&3];
  pz += trj[(fails>>1)&3];
  if( fails&1 )
    pz += 8;
  pz = pz/2;
  const int k1 = c0;
  const int k4 = (c0*2)^(hash(b1, (x5>>8)&255, (x5>>16)&0x80ff)&0x1ffff);
  const int k2 = (c0*8)^(hash(29, failz&2047)&0x7fff);
  const int k5 = hash(c0, w5&0xfffff)&0xffff;
  const int k3 = (c0*32)^(hash(19, x5&0x80ffff)&0x7fff);
  const int k6 = (c0*4)^(hash(min(9, pz), x5&0x80ff)&0xffff);
  a1.pf(k1);
  a4.pf(k4);
  a2.pf(k2);
  a5.pf(k5);
  a3.pf(k3);
  a6.pf(k6);

  pr = contextModel.p();

  int pt, pv, pu = pmix( A1_w, a1.p(pr, k1, A1_MUL), P_SCALE-A1_w, pr );
  pu = a4.p(pu, k4, m4);
  pv = a2.p(pr, k2, m2);
  pv = a5.p(pv, k5, m5);
  pt = a3.p(pr, k3, m3);
  pz = a6.p(pu, k6, m6);
#else
  pr = contextModel.p();

  const int m2 = A2_MUL>>((pos>A2_SW1)+(pos>A2_SW2));
  const int m3 = A3_MUL>>((pos>A3_SW1)+(pos>A3_SW2));
  const int m4 = A4_MUL>>((pos>A4_SW1)+(pos>A4_SW2));
  const int m5 = A5_MUL>>((pos>A5_SW1)+(pos>A5_SW2));
  const int m6 = A6_MUL>>((pos>A6_SW1)+(pos>A6_SW2));
  int pt, pv, pz = failcount+1;
  int pu = pmix( A1_w, a1.p(pr, c0, A1_MUL), P_SCALE-A1_w, pr );
  pz += tri[(fails>>5)&3];
  pz += trj[(fails>>3)&3];
  pz += trj[(fails>>1)&3];
  if( fails&1 )
    pz += 8;
  pz = pz/2;

  pu = a4.p(pu, (c0*2)^(hash(b1, (x5>>8)&255, (x5>>16)&0x80ff)&0x1ffff), m4);
  pv = a2.p(pr, (c0*8)^(hash(29, failz&2047)&0x7fff), m2);
  pv = a5.p(pv, hash(c0, w5&0xfffff)&0xffff, m5);
  pt = a3.p(pr, (c0*32)^(hash(19, x5&0x80ffff)&0x7fff), m3);
  pz = a6.p(pu, (c0*4)^(hash(min(9, pz), x5&0x80ff)&0xffff), m6);
#endif

  if( fails&255 )
    pr = pmix( MIXA_pt_w,pt, MIXA_pu_w,pu, MIXA_pv_w,pv, MIXA_pz_w,pz );
  else
    pr = pmix( MIXB_pt_w,pt, MIXB_pu_w,pu, MIXB_pv_w,pv, MIXB_pz_w,pz );
}
};
} // namespace paq8hp
