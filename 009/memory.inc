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
static int bmf_bucket_of(size_t n) {
  int b = 0;
  while( (size_t)1<<(b+5)<n&&b<BMF_BUCKETS-1 )
    b++;
  return b;
}

static void*bmf_malloc(size_t n) {
  if( !bmf_arena )
    bmf_arena_init();
  int b = bmf_bucket_of(n);
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
  int b = (int)((size_t*)p)[-2];
  if( b<0||b>=BMF_BUCKETS )
    return;
  *(void**)p = bmf_bucket[b];
  bmf_bucket[b] = p;
}

#define malloc bmf_malloc
#define free bmf_free
#endif
#define BMF_PAGE ((uintptr_t)4096)
static void*bmf_page_alloc(size_t n) {
  void* base = malloc(n+BMF_PAGE+sizeof(void*));
  if( !base )
    return nullptr;
  uintptr_t raw = (uintptr_t)base+sizeof(void*);
  char* p = (char*)((raw+(BMF_PAGE-1))&~(BMF_PAGE-1));
  ((void**)p)[-1] = base;
  memset(p, 0, n);
  return p;
}

static void bmf_page_free(void* p) {
  if( p )
    free(((void**)p)[-1]);
}

void __out_of_memory_handler();
typedef void (*t_new_handler)();
static t_new_handler __pout_of_memory_handler = nullptr;
static void*bmf_new(size_t n) {
  for(;; ) {
    void* p = malloc(n ? n : 1);
    if( p )
      return p;
    t_new_handler handler = __pout_of_memory_handler;
    if( !handler )
      return nullptr;
    handler();
  }
}
