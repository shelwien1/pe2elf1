// famvars.inc -- what the model gives each family: its decision nodes and sign contexts, and the
// variables its IDX/l<L>/<family>.idx reads (the model fills them; the levels read parts of them).
// Apart from model.inc for IDX/famtune.cpp, which replays a family's decisions without the model.

// the families, X(name) each, in the order the model has them
#define AC3P_FAMS(X) X(hdr) X(sid) X(raw) X(exp) X(mag) X(aht)

// The fields the hdr and sid families code, each of their own values (fld): hdr's are the frame
// header's (syncinfo, bsi, audfrm) and the trailer's, numbered in AC3_FIELDS' order (syntax.inc),
// then the container's (below); sid's are an audio block's side information, with the exponents'
// absexp and gainrng and AHT's gaqmod (model_core.inc numbers them).
constexpr int NHF = 128, NSF = 64;
// the container's fields, after the frame's: a chunk's kind, the length of bytes kept as bytes,
// the bits after the last block all zero, and how many of its last bytes are not
enum { HX_KIND, HX_RAWLEN, HX_TAILZ, HX_TAILNZ, HX_N };

// decisions per context (ND, mantissa nodes MND), sign contexts (NG); the nodes of each:
//   hdr  code() of a field's value less its last one (nodes 0-12, the sign by field), and 13 the
//        value is the last one, 14 the one before it, 15 a flag (a field of one bit)
//   sid  the same, and 14 the value is that of the field's occurrence before it in the block
//   raw  a byte's bits, by their place in its binary tree (1-255)
//   exp  an exponent's delta: 0 none?, 1 down?, 2 down by 2?, 3 up by 2?
//   mag  a mantissa's magnitude: bap 1-5 0 + k (k 0-7): above k?; bap 6-15 8-39 its length (the
//        bits it takes) as a tree (model_core.inc's code_len()), 40-90 its two bits below the top
//        one by the length and the bits before them, the rest in the mantissa plane; the sign by
//        bap
//   aht  0 a gain of modes 1-2 (gaqgain); 1-2 one of mode 3 (0?, 1?); 3-18 a vector index's top 4
//        bits (a tree), 19-34 the bits below them by place; 35-117 a gain adaptive mantissa's
//        magnitude as mag's of bap 6-15 (the sign 0, of an escape's large mantissa 1)
enum { HN_SAME = TC_NODE, HN_SAME2, HN_BIT, HN_N };
enum { SN_SAME = TC_NODE, SN_SAMEP, SN_BIT, SN_N };
enum { EXN_ZERO, EXN_DOWN, EXN_DOWN2, EXN_UP2, EXN_N };
constexpr int MAG_SMALL = 8;   // mag: the nodes of bap 1-5's magnitudes, "above k?"
constexpr int LEN_NODES = 32;  // a length's tree: its nodes by the bits so far (1-31)
constexpr int LEN_TOP = 2;     // and the bits below the top one that follow it, by the length and the bits before them
constexpr int LEN_TOPN = ((1 << LEN_TOP) - 1) * 17;  // (lengths 0-16)
enum { MN_U = 0, MN_LEN = MN_U + MAG_SMALL, MN_N = MN_LEN + LEN_NODES + LEN_TOPN };
constexpr int AHT_VQTREE = 4;  // aht: a vector index's top bits, coded as a tree
enum { AN_GAIN, AN_G3A, AN_G3B, AN_VQ, AN_VQPOS = AN_VQ + (1 << AHT_VQTREE), AN_LEN = AN_VQPOS + 16,
       AN_N = AN_LEN + LEN_NODES + LEN_TOPN };
enum { AG_MANT, AG_BIG, AG_N };  // aht's sign contexts
constexpr int M_hdr_ND = HN_N, M_hdr_MND = TC_MNODE, M_hdr_NG = NHF;
constexpr int M_sid_ND = SN_N, M_sid_MND = TC_MNODE, M_sid_NG = NSF;
constexpr int M_raw_ND = BYTE_VALUES, M_raw_MND = 1, M_raw_NG = 1;
constexpr int M_exp_ND = EXN_N, M_exp_MND = 1, M_exp_NG = 1;
constexpr int M_mag_ND = MN_N, M_mag_MND = TC_MNODE, M_mag_NG = 16;
constexpr int M_aht_ND = AN_N, M_aht_MND = TC_MNODE, M_aht_NG = AG_N;

// Each family's variables, X(name) each, as M_<family>_v has them: what the model fills and the
// family's IDX/l<L>/<family>.idx reads. (Names of one letter are the index builders'.) They are as
// they are, or capped where few values lie above; where to split them is each .idx line's
// thresholds. "Last" is the substream's: each kind of substream (AC-3; E-AC-3 by strmtyp and
// substreamid) has its own frames before, and its last block is the one before in its frame, or
// the last of its frame before. An estimate is a coefficient's magnitude as a neighbour gives it,
// in quarter levels of the bin's own quantiser (model_mant.inc: 0 none, 1 the neighbour has no
// mantissa, else 2 + the estimate: 0-7 as they are, then two buckets a doubling, to 23). A bin is
// given as its band (A/52's, 0-49).
//
// hdr: fld the field, lv its last value at the same place in the frame (0 none, else 1 + it, to
//      63), l2 the value before that (0 none, 1 the same as the last, 2 another), pos the place
//      (the field's how many-th in the frame, to 31), st whether the field coded before it in the
//      frame kept its last value (0 none, 1 kept, 2 changed), nch how many fields changed so far
//      (to 3), sk the substream (0 AC-3, 1 E-AC-3 independent, 2 dependent, 3 converted, 4 not
//      known yet: bsid, strmtyp and substreamid, of the frame before whatever its substream)
// sid: fld, lv, pos, st, nch, sk as hdr's (lv the last block's, pos the field's place in the
//      block, st and nch in the block), blk the block, pq the value of the field's occurrence
//      before it in the block (0 none, else 1 + it, to 63), pr whether that one kept its last
//      value (as st), lb whether the last value is pq's (0 one of them none, 1 the same, 2 not)
// raw: c1, c2 the bytes before, code what the bytes are (0 between frames, 1 a frame kept as
//      bytes, 2 addbsi, 3 mixdata, 4 blkstrtinfo, 5 a skip field, 6 the bits after the last
//      block), at the byte's place (tc_qlog; the tail's from its end), cl the byte at the place
//      in the last field of the kind (0 none, else 1 + it), sk as hdr's
// exp: es the exponent strategy (1 D15, 2 D25, 3 D45), cc the channel (0 coupling, 1 the first
//      full bandwidth one, 2 another, 3 the LFE), pd the last block's exponent there (the least of
//      the delta's bins) less the exponent before the delta (4 + it, -4..4; 9 none), po the same
//      of the channel the channel is most like in the block (model_core.inc's partner()), d1, d2
//      the deltas before (0-4, 5 none), bin the delta's first bin, ex the exponent before the
//      delta (0-24), bs the block switch flag, ht whether the last three deltas were pd's (a bit
//      each), gi the delta's place in its group (0-2), bk the block, pl the last block's delta there
//      (0-4, 5 none)
// mag: bap the bin's (1-15), gr where it is in its group (0 the first of bap 1, 2, 4, 1 another,
//      2 bap 3 and 5, 3 bap 6-15), ex the bin's exponent, dx the bin's exponent less the one before
//      (4 + it, -4..4; 9 none), dn the next bin's less the bin's (the same), gs the exponents'
//      group size (1-3: 1, 2, 4 bins; 0 none), bs, cc as exp's, bin, e1, e2 the estimates of the
//      bins before, ep that of the last block at the bin, eo that of the partner at the bin, sm
//      the mean of the estimates of the four bins before (as an estimate, 0 none), ew that of the
//      last block's bins around the bin (the one before, it, the one after), ow that of the
//      partner's, m1 the last
//      magnitude (to 15, 16 the bin before has no mantissa, 17 none), nz how many of the four
//      mantissas before are not 0; for the signs (the g index), s1 the sign of the bin before, sp
//      the last block's at the bin, so the partner's (0 none or 0, 1 +, 2 -)
// aht: kd what (0 a vector index, 1 a gain of modes 1-2, 2 one of mode 3, 3 a gain adaptive
//      mantissa, 4 an escape's large mantissa), hb the hebap (0-19), lg the gain (log2, 0-2), gm
//      gaqmod, bk the block of a mantissa (0-5), pm the magnitude of the bin's mantissa in the
//      block before (in eighths of its range, 1 + it, 0 none), pb that of the block's mantissa of
//      the bin before with gain adaptive mantissas, pg the gain before (0-2, 3 none), ex the bin's
//      exponent, bin; the estimates (as mag's, of the mantissas' step, 2^(1 - bits), and the
//      exponent): q1, q2 those of the block's mantissas of the two bins before with gain adaptive
//      mantissas, q0 that of the bin's mantissa in block 0 (0 none: block 0), qa the mean of the
//      bin's blocks before
#define M_hdr_VARS(X) X(fld) X(lv) X(l2) X(pos) X(st) X(nch) X(sk)
#define M_sid_VARS(X) X(fld) X(lv) X(pos) X(st) X(nch) X(sk) X(blk) X(pq) X(pr) X(lb)
#define M_raw_VARS(X) X(c1) X(c2) X(code) X(at) X(cl) X(sk)
#define M_exp_VARS(X) X(es) X(cc) X(pd) X(po) X(d1) X(d2) X(bin) X(ex) X(bs) X(ht) X(gi) X(bk) X(pl)
#define M_mag_VARS(X)                                                                                   \
  X(bap) X(gr) X(ex) X(dx) X(dn) X(gs) X(bs) X(cc) X(bin) X(e1) X(e2) X(ep) X(eo) X(sm) X(ew) X(ow) X(m1) \
  X(nz) X(s1) X(sp) X(so)
#define M_aht_VARS(X) X(kd) X(hb) X(lg) X(gm) X(bk) X(pm) X(pb) X(pg) X(ex) X(bin) X(q1) X(q2) X(q0) X(qa)

// the variables as the model gives them to a family
#define M_FIELD(f) int f;
#define M_STRUCT(F) struct M_##F##_v { M_##F##_VARS(M_FIELD) };
AC3P_FAMS(M_STRUCT)
#undef M_STRUCT
#undef M_FIELD

// a family's variables as the locals of its index builder (IDX/l<L>/family.inc)
#define M_LOCAL(f) \
  int f = D.f;     \
  (void)f;
