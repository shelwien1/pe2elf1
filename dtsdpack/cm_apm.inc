// cm_apm.inc -- cm.inc's cm_apm: an interpolated APM, and its count limit by rate.

// an APM's count limit by its rate (1-CM_APM_RATE_MAX)
constexpr int cm_apm_limit(int rate) {
  return rate >= CM_APM_RATE_FULL ? CM_AMAX : (1 << rate) - 2 < 1 ? 1 : (1 << rate) - 2;
}
// an APM's bucket: its value (less the identity's) and its count, side by side (a decision's two
// buckets in one cache line, mostly, and one page: the tables' pages a short stream touches fewer)
struct cm_apme {
  i16 t;
  u8 c, pad;
};
struct cm_apm {
  rp<cm_apme> e;
  int idx, bkt, q;
  u16 ident[CM_QMAX];
  void init(rp<cm_apme> tab, int qq) {
    int j;
    e = tab;
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
    return (((int)ident[bkt] + e[idx].t) * ((1 << CM_APM_WBITS) - w) + ((int)ident[bkt + 1] + e[idx + 1].t) * w) >>
           CM_APM_WBITS;
  }
  INLINE void upd1(int i, int b, int g, int lim) {
    cm_apme& x = e[i];
    int k = x.c;
    int v = (int)ident[b] + x.t;
    x.c = (u8)(k + (k < lim));
    v += ((g - v) * cm_arec[k]) >> CM_CBITS;
    x.t = (i16)(v - (int)ident[b]);
  }
  INLINE void upd(int bit, int lim) {
    int g = bit ? 0 : CM_PONE;
    upd1(idx, bkt, g, lim);
    upd1(idx + 1, bkt + 1, g, lim);
  }
};
