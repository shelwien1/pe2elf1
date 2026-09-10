constexpr i32 RS_DIGIT_MAX = 0x7FFFFF;
static int tc_enc;
#ifndef OC_CARRYLESS
#define OC_CARRYLESS 0
#endif
static Rangecoder<0, OC_CARRYLESS> rce;
static Rangecoder<1, OC_CARRYLESS> rcd;
constexpr int TC_MIXN = 7;
constexpr int TC_NODE = 29;
constexpr int TC_MNODE = 169;
constexpr int TC_NBMAX = 62;
static INLINE int tc_node_len(int k) { return 3 + (k < 25 ? k : 25); }
static INLINE int tc_node_man(int nb, int p) { return (nb > 12 ? 12 : nb) * 13 + (p > 12 ? 12 : p); }
static INLINE i32 tc_qlog(i64 x) {
  u64 u = (u64)(x < 0 ? -x : x);
  int n = 62 - __builtin_clzll(u | 1);
  i32 q = 2 + n * 2 + (i32)((u >> n) & 1);
  return u < 4 ? (i32)u : q;
}
static INLINE i32 tc_abs(i32 v) { return v < 0 ? -v : v; }
static INLINE int tc_clamp(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static int tc_huge;
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
  return p;
}
static INLINE i32 tc_sq(i32 v) {
  i32 q = tc_qlog(v);
  return v < 0 ? -q : q;
}
