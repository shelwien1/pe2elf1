/* pngdump - PNG/APNG → metainfo + raw deflate streams (round-trip exact).
 *
 * Encode: <in.png> → <meta>  +  dumpNN.bin (one per zlib stream)
 * Decode: <meta>   →  <out.png>   (reads dumpNN.bin from meta's directory)
 *
 * No compression, no recompression, no entropy coding. Pure structural
 * deconstruction of the PNG container. Each dumpNN.bin holds the raw
 * DEFLATE payload of one frame's IDAT or fdAT chain — i.e. the bytes
 * between the zlib header (2 or 6 bytes) and the ADLER32 trailer
 * (4 bytes). The zlib wrappers live in the meta file.
 *
 * Streams:
 *   stream 0 = IDAT chain (default image; also frame 0 of APNG when the
 *              default image is the first frame).
 *   Each fcTL that follows data opens a new stream for the next fdAT chain.
 *   So N-frame APNG produces N streams (default = frame 0) or N+1 streams
 *   (hidden default + N animated frames).
 *
 * Metainfo binary format v2 (little-endian for meta fields, BE for PNG fields):
 *   [8]   magic = "PNGDUMP\x02"
 *   [4]   n_streams
 *   [4]   n_chunks
 *   per-stream block, n_streams entries:
 *     [1]   zlib_header_len:  0 (empty stream) | 2 (no preset dict) | 6 (FDICT=1)
 *     [hlen] zlib header bytes (CMF, FLG [, 4-byte DICTID if FDICT=1])
 *     [4]   ADLER32 (only when hlen > 0; raw 4 bytes, big-endian as in PNG)
 *   per-chunk entries, n_chunks of them:
 *     [1]   kind: 0=RAW, 1=IDAT slice, 2=fdAT slice
 *     [4]   length  (PNG chunk data length; for fdAT, excludes the 4-byte seq prefix)
 *     RAW : [4] type + [length] data
 *     IDAT: [4] stream_idx
 *     fdAT: [4] stream_idx + [4] sequence
 *   [4]   trailer_length
 *   [...] trailer bytes (anything found after IEND in the source, normally 0)
 *
 * CRCs are never stored — they are determined by type+data and recomputed
 * on decode. RAW chunks are verified at encode time; bad-CRC inputs are
 * rejected so round-trip stays bit-exact.
 *
 * Build:  c++ -O2 -std=c++17 pngdump.cpp -o pngdump
 */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

/* ─── magic ──────────────────────────────────────────────────────────────── */

static const uint8_t PNG_SIG[8]    = {0x89,'P','N','G','\r','\n',0x1a,'\n'};
static const uint8_t DUMP_MAGIC[8] = {'P','N','G','D','U','M','P',0x02};

enum : uint8_t { KIND_RAW = 0, KIND_IDAT = 1, KIND_FDAT = 2 };

/* ─── byte ops ───────────────────────────────────────────────────────────── */

static inline uint32_t rd_be32(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16)
         | (uint32_t(p[2]) <<  8) |  uint32_t(p[3]);
}
static inline uint32_t rd_le32(const uint8_t* p) {
    return  uint32_t(p[0])        | (uint32_t(p[1]) <<  8)
         | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
static inline void wr_be32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(uint8_t(x >> 24)); v.push_back(uint8_t(x >> 16));
    v.push_back(uint8_t(x >>  8)); v.push_back(uint8_t(x));
}
static inline void wr_le32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(uint8_t(x));       v.push_back(uint8_t(x >>  8));
    v.push_back(uint8_t(x >> 16)); v.push_back(uint8_t(x >> 24));
}

/* ─── CRC32 (PNG/zlib polynomial 0xEDB88320) ─────────────────────────────── */

static const uint32_t* crc_table() {
    static uint32_t t[256];
    static bool init = false;
    if (!init) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int j = 0; j < 8; j++)
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            t[i] = c;
        }
        init = true;
    }
    return t;
}
static uint32_t chunk_crc(const uint8_t* type, const uint8_t* data, size_t dlen) {
    const uint32_t* tbl = crc_table();
    uint32_t c = 0xFFFFFFFFu;
    for (int i = 0; i < 4; i++)
        c = tbl[(c ^ type[i]) & 0xff] ^ (c >> 8);
    for (size_t i = 0; i < dlen; i++)
        c = tbl[(c ^ data[i]) & 0xff] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

/* ─── file I/O ───────────────────────────────────────────────────────────── */

static bool read_file(const std::string& path, std::vector<uint8_t>& out) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return false; }
    long n = ftell(f);
    if (n < 0) { fclose(f); return false; }
    rewind(f);
    out.resize(size_t(n));
    bool ok = (n == 0) || (fread(out.data(), 1, out.size(), f) == out.size());
    fclose(f);
    return ok;
}
static bool write_file(const std::string& path, const uint8_t* d, size_t n) {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return false;
    bool ok = (n == 0) || (fwrite(d, 1, n, f) == n);
    fclose(f);
    return ok;
}
static bool write_file(const std::string& path, const std::vector<uint8_t>& v) {
    return write_file(path, v.data(), v.size());
}

/* ─── stream-name helper (consistent width on both sides) ────────────────── */

static std::string stream_name(uint32_t i, uint32_t n_streams) {
    int w = 2;
    if (n_streams > 0) {
        uint32_t m = n_streams - 1;
        int d = 1;
        while (m >= 10) { m /= 10; d++; }
        if (d > w) w = d;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "dump%0*u.bin", w, i);
    return std::string(buf);
}

/* ─── encode: PNG → meta + dumpNN.bin ────────────────────────────────────── */

struct ChunkEntry {
    uint8_t  kind;             // KIND_RAW | KIND_IDAT | KIND_FDAT
    uint32_t length;           // RAW: chunk data length
                               // IDAT: bytes contributed to stream
                               // FDAT: bytes contributed to stream (= chunk length − 4)
    uint8_t  type[4];          // RAW only
    std::vector<uint8_t> data; // RAW only
    uint32_t stream_idx;       // IDAT / FDAT
    uint32_t sequence;         // FDAT only
};

static bool encode(const std::string& png_path, const std::string& meta_path) {
    std::vector<uint8_t> buf;
    if (!read_file(png_path, buf)) {
        fprintf(stderr, "pngdump: cannot read %s\n", png_path.c_str());
        return false;
    }
    if (buf.size() < 8 || memcmp(buf.data(), PNG_SIG, 8) != 0) {
        fprintf(stderr, "pngdump: %s: bad PNG signature\n", png_path.c_str());
        return false;
    }

    std::vector<ChunkEntry> entries;
    std::vector<std::vector<uint8_t>> streams;
    streams.emplace_back();                // stream 0 always exists
    uint32_t cur_stream = 0;
    bool data_in_current = false;          // has cur_stream received any bytes?

    size_t pos = 8;
    bool seen_iend = false;
    while (pos + 12 <= buf.size()) {
        uint32_t length = rd_be32(buf.data() + pos); pos += 4;
        uint8_t type[4]; memcpy(type, buf.data() + pos, 4); pos += 4;
        if (pos + size_t(length) + 4 > buf.size()) {
            fprintf(stderr, "pngdump: truncated chunk %.4s\n", type);
            return false;
        }
        const uint8_t* dp  = buf.data() + pos;
        uint32_t crc_in    = rd_be32(buf.data() + pos + length);
        uint32_t crc_calc  = chunk_crc(type, dp, length);
        if (crc_in != crc_calc) {
            fprintf(stderr, "pngdump: CRC mismatch in %.4s "
                    "(stored=0x%08x calc=0x%08x)\n", type, crc_in, crc_calc);
            return false;
        }

        if (memcmp(type, "IDAT", 4) == 0) {
            streams[cur_stream].insert(streams[cur_stream].end(),
                                       dp, dp + length);
            ChunkEntry e{};
            e.kind = KIND_IDAT;
            e.length = length;
            e.stream_idx = cur_stream;
            entries.push_back(std::move(e));
            data_in_current = true;
        }
        else if (memcmp(type, "fdAT", 4) == 0) {
            if (length < 4) {
                fprintf(stderr, "pngdump: fdAT chunk shorter than 4 bytes\n");
                return false;
            }
            uint32_t seq = rd_be32(dp);
            uint32_t payload = length - 4;
            streams[cur_stream].insert(streams[cur_stream].end(),
                                       dp + 4, dp + 4 + payload);
            ChunkEntry e{};
            e.kind = KIND_FDAT;
            e.length = payload;
            e.stream_idx = cur_stream;
            e.sequence = seq;
            entries.push_back(std::move(e));
            data_in_current = true;
        }
        else {
            // fcTL only opens a new stream if the current one already holds
            // data. fcTL placed before any IDAT (typical for APNGs whose
            // default image is the first frame) is a pure RAW record.
            if (memcmp(type, "fcTL", 4) == 0 && data_in_current) {
                streams.emplace_back();
                cur_stream++;
                data_in_current = false;
            }
            ChunkEntry e{};
            e.kind = KIND_RAW;
            e.length = length;
            memcpy(e.type, type, 4);
            e.data.assign(dp, dp + length);
            entries.push_back(std::move(e));
            if (memcmp(type, "IEND", 4) == 0) {
                seen_iend = true;
                pos += length + 4;
                break;
            }
        }
        pos += length + 4;
    }

    if (!seen_iend)
        fprintf(stderr, "pngdump: warning: no IEND chunk\n");

    // Trailer: any bytes after IEND. Preserved verbatim.
    std::vector<uint8_t> trailer(buf.begin() + pos, buf.end());

    fs::path mp = meta_path;
    fs::path dir = mp.parent_path();
    if (dir.empty()) dir = ".";
    std::error_code ec;
    fs::create_directories(dir, ec);

    // Split each zlib stream into [header | deflate | adler32]. The header
    // is 2 bytes (or 6 if FDICT set — forbidden by PNG, but tolerated here).
    // Only the deflate goes to dumpNN.bin; header + adler go to meta.
    uint32_t n_streams = (uint32_t)streams.size();
    std::vector<std::vector<uint8_t>> hdrs(n_streams);
    std::vector<std::vector<uint8_t>> adls(n_streams);
    std::vector<std::vector<uint8_t>> defl(n_streams);
    for (uint32_t i = 0; i < n_streams; i++) {
        auto& s = streams[i];
        if (s.empty()) continue;                          // empty stream
        if (s.size() < 6) {
            fprintf(stderr, "pngdump: stream %u too short (%zu bytes) "
                    "to be a zlib stream\n", i, s.size());
            return false;
        }
        size_t hlen = (s[1] & 0x20) ? 6 : 2;              // FDICT bit
        if (s.size() < hlen + 4) {
            fprintf(stderr, "pngdump: stream %u too short for hlen=%zu "
                    "+ adler32\n", i, hlen);
            return false;
        }
        hdrs[i].assign(s.begin(),         s.begin() + hlen);
        adls[i].assign(s.end()   - 4,     s.end());
        defl[i].assign(s.begin() + hlen,  s.end() - 4);
    }

    // Write dumpNN.bin files (raw deflate only).
    for (uint32_t i = 0; i < n_streams; i++) {
        fs::path sp = dir / stream_name(i, n_streams);
        if (!write_file(sp.string(), defl[i])) {
            fprintf(stderr, "pngdump: cannot write %s\n", sp.string().c_str());
            return false;
        }
    }

    // Build & write metainfo.
    std::vector<uint8_t> meta;
    meta.insert(meta.end(), DUMP_MAGIC, DUMP_MAGIC + 8);
    wr_le32(meta, n_streams);
    wr_le32(meta, (uint32_t)entries.size());
    // Per-stream block: zlib header + adler32 for each stream.
    for (uint32_t i = 0; i < n_streams; i++) {
        if (streams[i].empty()) {
            meta.push_back(0);     // header_len = 0 marks empty stream
        } else {
            meta.push_back(uint8_t(hdrs[i].size()));
            meta.insert(meta.end(), hdrs[i].begin(), hdrs[i].end());
            meta.insert(meta.end(), adls[i].begin(), adls[i].end());
        }
    }
    for (auto& e : entries) {
        meta.push_back(e.kind);
        wr_le32(meta, e.length);
        if (e.kind == KIND_RAW) {
            meta.insert(meta.end(), e.type, e.type + 4);
            meta.insert(meta.end(), e.data.begin(), e.data.end());
        } else if (e.kind == KIND_IDAT) {
            wr_le32(meta, e.stream_idx);
        } else {  // KIND_FDAT
            wr_le32(meta, e.stream_idx);
            wr_le32(meta, e.sequence);
        }
    }
    wr_le32(meta, (uint32_t)trailer.size());
    meta.insert(meta.end(), trailer.begin(), trailer.end());

    if (!write_file(meta_path, meta)) {
        fprintf(stderr, "pngdump: cannot write %s\n", meta_path.c_str());
        return false;
    }

    size_t deflate_bytes = 0;
    for (auto& d : defl) deflate_bytes += d.size();
    fprintf(stdout,
            "encoded: %zu chunks, %u stream%s, %zu deflate bytes, "
            "meta=%zu bytes%s\n",
            entries.size(), n_streams, n_streams == 1 ? "" : "s",
            deflate_bytes, meta.size(),
            trailer.empty() ? "" : " (+trailer)");
    return true;
}

/* ─── decode: meta + dumpNN.bin → PNG ────────────────────────────────────── */

static bool decode(const std::string& meta_path, const std::string& png_path) {
    std::vector<uint8_t> meta;
    if (!read_file(meta_path, meta)) {
        fprintf(stderr, "pngdump: cannot read %s\n", meta_path.c_str());
        return false;
    }
    if (meta.size() < 16 || memcmp(meta.data(), DUMP_MAGIC, 8) != 0) {
        fprintf(stderr, "pngdump: %s: bad PNGDUMP magic\n", meta_path.c_str());
        return false;
    }
    size_t p = 8;
    uint32_t n_streams = rd_le32(meta.data() + p); p += 4;
    uint32_t n_chunks  = rd_le32(meta.data() + p); p += 4;

    fs::path mp = meta_path;
    fs::path dir = mp.parent_path();
    if (dir.empty()) dir = ".";

    // Per-stream block: zlib header + adler32 for each stream.
    std::vector<std::vector<uint8_t>> hdrs(n_streams);
    std::vector<std::vector<uint8_t>> adls(n_streams);
    for (uint32_t i = 0; i < n_streams; i++) {
        if (p + 1 > meta.size()) {
            fprintf(stderr, "pngdump: meta truncated at stream %u header\n", i);
            return false;
        }
        uint8_t hlen = meta[p++];
        if (hlen == 0) continue;                          // empty stream
        if (hlen != 2 && hlen != 6) {
            fprintf(stderr, "pngdump: stream %u: invalid header_len %u\n", i, hlen);
            return false;
        }
        if (p + size_t(hlen) + 4 > meta.size()) {
            fprintf(stderr, "pngdump: meta truncated at stream %u body\n", i);
            return false;
        }
        hdrs[i].assign(meta.data() + p, meta.data() + p + hlen); p += hlen;
        adls[i].assign(meta.data() + p, meta.data() + p + 4);    p += 4;
    }

    // Load raw deflate per stream from dump files, then splice
    // streams[i] = header[i] || deflate[i] || adler32[i].
    std::vector<std::vector<uint8_t>> streams(n_streams);
    std::vector<size_t> off(n_streams, 0);
    for (uint32_t i = 0; i < n_streams; i++) {
        fs::path sp = dir / stream_name(i, n_streams);
        std::vector<uint8_t> deflate_bytes;
        if (!read_file(sp.string(), deflate_bytes)) {
            fprintf(stderr, "pngdump: cannot read %s\n", sp.string().c_str());
            return false;
        }
        if (hdrs[i].empty()) {
            if (!deflate_bytes.empty()) {
                fprintf(stderr, "pngdump: stream %u marked empty but "
                        "dump file has %zu bytes\n", i, deflate_bytes.size());
                return false;
            }
            continue;
        }
        streams[i].reserve(hdrs[i].size() + deflate_bytes.size() + 4);
        streams[i].insert(streams[i].end(), hdrs[i].begin(), hdrs[i].end());
        streams[i].insert(streams[i].end(),
                          deflate_bytes.begin(), deflate_bytes.end());
        streams[i].insert(streams[i].end(), adls[i].begin(), adls[i].end());
    }

    std::vector<uint8_t> out;
    out.insert(out.end(), PNG_SIG, PNG_SIG + 8);

    for (uint32_t i = 0; i < n_chunks; i++) {
        if (p + 5 > meta.size()) {
            fprintf(stderr, "pngdump: truncated meta entry %u\n", i);
            return false;
        }
        uint8_t kind   = meta[p++];
        uint32_t length = rd_le32(meta.data() + p); p += 4;

        if (kind == KIND_RAW) {
            if (p + 4 + size_t(length) > meta.size()) {
                fprintf(stderr, "pngdump: truncated RAW chunk %u\n", i);
                return false;
            }
            uint8_t type[4]; memcpy(type, meta.data() + p, 4); p += 4;
            const uint8_t* dp = meta.data() + p; p += length;
            uint32_t crc = chunk_crc(type, dp, length);
            wr_be32(out, length);
            out.insert(out.end(), type, type + 4);
            out.insert(out.end(), dp, dp + length);
            wr_be32(out, crc);
        }
        else if (kind == KIND_IDAT) {
            if (p + 4 > meta.size()) {
                fprintf(stderr, "pngdump: truncated IDAT entry %u\n", i);
                return false;
            }
            uint32_t sidx = rd_le32(meta.data() + p); p += 4;
            if (sidx >= n_streams ||
                off[sidx] + length > streams[sidx].size()) {
                fprintf(stderr, "pngdump: IDAT slice out of range "
                        "(stream %u, off %zu + len %u > %zu)\n",
                        sidx, off[sidx], length, streams[sidx].size());
                return false;
            }
            uint8_t type[4] = {'I','D','A','T'};
            const uint8_t* dp = streams[sidx].data() + off[sidx];
            uint32_t crc = chunk_crc(type, dp, length);
            wr_be32(out, length);
            out.insert(out.end(), type, type + 4);
            out.insert(out.end(), dp, dp + length);
            wr_be32(out, crc);
            off[sidx] += length;
        }
        else if (kind == KIND_FDAT) {
            if (p + 8 > meta.size()) {
                fprintf(stderr, "pngdump: truncated fdAT entry %u\n", i);
                return false;
            }
            uint32_t sidx = rd_le32(meta.data() + p); p += 4;
            uint32_t seq  = rd_le32(meta.data() + p); p += 4;
            if (sidx >= n_streams ||
                off[sidx] + length > streams[sidx].size()) {
                fprintf(stderr, "pngdump: fdAT slice out of range\n");
                return false;
            }
            uint8_t type[4] = {'f','d','A','T'};
            std::vector<uint8_t> payload;
            payload.reserve(4 + size_t(length));
            wr_be32(payload, seq);
            payload.insert(payload.end(),
                           streams[sidx].data() + off[sidx],
                           streams[sidx].data() + off[sidx] + length);
            uint32_t total_len = (uint32_t)payload.size();
            uint32_t crc = chunk_crc(type, payload.data(), total_len);
            wr_be32(out, total_len);
            out.insert(out.end(), type, type + 4);
            out.insert(out.end(), payload.begin(), payload.end());
            wr_be32(out, crc);
            off[sidx] += length;
        }
        else {
            fprintf(stderr, "pngdump: unknown chunk kind %u at entry %u\n", kind, i);
            return false;
        }
    }

    // Trailer (post-IEND bytes).
    if (p + 4 > meta.size()) {
        fprintf(stderr, "pngdump: meta truncated before trailer\n");
        return false;
    }
    uint32_t tail_len = rd_le32(meta.data() + p); p += 4;
    if (p + size_t(tail_len) > meta.size()) {
        fprintf(stderr, "pngdump: trailer truncated\n");
        return false;
    }
    out.insert(out.end(), meta.data() + p, meta.data() + p + tail_len);

    for (uint32_t i = 0; i < n_streams; i++) {
        if (off[i] != streams[i].size()) {
            fprintf(stderr, "pngdump: stream %u: consumed %zu of %zu bytes\n",
                    i, off[i], streams[i].size());
            return false;
        }
    }

    if (!write_file(png_path, out)) {
        fprintf(stderr, "pngdump: cannot write %s\n", png_path.c_str());
        return false;
    }
    fprintf(stdout, "decoded: %u chunks, %u stream%s, %zu bytes\n",
            n_chunks, n_streams, n_streams == 1 ? "" : "s", out.size());
    return true;
}

/* ─── main ───────────────────────────────────────────────────────────────── */

static void usage() {
    fprintf(stderr,
        "pngdump - PNG ↔ deflate-stream dump (round-trip exact, no compression)\n"
        "\n"
        "  pngdump <in.png>  <out.meta>   encode: writes meta + dumpNN.bin\n"
        "  pngdump <in.meta> <out.png>    decode: reads  meta + dumpNN.bin\n"
        "\n"
        "Direction is auto-detected from the input's magic bytes.\n"
        "dumpNN.bin files live in the meta file's directory.\n");
}

int main(int argc, char** argv) {
    if (argc != 3) { usage(); return 1; }
    std::string in = argv[1], out = argv[2];

    uint8_t hdr[8] = {0};
    FILE* f = fopen(in.c_str(), "rb");
    if (!f) {
        fprintf(stderr, "pngdump: cannot open %s\n", in.c_str());
        return 1;
    }
    size_t got = fread(hdr, 1, 8, f);
    fclose(f);
    if (got < 8) {
        fprintf(stderr, "pngdump: %s: too short to identify\n", in.c_str());
        return 1;
    }
    if (memcmp(hdr, PNG_SIG, 8) == 0)
        return encode(in, out) ? 0 : 1;
    if (memcmp(hdr, DUMP_MAGIC, 8) == 0)
        return decode(in, out) ? 0 : 1;
    fprintf(stderr,
            "pngdump: %s: not a PNG and not a PNGDUMP meta file\n", in.c_str());
    return 1;
}
