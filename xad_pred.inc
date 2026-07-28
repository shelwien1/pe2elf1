/* xad_pred.inc -- part of xadpcm.cpp; not standalone.
 *
 * Pred<VAR> -- the cascaded NLMS signal predictor. */

#define WSH pclamp(G0_WSH, 4, 24)                    // weight fixed-point scale
static const int PRED_NMAX = 512;                    // widest stage the lengths may ask for, NOT tunable
static const int PRED_DMAX = pclamp(G0_PRED_DMAX, 1, 1<<20);
static const int PRED_SHMAX = pclamp(G0_PRED_SHMAX, 8, 62);
static_assert(PRED_NMAX%8==0, "the kernels round stage lengths up to a multiple of 8");

/* THE HISTORY IS A NEWEST-FIRST SLIDING WINDOW, not a ring.
 *
 * It used to be a 1024-entry ring indexed `h[(t-1-i)&HM]`, which put a load, an
 * AND and a variable index on every one of ~1500 taps per sample-channel and
 * made the loops unvectorizable: consecutive i touch descending, wrapping
 * addresses.  Here h[0] is the newest value and h[i] is i instants older, so
 * the weights (lag-ascending) and the history are parallel and both loops are
 * contiguous ascending runs -- which is what lets the kernels in xad_simd.inc
 * exist at all.
 *
 * push writes DOWNWARD (*--h), so the window slides toward buf; when it reaches
 * the bottom the live part is copied back up to the top of the slack and the
 * cursor resets.  Amortized cost is n8*4/HSLACK bytes per sample -- about 6
 * bytes at the shipped shapes -- against a per-sample tap count in the
 * hundreds.
 *
 * NOT a mirrored/ghost buffer (the value written at both pos and pos+HN): that
 * keeps the history time-ascending while the weights are lag-ascending, so
 * every SIMD iteration would need a lane reverse, and it doubles the footprint.
 *
 * HSLACK trades that copy against the footprint.  256 puts a history at
 * PRED_NMAX+264 entries = 3.0 KB, which is *less* than the 4 KB ring it
 * replaces, so the working set went down as well. */
enum { HSLACK = 256 };

/* Sized from the stage it serves, not from PRED_NMAX (v4 §2.1).  The fixed
   PRED_NMAX/HN sizing cost 20.6 KB per Pred against a live footprint of a
   fraction of that, and there are four Preds (two cascades x two channels), so
   the predictor alone was 82 KB -- comfortably out of L1 for no benefit.  Sized
   per stage it is 29 KB, which fits alongside the counter rows.
   PRED_NMAX stays the clamp bound on the IDX numbers; it is no longer the
   allocation. */
struct Hist {
  i32* buf = nullptr;
  i32* h = nullptr;
  int n8 = 0;

  size_t bytes() const {
    return size_t(n8+HSLACK+8)*sizeof(i32);
  }
  void alloc(Arena &a, int n8_) {
    n8 = n8_;
    buf = (i32*)a.take(bytes());
  }
  void reset() {
    memset(buf, 0, bytes());
    h = buf+HSLACK;
  }
  // the value about to leave an n-wide window -- read BEFORE the push
  INLINE i32 oldest(int n) const {
    return h[n-1];
  }
  INLINE void push(i32 v) {
    if( h==buf ) {
      memmove(buf+HSLACK, buf, size_t(n8)*sizeof(i32));
      h = buf+HSLACK;
    }
    *--h = v;
  }
};

/* Two cascades run side by side over the same signal, with independently tuned
   shapes, and the mixer decides per file which to believe.  Separate optimizer
   runs kept landing on genuinely different optima -- one long (92/199/213 taps,
   no cross stage) and one short (64/152/17 with a 24-tap cross stage) -- each
   clearly better on some material and clearly worse on other.  That is not a
   tuning ambiguity to be resolved by picking a winner: it says the two shapes
   see different things, so the model should have both.

   VAR selects between them, and it is a TEMPLATE parameter rather than the
   setvar() argument it used to be, because what it selects are the trip counts
   of the four NLMS loops -- and those loops are the hot path of the whole
   codec: two cascades x two channels x ~760 taps per sample, twice (dot then
   adapt).  In the Const build the values below are compile-time constants, so
   every loop has a known bound and the kernels specialise on it; in the Debug
   build they are per-object consts read once, exactly what setvar() left
   behind.  Storage is still sized for PRED_NMAX, the widest set the optimizer
   may ask for.

   VAR is a plain int and not the shape itself: it is 0 or 1, IDXC does the
   selecting below, and thirteen reference parameters in one list would be
   unreadable for no gain. */
template<int VAR>
struct Pred {
  /* Weights and histories both come from the Codec's arena, which hands out
     64B-aligned blocks -- so the kernels' aligned loads on the weights are
     valid.  The history is read unaligned by construction (the cursor moves one
     element per push), which costs nothing unless an access straddles a page. */
  i32 *w1 = nullptr, *w2 = nullptr, *w3 = nullptr, *w4 = nullptr;
  Hist H0, H1, H2, HX;
  u64 en0, en1, en2, enx;
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

  /* Stage lengths rounded up to a whole vector.
     dot() runs to n#p and relies on w[n..n#p) being ZERO: adapt() writes only
     [0,n), reset() clears the whole array, and the lengths never change at run
     time, so those lanes are zero for the life of the object and contribute
     exactly nothing.  That is what removes dot's scalar tail without changing a
     bit of the result. */
  IDXC(int, n1p, (pclamp(VAR ? G0_N1B : G0_N1, 1, PRED_NMAX)+7)&~7);
  IDXC(int, n2p, (pclamp(VAR ? G0_N2B : G0_N2, 1, PRED_NMAX)+7)&~7);
  IDXC(int, n3p, (pclamp(VAR ? G0_N3B : G0_N3, 1, PRED_NMAX)+7)&~7);
  IDXC(int, n4p, (pclamp(VAR ? G0_N4B : G0_N4, 0, PRED_NMAX)+7)&~7);

  void alloc(Arena &a) {
    w1 = (i32*)a.take(size_t(n1p)*sizeof(i32));
    w2 = (i32*)a.take(size_t(n2p)*sizeof(i32));
    w3 = (i32*)a.take(size_t(n3p)*sizeof(i32));
    w4 = (i32*)a.take(size_t(n4p ? n4p : 8)*sizeof(i32));
    H0.alloc(a, n1p);
    H1.alloc(a, n2p);
    H2.alloc(a, n3p);
    HX.alloc(a, n4p);
  }

  void reset(int keepw) {
    if( !keepw ) {
      memset(w1, 0, size_t(n1p)*sizeof(i32));
      memset(w2, 0, size_t(n2p)*sizeof(i32));
      memset(w3, 0, size_t(n3p)*sizeof(i32));
      memset(w4, 0, size_t(n4p ? n4p : 8)*sizeof(i32)); // no prior on the other channel: start at silence
      w1[0] = pw0;               // seeded at r[t-1] + (r[t-1]-r[t-2])
    }
    H0.reset();
    H1.reset();
    H2.reset();
    HX.reset();
    en0 = en1 = en2 = enx = 0;
    xset = 0;
    last = p1 = p2 = p3 = p4 = d0last = 0;
  }

  /* dot and adapt read h[i] for BOTH the own-signal and the cross histories.
     They used to differ by one instant -- h[(t-1-i)] against h[(t-i)] -- but
     that difference was only ever about whether the current instant's value had
     been pushed yet, and with a sliding window the push itself expresses it:
     set_cross pushes before predict reads, the own-signal push happens after.
     So the two pairs of kernels collapse into one pair. */
  INLINE i32 dot(const i32* w, const i32* h, int n8) const {
    return i32(xad_dot(w, h, n8)>>WSH);
  }

  /* What this channel's own three stages predict for its NEXT delta, without
     touching any state.  It is what the channel coded FIRST at an instant has
     to offer the others: its real delta is not available to them yet, but this
     is, and both sides can compute it identically. */
  INLINE i32 predict_own(int depth) const {
    if( depth<1 )
      return 0;
    i32 v = dot(w1, H0.h, n1p);
    if( depth>1 )
      v += dot(w2, H1.h, n2p);
    if( depth>2 )
      v += dot(w3, H2.h, n3p);
    return v;
  }

  // NLMS: step normalised by the window energy, so adaptation does not depend
  // on how loud the passage is.  Power-of-two normalisation keeps it integer.
  INLINE void adapt(i32* w, const i32* h, int n, i32 err, u64 en, int g, int fl) {
    if( !err )
      return;
    /* was: for( u64 e = en+n*fl; e>1; e>>=1 ) s++;  -- i.e. floor(log2(e)), 0
       for e<=1.  The |1 guards clzll(0), which is undefined; the argument is
       never 0 in practice (n and fl are both pclamped to >=1) but the guard is
       free and survives a widened clamp. */
    int sh = 63-__builtin_clzll((en+u64(n)*u64(fl))|1);
    sh += g-WSH;
    sh = sh<0 ? 0 : sh>PRED_SHMAX ? PRED_SHMAX : sh;
    xad_adapt(w, h, n, err, sh);
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
    i32 old = HX.oldest(n4);
    enx += u64(i64(v)*i64(v))-u64(i64(old)*i64(old));
    HX.push(v);
    xset = 1;
  }

  INLINE void push(Hist &H, u64 &en, int n, i32 v) {
    i32 old = H.oldest(n);
    en += u64(i64(v)*i64(v))-u64(i64(old)*i64(old));
    H.push(v);
  }

  // predicted next-sample delta (r^[t] - r[t-1])
  INLINE i32 predict() {
    set_cross(0); // header samples have no cross value; fill the slot
    p1 = dot(w1, H0.h, n1p);
    p2 = dot(w2, H1.h, n2p);
    p3 = dot(w3, H2.h, n3p);
    p4 = n4 ? dot(w4, HX.h, n4p) : 0;
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
    adapt(w1, H0.h, n1, r1, en0, mu1, fl1);
    adapt(w2, H1.h, n2, r2, en1, mu2, fl2);
    adapt(w3, H2.h, n3, r3, en2, mu3, fl3);
    if( n4 )
      adapt(w4, HX.h, n4, r4, enx, mu4, fl4);
    push(H0, en0, n1, d0);
    push(H1, en1, n2, r1);
    push(H2, en2, n3, r2);
    d0last = d0;
    xset = 0;
    last = r;
  }
};
