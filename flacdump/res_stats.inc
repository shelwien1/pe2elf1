// flacdump: ResStats - residual statistics for partition order / rice parameter prediction
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// Per-subframe residual statistics for porder / rice parameter prediction
static u64 g_rs_asum[2 * MAXPART + 8], g_rs_zsum[2 * MAXPART + 8];  // sums per partition order (1 MiB)

struct ResStats {
    int bs, order, maxpo;
    int k0;  // libFLAC <= 1.3: rice parameter used for partition order 0 (-1: mean based)
    // sums per order: abs sums (libFLAC) and zigzag sums (ffmpeg); layout: offset[o] .. +2^o
    u64* asum;
    u64* zsum;
    int off[16];
    void init() {
        k0 = -1;
        asum = g_rs_asum;
        zsum = g_rs_zsum;
    }
    void compute(const s64* r, int bs_, int order_, bool withz = true) {
        bs = bs_;
        order = order_;
        maxpo = 0;
        while (maxpo < 15 && !((bs >> maxpo) & 1) && ((u32)bs >> (maxpo + 1)) >= (u32)order) maxpo++;
        // offsets: highest order first
        int o = 0;
        for (int po = maxpo; po >= 0; po--) {
            off[po] = o;
            o += 1 << po;
        }
        int np = 1 << maxpo;
        u32 psz = (u32)bs >> maxpo;
        const s64* p = r;
        u64* A = asum + off[maxpo];
        u64* Z = zsum + off[maxpo];
        for (int i = 0; i < np; i++) {
            u32 n = psz - (i == 0 ? order : 0);
            u64 a = 0, z = 0;
            if (withz) {
                for (u32 j = 0; j < n; j++) {
                    s64 v = p[j];
                    a += (u64)(v < 0 ? -v : v);
                    z += ((u64)v << 1) ^ (u64)(v >> 63);
                }
            } else {
                for (u32 j = 0; j < n; j++) {
                    s64 v = p[j];
                    a += (u64)(v < 0 ? -v : v);
                }
            }
            p += n;
            A[i] = a;
            Z[i] = z;
        }
        for (int po = maxpo - 1; po >= 0; po--) {
            u64* a2 = asum + off[po];
            u64* a1 = asum + off[po + 1];
            u64* z2 = zsum + off[po];
            u64* z1 = zsum + off[po + 1];
            for (int i = 0; i < (1 << po); i++) {
                a2[i] = a1[2 * i] + a1[2 * i + 1];
                if (withz) z2[i] = z1[2 * i] + z1[2 * i + 1];
            }
        }
    }
    u32 psamples(int po, int i) const { return ((u32)bs >> po) - (i == 0 ? order : 0); }
    // libFLAC estimate for partition i of order po
    int est_lf(int po, int i, int lim, bool newv) const {
        u32 ps = psamples(po, i);
        if (ps == 0) return 0;
        u64 m = asum[off[po] + i];
        int k = newv ? rice_est_new(m, ps) : rice_est_old(m, ps);
        return Min(k, lim);
    }
    int est_ff(int po, int i, int lim) const {
        u32 ps = psamples(po, i);
        if (ps == 0) return 0;
        return rice_est_ff(zsum[off[po] + i], ps, lim);
    }
    // best partition order libFLAC style in [lo,hi] (hi already limited)
    // minimal libFLAC-estimated residual bits over partition orders [lo,hi]
    u64 bits_lf(int lo, int hi, int lim, bool newv) const {
        u64 bb = ~0ull;
        for (int po = hi; po >= lo; po--) {
            if (((u32)bs >> po) <= (u32)order && po > 0) continue;
            u64 bits = 6;
            for (int i = 0; i < (1 << po); i++) {
                u32 ps = psamples(po, i);
                int k = (po == 0 && k0 >= 0) ? k0 : est_lf(po, i, lim, newv);
                bits += lf_bits(k, ps, asum[off[po] + i]);
            }
            if (bits < bb) bb = bits;
        }
        return bb;
    }
    int best_lf(int lo, int hi, int lim, bool newv) const {
        int best = hi;
        u64 bb = ~0ull;
        for (int po = hi; po >= lo; po--) {
            if (((u32)bs >> po) <= (u32)order && po > 0) continue;
            u64 bits = 6;
            for (int i = 0; i < (1 << po); i++) {
                u32 ps = psamples(po, i);
                int k = (po == 0 && k0 >= 0) ? k0 : est_lf(po, i, lim, newv);
                bits += lf_bits(k, ps, asum[off[po] + i]);
            }
            if (bits < bb) {
                bb = bits;
                best = po;
            }
        }
        return best;
    }
    u64 bits_ff(int lo, int hi, int lim) const {
        u64 bb = ~0ull;
        for (int po = hi; po >= lo; po--) {
            if (((u32)bs >> po) < (u32)order) continue;
            u64 bits = 4ull << po;
            for (int i = 0; i < (1 << po); i++) {
                u32 n = psamples(po, i);
                int k = est_ff(po, i, lim);
                u64 s = zsum[off[po] + i];
                bits += (u64)n * (k + 1) + ((s - (n >> 1)) >> k);
            }
            if (bits < bb || po == hi) bb = Min(bb, bits);
        }
        return bb;
    }
    int best_ff(int lo, int hi, int lim) const {
        int best = hi;
        u64 bb = ~0ull;
        for (int po = hi; po >= lo; po--) {
            if (((u32)bs >> po) < (u32)order) continue;
            u64 bits = 4ull << po;
            for (int i = 0; i < (1 << po); i++) {
                u32 n = psamples(po, i);
                int k = est_ff(po, i, lim);
                u64 s = zsum[off[po] + i];
                bits += (u64)n * (k + 1) + ((s - (n >> 1)) >> k);
            }
            if (bits < bb) {
                bb = bits;
                best = po;
            }
        }
        return best;
    }
};
