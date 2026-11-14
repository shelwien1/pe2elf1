// flacdump: byte regions: finding cuts (embedded files, base64 text) and coding them
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// find embedded files: kind 1 = FLAC PICTURE block (exact), 2 = whole buffer, otherwise scan for
// JPEG/PNG (if extracting) and base64 text
static int find_cuts(const u8* d, size_t n, int kind, Cut* cuts, bool extract) {
    int nc = 0;
    if (kind == 2) {  // the whole buffer is one file (e.g. base64-decoded cover art)
        if (n < MINCUT) return 0;
        cuts[0].off = 0;
        cuts[0].len = n;
        cuts[0].ext = sniff_ext(d, n);
        cuts[0].b64 = 0;
        return 1;
    }
    if (kind == 1) {
        // type, mime, description, w, h, depth, colors, data_len, data
        size_t p = 4;
        for (int k = 0; k < 2; k++) {
            if (p + 4 > n) return 0;
            u32 l = rd_be32(d + p);
            if ((u64)p + 4 + l > n) return 0;
            p += 4 + l;
        }
        p += 16;
        if (p + 4 > n) return 0;
        u32 dl = rd_be32(d + p);
        p += 4;
        if ((u64)p + dl != n || dl < MINCUT) return 0;
        cuts[0].off = p;
        cuts[0].len = dl;
        cuts[0].ext = sniff_ext(d + p, dl);
        cuts[0].b64 = 0;
        return 1;
    }
    // text values of ID3v2 / APEv2 tags found in the region
    static SpanList L;
    L.n = L.nt = 0;
    for (size_t i = 0; i + 10 <= n; i++) {
        if (d[i] == 'I' && d[i + 1] == 'D' && d[i + 2] == '3') {
            size_t t = id3_spans(d, n, i, L);
            if (t) i += t - 1;
        } else if (d[i] == 'A' && d[i + 1] == 'P' && !memcmp(d + i, "APETAGEX", 8))
            ape_spans(d, n, i, L);
    }
    for (int k = 0; k < L.n; k++)  // spans are sorted unless an APE footer preceded its items
        for (int m = k; m > 0 && L.s[m - 1].off > L.s[m].off; m--) {
            Span t = L.s[m];
            L.s[m] = L.s[m - 1];
            L.s[m - 1] = t;
        }
    int si = 0;
    for (size_t i = 0; i + 8 <= n && nc < MAXCUTS;) {
        while (si < L.n && L.s[si].off < i) si++;
        if (si < L.n && L.s[si].off == i) {  // a tag value
            Span sp = L.s[si++];
            Cut& c = cuts[nc];
            bool done = false;
            size_t se = sp.off + sp.len;
            if (sp.type == 0)
                done = b64_try(d, sp.off, se, extract, c);
            else if (sp.type <= 2)
                done = b64_try_wide(d, sp.off, se, sp.type, extract, c);
            else {
                // binary object: base64 text after a short binary header (e.g. Serato GEOB), else a file
                for (size_t k = 0; k < 8 && !done && sp.off + k < se; k++)
                    if (is_b64c(d[sp.off + k])) done = b64_try(d, sp.off + k, se, extract, c);
                if (!done && extract && sp.len >= MINCUT) {
                    c.off = sp.off;
                    c.len = sp.len;
                    c.ext = sniff_ext(d + sp.off, sp.len);
                    c.b64 = 0;
                    done = true;
                }
            }
            if (done) {
                i = c.off + c.len;
                nc++;
                continue;
            }
        }
        size_t l = 0;
        if (extract && d[i] == 0xFF && d[i + 1] == 0xD8 && d[i + 2] == 0xFF)
            l = jpeg_len(d + i, n - i);
        else if (extract && d[i] == 0x89 && d[i + 1] == 'P')
            l = png_len(d + i, n - i);
        if (l >= MINCUT) {
            cuts[nc].off = i;
            cuts[nc].len = l;
            cuts[nc].ext = sniff_ext(d + i, l);
            cuts[nc].b64 = 0;
            nc++;
            i += l;
        } else if (is_b64c(d[i]) && !in_tag(L, i)) {
            size_t r = find_b64_run(d, n, i, extract, cuts[nc]);
            if (cuts[nc].b64) {
                i = cuts[nc].off + cuts[nc].len;
                nc++;
            } else
                i += r;
        } else
            i++;
    }
    return nc;
}

static void ext_name(char* buf, size_t bn, int num, int ext) {
    snprintf(buf, bn, "%s.%d.%s", g_ext.prefix, num, k_ext_name[ext & 15]);
}

struct ExtModels {
    UModel n, off, len, line, dlen;
    BM ext[16];
    BM isb64, pad, crlf, fin;
    BM kind[4];
    BM wide[4];
};
static ExtModels g_extm;
static void extm_init() {
    model_init(g_extm);
    g_extm.n.len[0].p = 3000;  // most regions contain no embedded files
    g_extm.n.len[0].n = 4;
}

static bool code_region(AC& ac, u8* data, size_t len, int kind, bool prime);

// one cut: an external file, or base64 text replaced by its decoded data (coded recursively)
static bool code_cut(AC& ac, u8* data, Cut& c) {
    c.b64 = ac.bit(g_extm.isb64, c.b64);
    if (c.b64) {
        size_t dn = 0;
        if (!ac.dec) {  // decode again (the search phase does not keep decoded data)
            const u8* t = data + c.off;
            size_t tl = (size_t)c.len;
            if (c.wide) {
                tl /= 2;
                for (size_t k = 0; k < tl; k++) g_b64narrow[k] = data[c.off + 2 * k + (c.wide == 2)];
                t = g_b64narrow;
            }
            B64Fmt f2;
            if (!b64f_parse(t, tl, g_b64dec_r, B64MAX, dn, f2) || dn != c.dn) die("internal: base64 cut");
        }
        code_b64fmt(ac, c.fmt, g_extm.pad, g_extm.line, g_extm.crlf, g_extm.fin);
        c.kind = (int)ac.tree(g_extm.kind, 2, (u32)c.kind);
        c.wide = (int)ac.tree(g_extm.wide, 2, (u32)c.wide);
        dn = (size_t)code_uint(ac, g_extm.dlen, dn);
        size_t tl = b64f_len(dn, c.fmt);
        if (c.kind == 0 || c.wide == 3 || dn > B64MAX || tl * (c.wide ? 2 : 1) != c.len) return false;
        if (!code_region(ac, g_b64dec_r, dn, c.kind == 1 ? 1 : 2, false)) return false;
        if (ac.dec) {
            if (!c.wide)
                b64f_encode(g_b64dec_r, dn, c.fmt, data + c.off);
            else
                b64f_encode_wide(g_b64dec_r, dn, c.fmt, data + c.off, c.wide);
        }
        return true;
    }
    c.ext = (int)ac.tree(g_extm.ext, 4, (u32)c.ext);
    u16 crc = ac.dec ? 0 : crc16(data + c.off, (size_t)c.len);
    u16 want = (u16)ac.raw(crc, 16);
    char name[4200];
    ext_name(name, sizeof(name), ++g_ext.count, c.ext);
    if (!ac.dec) {
        FILE* f = fopen(name, "wb");
        if (!f || fwrite(data + c.off, 1, (size_t)c.len, f) != c.len || fclose(f)) die("cannot write embedded file");
        g_ext.bytes += c.len;
        return true;
    }
    FILE* f = fopen(name, "rb");
    if (!f) {
        fprintf(stderr, "flacdump: cannot open embedded file %s\n", name);
        return false;
    }
    size_t got = fread(data + c.off, 1, (size_t)c.len, f);
    int extra = fgetc(f);
    fclose(f);
    if (got != c.len || extra != EOF || crc16(data + c.off, (size_t)c.len) != want) {
        fprintf(stderr, "flacdump: embedded file %s does not match the .meta\n", name);
        return false;
    }
    return true;
}

// cut lists come from a static pool; regions nest at most twice (region -> decoded base64 value)
enum { CUT_DEPTH = 3 };
static Cut g_cutpool[CUT_DEPTH][MAXCUTS];
static int g_cutdepth;

// Code a byte region whose length is known on both sides, extracting embedded files
// and decoding base64 text.
static bool code_region(AC& ac, u8* data, size_t len, int kind, bool prime) {
    if (g_cutdepth >= CUT_DEPTH) die("internal: regions nested too deep");
    Cut* cuts = g_cutpool[g_cutdepth++];
    memset(cuts, 0, sizeof(Cut) * MAXCUTS);
    int nc = 0;
    bool extract = !g_ext.keep_inline && g_ext.prefix;
    if (!ac.dec && (extract || kind == 0)) nc = find_cuts(data, len, kind, cuts, extract);
    int savecat = ac.cat;
    ac.cat = C_META;
    nc = (int)code_uint(ac, g_extm.n, (u64)nc);
    bool ok = nc <= MAXCUTS;
    u64 pos = 0;
    for (int i = 0; i < nc && ok; i++) {
        Cut& c = cuts[i];
        u64 gap = code_uint(ac, g_extm.off, ac.dec ? 0 : c.off - pos);
        c.off = pos + gap;
        c.len = code_uint(ac, g_extm.len, c.len);
        if (c.off + c.len > len || !code_cut(ac, data, c)) ok = false;
        pos = c.off + c.len;
    }
    ac.cat = savecat;
    if (ok) {  // the remaining bytes: the gaps between the cuts
        Piece pc[MAXCUTS + 1];
        u64 p0 = 0;
        for (int i = 0; i < nc; i++) {
            pc[i] = {data + p0, (size_t)(cuts[i].off - p0)};
            p0 = cuts[i].off + cuts[i].len;
        }
        pc[nc] = {data + p0, (size_t)(len - p0)};
        code_pieces(ac, pc, nc + 1, prime);
    }
    g_cutdepth--;
    return ok;
}
