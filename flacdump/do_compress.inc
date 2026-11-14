// flacdump: do_compress - c mode: flac -> wav + meta (+ embedded files), with verification
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

static int do_compress(const char* inname, const char* prefix, const Options& opt) {
    g_ext.prefix = prefix;
    Buf in;
    if (!read_file(inname, in, 64)) die("cannot read input");
    FlacLayout L = {};
    u64 audio_start = 0;
    if (!parse_layout(in, L, audio_start)) die("not a FLAC file (or broken metadata)");
    const StreamParams& sp = L.sp;
    char wname[4096], mname[4096];
    snprintf(wname, sizeof(wname), "%s.wav", prefix);
    snprintf(mname, sizeof(mname), "%s.meta", prefix);

    // pass 1: segment the audio region, decode, verify re-encoding, write wav
    WavWriter ww;
    if (!ww.open(wname, sp.nch, sp.rate, sp.bps)) die("cannot create wav");
    Vec<Segment> segs;
    FrameInfo fi;
    frameinfo_init(fi);
    Buf side;
    if (opt.dump) side.append(in.p, (size_t)audio_start);
    u64 pos = audio_start;
    u64 nframes = 0, nraw = 0, njunk = 0;
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
            segs.push(s);
            ww.write(g_pcm, fi.bs);
            if (same)
                nframes++;
            else
                nraw++;
            pos += len;
        } else {
            u64 q = pos + 1;
            while (q < in.n) {
                if (in.p[q] == 0xFF && (in.p[q + 1] & 0xFE) == 0xF8 &&
                    parse_frame(in.p + q, (size_t)(in.n - q), sp, fi, g_pcm))
                    break;
                q++;
            }
            if (segs.n && segs.back().kind == SEG_RAWBYTES)
                segs.back().len += q - pos;
            else {
                Segment s;
                s.kind = SEG_RAWBYTES;
                s.off = pos;
                s.len = q - pos;
                s.nsamp = 0;
                segs.push(s);
            }
            njunk += q - pos;
            if (opt.dump) side.append(in.p + pos, (size_t)(q - pos));
            pos = q;
        }
    }
    if (opt.dump) {
        char sname[4200];
        snprintf(sname, sizeof(sname), "%s.side", prefix);
        FILE* sf = fopen(sname, "wb");
        if (!sf || fwrite(side.p, 1, side.n, sf) != side.n || fclose(sf)) die("cannot write side dump");
        side.release();
    }
    u64 total_samples = ww.nsamples;
    if (!ww.close()) die("error writing wav");

    // pass 2: model
    Buf meta;
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
    cx.total_samples = total_samples;
    FrameState st;
    fstate_init(st, sp, L.vendor);
    FrameStats fs;
    fs.init();
    fs.audio_start = audio_start;
    u64 done = 0;
    int pk = 4;
    for (size_t i = 0; i <= segs.n; i++) {
        int kind = i < segs.n ? segs[i].kind : SEG_END;
        ac.cat = C_HDR;
        ac.tree(M->seg[pk], 2, kind);
        pk = kind;
        if (kind == SEG_END) break;
        Segment& s = segs[i];
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
    if (g_verbose) {
        fprintf(stderr, "frames: %llu modelled, %llu raw, %llu junk bytes; %llu samples\n", (unsigned long long)nframes,
                (unsigned long long)nraw, (unsigned long long)njunk, (unsigned long long)total_samples);
        print_stats(ac, meta.n);
        fprintf(stderr, "lpc model: stage0 %u/%u, stage1 %u/%u (hit/miss)\n", st.dbg[1], st.dbg[0], st.dbg[3],
                st.dbg[2]);
        fprintf(stderr, "stereo sim: %u hit, %u miss; rice_new=%d fix_le=%d exh=%d maxo=%d\n", st.simstat[1], st.simstat[0],
                st.rice_new, st.fix_le, st.lpc_exh, st.lpc_maxo);
        LpcPredictor& P = *cx.lp;
        fprintf(stderr, "lpc subframes %u, candidate hits:", P.nsub);
        for (int i = 0; i < P.ncand; i++)
            if (P.hits[i]) fprintf(stderr, " %d:%u", i, P.hits[i]);
        fprintf(stderr, "\n");
    }
    printf("%s: %zu bytes -> wav %llu samples, meta %zu bytes", inname, in.n, (unsigned long long)total_samples,
           meta.n);
    if (g_ext.count) printf(", %d embedded file(s) %llu bytes", g_ext.count, (unsigned long long)g_ext.bytes);
    printf("\n");
    if (opt.verify) {
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
        if (!ok || sk.mismatch || sk.pos != in.n) {
            fprintf(stderr, "flacdump: VERIFY FAILED\n");
            return 3;
        }
        if (g_verbose) fprintf(stderr, "verify ok\n");
    }
    return 0;
}
