// flacdump: FLAC frame parser/decoder and encoder (inverse of parse_frame given FrameInfo + PCM)
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// Frame parser / decoder. Returns frame length in bytes, 0 if not a valid frame.

static bool decode_residual(BitReader& br, int bs, int order, SubInfo& s, s64* res) {
    s.method = br.get(2);
    if (s.method > 1) return false;
    s.porder = br.get(4);
    int np = 1 << s.porder;
    u32 psz = (u32)bs >> s.porder;
    if ((psz << s.porder) != (u32)bs) return false;
    if (psz < (u32)order) return false;
    int pb = s.method ? 5 : 4, esc = s.method ? 31 : 15;
    s64* r = res;
    for (int p = 0; p < np; p++) {
        u32 n = psz - (p == 0 ? order : 0);
        int k = br.get(pb);
        s.param[p] = (u8)k;
        if (k == esc) {
            int rb = br.get(5);
            s.raw[p] = (u8)rb;
            if (br.pos + (u64)rb * n > br.nbits) return false;
            for (u32 i = 0; i < n; i++) *r++ = br.gets(rb);
        } else {
            s.raw[p] = 0;
            for (u32 i = 0; i < n; i++) {
                u64 q;
                if (!br.unary(q)) return false;
                if (q > (1ull << 40)) return false;
                u64 u = (q << k) | br.get(k);
                *r++ = (s64)(u >> 1) ^ -(s64)(u & 1);
                if (br.pos > br.nbits) return false;
            }
        }
    }
    return br.ok();
}

static size_t parse_header(const u8* d, size_t avail, const StreamParams& sp, FrameInfo& f) {
    if (avail < 6) return 0;
    if (d[0] != 0xFF || (d[1] & 0xFE) != 0xF8) return 0;
    f.blocking = d[1] & 1;
    f.bscode = d[2] >> 4;
    f.srcode = d[2] & 15;
    f.chcode = d[3] >> 4;
    f.bpscode = (d[3] >> 1) & 7;
    if (d[3] & 1) return 0;
    if (f.bscode == 0 || f.srcode == 15 || f.chcode > 10 || f.bpscode == 3) return 0;
    size_t p = 4;
    u32 b0 = d[p++];
    u64 v;
    int len;
    if (!(b0 & 0x80)) {
        v = b0;
        len = 0;
    } else if ((b0 & 0xE0) == 0xC0) {
        v = b0 & 0x1F;
        len = 1;
    } else if ((b0 & 0xF0) == 0xE0) {
        v = b0 & 0x0F;
        len = 2;
    } else if ((b0 & 0xF8) == 0xF0) {
        v = b0 & 0x07;
        len = 3;
    } else if ((b0 & 0xFC) == 0xF8) {
        v = b0 & 0x03;
        len = 4;
    } else if ((b0 & 0xFE) == 0xFC) {
        v = b0 & 0x01;
        len = 5;
    } else if (b0 == 0xFE) {
        v = 0;
        len = 6;
    } else
        return 0;
    if (p + len + 5 > avail) return 0;
    for (int i = 0; i < len; i++) {
        u32 b = d[p++];
        if ((b & 0xC0) != 0x80) return 0;
        v = (v << 6) | (b & 0x3F);
    }
    f.number = v;
    if (f.bscode == 1)
        f.bs = 192;
    else if (f.bscode <= 5)
        f.bs = 576u << (f.bscode - 2);
    else if (f.bscode == 6)
        f.bs = d[p++] + 1;
    else if (f.bscode == 7) {
        f.bs = ((d[p] << 8) | d[p + 1]) + 1;
        p += 2;
    } else
        f.bs = 256u << (f.bscode - 8);
    f.srval = 0;
    if (f.srcode == 12)
        f.srval = d[p++];
    else if (f.srcode == 13 || f.srcode == 14) {
        f.srval = (d[p] << 8) | d[p + 1];
        p += 2;
    }
    if (p >= avail) return 0;
    if (crc8(d, p) != d[p]) return 0;
    p++;
    f.nch = f.chcode < 8 ? f.chcode + 1 : 2;
    f.bps = f.bpscode ? k_bps_tab[f.bpscode] : sp.bps;
    if (f.nch != sp.nch || f.bps != sp.bps) return 0;
    if (f.bs > MAXBS - 1 && f.bs != MAXBS) return 0;
    return p;
}

static size_t parse_frame(const u8* d, size_t avail, const StreamParams& sp, FrameInfo& f, s32** pcm) {
    size_t hl = parse_header(d, avail, sp, f);
    if (!hl) return 0;
    BitReader br;
    br.init(d, avail);
    br.pos = hl * 8;
    int bs = (int)f.bs;
    for (int ch = 0; ch < f.nch; ch++) {
        SubInfo& s = f.sub[ch];
        int sbps = f.bps + side_channel(f.chcode, ch);
        if (br.get(1)) return 0;
        int t = br.get(6);
        s.wasted = 0;
        if (br.get(1)) {
            u64 z;
            if (!br.unary(z)) return 0;
            if (z >= 40) return 0;
            s.wasted = (int)z + 1;
        }
        if (s.wasted >= sbps) return 0;
        int eb = sbps - s.wasted;
        s64* x = g_sig[ch];
        s.order = 0;
        if (t == 0) {
            s.type = ST_CONST;
            s64 v = br.gets(eb);
            for (int i = 0; i < bs; i++) x[i] = v;
        } else if (t == 1) {
            s.type = ST_VERB;
            if (br.pos + (u64)eb * bs > br.nbits) return 0;
            for (int i = 0; i < bs; i++) x[i] = br.gets(eb);
        } else if (t >= 8 && t <= 12) {
            s.type = ST_FIXED;
            s.order = t - 8;
            if (s.order > bs) return 0;
            for (int i = 0; i < s.order; i++) x[i] = br.gets(eb);
            if (!decode_residual(br, bs, s.order, s, g_res)) return 0;
            fixed_restore(x, bs, s.order, g_res);
        } else if (t >= 32) {
            s.type = ST_LPC;
            s.order = t - 31;
            if (s.order > bs) return 0;
            for (int i = 0; i < s.order; i++) x[i] = br.gets(eb);
            s.prec = br.get(4) + 1;
            if (s.prec == 16) return 0;
            s.shift = (int)br.gets(5);
            if (s.shift < 0) return 0;
            for (int i = 0; i < s.order; i++) s.coef[i] = (s32)br.gets(s.prec);
            if (!decode_residual(br, bs, s.order, s, g_res)) return 0;
            lpc_restore(x, bs, s.coef, s.order, s.shift, g_res);
        } else
            return 0;
        if (!br.ok()) return 0;
        if (s.wasted)
            for (int i = 0; i < bs; i++) x[i] = (s64)((u64)x[i] << s.wasted);
    }
    br.pos = (br.pos + 7) & ~(u64)7;
    if (br.pos + 16 > br.nbits) return 0;
    size_t end = (size_t)(br.pos >> 3);
    u16 c = (u16)((d[end] << 8) | d[end + 1]);
    if (crc16(d, end) != c) return 0;
    // decorrelate + range check
    s64 lo = -(1ll << (f.bps - 1)), hi = (1ll << (f.bps - 1)) - 1;
    for (int ch = 0; ch < f.nch; ch++) {
        s32* o = pcm[ch];
        const s64* a = g_sig[0];
        const s64* b = g_sig[1];
        for (int i = 0; i < bs; i++) {
            s64 v;
            if (f.chcode < 8)
                v = g_sig[ch][i];
            else if (f.chcode == 8)
                v = ch == 0 ? a[i] : a[i] - b[i];
            else if (f.chcode == 9)
                v = ch == 0 ? a[i] + b[i] : b[i];
            else {
                s64 m = (s64)((u64)a[i] << 1) | (b[i] & 1);
                v = ch == 0 ? (m + b[i]) >> 1 : (m - b[i]) >> 1;
            }
            if (v < lo || v > hi) return 0;
            o[i] = (s32)v;
        }
    }
    return end + 2;
}

// Frame encoder (inverse of parse_frame given FrameInfo + PCM)

static void write_residual(BitWriter& bw, const SubInfo& s, const s64* r, int bs, int order) {
    bw.put(s.method, 2);
    bw.put(s.porder, 4);
    int np = 1 << s.porder;
    u32 psz = (u32)bs >> s.porder;
    int pb = s.method ? 5 : 4, esc = s.method ? 31 : 15;
    for (int p = 0; p < np; p++) {
        u32 n = psz - (p == 0 ? order : 0);
        int k = s.param[p];
        bw.put(k, pb);
        if (k == esc) {
            int rb = s.raw[p];
            bw.put(rb, 5);
            if (rb)
                for (u32 i = 0; i < n; i++) bw.puts(r[i], rb);
        } else {
            for (u32 i = 0; i < n; i++) {
                s64 v = r[i];
                u64 u = ((u64)v << 1) ^ (u64)(v >> 63);
                bw.rice(u, k);
            }
        }
        r += n;
    }
}

// returns false if the frame does not fit into the output storage
static bool encode_frame(const FrameInfo& f, s32** pcm, FixOut& out) {
    size_t start = out.n;
    u8 h[16];
    int p = 0;
    h[p++] = 0xFF;
    h[p++] = (u8)(0xF8 | f.blocking);
    h[p++] = (u8)((f.bscode << 4) | f.srcode);
    h[p++] = (u8)((f.chcode << 4) | (f.bpscode << 1));
    u64 v = f.number;
    if (v < 0x80)
        h[p++] = (u8)v;
    else {
        int nb = bitlen(v);
        int len = nb <= 11 ? 1 : nb <= 16 ? 2 : nb <= 21 ? 3 : nb <= 26 ? 4 : nb <= 31 ? 5 : 6;
        static const u8 lead[7] = {0, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE};
        h[p++] = (u8)(lead[len] | (v >> (6 * len)));
        for (int i = len - 1; i >= 0; i--) h[p++] = (u8)(0x80 | ((v >> (6 * i)) & 0x3F));
    }
    if (f.bscode == 6)
        h[p++] = (u8)(f.bs - 1);
    else if (f.bscode == 7) {
        h[p++] = (u8)((f.bs - 1) >> 8);
        h[p++] = (u8)(f.bs - 1);
    }
    if (f.srcode == 12)
        h[p++] = (u8)f.srval;
    else if (f.srcode == 13 || f.srcode == 14) {
        h[p++] = (u8)(f.srval >> 8);
        h[p++] = (u8)f.srval;
    }
    h[p] = crc8(h, p);
    p++;
    out.append(h, p);
    BitWriter bw;
    bw.init(&out);
    int bs = (int)f.bs;
    for (int ch = 0; ch < f.nch; ch++) {
        const SubInfo& s = f.sub[ch];
        int sbps = f.bps + side_channel(f.chcode, ch);
        s64* x = g_x;
        subframe_signal(f.chcode, ch, pcm, bs, x);
        if (s.wasted)
            for (int i = 0; i < bs; i++) x[i] >>= s.wasted;
        int eb = sbps - s.wasted;
        int t = s.type == ST_CONST ? 0 : s.type == ST_VERB ? 1 : s.type == ST_FIXED ? 8 + s.order : 31 + s.order;
        bw.put(t << 1 | (s.wasted ? 1 : 0), 8);
        if (s.wasted) {
            bw.zeros(s.wasted - 1);
            bw.put(1, 1);
        }
        if (s.type == ST_CONST)
            bw.puts(x[0], eb);
        else if (s.type == ST_VERB)
            for (int i = 0; i < bs; i++) bw.puts(x[i], eb);
        else {
            for (int i = 0; i < s.order; i++) bw.puts(x[i], eb);
            if (s.type == ST_FIXED)
                fixed_residual(x, bs, s.order, g_res);
            else {
                bw.put(s.prec - 1, 4);
                bw.puts(s.shift, 5);
                for (int i = 0; i < s.order; i++) bw.puts(s.coef[i], s.prec);
                lpc_residual(x, bs, s.coef, s.order, s.shift, g_res);
            }
            write_residual(bw, s, g_res, bs, s.order);
        }
    }
    bw.align();
    u16 c = crc16(out.p + start, out.n - start);
    out.put((u8)(c >> 8));
    out.put((u8)c);
    return !out.over;
}

enum { FRAMEMAX = 8 << 20 };   // largest re-encoded frame (verbatim worst case is ~2.2 MB)
static u8 g_framebuf[FRAMEMAX];
