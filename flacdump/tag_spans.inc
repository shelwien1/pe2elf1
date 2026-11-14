// flacdump: ID3v2 / APEv2 tag parsing: exact spans of text values, objects and pictures
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// ---- tag structures: exact spans of text values (ID3v2 text/comment frames, APEv2 text items) ----
struct Span {
    size_t off, len;
    int type;  // 0 text, 1 UTF-16LE text, 2 UTF-16BE text, 3 binary object
};
struct SpanList {
    Span s[512];
    int n;
    Span tag[16];  // parsed tag structures (no raw base64 scanning inside)
    int nt;
};
static void add_span(SpanList& L, size_t off, size_t len, int type = 0) {
    if (L.n < 512 && len >= (type == 3 ? 16u : 64u)) L.s[L.n++] = {off, len, type};
}
static inline u32 syncsafe32(const u8* p) { return ((p[0] & 0x7F) << 21) | ((p[1] & 0x7F) << 14) | ((p[2] & 0x7F) << 7) | (p[3] & 0x7F); }

// position after an ID3 string terminated by 0 (encodings 0, 3) or 00 00 (UTF-16, encodings 1, 2)
static size_t id3_skip_str(const u8* d, size_t v, size_t ve, int enc) {
    if (v >= ve) return ve;
    if (enc == 1 || enc == 2) {
        size_t k = v;
        while (k + 1 < ve && (d[k] || d[k + 1])) k += 2;
        return Min(k + 2, ve);
    }
    while (v < ve && d[v]) v++;
    return Min(v + 1, ve);
}

// ID3v2 tag at d[p]: collects text values (any encoding), GEOB objects and APIC pictures;
// returns tag size (0 if not a tag)
static size_t id3_spans(const u8* d, size_t n, size_t p, SpanList& L) {
    if (p + 10 > n || memcmp(d + p, "ID3", 3)) return 0;
    int ver = d[p + 3], fl = d[p + 5];
    if (ver < 2 || ver > 4 || d[p + 4] == 0xFF || (d[p + 6] | d[p + 7] | d[p + 8] | d[p + 9]) & 0x80) return 0;
    size_t size = 10 + syncsafe32(d + p + 6) + ((ver == 4 && (fl & 0x10)) ? 10 : 0);
    if (p + size > n) return 0;
    if (fl & 0x80) return size;  // unsynchronised: values are not plain text
    size_t q = p + 10, end = p + 10 + syncsafe32(d + p + 6);
    if (fl & 0x40) {  // extended header
        if (q + 4 > end) return size;
        q += ver == 4 ? syncsafe32(d + q) : rd_be32(d + q) + 4;
    }
    while (q + (ver == 2 ? 6 : 10) <= end) {
        const u8* h = d + q;
        if (!h[0]) break;  // padding
        size_t fs, hl;
        int fflags = 0;
        bool txxx, text, comm;
        if (ver == 2) {
            fs = rd_be24(h + 3);
            hl = 6;
            txxx = !memcmp(h, "TXX", 3);
            comm = !memcmp(h, "COM", 3);
        } else {
            fs = ver == 4 ? syncsafe32(h + 4) : rd_be32(h + 4);
            fflags = (h[8] << 8) | h[9];
            hl = 10;
            txxx = !memcmp(h, "TXXX", 4);
            comm = !memcmp(h, "COMM", 4);
        }
        text = h[0] == 'T';
        bool geob = ver == 2 ? !memcmp(h, "GEO", 3) : !memcmp(h, "GEOB", 4);
        bool apic = ver == 2 ? !memcmp(h, "PIC", 3) : !memcmp(h, "APIC", 4);
        if (q + hl + fs > end) break;
        size_t v = q + hl, ve = q + hl + fs;
        bool plain = ver == 2 || !(ver == 3 ? (fflags & 0xC0) : (fflags & 0x0F));
        int enc = fs > 1 ? d[v] : -1;
        if (plain && enc >= 0 && enc <= 3) {
            v++;
            if (text || comm) {
                if (comm) v += 3;
                if (txxx || comm) v = id3_skip_str(d, v, ve, enc);  // description
                int type = 0;
                if (enc == 1 || enc == 2) {  // UTF-16: BOM decides the byte order (enc 2: big endian)
                    type = enc == 2 ? 2 : 1;
                    if (v + 2 <= ve && d[v] == 0xFF && d[v + 1] == 0xFE) {
                        type = 1;
                        v += 2;
                    } else if (v + 2 <= ve && d[v] == 0xFE && d[v + 1] == 0xFF) {
                        type = 2;
                        v += 2;
                    }
                    if ((ve - v) & 1) ve--;
                }
                if (v < ve) add_span(L, v, ve - v, type);
            } else if (geob) {  // MIME, filename, description, object
                v = id3_skip_str(d, v, ve, 0);
                v = id3_skip_str(d, v, ve, enc);
                v = id3_skip_str(d, v, ve, enc);
                if (v < ve) add_span(L, v, ve - v, 3);
            } else if (apic) {  // MIME (v2.2: 3 char format), picture type, description, data
                v = ver == 2 ? v + 3 : id3_skip_str(d, v, ve, 0);
                v = id3_skip_str(d, v + 1, ve, enc);
                if (v < ve) add_span(L, v, ve - v, 3);
            }
        }
        q = ve;
    }
    if (L.nt < 16) L.tag[L.nt++] = {p, size};
    return size;
}

// APEv2 tag whose header or footer starts at d[p] ("APETAGEX"); returns 1 if parsed
static bool ape_spans(const u8* d, size_t n, size_t p, SpanList& L) {
    if (p + 32 > n || memcmp(d + p, "APETAGEX", 8)) return false;
    u32 size = rd_le32(d + p + 12), cnt = rd_le32(d + p + 16), fl = rd_le32(d + p + 20);
    size_t items, end;
    if (fl & (1u << 29)) {  // header: items follow
        items = p + 32;
        end = items + size - 32;
    } else {  // footer: items precede
        if (size < 32 || size > p + 32) return false;
        items = p + 32 - size;
        end = p;
    }
    if (end > n || items > end) return false;
    size_t q = items;
    for (u32 i = 0; i < cnt && q + 9 <= end; i++) {
        u32 vl = rd_le32(d + q), ifl = rd_le32(d + q + 4);
        size_t k = q + 8;
        while (k < end && d[k]) k++;
        size_t v = k + 1;
        if (v + vl > end) break;
        int it = (ifl >> 1) & 3;
        if (it == 0)
            add_span(L, v, vl);  // UTF-8 text item
        else if (it == 1) {      // binary item, by convention "filename\0data"
            size_t z = v;
            while (z < v + vl && z < v + 256 && d[z]) z++;
            if (z < v + vl && z < v + 256 && !d[z])
                add_span(L, z + 1, v + vl - z - 1, 3);
            else
                add_span(L, v, vl, 3);
        }
        q = v + vl;
    }
    if (L.nt < 16) L.tag[L.nt++] = {items, end - items};
    return true;
}

static bool in_tag(const SpanList& L, size_t i) {
    for (int k = 0; k < L.nt; k++)
        if (i >= L.tag[k].off && i < L.tag[k].off + L.tag[k].len) return true;
    return false;
}
