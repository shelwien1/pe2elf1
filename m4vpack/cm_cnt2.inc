// cm_cnt2.inc -- cm.inc's cm_cnt2: a counter with a second, fast probability.

// a counter of a family's main tables: the adaptive-rate probability of
// cm_cnt and one more with a fixed fast rate (1/8)
struct cm_cnt2 {
  u16 p, t, q;
  INLINE int P() const {
    int v = t ? p >> (CM_CBITS - CM_PBITS) : CM_PHALF;
    return v < 1 ? 1 : v > CM_PONE - 1 ? CM_PONE - 1 : v;
  }
  INLINE int Q() const {
    int v = t ? q >> (CM_CBITS - CM_PBITS) : CM_PHALF;
    return v < 1 ? 1 : v > CM_PONE - 1 ? CM_PONE - 1 : v;
  }
  INLINE void upd(int bit, int lim, int mw) {
    int target = bit ? mw : CM_CONE - 1 - mw;
    int cur = t ? (int)p : CM_CONE / 2, cq = t ? (int)q : CM_CONE / 2;
    t = (u16)(t + (t < lim));
    p = (u16)(cur + (((target - cur) * cm_rec[t]) >> CM_CBITS));
    q = (u16)(cq + ((target - cq) >> CM_FAST_SHIFT));
  }
};
