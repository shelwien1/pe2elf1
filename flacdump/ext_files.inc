// flacdump: ExtFiles - numbering and naming of the embedded files <prefix>.<n>.<ext>
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct ExtFiles {
    const char* prefix;  // file name prefix (out_prefix / inp_prefix)
    int count;           // files referenced so far (numbering, same order in c and d)
    int keep_inline;     // -i: do not extract
    u64 bytes;           // total extracted bytes
    // file name of embedded file number num with extension type ext: <prefix>.<num>.<ext>
    void name(char* buf, size_t bn, int num, int ext) const {
        snprintf(buf, bn, "%s.%d.%s", prefix, num, k_ext_name[ext & 15]);
    }
};
static ExtFiles g_ext = {nullptr, 0, 0, 0};
