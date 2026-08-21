// memory.inc -- the allocator.
//
// Two of them.  The default is plain `malloc`/`free`; everything below the
// `#if` is a bucketed free list over an `mmap`ed arena placed above 4 GB, built
// with `-DBMF_HIGH_ARENA` and existing for one purpose -- to prove the program
// does not care where its memory is.  A pointer stored through a 32-bit
// integer would fault there and nowhere else, which is what `tools/x32.sh` and
// the high-arena gate (110 of 111 checks) are between them checking.
//
// **The arena is the one thing left that two codecs share.**  Everything a
// `BMFCodec` touches is a member of it, so two of them run in one process
// without meeting -- except here, where the bump pointer and the forty free
// lists are file-scope by construction and cannot be anything else: an
// allocator per codec would defeat what this leg exists to prove.  So the four
// entry points take a lock.  See `bmf_arena_lock` below for why it is a
// spinlock and not a `std::mutex`.
//
// **This lock is there by inspection and not by a test that failed without it**,
// which is the opposite of how everything else in this tree was justified, so
// it is worth saying why.  `tools/parallel.cpp` passes against this leg with the
// lock stubbed out -- eight threads allocating a few dozen times each will not
// reliably interleave a bump pointer -- and ThreadSanitizer, which would see it
// on the first access, *will not start against this leg at all*: with
// `-DBMF_HIGH_ARENA` its own shadow reservation fails before `main`
// (`ThreadSanitizer: mmap at bad address`), reproducibly and with ASLR off, and
// the same tree built without the define runs under it fine.  So there is no
// instrument here that can report, and what stands instead is the code: both
// `bmf_arena_used += need+16` and the free list's head swap are
// read-modify-write on shared words, and neither is atomic.
#if UINTPTR_MAX>0xFFFFFFFFu&&defined(BMF_HIGH_ARENA)
#include <sys/mman.h>
#define BMF_ARENA_BYTES ((size_t)768<<20)
#define BMF_ARENA_AT ((void*)0x0000200000000000ull)
static char* bmf_arena;
static size_t bmf_arena_used;

// The whole allocator under one lock, held across a bump or a list push -- tens
// of instructions, never a syscall except the one `mmap` the first caller makes.
// A `std::mutex` would be correct too and would pull `<mutex>` and a
// pthread dependency into a leg whose entire point is that it is the same
// program somewhere else in the address space; `__atomic_test_and_set` is two
// instructions, needs no header, and both compilers `tools/sweep.sh` runs have
// had it for a decade.  The default build has none of this: `malloc` is already
// thread-safe and the `#if` leaves it alone.
static volatile char bmf_arena_lock;
static void bmf_arena_acquire(void) {
  while( __atomic_test_and_set(&bmf_arena_lock, __ATOMIC_ACQUIRE) )
    __builtin_ia32_pause();
}
static void bmf_arena_release(void) {
  __atomic_clear(&bmf_arena_lock, __ATOMIC_RELEASE);
}
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

// The body is what it was; the lock is the wrapper below it, so that what this
// leg is checking -- the arithmetic on a pointer above 4 GB -- reads the same as
// it did before anything ran two of these at once.
static void* bmf_malloc_locked(size_t n) {
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

static void*bmf_malloc(size_t n) {
  bmf_arena_acquire();
  void* p = bmf_malloc_locked(n);
  bmf_arena_release();
  return p;
}

static void bmf_free(void* p) {
  if( !p )
    return;
  int32_t b = (int32_t)((size_t*)p)[-2];
  // One comparison, not two: a negative `b` wraps past the bucket count.
  if( (uint32_t)b>=BMF_BUCKETS )
    return;
  // The read of the bucket word is inside the lock with the write, which is the
  // whole of what the free list needs: two threads pushing into one bucket would
  // otherwise both read the same head and one of the two blocks would be lost.
  bmf_arena_acquire();
  *(void**)p = bmf_bucket[b];
  bmf_bucket[b] = p;
  bmf_arena_release();
}

#define malloc bmf_malloc
#define free bmf_free
#endif
#define BMF_PAGE ((uintptr_t)4096)
// `bmf_page_alloc` and `bmf_page_free` were here, with a `PageHeader` holding
// the unaligned pointer `free` needed in the word before the block.  The only
// two callers were the `AltP2Block` allocations, and both take from a
// `BlockPool` now; `deadcheck.py` reported the pair the moment they did.
//
// What the page alignment was for went with them.  It is kept as `alignas(4096)`
// on the pool's storage -- nothing is known to need it, and keeping it means the
// question does not have to be answered -- but it is now a property of an array
// rather than four hundred bytes of allocator.

// A fixed number of blocks of one type, in the data segment rather than on the
// heap.  Three of this program's allocations are a whole model's working set --
// `AltP1Block` at 9.6 MB, `ModelBlock` at 7.7 MB, `AltP2Block` at 1.0 MB -- and
// every one of them is a compile-time size that never depends on the input.  A
// block whose size is known and whose count is bounded does not need an
// allocator; it needs an array.
//
// `N` is the most that can be live at once, and for the two per-plane models
// that is `plane_count`, which is `((depth & 0x3F) + 7) >> 3` and so at most 4
// -- `expand_image` refuses anything outside 1..4 outright.  Measured as well as
// derived: instrumented over the corpus both directions, the high-water marks
// are 4, 4 and 1, which is what the three pools are sized for.
//
// `take` is fatal when the pool is out for the same reason `bmf_new` is fatal
// when malloc fails: every caller in this tree stores the result without
// looking, and a nullable allocator is the outlier that invites the next
// unchecked store.
//
// `zero` is not a convenience.  The page allocator this replaces memset the
// block it returned and `bmf_new` did not, so the pools differ in exactly that
// -- and a pool hands back a *reused* slot where malloc handed back a fresh one.
// See the note at `p1_blocks` for what was measured about the difference, and
// note that the p2 pool's `true` is load-bearing: filling its slots with 0xA5
// instead fails 15 of 111 checks.
template<class T, int32_t N>
struct BlockPool {
  // Page-aligned because the allocator this replaces was, and two of the three
  // went through it.  Nothing here is known to need it; keeping it means the
  // question does not have to be answered.
  alignas(4096) T slot[N];

  // **Zeroed, and the slots deliberately are not.**  A pool used to be a
  // file-scope static, so `taken` started zero because `.bss` does; a codec on
  // the heap starts with whatever was there, and `take` would then find no free
  // slot and call `bmf_fatal`.  The parallel test found this the moment it made
  // a `BMFCodec` with `new`.
  //
  // The slots themselves keep no initialiser: what a reused slot holds was
  // measured when these pools were introduced -- filling every one with 0xA5 on
  // take leaves the p1 and model pools green at 111 checks -- so the contents
  // are not read before they are written, and zeroing 86 MB per codec would
  // cost that for nothing.
  uint8_t taken[N] = {};

  T* take(bool zero) {
    for( int32_t k = 0; k<N; ++k ) {
      if( taken[k] )
        continue;
      taken[k] = 1;
      if( zero )
        memset(&slot[k], 0, sizeof(T));
      return &slot[k];
    }
    bmf_fatal(bmf_no_memory);
  }

  void give(T* p) {
    const int32_t k = (int32_t)(p-slot);
    BMF_ASSUME((uint32_t)k<(uint32_t)N);
    taken[k] = 0;
  }
};

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
