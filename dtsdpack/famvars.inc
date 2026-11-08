// famvars.inc -- what the model gives each family: its decision nodes and sign contexts, and the
// variables its IDX/l<L>/<family>.idx reads (the model fills them; the levels read parts of them).
// Apart from model.inc for IDX/famtune.cpp, which replays a family's decisions without the model.

// the families, X(name) each, in the order the model has them
#define DTSD_FAMS(X) X(hdr) X(sid) X(abt) X(scl) X(smp) X(raw)

// The fields the hdr family codes, each of their own values (fld): the core frame's header and
// coding header, a subframe's header fields, then the container's (below); and the sid family's,
// a subband's side information (SF_*).
constexpr int NHF = 64;
// the container's fields, after the frame's: a chunk's kind, the length of bytes kept as bytes,
// the bits after the subframes all zero, and how many of their last bytes are not
enum { HX_KIND, HX_RAWLEN, HX_TAILZ, HX_TAILNZ, HX_N };
// sid's fields
enum { SF_PMODE, SF_PVQ, SF_TMODE, SF_JSHUFF, SF_JSCALE, SF_HFVQ, SF_LFE, SF_N };

// decisions per context (ND, mantissa nodes MND), sign contexts (NG); the nodes of each:
//   hdr  code() of a field's value less its last one (nodes 0-12, the sign by field), and 13 the
//        value is the last one, 14 the one before it, 15 a flag (a field of one bit)
//   sid  the same (14: the value is that of the place before in the subframe), then an index
//        (PVQ, HFVQ): 16-31 its top 4 bits as a tree, 32-47 the bits below them by place
//   abt  ABITS: code() of its difference from the last (0-12), 13 the last one, 14 the band below's
//   scl  a scale factor: code() of its difference from the prediction (0-12), 13 the prediction,
//        14 the last one (no prediction)
//   smp  a sample's index: 0 the audio's, 1 the one on the other side of the rounding boundary,
//        then its magnitude, the length (the bits it takes) as a tree and the bits below the top
//        (2-108: model_core.inc's code_len()), the rest in the mantissa plane; 109 the sign
//   raw  a byte's bits, by their place in its binary tree (1-255)
enum { HN_SAME = TC_NODE, HN_SAME2, HN_BIT, HN_N };
enum { SN_SAME = TC_NODE, SN_SAMEP, SN_BIT, SN_IX, SN_IXPOS = SN_IX + 16, SN_N = SN_IXPOS + 16 };
enum { AN_SAME = TC_NODE, AN_BELOW, AN_N };
enum { CN_HIT = TC_NODE, CN_SAME, CN_N };
constexpr int LEN_NODES = 32;  // a length's tree: its nodes by the bits so far (1-31)
constexpr int LEN_TOP = 2;     // and the bits below the top one that follow it, by the length and the bits before them
constexpr int LEN_MAX = 24;    // (lengths 0-24)
constexpr int LEN_TOPN = ((1 << LEN_TOP) - 1) * (LEN_MAX + 1);
enum { QN_HIT, QN_ALT, QN_LEN, QN_SIGN = QN_LEN + LEN_NODES + LEN_TOPN, QN_N };
constexpr int M_hdr_ND = HN_N, M_hdr_MND = TC_MNODE, M_hdr_NG = NHF;
constexpr int M_sid_ND = SN_N, M_sid_MND = TC_MNODE, M_sid_NG = SF_N;
constexpr int M_abt_ND = AN_N, M_abt_MND = TC_MNODE, M_abt_NG = 32;
constexpr int M_scl_ND = CN_N, M_scl_MND = TC_MNODE, M_scl_NG = 4;
constexpr int M_smp_ND = QN_N, M_smp_MND = TC_MNODE, M_smp_NG = 1;
constexpr int M_raw_ND = BYTE_VALUES, M_raw_MND = 1, M_raw_NG = 1;

// Each family's variables, X(name) each, as M_<family>_v has them: what the model fills and the
// family's IDX/l<L>/<family>.idx reads. (Names of one letter are the index builders'.) "Last" is
// the subband's in the subframe before (the frame before's, mostly). A value as a variable is 0 for
// none, else 1 + it (capped); a difference -4..4 is 0-8 (9 none).
//
// hdr: fld the field, lv its last value at the same place in the frame (0 none, else 1 + it, to
//      63), l2 the value before that (0 none, 1 the same as the last, 2 another), pos the place
//      (the field's how many-th in the frame, to 31), st whether the field coded before it kept its
//      last value (0 none, 1 kept, 2 changed), nch how many fields changed so far (to 3)
// sid: fld (SF_*), cc the channel (0 the first, 1 the next two, 2 the others), bd the subband,
//      lv its last value (0 none, else 1 + it, to 63), pq the value of the place before in the
//      subframe (the subband below's; LFE: the sample before), st whether that one kept its last
//      value, ac the audio's subband's lag-1 autocorrelation over the subframe (0-7, as -1..1; 8
//      none), ab the subband's ABITS where coded before (1 + it; 0 none), ec how its energy changes
//      over the subframe's subsubframes (0-8, 9 none)
// abt: bd, cc, la the last ABITS (1 + it, 0 none), lb the subband below's in the subframe (1 + it,
//      0 none), es the audio's energy in the subband as a scale factor index (the RMS rule's, / 4;
//      1 + it, 0 none), de its change from the last (0-8, 9 none), ls the last scale factor less
//      the last prediction (0-8, 9 none), pm whether the subband predicts (ADPCM), st whether the
//      subband below kept its last ABITS (0 none, 1 kept, 2 changed)
// scl: kd (0 a subband's first scale factor, 1 its second, after its transient, 2 a high-frequency
//      VQ subband's), ab its ABITS (1 + it, 0 none), pm whether it predicts, pk whether the audio
//      predicts the scale factor, pn how near its RMS lies to the midpoint between two (0-7), lp
//      the last scale factor less the last prediction (0-8, 9 none), hm whether the channel's
//      scale factor before was the prediction (0 none, 1 it was, 2 not), bd the subband
// smp: ab ABITS (1-26), hw the code (1 Huffman, 2 plain, 3 block codes), pk whether the audio
//      predicts the index, nr how near the rounding boundary its value lies (0-20: within 1/400 of
//      a step to 1/20, 20 farther), ql the length of the audio's index (0-21), hm how many of the
//      subband's last 8 indexes weren't the audio's (to 3), pm whether the subband predicts, bd the
//      subband, m1, m2 the lengths of the two indexes before in the subband (22 none), sq the sign
//      of the audio's index (0 none or 0, 1 +, 2 -), s1 that of the index before
// raw: c1, c2 the bytes before, code what the bytes are (0 between frames, 1 a frame kept as bytes,
//      2 the bits after the subframes), at the byte's place (tc_qlog; the tail's from its end), cl
//      the byte at the place in the last field of the kind (0 none, else 1 + it)
#define M_hdr_VARS(X) X(fld) X(lv) X(l2) X(pos) X(st) X(nch)
#define M_sid_VARS(X) X(fld) X(cc) X(bd) X(lv) X(pq) X(st) X(ac) X(ab) X(ec)
#define M_abt_VARS(X) X(bd) X(cc) X(la) X(lb) X(es) X(de) X(ls) X(pm) X(st)
#define M_scl_VARS(X) X(kd) X(ab) X(pm) X(pk) X(pn) X(lp) X(hm) X(bd)
#define M_smp_VARS(X) X(ab) X(hw) X(pk) X(nr) X(ql) X(hm) X(pm) X(bd) X(m1) X(m2) X(sq) X(s1)
#define M_raw_VARS(X) X(c1) X(c2) X(code) X(at) X(cl)

// the variables as the model gives them to a family
#define M_FIELD(f) int f;
#define M_STRUCT(F) struct M_##F##_v { M_##F##_VARS(M_FIELD) };
DTSD_FAMS(M_STRUCT)
#undef M_STRUCT
#undef M_FIELD

// a family's variables as the locals of its index builder (IDX/l<L>/family.inc)
#define M_LOCAL(f) \
  int f = D.f;     \
  (void)f;
