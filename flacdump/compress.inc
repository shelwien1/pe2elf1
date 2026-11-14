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
        o.push((u8)(s.type * 64 + s.order));
        o.push((u8)s.wasted);
        if (s.type < ST_FIXED) continue;
        if (s.type == ST_LPC) {
            o.push((u8)s.prec);
            o.push((u8)s.shift);
            for (int i = 0; i < s.order; i++) {
                o.push((u8)s.coef[i]);
                o.push((u8)(s.coef[i] >> 8));
            }
        }
        o.push((u8)(s.method * 16 + s.porder));
        for (int i = 0; i < (1 << s.porder); i++) {
            o.push(s.param[i]);
            if (s.param[i] == (s.method ? 31 : 15)) o.push(s.raw[i]);
        }
    }
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
    fi.init();
    Buf side;
    if (opt.dump) side.append(in.p, (size_t)audio_start);
    u64 pos = audio_start;
    while (pos < in.n) {
        size_t len = parse_frame(in.p + pos, (size_t)(in.n - pos), sp, fi, g_pcm);
        if (len && opt.dump) {
            FrameInfo fh;
            fh.init();
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
    ac.print_stats(meta_n);
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
    meta.append(k_magic, 2);
    AC ac;
    ac.enc_init(&meta);
    reset_run_state();
    if (!code_layout(ac, g_models, L, nullptr)) die("internal: layout");
    SegmentCoder r;
    r.init(ac, L, sc.total_samples, audio_start);
    u64 pos = audio_start;
    if (!r.code(&sc.segs, &in, nullptr, nullptr, pos)) die("internal: segments");
    r.fs.md5.final(r.fs.md5sum);
    code_derived(ac, L, r.fs, nullptr);
    ac.flush();
    FILE* mf = fopen(mname, "wb");
    if (!mf) die("cannot create meta");
    fwrite(meta.p, 1, meta.n, mf);
    if (fclose(mf)) die("error writing meta");
    if (g_verbose) print_compress_stats(sc, ac, meta.n, r.st, *r.cx.lp);
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
    if (!L.parse(in, audio_start)) die("not a FLAC file (or broken metadata)");
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
