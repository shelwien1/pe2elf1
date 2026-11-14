// flacdump: metadata coding: stream parameters, layout (part A), STREAMINFO derived fields (part C)
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

static const u32 k_rates[8] = {44100, 48000, 96000, 88200, 192000, 176400, 32000, 22050};

static void code_stream_params(AC& ac, StreamParams& sp) {
    ac.cat = C_META;
    int ri = 7;
    for (int i = 0; i < 7; i++)
        if (k_rates[i] == sp.rate) ri = i;
    ri = (int)ac.raw(ri, 3);
    if (ri < 7)
        sp.rate = k_rates[ri];
    else
        sp.rate = (u32)ac.raw(sp.rate, 20);
    int st = ac.code(sp.nch == 2, 50000);
    if (st)
        sp.nch = 2;
    else
        sp.nch = (int)ac.raw(sp.nch - 1, 3) + 1;
    int bi = sp.bps == 16 ? 0 : sp.bps == 24 ? 1 : sp.bps == 8 ? 2 : 3;
    bi = (int)ac.raw(bi, 2);
    if (bi == 0)
        sp.bps = 16;
    else if (bi == 1)
        sp.bps = 24;
    else if (bi == 2)
        sp.bps = 8;
    else
        sp.bps = (int)ac.raw(sp.bps - 1, 5) + 1;
}

// Part A: prefix, block list, contents not depending on frames.
// d: everything except STREAMINFO and seek table contents is written to 'out' right away.
static bool code_layout(AC& ac, Models& M, FlacLayout& L, Sink* out) {
    ac.cat = C_META;
    u64 plen = code_uint(ac, M.len_m, L.prefix_n);
    if (ac.dec) {  // unbounded size (e.g. an ID3v2 tag): temporary buffer, written out at once
        Buf pb;
        pb.reserve((size_t)plen + 1);
        bool ok = Regions::code_region(ac, pb.p, (size_t)plen, 0, false);
        if (ok) out->write_at(0, pb.p, (size_t)plen);
        pb.release();
        if (!ok) return false;
        L.prefix_n = (size_t)plen;
        out->write_at(plen, (const u8*)"fLaC", 4);
    } else if (!Regions::code_region(ac, L.prefix, (size_t)plen, 0, false))
        return false;
    L.vendor = 0;
    ac.cat = C_META;
    code_stream_params(ac, L.sp);
    if (L.sp.nch < 1 || L.sp.nch > 8 || L.sp.bps < 4 || L.sp.bps > 32) return false;
    // block 0 is STREAMINFO
    u64 nb = code_uint(ac, M.num_m, L.blocks.n);
    if (ac.dec) {
        if (nb == 0 || nb > MAXBLOCKS) return false;
        L.blocks.n = 0;
        for (u64 i = 0; i < nb; i++) {
            MBlock b;
            memset(&b, 0, sizeof(b));
            L.blocks.push(b);
        }
        L.blocks[0].type = 0;
        L.blocks[0].len = 34;
        L.blocks[0].data = L.si;
    }
    u64 boff = L.prefix_n + 4;
    L.blocks[0].off = boff;
    boff += 4 + 34;
    int pt = 0;
    for (size_t i = 1; i < L.blocks.n; i++) {
        if (ac.exhausted()) return false;
        MBlock& b = L.blocks[i];
        b.type = (int)ac.tree(M.mtype[Min(pt, 7)], 7, b.type);
        pt = b.type;
        b.len = (u32)code_uint(ac, M.len_m, b.len);
        if (b.len >= BLOCKMAX) return false;
        b.off = boff;
        boff += 4 + (u64)b.len;
        if (ac.dec) {
            b.data = g_blockbuf;
            memset(g_blockbuf, 0, b.len);
        }
        if (b.type == 1) {  // PADDING
            int z = 1;
            if (!ac.dec)
                for (u32 j = 0; j < b.len; j++)
                    if (b.data[j]) z = 0;
            z = ac.bit(M.mflag[0], z);
            b.structured = z;
            if (!z && !Regions::code_region(ac, b.data, b.len, 0, false)) return false;
        } else if (b.type == 3) {  // SEEKTABLE
            int s = ac.bit(M.mflag[1], b.len % 18 == 0);
            b.structured = s;
            if (!s) code_bytes(ac, b.data, b.len, false);
        } else if (b.type == 4) {
            if (!VorbisCoder::code(ac, M, b)) return false;
            if (!L.vendor) L.vendor = vendor_hint(b.data, b.len);
        } else {
            if (!Regions::code_region(ac, b.data, b.len, b.type == 6 ? 1 : 0, false)) return false;
        }
        if (ac.dec) {  // seek tables are filled in later (code_derived)
            if (!(b.type == 3 && b.structured)) out->write_at(b.off + 4, b.data, b.len);
            b.data = nullptr;
        }
    }
    if (ac.dec)  // block headers
        for (size_t i = 0; i < L.blocks.n; i++) {
            u8 h[4];
            h[0] = (u8)((i + 1 == L.blocks.n ? 0x80 : 0) | L.blocks[i].type);
            wr_be24(h + 1, L.blocks[i].len);
            out->write_at(L.blocks[i].off, h, 4);
        }
    return true;
}

// Part C: STREAMINFO derived fields + seek tables
static void code_choice_u64(AC& ac, BM* flags, u64& v, const u64* cand, int nc, int bits) {
    for (int i = 0; i < nc; i++) {
        int dup = 0;
        for (int j = 0; j < i; j++)
            if (cand[j] == cand[i]) dup = 1;
        if (dup) continue;
        if (ac.bit(flags[i], v == cand[i])) {
            v = cand[i];
            return;
        }
    }
    v = ac.raw(v, bits);
}

static bool code_derived(AC& ac, FlacLayout& L, FrameStats& fs, Sink* out) {
    ac.cat = C_META;
    BM fl[8][4];
    bm_init(&fl[0][0], 32);
    MBlock& si = L.blocks[0];
    u8* d = si.data;
    u64 minbs = 0, maxbs = 0, minfs = 0, maxfs = 0, total = 0;
    if (!ac.dec) {
        minbs = (d[0] << 8) | d[1];
        maxbs = (d[2] << 8) | d[3];
        minfs = rd_be24(d + 4);
        maxfs = rd_be24(d + 7);
        total = ((u64)(d[13] & 15) << 32) | rd_be32(d + 14);
    }
    u32 mfs = fs.nframes ? fs.min_fs : 0;
    u32 xfs = fs.nframes ? fs.max_fs : 0;
    u64 c_minbs[4] = {fs.first_bs, fs.nframes > 1 ? fs.min_bs_nl : fs.min_bs, fs.min_bs, 0};
    u64 c_maxbs[4] = {fs.first_bs, fs.max_bs, fs.nframes > 1 ? fs.max_bs_nl : fs.max_bs, 0};
    code_choice_u64(ac, fl[0], minbs, c_minbs, 4, 16);
    u64 c_maxbs2[4] = {minbs, c_maxbs[1], c_maxbs[2], c_maxbs[0]};
    code_choice_u64(ac, fl[1], maxbs, c_maxbs2, 4, 16);
    u64 c_minfs[2] = {mfs, 0};
    code_choice_u64(ac, fl[2], minfs, c_minfs, 2, 24);
    u64 c_maxfs[2] = {xfs, 0};
    code_choice_u64(ac, fl[3], maxfs, c_maxfs, 2, 24);
    u64 c_tot[2] = {fs.total, 0};
    code_choice_u64(ac, fl[4], total, c_tot, 2, 36);
    // md5
    int which = 2;
    if (!ac.dec) {
        static const u8 zero[16] = {0};
        if (!memcmp(d + 18, fs.md5sum, 16))
            which = 0;
        else if (!memcmp(d + 18, zero, 16))
            which = 1;
    }
    if (ac.bit(fl[5][0], which == 0))
        which = 0;
    else if (ac.bit(fl[5][1], which == 1))
        which = 1;
    else
        which = 2;
    u8 md5[16];
    if (which == 0)
        memcpy(md5, fs.md5sum, 16);
    else if (which == 1)
        memset(md5, 0, 16);
    else
        for (int i = 0; i < 16; i++) md5[i] = (u8)ac.raw(d[18 + i], 8);
    if (ac.dec) {
        d[0] = (u8)(minbs >> 8);
        d[1] = (u8)minbs;
        d[2] = (u8)(maxbs >> 8);
        d[3] = (u8)maxbs;
        wr_be24(d + 4, (u32)minfs);
        wr_be24(d + 7, (u32)maxfs);
        const StreamParams& sp = L.sp;
        d[10] = (u8)(sp.rate >> 12);
        d[11] = (u8)(sp.rate >> 4);
        d[12] = (u8)(((sp.rate & 15) << 4) | ((sp.nch - 1) << 1) | ((sp.bps - 1) >> 4));
        d[13] = (u8)((((sp.bps - 1) & 15) << 4) | ((total >> 32) & 15));
        wr_be32(d + 14, (u32)total);
        memcpy(d + 18, md5, 16);
        out->write_at(si.off + 4, d, 34);
    }
    for (size_t i = 1; i < L.blocks.n; i++)
        if (L.blocks[i].type == 3 && L.blocks[i].structured) code_seektable(ac, L.blocks[i], fs, L.sp, out);
    // 16-bit check of the PCM (detects a .wav that does not belong to this .meta)
    u32 chk = (fs.md5sum[0] << 8) | fs.md5sum[1];
    u32 got = (u32)ac.raw(chk, 16);
    return got == chk;
}

