// flacdump: VORBIS_COMMENT: vendor dictionary, validation, encoder hint, structured coding
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

static const char* k_vendors[] = {
    "reference libFLAC 1.1.0 20030126", "reference libFLAC 1.1.1 20041001", "reference libFLAC 1.1.2 20050205",
    "reference libFLAC 1.1.3 20061120", "reference libFLAC 1.1.4 20070213", "reference libFLAC 1.2.0 20070715",
    "reference libFLAC 1.2.1 20070917", "reference libFLAC 1.3.0 20130526", "reference libFLAC 1.3.1 20141125",
    "reference libFLAC 1.3.2 20170101", "reference libFLAC 1.3.3 20190804", "reference libFLAC 1.3.4 20220220",
    "reference libFLAC 1.4.0 20220909", "reference libFLAC 1.4.1 20220922", "reference libFLAC 1.4.2 20221022",
    "reference libFLAC 1.4.3 20230623", "reference libFLAC 1.5.0 20250211"};
enum { NVENDORS = sizeof(k_vendors) / sizeof(k_vendors[0]) };

static bool vorbis_ok(const u8* d, u32 len) {
    if (len < 8) return false;
    u32 vl = rd_le32(d);
    if ((u64)vl + 8 > len) return false;
    u32 p = 4 + vl;
    u32 cnt = rd_le32(d + p);
    p += 4;
    for (u32 i = 0; i < cnt; i++) {
        if (p + 4 > len) return false;
        u32 l = rd_le32(d + p);
        p += 4;
        if ((u64)p + l > len) return false;
        p += l;
    }
    return p == len;
}

// encoder hint from the VORBIS_COMMENT vendor string: 0 unknown, -1 ffmpeg, libFLAC version*100
static int vendor_hint(const u8* data, u32 len) {
    if (len < 4) return 0;
    u32 vl = rd_le32(data);
    if ((u64)vl + 4 > len) return 0;
    const char* v = (const char*)data + 4;
    if (vl >= 4 && (!memcmp(v, "Lavf", 4) || !memcmp(v, "Lavc", 4))) return -1;
    const char* pre = "reference libFLAC ";
    size_t pl = strlen(pre);
    if (vl > pl && !memcmp(v, pre, pl)) {
        int d[3] = {0, 0, 0}, k = 0;
        for (u32 j = (u32)pl; j < vl && k < 3; j++) {
            char c = v[j];
            if (c >= '0' && c <= '9')
                d[k] = d[k] * 10 + (c - '0');
            else if (c == '.')
                k++;
            else
                break;
        }
        if (d[0] == 0 && d[1] == 0) return 150;  // git builds
        return d[0] * 100 + d[1] * 10 + d[2];
    }
    return 0;
}

// position in a VORBIS_COMMENT block being coded (d: being filled)
struct VcCursor {
    AC* ac;
    ByteCM* cm;  // shared text model
    MBlock* b;
    u32 p;
};
static bool vc_put32(VcCursor& v, u32 x) {
    if (v.p + 4 > v.b->len) return false;
    wr_le32(v.b->data + v.p, x);
    v.p += 4;
    return true;
}
// n bytes of text with the shared CM model (its order-1..3 contexts restart for each string)
static bool vc_text(VcCursor& v, u32 n) {
    if (v.p + n > v.b->len) return false;
    ByteCM& cm = *v.cm;
    u8* d = v.b->data + v.p;
    cm.c1 = cm.c2 = cm.c3 = 0;
    for (u32 i = 0; i < n; i++) d[i] = (u8)cm.byte(v.ac, d[i], nullptr);
    v.p += n;
    return true;
}

// vendor string: an index into k_vendors, or its length and text
static bool vc_vendor(VcCursor& v, Models& M) {
    AC& ac = *v.ac;
    MBlock& b = *v.b;
    u32 vl = ac.dec ? 0 : rd_le32(b.data);
    int vi = NVENDORS;
    if (!ac.dec)
        for (int i = 0; i < NVENDORS; i++)
            if (strlen(k_vendors[i]) == vl && !memcmp(b.data + 4, k_vendors[i], vl)) vi = i;
    vi = ac.tree(M.vendor_t, 5, vi);
    if (vi < NVENDORS) {
        vl = (u32)strlen(k_vendors[vi]);
        if (!vc_put32(v, vl) || v.p + vl > b.len) return false;
        memcpy(b.data + v.p, k_vendors[vi], vl);
        v.p += vl;
        return true;
    }
    vl = (u32)code_uint(ac, M.len_m, vl);
    return vc_put32(v, vl) && vc_text(v, vl);
}

// encoder: is the value of the comment c[0..l) ("KEY=value") base64 that is worth decoding?
// Sets the key length (with '='), the text layout, the decoded data (g_b64dec_v, dn) and its kind.
static int vc_b64_detect(const u8* c, u32 l, u32& klen, B64Fmt& fmt, size_t& dn, int& kind) {
    if (l < 20) return 0;
    while (klen < l && c[klen] != '=') klen++;
    if (klen >= l || klen >= 256 || l - klen - 1 < 16) return 0;
    klen++;
    const u8* v = c + klen;
    u32 vn = l - klen;
    bool hex = true;
    for (u32 j = 0; j < vn && hex; j++)
        hex = (v[j] >= '0' && v[j] <= '9') || (v[j] >= 'a' && v[j] <= 'f') || (v[j] >= 'A' && v[j] <= 'F') ||
              v[j] == '\r' || v[j] == '\n';
    if (hex || !b64f_parse(v, vn, g_b64dec_v, B64MAX, dn, fmt)) return 0;
    return b64_worth(g_b64dec_v, dn, fmt, g_ext.prefix && !g_ext.keep_inline, kind);
}

// one comment: its length, then the text; for a base64 value the key text, the layout and the
// decoded data instead (coded as a region: pictures and large blobs become external files,
// small blobs are coded as binary if that is cheaper)
static bool vc_comment(VcCursor& v, Models& M) {
    AC& ac = *v.ac;
    MBlock& b = *v.b;
    u32 l = (u32)code_uint(ac, M.clen_m, ac.dec ? 0 : rd_le32(b.data + v.p));
    if (!vc_put32(v, l) || v.p + l > b.len) return false;
    int kind = 0;
    B64Fmt fmt = {0, 0, 0, 0};
    u32 klen = 0;
    size_t dn = 0;
    int b64 = ac.dec ? 0 : vc_b64_detect(b.data + v.p, l, klen, fmt, dn, kind);
    if (!ac.bit(M.mflag[3], b64)) return vc_text(v, l);
    klen = (u32)code_uint(ac, M.len_m, klen);
    if (klen > l) return false;
    bool good = vc_text(v, klen);
    fmt.pad = ac.bit(M.mflag[4], fmt.pad);
    fmt.line = (u32)code_uint(ac, M.len_m, fmt.line);
    if (fmt.line) {
        fmt.crlf = ac.bit(M.mflag[6], fmt.crlf);
        fmt.final_eol = ac.bit(M.mflag[7], fmt.final_eol);
    }
    kind = (int)ac.tree(M.b64kind, 2, (u32)kind);
    dn = (size_t)code_uint(ac, M.len_m, dn);
    if (!good || kind == 0 || dn > B64MAX || b64f_len(dn, fmt) != l - klen) return false;
    good = code_region(ac, g_b64dec_v, dn, kind == 1 ? 1 : 2, false);
    if (good && ac.dec) b64f_encode(g_b64dec_v, dn, fmt, b.data + v.p);
    v.p += l - klen;
    return good;
}

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
    VcCursor v = {&ac, &cm, &b, 0};
    bool good = vc_vendor(v, M);
    u32 cnt = 0;
    if (good) {
        cnt = (u32)code_uint(ac, M.num_m, ac.dec ? 0 : rd_le32(b.data + v.p));
        good = vc_put32(v, cnt);
    }
    for (u32 i = 0; i < cnt && good; i++) good = vc_comment(v, M);
    cm.release();
    return good && v.p == b.len;
}
