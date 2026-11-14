// flacdump: SpanList - value spans and tag extents found while parsing ID3v2 / APEv2 tags
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
    void clear() { n = nt = 0; }
    // a value worth looking at: text of 64+ characters, binary objects of 16+ bytes
    void add(size_t off, size_t len, int type = 0) {
        if (n < 512 && len >= (type == 3 ? 16u : 64u)) s[n++] = {off, len, type};
    }
    void add_tag(size_t off, size_t len) {
        if (nt < 16) tag[nt++] = {off, len, 0};
    }
    bool in_tag(size_t i) const {
        for (int k = 0; k < nt; k++)
            if (i >= tag[k].off && i < tag[k].off + tag[k].len) return true;
        return false;
    }
};
