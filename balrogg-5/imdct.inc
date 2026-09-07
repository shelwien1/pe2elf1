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

/*  Inverse MDCT and the Vorbis window, the two pieces of synthesis that are
    pure arithmetic.

    The IMDCT of M coefficients to 2M samples,

        y[j] = sum_k X[k] cos(pi/M (j + 1/2 + M/2)(k + 1/2)),

    is a DCT-IV of the same M points read back through the DCT-IV symmetries
    (even about -1/2, odd about M-1/2), which is the unpack() below.  The
    DCT-IV itself factors into one M/2-point complex FFT: writing
    D[k] = u[2k] - i u[M-1-2k] and splitting the sum over even and odd inputs
    gives D[k] = sum_n (x[2n] + i x[M-1-2n]) exp(-i pi/M (2k+1/2)(2n+1/2)),
    and the exponent expands to 2pi k n/(M/2) plus a twiddle on each end.  */

constexpr u32 MD_MAXBLK = 8192;         /*  the largest Vorbis block  */
constexpr u32 MD_MAXM   = MD_MAXBLK / 2;

struct imdct {
  /*  One table set per block size: a stream alternates between them from one
      packet to the next, so recomputing on every switch would dominate.  */
  double wr[2][MD_MAXM / 2], wi[2][MD_MAXM / 2];  /*  FFT roots  */
  double tr[2][MD_MAXM / 2], ti[2][MD_MAXM / 2];  /*  twiddles  */
  double zr[MD_MAXM / 2], zi[MD_MAXM / 2];        /*  scratch  */
  double u[MD_MAXM];                              /*  DCT-IV output  */
  float win[2][MD_MAXM];                          /*  rising half-windows  */
  u32 blk[2];

  /*  w[i] = sin(pi/2 sin^2(pi/(2n) (i + 1/2))), the rising half of the
      Vorbis window over a block of 2n.  */
  void setup(u32 b0, u32 b1) {
    u32 s, i;
    blk[0] = b0;  blk[1] = b1;
    for (s = 0; s < 2; s++) {
      u32 n = blk[s] / 2, N = n / 2;
      for (i = 0; i < n; i++) {
        double t = sin(M_PI / (2.0 * n) * (i + 0.5));
        win[s][i] = (float) sin(M_PI / 2.0 * t * t);
      }
      for (i = 0; i < N; i++) {
        double a = -M_PI * (i + 0.125) / n;
        wr[s][i] = cos(-2.0 * M_PI * i / N);
        wi[s][i] = sin(-2.0 * M_PI * i / N);
        tr[s][i] = cos(a);  ti[s][i] = sin(a);
      }
    }
  }

  /*  In-place radix-2 decimation-in-time complex FFT of N points, N a power
      of two, kernel exp(-2 pi i k n / N).  */
  void fft(double * re, double * im, u32 N, u32 s) {
    u32 i, j = 0, len, k, step;
    for (i = 1; i < N; i++) {                 /*  bit reversal  */
      u32 bit = N >> 1;
      for (; j & bit; bit >>= 1) j ^= bit;
      j ^= bit;
      if (i < j) {
        double t = re[i];  re[i] = re[j];  re[j] = t;
        t = im[i];  im[i] = im[j];  im[j] = t;
      }
    }
    for (len = 2; len <= N; len <<= 1) {
      step = N / len;
      for (i = 0; i < N; i += len)
        for (k = 0; k < len / 2; k++) {
          double cr = wr[s][k * step], ci = wi[s][k * step];
          double ar = re[i + k], ai = im[i + k];
          double br = re[i + k + len / 2], bi = im[i + k + len / 2];
          double xr = br * cr - bi * ci, xi = br * ci + bi * cr;
          re[i + k] = ar + xr;  im[i + k] = ai + xi;
          re[i + k + len / 2] = ar - xr;  im[i + k + len / 2] = ai - xi;
        }
    }
  }

  /*  u[] = DCT-IV of x[0..M-1].  */
  void dct4(const float * x, u32 M) {
    u32 n, k, N = M / 2, s = (2 * M == blk[1]);
    for (n = 0; n < N; n++) {
      double a = x[2 * n], b = x[M - 1 - 2 * n];
      zr[n] = a * tr[s][n] - b * ti[s][n];
      zi[n] = a * ti[s][n] + b * tr[s][n];
    }
    fft(zr, zi, N, s);
    for (k = 0; k < N; k++) {
      double dr = zr[k] * tr[s][k] - zi[k] * ti[s][k];
      double di = zr[k] * ti[s][k] + zi[k] * tr[s][k];
      u[2 * k] = dr;  u[M - 1 - 2 * k] = -di;
    }
  }

  /*  2M samples from M coefficients.  */
  void backward(const float * X, float * y, u32 M) {
    u32 j, h = M / 2;
    dct4(X, M);
    for (j = 0; j < h; j++)            y[j] = (float) u[h + j];
    for (j = h; j < M + h; j++)        y[j] = (float) -u[3 * h - 1 - j];
    for (j = M + h; j < 2 * M; j++)    y[j] = (float) -u[j - 3 * h];
  }

  /*  Reference transform, used only by the self test.  */
  static void naive(const float * X, float * y, u32 M) {
    u32 j, k;
    for (j = 0; j < 2 * M; j++) {
      double s = 0;
      for (k = 0; k < M; k++)
        s += X[k] * cos(M_PI / M * (j + 0.5 + M / 2.0) * (k + 0.5));
      y[j] = (float) s;
    }
  }
};
