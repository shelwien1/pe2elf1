// flacdump: d mode: wav + meta (+ embedded files) -> flac
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

static bool decompress_core(const u8* meta, size_t meta_n, WavReader& wav, Sink& out) {
    if (meta_n < 2 || memcmp(meta, k_magic, 2)) return false;
    AC ac;
    ac.dec_init(meta + 2, meta_n - 2);
    Models* M = &g_models;
    models_init(*M);
    FlacLayout L = {};
    if (!code_layout(ac, *M, L, &out)) return false;
    const StreamParams& sp = L.sp;
    wav.bps = sp.bps;
    if (wav.nch != sp.nch || wav.cb != (sp.bps + 7) / 8) return false;
    u64 pos = header_size(L);
    Ctx cx;
    cx.ac = &ac;
    cx.m = M;
    cx.sp = sp;
    cx.rs.alloc();
    cx.lp = &g_lpred;
    cx.lp->init();
    cx.total_samples = wav.nsamples;
    FrameState st;
    fstate_init(st, sp, L.vendor);
    FrameStats fs;
    fs.init();
    fs.audio_start = pos;
    FrameInfo fi;
    frameinfo_init(fi);
    Buf tmp;  // raw junk regions (unbounded size)
    u64 done = 0;
    int pk = 4;
    for (;;) {
        int kind = ac.tree(M->seg[pk], 2, 0);
        pk = kind;
        if (kind == SEG_END) break;
        if (kind == SEG_FRAME) {
            if (!model_frame(cx, fi, g_pcm, st, &wav, done)) return false;
            FixOut fo = {g_framebuf, 0, FRAMEMAX, false};
            if (!encode_frame(fi, g_pcm, fo)) return false;
            out.write_at(pos, g_framebuf, fo.n);
            fs.add_frame(pos, (u32)fo.n, fi.bs);
            fs.add_pcm(g_pcm, sp.nch, sp.bps, fi.bs);
            pos += fo.n;
            done += fi.bs;
        } else if (kind == SEG_RAWFRAME) {
            u32 ns = (u32)code_uint(ac, M->len_m, 0) + 1;
            u64 len = code_uint(ac, M->len_m, 0);
            if (ns > MAXBS || !wav.read(g_pcm, sp.nch, ns)) return false;
            for (u64 j = 0; j < len;) {  // stream through the frame buffer
                size_t k = (size_t)Min(len - j, (u64)FRAMEMAX);
                for (size_t q = 0; q < k; q++) g_framebuf[q] = (u8)ac.raw(0, 8);
                out.write_at(pos + j, g_framebuf, k);
                j += k;
            }
            fs.add_frame(pos, (u32)len, ns);
            fs.add_pcm(g_pcm, sp.nch, sp.bps, ns);
            pos += len;
            done += ns;
            st.frame_counter++;
            st.sample_counter += ns;
        } else {
            u64 len = code_uint(ac, M->len_m, 0) + 1;
            tmp.n = 0;
            tmp.reserve(len);
            if (!code_region(ac, tmp.p, (size_t)len, 0, false)) return false;
            out.write_at(pos, tmp.p, (size_t)len);
            pos += len;
        }
    }
    fs.md5.final(fs.md5sum);
    if (!code_derived(ac, L, fs, &out)) {
        fprintf(stderr, "flacdump: the .wav does not match the .meta\n");
        return false;
    }
    out.pos = pos;
    L.blocks.release();
    tmp.release();
    return true;
}

static int do_decompress(const char* prefix, const char* outname) {
    g_ext.prefix = prefix;
    char wname[4096], mname[4096];
    snprintf(wname, sizeof(wname), "%s.wav", prefix);
    snprintf(mname, sizeof(mname), "%s.meta", prefix);
    Buf meta;
    if (!read_file(mname, meta, 16)) die("cannot read meta");
    WavReader wr;
    wr.f = nullptr;
    if (!wr.open(wname)) die("cannot read wav");
    Sink sk;
    memset(&sk, 0, sizeof(sk));
    sk.f = fopen(outname, "wb");
    if (!sk.f) die("cannot create output");
    bool ok = decompress_core(meta.p, meta.n, wr, sk);
    wr.close();
    if (fclose(sk.f) || !ok) {
        remove(outname);
        die("restore failed (corrupt or mismatched meta/wav)");
    }
    return 0;
}
