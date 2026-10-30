// fam.inc -- a model family (h264p's tc_fam, from oggcomp, balrogg-P-hash tc_fam.inc): the
// integer coder every IDX family is. A value is a cascade of binary decisions -- zero? one? two?
// then the length of the rest in unary and its bits below the top one -- and each decision is
// predicted from up to ten counters (tables A-E, H-L under as many contexts), an APM, three
// mixers and a second APM, all indexed by the decision's node in the cascade; a level's families
// may have fewer (four counters and one mixer at -1: the shape is the family's class's,
// famcfg.inc). Decisions outside the cascade use bit() with nodes of their own, and the sign of a
// value has its own counters (G). prp, when set, is one more prediction for the mixer; hint, a
// magnitude the value is expected near, one more set of counters.
//
// Changed from h264p's: the bits go through m4vpack's range coder (Coder, coder.inc), a value
// too long to be one marks the coder's error (damaged data: the walk fails its chunk) instead of
// jumping to a trap, and what can only be a bug ends the program (fatal()).

//  A value's cascade: the values below TC_SMALL decided one by one (zero? one? two?), then the
//  rest's length in unary (its first TC_ULEN decisions by their own nodes, the rest by one more),
//  then its bits below the top one; the nodes of the first two parts
constexpr int TC_SMALL = 3, TC_ULEN = 9;
constexpr int TC_NODE = TC_SMALL + TC_ULEN + 1;
//  the mantissa's nodes: its lengths (TC_MLEN, the longer as the last), its tree over its top bits
//  (TC_MTREE deep), its other bits by place (TC_MPOS, the later as the last)
constexpr int TC_MLEN = 13, TC_MTREE = 6, TC_MPOS = 13;
constexpr int TC_MPER = (1 << TC_MTREE) - 1 + TC_MPOS;
constexpr int TC_MNODE = TC_MLEN * TC_MPER;
constexpr int TC_NBMAX = 62;
constexpr sz TC_VOL_MAX = (sz)1 << 31;  // a table's rows (times its nodes), at most
constexpr int TC_NC_MAX = 10;           // a family's counters, at most
//  the hint's states (hs): none, it decides 0, it decides 1, the value is above it, below it
enum { HS_NONE, HS_AT0, HS_AT1, HS_ABOVE, HS_BELOW, HS_N };
//  the mixers' constant input, the bias
constexpr int TC_BIAS = 256;
//  the final probability: the mixing's share of it against APM 2's, in TC_BW_ONE-ths (bw)
constexpr int TC_BW_BITS = 4, TC_BW_ONE = 1 << TC_BW_BITS;
//  the final mixer's inputs (the three mixers', the bias), the mantissa mixer's (counter T's, the
//  hint's counter's, the bias)
constexpr int TC_FIN = 4, TC_MIN = 3;
//  the mixers' inputs besides the counters' (A's APM refinement, the outside prediction, the bias,
//  the hint's counter), all of them padded to TC_LANES (AVX2's 32-bit lanes)
constexpr int TC_XIN = 4, TC_LANES = 8;
constexpr int tc_nin(int nc) { return (2 * nc + TC_XIN + TC_LANES - 1) / TC_LANES * TC_LANES; }
//  the .idx parameters' ranges: a counter's target margin (in TC_MW_UNIT-ths of CM_CONE, to
//  TC_MW_MAX), a mixer's learning rate (1 to TC_LR_MAX) and boost (to TC_MB_MAX)
constexpr int TC_MW_UNIT = 64, TC_MW_MAX = CM_CONE / 4, TC_LR_MAX = 64, TC_MB_MAX = 255;
//  AVX2: the counters a vector holds (a 64-bit lane each), the counters' lanes padded to vectors
constexpr int TC_CPV = 4;
constexpr int TC_NC_PAD = (TC_NC_MAX + TC_CPV - 1) / TC_CPV * TC_CPV;
static INLINE int tc_node_len(int k) { return TC_SMALL + (k < TC_ULEN ? k : TC_ULEN); }
//  A mantissa bit's node: by the length, and for the top mt bits (up to
//  TC_MTREE) by the bits above them (pref, a 1 and those), the rest by
//  position
static INLINE int tc_node_man(int nb, int p, u64 pref, int mt) {
  return (nb > TC_MLEN - 1 ? TC_MLEN - 1 : nb) * TC_MPER +
         (p < mt ? (int)pref - 1 : (1 << TC_MTREE) - 1 + (p > TC_MPOS - 1 ? TC_MPOS - 1 : p));
}
static void tc_vfits(const char *what, sz vol, sz nodes) {
  if(!(vol > 0 && vol < TC_VOL_MAX && nodes > 0))
    fatal("an IDX family gives the %s an index of %llu rows; a Volume must stay under 2^31", what, (unsigned long long)vol);
}
static void tc_ifits(const char *what, sz vol, sz mult) {
  tc_vfits(what, vol, mult);
  if(!(vol * mult < TC_VOL_MAX))
    fatal("an IDX family gives the %s %llu rows of %llu; the product must stay under 2^31", what, (unsigned long long)vol,
          (unsigned long long)mult);
}
//  The context indices of one value: what idx2inc.pl's MakeIndex builds
//  (c: the counters', A-E, H-L in turn)
struct tcx {
  int c[TC_NC_MAX];
  int s, f, m, w, t, n, g;
};
#include "tc_tabs.inc"  // tc_tabs: a family's tables

//  One bit through the coder of the run (coder.inc), p the probability of a 0 (12 bits)
static INLINE int tc_bit(Coder *rc, int p, int bit) { return rc->code_bit((U32)(CM_PONE - p) << (CO_PBITS - CM_PBITS), bit); }

//  The dump (IDX/famtune.cpp): what each family is asked to code as the
//  compressor runs, written by a build with M4VPACK_DUMP to
//  $M4VPACK_DUMP/<family>.dump -- R where a stream starts, then the
//  variables of each value (M, at make()), each select() (S), and each
//  decision with what reaches it from outside the family: the outside
//  prediction and the hint's state (B a decision of bit_x(); m a mantissa
//  bit, by its place in the number, so that the tree's depth mt can
//  change; G a sign). None of it depends on the family's parameters, so
//  that famtune codes one dump again under each of them. A record is its
//  letter and its numbers, each a zigzag varint (a byte for those within
//  -64..63), in the order of tc_dump's calls (tc_dump.inc).
constexpr int TC_VBITS = BYTE_BITS - 1;   // a varint's bits per byte,
constexpr int TC_VMORE = 1 << TC_VBITS;   // and its flag of more to come
static INLINE void tc_dump_put(FILE *f, i64 v) {
  u64 u = ((u64)v << 1) ^ (u64)(v >> (U64_BITS - 1));
  for(; u >= TC_VMORE; u >>= TC_VBITS)
    putc((int)(u & (TC_VMORE - 1)) | TC_VMORE, f);
  putc((int)u, f);
}
static INLINE i64 tc_dump_get(const char *&p, const char *e) {
  u64 u = 0;
  for(int s = 0;; s += TC_VBITS) {
    if(UNLIKELY(p == e || s > U64_BITS - 1))
      fatal("a dump cut short");
    u8 c = (u8)*p++;
    u |= (u64)(c & (TC_VMORE - 1)) << s;
    if(!(c & TC_VMORE))
      break;
  }
  return (i64)(u >> 1) ^ -(i64)(u & 1);
}
#ifdef M4VPACK_DUMP
#include "tc_dump.inc"  // tc_dump: a family's dump
#define TC_DUMP(x)                                                                                                     \
  do {                                                                                                                 \
    if(ENC)                                                                                                            \
      dump_.x;                                                                                                         \
  } while(0)
#else
#define TC_DUMP(x)                                                                                                     \
  do {                                                                                                                 \
  } while(0)
#endif

//  A counter's probabilities stretched: looked up (cm_cnt2), or as the
//  counter keeps them (cm_cnt2s)
static INLINE int tc_sp(const cm_cnt2 &c) { return cm_stretch(c.P()); }
static INLINE int tc_sq(const cm_cnt2 &c) { return cm_stretch(c.Q()); }
static INLINE int tc_sp(const cm_cnt2s &c) { return c.sp; }
static INLINE int tc_sq(const cm_cnt2s &c) { return c.sq; }
#ifdef M4V_AVX2
//  The same of four counters at once (four false: two, the upper lanes 0),
//  as the 32-bit lanes P and Q of each in turn (v: P and Q so)
constexpr int TC_I16_BITS = BYTE_BITS * sizeof(i16);  // (a 32-bit lane's upper half)
constexpr int TC_DUP_EVEN = _MM_SHUFFLE(2, 2, 0, 0);    // (each 64-bit lane's low 32 bits, twice)
constexpr int TC_ODD32 = 0xAA;                         // (a blend's odd 32-bit lanes)
static INLINE __m256i tc_st4(cm_cnt2 *const *, bool, __m256i v) {
  return _mm256_srai_epi32(_mm256_slli_epi32(_mm256_i32gather_epi32((const int *)cm_st, v, sizeof(i16)), TC_I16_BITS),
                           TC_I16_BITS);
}
static INLINE __m256i tc_st4(cm_cnt2s *const *c, bool four, __m256i) {
  auto ld = [&](int k) {  // (sp and sq)
    int v;
    static_assert(sizeof v == 2 * sizeof(i16), "tc_st4: two 16-bit lanes in a 32-bit one");
    memcpy(&v, &c[k]->sp, sizeof v);
    return v;
  };
  return _mm256_cvtepi16_epi32(four ? _mm_setr_epi32(ld(0), ld(1), ld(2), ld(3)) : _mm_setr_epi32(ld(0), ld(1), 0, 0));
}
//  At their update, what a counter's 8-byte store writes above its q, in
//  the odd lanes' upper halves: the next counter's p as it was (cm_cnt2,
//  from cv, the counters as read), or its new P stretched (cm_cnt2s, from
//  nv, the new p and q, and t1, the new counts; sn: P and Q stretched)
static INLINE __m256i tc_upper(cm_cnt2 *const *, __m256i cv, __m256i, __m256i, __m256i &) {
  return _mm256_andnot_si256(_mm256_set1_epi32(UINT16_MAX), cv);
}
static INLINE __m256i tc_upper(cm_cnt2s *const *, __m256i, __m256i nv, __m256i t1, __m256i &sn) {
  const __m256i m16 = _mm256_set1_epi32(UINT16_MAX);
  __m256i v = _mm256_max_epi32(_mm256_srli_epi32(_mm256_and_si256(nv, m16), CM_CBITS - CM_PBITS), _mm256_set1_epi32(1));
  v = _mm256_blendv_epi8(v, _mm256_set1_epi32(CM_PHALF), _mm256_cmpeq_epi32(t1, _mm256_setzero_si256()));
  sn = _mm256_and_si256(_mm256_i32gather_epi32((const int *)cm_st, v, sizeof(i16)), m16);
  return _mm256_slli_epi64(_mm256_and_si256(sn, _mm256_set1_epi64x(UINT16_MAX)), 3 * TC_I16_BITS);  // (the top lane)
}
//  and cm_cnt2s's Q stretched, after the store (the 16-bit lanes of the counters' Qs)
static INLINE void tc_put_sq(cm_cnt2 *const *, bool, __m256i) {}
static INLINE void tc_put_sq(cm_cnt2s *const *c, bool four, __m256i sn) {
  c[0]->sq = (i16)_mm256_extract_epi16(sn, 2);
  c[1]->sq = (i16)_mm256_extract_epi16(sn, 6);
  if(four) {
    c[2]->sq = (i16)_mm256_extract_epi16(sn, 10);
    c[3]->sq = (i16)_mm256_extract_epi16(sn, 14);
  }
}
#endif
//  counter A's P stretched, as the mixers see it without APMs (p1 = its P)
static INLINE int tc_x20(const cm_cnt2 *, int p1) { return cm_stretch(p1); }
static INLINE int tc_x20(const cm_cnt2s *c, int) { return c->sp; }

#include "tc_fam.inc"  // tc_fam: a family of a level
