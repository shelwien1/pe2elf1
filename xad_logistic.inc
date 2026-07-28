/* xad_logistic.inc -- part of xadpcm.cpp; not standalone.
 *
 * squash / stretch / code-length tables, the per-role counter rate schedules,
 * and init_tables().  SQT and STT feed every mixed bit. */

static const int ST_SCALE = pclamp(G0_ST_SCALE, 16, 2048);

static int SQT[P_ONE];    // squash: d+P_MID -> p12
static short STT[P_ONE];  // stretch: p12 -> d
#if XAD_STATS
static u16 CLT[P_ONE][2]; // -log2 P(bit) in 1/ST_SCALE bit, accounting only
#endif
/* A counter's adaptation is three numbers -- the rate schedule by update
   count, where that schedule stops advancing, and the prior it starts from --
   and the three model roles want different ones.  The code tree sees a symbol
   per sample and wants to track; the block-header fields see a few values per
   block; the literal bytes are near-static.  One shared schedule made all three
   compromise, so each role now carries its own, and the set travels with the
   call site rather than with the counter, which keeps Ctr at four bytes. */
struct RateSet {
  u32 cw[CTR_NRATE];
  int nmax;
  u16 init;
};
static RateSet RS_CODE, RS_HDR, RS_LIT, RS_MATCH;

static void fill_rates(RateSet &r, const int* cw, int nmax, int init) {
  for( int i = 0; i<CTR_NRATE; i++ )
    r.cw[i] = u32(PMUL(cw[i]));
  r.nmax = pclamp(nmax, 1, CTR_NRATE-1);
  r.init = u16(pclamp(init, 1, CTR_MAX));
}

static void init_tables() {
  const int ccw[CTR_NRATE] = {G0_CW0, G0_CW1, G0_CW2, G0_CW3, G0_CW4, G0_CW5, G0_CW6, G0_CW7, G0_CW8, G0_CW9, G0_CW10, G0_CW11, G0_CW12, G0_CW13, G0_CW14, G0_CW15};
  const int hcw[CTR_NRATE] = {G0_HW0, G0_HW1, G0_HW2, G0_HW3, G0_HW4, G0_HW5, G0_HW6, G0_HW7, G0_HW8, G0_HW9, G0_HW10, G0_HW11, G0_HW12, G0_HW13, G0_HW14, G0_HW15};
  const int lcw[CTR_NRATE] = {G0_LW0, G0_LW1, G0_LW2, G0_LW3, G0_LW4, G0_LW5, G0_LW6, G0_LW7, G0_LW8, G0_LW9, G0_LW10, G0_LW11, G0_LW12, G0_LW13, G0_LW14, G0_LW15};
  fill_rates(RS_CODE, ccw, G0_CW_NMAX, G0_CP_INIT);
  fill_rates(RS_HDR, hcw, G0_HW_NMAX, G0_HP_INIT);
  fill_rates(RS_LIT, lcw, G0_LW_NMAX, G0_LP_INIT);
  const int mcw[CTR_NRATE] = {G0_MW0, G0_MW1, G0_MW2, G0_MW3, G0_MW4, G0_MW5, G0_MW6, G0_MW7, G0_MW8, G0_MW9, G0_MW10, G0_MW11, G0_MW12, G0_MW13, G0_MW14, G0_MW15};
  fill_rates(RS_MATCH, mcw, G0_MW_NMAX, G0_MP_INIT);
  for( int d = -P_MID; d<P_MID; d++ ) {
    int p = int(double(P_ONE)/(1.0+exp(-double(d)/double(ST_SCALE)))+0.5);
    SQT[d+P_MID] = p<P_MIN ? P_MIN : p>P_MAX ? P_MAX : p;
  }
  for( int p = 0; p<P_ONE; p++ ) {
    double q = p<P_MIN ? P_MIN : p>P_MAX ? P_MAX : p;
    int d = int(floor(double(ST_SCALE)*log(q/(double(P_ONE)-q))+0.5));
    STT[p] = short(d<ST_MIN ? ST_MIN : d>ST_MAX ? ST_MAX : d);
#if XAD_STATS
    CLT[p][1] = u16(-double(ST_SCALE)*log2(q/double(P_ONE))+0.5);
    CLT[p][0] = u16(-double(ST_SCALE)*log2(1.0-q/double(P_ONE))+0.5);
#endif
  }
}

static INLINE int squash(int d) {
  return SQT[(d<ST_MIN ? ST_MIN : d>ST_MAX ? ST_MAX : d)+P_MID];
}
