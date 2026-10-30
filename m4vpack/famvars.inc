// famvars.inc -- what the model gives each family: its decision nodes and sign contexts, and the
// variables its IDX/l<L>/<family>.idx reads (the model fills them; the levels read parts of them)
// with what each needs of the model's parts. Apart from model.inc for IDX/famtune.cpp, which
// replays a family's decisions without the model.

// the families, X(name) each, in the order the model has them
#define M4V_FAMS(X) X(hdr) X(raw) X(skp) X(mbt) X(cbp) X(qpd) X(mvd) X(mvg) X(dcs) X(sig) X(lvl) X(sgn) X(esc)

// decisions per context (ND, mantissa nodes MND), sign contexts (NG); the nodes of each:
//   hdr  code() of a header field's value (nodes 0-12), bit() of a flag (0)
//   raw  a byte's bits, by their place in its binary tree (1-255)
//   skp  0 a P-VOP macroblock not coded, 1 a B-VOP's skipped with its co-located one, 2 a B-VOP's
//        direct with no vector or coefficients (modb 0)
//   mbt  0 intra?, 1 4MV?, 2 GMC (mcsel), 3 B: a coded block pattern (modb 2)?, 4 direct?,
//        5 forward?, 6 backward (else both)?, 7 AC prediction, 8 field prediction, 9 a field
//        select, 10 field DCT
//   cbp  the 8x8 block (0-5)
//   qpd  0 a P-VOP's dquant nonzero?, 1-3 its index (a tree), 4 a B-VOP's nonzero?, 5 its sign
//   mvd  code() of a vector difference in units of the VLC code (0-12), 13-18 the f_code residual
//        bits, from the top
//   mvg  the sign of a vector difference
//   dcs  code() of the magnitude of an intra DC difference (0-12), 13 its sign
//   sig  0 significance, 1 last
//   lvl  code() of a coefficient's magnitude less one (0-12)
//   sgn  a coefficient's sign
//   esc  0 the escape mode is the usual one, 1-3 which (a tree)
// The header's fields, each of its own values in the hdr family (its fld, and its sign context)
enum {
  F_KIND, F_RAWLEN, F_VTYPE, F_MTB, F_TIB, F_TSAME, F_TINC, F_CODED, F_RND, F_CPLX, F_DCTHR, F_TFF, F_ALT, F_WARP, F_QUANT,
  F_FCODE, F_BCODE, F_VPST, F_VPMBSAME, F_VPMB, F_VPQ, F_HEC, F_HMTB, F_HTINC, F_HTYPE, F_HDCTHR, F_HWARP, F_HFCODE,
  F_HBCODE, F_TAILC, F_TAILB, F_TAILN, F_TAILBYTE, F_MBST, F_VPFLAG, F_N
};
// the nodes of the families that have a few of their own
enum { SKP_P, SKP_B, SKP_BDIR, SKP_N };
enum { MBN_INTRA, MBN_4MV, MBN_GMC, MBN_BCBP, MBN_BDIRECT, MBN_BFWD, MBN_BBWD, MBN_AC, MBN_FIELD, MBN_FSEL, MBN_DCT, MBN_N };
enum { QPD_P, QPD_IDX, QPD_IDX_LO, QPD_B = QPD_IDX_LO + 2, QPD_BSIGN, QPD_N };
enum { SIG_SIG, SIG_LAST, SIG_N };
enum { ESCN_USUAL, ESCN_HI, ESCN_LO, ESCN_N = ESCN_LO + 2 };
constexpr int M_hdr_ND = TC_NODE, M_hdr_MND = TC_MNODE, M_hdr_NG = F_N;
constexpr int M_raw_ND = BYTE_VALUES, M_raw_MND = 1, M_raw_NG = 1;
constexpr int M_skp_ND = SKP_N, M_skp_MND = 1, M_skp_NG = 1;
constexpr int M_mbt_ND = MBN_N, M_mbt_MND = 1, M_mbt_NG = 1;
constexpr int M_cbp_ND = MB_BLOCKS, M_cbp_MND = 1, M_cbp_NG = 1;
constexpr int M_qpd_ND = QPD_N, M_qpd_MND = 1, M_qpd_NG = 1;
constexpr int M_mvd_ND = TC_NODE + FCODE_MAX - FCODE_MIN, M_mvd_MND = TC_MNODE, M_mvd_NG = 1;
constexpr int M_mvg_ND = 1, M_mvg_MND = 1, M_mvg_NG = 1;
constexpr int M_dcs_ND = TC_NODE + 1, M_dcs_MND = TC_MNODE, M_dcs_NG = 1;
constexpr int M_sig_ND = SIG_N, M_sig_MND = 1, M_sig_NG = 1;
constexpr int M_lvl_ND = TC_NODE, M_lvl_MND = TC_MNODE, M_lvl_NG = 1;
constexpr int M_sgn_ND = 1, M_sgn_MND = 1, M_sgn_NG = 1;
constexpr int M_esc_ND = ESCN_N, M_esc_MND = 1, M_esc_NG = 1;


// What a variable needs of the model's parts: the decoder in the loop (the pictures as decoded,
// level 9's recon.inc). A level must not read what its parts cannot make (Model's
// static_asserts): such a line goes from its .idx.
enum : unsigned {
  NEED_REC = 1,
};

// Each family's variables, X(name, needs) each, as M_<family>_v has them: what the model fills
// and the family's IDX/l<L>/<family>.idx reads. (Names of one letter are the index builders'.)
// They are as they are, or capped (model.inc: kCap, kNone ...) where few values lie above, or on
// tc_qlog's half octaves where the range is wide ("qlog": 0 for none where there is one, else
// 1 + tc_qlog); where to split them is each .idx line's thresholds.
//
// hdr: fld the field (F_*, above), ty the VOP's type (0 I, 1 P, 2 B, 3 S, 4 none), p1
//      the field's last value (to kHdrCap), pos a flag's place in its field
// raw: c1, c2 the bytes before, code the chunk's start code (ChunkClass, model.inc: 0 not known
//      yet), at the byte's place in the chunk (tc_qlog)
// skp: st the VOP (0 P, 1 S with GMC, 2 B), cl the left and above macroblocks of the kind the
//      decision asks of (skipped, direct), c4 those of the four before (left, above, above left
//      and right), rt the co-located one of that kind (2 none), rc its class (C_*, model.inc), run the
//      run of that kind before (to kCap), pz whether the predicted vector is nonzero, rz the
//      co-located vector (2 none), bsk the co-located macroblock skipped in B-VOPs' terms (2 none),
//      rr the co-located one coded, intra (1 coded, 2 intra, 0 not coded or none); with the decoder:
//      zt how far the decoded samples above and left of the macroblock are from the reference's
//      there (P-VOPs: the area changed since), ze how far the edges of the prediction the decision
//      would leave it (P the reference's unmoved, B direct without a vector) are from them (both
//      their mean difference in eighths of the quantiser's step, qlog)
// mbt: st the VOP (0 I, 1 P, 2 B, 3 S), cl, c4 as skp's (the kind the decision asks of), rt the
//      co-located macroblock (0 another kind, 1 that kind, 2 none, 3 the kind next to it: skipped
//      for intra, inter for 4MV), rc its class, cd the left and above B-VOP direct macroblocks, fsn
//      which field select (0-3), intra the macroblock's, zt, ze as skp's
// cbp: c3 the macroblock's kind (0 intra, 1 P, 2 B), lu the left and above blocks' bits (3x3, 2
//      none), mvc the vector differences' size (VLC units summed, to kCap; kNone none), rbit the
//      co-located block's (2 none), cnt the blocks coded so far, frac the vector's sub-pel
//      components (4 none), qs the quantiser (before the macroblock's dquant), pat the bits so far
//      (the last 5), lq whether the left and above macroblocks agree on having coded blocks (0),
//      else 1 + whether the co-located one has some; with the decoder, for an intra macroblock: ie
//      the intra estimate's AC coefficients of half a level or more in the block (0 no estimate,
//      else 1 + them, to kCap), tq how far to trust it (0 none; before the macroblock's first
//      block 1 no template, else 2 + its template's mean miss in quarter samples on tc_qlog's scale;
//      then 24 + how far the blocks so far were from it, levels per block, the same scale); for an
//      inter one: tx the texture of the block's prediction (its mean difference between
//      neighbouring samples, in eighths of the quantiser's step, qlog), em how far its edges are
//      from the decoded samples next to them (the same, 0 none; only edges on other macroblocks)
// qpd: c3 as cbp's, pdq the last dquant nonzero, qs the quantiser
// mvd: kind (0 P, 1 P field, 2 B forward, 3 B backward, 4 B direct, 5 P 4MV), comp (0 x, 1 y),
//      ns the left and above magnitudes summed (in VLC units, to kCap), oth the other component's
//      (kNone none), rs the co-located one's (kNone none), dz the difference that repeats the
//      co-located vector (kNone none), sp the spread of the prediction's candidates (kNone none),
//      lq, aq the left and above magnitudes (kNone none), ds that difference's sign (0 none or
//      zero, 1 +, 2 -), fc f_code less one, rbi the residual bit's place (for the nodes 13-18),
//      rsame whether the residual so far is that difference's (0 no, 1-2 yes and its bit there);
//      with the decoder: tz the difference that gives the vector the template matches best (kNone
//      none), ts its sign
// mvg: kind, comp, ls, as the left and above differences' signs (0 zero or none, 1 +, 2 -), ds,
//      dz as mvd's, mag the magnitude (to kCap), hm whether it is the co-located repeat's (0 none,
//      1 below, 2 equal, 3 above), ts as mvd's, thm whether it is the template's (as hm)
// dcs: lc (0 luma, 1 chroma), prev the last DC difference's size (0-12), ls, as the left and above
//      blocks' (kDcSizeNone none), coded the block's AC coefficients, qs the quantiser, lbn, abn the
//      left and above blocks' coefficient counts (to kCap, kNone none), ls2, as2 their DC
//      differences' signs (0 none or zero, 1 +, 2 -), psg the last DC difference's sign, sz the
//      magnitude's size (for the sign); with the decoder: ie the intra estimate's difference (0 none,
//      else 1 + its quarter levels, to kDcEstCap), es its sign (0 none or under half a level, 1 +,
//      2 -), tq as cbp's
// sig: cls the block class (inter, B, intra, intra with AC prediction; x luma/chroma), pos the
//      scan position, nc the neighbours' counts (CAVLC's nC: 0 none, else 1 + their mean, to
//      kCap), qs the quantiser, kb the coefficients so far (to kCap), nz the left and above
//      blocks' at the position (3x3, 2 none), past the position past their last (3x3, 2 none), run
//      the zeros before (to kCap), fp the significance of the frequency neighbours left, above,
//      above left, 2 left, 2 above (bits), mvc the vector's size (as cbp's, kMvcNone none;
//      kMvcIntra + the DC difference's size for an intra block), rnz the co-located block's count
//      (to kCap, kNone none), xm for Cr, Cb's coefficient there (to kTinyCap, kTinyCap + 1 none),
//      h8 the last 8 flags, h16 the last 16, sl, sa the left and above blocks' coefficients past
//      the position (to kCap, kNone none); with the decoder, for an intra block: ie the intra
//      estimate's level there, the prediction taken off (0 none, else 1 + its quarter levels, to
//      kEstCap), tq as cbp's; for an inter block, tx, em as cbp's (every edge decoded), pe the
//      level its edges ask for there (the prediction's miss projected onto the coefficient's
//      pattern: as ie), pf the prediction's own transform there against the quantiser (qlog), td
//      which way its texture runs (0 its samples change more down the columns, 1 neither, 2 along
//      the rows, 3 none)
// lvl: cls, pos, qs as sig's, gs the levels so far (0 one above 1, else 1 + those of 1, to
//      kTinyCap), nmb the left and above blocks' magnitudes there (summed, to kTinyCap), kc the
//      block's count (to kCountCap), pm the last magnitude coded (to kTinyCap), nb2 the neighbours
//      right, below, below right (their sum, to kTinyCap), den how many of the four around are
//      significant, mvc as sig's, lh the last 3 magnitudes (2 bits each), rb the right and below
//      ones (3 at most, 4x4), ssb their sum (qlog), ngt the levels above 1 so far (to kTinyCap); ie,
//      tq, tx, em, pe, pf as sig's (and the estimate's level the hint)
// sgn: cls, pos as sig's, first the block's first, lsa the left and above blocks' signs there
//      (3x3, 2 none), gh the last 3 signs coded (base 3, 0 none), ps the last, flu the frequency
//      neighbours' left and above (0 none, 1 +, 2 -; 3x3), flu2 those 2 away, vote their signs by
//      their magnitudes (kVoteMax + the vote), fdlu with the one above left (3x3x3), xs for Cr,
//      Cb's sign there (0 none, 1 +, 2 -), lam the left and above blocks' sign and magnitude (5x5);
//      with the decoder, for the first signs of a block (coded after the others): pp the sign the
//      edges predict (0 none, 1 +, 2 -), mg by how much (quarter samples, qlog), pp2, mg2 the same
//      by the samples next to them alone, rk which of them (0-7, 8 none); es the intra estimate's
//      sign there (0 none or under half a level, 1 +, 2 -), ie as sig's, pj, pe the sign and level
//      an inter block's edges ask for there (as es, and sig's pe)
// esc: dm the usual mode
#define M_hdr_VARS(X) X(fld, 0) X(ty, 0) X(p1, 0) X(pos, 0)
#define M_raw_VARS(X) X(c1, 0) X(c2, 0) X(code, 0) X(at, 0)
#define M_skp_VARS(X)                                                                                      \
  X(st, 0) X(cl, 0) X(c4, 0) X(rt, 0) X(rc, 0) X(run, 0) X(pz, 0) X(rz, 0) X(bsk, 0) X(rr, 0)             \
  X(zt, NEED_REC) X(ze, NEED_REC)
#define M_mbt_VARS(X)                                                                                      \
  X(st, 0) X(cl, 0) X(c4, 0) X(rt, 0) X(rc, 0) X(cd, 0) X(fsn, 0) X(intra, 0) X(zt, NEED_REC) X(ze, NEED_REC)
#define M_cbp_VARS(X)                                                                                      \
  X(c3, 0) X(lu, 0) X(mvc, 0) X(rbit, 0) X(cnt, 0) X(frac, 0) X(qs, 0) X(pat, 0) X(lq, 0) X(ie, NEED_REC)  \
  X(tq, NEED_REC) X(tx, NEED_REC) X(em, NEED_REC)
#define M_qpd_VARS(X) X(c3, 0) X(pdq, 0) X(qs, 0)
#define M_mvd_VARS(X)                                                                                      \
  X(kind, 0) X(comp, 0) X(ns, 0) X(oth, 0) X(rs, 0) X(dz, 0) X(sp, 0) X(lq, 0) X(aq, 0) X(ds, 0) X(fc, 0) \
  X(rbi, 0) X(rsame, 0) X(tz, NEED_REC) X(ts, NEED_REC)
#define M_mvg_VARS(X)                                                                                      \
  X(kind, 0) X(comp, 0) X(ls, 0) X(as, 0) X(ds, 0) X(dz, 0) X(mag, 0) X(hm, 0) X(ts, NEED_REC) X(thm, NEED_REC)
#define M_dcs_VARS(X)                                                                                      \
  X(lc, 0) X(prev, 0) X(ls, 0) X(as, 0) X(coded, 0) X(qs, 0) X(lbn, 0) X(abn, 0) X(ls2, 0) X(as2, 0)     \
  X(psg, 0) X(sz, 0) X(ie, NEED_REC) X(es, NEED_REC) X(tq, NEED_REC)
#define M_sig_VARS(X)                                                                                      \
  X(cls, 0) X(pos, 0) X(nc, 0) X(qs, 0) X(kb, 0) X(nz, 0) X(past, 0) X(run, 0) X(fp, 0) X(mvc, 0)         \
  X(rnz, 0) X(xm, 0) X(h8, 0) X(h16, 0) X(sl, 0) X(sa, 0) X(ie, NEED_REC) X(tq, NEED_REC)               \
  X(tx, NEED_REC) X(em, NEED_REC) X(pe, NEED_REC) X(pf, NEED_REC) X(td, NEED_REC)
#define M_lvl_VARS(X)                                                                                      \
  X(cls, 0) X(pos, 0) X(qs, 0) X(gs, 0) X(nmb, 0) X(kc, 0) X(pm, 0) X(nb2, 0) X(den, 0) X(mvc, 0)          \
  X(lh, 0) X(rb, 0) X(ssb, 0) X(ngt, 0) X(ie, NEED_REC) X(tq, NEED_REC) X(tx, NEED_REC) X(em, NEED_REC)    \
  X(pe, NEED_REC) X(pf, NEED_REC)
#define M_sgn_VARS(X)                                                                                      \
  X(cls, 0) X(pos, 0) X(first, 0) X(lsa, 0) X(gh, 0) X(ps, 0) X(flu, 0) X(flu2, 0) X(vote, 0)             \
  X(fdlu, 0) X(xs, 0) X(lam, 0) X(pp, NEED_REC) X(mg, NEED_REC) X(pp2, NEED_REC)    \
  X(mg2, NEED_REC) X(rk, NEED_REC) X(es, NEED_REC) X(ie, NEED_REC) X(pj, NEED_REC)  \
  X(pe, NEED_REC)
#define M_esc_VARS(X) X(dm, 0)

// the variables as the model gives them to a family
#define M_FIELD(f, needs) int f;
#define M_STRUCT(F) struct M_##F##_v { M_##F##_VARS(M_FIELD) };
M4V_FAMS(M_STRUCT)
#undef M_STRUCT
#undef M_FIELD

// a family's variables that need the part n (NEED_*), a bit per field (as famcfg.inc's Uses
// has them)
#define M_NEED(f, needs) | ((needs) & n ? 1ull << offsetof(V, f) / sizeof(int) : 0)
#define M_NEEDS(F)                 \
  template <unsigned n>            \
  constexpr u64 M_##F##_needs() {  \
    typedef M_##F##_v V;           \
    return 0 M_##F##_VARS(M_NEED); \
  }
M4V_FAMS(M_NEEDS)
#undef M_NEEDS
#undef M_NEED

// a family's variables as the locals of its index builder (IDX/l<L>/family.inc)
#define M_LOCAL(f, needs) \
  int f = D.f;            \
  (void)f;
