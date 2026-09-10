constexpr sz TC_SPANMAX = 1u << 17;
constexpr sz TC_HISTMAX = 1u << 23;
//  The per-stream history: what the digit, class and floor models remember
//  from the previous packets, sized by the setup header -- channels, the
//  span of each residue -- and rebuilt at every setup.  It comes out of one
//  static pool, a bump pointer over zeroed memory, because every real
//  stream fits: a stereo residue over half a block is a megabyte, a 5.1
//  one three, and the pool holds a hundred of those or one residue at the
//  TC_HISTMAX ceiling, which only a stream of over 128 channels reaches.
//  Past the pool -- sixty-four such residues are what the format allows --
//  the rest is calloc'd, zeroed all the same, and given back at reset; the
//  model neither knows nor cares where a table sits.
constexpr sz TC_HISTPOOL = (sz)128 << 20;
constexpr int TC_HISTEXTRA = 8 * (int)VB_MAXRES + 8;
struct oc_hist {
  i16 *dg_hist[VB_MAXRES];
  i16 *dg_hist2[VB_MAXRES];
  u32 *dg_avg[VB_MAXRES];
  i16 *dg_pp[VB_MAXRES];
  i16 *dg_ps[VB_MAXRES];
  u8 *dg_pn[VB_MAXRES];
  u8 *cl_hist[VB_MAXRES];
  sz dg_span[VB_MAXRES], cl_np[VB_MAXRES];
  i32 *dg_q1, *dg_q2, *dg_zr;
  i32 cl_last[VB_MAXRES][VB_MAXCH], cl_last2[VB_MAXRES][VB_MAXCH];
  i16 *fl_hist;
  i16 *fl_yhist;
  i32 *fl_who;
  u32 tc_blk;
  u32 tc_nchan;
  sz used;
  int nextra;
  void *extra[TC_HISTEXTRA];
  alignas(64) u8 pool[TC_HISTPOOL];
  void *take(sz n) {
    sz want = (n + 63) & ~(sz)63;
    void *p;
    if(want <= TC_HISTPOOL - used) {
      p = pool + used;
      used += want;
      memset(p, 0, want);
      return p;
    }
    FATAL_UNLESS(nextra < TC_HISTEXTRA, "internal: more history tables than a setup can have");
    p = calloc(n ? n : 1, 1);
    if(!p)
      FATAL_CODE(OGC_EXIT_IO, "out of memory for %" PRIu64 " bytes", (u64)n);
    extra[nextra++] = p;
    return p;
  }
  void reset() {
    u32 i;
    for(i = 0; i < VB_MAXRES; i++) {
      dg_hist[i] = nullptr;
      dg_hist2[i] = nullptr;
      dg_avg[i] = nullptr;
      dg_pp[i] = nullptr;
      dg_ps[i] = nullptr;
      dg_pn[i] = nullptr;
      cl_hist[i] = nullptr;
      dg_span[i] = cl_np[i] = 0;
    }
    dg_q1 = dg_q2 = dg_zr = nullptr;
    fl_hist = nullptr;
    fl_yhist = nullptr;
    fl_who = nullptr;
    while(nextra)
      free(extra[--nextra]);
    used = 0;
  }
};
static oc_hist hist;
