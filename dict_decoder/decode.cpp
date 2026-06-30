/*
 * decode - reference driver for DictDecoder.
 *
 * Equivalent to:   preprocess d <dict> <book1p> <out>
 *
 * It reads the dictionary, then streams the big-endian 16-bit symbols of
 * <book1p>, unpacking each one into an 8-bit buffer that is flushed to the
 * output file in chunks.
 */
#include "DictDecoder.h"

#include <cstdint>
#include <cstdio>
#include <vector>

static const size_t FLUSH_AT = 1u << 20;   /* flush the buffer past 1 MiB */

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "usage: %s <dict> <book1p> <out>\n", argv[0]);
        return 1;
    }
    const char *dict_file = argv[1];
    const char *in_file   = argv[2];
    const char *out_file  = argv[3];

    DictDecoder dec(dict_file);

    FILE *fi = fopen(in_file, "rb");
    if (!fi) { perror(in_file); return 1; }
    FILE *fo = fopen(out_file, "wb");
    if (!fo) { perror(out_file); fclose(fi); return 1; }

    std::vector<uint8_t> out;
    out.reserve(FLUSH_AT + 4096);

    uint8_t hdr[2];
    while (fread(hdr, 1, 2, fi) == 2) {
        uint16_t sym = (uint16_t(hdr[0]) << 8) | hdr[1];   /* big-endian */
        dec.unpack(sym, out);
        if (out.size() >= FLUSH_AT) {
            fwrite(out.data(), 1, out.size(), fo);
            out.clear();
        }
    }
    if (!out.empty())
        fwrite(out.data(), 1, out.size(), fo);

    fclose(fi);
    fclose(fo);
    return 0;
}
