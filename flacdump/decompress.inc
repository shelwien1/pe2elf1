// flacdump: d mode: wav + meta (+ embedded files) -> flac
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

static bool decompress_core(const u8* meta, size_t meta_n, WavReader& wav, Sink& out) {
    if (meta_n < 2 || memcmp(meta, k_magic, 2)) return false;
    AC ac;
    ac.dec_init(meta + 2, meta_n - 2);
    reset_run_state();
    FlacLayout L = {};
    if (!code_layout(ac, g_models, L, &out)) return false;
    const StreamParams& sp = L.sp;
    wav.bps = sp.bps;
    if (wav.nch != sp.nch || wav.cb != (sp.bps + 7) / 8) return false;
    u64 pos = L.header_size();
    SegmentCoder r;
    r.init(ac, L, wav.nsamples, pos);
    if (!r.code(nullptr, nullptr, &wav, &out, pos)) return false;
    r.fs.md5.final(r.fs.md5sum);
    if (!code_derived(ac, L, r.fs, &out)) {
        fprintf(stderr, "flacdump: the .wav does not match the .meta\n");
        return false;
    }
    out.pos = pos;
    L.blocks.release();
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
