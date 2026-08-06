/* d5c_counter.inc -- part of dxt5comp_core.inc; not standalone.
 *
 * Ctr / CtrS DECLARATIONS and the restrict pointer types.
 *
 * ORDERING: this must precede the MOD/*_h.inc includes, because the generated
 * Table() members are typed `Ctr`.  The upd() BODIES cannot live here -- they
 * need CW[] and G0_SLOW_MUL, which those same generated headers define -- so
 * they are in d5c_counter_upd.inc, included after them. */

static u32 CW[16]; // counter rate multiplier schedule, filled from G0_CW* in init_tables()

/* Element count for a generated Table().
 *
 * In the Debug build a table's size is an IDX volume, computed at load time
 * from the !MAP! pattern strings that opt.pl rewrites inside the binary, so
 * the compiler knows nothing about its range -- and it is a signed int, which
 * converts to an enormous size_t if it were ever negative.  `new T[n]` must
 * therefore carry the standard guard: the edge that calls operator new[] with
 * (size_t)-1 (or throws bad_array_new_length) so the allocation is certain to
 * fail.  GCC 13+ reports that compiler-inserted edge as
 * -Walloc-size-larger-than, once per table, which is pure noise that buries
 * real diagnostics.
 *
 * Checking the bound here instead removes the edge -- after the abort the
 * compiler knows n is in [1, 2^32], so the byte count provably cannot
 * overflow -- and turns a genuinely broken parameter set (a bit flip that
 * widens a mask far enough to ask for a nonsense allocation) into an
 * immediate, named abort rather than a bad_alloc or silent corruption. */
#define D5C_TBL_MAX (1LL << 32)
static inline size_t tbl_n(long long n) {
  if(n < 1 || n > D5C_TBL_MAX) {
    fprintf(stderr, "dxt5comp: table size %lld out of range -- bad IDX parameter set\n", n);
    abort();
  }
  return (size_t)n;
}

struct Ctr {
  u16 p = CTR_INIT; u16 n = 0;
  inline int pr() const { int v = p >> CTR_SH; return v < P_MIN ? P_MIN : v > P_MAX ? P_MAX : v; }
  inline void upd(int bit);        // needs CW[]; defined below
};

struct CtrS { // slow fixed-rate counter
  u16 p = CTR_INIT;
  inline int pr() const { int v = p >> CTR_SH; return v < P_MIN ? P_MIN : v > P_MAX ? P_MAX : v; }
  inline void upd(int bit);        // needs G0_SLOW_MUL; defined below
};

/* Restrict-qualified pointer types, so the declarations below stay short.
 *
 * __restrict is a promise that, for the lifetime of the declaration, the
 * pointed-to object is reached only through this pointer or one derived from
 * it.  Breaking it is UB the optimizer will act on, silently, so these are
 * used ONLY where non-aliasing is structural rather than merely true today:
 *   - context tables passed side by side (tree(), match(), mbit()): every
 *     call site draws its rows from DIFFERENT CtrTab slabs, checked
 *     statically and, under -DD5C_RESTRICT_CHECK=1, asserted at run time
 *   - the texture image against the model tables
 *   - a mixer's weight row against its input array
 *
 * They are deliberately NOT used on the swapped geometry members (pR/qR,
 * cBlk/qBlk, rowP/rowC, pA/pA2 ...).  level_init rotates the finished level
 * into the parent slot and achan_swap exchanges the BC5 channel set, so over
 * the object's lifetime the same slab really is reached through more than one
 * of those members -- precisely what restrict forbids.  Nor on the model-table
 * views, which alias the generated Table() members they were bound to. */
typedef Ctr*        __restrict CtrP;
typedef CtrS*       __restrict CtrSP;
typedef Ctr* const* __restrict CtrPP;
typedef u8*         __restrict u8P;
typedef const u8*   __restrict cu8P;
typedef int*        __restrict intP;
typedef u16*        __restrict u16P;

/* Reset state of a counter is NOT all-zero (p = CTR_INIT), so refilling a table
 * is a pattern fill rather than a memset.  These are the only two places that
 * know it. */
static inline void fill_ctr(Ctr* t, size_t n) { for(size_t i = 0; i < n; i++) t[i] = Ctr(); }
static inline void fill_ctrs(CtrS* t, size_t n) { for(size_t i = 0; i < n; i++) t[i] = CtrS(); }

// generated parameter / context definitions (IDX system; see IDX/*.idx)
