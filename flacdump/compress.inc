// flacdump: c mode helpers: options, layout parsing, side dump, statistics
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct Options {
    int verify;
    int dump;  // -x: write <prefix>.side, a plain serialization of the side info (for comparisons)
};

// plain serialization of everything except PCM-derived data (diagnostics only)
static void dump_frame_side(Buf& o, const u8* frame, size_t hl, const FrameInfo& f) {
    o.append(frame, hl - 1);  // header without CRC-8
    for (int ch = 0; ch < f.nch; ch++) {
        const SubInfo& s = f.sub[ch];
        o.put((u8)(s.type * 64 + s.order));
        o.put((u8)s.wasted);
        if (s.type < ST_FIXED) continue;
        if (s.type == ST_LPC) {
            o.put((u8)s.prec);
            o.put((u8)s.shift);
            for (int i = 0; i < s.order; i++) {
                o.put((u8)s.coef[i]);
                o.put((u8)(s.coef[i] >> 8));
            }
        }
        o.put((u8)(s.method * 16 + s.porder));
        for (int i = 0; i < (1 << s.porder); i++) {
            o.put(s.param[i]);
            if (s.param[i] == (s.method ? 31 : 15)) o.put(s.raw[i]);
        }
    }
}

// blocks and prefix point into the input buffer (no copies)
static bool parse_layout(Buf& in, FlacLayout& L, u64& audio_start) {
    size_t pos = 0;
    // skip ID3v2 tags
    while (pos + 10 <= in.n && !memcmp(in.p + pos, "ID3", 3)) {
        const u8* h = in.p + pos;
        u32 sz = ((h[6] & 0x7F) << 21) | ((h[7] & 0x7F) << 14) | ((h[8] & 0x7F) << 7) | (h[9] & 0x7F);
        pos += 10 + sz + ((h[5] & 0x10) ? 10 : 0);
    }
    if (pos + 4 > in.n || memcmp(in.p + pos, "fLaC", 4)) {
        // search a bit
        size_t lim = Min(in.n, (size_t)(1 << 26));
        size_t q = 0;
        for (; q + 8 <= lim; q++)
            if (!memcmp(in.p + q, "fLaC", 4) && (in.p[q + 4] & 0x7F) == 0 && rd_be24(in.p + q + 5) == 34) break;
        if (q + 8 > lim) return false;
        pos = q;
    }
    L.prefix = in.p;
    L.prefix_n = pos;
    pos += 4;
    L.blocks.n = 0;
    for (;;) {
        if (pos + 4 > in.n) return false;
        const u8* h = in.p + pos;
        MBlock b;
        b.type = h[0] & 0x7F;
        b.len = rd_be24(h + 1);
        b.structured = 0;
        int last = h[0] >> 7;
        pos += 4;
        if (pos + b.len > in.n) return false;
        b.off = pos - 4;
        b.data = in.p + pos;
        pos += b.len;
        L.blocks.push(b);
        if (last) break;
    }
    if (L.blocks[0].type != 0 || L.blocks[0].len != 34) return false;
    const u8* d = L.blocks[0].data;
    L.sp.rate = (d[10] << 12) | (d[11] << 4) | (d[12] >> 4);
    L.sp.nch = ((d[12] >> 1) & 7) + 1;
    L.sp.bps = (((d[12] & 1) << 4) | (d[13] >> 4)) + 1;
    audio_start = pos;
    return true;
}

static void print_stats(AC& ac, size_t total) {
    double sum = 0;
    for (int i = 0; i < NCAT; i++) sum += ac.cost[i];
    fprintf(stderr, "meta breakdown (bytes):\n");
    for (int i = 0; i < NCAT; i++)
        if (ac.cost[i] > 0) fprintf(stderr, "  %-10s %10.1f\n", k_catname[i], ac.cost[i] / 8);
    fprintf(stderr, "  %-10s %10.1f (actual %zu)\n", "total", sum / 8, total);
}
