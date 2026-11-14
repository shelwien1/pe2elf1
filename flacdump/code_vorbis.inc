// flacdump: code_vorbis - structured VORBIS_COMMENT coding (vendor, lengths, text, base64 values)
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// structured VORBIS_COMMENT coding: vendor from a dictionary, lengths as numbers, text with a shared CM
static bool code_vorbis(AC& ac, Models& M, MBlock& b) {
    int ok = ac.dec ? 0 : vorbis_ok(b.data, b.len);
    ok = ac.bit(M.mflag[2], ok);
    if (!ok) {
        code_bytes(ac, b.data, b.len, true);
        return true;
    }
    ByteCM cm;
    cm.init();
    for (const char* t = k_prime_text; *t; t++) cm.byte(nullptr, (u8)*t, nullptr);
    u32 p = 0;
    auto put32 = [&](u32 v) {
        if (p + 4 > b.len) return false;
        wr_le32(b.data + p, v);
        p += 4;
        return true;
    };
    auto text = [&](u32 n) {
        if (p + n > b.len) return false;
        cm.c1 = cm.c2 = cm.c3 = 0;
        for (u32 i = 0; i < n; i++) b.data[p + i] = (u8)cm.byte(&ac, b.data[p + i], nullptr);
        p += n;
        return true;
    };
    // vendor
    u32 vl = ac.dec ? 0 : rd_le32(b.data);
    int vi = NVENDORS;
    if (!ac.dec)
        for (int i = 0; i < NVENDORS; i++)
            if (strlen(k_vendors[i]) == vl && !memcmp(b.data + 4, k_vendors[i], vl)) vi = i;
    vi = ac.tree(M.vendor_t, 5, vi);
    bool good = true;
    if (vi < NVENDORS) {
        vl = (u32)strlen(k_vendors[vi]);
        good = put32(vl) && p + vl <= b.len;
        if (good) {
            memcpy(b.data + p, k_vendors[vi], vl);
            p += vl;
        }
    } else {
        vl = (u32)code_uint(ac, M.len_m, vl);
        good = put32(vl) && text(vl);
    }
    if (!good) {
        cm.release();
        return false;
    }
    u32 cnt = (u32)code_uint(ac, M.num_m, ac.dec ? 0 : rd_le32(b.data + p));
    if (!put32(cnt)) {
        cm.release();
        return false;
    }
    for (u32 i = 0; i < cnt && good; i++) {
        u32 l = (u32)code_uint(ac, M.clen_m, ac.dec ? 0 : rd_le32(b.data + p));
        good = put32(l);
        if (!good || p + l > b.len) {
            good = false;
            break;
        }
        // base64 value? Pictures / images / other binary are decoded; pictures and large
        // blobs become external files, small blobs are coded as binary if that is cheaper
        int b64 = 0, kind = 0;
        B64Fmt fmt = {0, 0, 0, 0};
        u32 klen = 0;
        size_t dn = 0;
        if (!ac.dec && l >= 20) {
            const u8* c = b.data + p;
            while (klen < l && c[klen] != '=') klen++;
            if (klen < l && klen < 256 && l - klen - 1 >= 16) {
                klen++;
                const u8* v = c + klen;
                u32 vn = l - klen;
                bool hex = true;
                for (u32 j = 0; j < vn && hex; j++)
                    hex = (v[j] >= '0' && v[j] <= '9') || (v[j] >= 'a' && v[j] <= 'f') || (v[j] >= 'A' && v[j] <= 'F') ||
                          v[j] == '\r' || v[j] == '\n';
                if (!hex && b64f_parse(v, vn, g_b64dec_v, B64MAX, dn, fmt))
                    b64 = b64_worth(g_b64dec_v, dn, fmt, g_ext.prefix && !g_ext.keep_inline, kind);
            }
        }
        b64 = ac.bit(M.mflag[3], b64);
        if (!b64) {
            good = text(l);
            continue;
        }
        klen = (u32)code_uint(ac, M.len_m, klen);
        if (klen > l) {
            good = false;
            break;
        }
        good = text(klen);
        fmt.pad = ac.bit(M.mflag[4], fmt.pad);
        fmt.line = (u32)code_uint(ac, M.len_m, fmt.line);
        if (fmt.line) {
            fmt.crlf = ac.bit(M.mflag[6], fmt.crlf);
            fmt.final_eol = ac.bit(M.mflag[7], fmt.final_eol);
        }
        kind = (int)ac.tree(M.b64kind, 2, (u32)kind);
        dn = (size_t)code_uint(ac, M.len_m, dn);
        if (!good || kind == 0 || dn > B64MAX || b64f_len(dn, fmt) != l - klen) {
            good = false;
            break;
        }
        good = code_region(ac, g_b64dec_v, dn, kind == 1 ? 1 : 2, false);
        if (good && ac.dec) b64f_encode(g_b64dec_v, dn, fmt, b.data + p);
        p += l - klen;
    }
    cm.release();
    return good && p == b.len;
}
