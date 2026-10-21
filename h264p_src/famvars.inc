// famvars.inc -- what the model gives each family: its decision nodes and
// sign contexts, and the variables its IDX/l<L>/<family>.idx reads (the
// model fills them; the levels read parts of them) with what each needs of
// the model's parts. Apart from model.inc for IDX/famtune.cpp, which
// replays a family's decisions without the model.

namespace h264p {

// the families, X(name) each, in the order the model has them
#define H264P_FAMS(X) X(nal) X(hdr) X(skp) X(mbt) X(cbp) X(ipm) X(mvs) X(qpd) X(cbf) X(sig) X(lvl) X(sgn) X(mvg) X(qpg) X(pcm)

// decisions per context (ND, mantissa nodes MND), sign contexts (NG)
constexpr int H_nal_ND = TC_NODE, H_nal_MND = TC_MNODE, H_nal_NG = 32;
constexpr int H_hdr_ND = TC_NODE, H_hdr_MND = TC_MNODE, H_hdr_NG = 256;
constexpr int H_skp_ND = 3, H_skp_MND = 1, H_skp_NG = 1;
constexpr int H_mbt_ND = 29, H_mbt_MND = 1, H_mbt_NG = 1;
constexpr int H_cbp_ND = 7, H_cbp_MND = 1, H_cbp_NG = 1;
constexpr int H_ipm_ND = 19, H_ipm_MND = 1, H_ipm_NG = 1;
constexpr int H_mvs_ND = 16, H_mvs_MND = TC_MNODE, H_mvs_NG = 1;  // sub()'s tree of B sub_mb_type: nodes 1-15
constexpr int H_qpd_ND = TC_NODE, H_qpd_MND = TC_MNODE, H_qpd_NG = 1;
constexpr int H_cbf_ND = TC_NODE, H_cbf_MND = TC_MNODE, H_cbf_NG = 1;
constexpr int H_sig_ND = 1, H_sig_MND = 1, H_sig_NG = 1;
constexpr int H_lvl_ND = TC_NODE, H_lvl_MND = TC_MNODE, H_lvl_NG = 1;
constexpr int H_sgn_ND = 1, H_sgn_MND = 1, H_sgn_NG = 1;
constexpr int H_mvg_ND = 1, H_mvg_MND = 1, H_mvg_NG = 1;
constexpr int H_qpg_ND = 1, H_qpg_MND = 1, H_qpg_NG = 1;
constexpr int H_pcm_ND = TC_NODE, H_pcm_MND = TC_MNODE, H_pcm_NG = 3;

// What a variable needs of the model's parts (h264p_levels.md section 7.3),
// set where it is "not known" without the part (found by coding the corpus
// with each part switched off: a variable that varies with the part and not
// without it): the decoder's pictures, only their motion and reference lists
// (which the decoder has too), the template searches, the intra proxy, sign
// prediction, the skip prediction's pixels, the motion vectors as the
// decoder has them. A level must not read what its parts cannot make
// (Model's static_asserts): such a line goes from its .idx.
enum : unsigned {
  NEED_REC = 1,
  NEED_REFS = 2,
  NEED_SEARCH = 4,
  NEED_PROXY = 8,
  NEED_SIGNP = 16,
  NEED_SKIPPX = 32,
  NEED_MVP = 64,
};

// Each family's variables, X(name, needs) each, as H_<family>_v has them:
// what the model fills and the family's IDX/l<L>/<family>.idx reads
#define H_nal_VARS(X)                   \
  X(fld, 0) X(ctx, 0) X(p1, 0) X(p2, 0)
#define H_hdr_VARS(X)                                                          \
  X(tag, 0) X(fld, 0) X(p1, 0) X(p2, 0) X(pos, 0) X(p1r, 0) X(sty, 0) X(pr, 0)
#define H_skp_VARS(X)                                                                                   \
  X(st, 0) X(a3, 0) X(b3, 0) X(col, 0) X(c9, 0) X(cd, 0) X(run, 0) X(cnt, 0) X(eq, 0) X(lf, 0) X(af, 0) \
  X(td, NEED_REC) X(en, NEED_REC) X(sm, NEED_REC | NEED_SKIPPX) X(tm, NEED_REC | NEED_SKIPPX)           \
  X(tb, NEED_REC | NEED_SEARCH | NEED_SKIPPX) X(bd, NEED_REC | NEED_SKIPPX) X(qc, 0) X(c1, NEED_REFS)   \
  X(k9, NEED_REFS) X(tx, NEED_REC | NEED_SKIPPX)
#define H_mbt_VARS(X)                                                                                               \
  X(st, 0) X(ca, 0) X(cb, 0) X(cc, 0) X(nsk, 0) X(en, NEED_REC) X(td, NEED_REC) X(sm, NEED_REC | NEED_SKIPPX)       \
  X(tm, NEED_REC | NEED_SKIPPX) X(tb, NEED_REC | NEED_SEARCH | NEED_SKIPPX) X(dh, NEED_REC | NEED_SKIPPX)           \
  X(bd, NEED_REC | NEED_SKIPPX) X(qc, 0) X(im, NEED_REC | NEED_PROXY) X(ig, NEED_REC | NEED_PROXY) X(c1, NEED_REFS) \
  X(tx, NEED_REC | NEED_SKIPPX)
#define H_cbp_VARS(X)                                                                                      \
  X(inc, 0) X(pc, 0) X(cls, 0) X(col, 0) X(q6, 0) X(st, 0) X(en, NEED_REC) X(td, NEED_REC) X(pg, NEED_REC) \
  X(tp, NEED_REC) X(em, NEED_REC) X(pq, NEED_REC | NEED_PROXY)
#define H_ipm_VARS(X)                                                                                              \
  X(k8, 0) X(ma, 0) X(mb, 0) X(mp, 0) X(blk, 0) X(pv, 0) X(col, 0) X(dm, NEED_REC) X(ds, NEED_REC)                 \
  X(pm, NEED_REC | NEED_PROXY) X(pr, NEED_REC | NEED_PROXY) X(dr, NEED_REC) X(pq, NEED_REC | NEED_SEARCH) X(mh, 0) \
  X(mc, 0) X(mg, NEED_REC | NEED_PROXY) X(rg, NEED_REC | NEED_PROXY) X(p2, NEED_REC | NEED_PROXY)
#define H_mvs_VARS(X)                                                                                             \
  X(kind, 0) X(comp, 0) X(lst, 0) X(ma, 0) X(mb, 0) X(oth, 0) X(ps, 0) X(rf, 0) X(pi, 0) X(nr, 0) X(pd, NEED_MVP) \
  X(sp, NEED_MVP) X(tv, NEED_REC | NEED_SEARCH) X(zd, NEED_MVP) X(cq, 0) X(tq, NEED_REC | NEED_SEARCH)            \
  X(rt, NEED_REC) X(ea, NEED_REC | NEED_SEARCH) X(eb, NEED_REC | NEED_SEARCH) X(ta, NEED_REC | NEED_SEARCH)
#define H_qpd_VARS(X)                                                                                       \
  X(lnz, 0) X(pc, 0) X(cc, 0) X(cls, 0) X(dc, 0) X(db, 0) X(lz, 0) X(st, 0) X(ae, NEED_REC) X(aq, NEED_REC) \
  X(ac, NEED_REC) X(td, NEED_REC) X(sq, 0) X(xe, NEED_REC) X(lq, NEED_REC) X(bc, 0) X(cl, 0) X(kc, 0)
#define H_cbf_VARS(X)                                                                                        \
  X(cat, 0) X(na, 0) X(nb, 0) X(intra, 0) X(q6, 0) X(np, 0) X(pos, 0) X(crcb, 0) X(col, 0) X(nc, 0) X(kn, 0) \
  X(md, 0) X(en, NEED_REC) X(pg, NEED_REC) X(em, NEED_REC) X(pz, NEED_REC | NEED_PROXY) X(tn, NEED_REC)      \
  X(pq, NEED_REC | NEED_PROXY) X(ch, 0) X(ds, 0) X(hv, NEED_REC) X(ly, 0) X(dq, 0)
#define H_sig_VARS(X)                                                                                             \
  X(cg, 0) X(pos, 0) X(rem, 0) X(zl, 0) X(md, 0) X(nc, 0) X(fld, 0) X(q6, 0) X(pg, NEED_REC) X(em, NEED_REC)      \
  X(pe, NEED_REC | NEED_SIGNP) X(ps, NEED_REC | NEED_PROXY) X(tz, NEED_REC) X(pq, NEED_REC | NEED_PROXY) X(hb, 0) \
  X(ft, 0) X(h3, 0) X(pr, NEED_REC | NEED_PROXY) X(sn, 0) X(hv, NEED_REC) X(pf, NEED_REC)
#define H_lvl_VARS(X)                                                                                       \
  X(cg, 0) X(g1, 0) X(e1, 0) X(nc, 0) X(pos, 0) X(sa, 0) X(sb, 0) X(q6, 0) X(tot, 0) X(md, 0) X(pm, 0)      \
  X(pg, NEED_REC) X(em, NEED_REC) X(pe, NEED_REC | NEED_SIGNP) X(pl, NEED_REC | NEED_PROXY) X(tz, NEED_REC) \
  X(pq, NEED_REC | NEED_PROXY) X(lh, 0) X(lt, 0) X(ls, NEED_REC | NEED_PROXY) X(hv, NEED_REC) X(fq, 0)
#define H_sgn_VARS(X)                                                                                             \
  X(cg, 0) X(pos, 0) X(sa, 0) X(sb, 0) X(scb, 0) X(mag, 0) X(md, 0) X(pp, NEED_REC | NEED_SIGNP)                  \
  X(mg, NEED_REC | NEED_SIGNP) X(ez, NEED_REC | NEED_PROXY) X(ts, NEED_REC) X(pq, NEED_REC | NEED_PROXY) X(gh, 0) \
  X(se, NEED_REC | NEED_PROXY) X(dm, NEED_REC)
#define H_mvg_VARS(X)                                                                                            \
  X(comp, 0) X(lst, 0) X(ts, NEED_REC | NEED_SEARCH) X(zs, NEED_MVP) X(ds, NEED_MVP) X(sa, 0) X(sb, 0) X(mag, 0) \
  X(tq, NEED_REC | NEED_SEARCH) X(ox, 0) X(ns, NEED_MVP) X(es, NEED_REC | NEED_SEARCH)                           \
  X(as, NEED_REC | NEED_SEARCH)
#define H_qpg_VARS(X)                                                                                              \
  X(as, NEED_REC) X(cs, 0) X(bs, 0) X(ls, 0) X(rs, NEED_REC) X(mag, 0) X(cls, 0) X(st, 0) X(ss, 0) X(xe, NEED_REC) \
  X(lq, NEED_REC) X(bk, 0)
#define H_pcm_VARS(X)                  \
  X(pl, 0) X(dp, 0) X(pd, 0) X(pd2, 0)

// the variables as the model gives them to a family
#define H_FIELD(f, needs) int f;
#define H_STRUCT(F) struct H_##F##_v { H_##F##_VARS(H_FIELD) };
H264P_FAMS(H_STRUCT)
#undef H_STRUCT
#undef H_FIELD

// a family's variables that need the part n (NEED_*), a bit per field (as
// famcfg.inc's Uses has them)
#define H_NEED(f, needs) | ((needs) & n ? 1ull << offsetof(V, f) / sizeof(int) : 0)
#define H_NEEDS(F)                 \
  template <unsigned n>            \
  constexpr u64 H_##F##_needs() {  \
    typedef H_##F##_v V;           \
    return 0 H_##F##_VARS(H_NEED); \
  }
H264P_FAMS(H_NEEDS)
#undef H_NEEDS
#undef H_NEED

// a family's variables as the locals of its index builder (IDX/l<L>/family.inc)
#define H_LOCAL(f, needs) \
  int f = D.f;            \
  (void)f;

}  // namespace h264p
