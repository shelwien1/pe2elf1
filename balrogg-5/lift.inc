/*  Copyright (C) 2026 Kamila Szewczyk

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.  */

/*  Fixed-point integer lifting, after mp3synth's primitives.

    Spectral and time values are i64 with F fractional bits: the integer X
    stands for X / 2^F.  Every rotation is realised as three reversible shears,
    so rot and irot are exact inverses whatever the rounding does -- the shear
    reads a value that the next shear has not yet touched, so the rounding
    error is reproduced identically on the way back.  Butterflies are exact
    integer sums and differences and invert through the parity of a+b.

    F only controls fidelity, never reversibility.  */

typedef int64_t i64;
typedef int8_t  i8;
constexpr int F = 32;

static inline i64 _rnd(i64 x) {            /*  round half away from zero  */
  i64 h = (i64) 1 << (F - 1);
  return x >= 0 ? (x + h) >> F : -(((-x) + h) >> F);
}

/*  round(C*x / 2^F) with a 128-bit intermediate: C and x are both full-range
    fixed point, so the product does not fit in 64 bits.  */
/*  Which way the shears round.

    A three-shear rotation inverts exactly whatever the rounding does -- each
    shear reads a value the next has not touched yet -- so the rule is free to
    choose.  It is not free of consequence: it moves the synthesized PCM by
    well under a sample step, which moves the recovered residue, which moves
    how many digits have to be corrected.  Mode c tries each rule and keeps
    whichever leaves the least to carry; the choice travels in the meta,
    because both directions must round alike or the transform stops being an
    inverse of itself.

      0  nearest, half away from zero      2  toward +infinity
      1  toward -infinity                  3  toward zero  */
static int lift_rnd = 0;

static inline i64 pmul(i64 x, i64 C) {
  __int128 p = (__int128) x * (__int128) C;
  switch (lift_rnd) {
    case 1:  return (i64) (p >> F);                 /*  arithmetic shift floors  */
    case 2:  return (i64) (-((-p) >> F));
    case 3:  return (i64) (p >= 0 ? p >> F : -((-p) >> F));
    default: { __int128 h = (__int128) 1 << (F - 1);
               return (i64) (p >= 0 ? (p + h) >> F : -(((-p) + h) >> F)); }
  }
}

/*  Givens rotation by +theta as three shears.  Ct = tan(theta/2), Cs = sin
    theta.

    tan(theta/2) runs away as theta approaches +-pi -- it reaches 652 for the
    outer FFT roots at N = 2048 -- and every shear multiplies its own rounding
    by that constant.  So each angle is first reduced into [-pi/4, pi/4] by
    whole quarter turns, which are exact signed swaps costing no precision,
    leaving |Ct| <= tan(pi/8) = 0.414.  */
static inline void quarter(i64 & a, i64 & b, int q) {
  i64 t;
  switch (q & 3) {
    case 1: t = a;  a = -b;  b = t;  break;      /*  R(+pi/2)  */
    case 2: a = -a;  b = -b;  break;             /*  R(pi)  */
    case 3: t = a;  a = b;  b = -t;  break;      /*  R(-pi/2)  */
    default: break;
  }
}
static inline void rot(i64 & a, i64 & b, i64 Ct, i64 Cs, int q) {
  quarter(a, b, q);
  a -= pmul(b, Ct);
  b += pmul(a, Cs);
  a -= pmul(b, Ct);
}
static inline void irot(i64 & a, i64 & b, i64 Ct, i64 Cs, int q) {
  a += pmul(b, Ct);
  b -= pmul(a, Cs);
  a += pmul(b, Ct);
  quarter(a, b, -q);
}

static inline i64 fx(double v) {
  double s = v * (double) ((i64) 1 << F);
  return (i64) (s >= 0 ? s + 0.5 : s - 0.5);
}

/*  A DCT-IV of M points, built from one M/2-point complex FFT.  DCT-IV is its
    own inverse up to the scale M/2, so one routine serves both directions:
    the analysis takes (2/M)*dct4, the synthesis takes dct4 as it stands.

    The twiddles are rotations, so they lift.  The FFT butterfly is
    (a+t, a-t), whose sum and difference are both even, so it inverts exactly
    with no rounding at all.  */
struct lifted_dct4 {
  static constexpr u32 MAXM = 4096;
  /*  per size: rotation constants for the FFT roots and the DCT twiddles  */
  i64 wt[2][MAXM / 2], ws[2][MAXM / 2];     /*  FFT root, as (tan, sin)  */
  i8  wq[2][MAXM / 2];                      /*  ... and its quarter turns  */
  i64 tt[2][MAXM / 2], ts[2][MAXM / 2];     /*  DCT twiddle  */
  i8  tq[2][MAXM / 2];
  i64 zr[MAXM / 2], zi[MAXM / 2];
  u32 sz[2];

  /*  Split the angle into whole quarter turns plus a residual in
      [-pi/4, pi/4], and give the residual as its two shear constants.  */
  static void angle(double a, i64 & ct, i64 & cs, i8 & q) {
    double r = a / (M_PI / 2);
    int n = (int) (r >= 0 ? r + 0.5 : r - 0.5);
    a -= n * (M_PI / 2);
    q = (i8) (n & 3);
    ct = fx(tan(a / 2));  cs = fx(sin(a));
  }

  void setup(u32 M0, u32 M1) {
    u32 s, i;
    sz[0] = M0;  sz[1] = M1;
    for (s = 0; s < 2; s++) {
      u32 M = sz[s], N = M / 2;
      for (i = 0; i < N; i++) {
        angle(-2.0 * M_PI * i / N, wt[s][i], ws[s][i], wq[s][i]);
        angle(-M_PI * (i + 0.125) / M, tt[s][i], ts[s][i], tq[s][i]);
      }
    }
  }

  int slot(u32 M) const { return M == sz[1]; }

  void fft(i64 * re, i64 * im, u32 N, int s) {
    u32 i, j = 0, len, k, step;
    for (i = 1; i < N; i++) {
      u32 bit = N >> 1;
      for (; j & bit; bit >>= 1) j ^= bit;
      j ^= bit;
      if (i < j) { i64 t = re[i];  re[i] = re[j];  re[j] = t;
                   t = im[i];  im[i] = im[j];  im[j] = t; }
    }
    for (len = 2; len <= N; len <<= 1) {
      step = N / len;
      for (i = 0; i < N; i += len)
        for (k = 0; k < len / 2; k++) {
          i64 br = re[i + k + len / 2], bi = im[i + k + len / 2];
          i64 ar = re[i + k], ai = im[i + k];
          rot(br, bi, wt[s][k * step], ws[s][k * step], wq[s][k * step]);
          re[i + k] = ar + br;  im[i + k] = ai + bi;
          re[i + k + len / 2] = ar - br;  im[i + k + len / 2] = ai - bi;
        }
    }
  }

  /*  The inverse of fft(): undo the stages in reverse, recovering each
      butterfly through the parity of the sum and difference.  */
  void ifft(i64 * re, i64 * im, u32 N, int s) {
    u32 i, j = 0, len, k, step;
    for (len = N; len >= 2; len >>= 1) {
      step = N / len;
      for (i = 0; i < N; i += len)
        for (k = 0; k < len / 2; k++) {
          i64 pr = re[i + k], pi = im[i + k];
          i64 qr = re[i + k + len / 2], qi = im[i + k + len / 2];
          i64 ar = (pr + qr) / 2, ai = (pi + qi) / 2;
          i64 br = (pr - qr) / 2, bi = (pi - qi) / 2;
          irot(br, bi, wt[s][k * step], ws[s][k * step], wq[s][k * step]);
          re[i + k] = ar;  im[i + k] = ai;
          re[i + k + len / 2] = br;  im[i + k + len / 2] = bi;
        }
    }
    for (i = 1; i < N; i++) {
      u32 bit = N >> 1;
      for (; j & bit; bit >>= 1) j ^= bit;
      j ^= bit;
      if (i < j) { i64 t = re[i];  re[i] = re[j];  re[j] = t;
                   t = im[i];  im[i] = im[j];  im[j] = t; }
    }
  }

  void dct4(const i64 * x, i64 * u, u32 M) {
    u32 n, k, N = M / 2;
    int s = slot(M);
    for (n = 0; n < N; n++) {
      zr[n] = x[2 * n];  zi[n] = x[M - 1 - 2 * n];
      rot(zr[n], zi[n], tt[s][n], ts[s][n], tq[s][n]);
    }
    fft(zr, zi, N, s);
    for (k = 0; k < N; k++) {
      rot(zr[k], zi[k], tt[s][k], ts[s][k], tq[s][k]);
      u[2 * k] = zr[k];  u[M - 1 - 2 * k] = -zi[k];
    }
  }

  /*  2M time samples from M spectral values: a DCT-IV read back through its
      own symmetries (even about -1/2, odd about M-1/2).  */
  void backward(const i64 * X, i64 * y, u32 M) {
    u32 j, h = M / 2;
    static i64 u[MAXM];
    dct4(X, u, M);
    for (j = 0; j < h; j++)         y[j] = u[h + j];
    for (j = h; j < M + h; j++)     y[j] = -u[3 * h - 1 - j];
    for (j = M + h; j < 2 * M; j++) y[j] = -u[j - 3 * h];
  }

  /*  Exact inverse of dct4 as a map, used only by the self test: the real
      inverse transform is (2/M)*dct4.  */
  void idct4(const i64 * u, i64 * x, u32 M) {
    u32 n, k, N = M / 2;
    int s = slot(M);
    for (k = 0; k < N; k++) {
      zr[k] = u[2 * k];  zi[k] = -u[M - 1 - 2 * k];
      irot(zr[k], zi[k], tt[s][k], ts[s][k], tq[s][k]);
    }
    ifft(zr, zi, N, s);
    for (n = 0; n < N; n++) {
      irot(zr[n], zi[n], tt[s][n], ts[s][n], tq[s][n]);
      x[2 * n] = zr[n];  x[M - 1 - 2 * n] = zi[n];
    }
  }
};
