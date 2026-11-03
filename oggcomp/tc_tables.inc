constexpr long long TC_VOLMAX = (long long)1 << 60;
constexpr long long tc_vmul(long long a, long long b) { return (a <= 0 || b <= 0 || a > TC_VOLMAX / b) ? TC_VOLMAX : a * b; }
static INLINE u64 tbl_n(u64 n) {
  FATAL_UNLESS(n > 0 && n <= ((u64)1 << 40), "IDX/l*/tsvcomp-*.idx asks for a table of %" PRIu64 " entries", n);
  return n;
}
static void tc_vfits(const char *what, sz vol, sz nodes) {
  FATAL_UNLESS(vol > 0 && vol < ((sz)1 << 31) && nodes > 0,
               "IDX/l*/tsvcomp-*.idx gives the %s an index of %" PRIu64 " rows -- "
               "idx2inc.pl builds indices in int, so a Volume must stay under "
               "2147483648",
               what, (u64)vol);
}
static void tc_ifits(const char *what, sz vol, sz mult) {
  tc_vfits(what, vol, mult);
  FATAL_UNLESS(vol * mult < ((sz)1 << 31),
               "IDX/l*/tsvcomp-*.idx gives the %s %" PRIu64 " rows of %" PRIu64 " -- it addresses them with an int, so the product must stay "
               "under 2147483648",
               what, (u64)vol, (u64)mult);
}
//  F_RAWLEN and after are the v3 fields: a run of bytes that are not a page,
//  whether a group has another page, whether a page's payload is coded as
//  bytes, whether its CRC is not the one its content gives, and whether a
//  packet has bits after its last field.  IDX/l*/tsvcomp-aux.idx's fld
//  patterns are one position per field; a field past their end is clamped
//  onto the last position and shares its context, so they are widened with
//  the enum.
enum { F_MORE, F_PGTYPE, F_GRAN, F_SERIAL, F_SEQ, F_NPKT, F_PLEN, F_TAIL, F_SPILL, F_MODE, F_WPREV, F_WNEXT, F_USED, F_RAWLEN, F_PGMORE, F_PGRAW, F_CRCBAD, F_PADNZ, F_N };
constexpr int TC_MAXTAG = 128;
constexpr int TC_SGN_AUX = F_N;
constexpr int TC_SGN_HDR = TC_MAXTAG;
constexpr int TC_SGN_FLR = 1;
//  How many rows a table has.  A context index runs over its Volume, the
//  mixed-radix product the IDX patterns define, and a table whose Volume
//  fits in 2^h rows is indexed by it directly, as every table was.  One
//  that does not is hashed into 2^h rows (tc_hrow, tc_fam.inc) and
//  contexts share what they land on.  h is the family's `Number h*` in
//  its .idx -- a constant in the shipping build, so the generated array is
//  declared at this size, and a load-time value in the tuning build, so
//  _Init() maps this much.
constexpr int tc_hbits(int h) { return h < 1 ? 1 : h > 31 ? 31 : h; }
constexpr sz tc_rows(long long vol, int h) { return vol <= (1LL << tc_hbits(h)) ? (sz)vol : (sz)1 << tc_hbits(h); }
//  Every level's generated headers: its parameters, and its tables' structs.
#include "levels_h.inc"
//  Where the one object of each table type lives.  In the shipping build a
//  TC_*_T is its arrays and sizeof is the whole table; each is mapped, per
//  thread, sparse and huge-page-able, as the digit family's 893 MB always
//  was.  In the tuning build the struct is pointers and _Init() maps each
//  array at a size the live mapping objects decide, so every struct is
//  static there -- a few hundred bytes of it per thread -- and the arrays
//  are mapped, which is what that build is for.  (Not mapped: reset()
//  zeroes every mapping, and the pointers in it would go with the tables.)
//
//  The shipping build used to keep every table under 256 MB as a static
//  object -- BSS, demand-zero, one per process.  With the model's state the
//  thread's (OC_TLS), that made them 68 MB of static thread-local storage:
//  glibc zero-fills it at every thread's start, so even one thread had it
//  all resident, and it carves it out of each new thread's stack, so the
//  workers of `-t N` -- started with the default stack -- were left a few
//  kilobytes of it and died at their first call.  The tables are reached
//  through tabs' pointers either way, so mapping them changes no code that
//  touches them and no byte that is coded.  A static member of a class
//  template exists only where it is used, so in the shipping build the
//  static home is never instantiated.
constexpr sz TC_STATIC_MAX = USE_NEW ? (sz)256 << 20 : 0;
template <class T, bool S = (sizeof(T) <= TC_STATIC_MAX)> struct tc_home;
template <class T> struct tc_home<T, true> {
  static OC_TLS T obj;
  static T *get() { return &obj; }
  static void zero() { tc_zero(&obj, sizeof(T), 0); }
};
template <class T> struct tc_home<T, false> {
  static T *get() { return (T *)tc_map(sizeof(T)); }
  static void zero() {}
};
template <class T> OC_TLS T tc_home<T, true>::obj;
#ifndef TC_MEMCAP
#define TC_MEMCAP ((u64)64 << 30)
#endif
//  A level's tables: one object of each of its six families, wherever
//  tc_home put it, and the list of what it mapped.  One per level per
//  thread (oc_tabs<Lv>()), mapped by the thread's first Init at that level,
//  given back by oggcomp_Free.
template <class Lv> struct oc_tables {
  typename Lv::dig_T *dig;
  typename Lv::sgn_T *sgn;
  typename Lv::flr_T *flr;
  typename Lv::cls_T *cls;
  typename Lv::aux_T *aux;
  typename Lv::hdr_T *hdr;
  tc_maplist maps;
  int mapped;  // init() ran to its end
  u64 bytes() const { return Lv::bytes(*this); }
  void init() {
    //  From nothing, whatever an init before this one left: one that
    //  failed part way -- a mapping refused, TC_MEMCAP -- left its
    //  mappings, and the next Init mapped a second set beside them, until
    //  the list was full and said so as an internal error.
    unmap();
    tc_maps_to = &maps;
    Lv::map(*this);
    tc_maps_to = nullptr;
    FATAL_UNLESS(bytes() <= TC_MEMCAP,
                 "IDX/l%d/tsvcomp-*.idx asks for %" PRIu64 " MB of model tables, past "
                 "the %" PRIu64 " MB this build allows -- narrow a context, or "
                 "rebuild with a larger TC_MEMCAP",
                 Lv::id, (u64)(bytes() >> 20), (u64)(TC_MEMCAP >> 20));
    mapped = 1;
  }
  //  Every table back to zero, which is its initial state: what was mapped
  //  through the mappings, what is a static object through its home.  In
  //  the tuning build the objects are pointers into the mappings and are
  //  left alone.
  void reset() {
    int i;
    for(i = 0; i < maps.n; i++)
      tc_zero(maps.m[i].p, maps.m[i].n, 1);
#if !USE_NEW
    Lv::zero_static();
#endif
  }
  void unmap() {
    maps.unmap();
    mapped = 0;
  }
};
template <class Lv> struct oc_tabs_of {
  static OC_TLS oc_tables<Lv> t;
};
template <class Lv> OC_TLS oc_tables<Lv> oc_tabs_of<Lv>::t;
template <class Lv> static INLINE oc_tables<Lv> &oc_tabs() { return oc_tabs_of<Lv>::t; }
struct tc_dv {
  int rno, pass, band, col, vpos, q1, q2, q1s, q2s, t1, t1s, t2, n1, w1, p0, p0s, pn, cls, zrun, blk, bkq, ax, mg, pq, sq, ps, chn, crun, cm, x1, x1s, pmx, psm, prm;
};
struct tcx {
  int a, b, c, d, s, f, m, t, n, g;
};
//  Every level's index builders, and its class (Lv9, Lv7, ...), which the
//  model takes as its template parameter.
#include "levels.inc"
enum { STG_HDR, STG_PAGE, STG_AUD, STG_FLOOR, STG_CLASS, STG_DIGIT, STG_RAW, STG_N };
static const char *const TC_STAGE[STG_N] = {"headers", "pages", "packets", "floor", "class", "digits", "raw"};
static OC_TLS double tc_bits[STG_N];
static OC_TLS u64 tc_syms[STG_N];
static OC_TLS int tc_stage, tc_verbose;

