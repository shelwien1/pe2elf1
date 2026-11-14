// flacdump: libFLAC LPC analysis replica: autocorrelation variants, Levinson-Durbin, best order, quantization
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// autocorrelation variants
enum { AC_DBL = 0, AC_FASC, AC_FNEW8, AC_FNEW12, AC_FNEW16, AC_X87, AC_FFMA, AC_DDESC, NAC };

static void autoc_compute(const float* d, int n, int lags, int mode, double* out) {
    if (mode == AC_DDESC) {
        // libFLAC 1.4 SSE2 / NEON intrinsics: samples processed from the end
        double ac[33];
        for (int c = 0; c < lags; c++) ac[c] = 0.0;
        for (int i = n - 1; i >= 0; i--) {
            double di = d[i];
            int m = Min(lags, n - i);
            for (int c = 0; c < m; c++) ac[c] += di * (double)d[i + c];
        }
        for (int c = 0; c < lags; c++) out[c] = ac[c];
    } else if (mode == AC_DBL) {
        double ac[33];
        for (int c = 0; c < lags; c++) ac[c] = 0.0;
        for (int i = 0; i < n; i++) {
            double di = d[i];
            int m = Min(lags, n - i);
            for (int c = 0; c < m; c++) ac[c] += di * (double)d[i + c];
        }
        for (int c = 0; c < lags; c++) out[c] = ac[c];
    } else if (mode == AC_FASC) {
        float ac[33];
        for (int c = 0; c < lags; c++) ac[c] = 0.0f;
        for (int i = 0; i < n; i++) {
            float di = d[i];
            int m = Min(lags, n - i);
            for (int c = 0; c < m; c++) ac[c] += di * d[i + c];
        }
        for (int c = 0; c < lags; c++) out[c] = ac[c];
    } else if (mode == AC_FFMA) {
        // generic C code compiled with fused multiply-add (e.g. arm64)
        float ac[33];
        for (int c = 0; c < lags; c++) ac[c] = 0.0f;
        for (int i = 0; i < n; i++) {
            float di = d[i];
            int m = Min(lags, n - i);
            for (int c = 0; c < m; c++) ac[c] = fmaf(di, d[i + c], ac[c]);
        }
        for (int c = 0; c < lags; c++) out[c] = ac[c];
    } else if (mode == AC_X87) {
        // x87 asm (libFLAC <= 1.3 ia32), precision control = double
        float ac[33];
        for (int c = 0; c < lags; c++) ac[c] = 0.0f;
        for (int i = 0; i < n; i++) {
            double di = d[i];
            int m = Min(lags, n - i);
            for (int c = 0; c < m; c++) ac[c] = (float)(di * (double)d[i + c] + (double)ac[c]);
        }
        for (int c = 0; c < lags; c++) out[c] = ac[c];
    } else {
        int N = mode == AC_FNEW8 ? 8 : mode == AC_FNEW12 ? 12 : 16;
        float ac[16];
        for (int c = 0; c < N; c++) ac[c] = 0.0f;
        int limit = n - N;
        int i;
        for (i = 0; i <= limit; i++) {
            float di = d[i];
            for (int c = 0; c < N; c++) ac[c] += d[i + c] * di;
        }
        limit++;
        if (limit < 0) limit = 0;
        for (i = n - 1; i >= limit; i--) {
            float di = d[i];
            for (int c = 0; c < N && i + c < n; c++) ac[c] += di * d[i + c];
        }
        for (int c = 0; c < lags; c++) out[c] = c < N ? ac[c] : 0.0;
    }
}

// libFLAC FLAC__lpc_compute_lp_coefficients: all orders 1..max_order.
// lp[i*32 + j] = coefficient j of order i+1. Returns the usable max order.
// fm: emulate a build with fused multiply-add contraction.
static int levinson_all(const double* autoc, int max_order, float* lp, double* error, bool fm) {
    double err = autoc[0], lpc[32];
    if (err == 0.0) return 0;
    for (int i = 0; i < max_order; i++) {
        double r = -autoc[i + 1];
        int j;
        if (fm) {
            for (j = 0; j < i; j++) r = fma(-lpc[j], autoc[i - j], r);
        } else {
            for (j = 0; j < i; j++) r -= lpc[j] * autoc[i - j];
        }
        r /= err;
        lpc[i] = r;
        if (fm) {
            for (j = 0; j < (i >> 1); j++) {
                double tmp = lpc[j];
                lpc[j] = fma(r, lpc[i - 1 - j], lpc[j]);
                lpc[i - 1 - j] = fma(r, tmp, lpc[i - 1 - j]);
            }
            if (i & 1) lpc[j] = fma(lpc[j], r, lpc[j]);
            err *= fma(-r, r, 1.0);
        } else {
            for (j = 0; j < (i >> 1); j++) {
                double tmp = lpc[j];
                lpc[j] += r * lpc[i - 1 - j];
                lpc[i - 1 - j] += r * tmp;
            }
            if (i & 1) lpc[j] += lpc[j] * r;
            err *= (1.0 - r * r);
        }
        for (j = 0; j <= i; j++) lp[i * 32 + j] = (float)(-lpc[j]);
        error[i] = err;
        if (err == 0.0) return i + 1;
    }
    return max_order;
}

// FLAC__lpc_compute_best_order
static int lpc_best_order(const double* lpc_error, int max_order, int total_samples, int overhead) {
    double error_scale = err_scale(total_samples);
    int best = 0;
    double best_bits = 4294967295.0;
    for (int i = 0; i < max_order; i++) {
        int order = i + 1;
        double e = lpc_error[i], bps;
        if (e > 0.0) {
            bps = 0.5 * det_log2(error_scale * e);
            if (bps < 0.0) bps = 0.0;
        } else if (e < 0.0)
            bps = 1e32;
        else
            bps = 0.0;
        double bits = bps * (double)(total_samples - order) + (double)(order * overhead);
        if (bits < best_bits) {
            best = i;
            best_bits = bits;
        }
    }
    return best + 1;
}

// libFLAC FLAC__lpc_quantize_coefficients
static bool lpc_quantize(const float* lp, int order, int precision, s32* q, int* shift) {
    precision--;
    s32 qmax = 1 << precision, qmin = -qmax;
    qmax--;
    double cmax = 0.0;
    for (int i = 0; i < order; i++) {
        double d = fabs((double)lp[i]);
        if (d > cmax) cmax = d;
    }
    if (cmax <= 0.0) return false;
    int log2cmax;
    (void)frexp(cmax, &log2cmax);
    log2cmax--;
    int sh = precision - log2cmax - 1;
    if (sh > 15)
        sh = 15;
    else if (sh < -16)
        return false;
    double error = 0.0;
    if (sh >= 0) {
        float mul = (float)(1 << sh);
        for (int i = 0; i < order; i++) {
            error += (double)(lp[i] * mul);
            s32 v = (s32)lround(error);
            if (v > qmax)
                v = qmax;
            else if (v < qmin)
                v = qmin;
            error -= v;
            q[i] = v;
        }
    } else {
        float div = (float)(1 << -sh);
        for (int i = 0; i < order; i++) {
            error += (double)(lp[i] / div);
            s32 v = (s32)lround(error);
            if (v > qmax)
                v = qmax;
            else if (v < qmin)
                v = qmin;
            error -= v;
            q[i] = v;
        }
        sh = 0;
    }
    *shift = sh;
    return true;
}

// quantize at a given shift (used as a guess when no candidate matches)
static void lpc_quantize_at(const float* lp, int order, int precision, int sh, s32* q) {
    s32 qmax = (1 << (precision - 1)) - 1, qmin = -(1 << (precision - 1));
    double error = 0.0;
    float mul = (float)(1 << sh);
    for (int i = 0; i < order; i++) {
        error += (double)(lp[i] * mul);
        s32 v = (s32)lround(error);
        if (v > qmax)
            v = qmax;
        else if (v < qmin)
            v = qmin;
        error -= v;
        q[i] = v;
    }
}
