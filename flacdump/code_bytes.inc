// flacdump: byte strings, coded with the CM model or stored
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

static const char* k_prime_text =
    "reference libFLAC 1.2.1 20070917reference libFLAC 1.3.0 20130526reference libFLAC 1.3.1 20141125"
    "reference libFLAC 1.3.2 20170101reference libFLAC 1.3.3 20190804reference libFLAC 1.3.4 20220220"
    "reference libFLAC 1.4.0 20220909reference libFLAC 1.4.1 20220922reference libFLAC 1.4.2 20221022"
    "reference libFLAC 1.4.3 20230623reference libFLAC 1.5.0 20250211reference libFLAC 1.1.4 20070213"
    "reference libFLAC 1.1.2 20050205reference libFLAC 1.2.0 20070715Lavf58.76.100Lavf60.16.100Lavf61.7.100"
    "TITLE=ARTIST=ALBUM=DATE=TRACKNUMBER=TRACKTOTAL=DISCNUMBER=GENRE=COMMENT=ALBUMARTIST=COMPOSER="
    "ENCODER=REPLAYGAIN_TRACK_GAIN=REPLAYGAIN_TRACK_PEAK=REPLAYGAIN_ALBUM_GAIN=REPLAYGAIN_ALBUM_PEAK=";

// Code a byte string given as pieces (concatenation, total length known to both sides).
// Chooses CM or stored. Pieces avoid copying the bytes around extracted parts of a region.
struct Piece {
    u8* p;
    size_t n;
};
static void code_pieces(AC& ac, const Piece* pc, int np, bool prime) {
    size_t len = 0;
    for (int k = 0; k < np; k++) len += pc[k].n;
    if (len == 0) return;
    int savecat = ac.cat;
    int stored = 0;
    ByteCM cm;
    cm.init();
    if (prime) {
        const char* s = k_prime_text;
        for (size_t i = 0; s[i]; i++) cm.byte(nullptr, (u8)s[i], nullptr);
        cm.c1 = cm.c2 = cm.c3 = 0;
    }
    if (!ac.dec) {
        // measure CM cost on a copy of the model
        if (len > 64) {
            ByteCM t;
            t.init();
            if (prime) {
                memcpy(t.t0, cm.t0, 256 * 2);
                memcpy(t.t1, cm.t1, 65536 * 2);
                memcpy(t.t2, cm.t2, ByteCM::H2 * 2);
                memcpy(t.t3, cm.t3, ByteCM::H3 * 2);
                memcpy(t.w, cm.w, sizeof(cm.w));
            }
            double cost = 0;
            for (int k = 0; k < np; k++)
                for (size_t i = 0; i < pc[k].n; i++) t.byte(nullptr, pc[k].p[i], &cost);
            t.release();
            stored = cost > (double)len * 8;
        }
    }
    stored = ac.code(stored, 32768);
    if (stored) ac.cat = C_RAW;
    for (int k = 0; k < np; k++) {
        u8* d = pc[k].p;
        for (size_t i = 0; i < pc[k].n; i++) d[i] = stored ? (u8)ac.raw(d[i], 8) : (u8)cm.byte(&ac, d[i], nullptr);
    }
    cm.release();
    ac.cat = savecat;
}
static void code_bytes(AC& ac, u8* data, size_t len, bool prime) {
    Piece pc = {data, len};
    code_pieces(ac, &pc, 1, prime);
}
