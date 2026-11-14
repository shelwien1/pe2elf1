// flacdump: file reading, CRC-8 / CRC-16 as used by FLAC
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

static bool read_file(const char* name, Buf& b, size_t pad) {
    FILE* f = fopen(name, "rb");
    if (!f) return false;
    fseek64(f, 0, SEEK_END);
    s64 sz = ftell64(f);
    fseek64(f, 0, SEEK_SET);
    if (sz < 0) {
        fclose(f);
        return false;
    }
    b.reserve((size_t)sz + pad);
    size_t got = fread(b.p, 1, (size_t)sz, f);
    fclose(f);
    if (got != (size_t)sz) return false;
    b.n = (size_t)sz;
    memset(b.p + b.n, 0, pad);
    return true;
}

static u8 g_crc8[256];
static u16 g_crc16[256];

static void init_crc() {
    for (int i = 0; i < 256; i++) {
        u32 c = i;
        for (int j = 0; j < 8; j++) c = (c & 0x80) ? ((c << 1) ^ 0x07) : (c << 1);
        g_crc8[i] = (u8)c;
        u32 d = i << 8;
        for (int j = 0; j < 8; j++) d = (d & 0x8000) ? ((d << 1) ^ 0x8005) : (d << 1);
        g_crc16[i] = (u16)d;
    }
}
static u8 crc8(const u8* p, size_t n) {
    u8 c = 0;
    for (size_t i = 0; i < n; i++) c = g_crc8[c ^ p[i]];
    return c;
}
static u16 crc16(const u8* p, size_t n) {
    u16 c = 0;
    for (size_t i = 0; i < n; i++) c = (u16)((c << 8) ^ g_crc16[(c >> 8) ^ p[i]]);
    return c;
}
