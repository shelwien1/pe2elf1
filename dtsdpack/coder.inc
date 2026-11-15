// coder.inc -- the range coder of a run, as dtsdpack codes through it: rc.inc's (oggcomp's, as
// h264p has it), every family's decisions (tc_fam.inc's fbit()) and the stream's CRC, with where the
// bits go (-s) and whether what it decodes is damaged.

#include "rc.inc"  // h264p::Rangecoder: the coder, on the bytes of the coroutine it is

// what the bits are spent on (-s): bytes kept as bytes; the headers and the container; the
// subbands' side information (PMODE, PVQ, TMODE, the joint scale factors, the HF VQ vectors, the
// LFE); the bit allocations; the scale factors; the subband samples; the bits after the subframes
enum { CAT_RAW, CAT_HDR, CAT_SIDE, CAT_ABITS, CAT_SCALE, CAT_SMP, CAT_TAIL, CAT_X96, NCAT };

constexpr int CO_PBITS = h264p::SCALElog;               // a probability's bits (code_bit()'s p)
constexpr U32 CO_ONE = 1u << CO_PBITS;                  // (certain)
// A bit's cost by the probability q (CO_PBITS bits) of its value, -log2(q / CO_ONE), for the
// statistics (-s, and famtune's costs): by q's leading bit and log2(1 + i / 256) for the 8 after
// it (a table made as the program starts, as cm.inc's are), the bits below going linearly between
// its entries (within 3e-6 of a bit); and within 2^-8 of certain by the first two terms of
// -log2(1 - x) (within 3e-8 of a bit), where the families' and the hit flags' all but sure
// decisions cost as little as 2^-18 / ln 2 (each probability's own since format 102: its top 12
// bits made 3 MB of s.2.dts's start's subband samples 10% dearer than they are)
struct CoCosts {
  float m[257];
};
static CoCosts co_make_costs(void) {
  CoCosts t;
  for (int i = 0; i <= 256; i++) t.m[i] = (float)(log(1.0 + i / 256.0) / log(2.0));
  return t;
}
static const CoCosts bitcost_ = co_make_costs();
static INLINE float co_cost(U32 q) {
  if (q >= CO_ONE - (CO_ONE >> 8)) {
    const float x = (float)(CO_ONE - q) / (float)CO_ONE;
    return x * (1.4426950f + 0.7213475f * x);
  }
  const int e = 31 - __builtin_clz(q | 1), s = e - 8;
  const U32 i = (s >= 0 ? q >> s : q << -s) & 255;
  const float r = s > 0 ? (float)(q & ((1u << s) - 1)) / (float)(1u << s) : 0.0f;
  return (float)(CO_PBITS - e) - (bitcost_.m[i] + (bitcost_.m[i + 1] - bitcost_.m[i]) * r);
}

// The range coder of a run, coding (f_DEC 0) or decoding (1): the coroutine itself, which its base
// is (rc.inc's Rangecoder: its bytes go to the coroutine's output, or come from its input), with
// where its bits went by category and whether the data decoded is damaged. Plain data, on its run's
// zeroed pages: coder_start() as the coded data starts (decoding, its first bytes read then),
// coder_end() after the last decision (coding, the flush).
template <int f_DEC>
struct Coder : h264p::Rangecoder<f_DEC> {
  typedef h264p::Rangecoder<f_DEC> RC;
  using RC::f_quit;
  int err;            // dec: the data is damaged (a value no encoder writes)
  int cat;            // statistics: category of the bits coded now
  int stats;          // enc: count them (-s)
  double cost[NCAT];  // bits spent per category (enc)
  double orig[NCAT];  // bits they took in the input
#ifdef DTSD_FSTAT
  int fid;                 // (development: the bits by field)
  double fcost[F_N + 1];
#endif

  void coder_start() {
    err = cat = 0;
    memset(cost, 0, sizeof cost);
    memset(orig, 0, sizeof orig);
    RC::rc_Init();
  }
  void coder_end() { RC::rc_Quit(); }
  // dec: the data is damaged: a value no encoder writes, or the stream cut short (the decoder asked
  // for bytes past the input's end, where the driver has set f_quit: the coder reads exactly the
  // bytes it wrote)
  int damaged() const { return err || f_quit; }

  // a decision, p the probability of a 0 (CO_PBITS bits)
  INLINE int code_bit(U32 p, int bit) {
    uint b = (uint)bit;
    if (!f_DEC && stats) cost[cat] += co_cost(bit ? CO_ONE - p : p);
#ifdef DTSD_FSTAT
    if (!f_DEC && stats) fcost[fid] += co_cost(bit ? CO_ONE - p : p);
#endif
    RC::rc_Process(p, b);
    return (int)b;
  }

  // 32 bits as they are (the CRC), a byte at a time, each value of each as likely
  U32 code_u32(U32 v) {
    U32 r = 0;
    int i;
    cat = CAT_HDR;
    for (i = U32_BYTES - 1; i >= 0; i--) {
      if (!f_DEC && stats) cost[cat] += BYTE_BITS;
      r |= (U32)RC::rc_Byte((v >> (i * BYTE_BITS)) & BYTE_MASK) << (i * BYTE_BITS);
    }
    return r;
  }
};
