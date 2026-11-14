// flacdump: ffmpeg flacenc LPC analysis replica (welch window, autocorrelation, Levinson, quantizer)
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// ---- ffmpeg flacenc (levinson) analysis ----
static void ff_welch(const s64* x, int len, double* w) {
    if (len == 1) {
        w[0] = 0.0;
        return;
    }
    int n2 = len >> 1;
    double c = 2.0 / (len - 1.0);
    if (len & 1) {
        for (int i = 0; i < n2; i++) {
            double ww = c - i - 1.0;
            ww = 1.0 - (ww * ww);
            w[i] = (double)x[i] * ww;
            w[len - 1 - i] = (double)x[len - 1 - i] * ww;
        }
        w[n2] = 0.0;
        return;
    }
    for (int i = 0; i < n2; i++) {
        double ww = c - n2 + i;
        ww = 1.0 - (ww * ww);
        w[n2 - i - 1] = (double)x[n2 - i - 1] * ww;
        w[n2 + i] = (double)x[n2 + i] * ww;
    }
}
// d must have zero padding at d[-1] and d[len]
static void ff_autocorr(const double* d, int len, int lag, double* autoc) {
    int i, j;
    for (j = 0; j < lag; j += 2) {
        double sum0 = 1.0, sum1 = 1.0;
        for (i = j; i < len; i++) {
            sum0 += d[i] * d[i - j];
            sum1 += d[i] * d[i - j - 1];
        }
        autoc[j] = sum0;
        autoc[j + 1] = sum1;
    }
    if (j == lag) {
        double sum = 1.0;
        for (i = j - 1; i < len; i += 2) sum += d[i] * d[i - j] + d[i + 1] * d[i - j + 1];
        autoc[j] = sum;
    }
}
// compute_lpc_coefs(normalize=1, fail=0); lpc[i*32+j]
static void ff_levinson(const double* autoc, int max_order, double* lpc) {
    double err = autoc[0];
    const double* ac = autoc + 1;
    for (int i = 0; i < max_order; i++) {
        double r = -ac[i];
        const double* last = lpc + (i ? (i - 1) * 32 : 0);
        double* cur = lpc + i * 32;
        for (int j = 0; j < i; j++) r -= last[j] * ac[i - j - 1];
        if (err) r /= err;
        err *= 1.0 - (r * r);
        cur[i] = r;
        for (int j = 0; j < (i + 1) >> 1; j++) {
            double f = last[j], b = last[i - 1 - j];
            cur[j] = f + r * b;
            cur[i - 1 - j] = b + r * f;
        }
    }
}
static void ff_quantize(const double* lpc_in0, int order, int precision, s32* out, int* shift, int forced_sh) {
    double lpc_in[32];
    memcpy(lpc_in, lpc_in0, sizeof(double) * order);
    s32 qmax = (1 << (precision - 1)) - 1;
    double cmax = 0.0;
    for (int i = 0; i < order; i++) cmax = Max(cmax, fabs(lpc_in[i]));
    int sh;
    if (forced_sh < 0) {
        if (cmax * (1 << 15) < 1.0) {
            *shift = 0;
            memset(out, 0, sizeof(s32) * order);
            return;
        }
        sh = 15;
        while ((cmax * (1 << sh) > qmax) && (sh > 0)) sh--;
        if (sh == 0 && cmax > qmax) {
            double scale = ((double)qmax) / cmax;
            for (int i = 0; i < order; i++) lpc_in[i] *= scale;
        }
    } else
        sh = forced_sh;
    double error = 0;
    for (int i = 0; i < order; i++) {
        error -= lpc_in[i] * (1 << sh);
        long v = lrintf((float)error);
        if (v > qmax) v = qmax;
        if (v < -qmax) v = -qmax;
        out[i] = (s32)v;
        error -= out[i];
    }
    *shift = sh;
}
static int ff_est_order(const double* lpc, int min_order, int max_order) {
    int est = min_order;
    for (int i = max_order - 1; i >= min_order - 1; i--)
        if (fabs(lpc[i * 32 + i]) > 0.10) {
            est = i + 1;
            break;
        }
    return est;
}
