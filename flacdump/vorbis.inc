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
