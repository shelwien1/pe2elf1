//  levels.inc -- the second half of each level (levels_h.inc is the first):
//  its index builders (MOD/l<L>/tsvcomp-<family>_p.inc), and its class, Lv<L>,
//  which the model takes as its template parameter (oc_model<f_DEC, Lv>,
//  oc_model.inc).  The class is what of the model is the level's: the types
//  of its families' tables, its index builders, the parameters the model
//  reads beside its families', and how the families are wired to the tables
//  -- every name the generator gave the level, so that nothing else in the
//  model names one.  All of it is the one macro below, OC_LEVEL(n), so the
//  levels cannot drift apart in anything but their IDX sources.

//  A family wired to its tables and parameters (tc_fam.inc, wire()): the
//  generated names of family F at the level whose prefix is P (L9_, ...), in
//  the order wire() takes them.
#define OC_WIRE(f, t, P, F, g, ng, rt, mwt, rg, mwg, lrmt, mbmt)                                                                                      \
  (f).wire((t)->P##F##_A, P##F##_a_Volume, (t)->P##F##_B, P##F##_b_Volume, (t)->P##F##_C, P##F##_c_Volume, (t)->P##F##_D, P##F##_d_Volume,      \
           (t)->P##F##_MT, P##F##_t_Volume, (t)->P##F##_S, (t)->P##F##_SC, P##F##_s_Volume, (t)->P##F##_F, (t)->P##F##_FC, P##F##_f_Volume,       \
           (t)->P##F##_W, P##F##_m_Volume, (g), (ng), P##F##_g_Volume, (t)->P##F##_WM, P##F##_n_Volume, (t)->P##F##_WC, (t)->P##F##_WMC,         \
           P##F##_rA, P##F##_rB, P##F##_rC, P##F##_rD, (rt), (rg), P##F##_mwA, P##F##_mwB, P##F##_mwC, P##F##_mwD, (mwt), (mwg), P##F##_rS1,      \
           P##F##_rS2, P##F##_lr, P##F##_mb, (lrmt), (mbmt), P##F##_bw, P##F##_qs, P##F##_qf, P##F##_ND, P##F##_MND, P##F##_hA, P##F##_hB,        \
           P##F##_hD, P##F##_hS, P##F##_hF, P##F##_hM, P##F##_hT, P##F##_hN)
//  A family with a mantissa plane and sign counters of its own.
#define OC_WIRE_ALL(f, t, P, F, g, ng) OC_WIRE(f, t, P, F, g, ng, P##F##_rT, P##F##_mwT, P##F##_rG, P##F##_mwG, P##F##_lrm, P##F##_mbm)

//  The level whose IDX sources are IDX/l<n>: its tables, builders and
//  parameters by the prefix the generator gave them, L<n>_.
#define OC_LEVEL(n)                                                                                                     \
  struct Lv##n {                                                                                                        \
    static constexpr int id = n; /* the header's level byte */                                                          \
    typedef L##n##_dig_T dig_T;                                                                                         \
    typedef L##n##_sgn_T sgn_T;                                                                                         \
    typedef L##n##_flr_T flr_T;                                                                                         \
    typedef L##n##_cls_T cls_T;                                                                                         \
    typedef L##n##_aux_T aux_T;                                                                                         \
    typedef L##n##_hdr_T hdr_T;                                                                                         \
    /* the index builders, as the templates declare them */                                                             \
    static INLINE void make_dig(const tc_dv &d, tcx &x) { L##n##_dig_make(d, x); }                                      \
    static INLINE void make_sgn(const tc_dv &d, tcx &x) { L##n##_sgn_make(d, x); }                                      \
    template <class... A> static INLINE void make_flr(A &&...a) { L##n##_flr_make(static_cast<A &&>(a)...); }           \
    template <class... A> static INLINE void make_cls(A &&...a) { L##n##_cls_make(static_cast<A &&>(a)...); }           \
    template <class... A> static INLINE void make_aux(A &&...a) { L##n##_aux_make(static_cast<A &&>(a)...); }           \
    template <class... A> static INLINE void make_hdr(A &&...a) { L##n##_hdr_make(static_cast<A &&>(a)...); }           \
    /* the weights of a digit's expected magnitude (oc_residue.inc) */                                                  \
    static INLINE i64 avA() { return L##n##_dig_avA; }                                                                  \
    static INLINE i64 avC() { return L##n##_dig_avC; }                                                                  \
    static INLINE i64 avD() { return L##n##_dig_avD; }                                                                  \
    static INLINE i64 avN1() { return L##n##_dig_avN1; }                                                                \
    static INLINE i64 avQ1() { return L##n##_dig_avQ1; }                                                                \
    static INLINE i64 avQ2() { return L##n##_dig_avQ2; }                                                                \
    static INLINE i64 avT1() { return L##n##_dig_avT1; }                                                                \
    static INLINE i64 avT2() { return L##n##_dig_avT2; }                                                                \
    static INLINE i64 avW1() { return L##n##_dig_avW1; }                                                                \
    static INLINE i64 avX1() { return L##n##_dig_avX1; }                                                                \
    /* the tables (oc_tables, tc_tables.inc): where tc_home puts each, mapped and sized by the generated _Init */      \
    template <class T> static void map(T &t) {                                                                          \
      t.dig = tc_home<dig_T>::get(), t.dig->L##n##_dig_Init();                                                          \
      t.sgn = tc_home<sgn_T>::get(), t.sgn->L##n##_sgn_Init();                                                          \
      t.flr = tc_home<flr_T>::get(), t.flr->L##n##_flr_Init();                                                          \
      t.cls = tc_home<cls_T>::get(), t.cls->L##n##_cls_Init();                                                          \
      t.aux = tc_home<aux_T>::get(), t.aux->L##n##_aux_Init();                                                          \
      t.hdr = tc_home<hdr_T>::get(), t.hdr->L##n##_hdr_Init();                                                          \
    }                                                                                                                   \
    template <class T> static u64 bytes(const T &t) {                                                                   \
      return (u64)t.dig->L##n##_dig_Size + (u64)t.sgn->L##n##_sgn_Size + (u64)t.flr->L##n##_flr_Size +                  \
             (u64)t.cls->L##n##_cls_Size + (u64)t.aux->L##n##_aux_Size + (u64)t.hdr->L##n##_hdr_Size;                   \
    }                                                                                                                   \
    static void zero_static() {                                                                                         \
      tc_home<dig_T>::zero();                                                                                           \
      tc_home<sgn_T>::zero();                                                                                           \
      tc_home<flr_T>::zero();                                                                                           \
      tc_home<cls_T>::zero();                                                                                           \
      tc_home<aux_T>::zero();                                                                                           \
      tc_home<hdr_T>::zero();                                                                                           \
    }                                                                                                                   \
    /* the model's six families wired to the tables t of this level */                                                  \
    template <class M, class T> static void wire(M &m, T &t) {                                                          \
      OC_WIRE(m.fam_dig, t.dig, L##n##_, dig, (cm_cnt *)nullptr, 0, L##n##_dig_rT, L##n##_dig_mwT, 0, 0, L##n##_dig_lrm, \
              L##n##_dig_mbm);                                                                                          \
      OC_WIRE(m.fam_sgn, t.sgn, L##n##_, sgn, (cm_cnt *)nullptr, 0, 0, 0, 0, 0, 0, 0);                                  \
      OC_WIRE_ALL(m.fam_flr, t.flr, L##n##_, flr, t.flr->L##n##_flr_G, TC_SGN_FLR);                                     \
      OC_WIRE(m.fam_cls, t.cls, L##n##_, cls, (cm_cnt *)nullptr, 0, L##n##_cls_rT, L##n##_cls_mwT, 0, 0, L##n##_cls_lrm, \
              L##n##_cls_mbm);                                                                                          \
      OC_WIRE_ALL(m.fam_aux, t.aux, L##n##_, aux, t.aux->L##n##_aux_G, TC_SGN_AUX);                                     \
      OC_WIRE_ALL(m.fam_hdr, t.hdr, L##n##_, hdr, t.hdr->L##n##_hdr_G, TC_SGN_HDR);                                     \
    }                                                                                                                   \
  };

//  In the shipping build every table's size is a constant, and a level that
//  asks for more than TC_MEMCAP is refused at compile time.
#define OC_LV_CAP(n)                                                                                          \
  static_assert((u64)L##n##_dig_T::L##n##_dig_Size + L##n##_sgn_T::L##n##_sgn_Size + L##n##_flr_T::L##n##_flr_Size + \
                        L##n##_cls_T::L##n##_cls_Size + L##n##_aux_T::L##n##_aux_Size + L##n##_hdr_T::L##n##_hdr_Size <= \
                    TC_MEMCAP,                                                                                \
                "IDX/l" #n "/tsvcomp-*.idx ask for more model tables than TC_MEMCAP allows");

#if OC_LV(9)
#include "MOD/l9/tsvcomp-aux_p.inc"
#include "MOD/l9/tsvcomp-cls_p.inc"
#include "MOD/l9/tsvcomp-dig_p.inc"
#include "MOD/l9/tsvcomp-flr_p.inc"
#include "MOD/l9/tsvcomp-hdr_p.inc"
#include "MOD/l9/tsvcomp-sgn_p.inc"
OC_LEVEL(9)
#if !USE_NEW
OC_LV_CAP(9)
#endif
#define OC_L9(X) X(9)
#else
#define OC_L9(X)
#endif
#if OC_LV(7)
#include "MOD/l7/tsvcomp-aux_p.inc"
#include "MOD/l7/tsvcomp-cls_p.inc"
#include "MOD/l7/tsvcomp-dig_p.inc"
#include "MOD/l7/tsvcomp-flr_p.inc"
#include "MOD/l7/tsvcomp-hdr_p.inc"
#include "MOD/l7/tsvcomp-sgn_p.inc"
OC_LEVEL(7)
#if !USE_NEW
OC_LV_CAP(7)
#endif
#define OC_L7(X) X(7)
#else
#define OC_L7(X)
#endif
#if OC_LV(3)
#include "MOD/l3/tsvcomp-aux_p.inc"
#include "MOD/l3/tsvcomp-cls_p.inc"
#include "MOD/l3/tsvcomp-dig_p.inc"
#include "MOD/l3/tsvcomp-flr_p.inc"
#include "MOD/l3/tsvcomp-hdr_p.inc"
#include "MOD/l3/tsvcomp-sgn_p.inc"
OC_LEVEL(3)
#if !USE_NEW
OC_LV_CAP(3)
#endif
#define OC_L3(X) X(3)
#else
#define OC_L3(X)
#endif
#if OC_LV(1)
#include "MOD/l1/tsvcomp-aux_p.inc"
#include "MOD/l1/tsvcomp-cls_p.inc"
#include "MOD/l1/tsvcomp-dig_p.inc"
#include "MOD/l1/tsvcomp-flr_p.inc"
#include "MOD/l1/tsvcomp-hdr_p.inc"
#include "MOD/l1/tsvcomp-sgn_p.inc"
OC_LEVEL(1)
#if !USE_NEW
OC_LV_CAP(1)
#endif
#define OC_L1(X) X(1)
#else
#define OC_L1(X)
#endif
#undef OC_LV_CAP

//  The levels of this build, the strongest first: X(9) X(7) X(3) X(1), or the
//  one an OC_ONLY build has.  The first is what c codes at when not told.
#define OC_LEVELS(X) OC_L9(X) OC_L7(X) OC_L3(X) OC_L1(X)
#define OC_LV_FIRST(n) n,
static constexpr int oc_levels[] = {OC_LEVELS(OC_LV_FIRST)};
#undef OC_LV_FIRST
static constexpr int OC_LEVEL_DEFAULT = oc_levels[0];
//  Whether this build has level n.
static inline int oc_has_level(int n) {
  for(int l : oc_levels)
    if(l == n)
      return 1;
  return 0;
}
