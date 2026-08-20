// memory.inc -- the allocator.
//
// Two of them.  The default is plain `malloc`/`free`; everything below the
// `#if` is a bucketed free list over an `mmap`ed arena placed above 4 GB, built
// with `-DBMF_HIGH_ARENA` and existing for one purpose -- to prove the program
// does not care where its memory is.  A pointer stored through a 32-bit
// integer would fault there and nowhere else, which is what `tools/x32.sh` and
// the high-arena gate (92 of 93 checks) are between them checking.
#if UINTPTR_MAX>0xFFFFFFFFu&&defined(BMF_HIGH_ARENA)
#include <sys/mman.h>
#define BMF_ARENA_BYTES ((size_t)768<<20)
#define BMF_ARENA_AT ((void*)0x0000200000000000ull)
static char* bmf_arena;
static size_t bmf_arena_used;
static void bmf_arena_init(void) {
  void* p = mmap(BMF_ARENA_AT, BMF_ARENA_BYTES, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE, -1, 0);
  bmf_arena = (p==MAP_FAILED) ? nullptr : (char*)p;
  if( !bmf_arena ) {
    fprintf(stderr, "bmf: no high arena\n");
    exit(7);
  }
  if( (uintptr_t)bmf_arena<=0xFFFFFFFFu ) {
    fprintf(stderr, "bmf: the high arena is not high\n");
    exit(7);
  }
}

#define BMF_BUCKETS 40
static void* bmf_bucket[BMF_BUCKETS];
// The smallest bucket that holds `n`: bucket `b` is `1 << (b+5)` bytes, so the
// answer is `ceil(log2(n)) - 5`, clamped.  This was the shift loop that says
// the same thing one bucket at a time; it is a cold path either way, so the
// change is for the reader.
static int32_t bmf_bucket_of(size_t n) {
  if( n<=32 )
    return 0;
  int32_t b = 64-5-__builtin_clzll((uint64_t)(n-1));
  return min32(b, BMF_BUCKETS-1);
}

static void*bmf_malloc(size_t n) {
  if( !bmf_arena )
    bmf_arena_init();
  int32_t b = bmf_bucket_of(n);
  size_t need = (size_t)1<<(b+5);
  if( bmf_bucket[b] ) {
    void* p = bmf_bucket[b];
    bmf_bucket[b] = *(void**)p;
    ((size_t*)p)[-2] = (size_t)b;
    return p;
  }
  if( bmf_arena_used+need+16>BMF_ARENA_BYTES )
    return nullptr;
  char* p = bmf_arena+bmf_arena_used+16;
  ((size_t*)p)[-2] = (size_t)b;
  bmf_arena_used += need+16;
  return p;
}

static void bmf_free(void* p) {
  if( !p )
    return;
  int32_t b = (int32_t)((size_t*)p)[-2];
  // One comparison, not two: a negative `b` wraps past the bucket count.
  if( (uint32_t)b>=BMF_BUCKETS )
    return;
  *(void**)p = bmf_bucket[b];
  bmf_bucket[b] = p;
}

#define malloc bmf_malloc
#define free bmf_free
#endif
#define BMF_PAGE ((uintptr_t)4096)
// Fatal on failure, like `bmf_new` two functions down, and that is a fix
// rather than a tidy: this returned null, `alt_p2_planes_alloc` stored the null
// into `plane[pl]`, and `alt_p2_start_row` dereferenced it on the first row
// without looking.  `alt_p2_planes_free` *did* look -- `if( plane[pl] )` -- so
// the check existed on the path that could not crash and was missing from the
// one that could.
//
// Routing it here rather than at the two call sites is what makes the class go
// away instead of one instance of it: every other allocation in this tree is
// already fatal, so a nullable allocator is the outlier that invites the next
// unchecked store.
// A page-aligned block, with the un-aligned pointer `free` needs kept in the
// word before it.  `((void**)p)[-1]` at both ends is that word; naming it says
// which one thing the two sites agree about.
struct PageHeader {
  void* base;
};

static inline PageHeader* bmf_page_header(void* p) {
  return (PageHeader*)p-1;
}

static void*bmf_page_alloc(size_t n) {
  void* base = malloc(n+BMF_PAGE+sizeof(PageHeader));
  if( !base )
    bmf_fatal(bmf_no_memory);
  uintptr_t raw = (uintptr_t)base+sizeof(PageHeader);
  char* p = (char*)((raw+(BMF_PAGE-1))&~(BMF_PAGE-1));
  bmf_page_header(p)->base = base;
  memset(p, 0, n);
  return p;
}

static void bmf_page_free(void* p) {
  if( p )
    free(bmf_page_header(p)->base);
}

// The original installed a new-handler and let `operator new` retry through
// it forever; the handler it installed never returns.  So the loop had one
// iteration and the indirection had one target, and both are gone: this is
// the check they amounted to.
//
// `n ? n : 1` stays.  A zero-byte `malloc` may return null without being out
// of memory, and this build's `bmf_new(0)` callers -- an image with no
// palette -- would take the failure path.
static void*bmf_new(size_t n) {
  void* p = malloc(n ? n : 1);
  if( !p )
    bmf_fatal(bmf_no_memory);
  return p;
}
