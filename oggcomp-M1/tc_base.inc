constexpr i32 RS_DIGIT_MAX = 0x7FFFFF;
#ifndef OC_CARRYLESS
#define OC_CARRYLESS 0
#endif
constexpr int TC_MIXN = 7;
constexpr int TC_NODE = 29;
constexpr int TC_MNODE = 169;
//  The widest magnitude the value coder carries: 2^64 - 1, which is any
//  64-bit field, and in particular the difference of two granule positions,
//  a 64-bit field the container gives no meaning to.  The node tables reach:
//  tc_node_len(63) is 28 of TC_NODE, tc_node_man(64, 63) is 168 of TC_MNODE.
constexpr int TC_NBMAX = 64;
static INLINE int tc_node_len(int k) { return 3 + (k < 25 ? k : 25); }
static INLINE int tc_node_man(int nb, int p) { return (nb > 12 ? 12 : nb) * 13 + (p > 12 ? 12 : p); }
//  The magnitude is taken in unsigned arithmetic, so the most negative
//  value has one; and the small values leave before `n` is computed, since
//  for 0 and 1 it would be -1 and the shift by it, though discarded, is
//  undefined.
static INLINE i32 tc_qlog(i64 x) {
  u64 u = x < 0 ? ~(u64)x + 1 : (u64)x;
  int n;
  if(u < 4)
    return (i32)u;
  n = 62 - __builtin_clzll(u);
  return 2 + n * 2 + (i32)((u >> n) & 1);
}
static INLINE i32 tc_abs(i32 v) { return v < 0 ? -v : v; }
static INLINE int tc_clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static int tc_huge;
//  Every mapping tc_map made, so that tc_zero can give the tables their
//  initial state back: a frontend that codes several streams in one run
//  (oggdet) wants each to see the model a fresh process would.
struct tc_mapped {
  void *p;
  sz n;
};
constexpr int TC_MAPMAX = 128;
static tc_mapped tc_maps[TC_MAPMAX];
static int tc_nmaps;
static void *tc_map(sz n) {
  void *p;
#ifdef _WIN32
  p = VirtualAlloc(nullptr, n, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
  p = mmap(nullptr, n, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if(p == MAP_FAILED)
    p = nullptr;
#ifdef MADV_HUGEPAGE
  if(p && tc_huge)
    madvise(p, n, MADV_HUGEPAGE);
#endif
#endif
  if(!p)
    FATAL_CODE(OGC_EXIT_IO,
               "cannot map %" PRIu64 " MB for a model table -- "
               "IDX/tsvcomp-*.idx asks for more than this machine will map",
               (u64)(n >> 20));
  FATAL_UNLESS(tc_nmaps < TC_MAPMAX, "internal: more model tables than tc_maps holds");
  tc_maps[tc_nmaps].p = p;
  tc_maps[tc_nmaps].n = n;
  tc_nmaps++;
  return p;
}
//  Zero a region the way a fresh process has it: for a mapping, or for a
//  static object's whole pages, by giving the pages back to the kernel, so
//  what was never touched stays untouched and what was is dropped rather
//  than written; the edges of a static object, and everything on Windows
//  outside a mapping, by memset.
static INLINE void tc_zero(void *p, sz n, int mapped) {
#ifdef _WIN32
  if(mapped) {
    VirtualFree(p, n, MEM_DECOMMIT);
    if(!VirtualAlloc(p, n, MEM_COMMIT, PAGE_READWRITE))
      FATAL_CODE(OGC_EXIT_IO, "cannot recommit %" PRIu64 " MB of model tables", (u64)(n >> 20));
    return;
  }
  memset(p, 0, n);
#else
  u8 *b = (u8 *)p;
  sz page = 4096, head = (sz)(-(uintptr_t)b) & (page - 1), mid;
  (void)mapped;
  if(head > n)
    head = n;
  memset(b, 0, head);
  b += head;
  n -= head;
  mid = n & ~(page - 1);
  if(mid && madvise(b, mid, MADV_DONTNEED))
    memset(b, 0, mid);
  b += mid;
  n -= mid;
  memset(b, 0, n);
#endif
}
static INLINE i32 tc_sq(i32 v) {
  i32 q = tc_qlog(v);
  return v < 0 ? -q : q;
}
