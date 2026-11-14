// flacdump: base64 text (pictures/binary in comments and APE/ID3 tags): layout, generation, parsing, detection
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// ---- base64 (pictures/binary embedded as text: comments, APE/ID3 tags) ----
static const char k_b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static size_t b64_len(size_t n, int pad) { return pad ? (n + 2) / 3 * 4 : (n * 4 + 2) / 3; }
// base64 text layout: padding, optional line wrapping
struct B64Fmt {
    int pad;
    u32 line;       // chars per line, 0 = single line
    int crlf;       // line break is CR LF (else LF)
    int final_eol;  // text ends with a line break
};
// decoded values larger than this stay text (bounds the static buffers below)
enum { B64MAX = 16 << 20, B64TEXTMAX = B64MAX / 3 * 4 + B64MAX / 8 };
static u8 g_b64scan[B64MAX];   // encoder: candidate values while searching
static u8 g_b64dec_v[B64MAX];  // decoded VORBIS_COMMENT value being coded
static u8 g_b64dec_r[B64MAX];  // decoded value of a cut in a raw region being coded
static u8 g_b64narrow[B64TEXTMAX];  // UTF-16 text narrowed to bytes

// layout of a base64 text: padding, line length; line break type and final line break if wrapped
static void code_b64fmt(AC& ac, B64Fmt& f, BM& pad, UModel& line, BM& crlf, BM& fin) {
    f.pad = ac.bit(pad, f.pad);
    f.line = (u32)code_uint(ac, line, f.line);
    if (f.line) {
        f.crlf = ac.bit(crlf, f.crlf);
        f.final_eol = ac.bit(fin, f.final_eol);
    } else
        f.crlf = f.final_eol = 0;
}
static size_t b64f_len(size_t n, const B64Fmt& f) {
    size_t t = b64_len(n, f.pad);
    if (!f.line || !t) return t;
    size_t lines = (t + f.line - 1) / f.line;
    return t + (lines - 1 + f.final_eol) * (f.crlf ? 2 : 1);
}
// generate the text of d[0..n) in layout f, one character at a time
template <class F> static void b64f_gen(const u8* d, size_t n, const B64Fmt& f, F&& put) {
    size_t t = b64_len(n, f.pad), col = 0, done = 0;
    auto emit = [&](u8 c) {
        put(c);
        done++;
        if (f.line && ++col == f.line && (done < t || f.final_eol)) {
            col = 0;
            if (f.crlf) put('\r');
            put('\n');
        }
    };
    size_t i = 0;
    for (; i + 3 <= n; i += 3) {
        u32 v = (d[i] << 16) | (d[i + 1] << 8) | d[i + 2];
        emit(k_b64[v >> 18]);
        emit(k_b64[(v >> 12) & 63]);
        emit(k_b64[(v >> 6) & 63]);
        emit(k_b64[v & 63]);
    }
    if (n - i == 1) {
        u32 v = d[i] << 16;
        emit(k_b64[v >> 18]);
        emit(k_b64[(v >> 12) & 63]);
        if (f.pad) {
            emit('=');
            emit('=');
        }
    } else if (n - i == 2) {
        u32 v = (d[i] << 16) | (d[i + 1] << 8);
        emit(k_b64[v >> 18]);
        emit(k_b64[(v >> 12) & 63]);
        emit(k_b64[(v >> 6) & 63]);
        if (f.pad) emit('=');
    }
    if (f.line && f.final_eol && col) {  // last line shorter than f.line
        if (f.crlf) put('\r');
        put('\n');
    }
}
static void b64f_encode(const u8* d, size_t n, const B64Fmt& f, u8* o) {
    b64f_gen(d, n, f, [&](u8 c) { *o++ = c; });
}
// UTF-16 text (wide = 1 little endian, 2 big endian)
static void b64f_encode_wide(const u8* d, size_t n, const B64Fmt& f, u8* o, int wide) {
    b64f_gen(d, n, f, [&](u8 c) {
        o[wide == 2] = c;
        o[wide == 1] = 0;
        o += 2;
    });
}
// strict parse incl. line wrapping into out[0..cap); true only if b64f_gen reproduces the text exactly
static bool b64f_parse(const u8* s, size_t m, u8* out, size_t cap, size_t& on, B64Fmt& f) {
    static s8 rev[256];
    if (!rev['B']) {
        for (int i = 0; i < 256; i++) rev[i] = -1;
        for (int i = 0; i < 64; i++) rev[(u8)k_b64[i]] = (s8)i;
    }
    on = 0;
    f.line = 0;
    f.crlf = 0;
    f.final_eol = 0;
    size_t e = 0;
    while (e < m && s[e] != '\r' && s[e] != '\n') e++;
    if (e < m) {
        if (e < 4) return false;
        f.line = (u32)e;
        f.crlf = s[e] == '\r';
        f.final_eol = s[m - 1] == '\n';
    }
    // characters without line breaks, trailing padding
    size_t chars = 0, np = 0;
    for (size_t i = 0; i < m; i++)
        if (s[i] != '\r' && s[i] != '\n') chars++;
    for (size_t i = m; i > 0 && np < 2;) {
        i--;
        if (s[i] == '\r' || s[i] == '\n') continue;
        if (s[i] != '=') break;
        np++;
    }
    if (!chars) return false;
    f.pad = np > 0 || (chars % 4) == 0;
    size_t q = chars - np;
    if ((np && (chars % 4)) || (q % 4) == 1 || q / 4 * 3 + 2 > cap) return false;
    u32 acc = 0;
    int bits = 0;
    size_t seen = 0;
    for (size_t i = 0; i < m && seen < q; i++) {
        if (s[i] == '\r' || s[i] == '\n') continue;
        int v = rev[s[i]];
        if (v < 0) return false;
        seen++;
        acc = (acc << 6) | (u32)v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out[on++] = (u8)(acc >> bits);
        }
    }
    if (b64f_len(on, f) != m) return false;
    size_t k = 0;
    bool same = true;
    b64f_gen(out, on, f, [&](u8 c) {
        if (same && (k >= m || s[k] != c)) same = false;
        k++;
    });
    return same && k == m;
}




// hex digits (and line breaks) only: such values stay text
static bool is_hex_text(const u8* v, size_t n) {
    for (size_t k = 0; k < n; k++)
        if (!((v[k] >= '0' && v[k] <= '9') || (v[k] >= 'a' && v[k] <= 'f') || (v[k] >= 'A' && v[k] <= 'F') ||
              v[k] == '\r' || v[k] == '\n'))
            return false;
    return true;
}
static inline bool is_b64c(u8 c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/';
}



