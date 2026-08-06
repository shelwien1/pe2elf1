/* d5c_prelude.inc -- part of dxt5comp_core.inc; not standalone.
 *
 * types, inlining macros, clamp helpers and the build-time knobs.
 * Included first: everything below depends on these. */


using namespace std;
typedef int64_t i64;
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef uint64_t u64;
typedef unsigned char byte; typedef unsigned short word; typedef unsigned int uint;

#ifdef __GNUC__
 #define INLINE   __attribute__((always_inline)) inline
 #define NOINLINE __attribute__((noinline))
#else
 #define INLINE   __forceinline
 #define NOINLINE __declspec(noinline)
#endif
#ifndef __min
 #define __min(a,b) ((a)<(b)?(a):(b))
 #define __max(a,b) ((a)>(b)?(a):(b))
#endif


/* ---- declared BEFORE the generated headers -------------------------------
 * The MOD/*_h.inc files declare their storage with the IDX Table() directive,
 * which becomes a member of the generated %M%_T struct -- so the element types
 * (Ctr, CtrS, ...) and the size helpers (pclamp, cap_vol and the P* clamp
 * macros) have to be visible by then.  Everything here is layout only: the
 * update rules that need G0_* parameters are defined out of line further down,
 * once those parameters exist. */

// all clamps are constexpr: in the Const build a Table() size like
// cap_vol(M0_M1_Volume, PCAP(M0_M1_HB)) has to be an integer constant
// expression, because it becomes an array bound.
static constexpr int pclamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
#define PSHIFT(x)  pclamp((x), 1, 24)      // any >> amount
#define PHBITS(x)  pclamp((x), 8, 22)      // hash table size, log2
#define PBLEND(x)  pclamp((x), 0, 4)       // APM blend numerator /4
/* Structural constants of the coder's arithmetic.  None of these is a tuning
 * choice -- they are fixed by the range coder's resolution, the counter's
 * storage width, or the DXT block geometry -- so they deliberately do NOT go
 * through the IDX machinery.  Naming them separates "this number is a
 * consequence" from "this number was chosen", which is the distinction that
 * matters when reading the model or handing it to opt.pl. */
/* Allocation failure under -fno-exceptions.
 *
 * With exceptions off, libstdc++'s operator new does not return 0 -- it calls
 * std::__throw_bad_alloc(), which becomes abort().  So an out-of-memory run
 * dies with SIGABRT and no message, which is what running several instances in
 * parallel looks like from the outside: a crash.
 *
 * nothrow new DOES return 0, so every allocation goes through anew()/onew(),
 * which report what was being asked for and exit(3) cleanly.  A parallel driver
 * can then tell "out of memory, retry with fewer jobs" from a real bug. */
static void d5c_oom(const char* what, size_t n, size_t esz) {
  fprintf(stderr, "d5c: out of memory allocating %s: %zu x %zu B = %.1f MiB\n",
          what, n, esz, double(n) * double(esz) / 1048576.0);
  fflush(stderr);
  exit(3);
}
template<class T> static inline T* anew(size_t n, const char* what) {
  T* p = new (std::nothrow) T[n];
//printf( "anew: n=%.0lf\n", double(n) );
  if(!p) d5c_oom(what, n, sizeof(T));
  return p;
}
template<class T, class... A> static inline T* onew(const char* what, A&&... a) {
  T* p = new (std::nothrow) T(static_cast<A&&>(a)...);
//printf( "onew: n=%.0lf\n", double(sizeof(T(static_cast<A&&>(a)...))) );
  if(!p) d5c_oom(what, 1, sizeof(T));
  return p;
}

enum {
  P_BITS    = 12,                  // range coder probability resolution
  P_ONE     = 1 << P_BITS,         // 4096
  P_MAX     = P_ONE - 1,           // 4095  clamped away from certainty
  P_MIN     = 1,                   //    1
  P_MID     = P_ONE >> 1,          // 2048
  ST_MAX    = P_MID - 1,           // 2047  stretch domain, +-ST_MAX
  ST_MIN    = -ST_MAX,

  CTR_BITS  = 16,                  // counter probability storage width
  CTR_ONE   = 1 << CTR_BITS,       // 65536
  CTR_MAX   = CTR_ONE - 1,         // 65535 target for a 1 bit
  CTR_INIT  = CTR_ONE >> 1,        // 32768 unbiased prior
  CTR_SH    = CTR_BITS - P_BITS,   //     4 storage -> coder resolution
  CTR_NRATE = 16,                  // CW[] entries; the rate index saturates here

  DIST_A_MAX = 999,                // argmin sentinel > any 8-bit alpha distance
  DIST_C_MAX = 1 << 30,            // argmin sentinel > any squared RGB distance
  NO_W_BUCKET = 15,                // "no west neighbour" bucket id for lgb() codes
};

#define PMUL(x)    pclamp((x), 1, 65535)   // 16-bit fixed-point rate multiplier
#define PAPN(x)    pclamp((x), 2, 2048)    // APM interpolation interval count
#define PCAP(x)    pclamp((x), 4, 20)      // context-table size cap, log2

// Table-size cap for the match-history contexts. A `&` mask in .idx is a bit
// SELECTOR: each enabled bit doubles the table, and M1/M2 back 15/8 tables
// each, so letting the optimizer open history depth used to cost megabytes and
// blow the cache. cap_ctx() keeps the index exact while it fits in 2^bits and
// switches to a multiplicative hash only beyond that, so mask width now costs
// accuracy-on-collision instead of memory. vol is a compile-time constant in
// the const build, so the branch folds away.
static constexpr int cap_vol(int vol, int bits) { int n = 1 << bits; return vol < n ? vol : n; }
static inline int cap_ctx(int raw, int vol, int bits) {
  return vol <= (1 << bits) ? raw : int((u32(raw) * 2654435761u) >> (32 - bits));
}

/* Geometry bound.  Every per-level buffer (pixel planes, block grids, block
 * and raster rows) is sized from these instead of being grown per texture, so
 * the codec performs no allocation at all once constructed.  parse_dds refuses
 * anything larger, which on the encoder side simply routes the texture to the
 * verbatim-store path and on the decoder side rejects a corrupt or hostile
 * archive before a single buffer is touched. */
/* 0 disables the check; set to the value D5C_SQSTT=1 reports for this tree. */
/* mbit() must be inlined into its call sites.  It always was, until the
 * per-format split multiplied the instantiations past GCC's inlining budget:
 * at that point it silently became 26 out-of-line copies called once per coded
 * bit, and the whole per-format win (-0.8%) turned into a 1.4% LOSS.  The
 * heuristic is not wrong so much as blind to the fact that these
 * instantiations are each used from one or two sites, so state the intent
 * rather than depend on a budget that shifts whenever a template parameter is
 * added. */
#if defined(_MSC_VER) && !defined(__clang__)
 #define D5C_FORCE_INLINE __forceinline
#elif defined(__GNUC__)
 #define D5C_FORCE_INLINE inline __attribute__((always_inline))
#else
 #define D5C_FORCE_INLINE inline
#endif

#ifndef D5C_MIXCHK
#define D5C_MIXCHK 0
#endif
#ifndef D5C_SQSTT_HASH
#define D5C_SQSTT_HASH 0x7BCBA716
#endif
#ifndef D5C_RESTRICT_CHECK
#define D5C_RESTRICT_CHECK 0
#endif
#ifndef D5C_MAX_DIM
#define D5C_MAX_DIM 16384
#endif
static const u32 MAX_DIM = D5C_MAX_DIM;         // texels per side, level 0
/* D5C_DYN_GEOM: how the O(MAX_DIM^2) geometry slabs are stored.
 *
 *   1 (default) -- heap, one `new` per slab family in the Codec constructor.
 *   0           -- member arrays, as before.
 *
 * They are ~512 MiB at the default MAX_DIM = 4096 (18 planes of MAX_IW^2 plus
 * four Blk grids), and with static storage they sit inside the Codec object,
 * which makes sizeof(Solid) ~733 MiB in the Const build and turns `new Solid`
 * into one enormous allocation.  Splitting them out keeps the object small and
 * lets each family fail or succeed on its own.  Either way the pages stay
 * demand-zero -- Blk is trivially constructible on purpose and the `new[]`
 * carries no (), so nothing is written until level_init clears the live
 * region -- so RSS is unchanged; this is about the shape of the allocation,
 * not its cost. */
#ifndef D5C_DYN_GEOM
#define D5C_DYN_GEOM 1
#endif

static const u32 MAX_B   = (MAX_DIM + 3) / 4;   // blocks per side
static const u32 MAX_IW  = MAX_B * 4;           // plane side: the block grid rounds up
static const u32 MAX_BPP = 16;                  // widest raster pixel (A32B32G32R32F)
static const u32 MAX_BSZ = 16;                  // widest block (BC2/3/5/6/7)
