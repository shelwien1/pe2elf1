// flacdump: embedded files (cover art etc.) written as <prefix>.<n>.<ext>: types, image lengths
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

// Embedded files (cover art etc.): stored as separate files <prefix>.<n>.<ext>
// instead of inside the .meta. The .meta keeps their position, size, type and
// a 16-bit CRC; the bytes around them are coded normally.

struct ExtFiles {
    const char* prefix;  // file name prefix (out_prefix / inp_prefix)
    int count;           // files referenced so far (numbering, same order in c and d)
    int keep_inline;     // -i: do not extract
    u64 bytes;           // total extracted bytes
};
static ExtFiles g_ext = {nullptr, 0, 0, 0};

static const char* k_ext_name[16] = {"bin", "jpg", "png", "gif", "bmp", "webp", "tif", "pdf",
                                     "zip", "gz",  "xz",  "7z",  "ogg", "flac", "txt",  "bin"};
static inline bool ext_is_image(int e) { return e >= 1 && e <= 6; }

static int sniff_ext(const u8* p, size_t n) {
    if (n >= 3 && p[0] == 0xFF && p[1] == 0xD8 && p[2] == 0xFF) return 1;
    if (n >= 8 && !memcmp(p, "\x89PNG\r\n\x1a\n", 8)) return 2;
    if (n >= 6 && (!memcmp(p, "GIF87a", 6) || !memcmp(p, "GIF89a", 6))) return 3;
    if (n >= 2 && p[0] == 'B' && p[1] == 'M') return 4;
    if (n >= 12 && !memcmp(p, "RIFF", 4) && !memcmp(p + 8, "WEBP", 4)) return 5;
    if (n >= 4 && (!memcmp(p, "II*\0", 4) || !memcmp(p, "MM\0*", 4))) return 6;
    if (n >= 5 && !memcmp(p, "%PDF-", 5)) return 7;
    if (n >= 4 && !memcmp(p, "PK\3\4", 4)) return 8;
    if (n >= 3 && p[0] == 0x1F && p[1] == 0x8B && p[2] == 8) return 9;
    if (n >= 6 && !memcmp(p, "\xFD" "7zXZ\0", 6)) return 10;
    if (n >= 6 && !memcmp(p, "7z\xBC\xAF\x27\x1C", 6)) return 11;
    if (n >= 4 && !memcmp(p, "OggS", 4)) return 12;
    if (n >= 4 && !memcmp(p, "fLaC", 4)) return 13;
    if (n > 0) {
        size_t i = 0;
        while (i < n && ((p[i] >= 0x20 && p[i] < 0x7F) || p[i] == '\t' || p[i] == '\r' || p[i] == '\n')) i++;
        if (i == n) return 14;
    }
    return 0;
}

// length of a JPEG starting at p (0 if not a well-formed JPEG)
static size_t jpeg_len(const u8* p, size_t n) {
    if (n < 4 || p[0] != 0xFF || p[1] != 0xD8 || p[2] != 0xFF) return 0;
    size_t i = 2;
    while (i + 1 < n) {
        if (p[i] != 0xFF) return 0;
        while (i + 1 < n && p[i + 1] == 0xFF) i++;  // fill bytes
        if (i + 1 >= n) return 0;
        u8 m = p[i + 1];
        i += 2;
        if (m == 0xD9) return i;  // EOI
        if ((m >= 0xD0 && m <= 0xD7) || m == 0x01) continue;
        if (m == 0x00 || i + 2 > n) return 0;
        size_t sl = (p[i] << 8) | p[i + 1];
        if (sl < 2 || i + sl > n) return 0;
        i += sl;
        if (m == 0xDA) {  // entropy coded data until the next marker
            while (i + 1 < n) {
                if (p[i] == 0xFF && p[i + 1] != 0x00 && !(p[i + 1] >= 0xD0 && p[i + 1] <= 0xD7)) break;
                i++;
            }
        }
    }
    return 0;
}

static size_t png_len(const u8* p, size_t n) {
    if (n < 8 || memcmp(p, "\x89PNG\r\n\x1a\n", 8)) return 0;
    size_t i = 8;
    while (i + 12 <= n) {
        u32 cl = rd_be32(p + i);
        if (cl > n || i + 12 + (size_t)cl > n) return 0;
        bool iend = !memcmp(p + i + 4, "IEND", 4);
        i += 12 + cl;
        if (iend) return i;
    }
    return 0;
}
