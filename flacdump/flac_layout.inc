// flacdump: FlacLayout - prefix, metadata blocks and stream parameters of a FLAC file
// Part of the flacdump.cpp translation unit (included at a fixed position);
// relies on the definitions before that point; not a standalone header.

struct MBlock {
    int type;
    u32 len;
    u8* data;  // c: points into the input file; d: only valid while the block is decoded
    int structured;
    u64 off;  // file offset of the block header
};
struct FlacLayout {
    u8* prefix;  // c: bytes before "fLaC" (in the input buffer)
    size_t prefix_n;
    Vec<MBlock> blocks;
    StreamParams sp;
    int vendor;  // encoder hint from the vendor string
    u8 si[34];   // d: STREAMINFO contents
    // parse the container of the input (c): blocks and prefix point into 'in' (no copies)
    bool parse(Buf& in, u64& audio_start) {
        size_t pos = 0;
        // skip ID3v2 tags
        while (pos + 10 <= in.n && !memcmp(in.p + pos, "ID3", 3)) {
            const u8* h = in.p + pos;
            u32 sz = syncsafe32(h + 6);
            pos += 10 + sz + ((h[5] & 0x10) ? 10 : 0);
        }
        if (pos + 4 > in.n || memcmp(in.p + pos, "fLaC", 4)) {
            // search a bit
            size_t lim = Min(in.n, (size_t)(1 << 26));
            size_t q = 0;
            for (; q + 8 <= lim; q++)
                if (!memcmp(in.p + q, "fLaC", 4) && (in.p[q + 4] & 0x7F) == 0 && rd_be24(in.p + q + 5) == 34) break;
            if (q + 8 > lim) return false;
            pos = q;
        }
        prefix = in.p;
        prefix_n = pos;
        pos += 4;
        blocks.n = 0;
        for (;;) {
            if (pos + 4 > in.n) return false;
            const u8* h = in.p + pos;
            MBlock b;
            b.type = h[0] & 0x7F;
            b.len = rd_be24(h + 1);
            b.structured = 0;
            int last = h[0] >> 7;
            pos += 4;
            if (pos + b.len > in.n) return false;
            b.off = pos - 4;
            b.data = in.p + pos;
            pos += b.len;
            if (blocks.n == MAXBLOCKS) die("more than 100000 metadata blocks");
            blocks.push(b);
            if (last) break;
        }
        if (blocks[0].type != 0 || blocks[0].len != 34) return false;
        const u8* d = blocks[0].data;
        sp.rate = (d[10] << 12) | (d[11] << 4) | (d[12] >> 4);
        sp.nch = ((d[12] >> 1) & 7) + 1;
        sp.bps = (((d[12] & 1) << 4) | (d[13] >> 4)) + 1;
        if (sp.bps < 4) return false;  // FLAC: 4..32 bits per sample
        audio_start = pos;
        return true;
    }
    // file offset of the first frame
    u64 header_size() const {
        u64 s = prefix_n + 4;
        for (size_t i = 0; i < blocks.n; i++) s += 4 + blocks[i].len;
        return s;
    }
};
