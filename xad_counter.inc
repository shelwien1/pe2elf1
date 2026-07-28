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
struct Arena {
  u8* base = nullptr;
  size_t cap = 0, used = 0;
  int dry = 0;   // measuring pass: hand out nothing, just accumulate

  INLINE void* take(size_t n) {
    used = (used+63)&~size_t(63);
    void* p = dry ? nullptr : base+used;
    used += n;
    return p;
  }
  void rewind() { used = 0; }

  void reserve(size_t n) {
    if( n<=cap )
      return;
#ifdef __linux__
    /* Huge pages are worth 5-15% on their own here, purely from the dTLB.
       MADV_HUGEPAGE is a hint: if the kernel has THP disabled the mapping is
       still valid, just backed by 4 KB pages, so there is nothing to fall back
       from.  A failed mmap does fall back, below. */
    size_t sz = (n+(size_t(2)<<20)-1)&~((size_t(2)<<20)-1);
    void* p = mmap(nullptr, sz, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if( p!=MAP_FAILED ) {
      madvise(p, sz, MADV_HUGEPAGE);
      base = (u8*)p;
      cap = sz;
      return;
    }
#endif
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
    base = (u8*)q;
    cap = sz2;
  }
};

/* Reset state of a counter is p = rs.init, n = 0 -- not byte-uniform, so memset
   does not apply.  The pattern is built from a real Ctr rather than assembled
   from shifts, so it is right on either endianness. */
static INLINE void fill_ctr(void* dst, size_t n, u32 pat) {
  u32* q = (u32*)dst;
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
