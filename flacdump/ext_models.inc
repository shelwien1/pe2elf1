// flacdump: ExtModels - bit models for cut lists and embedded files in byte regions
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct ExtModels {
    UModel n, off, len, line, dlen;
    BM ext[16];
    BM isb64, pad, crlf, fin;
    BM kind[4];
    BM wide[4];
    void init() {
        model_init(*this);
        n.len[0].p = 3000;  // most regions contain no embedded files
        n.len[0].n = 4;
    }
};
static ExtModels g_extm;

