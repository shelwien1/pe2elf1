// cm_apm.inc -- cm.inc's cm_apm: an interpolated APM, and its count limit by rate.

// an APM's count limit by its rate (1-CM_APM_RATE_MAX)
constexpr int cm_apm_limit(int rate) {
  return rate >= CM_APM_RATE_FULL ? CM_AMAX : (1 << rate) - 2 < 1 ? 1 : (1 << rate) - 2;
}
struct cm_apm {
  i16 *__restrict t;
  u8 *__restrict c;
  int idx, bkt, q;
  u16 ident[CM_QMAX];
  void init(i16 *tab, u8 *cnt, int qq) {
    int j;
    t = tab;
    c = cnt;
    idx = 0;
    bkt = 0;
    q = cm_apm_q(qq);
    for(j = 0; j < q; j++) {
      int x = j * (1 << CM_SBITS) / (q - 1) - CM_STMAX - 1;
      ident[j] = (u16)cm_squash(x);
    }
  }
  INLINE int pp(int pr, u32 ctx) {
    int s = cm_stretch(pr) + CM_STMAX;
    int pos = s * (q - 1);
    int w = (pos & ((1 << CM_SBITS) - 1)) >> (CM_SBITS - CM_APM_WBITS);
    bkt = pos >> CM_SBITS;
    idx = (int)(ctx * (u32)q) + bkt;
    return (((int)ident[bkt] + t[idx]) * ((1 << CM_APM_WBITS) - w) + ((int)ident[bkt + 1] + t[idx + 1]) * w) >>
           CM_APM_WBITS;
  }
  INLINE void upd1(int i, int b, int g, int lim) {
    u8 *__restrict cc = c;
    i16 *__restrict tt = t;
    int k = cc[i];
    int v = (int)ident[b] + tt[i];
    cc[i] = (u8)(k + (k < lim));
    v += ((g - v) * cm_arec[k]) >> CM_CBITS;
    tt[i] = (i16)(v - (int)ident[b]);
  }
  INLINE void upd(int bit, int lim) {
    int g = bit ? 0 : CM_PONE;
    upd1(idx, bkt, g, lim);
    upd1(idx + 1, bkt + 1, g, lim);
  }
};
