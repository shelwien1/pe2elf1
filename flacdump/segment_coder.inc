// flacdump: SegmentCoder - the audio region: FRAME, raw frame and junk segments, coded in both
// directions (compression pass 2, or a restore)
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct SegmentCoder {
    Ctx cx;
    FrameState st;
    FrameStats fs;
    FrameInfo fi;
    u64 done;  // samples so far

    void init(AC& ac, const FlacLayout& L, u64 total_samples, u64 audio_start) {
        cx.ac = &ac;
        cx.m = &g_models;
        cx.sp = L.sp;
        cx.rs.init();
        cx.lp = &g_lpred;
        cx.total_samples = total_samples;
        st.init(L.sp, L.vendor);
        fs.init();
        fs.audio_start = audio_start;
        fi.init();
        done = 0;
    }

    // a frame of len bytes at pos with nsamp samples (PCM in g_pcm) is complete
    void frame_done(u64 pos, u32 len, u32 nsamp) {
        fs.add_frame(pos, len, nsamp);
        fs.add_pcm(g_pcm, cx.sp.nch, cx.sp.bps, nsamp);
        done += nsamp;
    }

    // FRAME: modelled (d: then re-encoded and written)
    bool seg_frame(const Segment* s, const Buf* in, WavReader* wav, Sink* out, u64& pos) {
        u32 len;
        if (!cx.ac->dec) {
            if (parse_frame(in->p + pos, (size_t)(in->n - pos), cx.sp, fi, g_pcm) != s->len) die("internal: reparse");
            if (!model_frame(cx, fi, g_pcm, st, nullptr, done)) die("internal: model_frame");
            len = (u32)s->len;
        } else {
            if (!model_frame(cx, fi, g_pcm, st, wav, done)) return false;
            FixOut fo = {g_framebuf, 0, FRAMEMAX, false};
            if (!encode_frame(fi, g_pcm, fo)) return false;
            out->write_at(pos, g_framebuf, fo.n);
            len = (u32)fo.n;
        }
        frame_done(pos, len, fi.bs);
        pos += len;
        return true;
    }

    // a frame that does not re-encode exactly: sample count, length and its bytes (its PCM is in the wav)
    bool seg_rawframe(const Segment* s, const Buf* in, WavReader* wav, Sink* out, u64& pos) {
        AC& ac = *cx.ac;
        Models& M = *cx.m;
        const StreamParams& sp = cx.sp;
        if (!ac.dec && parse_frame(in->p + pos, (size_t)(in->n - pos), sp, fi, g_pcm) != s->len)
            die("internal: reparse");
        ac.cat = C_RAW;
        u64 ns64 = code_uint(ac, M.len_m, ac.dec ? 0 : s->nsamp - 1) + 1;
        u64 len = code_uint(ac, M.len_m, ac.dec ? 0 : s->len);
        if (ns64 > MAXBS || len > FRAMEMAX) return false;  // no FLAC frame is that large
        u32 ns = (u32)ns64;
        if (!ac.dec)
            for (u64 j = 0; j < len; j++) ac.raw(in->p[pos + j], 8);
        else {
            if (!wav->read(g_pcm, sp.nch, ns)) return false;
            for (u64 j = 0; j < len;) {  // stream through the frame buffer
                size_t k = (size_t)Min(len - j, (u64)FRAMEMAX);
                for (size_t q = 0; q < k; q++) g_framebuf[q] = (u8)ac.raw(0, 8);
                out->write_at(pos + j, g_framebuf, k);
                j += k;
            }
        }
        frame_done(pos, (u32)len, ns);
        st.frame_counter++;
        st.sample_counter += ns;
        pos += len;
        return true;
    }

    // junk between frames: a byte region (embedded files are extracted); d: decoded into tmp
    bool seg_rawbytes(const Segment* s, const Buf* in, Sink* out, Buf& tmp, u64& pos) {
        AC& ac = *cx.ac;
        ac.cat = C_RAW;
        u64 len = code_uint(ac, cx.m->len_m, ac.dec ? 0 : s->len - 1) + 1;
        u8* data = in ? in->p + pos : nullptr;
        if (ac.dec) {
            tmp.n = 0;
            tmp.reserve(len);
            data = tmp.p;
        }
        if (!Regions::code_region(ac, data, (size_t)len, 0, false)) {
            if (!ac.dec) die("internal: region");
            return false;
        }
        if (ac.dec) out->write_at(pos, data, (size_t)len);
        pos += len;
        return true;
    }

    // The audio region from pos on, segment by segment. enc: the segments found by the scan, bytes and
    // PCM from 'in'; d: PCM from 'wav', the restored bytes go to 'out'. pos ends after the region.
    bool code(const Vec<Segment>* segs, const Buf* in, WavReader* wav, Sink* out, u64& pos) {
        AC& ac = *cx.ac;
        Buf tmp;  // d: a junk region (unbounded size)
        bool ok = true;
        int pk = 4;
        for (size_t i = 0; ok; i++) {
            const Segment* s = !ac.dec && i < segs->n ? &(*segs)[i] : nullptr;
            ac.cat = C_HDR;
            int kind = ac.tree(cx.m->seg[pk], 2, ac.dec ? 0 : s ? s->kind : SEG_END);
            pk = kind;
            if (kind == SEG_END) break;
            if (!ac.dec && s->off != pos) die("internal: segments");
            if (kind == SEG_FRAME)
                ok = seg_frame(s, in, wav, out, pos);
            else if (kind == SEG_RAWFRAME)
                ok = seg_rawframe(s, in, wav, out, pos);
            else
                ok = seg_rawbytes(s, in, out, tmp, pos);
        }
        tmp.release();
        return ok;
    }
};
