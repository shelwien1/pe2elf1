/*  The pieces a context-mixing coder is made of, after mp3c.

    Four things, in the order one binary decision meets them: a counter that
    remembers how a context has gone, an APM that corrects a counter that is
    right in shape and wrong in confidence, a logistic mixer that weighs two
    or more of those against each other, and the range coder underneath.
    MP3C-ALGORITHM.md describes the same four in the program this borrows
    from; what differs here is only which variables the contexts are built
    from, and those come from TSVTRANS.md's measurements.

    One convention throughout, because the alternative is sign errors: **a
    probability is always the probability that the bit is zero**, and it is
    12 bits wide -- [1, 4095] -- until it reaches the coder, which wants 15.  */

constexpr int CM_PBITS = 12;
constexpr int CM_PONE  = 1 << CM_PBITS;   /*  4096  */
constexpr int CM_PHALF = CM_PONE / 2;
constexpr int CM_STMAX = 2047;            /*  stretch saturates here  */

constexpr int CM_CBITS = 16;
constexpr int CM_CONE  = 1 << CM_CBITS;
constexpr int CM_TMAX  = 4096;            /*  the highest limit upd honours  */

static u16 cm_rec[CM_TMAX + 2];           /*  CM_CONE / (t + 2)  */

static i16 cm_st[CM_PONE];                /*  stretch: p -> log-odds  */
static u16 cm_sq[2 * CM_STMAX + 1];       /*  squash: log-odds -> p  */

static void cm_tables(void) {
  int i;
  for (i = 0; i <= CM_TMAX + 1; i++) cm_rec[i] = (u16) (CM_CONE / (i + 2));
  for (i = -CM_STMAX; i <= CM_STMAX; i++) {
    double v = CM_PONE / (1.0 + exp(-i / 256.0));
    int p = (int) (v + 0.5);
    if (p < 1) p = 1;
    if (p > CM_PONE - 1) p = CM_PONE - 1;
    cm_sq[i + CM_STMAX] = (u16) p;
  }
  /*  the inverse, by walking the squash curve once  */
  { int j = 0;
    for (i = 0; i < CM_PONE; i++) {
      while (j < 2 * CM_STMAX && cm_sq[j] < i) j++;
      cm_st[i] = (i16) (j - CM_STMAX);
    } }
}

static INLINE int cm_squash(int x) {
  if (x <= -CM_STMAX) return cm_sq[0];
  if (x >= CM_STMAX) return cm_sq[2 * CM_STMAX];
  return cm_sq[x + CM_STMAX];
}
static INLINE int cm_stretch(int p) { return cm_st[p]; }

/*  A counter: the probability a context has settled on, and how many times it
    has been asked.  The rate falls as the count rises, so a fresh context
    moves fast and a well-visited one holds still; `lim` is where it stops
    falling, which is what lets a context keep tracking a signal that drifts.

    The probability is held at 16 bits and handed out at 12.  It has to be:
    the update is the running mean, p += (target - p) / (t + 2), and at 12
    bits that quotient truncates to zero once t passes a few hundred -- a
    context that has been visited 4094 times stops moving at all, whatever it
    then sees.  The symptom is not a bad prediction but a frozen one, and it
    cost a factor of two on a file where `lim` was set high enough to reach
    it.  Four extra bits move the freeze out past any limit worth setting.  */
struct cm_cnt {
  u16 p, t;
  void init() { p = (u16) (CM_CONE / 2);  t = 0; }
  INLINE int P() const {
    int v = p >> (CM_CBITS - CM_PBITS);
    return v < 1 ? 1 : v > CM_PONE - 1 ? CM_PONE - 1 : v;
  }
  INLINE void upd(int bit, int lim) {
    int target = bit ? 0 : CM_CONE - 1;
    if (t < lim) t++;
    p = (u16) ((int) p + (((target - (int) p) * cm_rec[t]) >> CM_CBITS));
  }
};

/*  Secondary estimation.  The counter's probability is stretched, the two
    nearest of 33 buckets are interpolated, and the answer is what the bucket
    has actually seen at that confidence in this context -- which is how a
    model that is directionally right stops being overconfident.  */
struct cm_apm {
  u16 * t;
  u32 n;
  int idx;
  void init(u16 * tab, u32 nctx) {
    u32 i, j;
    t = tab;  n = nctx;  idx = 0;
    for (i = 0; i < nctx; i++)
      for (j = 0; j < 33; j++)
        t[i * 33 + j] = (u16) cm_squash(((int) j - 16) * 128);
  }
  INLINE int pp(int pr, u32 ctx) {
    int s = cm_stretch(pr) + CM_STMAX;    /*  [0, 4094]  */
    int w = s & 127;
    idx = (int) (ctx * 33) + (s >> 7);
    return (t[idx] * (128 - w) + t[idx + 1] * w) >> 7;
  }
  INLINE void upd(int bit, int rate) {
    int g = bit ? 0 : CM_PONE;
    t[idx]     = (u16) (t[idx]     + ((g - t[idx])     >> rate));
    t[idx + 1] = (u16) (t[idx + 1] + ((g - t[idx + 1]) >> rate));
  }
};

/*  The logistic mixer: inputs are stretched probabilities, the output is
    their weighted sum squashed back, and the weights move along the gradient
    of the coding error.  One weight set per mixer context.  */
template <int N>
struct cm_mix {
  i32 * w;                                /*  16.16, one row of N per context  */
  int x[N], pr, row;
  int nx;
  void init(i32 * wt, u32 nctx) {
    u32 i;
    w = wt;
    for (i = 0; i < nctx * N; i++) w[i] = (1 << 16) / N;  /*  their mean  */
    nx = 0;  row = 0;  pr = CM_PHALF;
  }
  INLINE void add(int st) { x[nx++] = st; }
  INLINE int mix(u32 ctx) {
    i64 s = 0;
    int i;
    row = (int) ctx * N;
    for (i = 0; i < nx; i++) s += (i64) w[row + i] * x[i];
    { int d = (int) (s >> 16);
      if (d < -CM_STMAX) d = -CM_STMAX;
      if (d > CM_STMAX) d = CM_STMAX;
      pr = cm_squash(d); }
    if (pr < 1) pr = 1;
    if (pr > CM_PONE - 1) pr = CM_PONE - 1;
    return pr;
  }
  INLINE void upd(int bit, int lr) {
    int err = ((bit ? 0 : CM_PONE) - pr) * lr;
    int i;
    for (i = 0; i < nx; i++) w[row + i] += (x[i] * err) >> 16;
    nx = 0;
  }
};
