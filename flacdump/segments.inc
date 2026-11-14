// flacdump: the audio region: FRAME, raw frame and junk segments, coded in both directions
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// state of one pass over the segments (compression pass 2, or a restore)
struct Run {
    Ctx cx;
    FrameState st;
    FrameStats fs;
    FrameInfo fi;
    u64 done;  // samples so far
};

static void run_init(Run& r, AC& ac, const FlacLayout& L, u64 total_samples, u64 audio_start) {
    r.cx.ac = &ac;
    r.cx.m = &g_models;
    r.cx.sp = L.sp;
    r.cx.rs.init();
    r.cx.lp = &g_lpred;
    r.cx.total_samples = total_samples;
    fstate_init(r.st, L.sp, L.vendor);
    r.fs.init();
    r.fs.audio_start = audio_start;
    frameinfo_init(r.fi);
    r.done = 0;
}

// a frame of len bytes at pos with nsamp samples (PCM in g_pcm) is complete
static void run_frame(Run& r, u64 pos, u32 len, u32 nsamp) {
    r.fs.add_frame(pos, len, nsamp);
    r.fs.add_pcm(g_pcm, r.cx.sp.nch, r.cx.sp.bps, nsamp);
    r.done += nsamp;
}

// FRAME: modelled (d: then re-encoded and written)
static bool seg_frame(Run& r, const Segment* s, const Buf* in, WavReader* wav, Sink* out, u64& pos) {
    u32 len;
    if (!r.cx.ac->dec) {
        if (parse_frame(in->p + pos, (size_t)(in->n - pos), r.cx.sp, r.fi, g_pcm) != s->len) die("internal: reparse");
        if (!model_frame(r.cx, r.fi, g_pcm, r.st, nullptr, r.done)) die("internal: model_frame");
        len = (u32)s->len;
    } else {
        if (!model_frame(r.cx, r.fi, g_pcm, r.st, wav, r.done)) return false;
        FixOut fo = {g_framebuf, 0, FRAMEMAX, false};
        if (!encode_frame(r.fi, g_pcm, fo)) return false;
        out->write_at(pos, g_framebuf, fo.n);
        len = (u32)fo.n;
    }
    run_frame(r, pos, len, r.fi.bs);
    pos += len;
    return true;
}

// a frame that does not re-encode exactly: sample count, length and its bytes (its PCM is in the wav)
static bool seg_rawframe(Run& r, const Segment* s, const Buf* in, WavReader* wav, Sink* out, u64& pos) {
    AC& ac = *r.cx.ac;
    Models& M = *r.cx.m;
    const StreamParams& sp = r.cx.sp;
    if (!ac.dec && parse_frame(in->p + pos, (size_t)(in->n - pos), sp, r.fi, g_pcm) != s->len)
        die("internal: reparse");
    ac.cat = C_RAW;
    u32 ns = (u32)code_uint(ac, M.len_m, ac.dec ? 0 : s->nsamp - 1) + 1;
    u64 len = code_uint(ac, M.len_m, ac.dec ? 0 : s->len);
    if (!ac.dec)
        for (u64 j = 0; j < len; j++) ac.raw(in->p[pos + j], 8);
    else {
        if (ns > MAXBS || !wav->read(g_pcm, sp.nch, ns)) return false;
        for (u64 j = 0; j < len;) {  // stream through the frame buffer
            size_t k = (size_t)Min(len - j, (u64)FRAMEMAX);
            for (size_t q = 0; q < k; q++) g_framebuf[q] = (u8)ac.raw(0, 8);
            out->write_at(pos + j, g_framebuf, k);
            j += k;
        }
    }
    run_frame(r, pos, (u32)len, ns);
    r.st.frame_counter++;
    r.st.sample_counter += ns;
    pos += len;
    return true;
}

// junk between frames: a byte region (embedded files are extracted); d: decoded into tmp
static bool seg_rawbytes(Run& r, const Segment* s, const Buf* in, Sink* out, Buf& tmp, u64& pos) {
    AC& ac = *r.cx.ac;
    ac.cat = C_RAW;
    u64 len = code_uint(ac, r.cx.m->len_m, ac.dec ? 0 : s->len - 1) + 1;
    u8* data = in ? in->p + pos : nullptr;
    if (ac.dec) {
        tmp.n = 0;
        tmp.reserve(len);
        data = tmp.p;
    }
    if (!code_region(ac, data, (size_t)len, 0, false)) {
        if (!ac.dec) die("internal: region");
        return false;
    }
    if (ac.dec) out->write_at(pos, data, (size_t)len);
    pos += len;
    return true;
}

// The audio region from pos on, segment by segment. enc: the segments found by the scan, bytes and
// PCM from 'in'; d: PCM from 'wav', the restored bytes go to 'out'. pos ends after the region.
static bool code_segments(Run& r, const Vec<Segment>* segs, const Buf* in, WavReader* wav, Sink* out, u64& pos) {
    AC& ac = *r.cx.ac;
    Buf tmp;  // d: a junk region (unbounded size)
    bool ok = true;
    int pk = 4;
    for (size_t i = 0; ok; i++) {
        const Segment* s = !ac.dec && i < segs->n ? &(*segs)[i] : nullptr;
        ac.cat = C_HDR;
        int kind = ac.tree(r.cx.m->seg[pk], 2, ac.dec ? 0 : s ? s->kind : SEG_END);
        pk = kind;
        if (kind == SEG_END) break;
        if (!ac.dec && s->off != pos) die("internal: segments");
        if (kind == SEG_FRAME)
            ok = seg_frame(r, s, in, wav, out, pos);
        else if (kind == SEG_RAWFRAME)
            ok = seg_rawframe(r, s, in, wav, out, pos);
        else
            ok = seg_rawbytes(r, s, in, out, tmp, pos);
    }
    tmp.release();
    return ok;
}
