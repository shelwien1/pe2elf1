// flacdump: code_seektable - SEEKTABLE coding, points predicted from the frame list
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

static void code_seektable(AC& ac, MBlock& b, const FrameStats& fs, const StreamParams& sp, Sink* out) {
    ac.cat = C_SEEK;
    SeekModels S;
    model_init(S);
    u32 np = b.len / 18;
    u8* d = b.data;
    // spacing mode: 0 = fixed spacing in samples, 1 = total*j/np, 2 = none
    u64 spacing = 0;
    int mode = 2;
    u64 T = fs.total;
    if (!ac.dec) {
        // try candidates; count matching points
        u64 cs[3] = {10ull * sp.rate, 0, 0};
        if (np >= 2) {
            u64 s1 = rd_be64(d + 18);
            if (s1 != ~0ull) {
                u64 k = frame_containing(fs, s1);
                cs[1] = s1;
                (void)k;
            }
        }
        int best = -1;
        u32 bestc = 0;
        for (int c = 0; c < 3; c++) {
            u32 cnt = 0;
            for (u32 j = 0; j < np; j++) {
                u64 target = c < 2 ? cs[c] * j : (np ? T * j / np : 0);
                if (c < 2 && cs[c] == 0) break;
                u64 k = frame_containing(fs, target);
                if (k < fs.fsamp.n && rd_be64(d + 18 * j) == fs.fsamp[k]) cnt++;
            }
            if (cnt > bestc) {
                bestc = cnt;
                best = c;
            }
        }
        if (best >= 0 && bestc > np / 4) {
            mode = best < 2 ? 0 : 1;
            spacing = best < 2 ? cs[best] : 0;
        }
    }
    mode = ac.tree(S.mode, 2, mode);
    if (mode == 0) {
        if (ac.code(spacing == 10ull * sp.rate, 32768))
            spacing = 10ull * sp.rate;
        else
            spacing = code_uint(ac, S.dk, spacing);
    }
    u64 prevk = 0;
    int pkind = 0;
    for (u32 j = 0; j < np; j++) {
        u8 pt[18];
        u8* p = ac.dec ? pt : d + 18 * j;
        u64 sn = 0, off = 0;
        u32 fsm = 0;
        int kind = 2;
        u64 k = 0;
        if (!ac.dec) {
            sn = rd_be64(p);
            off = rd_be64(p + 8);
            fsm = (p[16] << 8) | p[17];
            if (sn == ~0ull && off == 0 && fsm == 0)
                kind = 0;
            else {
                u64 kk = frame_containing(fs, sn);
                if (kk < fs.fsamp.n && fs.fsamp[kk] == sn && fs.foff[kk] == off && fs.fbs[kk] == fsm) {
                    kind = 1;
                    k = kk;
                }
            }
        }
        kind = ac.tree(S.kind[Min(pkind, 2)], 2, kind);
        pkind = kind;
        if (kind == 0) {
            sn = ~0ull;
            off = 0;
            fsm = 0;
        } else if (kind == 1) {
            u64 pk = prevk;
            int have = 0;
            if (mode == 0) {
                pk = frame_containing(fs, spacing * j);
                have = 1;
            } else if (mode == 1) {
                pk = frame_containing(fs, np ? T * j / np : 0);
                have = 1;
            }
            int mt = have ? ac.bit(S.match[mode], k == pk) : 0;
            if (mt)
                k = pk;
            else {
                // code k - prevk (>=0 normally) with sign
                int neg = ac.code(k < prevk, 2000);
                u64 dlt = neg ? prevk - k : k - prevk;
                dlt = code_uint(ac, S.dk, dlt);
                k = neg ? prevk - dlt : prevk + dlt;
            }
            if (k >= fs.fsamp.n) k = fs.fsamp.n ? fs.fsamp.n - 1 : 0;
            if (fs.fsamp.n) {
                sn = fs.fsamp[k];
                off = fs.foff[k];
                fsm = fs.fbs[k];
            }
            prevk = k;
        } else {
            sn = ac.raw(sn, 64);
            off = ac.raw(off, 64);
            fsm = (u32)ac.raw(fsm, 16);
        }
        if (ac.dec) {
            wr_be64(p, sn);
            wr_be64(p + 8, off);
            p[16] = (u8)(fsm >> 8);
            p[17] = (u8)fsm;
            out->write_at(b.off + 4 + 18 * (u64)j, p, 18);
        }
    }
}
