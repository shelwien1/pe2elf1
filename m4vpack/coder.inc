// coder.inc -- the binary arithmetic coder (a carryless 32-bit range coder as in lpaq) every
// family codes its decisions with (fam.inc's tc_bit), and where the bits go (-s).

enum { CAT_RAW, CAT_HDR, CAT_MB, CAT_CBP, CAT_MV, CAT_DC, CAT_SIG, CAT_LEV, CAT_SIGN, CAT_ESC, NCAT };

typedef struct Coder {
  int enc;
  U32 x1, x2, x;
  CoIo* io;           // where its bytes go (enc) or come from (dec)
  int err;            // dec: the data is damaged (a value no encoder writes)
  int cat;            // statistics: category of the bits coded now
  double cost[NCAT];  // bits spent per category (enc)
  double orig[NCAT];  // bits they took in the input (CAT_MB includes CAT_CBP, CAT_SIG all coefficient bits)
  int stats;
} Coder;

constexpr int CO_PBITS = 16;                            // a probability's bits (code_bit()'s p)
constexpr U32 CO_ONE = 1u << CO_PBITS;                  // (certain)
constexpr int CO_TOP = U32_BITS - BYTE_BITS;            // the shift of the range's top byte
constexpr U32 CO_TOP_MASK = BYTE_MASK << CO_TOP;
constexpr int CO_COST_SHIFT = 4;                        // bitcost[] by a probability's top bits
constexpr int CO_COSTS = 1 << (CO_PBITS - CO_COST_SHIFT);

static INLINE void co_put(Coder* c, int b) { c->io->putb(b); }
static INLINE int co_get(Coder* c) {  // after the end of the input: 255, which continues the flushed lower bound
  const int v = c->io->getb();
  return v < 0 ? (int)BYTE_MASK : v;
}
static void co_init(Coder* c, CoIo* io, int enc) {
  int i;
  memset(c, 0, sizeof(*c));
  c->enc = enc;
  c->io = io;
  c->x2 = UINT32_MAX;
  if (!enc) for (i = 0; i < U32_BYTES; i++) c->x = (c->x << BYTE_BITS) | (U32)co_get(c);
}
static inline void co_flush(Coder* c) { co_put(c, (int)(c->x1 >> CO_TOP)); }

// -log2(i / CO_COSTS): a bit's cost by its probability, made as the program starts (as cm.inc's tables
// are)
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

static int code_bit(Coder* c, U32 p, int bit) {  // p: probability of a 1 bit, CO_PBITS bits
  const U32 xmid = c->x1 + (U32)(((U64)(c->x2 - c->x1) * p) >> CO_PBITS);
  if (c->enc) {
    if (c->stats) c->cost[c->cat] += bitcost[(bit ? p : CO_ONE - p) >> CO_COST_SHIFT];
    if (bit) c->x2 = xmid; else c->x1 = xmid + 1;
    while (!((c->x1 ^ c->x2) & CO_TOP_MASK)) {
      co_put(c, (int)(c->x2 >> CO_TOP));
      c->x1 <<= BYTE_BITS;
      c->x2 = (c->x2 << BYTE_BITS) | BYTE_MASK;
    }
  } else {
    bit = c->x <= xmid;
    if (bit) c->x2 = xmid; else c->x1 = xmid + 1;
    while (!((c->x1 ^ c->x2) & CO_TOP_MASK)) {
      c->x1 <<= BYTE_BITS;
      c->x2 = (c->x2 << BYTE_BITS) | BYTE_MASK;
      c->x = (c->x << BYTE_BITS) | (U32)co_get(c);
    }
  }
  return bit;
}

// 32 bits at a probability of 1/2 each (the CRC)
static inline U32 code_u32(Coder* c, U32 v) {
  U32 x = 0;
  int i;
  c->cat = CAT_HDR;
  for (i = U32_BITS - 1; i >= 0; i--) x |= (U32)code_bit(c, CO_ONE / 2, (int)(v >> i) & 1) << i;
  return x;
}
