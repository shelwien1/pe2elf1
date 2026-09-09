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
constexpr int CM_AMAX = 255;              /*  where an APM bucket's count stops  */
static u16 cm_arec[CM_AMAX + 2];          /*  65536 / (n + 2), for the APM  */
static u16 cm_mrec[CM_AMAX + 2];          /*  65536 / (n + 1), for the mixer  */

static i16 cm_st[CM_PONE];                /*  stretch: p -> log-odds  */
static u16 cm_sq[2 * CM_STMAX + 1];       /*  squash: log-odds -> p  */
constexpr int CM_QMAX = 130;              /*  points an APM's curve may have  */

static void cm_tables(void) {
  int i;
  for (i = 0; i <= CM_TMAX + 1; i++) cm_rec[i] = (u16) (CM_CONE / (i + 2));
  for (i = 0; i <= CM_AMAX + 1; i++) cm_arec[i] = (u16) (65536 / (i + 2));
  for (i = 0; i <= CM_AMAX + 1; i++) cm_mrec[i] = (u16) (65535 / (i + 1));
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
  /*  All-bits-zero is "never asked", and stands for an even chance -- which
      is what lets the tables go uninitialised.  In the shipping build they
      are one static object, so their pages arrive zeroed from the operating
      system and only the ones a stream reaches are ever faulted in; filling
      a hundred million counters was five times the work of coding a 100 kB
      file.  `t` is the flag rather than `p`, because a counter may legitimately
      settle at p = 0 and must not then be read as fresh: t is zero only
      before the first update, since `lim` is clamped at 1 or more.  */
  INLINE int P() const {
    int v = t ? p >> (CM_CBITS - CM_PBITS) : CM_PHALF;
    return v < 1 ? 1 : v > CM_PONE - 1 ? CM_PONE - 1 : v;
  }
  /*  `mw` bounds the update: the counter runs towards mw and CONE-1-mw
      rather than towards the ends, so a context that has only ever seen one
      value stops short of claiming certainty about the next.  At mw = 0 this
      is the plain running mean it has always been, which is what the .idx
      starts every family at; what the right bound is depends on how much a
      family's contexts are diluted, so it is a parameter and not a constant.  */
  INLINE void upd(int bit, int lim, int mw) {
    int target = bit ? mw : CM_CONE - 1 - mw;
    int cur = t ? (int) p : CM_CONE / 2;
    if (t < lim) t++;
    p = (u16) (cur + (((target - cur) * cm_rec[t]) >> CM_CBITS));
  }
};

/*  Secondary estimation.  The counter's probability is stretched, the two
    nearest of 33 buckets are interpolated, and the answer is what the bucket
    has actually seen at that confidence in this context -- which is how a
    model that is directionally right stops being overconfident.

    The update is the counter's, not a fixed shift.  A shift is the right rate
    for a bucket that has been visited a thousand times and much too slow for
    one that has been visited twice, and an APM under a rich context is mostly
    the second kind: it starts on the identity curve and a fixed 1/128 step
    needs a hundred visits to leave it.  Each bucket carries its own count, so
    it moves at 1/(n+2) until n reaches where the rate would have been, and
    from there on at exactly the rate the .idx asked for.  That is what the
    counters have always done; the APM was the one part still on a constant.  */
struct cm_apm {
  /*  What is stored is each point's distance from the curve the APM starts
      on, not the curve itself, so that an untouched table is an unlearned
      APM and needs no filling.

      How many points that curve has is `q`, and it is a parameter.  33 is
      what this had, and it is far more than the shape of a correction curve
      needs: the points are what the visits are divided between, and an APM
      under a context rich enough to be worth having does not get 33 visits
      per row to spend.  mp3c tunes the same number per SSE and every one of
      its twelve lands between 5 and 8.  At q = 33 the arithmetic below is
      bit for bit what it was.  */
  i16 * t;
  u8  * c;
  u32 n;
  int idx, bkt, q;
  u16 ident[CM_QMAX];
  void init(i16 * tab, u8 * cnt, u32 nctx, int qq) {
    int j;
    t = tab;  c = cnt;  n = nctx;  idx = 0;  bkt = 0;
    q = qq < 3 ? 3 : qq > CM_QMAX ? CM_QMAX : qq;
    /*  point j sits at stretched position j * 4096 / (q - 1)  */
    for (j = 0; j < q; j++) {
      int x = j * 4096 / (q - 1) - CM_STMAX - 1;
      ident[j] = (u16) cm_squash(x);
    }
  }
  /*  s is the stretched probability in [0, 4094]; scaling by q - 1 and
      taking the top bits of a 4096 grid puts point 0 at one end and point
      q - 1 at the other, and never asks for a point past the last.  */
  INLINE int pp(int pr, u32 ctx) {
    int s = cm_stretch(pr) + CM_STMAX;
    int pos = s * (q - 1);
    int w = (pos & 4095) >> 5;
    bkt = pos >> 12;
    idx = (int) (ctx * (u32) q) + bkt;
    return (((int) ident[bkt] + t[idx]) * (128 - w)
          + ((int) ident[bkt + 1] + t[idx + 1]) * w) >> 7;
  }
  INLINE void upd1(int i, int b, int g, int lim) {
    int k = c[i];
    int v = (int) ident[b] + t[i];
    if (k < lim) c[i] = (u8) (k + 1);
    v += ((g - v) * cm_arec[k]) >> 16;
    t[i] = (i16) (v - (int) ident[b]);
  }
  INLINE void upd(int bit, int rate) {
    int g = bit ? 0 : CM_PONE;
    int lim = rate >= 8 ? CM_AMAX : (1 << rate) - 2;
    if (lim < 1) lim = 1;
    upd1(idx, bkt, g, lim);
    upd1(idx + 1, bkt + 1, g, lim);
  }
};

/*  The logistic mixer: inputs are stretched probabilities, the output is
    their weighted sum squashed back, and the weights move along the gradient
    of the coding error.  One weight set per mixer context.  */
template <int N>
struct cm_mix {
  i32 * w;                                /*  16.16, one row of N per context  */
  u8 * c;                                 /*  how often a row has been asked  */
  int x[N], pr, row, ctxn;
  int nx;
  /*  Stored as the distance from the mean the weights start at, for the same
      reason the APM stores a distance from its curve.  */
  enum { W0 = (1 << 16) / N };
  void init(i32 * wt, u8 * cnt, u32 nctx) {
    (void) nctx;
    w = wt;  c = cnt;
    nx = 0;  row = 0;  ctxn = 0;  pr = CM_PHALF;
  }
  INLINE void add(int st) { x[nx++] = st; }
  INLINE int mix(u32 ctx) {
    i64 s = 0;
    int i;
    ctxn = (int) ctx;  row = (int) ctx * N;
    for (i = 0; i < nx; i++) s += (i64) (w[row + i] + W0) * x[i];
    { int d = (int) (s >> 16);
      if (d < -CM_STMAX) d = -CM_STMAX;
      if (d > CM_STMAX) d = CM_STMAX;
      pr = cm_squash(d); }
    if (pr < 1) pr = 1;
    if (pr > CM_PONE - 1) pr = CM_PONE - 1;
    return pr;
  }
  /*  A weight set starts at the mean of its inputs and has to be taught which
      of them to believe; at the rate that suits a well-visited row that takes
      hundreds of decisions, and a mixer context rich enough to be worth
      having is mostly rows that never get hundreds.  `mb` buys the first
      visits a faster rate, decaying as 1/(n+1) to the .idx's own.  At mb = 0
      it is the fixed rate it has always been.  */
  INLINE void upd(int bit, int lr, int mb) {
    int err, i, n;
    if (mb) {
      n = c[ctxn];
      if (n < CM_AMAX) c[ctxn] = (u8) (n + 1);
      lr += (int) (((i64) mb * lr * cm_mrec[n]) >> 16);
    }
    err = ((bit ? 0 : CM_PONE) - pr) * lr;
    for (i = 0; i < nx; i++) w[row + i] += (x[i] * err) >> 16;
    nx = 0;
  }
};
