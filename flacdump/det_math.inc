// flacdump: deterministic cos / cosf / log2 (basic IEEE operations only: identical results on any platform)
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// cos(x) for |x| <= ~8, fdlibm-style kernel; deterministic
static double det_cos(double x) {
    const double invpio2 = 6.36619772367581382433e-01;
    const double pio2_1 = 1.57079632673412561417e+00;
    const double pio2_1t = 6.07710050650619224932e-11;
    double ax = x < 0 ? -x : x;
    int n = (int)(ax * invpio2 + 0.5);
    double fn = (double)n;
    double r = ax - fn * pio2_1;
    double w = fn * pio2_1t;
    double y = r - w;
    double yl = (r - y) - w;
    double z = y * y;
    double res;
    if ((n & 1) == 0) {  // cos(y)
        const double C1 = 4.16666666666666019037e-02, C2 = -1.38888888888741095749e-03,
                     C3 = 2.48015872894767294178e-05, C4 = -2.75573143513906633035e-07,
                     C5 = 2.08757232129817482790e-09, C6 = -1.13596475577881948265e-11;
        double rr = z * (C1 + z * (C2 + z * (C3 + z * (C4 + z * (C5 + z * C6)))));
        res = 1.0 - (0.5 * z - (z * rr - y * yl));
    } else {  // sin(y)
        const double S1 = -1.66666666666666324348e-01, S2 = 8.33333333332248946124e-03,
                     S3 = -1.98412698298579493134e-04, S4 = 2.75573137070700676789e-06,
                     S5 = -2.50507602534068634195e-08, S6 = 1.58969099521155010221e-10;
        double v = z * y;
        double rr = S2 + z * (S3 + z * (S4 + z * (S5 + z * S6)));
        res = y - ((z * (0.5 * yl - v * rr) - yl) - v * S1);
    }
    switch (n & 3) {
        case 0: return res;
        case 1: return -res;
        case 2: return -res;
        default: return res;
    }
}
static inline float det_cosf(float x) { return (float)det_cos((double)x); }

// glibc (>= 2.28) cosf for 0 <= y < 120, emulated with plain double arithmetic
static float glibc_cosf(float y) {
    static const double T[2][13] = {
        {1.0, -1.0, -1.0, 1.0, 0x1.45F306DC9C883p+23, 0x1.921FB54442D18p0, 0x1p0, -0x1.ffffffd0c621cp-2,
         0x1.55553e1068f19p-5, -0x1.6c087e89a359dp-10, 0x1.99343027bf8c3p-16, -0x1.555545995a603p-3,
         0x1.1107605230bc4p-7},
        {1.0, -1.0, -1.0, 1.0, 0x1.45F306DC9C883p+23, 0x1.921FB54442D18p0, -0x1p0, 0x1.ffffffd0c621cp-2,
         -0x1.55553e1068f19p-5, 0x1.6c087e89a359dp-10, -0x1.99343027bf8c3p-16, -0x1.555545995a603p-3,
         0x1.1107605230bc4p-7}};
    static const double S3 = -0x1.994eb3774cf24p-13;
    u32 yi;
    memcpy(&yi, &y, 4);
    u32 top = (yi >> 20) & 0x7ff;
    const float pio4 = 0x1.921FB6p-1f;
    u32 pi4i;
    memcpy(&pi4i, &pio4, 4);
    double x = y;
    int n;
    const double* p = T[0];
    if (top < ((pi4i >> 20) & 0x7ff)) {
        if (top < 0x398) return 1.0f;  // abstop12(0x1p-12f)
        n = 1;
    } else {
        double r = x * p[4];
        n = ((s32)r + 0x800000) >> 24;
        x = x - n * p[5];
        double sg = p[n & 3];
        if (n & 2) p = T[1];
        x = x * sg;
        n ^= 1;
    }
    double x2 = x * x;
    if ((n & 1) == 0) {
        double x3 = x * x2;
        double s1 = p[12] + x2 * S3;
        double x7 = x3 * x2;
        double sv = x + x3 * p[11];
        return (float)(sv + x7 * s1);
    }
    double x4 = x2 * x2;
    double c2 = p[9] + x2 * p[10];
    double c1 = p[6] + x2 * p[7];
    double x6 = x4 * x2;
    double c = c1 + x4 * p[8];
    return (float)(c + x6 * c2);
}

// deterministic log2 (frexp + atanh series), x > 0
static double det_log2(double x) {
    int e;
    double m = frexp(x, &e);  // [0.5,1)
    if (m < 0.70710678118654752440) {
        m *= 2.0;
        e--;
    }
    double t = (m - 1.0) / (m + 1.0), t2 = t * t, p = t, sum = 0.0;
    for (int k = 1; k <= 31; k += 2) {
        sum += p / k;
        p *= t2;
    }
    return (double)e + 2.0 * sum * 1.44269504088896340736;
}
