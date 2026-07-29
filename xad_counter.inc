/* xad_counter.inc -- part of xadpcm.cpp; not standalone.
 *
 * Ctr / CtrS and the tables of them.  Every rate is a 16-bit fixed-point
 * MULTIPLIER rather than a shift -- see the note below. */

/* The counters, mixer and APM all step by a 16-bit fixed-point MULTIPLIER
   rather than a shift.  mul = 65536>>shift reproduces any shift exactly -- the
   product is exact and the >>16 is the same arithmetic shift, negative deltas
   included -- so this is byte-identical to the shift model at the seeded
   values.  What it buys is the space between the powers of two, which a shift
   schedule can never reach and which is where the rate schedule (the biggest
   knob found by hand) is most likely to still have something left. */

struct Ctr {
  u16 p = CTR_INIT;
  u16 n = 0;

  INLINE int pr() const {
    int v = p>>CTR_SH;
    return v<P_MIN ? P_MIN : v>P_MAX ? P_MAX : v;
  }

  INLINE void upd(int bit, const RateSet &rs) {
    int d = (bit ? CTR_MAX : 0)-int(p);
    p = u16(int(p)+int((i64(d)*i64(rs.cw[n]))>>CTR_BITS));
    if( n<rs.nmax )
      n++;
  }
};

struct CtrS { // slow fixed-rate twin of the primary code counter
  u16 p = CTR_INIT;

  INLINE int pr() const {
    int v = p>>CTR_SH;
    return v<P_MIN ? P_MIN : v>P_MAX ? P_MAX : v;
  }

  INLINE void upd(int bit) {
    int d = (bit ? CTR_MAX : 0)-int(p);
    p = u16(int(p)+int((i64(d)*i64(PMUL(G0_SLOW_MUL)))>>CTR_BITS));
  }
};

/* Restrict-qualified counter pointers.  See the doctrine in xad_prelude.inc:
   these are for parameters and locals, never for the CtrTab/CtrSTab members
   they are read out of.  CtrPP is the row-base ARRAY mbit receives -- the outer
   pointer is restrict, the 26 inner ones are ordinary Ctr* because each is
   separately established to be non-aliasing at its own declaration. */
typedef Ctr* __restrict CtrP;
typedef CtrS* __restrict CtrSP;
typedef Ctr* const* __restrict CtrPP;

/* ONE ARENA FOR EVERY COUNTER TABLE.
 *
 * The tables were 24 separate std::vectors, each 16-byte aligned.  At nsym = 16
 * a row is sizeof(Ctr)*16 = 64 bytes -- exactly one cache line -- so on a
 * 16-aligned base about three rows in four straddled two lines and cost two
 * misses where one would do.  Within a symbol the node index walks
 * 1 -> {2,3} -> {4..7} -> {8..15}, all inside one row, so an aligned table
 * costs ONE line per symbol rather than one per bit.
 *
 * Sub-allocating them all from one block does three further things: it drops
 * the page count (~27 MB of essentially random access against a 1500-2500 entry
 * dTLB), it lets the whole arena be backed by huge pages where the OS offers
 * them (~7000 4 KB pages -> ~14 2 MB pages), and it makes reset_stats a walk
 * over contiguous memory instead of 24 vector::assign calls.
 *
 * The tables are NOT merged into a common indexed slab: they have different
 * index spaces and row counts (tP: nsym, tP4: nsym^4, tHJ: 2^hj_bits), so no
 * shared row index exists.  Alignment plus one allocation is the achievable
 * form of that intent. */
/* Blocks handed out are 64B-aligned and ZEROED.  The zeroing matters as much as
   the alignment: the match model's hash tables are half a gigabyte and must
   start at zero for the bounds test in MatchModel::update to be deterministic,
   but they are also touched sparsely -- so they must be zeroed LAZILY, by the
   kernel's demand-zero pages, not by a memset that walks all 512 MB up front.
   `huge` is what separates the two users:

     cta (counter tables, ~27 MB, densely touched, refilled per segment)
        -> MADV_HUGEPAGE.  ~7000 4 KB pages become ~14 2 MB ones.
     mta (match history + hash, ~512 MB, touched at random and sparsely)
        -> NO madvise.  Under THP a first touch would fault in 2 MB where the
           model wants 4 KB, which is 512x write amplification on a table most
           runs never fill. */
struct Arena {
  u8* base = nullptr;
  size_t cap = 0, used = 0;
  int dry = 0;    // measuring pass: hand out nothing, just accumulate
  int huge = 1;   // ask for huge pages (dense users only)
  int mapped = 0; // base came from mmap rather than the aligned_alloc fallback

  INLINE void* take(size_t n) {
    used = (used+63)&~size_t(63);
    void* p = dry ? nullptr : base+used;
    used += n;
    return p;
  }
  void rewind() { used = 0; }

  /* Hand the block back so the next reserve() sizes a fresh one.  Only the
     counter arena ever does this, and only when a wider segment arrives -- see
     Codec::grow_geom. */
  /* Which allocator handed out `base`.  reserve() falls back to aligned_alloc
     when mmap fails, and releasing with the wrong one is not a leak -- munmap on
     a malloc'd pointer fails with EINVAL and the block stays live forever. */
  void release() {
#ifdef __linux__
    if( base&&mapped )
      munmap(base, cap);
    else if( base )
      free(base);
#else
    free(base);
#endif
    mapped = 0;
    base = nullptr;
    cap = used = 0;
  }

  /* Caught here rather than as a heap corruption: sizes are fixed after
     Codec::init, so a second init with a wider geometry would lay a bigger
     table set into the block sized for the first one. */
  void check() const {
    if( !dry&&used>cap ) {
      fprintf(stderr, "xadpcm: model arena overrun (%zu > %zu) -- geometry changed after init\n", used, cap);
      abort();
    }
  }

  /* Fault the whole block in sequentially.  The pages are already zero -- this
     writes zeros over zeros -- but it converts N random demand faults into one
     streaming pass, which is the cheaper shape once the workload is going to
     touch most of the block anyway.  See Codec::init for when it is worth it. */
  void prefault() {
    if( base )
      memset(base, 0, used);
  }

  void reserve(size_t n) {
    if( n<=cap )
      return;
#ifdef __linux__
    size_t sz = (n+(size_t(2)<<20)-1)&~((size_t(2)<<20)-1);
    void* p = mmap(nullptr, sz, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if( p!=MAP_FAILED ) {
      if( huge )
        madvise(p, sz, MADV_HUGEPAGE);
      base = (u8*)p;   // mmap gives zeroed pages, faulted in on first touch
      cap = sz;
      mapped = 1;
      return;
    }
#endif
    /* Fallback: no demand-zero, so pay the memset once.  Correctness needs the
       zero; only the laziness is lost. */
    size_t sz2 = (n+4095)&~size_t(4095);
    void* q = 0;
#if defined(_WIN32)
    q = _aligned_malloc(sz2, 4096);
#else
    q = aligned_alloc(4096, sz2);
#endif
    if( !q ) {
      fprintf(stderr, "xadpcm: out of memory allocating %zu bytes of model tables\n", sz2);
      exit(3);
    }
    memset(q, 0, sz2);
    base = (u8*)q;
    cap = sz2;
  }
};

/* Reset state of a counter is p = rs.init, n = 0 -- not byte-uniform, so memset
   does not apply.  The pattern is built from a real Ctr rather than assembled
   from shifts, so it is right on either endianness. */
static INLINE void fill_ctr(void* dst, size_t n, u32 pat) {
  u32P q = (u32P)dst;
  for( size_t i = 0; i<n; i++ )
    q[i] = pat;
}

struct CtrTab {
  Ctr* v = nullptr;
  int w = 0;

  void init(Arena &a, int rows, int width, const RateSet &rs) {
    w = width;
    size_t n = size_t(rows)*size_t(width);
    v = (Ctr*)a.take(n*sizeof(Ctr));
    if( v ) {
      Ctr c;
      c.p = rs.init;
      c.n = 0;
      u32 pat;
      memcpy(&pat, &c, sizeof(pat));
      fill_ctr(v, n, pat);
    }
  }

  INLINE Ctr*row(int r) const {
    return v+size_t(r)*size_t(w);
  }
};

struct CtrSTab {
  CtrS* v = nullptr;
  int w = 0;

  void init(Arena &a, int rows, int width) {
    w = width;
    size_t n = size_t(rows)*size_t(width);
    v = (CtrS*)a.take(n*sizeof(CtrS));
    if( v ) {
      CtrS c;
      c.p = u16(pclamp(G0_SP_INIT, 1, CTR_MAX));
      for( size_t i = 0; i<n; i++ )
        v[i] = c;
    }
  }

  INLINE CtrS*row(int r) const {
    return v+size_t(r)*size_t(w);
  }
};
