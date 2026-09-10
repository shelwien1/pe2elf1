constexpr long long TC_VOLMAX = (long long)1 << 60;
constexpr long long tc_vmul(long long a, long long b) { return (a <= 0 || b <= 0 || a > TC_VOLMAX / b) ? TC_VOLMAX : a * b; }
static INLINE u64 tbl_n(u64 n) {
  FATAL_UNLESS(n > 0 && n <= ((u64)1 << 40), "IDX/tsvcomp-*.idx asks for a table of %" PRIu64 " entries", n);
  return n;
}
static void tc_vfits(const char *what, sz vol, sz nodes) {
  FATAL_UNLESS(vol > 0 && vol < ((sz)1 << 31) && nodes > 0,
               "IDX/tsvcomp-*.idx gives the %s an index of %" PRIu64 " rows -- "
               "idx2inc.pl builds indices in int, so a Volume must stay under "
               "2147483648",
               what, (u64)vol);
}
static void tc_ifits(const char *what, sz vol, sz mult) {
  tc_vfits(what, vol, mult);
  FATAL_UNLESS(vol * mult < ((sz)1 << 31),
               "IDX/tsvcomp-*.idx gives the %s %" PRIu64 " rows of %" PRIu64 " -- it addresses them with an int, so the product must stay "
               "under 2147483648",
               what, (u64)vol, (u64)mult);
}
//  F_RAWLEN and after are the v3 fields: a run of bytes that are not a page,
//  whether a group has another page, whether a page's payload is coded as
//  bytes, whether its CRC is not the one its content gives, and whether a
//  packet has bits after its last field.  IDX/tsvcomp-aux.idx's fld
//  patterns are one position per field; a field past their end is clamped
//  onto the last position and shares its context, so they are widened with
//  the enum.
enum { F_MORE, F_PGTYPE, F_GRAN, F_SERIAL, F_SEQ, F_NPKT, F_PLEN, F_TAIL, F_SPILL, F_MODE, F_WPREV, F_WNEXT, F_USED, F_RAWLEN, F_PGMORE, F_PGRAW, F_CRCBAD, F_PADNZ, F_N };
constexpr int TC_MAXTAG = 128;
constexpr int TC_SGN_AUX = F_N;
constexpr int TC_SGN_HDR = TC_MAXTAG;
constexpr int TC_SGN_FLR = 1;
constexpr int TC_dig_ND = TC_NODE, TC_dig_MND = TC_MNODE;
constexpr int TC_sgn_ND = 1, TC_sgn_MND = 1;
constexpr int TC_flr_ND = TC_NODE, TC_flr_MND = TC_MNODE;
constexpr int TC_cls_ND = TC_NODE, TC_cls_MND = TC_MNODE;
constexpr int TC_aux_ND = TC_NODE, TC_aux_MND = TC_MNODE;
constexpr int TC_hdr_ND = TC_NODE, TC_hdr_MND = TC_MNODE;
#include "MOD/tsvcomp-aux_h.inc"
#include "MOD/tsvcomp-cls_h.inc"
#include "MOD/tsvcomp-dig_h.inc"
#include "MOD/tsvcomp-flr_h.inc"
#include "MOD/tsvcomp-hdr_h.inc"
#include "MOD/tsvcomp-sgn_h.inc"
//  Where the one object of each table type lives.  In the shipping build a
//  TC_*_T is its arrays, sizeof is the whole table, and under 256 MB it is
//  a static object -- BSS, demand-zero, no call to make -- while the digit
//  family's 893 MB stays mapped, sparse and huge-page-able as before.  In
//  the tuning build the struct is pointers and _Init() maps each array at
//  a size the live mapping objects decide, so every struct is static there
//  and the arrays are mapped, which is what that build is for.  A static
//  member of a class template exists only where it is used, so no 893 MB
//  object is ever declared.
constexpr sz TC_STATIC_MAX = (sz)256 << 20;
template <class T, bool S = (sizeof(T) <= TC_STATIC_MAX)> struct tc_home;
template <class T> struct tc_home<T, true> {
  static T obj;
  static T *get() { return &obj; }
};
template <class T> struct tc_home<T, false> {
  static T *get() { return (T *)tc_map(sizeof(T)); }
};
template <class T> T tc_home<T, true>::obj;
#ifndef TC_MEMCAP
#define TC_MEMCAP ((u64)64 << 30)
#endif
#if !USE_NEW
static_assert(TC_dig_T::TC_dig_Size + TC_sgn_T::TC_sgn_Size + TC_flr_T::TC_flr_Size + TC_cls_T::TC_cls_Size + TC_aux_T::TC_aux_Size + TC_hdr_T::TC_hdr_Size <= TC_MEMCAP, "IDX/tsvcomp-*.idx ask for more model tables than TC_MEMCAP allows");
#endif
//  The six families' tables: one object each, wherever tc_home put it.
struct oc_tables {
  TC_dig_T *dig;
  TC_sgn_T *sgn;
  TC_flr_T *flr;
  TC_cls_T *cls;
  TC_aux_T *aux;
  TC_hdr_T *hdr;
  u64 bytes() const { return (u64)dig->TC_dig_Size + (u64)sgn->TC_sgn_Size + (u64)flr->TC_flr_Size + (u64)cls->TC_cls_Size + (u64)aux->TC_aux_Size + (u64)hdr->TC_hdr_Size; }
  void init() {
#define TC_MAP(F) (F = tc_home<TC_##F##_T>::get(), F->TC_##F##_Init())
    TC_MAP(dig);
    TC_MAP(sgn);
    TC_MAP(flr);
    TC_MAP(cls);
    TC_MAP(aux);
    TC_MAP(hdr);
#undef TC_MAP
    FATAL_UNLESS(bytes() <= TC_MEMCAP,
                 "IDX/tsvcomp-*.idx asks for %" PRIu64 " MB of model tables, past "
                 "the %" PRIu64 " MB this build allows -- narrow a context, or "
                 "rebuild with a larger TC_MEMCAP",
                 (u64)(bytes() >> 20), (u64)(TC_MEMCAP >> 20));
  }
};
static oc_tables tabs;
struct tc_dv {
  int rno, pass, band, col, vpos, q1, q2, q1s, q2s, t1, t1s, t2, n1, w1, p0, p0s, pn, cls, zrun, blk, bkq, ax, mg, pq, sq, ps, chn, crun, cm, x1, x1s;
};
struct tcx {
  int a, b, c, d, s, f, m, t, n, g;
};
#include "MOD/tsvcomp-aux_p.inc"
#include "MOD/tsvcomp-cls_p.inc"
#include "MOD/tsvcomp-dig_p.inc"
#include "MOD/tsvcomp-flr_p.inc"
#include "MOD/tsvcomp-hdr_p.inc"
#include "MOD/tsvcomp-sgn_p.inc"
enum { STG_HDR, STG_PAGE, STG_AUD, STG_FLOOR, STG_CLASS, STG_DIGIT, STG_RAW, STG_N };
static const char *const TC_STAGE[STG_N] = {"headers", "pages", "packets", "floor", "class", "digits", "raw"};
static double tc_bits[STG_N];
static u64 tc_syms[STG_N];
static int tc_stage, tc_verbose;
