// famvars.inc -- what the model gives each family (as h264p's): its
// decision nodes and sign contexts, and the variables its
// IDX/l<L>/<family>.idx reads (the model fills them, model.inc).

namespace aacpack {

// the families, X(name) each, in the order the model has them
#define AAC_FAMS(X) X(unt) X(hdr) X(sec) X(scf) X(spc) X(esc) X(sgn) X(sbr)

// decisions per context (ND, mantissa nodes MND), sign contexts (NG)
constexpr int H_unt_ND = TC_NODE, H_unt_MND = TC_MNODE, H_unt_NG = 1;
constexpr int H_hdr_ND = TC_NODE, H_hdr_MND = TC_MNODE, H_hdr_NG = 256;
constexpr int H_sec_ND = 16, H_sec_MND = 1, H_sec_NG = 1;  // the codebook's tree (1-15), a section's going on (0)
constexpr int H_scf_ND = TC_NODE, H_scf_MND = TC_MNODE, H_scf_NG = 16;
constexpr int H_spc_ND = 16, H_spc_MND = 1, H_spc_NG = 1;  // the magnitude in unary: > j (j 0-15)
constexpr int H_esc_ND = TC_NODE, H_esc_MND = TC_MNODE, H_esc_NG = 1;
constexpr int H_sgn_ND = 1, H_sgn_MND = 1, H_sgn_NG = 1;
constexpr int H_sbr_ND = TC_NODE, H_sbr_MND = TC_MNODE, H_sbr_NG = 64;

// Each family's variables, X(name) each, as H_<family>_v has them: what
// the model fills and the family's IDX/l<L>/<family>.idx reads
//
// unt: the stream's own values (cmstream.inc): fld which, ctx a context
//   the caller gives, p1 p2 the last two of the field (tc_qlog)
#define H_unt_VARS(X) X(fld) X(ctx) X(p1) X(p2)
// hdr: a field of the syntax (the walk's val()): tag the element (E_*),
//   fld its index in a run (up to 63), p1 p2 the last two differences of
//   the element's values at that index in that channel (sq2), p1r the last
//   value's low byte, pos how many values in a row the element has had
//   (tc_qlog), nb its bits (0 a count), ws the channel's window sequence
//   (4 none), ch the channel element's channel (0, 1; 2 none)
#define H_hdr_VARS(X) X(tag) X(fld) X(p1) X(p2) X(p1r) X(pos) X(nb) X(ws) X(ch)
// sec: section_data: kd 0 the codebook (its bits, a tree: nd the node),
//   1 whether a section goes on to the next band; cb the section's
//   codebook (16 not yet), pc the one before it in the group (16 none), fc
//   the band's in the last frame of the channel (16 not known), gc in the
//   group before (16 none), oc of the other channel of the CPE (16 none),
//   sb the band, run the bands of the section so far, rem those left to
//   max_sfb, ws the window sequence, gr the group, fn the band in the last
//   frame (going on: 1 + 2 if its codebook was another + 1 if a section
//   started there; 0 not known)
#define H_sec_VARS(X) X(kd) X(cb) X(pc) X(fc) X(gc) X(oc) X(sb) X(run) X(rem) X(ws) X(gr) X(fn)
// scf: scale_factor_data: kd 0 a scalefactor, 1 an intensity position, 2
//   a noise energy; sb the band, d1 d2 the last two deltas of the kind in
//   the channel (sq2), pd the delta at the band in the last frame (sq2;
//   0 not known), pa the last frame's value at the band less this band's
//   predecessor's (sq2), cb the band's codebook, oc the other channel's
//   delta at the band (CPE, sq2), ws the window sequence, gr the group,
//   gd global_gain less the last frame's (1 + sq2; 0 not known), pv the
//   value so far less global_gain (intensity: the value; sq2)
#define H_scf_VARS(X) X(kd) X(sb) X(d1) X(d2) X(pd) X(pa) X(cb) X(oc) X(ws) X(gr) X(gd) X(pv)
// spc: the magnitude of a spectral value: cb its codebook, sb its band, bp
//   its place in the band, a1 a2 the magnitudes before it in the window
//   (up to 15), a3 the sum of the four before (tc_qlog), pf the magnitude at
//   its place in the last frame (1 +, 0 not known), pn the sum about it there
//   (1 + tc_qlog), ex that scaled by the change of the band's scalefactor
//   (1 + tc_qlog of 4 x), oc the other channel's at the place (CPE: 1 +), ms
//   ms_used, ws long 0, short 1, tp the place in the codebook's tuple, nz
//   the nonzero ones so far in the band (up to 7), ds the band's
//   scalefactor less the last frame's (sq2), qn the band's magnitudes so
//   far (tc_qlog of the sum), kq the place (tc_qlog)
#define H_spc_VARS(X) \
  X(cb) X(sb) X(bp) X(a1) X(a2) X(a3) X(pf) X(pn) X(ex) X(oc) X(ms) X(ws) X(tp) X(nz) X(ds) X(qn) X(kq)
// esc: an escaped magnitude of codebook 11, less 16: kq the place
//   (tc_qlog), a1 the magnitude before (tc_qlog), pf the last frame's at
//   the place (tc_qlog, 0 not known), oc the other channel's (tc_qlog + 1,
//   0 none), ws, la the last escape of the channel's (tc_qlog), sb the band
#define H_esc_VARS(X) X(kq) X(a1) X(pf) X(oc) X(ws) X(la) X(sb)
// sgn: the sign of a nonzero spectral value: cb, oc the other channel's
//   at the place (0 zero or none, 1 +, 2 -), ms ms_used, s1 the value's
//   before it (0 1 2), pf the last frame's at the place (0 1 2), mg its
//   magnitude (up to 7), kq its place (tc_qlog), s2 the one before s1, ws
#define H_sgn_VARS(X) X(cb) X(oc) X(ms) X(s1) X(pf) X(mg) X(kq) X(s2) X(ws)
// sbr: SBR envelopes and noise floors and PS parameters (deltas): kd the
//   kind (0 envelope f, 1 t, 2 noise f, 3 t; 4-7 the same of a balance
//   channel; 8 iid f, 9 t, 10 icc f, 11 t, 12 ipd, 13 opd; 14 a start
//   value), bd the band, nb the bands, d1 d2 the last two values in the
//   envelope (sq2), pv the value at the band in the last envelope of the
//   kind (sq2), ar amp_res, fr freq_res, en the envelope, ch the
//   channel, lv the level the band had (tc_qlog; 0 not known), q iid's fine
//   quantization
#define H_sbr_VARS(X) X(kd) X(bd) X(nb) X(d1) X(d2) X(pv) X(ar) X(fr) X(en) X(ch) X(lv) X(q)

// the variables as the model gives them to a family
#define H_FIELD(f) int f;
#define H_STRUCT(F) \
  struct H_##F##_v { H_##F##_VARS(H_FIELD) };
AAC_FAMS(H_STRUCT)
#undef H_STRUCT
#undef H_FIELD

// a family's variables as the locals of its index builder (IDX/l<L>/family.inc)
#define H_LOCAL(f) \
  int f = D.f;     \
  (void)f;

}  // namespace aacpack
