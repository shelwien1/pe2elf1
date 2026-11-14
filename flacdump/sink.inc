// flacdump: Sink - output of the restored FLAC (file, or compare against the original)
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct Sink {
    FILE* f;
    const u8* ref;
    u64 ref_n;
    bool mismatch;
    u64 pos;
    void write_at(u64 off, const u8* p, size_t n) {
        if (f) {
            if (off != pos) fseek64(f, (s64)off, SEEK_SET);
            fwrite(p, 1, n, f);
            pos = off + n;
        } else {
            if (off + n > ref_n || memcmp(ref + off, p, n)) mismatch = true;
            pos = off + n;
        }
    }
};
