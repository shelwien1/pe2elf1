// famvars.inc -- what the model gives each family: its decision nodes and
// sign contexts, and the variables its IDX/l<L>/<family>.idx reads (the
// model fills them; the levels read parts of them). Apart from model.inc
// for IDX/famtune.cpp, which replays a family's decisions without the
// model.

namespace h264p {

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

struct H_nal_v { int fld, ctx, p1, p2; };
struct H_hdr_v { int tag, fld, p1, p2, pos, p1r, sty, pr; };
struct H_skp_v { int st, a3, b3, col, c9, cd, run, cnt, eq, lf, af, td, en, sm, tm, tb, bd, qc, c1, k9, tx; };
struct H_mbt_v { int st, ca, cb, cc, nsk, en, td, sm, tm, tb, dh, bd, qc, im, ig, c1, tx; };
struct H_cbp_v { int inc, pc, cls, col, q6, st, en, td, pg, tp, em, pq; };
struct H_ipm_v { int k8, ma, mb, mp, blk, pv, col, dm, ds, pm, pr, dr, pq, mh, mc, mg, rg, p2; };
struct H_mvs_v { int kind, comp, lst, ma, mb, oth, ps, rf, pi, nr, pd, sp, tv, zd, cq, tq, rt, ea, eb, ta; };
struct H_qpd_v { int lnz, pc, cc, cls, dc, db, lz, st, ae, aq, ac, td, sq, xe, lq, bc, cl, kc; };
struct H_cbf_v { int cat, na, nb, intra, q6, np, pos, crcb, col, nc, kn, md, en, pg, em, pz, tn, pq, ch, ds, hv, ly, dq; };
struct H_sig_v { int cg, pos, rem, zl, md, nc, fld, q6, pg, em, pe, ps, tz, pq, hb, ft, h3, pr, sn, hv, pf; };
struct H_lvl_v { int cg, g1, e1, nc, pos, sa, sb, q6, tot, md, pm, pg, em, pe, pl, tz, pq, lh, lt, ls, hv, fq; };
struct H_sgn_v { int cg, pos, sa, sb, scb, mag, md, pp, mg, ez, ts, pq, gh, se, dm; };
struct H_mvg_v { int comp, lst, ts, zs, ds, sa, sb, mag, tq, ox, ns, es, as; };
struct H_qpg_v { int as, cs, bs, ls, rs, mag, cls, st, ss, xe, lq, bk; };
struct H_pcm_v { int pl, dp, pd, pd2; };

}  // namespace h264p
