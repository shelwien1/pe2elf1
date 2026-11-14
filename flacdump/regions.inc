// flacdump: Regions - coding byte regions with their cuts (embedded files, decoded base64)
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// cut lists come from a static pool; regions nest at most twice (region -> decoded base64 value)
enum { CUT_DEPTH = 3 };
static Cut g_cutpool[CUT_DEPTH][MAXCUTS];
static int g_cutdepth;

// a cut can be base64 text whose decoded data is a region again: static members instead of
// forward declarations
struct Regions {
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
        g_ext.name(name, sizeof(name), ++g_ext.count, c.ext);
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

    // Code a byte region whose length is known on both sides, extracting embedded files
    // and decoding base64 text.
    static bool code_region(AC& ac, u8* data, size_t len, int kind, bool prime) {
        if (g_cutdepth >= CUT_DEPTH) die("internal: regions nested too deep");
        Cut* cuts = g_cutpool[g_cutdepth++];
        memset(cuts, 0, sizeof(Cut) * MAXCUTS);
        int nc = 0;
        bool extract = !g_ext.keep_inline && g_ext.prefix;
        if (!ac.dec && (extract || kind == 0)) nc = CutFinder::find_cuts(data, len, kind, cuts, extract);
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
};
