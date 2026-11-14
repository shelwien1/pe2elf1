// flacdump: VorbisCoder - structured VORBIS_COMMENT coding: vendor from a dictionary, lengths as
// numbers, text with a shared CM model, base64 values decoded
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct VorbisCoder {
    AC& ac;
    Models& M;
    ByteCM& cm;  // shared text model
    MBlock& b;
    u32 p;  // position in the block (d: being filled)

    bool put32(u32 x) {
        if (p + 4 > b.len) return false;
        wr_le32(b.data + p, x);
        p += 4;
        return true;
    }
    // n bytes of text with the shared CM model (its order-1..3 contexts restart for each string)
    bool text(u32 n) {
        if (p + n > b.len) return false;
        u8* d = b.data + p;
        cm.c1 = cm.c2 = cm.c3 = 0;
        for (u32 i = 0; i < n; i++) d[i] = (u8)cm.byte(&ac, d[i], nullptr);
        p += n;
        return true;
    }
    // vendor string: an index into k_vendors, or its length and text
    bool vendor() {
        u32 vl = ac.dec ? 0 : rd_le32(b.data);
        int vi = NVENDORS;
        if (!ac.dec)
            for (int i = 0; i < NVENDORS; i++)
                if (strlen(k_vendors[i]) == vl && !memcmp(b.data + 4, k_vendors[i], vl)) vi = i;
        vi = ac.tree(M.vendor_t, 5, vi);
        if (vi < NVENDORS) {
            vl = (u32)strlen(k_vendors[vi]);
            if (!put32(vl) || p + vl > b.len) return false;
            memcpy(b.data + p, k_vendors[vi], vl);
            p += vl;
            return true;
        }
        vl = (u32)code_uint(ac, M.len_m, vl);
        return put32(vl) && text(vl);
    }
    // encoder: is the value of the comment c[0..l) ("KEY=value") base64 that is worth decoding?
    // Sets the key length (with '='), the text layout, the decoded data (g_b64dec_v, dn) and its kind.
    static int b64_detect(const u8* c, u32 l, u32& klen, B64Fmt& fmt, size_t& dn, int& kind) {
        if (l < 20) return 0;
        while (klen < l && c[klen] != '=') klen++;
        if (klen >= l || klen >= 256 || l - klen - 1 < 16) return 0;
        klen++;
        const u8* v = c + klen;
        u32 vn = l - klen;
        if (is_hex_text(v, vn) || !b64f_parse(v, vn, g_b64dec_v, B64MAX, dn, fmt)) return 0;
        return CutFinder::b64_worth(g_b64dec_v, dn, fmt, g_ext.prefix && !g_ext.keep_inline, kind);
    }
    // one comment: its length, then the text; for a base64 value the key text, the layout and the
    // decoded data instead (coded as a region: pictures and large blobs become external files,
    // small blobs are coded as binary if that is cheaper)
    bool comment() {
        u32 l = (u32)code_uint(ac, M.clen_m, ac.dec ? 0 : rd_le32(b.data + p));
        if (!put32(l) || p + l > b.len) return false;
        int kind = 0;
        B64Fmt fmt = {0, 0, 0, 0};
        u32 klen = 0;
        size_t dn = 0;
        int b64 = ac.dec ? 0 : b64_detect(b.data + p, l, klen, fmt, dn, kind);
        if (!ac.bit(M.mflag[3], b64)) return text(l);
        klen = (u32)code_uint(ac, M.len_m, klen);
        if (klen > l) return false;
        bool good = text(klen);
        code_b64fmt(ac, fmt, M.mflag[4], M.len_m, M.mflag[5], M.mflag[6]);
        kind = (int)ac.tree(M.b64kind, 2, (u32)kind);
        dn = (size_t)code_uint(ac, M.len_m, dn);
        if (!good || kind == 0 || dn > B64MAX || b64f_len(dn, fmt) != l - klen) return false;
        good = Regions::code_region(ac, g_b64dec_v, dn, kind == 1 ? 1 : 2, false);
        if (good && ac.dec) b64f_encode(g_b64dec_v, dn, fmt, b.data + p);
        p += l - klen;
        return good;
    }
    // the whole block (stored as bytes if it is not a well-formed VORBIS_COMMENT)
    static bool code(AC& ac, Models& M, MBlock& b) {
        int ok = ac.dec ? 0 : vorbis_ok(b.data, b.len);
        ok = ac.bit(M.mflag[2], ok);
        if (!ok) {
            code_bytes(ac, b.data, b.len, true);
            return true;
        }
        ByteCM cm;
        cm.init();
        for (const char* t = k_prime_text; *t; t++) cm.byte(nullptr, (u8)*t, nullptr);
        VorbisCoder v = {ac, M, cm, b, 0};
        bool good = v.vendor();
        u32 cnt = 0;
        if (good) {
            cnt = (u32)code_uint(ac, M.num_m, ac.dec ? 0 : rd_le32(b.data + v.p));
            good = v.put32(cnt);
        }
        for (u32 i = 0; i < cnt && good; i++) good = v.comment();
        cm.release();
        return good && v.p == b.len;
    }
};
