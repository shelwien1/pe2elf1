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

constexpr int P_BITS  = 12;               // precision of a prediction
constexpr int P_SCALE = 1<<P_BITS;        // 4096, the coder's totFreq
constexpr int P_MAX   = P_SCALE-1;        // 4095, largest codable p
constexpr int P_MIN   = 1;                // smallest codable p; p==0 is undecodable
constexpr int P_HALF  = P_SCALE/2;        // 2048, "no information"

constexpr int ST_SCALE = 256;             // stretch's logistic scale
constexpr int ST_MAX   = P_HALF-1;        // 2047
constexpr int ST_MIN   = -ST_MAX;         // -2047

// squash interpolates the logistic on a SQ_STEP-wide grid of SQ_NODES points;
// APM reuses the same grid over the same stretch range, hence APM_NODES.
constexpr int SQ_BITS  = 7;
constexpr int SQ_STEP  = 1<<SQ_BITS;      // 128
constexpr int SQ_MASK  = SQ_STEP-1;       // 127
constexpr int SQ_NODES = 2*P_HALF/SQ_STEP + 1;   // 33
constexpr int SQ_MID   = SQ_NODES/2;      // 16, the node at d==0
constexpr int APM_NODES = SQ_NODES;       // 33 entries per APM context

constexpr int PR_BITS  = 16;              // StateMap/APM internal precision
constexpr int PR_ONE   = 1<<PR_BITS;      // 65536
constexpr int PR_MAX   = PR_ONE-1;        // 65535
constexpr int PR_SHIFT = PR_BITS-P_BITS;  // 4, PR_BITS <-> P_BITS

constexpr int N_STATES = 256;             // rows of State_table / StateMap::t

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
int order, bpos = 0, cxtfl = 3, sm_shft = 7, sm_add = PR_MAX+((1<<7)-1), sm_add_y = 0;

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

static const U8 State_table[256][4] = {{1, 2, 0, 0}, {3, 5, 1, 0}, {4, 6, 0, 1}, {7, 10, 2, 0}, {8, 12, 1, 1}, {9, 13, 1, 1}, {11, 14, 0, 2}, {15, 19, 3, 0}, {16, 23, 2, 1}, {17, 24, 2, 1}, {18, 25, 2, 1}, {20, 27, 1, 2}, {21, 28, 1, 2}, {22, 29, 1, 2}, {26, 30, 0, 3}, {31, 33, 4, 0}, {32, 35, 3, 1}, {32, 35, 3, 1}, {32, 35, 3, 1}, {32, 35, 3, 1}, {34, 37, 2, 2}, {34, 37, 2, 2}, {34, 37, 2, 2}, {34, 37, 2, 2}, {34, 37, 2, 2}, {34, 37, 2, 2}, {36, 39, 1, 3}, {36, 39, 1, 3}, {36, 39, 1, 3}, {36, 39, 1, 3}, {38, 40, 0, 4}, {41, 43, 5, 0}, {42, 45, 4, 1}, {42, 45, 4, 1}, {44, 47, 3, 2}, {44, 47, 3, 2}, {46, 49, 2, 3}, {46, 49, 2, 3}, {48, 51, 1, 4}, {48, 51, 1, 4}, {50, 52, 0, 5}, {53, 43, 6, 0}, {54, 57, 5, 1},
  {54, 57, 5, 1}, {56, 59, 4, 2}, {56, 59, 4, 2}, {58, 61, 3, 3}, {58, 61, 3, 3}, {60, 63, 2, 4}, {60, 63, 2, 4}, {62, 65, 1, 5}, {62, 65, 1, 5}, {50, 66, 0, 6}, {67, 55, 7, 0}, {68, 57, 6, 1}, {68, 57, 6, 1}, {70, 73, 5, 2}, {70, 73, 5, 2}, {72, 75, 4, 3}, {72, 75, 4, 3}, {74, 77, 3, 4}, {74, 77, 3, 4}, {76, 79, 2, 5}, {76, 79, 2, 5}, {62, 81, 1, 6}, {62, 81, 1, 6}, {64, 82, 0, 7}, {83, 69, 8, 0}, {84, 71, 7, 1}, {84, 71, 7, 1}, {86, 73, 6, 2}, {86, 73, 6, 2}, {44, 59, 5, 3}, {44, 59, 5, 3}, {58, 61, 4, 4}, {58, 61, 4, 4}, {60, 49, 3, 5}, {60, 49, 3, 5}, {76, 89, 2, 6}, {76, 89, 2, 6}, {78, 91, 1, 7}, {78, 91, 1, 7}, {80, 92, 0, 8}, {93, 69, 9, 0}, {94, 87, 8, 1}, {94, 87, 8, 1},
  {96, 45, 7, 2}, {96, 45, 7, 2}, {48, 99, 2, 7}, {48, 99, 2, 7}, {88, 101, 1, 8}, {88, 101, 1, 8}, {80, 102, 0, 9}, {103, 69, 10, 0}, {104, 87, 9, 1}, {104, 87, 9, 1}, {106, 57, 8, 2}, {106, 57, 8, 2}, {62, 109, 2, 8}, {62, 109, 2, 8}, {88, 111, 1, 9}, {88, 111, 1, 9}, {80, 112, 0, 10}, {113, 85, 11, 0}, {114, 87, 10, 1}, {114, 87, 10, 1}, {116, 57, 9, 2}, {116, 57, 9, 2}, {62, 119, 2, 9}, {62, 119, 2, 9}, {88, 121, 1, 10}, {88, 121, 1, 10}, {90, 122, 0, 11}, {123, 85, 12, 0}, {124, 97, 11, 1}, {124, 97, 11, 1}, {126, 57, 10, 2}, {126, 57, 10, 2}, {62, 129, 2, 10}, {62, 129, 2, 10}, {98, 131, 1, 11}, {98, 131, 1, 11}, {90, 132, 0, 12}, {133, 85, 13, 0}, {134, 97, 12, 1}, {134, 97, 12, 1}, {136, 57, 11, 2}, {136, 57, 11, 2}, {62, 139, 2, 11},
  {62, 139, 2, 11}, {98, 141, 1, 12}, {98, 141, 1, 12}, {90, 142, 0, 13}, {143, 95, 14, 0}, {144, 97, 13, 1}, {144, 97, 13, 1}, {68, 57, 12, 2}, {68, 57, 12, 2}, {62, 81, 2, 12}, {62, 81, 2, 12}, {98, 147, 1, 13}, {98, 147, 1, 13}, {100, 148, 0, 14}, {149, 95, 15, 0}, {150, 107, 14, 1}, {150, 107, 14, 1}, {108, 151, 1, 14}, {108, 151, 1, 14}, {100, 152, 0, 15}, {153, 95, 16, 0}, {154, 107, 15, 1}, {108, 155, 1, 15}, {100, 156, 0, 16}, {157, 95, 17, 0}, {158, 107, 16, 1}, {108, 159, 1, 16}, {100, 160, 0, 17}, {161, 105, 18, 0}, {162, 107, 17, 1}, {108, 163, 1, 17}, {110, 164, 0, 18}, {165, 105, 19, 0}, {166, 117, 18, 1}, {118, 167, 1, 18}, {110, 168, 0, 19}, {169, 105, 20, 0}, {170, 117, 19, 1}, {118, 171, 1, 19}, {110, 172, 0, 20}, {173, 105, 21, 0}, {174, 117, 20, 1}, {118, 175, 1, 20},
  {110, 176, 0, 21}, {177, 105, 22, 0}, {178, 117, 21, 1}, {118, 179, 1, 21}, {110, 180, 0, 22}, {181, 115, 23, 0}, {182, 117, 22, 1}, {118, 183, 1, 22}, {120, 184, 0, 23}, {185, 115, 24, 0}, {186, 127, 23, 1}, {128, 187, 1, 23}, {120, 188, 0, 24}, {189, 115, 25, 0}, {190, 127, 24, 1}, {128, 191, 1, 24}, {120, 192, 0, 25}, {193, 115, 26, 0}, {194, 127, 25, 1}, {128, 195, 1, 25}, {120, 196, 0, 26}, {197, 115, 27, 0}, {198, 127, 26, 1}, {128, 199, 1, 26}, {120, 200, 0, 27}, {201, 115, 28, 0}, {202, 127, 27, 1}, {128, 203, 1, 27}, {120, 204, 0, 28}, {205, 115, 29, 0}, {206, 127, 28, 1}, {128, 207, 1, 28}, {120, 208, 0, 29}, {209, 125, 30, 0}, {210, 127, 29, 1}, {128, 211, 1, 29}, {130, 212, 0, 30}, {213, 125, 31, 0}, {214, 137, 30, 1}, {138, 215, 1, 30}, {130, 216, 0, 31}, {217, 125, 32, 0}, {218, 137, 31, 1},
  {138, 219, 1, 31}, {130, 220, 0, 32}, {221, 125, 33, 0}, {222, 137, 32, 1}, {138, 223, 1, 32}, {130, 224, 0, 33}, {225, 125, 34, 0}, {226, 137, 33, 1}, {138, 227, 1, 33}, {130, 228, 0, 34}, {229, 125, 35, 0}, {230, 137, 34, 1}, {138, 231, 1, 34}, {130, 232, 0, 35}, {233, 125, 36, 0}, {234, 137, 35, 1}, {138, 235, 1, 35}, {130, 236, 0, 36}, {237, 125, 37, 0}, {238, 137, 36, 1}, {138, 239, 1, 36}, {130, 240, 0, 37}, {241, 125, 38, 0}, {242, 137, 37, 1}, {138, 243, 1, 37}, {130, 244, 0, 38}, {245, 135, 39, 0}, {246, 137, 38, 1}, {138, 247, 1, 38}, {140, 248, 0, 39}, {249, 135, 40, 0}, {250, 69, 39, 1}, {80, 251, 1, 39}, {140, 252, 0, 40}, {249, 135, 41, 0}, {250, 69, 40, 1}, {80, 251, 1, 40}, {140, 252, 0, 41}};

#define nex(state, sel) State_table[state][sel]

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
constexpr int Q8_BITS = 8;
constexpr int Q8_ONE  = 1<<Q8_BITS;       // 256
constexpr int Q8_HALF = Q8_ONE/2;         // 128

struct Logistic {
  short sqt[P_SCALE];   // squash:  d+P_HALF -> p at P_BITS
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
    for( int d = -P_HALF; d<P_HALF; d++ ) {
      long long t = (long long)d<<Q8_BITS;
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
      sqt[d+P_HALF] = short(best<P_MIN ? P_MIN : best>P_MAX ? P_MAX : best);
    }

    // The tables are the format: a build whose pair differs cannot decode
    // another's archive, and this says so at startup instead of as garbage.
    // -DPAQ_LOGISTIC_HASH=0 to silence while deliberately changing them.
#if !defined(PAQ_LOGISTIC_HASH)
  #define PAQ_LOGISTIC_HASH 0x7BCBA716u
#endif
    if( PAQ_LOGISTIC_HASH ) {
      U32 h = 2166136261u;                        // FNV-1a over the two tables
      for( int i = 0; i<P_SCALE; i++ ) {
        h = (h ^ U32((U16)sqt[i]))*16777619u;
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
  return g_logistic.sqt[d+P_HALF];
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
template <int n, int m, int s = 1, int w = 0> struct Mixer;

template <int S> struct SubMixer {
Mixer<S, 1, 1, 0x7fff> mp;

void update2() {
  mp.update2();
}

void add(int x) {
  mp.add(x);
}

int p() {
  return mp.p();
}
};

template <> struct SubMixer<1> {
void update2() {
}

void add(int) {
}

int p() {
  return 0;
}
};

template <int n, int m, int s, int w> struct Mixer {
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
    train2(&tx[0], &wx[cxt[i]*N], &wx[cxt[i+1]*N], nx, ((y<<12)-pr[i])*7, ((y<<12)-pr[i+1])*7);
  for(; i<ncxt; ++i )
    train(&tx[0], &wx[cxt[i]*N], nx, ((y<<12)-pr[i])*7);
#else
  for( int i = 0; i<ncxt; ++i ) {
    int err = ((y<<12)-pr[i])*7;
    train(&tx[0], &wx[cxt[i]*N], nx, err);
  }
#endif
  nx = base = ncxt = 0;
}

void update2() {
  train(&tx[0], &wx[0], nx, ((y<<12)-base)*3/2);
  nx = 0;
}

void add(int x) {
  tx[nx++] = x;
}

void mul(int x) {
  int z = tx[nx];
  z = z*x/4;
  tx[nx++] = z;
}

// The six m.set() calls in ContextModel::p() use disjoint base ranges, so
// cxt[0..5] are always distinct - which is what lets train2() above write two
// rows in one interleaved pass without aliasing.
void set(int cx, int range) {
  cxt[ncxt++] = base+cx;
  base += range;
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
      d0 = (d0*9)>>9;
      pr[i] = squash(d0);
      sub.add(d0);
      d1 = (d1*9)>>9;
      pr[i+1] = squash(d1);
      sub.add(d1);
    }
    for(; i<ncxt; ++i ) {
      int dp = dot_product(&tx[0], &wx[cxt[i]*N], nx);
      dp = (dp*9)>>9;
      pr[i] = squash(dp);
      sub.add(dp);
    }
#else
    for( int i = 0; i<ncxt; ++i ) {
      int dp = dot_product(&tx[0], &wx[cxt[i]*N], nx);
      dp = (dp*9)>>9;
      pr[i] = squash(dp);
      sub.add(dp);
    }
#endif
    return sub.p();
  } else {
    int z = dot_product(&tx[0], &wx[0], nx);
    base = squash((z*15)>>13);
    return squash(z>>9);
  }
}
};

// the one mixer the models are handed
typedef Mixer<456, 128*(16+14+14+12+14+16), 6, 512> MainMixer;

template <int n> struct APM {
int index;

U16 t[n*APM_NODES];

APM() : index(0) {
  for( int j = 0; j<APM_NODES; ++j )
    t[j] = squash((j-SQ_MID)*SQ_STEP)<<PR_SHIFT;
  for( int i = APM_NODES; i<n*APM_NODES; ++i )
    t[i] = t[i-APM_NODES];
}

// p() will touch t[cxt*APM_NODES + k] and +1 for some k in [0,APM_NODES-2],
// i.e. a 66-byte region spanning at most two lines.
void pf(int cxt) const {
  PAQ_PF(&t[cxt*APM_NODES]);
  PAQ_PF(&t[cxt*APM_NODES+APM_NODES-1]);
}

int p(int pr = P_HALF, int cxt = 0, int rate = 8) {
  pr = stretch(pr);
  int g = (y<<PR_BITS)+(y<<rate)-y*2;
  t[index] += (g-t[index])>>rate;
  t[index+1] += (g-t[index+1])>>rate;
  const int w = pr&SQ_MASK;
  index = ((pr+P_HALF)>>SQ_BITS)+cxt*APM_NODES;
  // U16 entries times weights summing to SQ_STEP, brought back to P_BITS
  return (t[index]*(SQ_STEP-w)+t[index+1]*w)>>(PR_BITS+SQ_BITS-P_BITS);
}
};

struct StateMap {
int cxt;
U16 t[N_STATES];

StateMap() : cxt(0) {
  for( int i = 0; i<N_STATES; ++i ) {
    int n0 = nex(i, 2);
    int n1 = nex(i, 3);
    if( n0==0 )
      n1 *= 128;
    if( n1==0 )
      n0 *= 128;
    t[i] = PR_ONE*(n1+1)/(n0+n1+2);
  }
}

int p(int cx) {
  int q = t[cxt];
  t[cxt] = q+((sm_add_y-q)>>sm_shft);
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
template <int CF> inline PAQ_HOTFN int mix2t(MainMixer &m, int s, StateMap &sm) {
  int p1 = sm.p(s);
  int n0 = -!nex(s, 2);
  int n1 = -!nex(s, 3);
  int st = stretch(p1);
  if( CF<0 ? (cxtfl!=0) : (CF!=0) ) {
    m.add(st/4);
    int p0 = P_MAX-p1;
    m.add((p1-p0)*3/64);
    m.add(st*(n1-n0)*3/16);
    m.add(((p1&n0)-(p0&n1))/16);
    m.add(((p0&n0)-(p1&n1))*7/64);
    return s>0;
  }
  m.add(st*9/32);
  m.add(st*(n1-n0)*3/16);
  int p0 = P_MAX-p1;
  m.add(((p1&n0)-(p0&n1))/16);
  m.add(((p0&n0)-(p1&n1))*7/64);
  return s>0;
}

inline int mix2(MainMixer &m, int s, StateMap &sm) {
  return mix2t<-1>(m, s, sm);
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
  else if( cp[0]<255 )
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

int mix(MainMixer &m) {
#if defined(PAQ_BPOS_T)
  switch( bpos ) {
    case 0:  m.add(pt<0>()); break;
    case 1:  m.add(pt<1>()); break;
    case 2:  m.add(pt<2>()); break;
    case 3:  m.add(pt<3>()); break;
    case 4:  m.add(pt<4>()); break;
    case 5:  m.add(pt<5>()); break;
    case 6:  m.add(pt<6>()); break;
    default: m.add(pt<7>()); break;
  }
#else
  m.add(pt<-1>());
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
    t[i] = 32768;
  cp = &t[0];
}

void set(U32 cx) {
  cxt = (cx*256)&(U32(MSZ/2)-256);
}

void mix(MainMixer &m) {
  if( pos<4000000 )
    *cp += ((y<<16)-(*cp)+(1<<8))>>9;
  else
    *cp += ((y<<16)-(*cp)+(1<<9))>>10;
  cp = &t[cxt+c0];
  m.add(stretch(*cp>>4)*mulc/32);
}
};

template <U32 MSZ, int NC> struct ContextMap {
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
StateMap sm[NC];
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
template <int CF, int BP> PAQ_HOTFN int mix1t(MainMixer &m, int cc, int c1, int y1) {

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
      int ns = nex(*cpi, y1);
      if( ns>=204&&(rnd()<<((452-ns)>>3)) )
        ns -= 4;
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
        else if( c0<254 )
          c0 += 2;
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
      m.add(b*c);
    } else
      m.add(0);

    result += mix2t<CF>(m, cpi ? *cpi : 0, sm[i]);
    cp[i] = cpi;
  }
  if( PAQ_BP==7 )
    cn = 0;
  return result;
}

int mix1(MainMixer &m, int cc, int c1, int y1) {
  return mix1t<-1,-1>(m, cc, c1, y1);
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
template <int CF> int mixbp(MainMixer &m) {
#if defined(PAQ_BPOS_T)
  switch( bpos ) {
    case 0:  return mix1t<CF,0>(m, c0, b1, y);
    case 1:  return mix1t<CF,1>(m, c0, b1, y);
    case 2:  return mix1t<CF,2>(m, c0, b1, y);
    case 3:  return mix1t<CF,3>(m, c0, b1, y);
    case 4:  return mix1t<CF,4>(m, c0, b1, y);
    case 5:  return mix1t<CF,5>(m, c0, b1, y);
    case 6:  return mix1t<CF,6>(m, c0, b1, y);
    default: return mix1t<CF,7>(m, c0, b1, y);
  }
#else
  return mix1t<CF,-1>(m, c0, b1, y);
#endif
}

int mix(MainMixer &m) {
  return mixbp<CF3>(m);
}

// RecordModel's cm/cn/cq run with cxtfl == 0; under PAQ_CXTFL_T that becomes a
// second instantiation instead of a per-context branch.
int mix0(MainMixer &m) {
  return mixbp<CF0>(m);
}

// hoist the whole probe set above whatever the caller does next
void pfset() const {
  pfprobe(c0);
}
};

static U32 col, frstchar = 0, spafdo = 0, spaces = 0, spacecount = 0, words = 0, wordcount = 0, fails = 0, failz = 0, failcount = 0;

template <int L> struct WordModel {
U32 word0, word1, word2, word3, word4;
ContextMap<((U32)MEM(L)*16), 46> cm;
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

    if( (c-'a')<=('z'-'a')||c==8||c==6||(c>127&&b2!=12) ) {
      ++words, ++wordcount;
      word0 = word0*263*8+c;
    } else {
      if( c==32||c==10 ) {
        ++spaces, ++spacecount;
        if( c==10 )
          nl1 = nl, nl = pos-1;
      }
      if( word0 ) {
        word4 = word3*43;
        word3 = word2*47;
        word2 = word1*53;
        word1 = word0*83;
        word0 = 0;
        if( c=='.'||c=='O'||c==('}'-'{'+'P') )
          f = 1, spafdo = 0;
        else {
          ++spafdo;
          spafdo = min(63, spafdo);
        }
      }
    }

    U32 h = word0*271+c;
    cm.set(word0);
    cm.set(h+word1);
    cm.set(word0*91+word1*89);
    cm.set(h+word1*79+word2*71);

    cm.set(h+word2);
    cm.set(h+word3);
    cm.set(h+word4);
    cm.set(h+word1*73+word3*61);
    cm.set(h+word2*67+word3*59);

    if( f ) {
      word4 = word3*31;
      word3 = word2*37;
      word2 = word1*41;
      word1 = '.';
    }

    cm.set(b3|b4<<8);
    cm.set(spafdo*8*((w4&3)==1));

    col = min(31, pos-nl);
    if( col<=2 ) {
      if( col==2 )
        frstchar = min(c, 96);
      else
        frstchar = 0;
    }
    if( frstchar=='['&&c==32 ) {
      if( b3==']'||b4==']' )
        frstchar = 96;
    }
    cm.set(frstchar<<11|c);

    int above = buf[nl1+col];

    cm.set(col<<16|c<<8|above);
    cm.set(col<<8|c);
    cm.set(col*(c==32));

    h = wordcount*64+spacecount;
    cm.set(spaces&0x7fff);
    cm.set(frstchar<<7);
    cm.set(spaces&0xff);
    cm.set(c*64+spacecount/2);
    cm.set((c<<13)+h);
    cm.set(h);

    U32 d = c4&0xffff;
    h = w4<<6;
    cm.set(c+(h&0xffffff00));
    cm.set(c+(h&0x00ffff00));
    cm.set(c+(h&0x0000ff00));
    h <<= 6;
    cm.set(d+(h&0xffff0000));
    cm.set(d+(h&0x00ff0000));
    h <<= 6, f = c4&0xffffff;
    cm.set(f+(h&0xff000000));

    U16 &r2 = t2[f>>8];
    r2 = r2<<8|c;
    U32 &r1 = t1[d>>8];
    r1 = r1<<8|c;
    U32 t = c|t1[c]<<8;
    cm.set(t&0xffff);
    cm.set(t&0xffffff);
    cm.set(t);
    cm.set(t&0xff00);
    t = d|t2[d]<<16;
    cm.set(t&0xffffff);
    cm.set(t);

    cm.set(x4&0x00ff00ff);
    cm.set(x4&0xff0000ff);
    cm.set(x4&0x00ffff00);
    cm.set(c4&0xff00ff00);
    cm.set(c+b5*256+(1<<17));
    cm.set(c+b6*256+(2<<17));
    cm.set(b4+b8*256+(4<<17));

    cm.set(d);
    cm.set(w4&15);
    cm.set(f4);
    cm.set((w4&63)*128+(5<<17));
    cm.set(d<<9|frstchar);
    cm.set((f4&0xffff)<<11|frstchar);
  }
  cm.mix(m);
}
};

// every size here is a literal, so this one needs no level parameter
struct RecordModel {
int cpos1[256];
int wpos1[0x10000];
ContextMap<32768/4, 2> cm;
ContextMap<32768/2, 5> cn;
ContextMap<32768, 4> co;
ContextMap<32768*2, 3> cp;
ContextMap<32768*4, 3> cq;

RecordModel() {
  memset(cpos1, 0, sizeof(cpos1));
  memset(wpos1, 0, sizeof(wpos1));
}

void mix(MainMixer &m) {
  if( !bpos ) {
    int c = b1, w = (b2<<8)+c, d = w&0xf0ff, e = c4&0xffffff;
    cm.set(c<<8|(min(255, pos-cpos1[c])/4));
    cm.set(w<<9|llog(pos-wpos1[w])>>2);
    cn.set(w);
    cn.set(d<<8);
    cn.set(c<<16);
    cn.set((f4&0xffff)<<3);
    int col = pos&3;
    cn.set(col|2<<12);

    co.set(c);
    co.set(w<<8);
    co.set(w5&0x3ffff);
    co.set(e<<3);

    cp.set(d);
    cp.set(c<<8);
    cp.set(w<<16);

    cq.set(w<<3);
    cq.set(c<<19);
    cq.set(e);

    cpos1[c] = pos;
    wpos1[w] = pos;
  }
  co.mix(m);
  cp.mix(m);
  cxtfl = 0;
  cm.mix0(m);
  cn.mix0(m);
  cq.mix0(m);
  cxtfl = 3;
}
};

template <int L> struct SparseModel {
ContextMap<((U32)(MEM(L)*2)), 5> cn;
SmallStationaryContextMap<0x20000, 17> scm1;
SmallStationaryContextMap<0x20000, 12> scm2;
SmallStationaryContextMap<0x20000, 12> scm3;
SmallStationaryContextMap<0x20000, 13> scm4;
SmallStationaryContextMap<0x10000, 12> scm5;
SmallStationaryContextMap<0x20000, 12> scm6;
SmallStationaryContextMap<0x2000, 12> scm7;
SmallStationaryContextMap<0x8000, 13> scm8;
SmallStationaryContextMap<0x1000, 12> scm9;
SmallStationaryContextMap<0x10000, 16> scma;

void mix(MainMixer &m) {
  if( bpos==0 ) {
    cn.set(words&0x1ffff);
    cn.set((f4&0x000fffff)*7);
    cn.set((x4&0xf8f8f8f8)+3);
    cn.set((tt&0x00000fff)*9);
    cn.set((x4&0x80f0f0ff)+6);
    scm1.set(b1);
    scm2.set(b2);
    scm3.set(b3);
    scm4.set(b4);
    scm5.set(words&127);
    scm6.set((words&12)*16+(w4&12)*4+(b1>>4));
    scm7.set(w4&15);
    scm8.set(spafdo*((w4&3)==1));
    scm9.set(col*(b1==32));
    scma.set(frstchar);
  }
  cn.mix(m);
  scm1.mix(m);
  scm2.mix(m);
  scm3.mix(m);
  scm4.mix(m);
  scm5.mix(m);
  scm6.mix(m);
  scm7.mix(m);
  scm8.mix(m);
  scm9.mix(m);
  scma.mix(m);
}
};

int primes[] = {0, 257, 251, 241, 239, 233, 229, 227, 223, 211, 199, 197, 193, 191};
static U32 WRT_mpw[16] = {3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0}, tri[4] = {0, 4, 3, 7}, trj[4] = {0, 6, 6, 12};
static U32 WRT_mtt[16] = {0, 0, 1, 2, 3, 4, 5, 5, 6, 6, 6, 6, 6, 7, 7, 7};

template <int L> struct ContextModel {
ContextMap<((U32)MEM(L)*16), 7> cm;
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
  m.add(64);

  if( bpos==0 ) {
    int i = 0, f2 = buf(2);

    if( f2=='.'||f2=='O'||f2=='M'||f2=='!'||f2==')'||f2==('}'-'{'+'P') ) {
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
  rcm7.mix(m);
  rcm9.mix(m);
  rcm10.mix(m);
  int qq = m.nx;
  order = cm.mix(m)-1;
  if( order<0 )
    order = 0;
  int zz = (m.nx-qq)/7;

  m.nx = qq+zz*3;
  for( qq = zz*2; qq!=0; --qq )
    m.mul(5);
  for( qq = zz; qq!=0; --qq )
    m.mul(6);
  for( qq = zz; qq!=0; --qq )
    m.mul(9);

  if( L>=4 ) {
    wordModel.mix(m);
    sparseModel.mix(m);
    recordModel.mix(m);
  }

  U32 c1 = b1, c2 = b2, c;
  if( c1==9||c1==10||c1==32 )
    c1 = 16;
  if( c2==9||c2==10||c2==32 )
    c2 = 16;

  m.set(256*order+(w4&240)+(c2>>4), 256*7);

  c = (words>>1)&63;
  m.set((w4&3)*64+c+order*256, 256*7);

  c = (w4&255)+256*bpos;
  m.set(c, 256*8);

  if( bpos ) {
    c = c0<<(8-bpos);
    if( bpos==1 )
      c += b3/2;
    c = (min(bpos, 5))*256+(tt&63)+(c&192);
  } else
    c = (words&12)*16+(tt&63);
  m.set(c, 1536);

  c = bpos;
  c2 = (c0<<(8-bpos))|(c1>>bpos);
  m.set(order*256+c+(c2&248), 256*7);

  c = c*256+((c0<<(8-bpos))&255);
  c1 = (words<<bpos)&255;
  m.set(c+(c1>>bpos), 2048);

  return m.p();
}
};

template <int L> struct Predictor {
int pr;

ContextModel<L> contextModel;

APM<256> a1;
APM<0x8000> a2;
APM<0x8000> a3;
APM<0x20000> a4;
APM<0x10000> a5;
APM<0x10000> a6;

Predictor() : pr(P_HALF) {
}

int p() const {
  return pr;
}

// was PAQ8HP::Perceive
void Perceive(int bit) {
  y = bit;
  sm_add_y = bit ? sm_add : 0;
  update();
}

void update() {
  c0 += c0+y;
  if( c0>=256 ) {
    buf[pos++] = c0;
    c0 -= 256;
    if( pos<=1024*1024 ) {
      // sm_add is PR_MAX plus the rounding for the new shift, so StateMap's
      // update keeps hitting the top of the PR_BITS range exactly.
      if( pos==1024*1024 )
        sm_shft = 9, sm_add = PR_MAX+((1<<9)-1);
      if( pos==512*1024 )
        sm_shft = 8, sm_add = PR_MAX+((1<<8)-1);
      sm_add_y = sm_add&(-y);
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
    if( c0=='.'||c0=='O'||c0=='M'||c0=='!'||c0==')'||c0==('}'-'{'+'P') ) {
      w5 = (w5<<8)|0x3ff, x5 = (x5<<8)+c0, f4 = (f4&0xfffffff0)+2;
      if( c0!='!'&&c0!='O' )
        w4 |= 12;
      if( c0!='!' )
        b2 = '.', tt = (tt&0xfffffff8)+1;
    }
    c4 = (c4<<8)+c0;
    x5 = (x5<<8)+c0;
    if( c0==32 )
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
  if( pr>=1820 )
    ++fails, ++failcount;
  if( pr>=848 )
    ++failz;

#if defined(PAQ_APMPF)
  // Every APM context here is a function of state that is already final at this
  // point (c0/b1/x5/w5/fails/failz/failcount are all updated above), so the six
  // hashes can be computed before contextModel.p() and their regions prefetched
  // under the whole model call.  Pure reordering of pure computation.
  int rate = 6+(pos>14*256*1024)+(pos>28*512*1024);
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

  int pt, pu = (a1.p(pr, k1, 3)+7*pr+4)>>3, pv;
  pu = a4.p(pu, k4, rate);
  pv = a2.p(pr, k2, rate+1);
  pv = a5.p(pv, k5, rate);
  pt = a3.p(pr, k3, rate);
  pz = a6.p(pu, k6, rate);
#else
  pr = contextModel.p();

  int rate = 6+(pos>14*256*1024)+(pos>28*512*1024);
  int pt, pu = (a1.p(pr, c0, 3)+7*pr+4)>>3, pv, pz = failcount+1;
  pz += tri[(fails>>5)&3];
  pz += trj[(fails>>3)&3];
  pz += trj[(fails>>1)&3];
  if( fails&1 )
    pz += 8;
  pz = pz/2;

  pu = a4.p(pu, (c0*2)^(hash(b1, (x5>>8)&255, (x5>>16)&0x80ff)&0x1ffff), rate);
  pv = a2.p(pr, (c0*8)^(hash(29, failz&2047)&0x7fff), rate+1);
  pv = a5.p(pv, hash(c0, w5&0xfffff)&0xffff, rate);
  pt = a3.p(pr, (c0*32)^(hash(19, x5&0x80ffff)&0x7fff), rate);
  pz = a6.p(pu, (c0*4)^(hash(min(9, pz), x5&0x80ff)&0xffff), rate);
#endif

  if( fails&255 )
    pr = (pt*6+pu+pv*11+pz*14+16)>>5;
  else
    pr = (pt*4+pu*5+pv*12+pz*11+16)>>5;
}
};
} // namespace paq8hp
