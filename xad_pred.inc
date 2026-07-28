/* xad_pred.inc -- part of xadpcm.cpp; not standalone.
 *
 * Pred<VAR> -- the cascaded NLMS signal predictor. */

#define WSH pclamp(G0_WSH, 4, 24)                    // weight fixed-point scale
static const int HB = 10, HN = 1<<HB, HM = HN-1;     // history ring, NOT tunable
static const int PRED_NMAX = 512;                    // widest stage the lengths may ask for, NOT tunable
static const int PRED_DMAX = pclamp(G0_PRED_DMAX, 1, 1<<20);
static const int PRED_SHMAX = pclamp(G0_PRED_SHMAX, 8, 62);
static_assert(PRED_NMAX<=HN, "a stage cannot be longer than the history ring");

/* Two cascades run side by side over the same signal, with independently tuned
   shapes, and the mixer decides per file which to believe.  Separate optimizer
   runs kept landing on genuinely different optima -- one long (48/192/192 taps)
   and one short (14/113/10 with a wider cross stage) -- each clearly better on
   some material and clearly worse on other.  That is not a tuning ambiguity to
   be resolved by picking a winner: it says the two shapes see different things,
   so the model should have both.

   VAR selects between them, and it is a TEMPLATE parameter rather than the
   setvar() argument it used to be, because what it selects are the trip counts
   of the four NLMS loops -- and those loops are the hot path of the whole
   codec: two cascades x two channels x (n1+n2+n3+n4) taps, twice per sample
   (dot then adapt).  In the Const build the thirteen values below are
   compile-time constants, so every one of those loops has a known bound and
   unrolls; in the Debug build they are per-object consts read once, exactly
   what setvar() left behind.  Storage is still sized for PRED_NMAX, the widest
   set the optimizer may ask for.

   VAR is a plain int and not the shape itself: it is 0 or 1, IDXC does the
   selecting below, and thirteen reference parameters in one list would be
   unreadable for no gain. */
template<int VAR>
struct Pred {
  i32 w1[PRED_NMAX], w2[PRED_NMAX], w3[PRED_NMAX], w4[PRED_NMAX];
  i32 e0[HN], e1[HN], e2[HN], ex[HN];
  u64 en0, en1, en2, enx;
  u32 t;
  i32 last, p1, p2, p3, p4, d0last;
  int xset;

  IDXC(int, n1, pclamp(VAR ? G0_N1B : G0_N1, 1, PRED_NMAX));
  IDXC(int, n2, pclamp(VAR ? G0_N2B : G0_N2, 1, PRED_NMAX));
  IDXC(int, n3, pclamp(VAR ? G0_N3B : G0_N3, 1, PRED_NMAX));
  IDXC(int, n4, pclamp(VAR ? G0_N4B : G0_N4, 0, PRED_NMAX));
  IDXC(int, mu1, VAR ? G0_MU1B : G0_MU1);
  IDXC(int, mu2, VAR ? G0_MU2B : G0_MU2);
  IDXC(int, mu3, VAR ? G0_MU3B : G0_MU3);
  IDXC(int, mu4, VAR ? G0_MU4B : G0_MU4);
  IDXC(int, fl1, pclamp(VAR ? G0_EFL1B : G0_EFL1, 1, 1<<20));
  IDXC(int, fl2, pclamp(VAR ? G0_EFL2B : G0_EFL2, 1, 1<<20));
  IDXC(int, fl3, pclamp(VAR ? G0_EFL3B : G0_EFL3, 1, 1<<20));
  IDXC(int, fl4, pclamp(VAR ? G0_EFL4B : G0_EFL4, 1, 1<<20));
  IDXC(int, pw0, VAR ? G0_PW0B : G0_PW0);

  void reset(int keepw) {
    if( !keepw ) {
      memset(w1, 0, sizeof(w1));
      memset(w2, 0, sizeof(w2));
      memset(w3, 0, sizeof(w3));
      memset(w4, 0, sizeof(w4)); // no prior on the other channel: start at silence
      w1[0] = pw0;               // seeded at r[t-1] + (r[t-1]-r[t-2])
    }
    memset(ex, 0, sizeof(ex));
    memset(e0, 0, sizeof(e0));
    memset(e1, 0, sizeof(e1));
    memset(e2, 0, sizeof(e2));
    en0 = en1 = en2 = enx = 0;
    t = 0;
    xset = 0;
    last = p1 = p2 = p3 = p4 = d0last = 0;
  }

  INLINE i32 dot(const i32* w, const i32* h, int n) const {
    i64 s = 0;
    for( int i = 0; i<n; i++ )
      s += i64(w[i])*i64(h[(t-1-u32(i))&HM]);
    return i32(s>>WSH);
  }

  /* What this channel's own three stages predict for its NEXT delta, without
     touching any state.  It is what the channel coded FIRST at an instant has
     to offer the others: its real delta is not available to them yet, but this
     is, and both sides can compute it identically. */
  INLINE i32 predict_own(int depth) const {
    if( depth<1 )
      return 0;
    i32 v = dot(w1, e0, n1);
    if( depth>1 )
      v += dot(w2, e1, n2);
    if( depth>2 )
      v += dot(w3, e2, n3);
    return v;
  }

  INLINE i32 dotx(const i32* w, const i32* h, int n) const {
    i64 s = 0;
    for( int i = 0; i<n; i++ )
      s += i64(w[i])*i64(h[(t-u32(i))&HM]);
    return i32(s>>WSH);
  }

  INLINE void adaptx(i32* w, const i32* h, int n, i32 err, u64 en, int g, int fl) {
    if( !err )
      return;
    /* was: for( u64 e = en+n*fl; e>1; e>>=1 ) sh++;  -- i.e. floor(log2(e)),
       0 for e<=1.  The |1 guards clzll(0), which is undefined; the argument is
       never 0 in practice (n and fl are both pclamped to >=1) but the guard is
       free and survives a widened clamp. */
    int sh = 63-__builtin_clzll((en+u64(n)*u64(fl))|1);
    sh += g-WSH;
    sh = sh<0 ? 0 : sh>PRED_SHMAX ? PRED_SHMAX : sh;
    for( int i = 0; i<n; i++ )
      w[i] += i32((i64(err)*i64(h[(t-u32(i))&HM]))>>sh);
  }

  /* The other channel's most recent delta, filed at this channel's own time
     index.  Which sample that is falls out of the coding order for free: the
     channel coded second at an instant sees the first one's delta from the
     SAME instant, the one coded first sees the previous instant's -- exactly
     what causality allows, with no special case.  In MS stereo that same-instant
     value is what a byte-aligned general-purpose coder gets for nothing, since
     a byte there is one nibble from each channel. */
  INLINE void set_cross(i32 v) {
    if( !n4||xset )
      return;
    i32 old = ex[(t-u32(n4))&HM];
    enx += u64(i64(v)*i64(v))-u64(i64(old)*i64(old));
    ex[t&HM] = v;
    xset = 1;
  }

  // NLMS: step normalised by the window energy, so adaptation does not depend
  // on how loud the passage is.  Power-of-two normalisation keeps it integer.
  INLINE void adapt(i32* w, const i32* h, int n, i32 err, u64 en, int g, int fl) {
    if( !err )
      return;
    int s = 63-__builtin_clzll((en+u64(n)*u64(fl))|1);   // see adaptx
    int sh = s+g-WSH;
    if( sh<0 )
      sh = 0;
    else if( sh>PRED_SHMAX )
      sh = PRED_SHMAX;
    for( int i = 0; i<n; i++ ) {
      i32 x = h[(t-1-u32(i))&HM];
      w[i] += i32((i64(err)*i64(x))>>sh);
    }
  }

  INLINE void push(i32* h, u64 &en, int n, i32 v) {
    i32 old = h[(t-u32(n))&HM];
    en += u64(i64(v)*i64(v))-u64(i64(old)*i64(old));
    h[t&HM] = v;
  }

  // predicted next-sample delta (r^[t] - r[t-1])
  INLINE i32 predict() {
    set_cross(0); // header samples have no cross value; fill the slot
    p1 = dot(w1, e0, n1);
    p2 = dot(w2, e1, n2);
    p3 = dot(w3, e2, n3);
    p4 = n4 ? dotx(w4, ex, n4) : 0;
    i32 d = p1+p2+p3+p4;
    return d<-PRED_DMAX ? -PRED_DMAX : d>PRED_DMAX ? PRED_DMAX : d;
  }

  INLINE i32 predicted_sample() {
    i32 v = last+predict();
    return v<-32768 ? -32768 : v>32767 ? 32767 : v;
  }

  // r = the sample that actually turned up
  INLINE void update(i32 r) {
    set_cross(0);
    i32 d0 = r-last, r1 = d0-p1, r2 = r1-p2, r3 = r2-p3, r4 = r3-p4;
    adapt(w1, e0, n1, r1, en0, mu1, fl1);
    adapt(w2, e1, n2, r2, en1, mu2, fl2);
    adapt(w3, e2, n3, r3, en2, mu3, fl3);
    if( n4 )
      adaptx(w4, ex, n4, r4, enx, mu4, fl4);
    push(e0, en0, n1, d0);
    push(e1, en1, n2, r1);
    push(e2, en2, n3, r2);
    d0last = d0;
    t++;
    xset = 0;
    last = r;
  }
};
