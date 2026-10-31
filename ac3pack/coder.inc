// coder.inc -- the range coder of a run, as ac3pack codes through it: rc.inc's (oggcomp's, as
// h264p has it), every family's decisions (fam.inc's tc_bit) and the stream's CRC, with where the
// bits go (-s) and whether what it decodes is damaged.

#include "rc.inc"  // h264p::Rangecoder: the coder, on the bytes of the coroutine it is

// what the bits are spent on (-s): bytes kept as bytes; the headers, the CRCs and the trailer; the
// blocks' side information; the exponents; the mantissas' magnitudes and their signs; AHT's data;
// the skip fields; the bits after the last block
enum { CAT_RAW, CAT_HDR, CAT_SIDE, CAT_EXP, CAT_MANT, CAT_SIGN, CAT_AHT, CAT_SKIP, CAT_TAIL, NCAT };

constexpr int CO_PBITS = h264p::SCALElog;               // a probability's bits (code_bit()'s p)
constexpr U32 CO_ONE = 1u << CO_PBITS;                  // (certain)
constexpr int CO_COST_SHIFT = 3;                        // bitcost[] by a probability's top bits
constexpr int CO_COSTS = 1 << (CO_PBITS - CO_COST_SHIFT);

// -log2(i / CO_COSTS): a bit's cost by its probability, made as the program starts (as cm.inc's
// tables are)
struct CoCosts {
  float c[CO_COSTS + 1];
};
static CoCosts co_make_costs(void) {
  CoCosts t;
  int i;
  t.c[0] = CO_PBITS;  // (a probability of 0 as one of the least)
  for (i = 1; i <= CO_COSTS; i++) t.c[i] = (float)(-log((double)i / CO_COSTS) / log(2.0));
  return t;
}
static const CoCosts bitcost_ = co_make_costs();
static const float (&bitcost)[CO_COSTS + 1] = bitcost_.c;

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
    if (!f_DEC && stats) cost[cat] += bitcost[(bit ? CO_ONE - p : p) >> CO_COST_SHIFT];
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
