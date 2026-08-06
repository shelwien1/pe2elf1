/* d5c_mix.inc -- part of dxt5comp_core.inc; not standalone.
 *
 * CtrTab / Mixer / APM -- the mixing machinery.
 * Mixer::mix and Mixer::upd run once per coded bit and are force-inlined. */

//------------------------------------------------------------------- counters / mixer / apm


/* update rules for the counters declared above the generated headers: these
 * need CW[] / G0_SLOW_MUL, which only exist down here */
inline void Ctr::upd(int bit) {
  int d = (bit ? CTR_MAX : 0) - int(p);
  p = u16(int(p) + int(((long long)d * (int)CW[n]) >> CTR_BITS));
  if(n < pclamp(G0_CW_NMAX, 1, CTR_NRATE - 1)) n++;
}
inline void CtrS::upd(int bit) {
  int d = (bit ? CTR_MAX : 0) - int(p);
  p = u16(int(p) + int(((long long)d * PMUL(G0_SLOW_MUL)) >> CTR_BITS));
}

/* CtrTab / Mixer / APM own no storage: each is a VIEW bound to a Table()
 * member of one of the generated %M%_T structs that Codec inherits.  bind()
 * replaces the old init() -- same arguments plus the slab -- and does the same
 * refill, so a rebind is exactly the old reallocation minus the allocator.
 * That matters for --ns, which re-inits the whole model once per .dds file. */
struct CtrTab {
  Ctr* v = nullptr; int w = 0;
  void bind(Ctr* store, int rows, int width) { v = store; w = width; fill_ctr(v, size_t(rows) * width); }
  inline Ctr* row(int r) { return v + size_t(r) * w; }
};

struct Mixer {
  int nin = 0; int* w = nullptr; int st[24]; int nx = 0, cx = 0, pr_ = P_MID, lr = 0;
  void bind(int* store, int ni, int nctx, int lrate) {
    nin = ni; lr = PMUL(lrate); w = store;
    for(size_t i = 0, n = size_t(ni) * nctx; i < n; i++) w[i] = G0_W_INIT;
  }
  inline void add(int s) { st[nx++] = s; }
  /* nx must never exceed nin: the weight rows are strided by nin, so an extra
   * input walks into the NEXT context's row -- and upd() then WRITES there.
   * Silent, and it corrupts a different context's weights rather than
   * crashing.  Adding a tree row without widening the mixer does exactly this. */
  D5C_FORCE_INLINE int mix(int ctx) {
#if D5C_MIXCHK
    if(nx > nin) { fprintf(stderr, "mixer overrun: nx=%d nin=%d\n", nx, nin); abort(); }
#endif
    cx = ctx; long long s = 0; intP W = w + size_t(cx) * nin;
    for(int i = 0; i < nx; i++) s += (long long)st[i] * W[i];
    pr_ = squash(int(s >> (2 * P_BITS - 8)));
    return pr_;
  }
  D5C_FORCE_INLINE void upd(int bit) {
    int err = (bit << P_BITS) - pr_; intP W = w + size_t(cx) * nin;
    for(int i = 0; i < nx; i++) {
      int nw = W[i] + int(((long long)st[i] * err * lr) >> 20);
      int cl = 1 << pclamp(G0_W_CLAMPL, 8, 29);
      W[i] = nw < -cl ? -cl : nw > cl ? cl : nw;
    }
    nx = 0;
  }
};

struct APM {
  u16* t = nullptr; int idx = 0, wt = 0; // wt: P_BITS-wide interpolation fraction
  int N = 32, ncl = 33, rt = 512;        // intervals (any integer), cells, rate mul
  void bind(u16* store, int nctx, int n, int rate) {
    N = PAPN(n); ncl = N + 1; rt = PMUL(rate);
    t = store;
    for(int c = 0; c < nctx; c++)
      for(int j = 0; j < ncl; j++) t[size_t(c) * ncl + j] = u16(squash(j * P_ONE / N - P_MID) << CTR_SH);
  }
  inline int pp(int pr, int cx) {
    int s = STT[pr] + P_MID; // P_MIN..P_MAX
    int sn = s * N, i = sn >> P_BITS, f = sn & P_MAX;
    idx = cx * ncl + i; wt = f;
    return (t[idx] * (P_ONE - f) + t[idx + 1] * f) >> CTR_BITS;
  }
  D5C_FORCE_INLINE void upd(int bit) {
    int g = bit ? CTR_MAX : 0; // rt = CTR_ONE -> full step at full weight
    t[idx] = u16(t[idx] + int(((long long)(g - int(t[idx])) * (P_ONE - wt) * rt) >> (P_BITS + CTR_BITS)));
    t[idx + 1] = u16(t[idx + 1] + int(((long long)(g - int(t[idx + 1])) * wt * rt) >> (P_BITS + CTR_BITS)));
  }
};
