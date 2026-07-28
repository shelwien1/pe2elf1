/* xad_counter.inc -- part of xadpcm.cpp; not standalone.
 *
 * Ctr / CtrS and the tables of them.  Every rate is a 16-bit fixed-point
 * MULTIPLIER rather than a shift -- see the note below. */

/* The counters, mixer and APM all step by a 16-bit fixed-point MULTIPLIER
   rather than a shift.  mul = 65536>>shift reproduces any shift exactly -- the
   product is exact and the >>16 is the same arithmetic shift, negative deltas
   included -- so this is byte-identical to the shift model at the seeded
   values.  What it buys is the space between the powers of two, which a shift
   schedule can never reach and which is where the rate schedule (the biggest
   knob found by hand) is most likely to still have something left. */

struct Ctr {
  u16 p = CTR_INIT;
  u16 n = 0;

  INLINE int pr() const {
    int v = p>>CTR_SH;
    return v<P_MIN ? P_MIN : v>P_MAX ? P_MAX : v;
  }

  INLINE void upd(int bit, const RateSet &rs) {
    int d = (bit ? CTR_MAX : 0)-int(p);
    p = u16(int(p)+int((i64(d)*i64(rs.cw[n]))>>CTR_BITS));
    if( n<rs.nmax )
      n++;
  }
};

struct CtrS { // slow fixed-rate twin of the primary code counter
  u16 p = CTR_INIT;

  INLINE int pr() const {
    int v = p>>CTR_SH;
    return v<P_MIN ? P_MIN : v>P_MAX ? P_MAX : v;
  }

  INLINE void upd(int bit) {
    int d = (bit ? CTR_MAX : 0)-int(p);
    p = u16(int(p)+int((i64(d)*i64(PMUL(G0_SLOW_MUL)))>>CTR_BITS));
  }
};

struct CtrTab {
  vector<Ctr> v;
  int w = 0;

  void init(int rows, int width, const RateSet &rs) {
    Ctr c;
    c.p = rs.init;
    w = width;
    v.assign(size_t(rows)*size_t(width), c);
  }

  INLINE Ctr*row(int r) {
    return &v[size_t(r)*size_t(w)];
  }
};

struct CtrSTab {
  vector<CtrS> v;
  int w = 0;

  void init(int rows, int width) {
    CtrS c;
    c.p = u16(pclamp(G0_SP_INIT, 1, CTR_MAX));
    w = width;
    v.assign(size_t(rows)*size_t(width), c);
  }

  INLINE CtrS*row(int r) {
    return &v[size_t(r)*size_t(w)];
  }
};
