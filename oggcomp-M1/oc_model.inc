//  What the floor model keeps per floor: the post order the setup implies,
//  and for every post the two neighbours the spec's renderer predicts from.
struct oc_floor {
  u8 pcl[VB_MAXPOST], ppos[VB_MAXPOST];
  u8 rnk[VB_MAXPOST];
  u8 lo[VB_MAXPOST], hi[VB_MAXPOST];
  void build(const vb_floor &fl);
};
//  The residue in hand: how many vectors, how many partitions, which
//  channel each vector is.
struct oc_rgeom {
  u32 vch, np, pv, w, ilv;
  u8 cidx[VB_MAXCH];
};
//  The partition in hand: where in the history its digits go and what the
//  digit model looks at.
struct oc_part {
  sz span;
  i16 *hist, *hist2;
  u32 *avg;
  i16 *pp, *ps;
  u8 *pn;
  i32 *q1, *q2, *zr;
  u32 str;
  const vb_book *bk;
  const tc_ptab *pt;
  u32 rno, pss, j, pc, psz, cls, dim, ilv, i;
  u32 vmod, smod;
  i32 bkq, clrun;
  int clmatch;
};
constexpr sz OC_TAGMAX = 31;
static INLINE void oc_raw_zero(void);
#ifdef TC_MEMCOST
#if TC_MEMCOST + 0 <= 1
#undef TC_MEMCOST
#define TC_MEMCOST 10000
#endif
#endif
#define TC_WIRE(f, F, g, ng, rt, mwt, rg, mwg, lrmt, mbmt) (f).wire(tabs.F->TC_##F##_A, TC_##F##_a_Volume, tabs.F->TC_##F##_B, TC_##F##_b_Volume, tabs.F->TC_##F##_C, TC_##F##_c_Volume, tabs.F->TC_##F##_D, TC_##F##_d_Volume, tabs.F->TC_##F##_MT, TC_##F##_t_Volume, tabs.F->TC_##F##_S, tabs.F->TC_##F##_SC, TC_##F##_s_Volume, tabs.F->TC_##F##_F, tabs.F->TC_##F##_FC, TC_##F##_f_Volume, tabs.F->TC_##F##_W, TC_##F##_m_Volume, (g), (ng), TC_##F##_g_Volume, tabs.F->TC_##F##_WM, TC_##F##_n_Volume, tabs.F->TC_##F##_WC, tabs.F->TC_##F##_WMC, TC_##F##_rA, TC_##F##_rB, TC_##F##_rC, TC_##F##_rD, (rt), (rg), TC_##F##_mwA, TC_##F##_mwB, TC_##F##_mwC, TC_##F##_mwD, (mwt), (mwg), TC_##F##_rS1, TC_##F##_rS2, TC_##F##_lr, TC_##F##_mb, (lrmt), (mbmt), TC_##F##_bw, TC_##F##_qs, TC_##F##_qf, TC_##F##_ND, TC_##F##_MND, TC_##F##_hA, TC_##F##_hB, TC_##F##_hD, TC_##F##_hS, TC_##F##_hF, TC_##F##_hM, TC_##F##_hT, TC_##F##_hN)
#define TC_WIRE_ALL(f, F, g, ng) TC_WIRE(f, F, g, ng, TC_##F##_rT, TC_##F##_mwT, TC_##F##_rG, TC_##F##_mwG, TC_##F##_lrm, TC_##F##_mbm)
//  The model: the six families and everything the coders remember between
//  symbols.  The methods are defined in oc_frame.inc, oc_header.inc,
//  oc_raw.inc, oc_floor.inc and oc_residue.inc, one layer each; the parser
//  (io_t) and the stream walker call them through this interface, and
//  null_sink below answers the same calls with nothing.
//  Header packets seen before -- comment and setup -- kept whole, so that
//  one seen again costs a symbol: which one.  Every encoder writes the
//  same codebooks for the same settings, so the links of a chained file,
//  and with oggdet -S the streams of a container, mostly share one setup
//  header -- a kilobyte, coded again each time it is not shared -- and the
//  comment header of an untagged file is the vendor string, shared the
//  same way.  Model state: cleared with the rest of it, kept across solid
//  segments.  Full, it stops taking; a header it does not hold is coded
//  as before.
constexpr u32 OC_HCMAX = 64;
constexpr sz OC_HCMEM = (sz)4 << 20;
struct oc_hcache {
  u32 n;
  sz used;
  u64 hash[OC_HCMAX];
  sz off[OC_HCMAX], len[OC_HCMAX];
  u8 mem[OC_HCMEM];
  static u64 h64(const u8 *b, sz n) {
    u64 h = 1469598103934665603ull;
    sz i;
    for(i = 0; i < n; i++) {
      h ^= b[i];
      h *= 1099511628211ull;
    }
    return h;
  }
  //  One more than the index of the entry holding these bytes; 0 for none.
  u32 find(const u8 *b, sz n) const {
    u64 h = h64(b, n);
    u32 i;
    for(i = 0; i < this->n; i++)
      if(hash[i] == h && len[i] == n && !memcmp(mem + off[i], b, n))
        return i + 1;
    return 0;
  }
  void add(const u8 *b, sz n) {
    if(this->n >= OC_HCMAX || n > OC_HCMEM - used || find(b, n))
      return;
    memcpy(mem + used, b, n);
    hash[this->n] = h64(b, n);
    off[this->n] = used;
    len[this->n] = n;
    used += n;
    this->n++;
  }
  //  Entry i, which the coded stream says holds n bytes: refused if not.
  const u8 *get(u32 i, sz n) const {
    FATAL_UNLESS(i < this->n && len[i] == n, "coded stream: header %u of %" PRIu64 " bytes was not seen before", i, (u64)n);
    return mem + off[i];
  }
  void clear() {
    n = 0;
    used = 0;
  }
};
static oc_hcache oc_hc;
template <int f_DEC> struct oc_model {
  static constexpr int ENC = (f_DEC == 0);
  static constexpr int DRY = 0;
  static Rangecoder<f_DEC, OC_CARRYLESS> rc;
  tc_fam<f_DEC> fam_dig, fam_sgn, fam_flr, fam_cls, fam_aux, fam_hdr;
  i64 tc_last[F_N], tc_last2[F_N];
  vb_ctx *vb;
  //  frame: the last mode and window flags, the last page's fields
  u32 tc_prevW, tc_prevmode, oc_nextW, oc_wp;
  i64 pg_prev;
  u32 pg_prevtype, pg_prevser, oc_seq;
  //  header: the tag table, and what each tag was last
  char tc_tagbuf[TC_MAXTAG * (OC_TAGMAX + 1)];
  const char *tc_tag[TC_MAXTAG];
  u32 tc_ntag;
  int tc_lasttag = -1;
  i64 tc_tlast[TC_MAXTAG], tc_tlast2[TC_MAXTAG];
  i64 tc_runpos;
  //  floor
  oc_floor oc_fl[VB_MAXFLOOR];
  u8 tc_used[VB_MAXCH];
  u8 oc_fno[VB_MAXCH];
  i16 fl_cur[VB_MAXPOST];
  i16 fl_fy[VB_MAXPOST];
  //  residue
  i32 cl_run[VB_MAXRES];
  oc_rgeom R;
  oc_part P;
  void init();
  void wire_all();
  void reset();
  void setup_done(vb_ctx &vv);
  //  aux fields, through fam_aux with the field's own history as context
  i64 auxc(int fld, i32 c, i64 x, int sgn);
  i64 aux(int fld, i64 x);
  //  oc_frame.inc
  u32 mode(u32 md, u32 nmd);
  u32 wprev(u32 w);
  u32 wnext(u32 w);
  void page(ogg_page &p, int cont, int bos);
  u32 link(u32 more);
  void link_begin();
  u32 rawlen(u32 n);
  u32 more(u32 m, int eos);
  u32 pgraw(u32 r, int lastraw);
  u32 crcbad(u32 c);
  u32 check(u32 c);
  u32 padnz(u32 z);
  u32 spill(u32 n);
  //  oc_header.inc
  int tagid(const char *tag);
  i64 hdrval(int id, i64 x);
  u32 hdr(const char *tag, u32 x, u64 hi);
  //  oc_raw.inc
  void raw_init();
  u32 raw(u32 b);
  //  oc_floor.inc
  void floor(u32 k, u32 fno, const vb_floor *f, u32 *y, int &used);
  //  oc_residue.inc
  void residue_begin(u32 rno, const vb_res *r, const u8 *nz, u32 nch, u32 n);
  u32 classify(u32 rno, u32 j, u32 slot, u32 cin);
  u32 cls(u32 rno, u32 jv, u32 pc, u32 k, u32 cin);
  void part_begin(u32 rno, u32 pss, u32 jv, u32 pc, const vb_res *r, const vb_book *bk, u32 bn, u32 cls);
  void vec_begin() { fam_dig.pc.start(P.pt); }
  i64 digit(i64 dg);
};
template <int f_DEC> Rangecoder<f_DEC, OC_CARRYLESS> oc_model<f_DEC>::rc;
template <int f_DEC> void oc_model<f_DEC>::init() {
  cm_tables();
  tabs.init();
  wire_all();
}
template <int f_DEC> void oc_model<f_DEC>::wire_all() {
  TC_WIRE(fam_dig, dig, (cm_cnt *)nullptr, 0, TC_dig_rT, TC_dig_mwT, 0, 0, TC_dig_lrm, TC_dig_mbm);
  TC_WIRE(fam_sgn, sgn, (cm_cnt *)nullptr, 0, 0, 0, 0, 0, 0, 0);
  TC_WIRE_ALL(fam_flr, flr, tabs.flr->TC_flr_G, TC_SGN_FLR);
  TC_WIRE(fam_cls, cls, (cm_cnt *)nullptr, 0, TC_cls_rT, TC_cls_mwT, 0, 0, TC_cls_lrm, TC_cls_mbm);
  TC_WIRE_ALL(fam_aux, aux, tabs.aux->TC_aux_G, TC_SGN_AUX);
  TC_WIRE_ALL(fam_hdr, hdr, tabs.hdr->TC_hdr_G, TC_SGN_HDR);
  raw_init();
}
//  The model as a fresh process has it, after init(): every table zero,
//  every pool empty, every remembered value forgotten, the families wired
//  again.  For a frontend that codes one stream after another and wants
//  each .oc to be what `oggcomp c` would have written for it.
template <int f_DEC> void oc_model<f_DEC>::reset() {
  tabs.reset();
  hist.clear();
  tcp.reset();
  vb_ar.reset();
  oc_raw_zero();
  oc_hc.clear();
  *this = oc_model();
  wire_all();
}
template <int f_DEC> i64 oc_model<f_DEC>::auxc(int fld, i32 c, i64 x, int sgn) {
  tcx v;
  i64 r;
  tc_make_aux(fld, c, tc_qlog(tc_last[fld]), v);
  r = sgn ? fam_aux.codes(v, x, (u32)fld) : fam_aux.code(v, x);
  tc_last2[fld] = tc_last[fld];
  tc_last[fld] = r;
  return r;
}
template <int f_DEC> i64 oc_model<f_DEC>::aux(int fld, i64 x) { return auxc(fld, tc_qlog(tc_last2[fld]), x, 0); }
template <int f_DEC> void oc_model<f_DEC>::setup_done(vb_ctx &vv) {
  const vb_setup *s;
  vb = &vv;
  s = vb->cur;
  u32 i;
  hist.reset();
  tcp.build(*vb->cur);
  hist.tc_nchan = vb->i.ch;
  for(i = 0; i < s->nrs; i++) {
    const vb_res *r = s->rs + i;
    sz span = r->end > r->beg ? r->end - r->beg : 0;
    sz need;
    if(span > TC_SPANMAX)
      span = 0;
    need = (sz)2 * 8 * hist.tc_nchan * span;
    if(need > TC_HISTMAX)
      span = 0;
    hist.dg_span[i] = span;
    if(span) {
      hist.dg_hist[i] = (i16 *)hist.take((sz)2 * 8 * hist.tc_nchan * span * sizeof(i16));
      hist.dg_hist2[i] = (i16 *)hist.take((sz)2 * 8 * hist.tc_nchan * span * sizeof(i16));
      hist.dg_avg[i] = (u32 *)hist.take((sz)2 * 8 * hist.tc_nchan * span * sizeof(u32));
      hist.dg_pp[i] = (i16 *)hist.take((sz)hist.tc_nchan * span * sizeof(i16));
      hist.dg_ps[i] = (i16 *)hist.take((sz)hist.tc_nchan * span * sizeof(i16));
      hist.dg_pn[i] = (u8 *)hist.take((sz)hist.tc_nchan * span);
    }
    hist.cl_np[i] = span ? span / r->psz + 2 : 0;
    if(hist.cl_np[i])
      hist.cl_hist[i] = (u8 *)hist.take((sz)2 * hist.tc_nchan * hist.cl_np[i]);
    memset(hist.cl_last[i], 0, sizeof hist.cl_last[i]);
    memset(hist.cl_last2[i], 0, sizeof hist.cl_last2[i]);
  }
  hist.dg_q1 = (i32 *)hist.take((sz)s->nrs * 8 * hist.tc_nchan * sizeof(i32));
  hist.dg_q2 = (i32 *)hist.take((sz)s->nrs * 8 * hist.tc_nchan * sizeof(i32));
  hist.dg_zr = (i32 *)hist.take((sz)s->nrs * 8 * hist.tc_nchan * sizeof(i32));
  hist.fl_hist = (i16 *)hist.take((sz)2 * hist.tc_nchan * VB_MAXPOST * sizeof(i16));
  hist.fl_yhist = (i16 *)hist.take((sz)2 * hist.tc_nchan * VB_MAXPOST * sizeof(i16));
  hist.fl_who = (i32 *)hist.take((sz)2 * hist.tc_nchan * sizeof(i32));
  for(i = 0; i < 2 * hist.tc_nchan; i++)
    hist.fl_who[i] = -1;
  for(i = 0; i < s->nfl; i++)
    oc_fl[i].build(s->fl[i]);
}
struct null_sink {
  static constexpr int ENC = 1;
  static constexpr int DRY = 1;
  u32 spill(u32 n) { return n; }
  u32 hdr(const char *, u32 x, u64) { return x; }
  void setup_done(vb_ctx &) {}
  u32 mode(u32 md, u32) { return md; }
  u32 wprev(u32 w) { return w; }
  u32 wnext(u32 w) { return w; }
  void floor(u32, u32, vb_floor *, u32 *, int &) {}
  void residue_begin(u32, vb_res *, const u8 *, u32, u32) {}
  u32 cls(u32, u32, u32, u32, u32 c) { return c; }
  void part_begin(u32, u32, u32, u32, vb_res *, vb_book *, u32, u32) {}
  void vec_begin() {}
  i64 digit(i64 d) { return d; }
  u32 padnz(u32 z) { return z; }
  u32 raw(u32 b) { return b; }
};
