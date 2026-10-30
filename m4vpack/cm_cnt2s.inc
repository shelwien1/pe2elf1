// cm_cnt2s.inc -- cm.inc's cm_cnt2s: cm_cnt2 keeping its probabilities stretched.

// cm_cnt2 with its two probabilities kept stretched, written at each update:
// a decision reads them with the counter instead of looking them up on its
// way to the mixer (the counters of the levels whose families have four)
struct cm_cnt2s {
  cm_cnt2 c;
  i16 sp, sq;  // cm_stretch(P()), cm_stretch(Q())
  INLINE int P() const { return c.P(); }
  INLINE int Q() const { return c.Q(); }
  INLINE void upd(int bit, int lim, int mw) {
    c.upd(bit, lim, mw);
    sp = cm_st[c.P()];
    sq = cm_st[c.Q()];
  }
};
