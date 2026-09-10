static void tc_setup_done(void) {
  const vb_setup *s = oc_v->cur;
  u32 i;
  tc_hist_free();
  tcp_build();
  tc_nchan = oc_v->i.ch;
  for(i = 0; i < s->nrs; i++) {
    const vb_res *r = s->rs + i;
    sz span = r->end > r->beg ? r->end - r->beg : 0;
    sz need;
    if(span > TC_SPANMAX)
      span = 0;
    need = (sz)2 * 8 * tc_nchan * span;
    if(need > TC_HISTMAX)
      span = 0;
    dg_span[i] = span;
    if(span) {
      dg_hist[i] = (i16 *)tc_alloc((sz)2 * 8 * tc_nchan * span * sizeof(i16));
      dg_hist2[i] = (i16 *)tc_alloc((sz)2 * 8 * tc_nchan * span * sizeof(i16));
      dg_avg[i] = (u32 *)tc_alloc((sz)2 * 8 * tc_nchan * span * sizeof(u32));
      dg_pp[i] = (i16 *)tc_alloc((sz)tc_nchan * span * sizeof(i16));
      dg_ps[i] = (i16 *)tc_alloc((sz)tc_nchan * span * sizeof(i16));
      dg_pn[i] = (u8 *)tc_alloc((sz)tc_nchan * span);
    }
    cl_np[i] = span ? span / r->psz + 2 : 0;
    if(cl_np[i])
      cl_hist[i] = (u8 *)tc_alloc((sz)2 * tc_nchan * cl_np[i]);
    memset(cl_last[i], 0, sizeof cl_last[i]);
    memset(cl_last2[i], 0, sizeof cl_last2[i]);
  }
  dg_q1 = (i32 *)tc_alloc((sz)s->nrs * 8 * tc_nchan * sizeof(i32));
  dg_q2 = (i32 *)tc_alloc((sz)s->nrs * 8 * tc_nchan * sizeof(i32));
  dg_zr = (i32 *)tc_alloc((sz)s->nrs * 8 * tc_nchan * sizeof(i32));
  fl_hist = (i16 *)tc_alloc((sz)2 * tc_nchan * VB_MAXPOST * sizeof(i16));
  fl_yhist = (i16 *)tc_alloc((sz)2 * tc_nchan * VB_MAXPOST * sizeof(i16));
  fl_who = (i32 *)tc_alloc((sz)2 * tc_nchan * sizeof(i32));
  for(i = 0; i < 2 * tc_nchan; i++)
    fl_who[i] = -1;
  for(i = 0; i < s->nfl; i++)
    oc_floor_tables(s->fl + i, oc_fl + i);
}
#ifdef TC_MEMCOST
#if TC_MEMCOST + 0 <= 1
#undef TC_MEMCOST
#define TC_MEMCOST 10000
#endif
#endif
#define TC_WIRE(f, F, g, ng, rt, mwt, rg, mwg, lrmt, mbmt) (f).wire(tcm_##F->TC_##F##_A, TC_##F##_a_Volume, tcm_##F->TC_##F##_B, TC_##F##_b_Volume, tcm_##F->TC_##F##_C, TC_##F##_c_Volume, tcm_##F->TC_##F##_D, TC_##F##_d_Volume, tcm_##F->TC_##F##_MT, TC_##F##_t_Volume, tcm_##F->TC_##F##_S, tcm_##F->TC_##F##_SC, TC_##F##_s_Volume, tcm_##F->TC_##F##_F, tcm_##F->TC_##F##_FC, TC_##F##_f_Volume, tcm_##F->TC_##F##_W, TC_##F##_m_Volume, (g), (ng), TC_##F##_g_Volume, tcm_##F->TC_##F##_WM, TC_##F##_n_Volume, tcm_##F->TC_##F##_WC, tcm_##F->TC_##F##_WMC, TC_##F##_rA, TC_##F##_rB, TC_##F##_rC, TC_##F##_rD, (rt), (rg), TC_##F##_mwA, TC_##F##_mwB, TC_##F##_mwC, TC_##F##_mwD, (mwt), (mwg), TC_##F##_rS1, TC_##F##_rS2, TC_##F##_lr, TC_##F##_mb, (lrmt), (mbmt), TC_##F##_bw, TC_##F##_qs, TC_##F##_qf, TC_##F##_ND, TC_##F##_MND)
#define TC_WIRE_ALL(f, F, g, ng) TC_WIRE(f, F, g, ng, TC_##F##_rT, TC_##F##_mwT, TC_##F##_rG, TC_##F##_mwG, TC_##F##_lrm, TC_##F##_mbm)
#ifndef TC_MEMCAP
#define TC_MEMCAP ((u64)64 << 30)
#endif
static INLINE u64 tc_tables(void) { return (u64)tcm_dig->TC_dig_Size + (u64)tcm_sgn->TC_sgn_Size + (u64)tcm_flr->TC_flr_Size + (u64)tcm_cls->TC_cls_Size + (u64)tcm_aux->TC_aux_Size + (u64)tcm_hdr->TC_hdr_Size; }
#if !USE_NEW
static_assert(TC_dig_T::TC_dig_Size + TC_sgn_T::TC_sgn_Size + TC_flr_T::TC_flr_Size + TC_cls_T::TC_cls_Size + TC_aux_T::TC_aux_Size + TC_hdr_T::TC_hdr_Size <= TC_MEMCAP, "IDX/tsvcomp-*.idx ask for more model tables than TC_MEMCAP allows");
#endif
static void tc_models(void) {
  cm_tables();
#define TC_MAP(F) (tcm_##F = (TC_##F##_T *)tc_map(sizeof(TC_##F##_T)), tcm_##F->TC_##F##_Init())
  TC_MAP(dig);
  TC_MAP(sgn);
  TC_MAP(flr);
  TC_MAP(cls);
  TC_MAP(aux);
  TC_MAP(hdr);
#undef TC_MAP
  FATAL_UNLESS(tc_tables() <= TC_MEMCAP,
               "IDX/tsvcomp-*.idx asks for %" PRIu64 " MB of model tables, past "
               "the %" PRIu64 " MB this build allows -- narrow a context, or "
               "rebuild with a larger TC_MEMCAP",
               (u64)(tc_tables() >> 20), (u64)(TC_MEMCAP >> 20));
  TC_WIRE(fam_dig, dig, (cm_cnt *)nullptr, 0, TC_dig_rT, TC_dig_mwT, 0, 0, TC_dig_lrm, TC_dig_mbm);
  TC_WIRE(fam_sgn, sgn, (cm_cnt *)nullptr, 0, 0, 0, 0, 0, 0, 0);
  TC_WIRE_ALL(fam_flr, flr, tcm_flr->TC_flr_G, TC_SGN_FLR);
  TC_WIRE(fam_cls, cls, (cm_cnt *)nullptr, 0, TC_cls_rT, TC_cls_mwT, 0, 0, TC_cls_lrm, TC_cls_mbm);
  TC_WIRE_ALL(fam_aux, aux, tcm_aux->TC_aux_G, TC_SGN_AUX);
  TC_WIRE_ALL(fam_hdr, hdr, tcm_hdr->TC_hdr_G, TC_SGN_HDR);
}
static void tc_nomem(void) {
  FATAL_CODE(OGC_EXIT_IO, "out of memory -- IDX/tsvcomp-*.idx asks for more "
                          "than this machine has");
}
struct oc_model {
  u32 link(u32 more) { return oc_link(more); }
  void link_begin() { oc_link_begin(); }
  void page(ogg_page &p, int cont) { oc_page(p, cont); }
  u32 spill(u32 n) { return oc_spill(n); }
  u32 hdr(const char *tag, u32 x, u64 hi) { return oc_hdr(tag, x, hi); }
  void setup_done(vb_ctx &v) {
    oc_v = &v;
    tc_setup_done();
  }
  u32 mode(u32 md, u32 nmd) { return oc_mode(md, nmd); }
  u32 wprev(u32 w) { return oc_wprev(w); }
  u32 wnext(u32 w) { return oc_wnext(w); }
  void floor(u32 k, u32 fno, vb_floor *f, u32 *y, int &used) { oc_flr(k, fno, f, y, used); }
  void residue_begin(u32 rno, vb_res *r, const u8 *nz, u32 nch, u32 n) { oc_residue_begin(rno, r, nz, nch, n); }
  u32 cls(u32 rno, u32 j, u32 pc, u32 k, u32 c) { return oc_cls(rno, j, pc, k, c); }
  void part_begin(u32 rno, u32 pass, u32 j, u32 pc, vb_res *r, vb_book *bk, u32 bn, u32 c) { oc_part_begin(rno, pass, j, pc, r, bk, bn, c); }
  void vec_begin() { fam_dig.pc.start(P.pt); }
  i64 digit(i64 d) { return oc_digit(d); }
};
typedef oc_model sink_t;
