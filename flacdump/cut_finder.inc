// flacdump: CutFinder - finds the parts of a byte region that are cut out: embedded files
// (pictures, images, objects in tags) and base64 text that is decoded (encoder only)
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct Cut {
    u64 off, len;
    int ext;
    int b64;     // region bytes are base64 text of 'dn' decoded bytes
    int wide;    // for b64: text is UTF-16 (1 = little endian, 2 = big endian)
    int kind;    // for b64: 1 PICTURE structure, 2 image, 3 other binary
    B64Fmt fmt;
    size_t dn;
};
enum { MAXCUTS = 64, MINCUT = 256 };

static Cut g_picture_cut[MAXCUTS];  // a decoded value parsed as a FLAC PICTURE structure
static SpanList g_spans;            // tag values of the region being scanned

// the functions call each other (a decoded base64 value is checked for a PICTURE structure),
// so they are static members: no forward declarations needed
struct CutFinder {
    // should decoded base64 data replace its text? (sets kind)
    static bool b64_worth(const u8* dec, size_t dn, const B64Fmt& f, bool extract, int& kind) {
        if (find_cuts(dec, dn, 1, g_picture_cut, false))
            kind = 1;
        else if (ext_is_image(sniff_ext(dec, dn)) && dn >= MINCUT)
            kind = 2;
        else
            kind = 3;
        if (extract && dn >= MINCUT) return true;
        // the base64 text itself costs ~6 bits per character
        return ByteCM::cost(dec, dn) + 24 < 6.0 * (double)b64_len(dn, f.pad);
    }

    // try to take d[s..e) as base64 text; also without trailing NULs / line breaks and after "KEY="
    static bool b64_try(const u8* d, size_t s, size_t e, bool extract, Cut& c) {
        c.b64 = 0;
        while (e > s && d[e - 1] == 0) e--;
        size_t st = s;
        while (st < e && (d[st] == '\r' || d[st] == '\n')) st++;
        for (int pass = 0; pass < 2; pass++) {
            if (pass) {  // "KEY=base64": start after the last '=' followed by base64 characters
                size_t k = e;
                while (k > st && !(d[k - 1] == '=' && k < e && is_b64c(d[k]))) k--;
                if (k <= st) break;
                st = k;
            }
            if (e - st < 64) continue;
            if (is_hex_text(d + st, e - st)) continue;
            for (int t = 0; t < 3; t++) {
                size_t en = e;
                for (int u = 0; u < t && en > st && (d[en - 1] == '\r' || d[en - 1] == '\n'); u++) en--;
                if (t && en == e) break;
                if (!b64f_parse(d + st, en - st, g_b64scan, B64MAX, c.dn, c.fmt)) continue;
                if (!b64_worth(g_b64scan, c.dn, c.fmt, extract, c.kind)) continue;
                c.b64 = 1;
                c.wide = 0;
                c.off = st;
                c.len = en - st;
                c.ext = 0;
                return true;
            }
        }
        return false;
    }

    // same for UTF-16 text (wide = 1 little endian, 2 big endian): characters above 0xFF break the text
    static bool b64_try_wide(const u8* d, size_t s, size_t e, int wide, bool extract, Cut& c) {
        size_t nch = (e - s) / 2;
        c.b64 = 0;
        if (nch < 64 || nch > B64TEXTMAX) return false;
        for (size_t k = 0; k < nch; k++) {
            u8 lo = d[s + 2 * k + (wide == 2)], hi = d[s + 2 * k + (wide == 1)];
            g_b64narrow[k] = hi ? 0xFF : lo;
        }
        bool ok = b64_try(g_b64narrow, 0, nch, extract, c);
        if (ok) {
            c.wide = wide;
            c.off = s + 2 * c.off;
            c.len = 2 * c.len;
        }
        return ok;
    }

    // base64 text run in an unparsed raw region starting at d[i]; returns run length to skip
    static size_t find_b64_run(const u8* d, size_t n, size_t i, bool extract, Cut& c) {
        size_t j = i;
        while (j < n && (is_b64c(d[j]) || d[j] == '=' || d[j] == '\r' || d[j] == '\n')) j++;
        size_t run = j - i;
        c.b64 = 0;
        if (run < 64) return run ? run : 1;
        for (size_t s0 = i; s0 < i + 4 && s0 + 64 <= j; s0++)
            if (is_b64c(d[s0]) && b64_try(d, s0, j, extract, c)) break;
        return run;
    }

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
        SpanList& L = g_spans;
        L.clear();
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
            } else if (is_b64c(d[i]) && !L.in_tag(i)) {
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
};
