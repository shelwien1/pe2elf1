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
enum { F_MORE, F_PGTYPE, F_GRAN, F_SERIAL, F_SEQ, F_NPKT, F_PLEN, F_TAIL, F_SPILL, F_MODE, F_WPREV, F_WNEXT, F_USED, F_N };
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
static TC_dig_T *tcm_dig;
static TC_sgn_T *tcm_sgn;
static TC_flr_T *tcm_flr;
static TC_cls_T *tcm_cls;
static TC_aux_T *tcm_aux;
static TC_hdr_T *tcm_hdr;
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
enum { STG_HDR, STG_PAGE, STG_AUD, STG_FLOOR, STG_CLASS, STG_DIGIT, STG_N };
static const char *const TC_STAGE[STG_N] = {"headers", "pages", "packets", "floor", "class", "digits"};
static double tc_bits[STG_N];
static u64 tc_syms[STG_N];
static int tc_stage, tc_verbose;
