// flacdump: c mode: flac -> wav + meta (+ embedded files): scan, model, verification
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

// result of pass 1
struct Scan {
    Vec<Segment> segs;
    u64 nframes, nraw, njunk;  // frames that re-encode exactly, other frames, junk bytes
    u64 total_samples;
};

static void write_side_dump(const char* prefix, Buf& side) {
    char sname[4200];
    snprintf(sname, sizeof(sname), "%s.side", prefix);
    FILE* sf = fopen(sname, "wb");
    if (!sf || fwrite(side.p, 1, side.n, sf) != side.n || fclose(sf)) die("cannot write side dump");
    side.release();
}

// pass 1: split the audio region into frames that re-encode exactly, other frames and junk;
// writes the PCM of all decodable frames to the wav (and with -x the side dump)
static void compress_scan(const Buf& in, const FlacLayout& L, u64 audio_start, const char* prefix,
                          const char* wname, const Options& opt, Scan& sc) {
    const StreamParams& sp = L.sp;
    WavWriter ww;
    if (!ww.open(wname, sp.nch, sp.rate, sp.bps)) die("cannot create wav");
    FrameInfo fi;
    frameinfo_init(fi);
    Buf side;
    if (opt.dump) side.append(in.p, (size_t)audio_start);
    u64 pos = audio_start;
    while (pos < in.n) {
        size_t len = parse_frame(in.p + pos, (size_t)(in.n - pos), sp, fi, g_pcm);
        if (len && opt.dump) {
            FrameInfo fh;
            frameinfo_init(fh);
            size_t hl = parse_header(in.p + pos, (size_t)(in.n - pos), sp, fh);
            dump_frame_side(side, in.p + pos, hl, fi);
        }
        if (len) {
            FixOut fo = {g_framebuf, 0, FRAMEMAX, false};
            int same = encode_frame(fi, g_pcm, fo) && fo.n == len && !memcmp(g_framebuf, in.p + pos, len);
            Segment s;
            s.kind = same ? SEG_FRAME : SEG_RAWFRAME;
            s.off = pos;
            s.len = len;
            s.nsamp = fi.bs;
            sc.segs.push(s);
            ww.write(g_pcm, fi.bs);
            if (same)
                sc.nframes++;
            else
                sc.nraw++;
            pos += len;
        } else {
            u64 q = pos + 1;
            while (q < in.n) {
                if (in.p[q] == 0xFF && (in.p[q + 1] & 0xFE) == 0xF8 &&
                    parse_frame(in.p + q, (size_t)(in.n - q), sp, fi, g_pcm))
                    break;
                q++;
            }
            if (sc.segs.n && sc.segs.back().kind == SEG_RAWBYTES)
                sc.segs.back().len += q - pos;
            else {
                Segment s;
                s.kind = SEG_RAWBYTES;
                s.off = pos;
                s.len = q - pos;
                s.nsamp = 0;
                sc.segs.push(s);
            }
            sc.njunk += q - pos;
            if (opt.dump) side.append(in.p + pos, (size_t)(q - pos));
            pos = q;
        }
    }
    if (opt.dump) write_side_dump(prefix, side);
    sc.total_samples = ww.nsamples;
    if (!ww.close()) die("error writing wav");
}

// -v: statistics of the compression run
static void print_compress_stats(const Scan& sc, AC& ac, size_t meta_n, const FrameState& st, const LpcPredictor& P) {
    fprintf(stderr, "frames: %llu modelled, %llu raw, %llu junk bytes; %llu samples\n", (unsigned long long)sc.nframes,
            (unsigned long long)sc.nraw, (unsigned long long)sc.njunk, (unsigned long long)sc.total_samples);
    print_stats(ac, meta_n);
    fprintf(stderr, "lpc model: stage0 %u/%u, stage1 %u/%u (hit/miss)\n", st.dbg[1], st.dbg[0], st.dbg[3],
            st.dbg[2]);
    fprintf(stderr, "stereo sim: %u hit, %u miss; rice_new=%d fix_le=%d exh=%d maxo=%d\n", st.simstat[1], st.simstat[0],
            st.rice_new, st.fix_le, st.lpc_exh, st.lpc_maxo);
    fprintf(stderr, "lpc subframes %u, candidate hits:", P.nsub);
    for (int i = 0; i < P.ncand; i++)
        if (P.hits[i]) fprintf(stderr, " %d:%u", i, P.hits[i]);
    fprintf(stderr, "\n");
}

// pass 2: code the layout, every segment and the derived fields; writes the .meta
static void compress_model(const Buf& in, FlacLayout& L, u64 audio_start, const Scan& sc, const char* mname,
                           Buf& meta) {
    const StreamParams& sp = L.sp;
    meta.append(k_magic, 2);
    AC ac;
    ac.enc_init(&meta);
    reset_run_state();
    Models* M = &g_models;
    if (!code_layout(ac, *M, L, nullptr)) die("internal: layout");
    Ctx cx;
    cx.ac = &ac;
    cx.m = M;
    cx.sp = sp;
    cx.rs.init();
    cx.lp = &g_lpred;
    cx.total_samples = sc.total_samples;
    FrameState st;
    fstate_init(st, sp, L.vendor);
    FrameStats fs;
    fs.init();
    fs.audio_start = audio_start;
    FrameInfo fi;
    frameinfo_init(fi);
    u64 done = 0;
    int pk = 4;
    for (size_t i = 0; i <= sc.segs.n; i++) {
        int kind = i < sc.segs.n ? sc.segs[i].kind : SEG_END;
        ac.cat = C_HDR;
        ac.tree(M->seg[pk], 2, kind);
        pk = kind;
        if (kind == SEG_END) break;
        const Segment& s = sc.segs[i];
        if (kind == SEG_FRAME) {
            size_t len = parse_frame(in.p + s.off, (size_t)(in.n - s.off), sp, fi, g_pcm);
            if (len != s.len) die("internal: reparse");
            if (!model_frame(cx, fi, g_pcm, st, nullptr, done)) die("internal: model_frame");
            fs.add_frame(s.off, (u32)s.len, fi.bs);
            fs.add_pcm(g_pcm, sp.nch, sp.bps, fi.bs);
            done += fi.bs;
        } else if (kind == SEG_RAWFRAME) {
            size_t len = parse_frame(in.p + s.off, (size_t)(in.n - s.off), sp, fi, g_pcm);
            if (len != s.len) die("internal: reparse");
            ac.cat = C_RAW;
            code_uint(ac, M->len_m, s.nsamp - 1);
            code_uint(ac, M->len_m, s.len);
            for (u64 j = 0; j < s.len; j++) ac.raw(in.p[s.off + j], 8);
            fs.add_frame(s.off, (u32)s.len, s.nsamp);
            fs.add_pcm(g_pcm, sp.nch, sp.bps, s.nsamp);
            done += s.nsamp;
            st.frame_counter++;
            st.sample_counter += s.nsamp;
        } else {
            ac.cat = C_RAW;
            code_uint(ac, M->len_m, s.len - 1);
            if (!code_region(ac, in.p + s.off, (size_t)s.len, 0, false)) die("internal: region");
        }
    }
    fs.md5.final(fs.md5sum);
    code_derived(ac, L, fs, nullptr);
    ac.flush();
    FILE* mf = fopen(mname, "wb");
    if (!mf) die("cannot create meta");
    fwrite(meta.p, 1, meta.n, mf);
    if (fclose(mf)) die("error writing meta");
    if (g_verbose) print_compress_stats(sc, ac, meta.n, st, *cx.lp);
}

// restore from (wav, meta) in memory and compare with the input
static bool verify_meta(const Buf& in, const char* wname, const StreamParams& sp, const Buf& meta) {
    WavReader wr;
    wr.f = nullptr;
    if (!wr.open(wname)) die("verify: cannot reopen wav");
    wr.bps = sp.bps;
    Sink sk;
    memset(&sk, 0, sizeof(sk));
    sk.ref = in.p;
    sk.ref_n = in.n;
    int g = g_verbose;
    g_verbose = 0;
    bool ok = decompress_core(meta.p, meta.n, wr, sk);
    g_verbose = g;
    wr.close();
    return ok && !sk.mismatch && sk.pos == in.n;
}

// c mode: flac -> wav + meta (+ embedded files), verified unless -n
static int do_compress(const char* inname, const char* prefix, const Options& opt) {
    g_ext.prefix = prefix;
    Buf in;
    if (!read_file(inname, in, 64)) die("cannot read input");
    FlacLayout L = {};
    u64 audio_start = 0;
    if (!parse_layout(in, L, audio_start)) die("not a FLAC file (or broken metadata)");
    char wname[4096], mname[4096];
    snprintf(wname, sizeof(wname), "%s.wav", prefix);
    snprintf(mname, sizeof(mname), "%s.meta", prefix);
    Scan sc = {};
    compress_scan(in, L, audio_start, prefix, wname, opt, sc);
    Buf meta;
    compress_model(in, L, audio_start, sc, mname, meta);
    printf("%s: %zu bytes -> wav %llu samples, meta %zu bytes", inname, in.n, (unsigned long long)sc.total_samples,
           meta.n);
    if (g_ext.count) printf(", %d embedded file(s) %llu bytes", g_ext.count, (unsigned long long)g_ext.bytes);
    printf("\n");
    if (opt.verify) {
        if (!verify_meta(in, wname, L.sp, meta)) {
            fprintf(stderr, "flacdump: VERIFY FAILED\n");
            return 3;
        }
        if (g_verbose) fprintf(stderr, "verify ok\n");
    }
    return 0;
}
