// famvars.inc -- what the model gives each family: its decision nodes and sign contexts, and the
// variables its IDX/l<L>/<family>.idx reads (the model fills them; the levels read parts of them).
// Apart from model.inc for IDX/famtune.cpp, which replays a family's decisions without the model.

// the families, X(name) each, in the order the model has them
#define DTSD_FAMS(X) X(hdr) X(sid) X(abt) X(scl) X(hit) X(smp) X(vqr) X(raw)

// The fields the hdr family codes, each of their own values (fld): the core frame's header and
// coding header, a subframe's header fields, then the container's (below), then the XCH
// extension's; and the sid family's, a subband's side information (SF_*).
constexpr int NHF = 80;  // (the levels' contexts take parts of a field's number: XXCH's, past 63, share theirs)
// the container's fields, after the frame's: a chunk's kind, the length of bytes kept as bytes,
// the bits after the subframes all zero, and how many of their last bytes are not; the same of the
// bits before the extension (and, at places 1-3, XXCH's other bits)
enum { HX_KIND, HX_RAWLEN, HX_TAILZ, HX_TAILNZ, HX_GAPZ, HX_GAPNZ, HX_TAILTZ, HX_GAPTZ, HX_N };
// sid's fields
// (SF_TMSR, SF_PMSR: TMODE against the transient search, PMODE against the values' fit)
enum { SF_PMODE, SF_PVQ, SF_TMODE, SF_JSHUFF, SF_JSCALE, SF_HFVQ, SF_LFE, SF_TMSR, SF_PMSR, SF_N };

// decisions per context (ND, mantissa nodes MND), sign contexts (NG); the nodes of each:
//   hdr  code() of a field's value less its last one (nodes 0-12, the sign by field), and 13 the
//        value is the last one, 14 the one before it, 15 a flag (a field of one bit)
//   sid  the same (14: the value is that of the place before in the subframe), then an index
//        (PVQ, HFVQ): 16-31 its top 4 bits as a tree, 32-47 the bits below them by place
//   abt  ABITS: code() of its difference from the last (0-12), 13 the last one, 14 the band below's
//   scl  a scale factor: code() of its difference from the prediction (0-12), 13 the prediction,
//        14 the last one (no prediction)
//   hit  13 an LFE sample's index is the audio's (a subband sample's goes to the coder as it is:
//        model_smp.inc's code_index()), 14 an index that isn't is the one on the other side of the
//        rounding boundary, 15 one that isn't either lies within HIT_NEAR of the audio's, 16 on the
//        side of that one (or above), 17-24 its distance in unary (by the side)
//   smp  a sample's index otherwise: its magnitude, the length (the bits it takes) as a tree and
//        the bits below the top (2-108: model_core.inc's code_len()), the rest in the mantissa
//        plane; 109 the sign
//   vqr  a rank (model_smp.inc's code_rank()): 13-24 its bucket (0; 1; 2-3; 4-7; ...) in unary,
//        25-63 the bits below the bucket's top, by the bucket and their place (the first two;
//        the rest)
//   raw  a byte's bits, by their place in its binary tree (1-255)
enum { HN_SAME = TC_NODE, HN_SAME2, HN_BIT, HN_N };
enum { SN_SAME = TC_NODE, SN_SAMEP, SN_BIT, SN_IX, SN_IXPOS = SN_IX + 16, SN_N = SN_IXPOS + 16 };
enum { AN_SAME = TC_NODE, AN_BELOW, AN_N };
enum { CN_HIT = TC_NODE, CN_SAME, CN_N };
constexpr int LEN_NODES = 32;  // a length's tree: its nodes by the bits so far (1-31)
constexpr int LEN_TOP = 2;     // and the bits below the top one that follow it, by the length and the bits before them
constexpr int LEN_MAX = 24;    // (lengths 0-24)
constexpr int LEN_TOPN = ((1 << LEN_TOP) - 1) * (LEN_MAX + 1);
enum { QN_LEN = 2, QN_SIGN = QN_LEN + LEN_NODES + LEN_TOPN, QN_N };  // (0-1 unused: hit has those decisions)
constexpr int RANK_BITS = 12;  // (a rank: 0 to 2^12 - 1, the ADPCM code book's)
enum { VN_BKT = TC_NODE, VN_BIT = VN_BKT + RANK_BITS, VN_N = VN_BIT + 3 * (RANK_BITS + 1) };
// what vqr ranks (kd): an ADPCM predictor's block (anal.inc's pvq_block_dist()), an HF VQ vector's
// group, the vector in its group, the ADPCM predictor in its block
enum { VK_PVQ, VK_HFG, VK_HFV, VK_PVE, VK_N };
constexpr int M_hdr_ND = HN_N, M_hdr_MND = TC_MNODE, M_hdr_NG = NHF;
constexpr int M_sid_ND = SN_N, M_sid_MND = TC_MNODE, M_sid_NG = SF_N;
constexpr int M_abt_ND = AN_N, M_abt_MND = TC_MNODE, M_abt_NG = 32;
constexpr int M_scl_ND = CN_N, M_scl_MND = TC_MNODE, M_scl_NG = 4;
constexpr int HIT_NEAR = 4;  // (an index neither the audio's nor alt, within this of the audio's: its distance from it)
enum { HT_HIT = TC_NODE, HT_ALT, HT_NEAR, HT_SIDE, HT_DIST, HT_N = HT_DIST + 2 * HIT_NEAR };
constexpr int M_hit_ND = HT_N, M_hit_MND = 1, M_hit_NG = 1;
constexpr int M_smp_ND = QN_N, M_smp_MND = TC_MNODE, M_smp_NG = 1;
constexpr int M_vqr_ND = VN_N, M_vqr_MND = 1, M_vqr_NG = 1;
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
//      none; a lossless WAV's TMODE: the transient's class, model_smp.inc's tmode_rule() as what
//      the ABITS said makes it, model.inc), ab the subband's ABITS where coded before (1 + it; 0
//      none; PMODE: the gain of the subband's LPC, in quarters of halvings, to 27), ec how its
//      energy changes over the subframe's subsubframes (0-8, 9 none; PMODE: model_smp.inc's
//      pmode_gain()), tr (PMODE, TMODE) how its energy rises against where the encoders seen take a
//      transient, and make no prediction (model_smp.inc's pm_rise(): 0-5; 0 for the other fields)
// abt: bd, cc, la the last ABITS (1 + it, 0 none), lb the subband below's in the subframe (1 + it,
//      0 none), es the audio's energy in the subband as a scale factor index (the RMS rule's; 1 +
//      it, 0 none), de its change from the last (0-8, 9 none), ls the last scale factor less the
//      last prediction (0-8, 9 none), pm whether the subband predicts (ADPCM), st how the subband
//      below's ABITS changed from its last (0-8, 9 none), nb the energy less its neighbours'
//      larger, in halves of scale factor steps (-4..4 as 0-8, 9 none), mx the loudest subband's in
//      the channel less it, in 4 steps (0-15, 16 none), sn the first guess (1 + it, 0 none): the
//      bit allocation search's (sc), else the staircase's (sm), else the ABITS the subband had the
//      last time it had this energy, in any channel (the one seen), sd whether sn is the last one
//      (0 none, 1 it is, 2 not), nn the ABITS seen at the energies either side less the one seen
//      (below's, -1..1 as 0-2 or 3 none, times 4, plus above's; 15 none; 14: that is the nearest
//      energy's, none having been seen at this one), sc where sn is the bit allocation search's
//      ABITS (a decoded WAV's), how sure it is: 1 + 2 times the log2 of its margin over the next
//      pair (to 6) + 1 where it is the first guess without the search (0 none), eq where in its
//      scale factor's bucket the energy lies, in quarters (0-3, 4 none: the encoder's thresholds
//      fall inside the buckets, so where the ABITS seen at the energy above or below differs, its
//      side's quarter makes the change likelier), sm where sn is the staircase's (model_smp.inc's
//      level_guess()), how near the energy lies to where its guess changes and on which side
//      (level_margin(): 0 none, 1-8 below, 9-16 above, by 0, 1, 2-3, 4-7, 8-15, 16-31, 32-63, more
//      128ths of a doubling), bg where sm isn't 0, the core's: the bits its subframe's samples may
//      still take (model_smp.inc's ab_budget()) toward that side's edge, in steps of the subband's
//      bits there (1 two or more, 2 one to two, 3 a half to one, 4 within a half, 5-7 the same away
//      from it; 0 none: the encoders fill their frames, so where the staircases would leave bits,
//      an ABITS near its upper edge rises, and where they would take too many, one near its lower
//      edge falls), ag where sn is the staircase's or the allocation table's, what the other one
//      says (0 none, 1 the same, 2 the ABITS across sn's nearer edge, 3 another; 4 a transient's,
//      whose guess at the other of its energies, the second guess, isn't sn: model_smp.inc's
//      ab_transient()); where sn is the bit allocation search's, what the staircase's or table's
//      guess says (0 none, 1 the same, 2 the last ABITS, 3 another)
// scl: kd (0 a subband's first scale factor, 1 its second, after its transient, 2 a high-frequency
//      VQ subband's, 3 the LFE's), ab its ABITS (1 + it, 0 none), pm whether it predicts, pk whether the audio
//      predicts the scale factor, pn how near its RMS lies to the midpoint between two (0-7), lp
//      the last scale factor less the last prediction (0-8, 9 none), hm whether the channel's
//      scale factor before was the prediction (0 none, 1 it was, 2 not), bd the subband, dp
//      where the run of the prediction's value in the table ends (anal.inc's sf_run(): 0 none, 1, 2
//      an entry below, above, 3 both ways, 4, 5 two entries, 6, 7 three or more), cc the
//      channel's class (`s.2.dts`'s encoder puts its fronts' and surrounds' top subband 1-4 entries
//      above the RMS rule's a third and a quarter of the time, its centre's 5%), nb the audio's
//      energy in the subband below less this one's, in halves of scale factor steps (-4..4 as
//      0-8, 9 none; the more the band below's, the likelier that; a lossless WAV's top subbands',
//      31 and 30, VQ: subband 0's or 1's power over their own, in doublings less 24, halved, 0-8),
//      pl a lossless WAV's RMS's nearness to the boundary as the encoder's arithmetic errs, in
//      doublings (model_smp.inc's scale_near_lev(): 0-14, 15 none; pn's, outside the VQ subbands)
// smp: ab ABITS (1-26; 27 an LFE sample's), hw the code (1 Huffman, 2 plain, 3 block codes), pk whether the audio
//      predicts the index, nr how near the rounding boundary its value lies, as log2 of its
//      distance in steps (0-15: 2^-17 or nearer to 2^-2 or farther), nl the same in the channel's
//      loudest subband's RMS (0-15: 2^-28 to 2^-13), ql the length of the audio's index (0-21), hm how many of the
//      subband's last 8 indexes weren't the audio's (to 3), pm whether the subband predicts, bd the
//      subband, m1, m2 the lengths of the two indexes before in the subband (22 none), sq the sign
//      of the audio's index (0 none or 0, 1 +, 2 -), s1 that of the index before
// hit: pm 0 a subband sample, 1 one of an ADPCM subband, 2 an LFE sample; nr, nl, ab, ql, hm as
//      smp's
// vqr: kd what is ranked (VK_*), cc, bd, mg how far the best candidate beats the next (0 within
//      1/256, 1 within 1/128 ... 11 by 4 times or more, 12 by all), gn the prediction gain of the subband's LPC in halvings
//      (VK_PVQ: 0-15) or its scale factor / 8 (0-15; a lossless WAV's top subbands', 31 and 30,
//      VQ: subband 0's or 1's power over their own, in doublings less 26, halved, model_smp.inc's
//      cpl_leak()), lh the subband's last rank of the kind (0 none, 1 the best, 2 another), ml a
//      lossless WAV's HF VQ group's or vector's margin against the channel's level times the scale
//      factor, in doublings from 2^-25 (the groups' scores' from 2^-6; 0-13, 14 the best exact,
//      15 none), nc a lossless WAV's HF VQ group's or vector's candidates but the nearest within
//      2^-18 of ml's unit of it (0, 1, 2, 3-4, 5-8, more: 0-5; 6 none), gr past a miss's first bucket t (the
//      rank 2^t or more?), how far the next bucket's first candidate (2^t) lies from the best against
//      this one's (2^(t-1)), in doublings (0 within 2 times, 6 by 64 or more; 7 none: the first
//      bucket, the bits below a bucket's top, no candidate 2^t)
// raw: c1, c2 the bytes before, code what the bytes are (0 between frames, 1 a frame kept as bytes,
//      2 the bits after the subframes, 3 those before the XCH extension), at the byte's place
//      (tc_qlog; the tail's and the gap's from their end), cl
//      the byte at the place in the last field of the kind (0 none, else 1 + it)
#define M_hdr_VARS(X) X(fld) X(lv) X(l2) X(pos) X(st) X(nch)
#define M_sid_VARS(X) X(fld) X(cc) X(bd) X(lv) X(pq) X(st) X(ac) X(ab) X(ec) X(tr)
#define M_abt_VARS(X) X(bd) X(cc) X(la) X(lb) X(es) X(de) X(ls) X(pm) X(st) X(nb) X(mx) X(sn) X(sd) X(nn) X(sc) X(eq) X(sm) X(bg) X(ag)
#define M_scl_VARS(X) X(kd) X(ab) X(pm) X(pk) X(pn) X(lp) X(hm) X(bd) X(dp) X(cc) X(nb) X(pl)
#define M_hit_VARS(X) X(pm) X(nr) X(nl) X(ab) X(ql) X(hm)
#define M_smp_VARS(X) X(ab) X(hw) X(pk) X(nr) X(nl) X(ql) X(hm) X(pm) X(bd) X(m1) X(m2) X(sq) X(s1)
#define M_vqr_VARS(X) X(kd) X(cc) X(bd) X(mg) X(gn) X(lh) X(ml) X(nc) X(gr)
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
