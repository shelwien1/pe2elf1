/* xad_mix.inc -- part of xadpcm.cpp; not standalone.
 *
 * Mixer<MLR,WCL> and APM<APN,APR> -- the logistic mixing stages. */

/* MLR is the learning rate and WCL the log2 weight clamp.  Both are read on
   EVERY coded bit -- the clamp was even being recomputed inside upd() -- so
   they earn a template parameter: in the Const build they fold to immediates,
   which is what turns the update loop into two multiplies and a clamp with no
   loads besides the weights themselves. */
template<IDXP(MLR), IDXP(WCL)>
struct Mixer {
  int nin = 0;
  vector<int> w;
  int st[MIX_MAXIN];
  int nx = 0, cx = 0, pr_ = P_MID;
  IDXC(int, lr, PMUL(MLR));
  IDXC(int, cl, 1<<pclamp(WCL, 8, 29));

  void init(int ni, int nctx) {
    nin = ni;
    w.assign(size_t(ni)*size_t(nctx), G0_W_INIT);
  }

  INLINE void add(int s) {
    st[nx++] = s;
  }

  INLINE int mix(int ctx) {
    cx = ctx;
    i64 s = 0;
    int* W = &w[size_t(cx)*size_t(nin)];
    for( int i = 0; i<nx; i++ )
      s += i64(st[i])*W[i];
    pr_ = squash(int(s>>MIX_SH));
    return pr_;
  }

  INLINE void upd(int bit) {
    int err = (bit<<P_BITS)-pr_;
    int* W = &w[size_t(cx)*size_t(nin)];
    for( int i = 0; i<nx; i++ ) {
      int nw = W[i]+int((i64(st[i])*i64(err)*i64(lr))>>MIX_LRSH);
      W[i] = nw<-cl ? -cl : nw>cl ? cl : nw;
    }
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
  vector<u16> t;
  int idx = 0, wt = 0;
  IDXC(int, N, PAPN(APN));
  IDXC(int, ncl, PAPN(APN)+1);
  IDXC(int, rt, PMUL(APR));

  void init(int nctx) {
    t.assign(size_t(nctx)*size_t(ncl), 0);
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
