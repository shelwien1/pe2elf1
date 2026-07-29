/* xad_mix.inc -- part of xadpcm.cpp; not standalone.
 *
 * Mixer<MLR,WCL> and APM<APN,APR> -- the logistic mixing stages. */

/* MLR is the learning rate and WCL the log2 weight clamp.  Both are read on
   EVERY coded bit -- the clamp was even being recomputed inside upd() -- so
   they earn a template parameter: in the Const build they fold to immediates,
   which is what turns the update loop into two multiplies and a clamp with no
   loads besides the weights themselves. */
/* THE PRODUCT err*lr FITS IN i32, so the mixer update needs no operand split.
   err = (bit<<P_BITS) - pr_ with pr_ from squash(), which is clamped to
   [P_MIN,P_MAX] = [1,4095], so |err| <= 4095; lr is PMUL()-clamped to <= 65535.
   4095*65535 = 268,364,025, a factor of eight inside i32.  And
   i64(st)*i64(err)*i64(lr) == i64(st)*i64(err*lr) exactly, because err*lr is
   itself exact.  So one mul_epi32 per lane does what the plan budgeted four
   for.  Asserted rather than asserted-in-prose, since both bounds come from the
   arithmetic contract and a widened PMUL clamp would break it: */
static_assert(4095LL*65535LL<2147483647LL, "err*lr must fit i32 for the mixer kernel");

template<IDXP(MLR), IDXP(WCL)>
struct Mixer {
  int* w = nullptr;
  ALIGN(64) int st[MIX_MAXIN];
  int nx = 0, cx = 0, pr_ = P_MID;
  IDXC(int, lr, PMUL(MLR));
  IDXC(int, cl, 1<<pclamp(WCL, 8, 29));

  void init(Arena &a, int nctx) {
    memset(st, 0, sizeof(st));       // st[nx..MIX_STRIDE) must stay zero
    size_t n = size_t(MIX_STRIDE)*size_t(nctx);
    w = (int*)a.take(n*sizeof(int));
    if( w )
      for( size_t i = 0; i<n; i++ )
        w[i] = G0_W_INIT;
  }

  INLINE void add(int s) {
    st[nx++] = s;
  }

  INLINE int mix(int ctx) {
    cx = ctx;
    intP W = w+size_t(cx)*size_t(MIX_STRIDE);
    pr_ = squash(int(xad_dot(W, st, MIX_STRIDE)>>MIX_SH));
    return pr_;
  }

  INLINE void upd(int bit) {
    int err = (bit<<P_BITS)-pr_;
    intP W = w+size_t(cx)*size_t(MIX_STRIDE);
    xad_mixupd(W, st, MIX_STRIDE, err*lr, MIX_LRSH, cl);
    nx = 0;
  }
};

/* The interpolation fraction is P_BITS wide whatever N is, so the interval
   count stops being tied to a power of two: N = 32 gives the old 33 cells and
   the old >>7 / &127 split exactly, and anything else is now reachable. */
/* Two instances differing only in their IDX numbers, and pp() runs on every
   coded bit with ncl as a multiplier -- so the numbers come in as template
   parameters and the multiply folds. */
template<IDXP(APN), IDXP(APR)>
struct APM {
  /* Arena-allocated like everything else in the model: it was the last
     std::vector on the coding path, and being a vector meant it was fully
     allocated AND filled twice at startup -- once in the arena's dry sizing
     pass, which hands out nothing but still ran this. */
  u16* t = nullptr;
  int idx = 0, wt = 0;
  IDXC(int, N, PAPN(APN));
  IDXC(int, ncl, PAPN(APN)+1);
  IDXC(int, rt, PMUL(APR));

  void init(Arena &a, int nctx) {
    t = (u16*)a.take(size_t(nctx)*size_t(ncl)*sizeof(u16));
    if( !t )
      return;
    for( int c = 0; c<nctx; c++ )
      for( int j = 0; j<ncl; j++ )
        t[size_t(c)*size_t(ncl)+size_t(j)] = u16(squash(j*P_ONE/N-P_MID)<<CTR_SH);
  }

  INLINE int pp(int pr, int cx) {
    int s = STT[pr]+P_MID;
    int sn = s*N, i = sn>>P_BITS, f = sn&P_MAX;
    idx = cx*ncl+i;
    wt = f;
    return (t[idx]*(P_ONE-f)+t[idx+1]*f)>>CTR_BITS;
  }

  INLINE void upd(int bit) {
    int g = bit ? CTR_MAX : 0; // rt = 65536 -> full step at full weight
    t[idx] = u16(t[idx]+int((i64(g-int(t[idx]))*i64(P_ONE-wt)*i64(rt))>>(P_BITS+CTR_BITS)));
    t[idx+1] = u16(t[idx+1]+int((i64(g-int(t[idx+1]))*i64(wt)*i64(rt))>>(P_BITS+CTR_BITS)));
  }
};
