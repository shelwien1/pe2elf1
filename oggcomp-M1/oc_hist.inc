constexpr sz TC_SPANMAX = 1u << 17;
constexpr sz TC_HISTMAX = 1u << 23;
static i16 *dg_hist[VB_MAXRES];
static i16 *dg_hist2[VB_MAXRES];
static u32 *dg_avg[VB_MAXRES];
static i16 *dg_pp[VB_MAXRES];
static i16 *dg_ps[VB_MAXRES];
static u8 *dg_pn[VB_MAXRES];
static u8 *cl_hist[VB_MAXRES];
static sz dg_span[VB_MAXRES], cl_np[VB_MAXRES];
static i32 *dg_q1, *dg_q2, *dg_zr;
static i32 cl_last[VB_MAXRES][VB_MAXCH], cl_last2[VB_MAXRES][VB_MAXCH];
static i16 *fl_hist;
static i16 *fl_yhist;
static i32 *fl_who;
static u32 tc_blk;
static u32 tc_nchan;
static void tc_hist_free(void) {
  u32 i;
  for(i = 0; i < VB_MAXRES; i++) {
    free(dg_hist[i]);
    dg_hist[i] = nullptr;
    free(dg_hist2[i]);
    dg_hist2[i] = nullptr;
    free(dg_avg[i]);
    dg_avg[i] = nullptr;
    free(dg_pp[i]);
    dg_pp[i] = nullptr;
    free(dg_ps[i]);
    dg_ps[i] = nullptr;
    free(dg_pn[i]);
    dg_pn[i] = nullptr;
    free(cl_hist[i]);
    cl_hist[i] = nullptr;
    dg_span[i] = cl_np[i] = 0;
  }
  free(dg_q1);
  free(dg_q2);
  free(dg_zr);
  free(fl_hist);
  free(fl_yhist);
  free(fl_who);
  dg_q1 = dg_q2 = dg_zr = nullptr;
  fl_hist = nullptr;
  fl_yhist = nullptr;
  fl_who = nullptr;
}
