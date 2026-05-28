/* pngtool - integrated PNG recompression tool.
 *
 *   pngtool c input.png output.bmp output.meta
 *     encode: PNG → per-frame BMP(s) + single meta file.
 *
 *   pngtool d input.bmp input.meta output.png
 *     decode: BMP(s) + meta → PNG (bit-exact).
 *
 * For multi-frame APNG the BMP path is taken as a template: the basename
 * gets a zero-padded index inserted before the extension. So with
 * output.bmp + 3 frames we write output0.bmp, output1.bmp, output2.bmp.
 *
 * The meta file holds: pngdump container blob, per-stream preflate
 * reconstruction info, per-stream PNG filter types (one byte per scanline,
 * including all Adam7 passes), per-stream frame dimensions.
 *
 * Combines logic from pngdump.cpp + png2bmp.cpp; uses libpreflate for
 * deflate split/reencode.
 */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <filesystem>
#include <string>
#include <vector>

#include "preflate/preflate_decoder.h"
#include "preflate/preflate_reencoder.h"

namespace fs = std::filesystem;

/* ─── magic ──────────────────────────────────────────────────────────────── */

static const uint8_t PNG_SIG[8]      = {0x89,'P','N','G','\r','\n',0x1a,'\n'};
static const uint8_t PNGTOOL_MAGIC[8] = {'P','N','G','T','O','O','L','1'};

enum : uint8_t { KIND_RAW = 0, KIND_IDAT = 1, KIND_FDAT = 2 };

/* ─── byte ops ───────────────────────────────────────────────────────────── */

static inline uint16_t rd_le16(const uint8_t* p) {
    return uint16_t(p[0]) | (uint16_t(p[1]) << 8);
}
static inline uint32_t rd_le32(const uint8_t* p) {
    return  uint32_t(p[0])        | (uint32_t(p[1]) <<  8)
         | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
static inline uint32_t rd_be32(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16)
         | (uint32_t(p[2]) <<  8) |  uint32_t(p[3]);
}
static inline void wr_le16(std::vector<uint8_t>& v, uint16_t x) {
    v.push_back(uint8_t(x)); v.push_back(uint8_t(x >> 8));
}
static inline void wr_le32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(uint8_t(x));       v.push_back(uint8_t(x >>  8));
    v.push_back(uint8_t(x >> 16)); v.push_back(uint8_t(x >> 24));
}
static inline void wr_be32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(uint8_t(x >> 24)); v.push_back(uint8_t(x >> 16));
    v.push_back(uint8_t(x >>  8)); v.push_back(uint8_t(x));
}

/* ─── error helper ───────────────────────────────────────────────────────── */

static bool err(const char* fmt, ...) {
    fprintf(stderr, "pngtool: ");
    va_list ap; va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    return false;
}

/* ─── CRC32 ──────────────────────────────────────────────────────────────── */

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
static bool write_file(const std::string& path, const std::vector<uint8_t>& v) {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) return false;
    bool ok = (v.empty()) || (fwrite(v.data(), 1, v.size(), f) == v.size());
    fclose(f);
    return ok;
}

/* ─── PNG → chunks + streams (pngdump encode logic) ──────────────────────── */

struct ChunkEntry {
    uint8_t  kind;
    uint32_t length;
    uint8_t  type[4];
    std::vector<uint8_t> data;
    uint32_t stream_idx;
    uint32_t sequence;
};

struct StreamSplit {
    std::vector<uint8_t> hdr;     // zlib header (2 or 6 bytes); empty = empty stream
    std::vector<uint8_t> adler;   // 4 bytes (empty if stream empty)
    std::vector<uint8_t> deflate; // raw deflate payload
    uint32_t W = 0, H = 0;        // frame dimensions
};

struct Ihdr {
    uint32_t width = 0, height = 0;
    uint8_t  bit_depth = 0, color_type = 0;
    uint8_t  interlace = 0;
};

static bool parse_ihdr(const uint8_t* data, uint32_t length, Ihdr& h) {
    if (length != 13) return err("IHDR not 13 bytes");
    h.width      = rd_be32(data);
    h.height     = rd_be32(data + 4);
    h.bit_depth  = data[8];
    h.color_type = data[9];
    h.interlace  = data[12];
    return true;
}

// Parse PNG → chunks, streams. Per-stream W/H come from IHDR (stream 0
// when no preceding fcTL) or from the nearest preceding fcTL.
static bool dissect_png(const std::vector<uint8_t>& buf,
                        std::vector<ChunkEntry>& entries,
                        std::vector<StreamSplit>& streams,
                        std::vector<uint8_t>& trailer,
                        Ihdr& ihdr)
{
    if (buf.size() < 8 || memcmp(buf.data(), PNG_SIG, 8) != 0)
        return err("bad PNG signature");

    streams.clear();
    streams.emplace_back();           // stream 0 always exists
    uint32_t cur_stream = 0;
    bool data_in_current = false;
    bool have_ihdr = false;
    bool pending_dims = false;
    uint32_t pending_W = 0, pending_H = 0;

    // Per-stream raw bytes (header+deflate+adler) collected here first.
    std::vector<std::vector<uint8_t>> raw_streams;
    raw_streams.emplace_back();
    std::vector<std::pair<uint32_t,uint32_t>> stream_dims;
    stream_dims.emplace_back(0, 0);   // filled when IHDR seen

    size_t pos = 8;
    bool seen_iend = false;
    while (pos + 12 <= buf.size()) {
        uint32_t length = rd_be32(buf.data() + pos); pos += 4;
        uint8_t type[4]; memcpy(type, buf.data() + pos, 4); pos += 4;
        if (pos + size_t(length) + 4 > buf.size())
            return err("truncated chunk %.4s", type);
        const uint8_t* dp  = buf.data() + pos;
        uint32_t crc_in    = rd_be32(buf.data() + pos + length);
        uint32_t crc_calc  = chunk_crc(type, dp, length);
        if (crc_in != crc_calc)
            return err("CRC mismatch in %.4s", type);

        if (memcmp(type, "IHDR", 4) == 0) {
            if (!parse_ihdr(dp, length, ihdr)) return false;
            have_ihdr = true;
            if (stream_dims[0].first == 0) {
                stream_dims[0] = {ihdr.width, ihdr.height};
            }
            ChunkEntry e{};
            e.kind = KIND_RAW;
            e.length = length;
            memcpy(e.type, type, 4);
            e.data.assign(dp, dp + length);
            entries.push_back(std::move(e));
        }
        else if (memcmp(type, "IDAT", 4) == 0) {
            if (pending_dims) {
                stream_dims[cur_stream] = {pending_W, pending_H};
                pending_dims = false;
            }
            raw_streams[cur_stream].insert(raw_streams[cur_stream].end(),
                                           dp, dp + length);
            ChunkEntry e{};
            e.kind = KIND_IDAT;
            e.length = length;
            e.stream_idx = cur_stream;
            entries.push_back(std::move(e));
            data_in_current = true;
        }
        else if (memcmp(type, "fdAT", 4) == 0) {
            if (length < 4) return err("fdAT chunk shorter than 4 bytes");
            if (pending_dims) {
                stream_dims[cur_stream] = {pending_W, pending_H};
                pending_dims = false;
            }
            uint32_t seq = rd_be32(dp);
            uint32_t payload = length - 4;
            raw_streams[cur_stream].insert(raw_streams[cur_stream].end(),
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
            bool is_fctl = (memcmp(type, "fcTL", 4) == 0);
            if (is_fctl && data_in_current) {
                raw_streams.emplace_back();
                stream_dims.emplace_back(0, 0);
                cur_stream++;
                data_in_current = false;
            }
            if (is_fctl) {
                if (length < 26) return err("fcTL too short (%u)", length);
                pending_W = rd_be32(dp + 4);
                pending_H = rd_be32(dp + 8);
                pending_dims = true;
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

    if (!have_ihdr) return err("no IHDR chunk");
    if (!seen_iend) fprintf(stderr, "pngtool: warning: no IEND chunk\n");

    trailer.assign(buf.begin() + pos, buf.end());

    // Split each stream into header / deflate / adler.
    uint32_t n = (uint32_t)raw_streams.size();
    streams.assign(n, {});
    for (uint32_t i = 0; i < n; i++) {
        streams[i].W = stream_dims[i].first;
        streams[i].H = stream_dims[i].second;
        auto& s = raw_streams[i];
        if (s.empty()) continue;
        if (s.size() < 6) return err("stream %u too short (%zu bytes)", i, s.size());
        size_t hlen = (s[1] & 0x20) ? 6 : 2;
        if (s.size() < hlen + 4) return err("stream %u too short for header+adler", i);
        streams[i].hdr  .assign(s.begin(),         s.begin() + hlen);
        streams[i].adler.assign(s.end()   - 4,     s.end());
        streams[i].deflate.assign(s.begin() + hlen, s.end() - 4);
    }
    return true;
}

/* ─── pngdump meta blob ──────────────────────────────────────────────────── */

static void build_pngdump_blob(const std::vector<ChunkEntry>& entries,
                               const std::vector<StreamSplit>& streams,
                               const std::vector<uint8_t>& trailer,
                               std::vector<uint8_t>& blob)
{
    static const uint8_t DUMP_MAGIC[8] = {'P','N','G','D','U','M','P', 0x02};
    blob.clear();
    blob.insert(blob.end(), DUMP_MAGIC, DUMP_MAGIC + 8);
    uint32_t n = (uint32_t)streams.size();
    wr_le32(blob, n);
    wr_le32(blob, (uint32_t)entries.size());
    for (uint32_t i = 0; i < n; i++) {
        if (streams[i].hdr.empty()) {
            blob.push_back(0);
        } else {
            blob.push_back(uint8_t(streams[i].hdr.size()));
            blob.insert(blob.end(), streams[i].hdr.begin(), streams[i].hdr.end());
            blob.insert(blob.end(), streams[i].adler.begin(), streams[i].adler.end());
        }
    }
    for (auto& e : entries) {
        blob.push_back(e.kind);
        wr_le32(blob, e.length);
        if (e.kind == KIND_RAW) {
            blob.insert(blob.end(), e.type, e.type + 4);
            blob.insert(blob.end(), e.data.begin(), e.data.end());
        } else if (e.kind == KIND_IDAT) {
            wr_le32(blob, e.stream_idx);
        } else {
            wr_le32(blob, e.stream_idx);
            wr_le32(blob, e.sequence);
        }
    }
    wr_le32(blob, (uint32_t)trailer.size());
    blob.insert(blob.end(), trailer.begin(), trailer.end());
}

// Decode the pngdump blob back to chunks+streams+trailer. We need this for
// the decode path (pngtool d) to reassemble the PNG.
static bool parse_pngdump_blob(const std::vector<uint8_t>& blob,
                               std::vector<ChunkEntry>& entries,
                               uint32_t& n_streams,
                               std::vector<std::vector<uint8_t>>& hdrs,
                               std::vector<std::vector<uint8_t>>& adls,
                               std::vector<uint8_t>& trailer,
                               Ihdr& ihdr)
{
    static const uint8_t DUMP_MAGIC[8] = {'P','N','G','D','U','M','P', 0x02};
    if (blob.size() < 16 || memcmp(blob.data(), DUMP_MAGIC, 8) != 0)
        return err("bad PNGDUMP magic in meta blob");
    size_t p = 8;
    n_streams = rd_le32(blob.data() + p); p += 4;
    uint32_t n_chunks = rd_le32(blob.data() + p); p += 4;
    hdrs.assign(n_streams, {});
    adls.assign(n_streams, {});
    for (uint32_t i = 0; i < n_streams; i++) {
        if (p + 1 > blob.size()) return err("blob truncated at stream %u hdr", i);
        uint8_t hlen = blob[p++];
        if (hlen == 0) continue;
        if (hlen != 2 && hlen != 6) return err("stream %u hlen %u invalid", i, hlen);
        if (p + hlen + 4 > blob.size()) return err("blob truncated stream %u body", i);
        hdrs[i].assign(blob.data() + p, blob.data() + p + hlen); p += hlen;
        adls[i].assign(blob.data() + p, blob.data() + p + 4);    p += 4;
    }
    bool have_ihdr = false;
    entries.clear();
    entries.reserve(n_chunks);
    for (uint32_t i = 0; i < n_chunks; i++) {
        if (p + 5 > blob.size()) return err("blob truncated entry %u", i);
        ChunkEntry e{};
        e.kind = blob[p++];
        e.length = rd_le32(blob.data() + p); p += 4;
        if (e.kind == KIND_RAW) {
            if (p + 4 + e.length > blob.size()) return err("RAW %u truncated", i);
            memcpy(e.type, blob.data() + p, 4); p += 4;
            e.data.assign(blob.data() + p, blob.data() + p + e.length); p += e.length;
            if (memcmp(e.type, "IHDR", 4) == 0) {
                if (!parse_ihdr(e.data.data(), e.length, ihdr)) return false;
                have_ihdr = true;
            }
        } else if (e.kind == KIND_IDAT) {
            if (p + 4 > blob.size()) return err("IDAT %u truncated", i);
            e.stream_idx = rd_le32(blob.data() + p); p += 4;
        } else if (e.kind == KIND_FDAT) {
            if (p + 8 > blob.size()) return err("fdAT %u truncated", i);
            e.stream_idx = rd_le32(blob.data() + p); p += 4;
            e.sequence   = rd_le32(blob.data() + p); p += 4;
        } else {
            return err("unknown kind %u at entry %u", e.kind, i);
        }
        entries.push_back(std::move(e));
    }
    if (!have_ihdr) return err("blob has no IHDR");
    if (p + 4 > blob.size()) return err("blob trailer length missing");
    uint32_t tlen = rd_le32(blob.data() + p); p += 4;
    if (p + tlen > blob.size()) return err("blob trailer truncated");
    trailer.assign(blob.data() + p, blob.data() + p + tlen);
    return true;
}

/* ─── filter / unfilter (from png2bmp) ───────────────────────────────────── */

static uint32_t samples_per_pixel(uint8_t ct) {
    switch (ct) {
        case 0: return 1; case 2: return 3; case 3: return 1;
        case 4: return 2; case 6: return 4;
        default: return 0;
    }
}
static inline uint8_t paeth(uint8_t a, uint8_t b, uint8_t c) {
    int p  = int(a) + int(b) - int(c);
    int pa = std::abs(p - int(a));
    int pb = std::abs(p - int(b));
    int pc = std::abs(p - int(c));
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

static bool defilter(const uint8_t* unp, uint32_t H,
                     uint32_t stride, uint32_t bpp,
                     std::vector<uint8_t>& raw,
                     std::vector<uint8_t>& filters)
{
    raw.assign(size_t(H) * stride, 0);
    filters.assign(H, 0);
    size_t in_row = size_t(1) + stride;
    for (uint32_t r = 0; r < H; r++) {
        uint8_t ft = unp[r * in_row];
        filters[r] = ft;
        const uint8_t* fdata = unp + r * in_row + 1;
        uint8_t*       out   = raw.data() + size_t(r) * stride;
        const uint8_t* prior = (r > 0) ? raw.data() + size_t(r - 1) * stride : nullptr;
        for (uint32_t i = 0; i < stride; i++) {
            uint8_t f = fdata[i];
            uint8_t a = (i >= bpp) ? out[i - bpp] : 0;
            uint8_t b =  prior ?  prior[i] : 0;
            uint8_t c = (prior && i >= bpp) ? prior[i - bpp] : 0;
            uint8_t v;
            switch (ft) {
                case 0: v = f; break;
                case 1: v = uint8_t(f + a); break;
                case 2: v = uint8_t(f + b); break;
                case 3: v = uint8_t(f + uint8_t((unsigned(a) + unsigned(b)) / 2)); break;
                case 4: v = uint8_t(f + paeth(a, b, c)); break;
                default: return err("unknown filter type %u on row %u", ft, r);
            }
            out[i] = v;
        }
    }
    return true;
}
static void refilter(const uint8_t* raw, uint32_t H,
                     uint32_t stride, uint32_t bpp,
                     const uint8_t* filters,
                     std::vector<uint8_t>& unp)
{
    size_t out_row = size_t(1) + stride;
    unp.assign(size_t(H) * out_row, 0);
    for (uint32_t r = 0; r < H; r++) {
        uint8_t ft = filters[r];
        unp[r * out_row] = ft;
        uint8_t*       out   = unp.data() + r * out_row + 1;
        const uint8_t* row   = raw + size_t(r) * stride;
        const uint8_t* prior = (r > 0) ? raw + size_t(r - 1) * stride : nullptr;
        for (uint32_t i = 0; i < stride; i++) {
            uint8_t x = row[i];
            uint8_t a = (i >= bpp) ? row[i - bpp] : 0;
            uint8_t b =  prior ?  prior[i] : 0;
            uint8_t c = (prior && i >= bpp) ? prior[i - bpp] : 0;
            switch (ft) {
                case 0: out[i] = x; break;
                case 1: out[i] = uint8_t(x - a); break;
                case 2: out[i] = uint8_t(x - b); break;
                case 3: out[i] = uint8_t(x - uint8_t((unsigned(a) + unsigned(b)) / 2)); break;
                case 4: out[i] = uint8_t(x - paeth(a, b, c)); break;
                default: out[i] = x; break;
            }
        }
    }
}

/* ─── Adam7 ──────────────────────────────────────────────────────────────── */

struct A7Pass { uint8_t xs, ys, xstep, ystep; };
static const A7Pass A7P[7] = {
    {0,0,8,8},{4,0,8,8},{0,4,4,8},{2,0,4,4},{0,2,2,4},{1,0,2,2},{0,1,1,2}
};
static void pass_dims(int p, uint32_t W, uint32_t H, uint32_t& Wp, uint32_t& Hp) {
    const A7Pass& a = A7P[p];
    Wp = (W > a.xs) ? (W - a.xs + a.xstep - 1) / a.xstep : 0;
    Hp = (H > a.ys) ? (H - a.ys + a.ystep - 1) / a.ystep : 0;
}

static void deinterlace_to_raw(uint32_t W, uint32_t H, uint8_t ct, uint8_t bd,
                               const std::vector<std::vector<uint8_t>>& pass_raws,
                               std::vector<uint8_t>& raw_full)
{
    uint32_t bits = samples_per_pixel(ct) * bd;
    uint32_t full_stride = (W * bits + 7) / 8;
    raw_full.assign(size_t(H) * full_stride, 0);
    for (int p = 0; p < 7; p++) {
        if (pass_raws[p].empty()) continue;
        uint32_t Wp, Hp; pass_dims(p, W, H, Wp, Hp);
        uint32_t pass_stride = (Wp * bits + 7) / 8;
        const A7Pass& a = A7P[p];
        if (bits >= 8) {
            uint32_t pxb = bits / 8;
            for (uint32_t py = 0; py < Hp; py++) {
                uint32_t fy = a.ys + py * a.ystep;
                const uint8_t* src = pass_raws[p].data() + size_t(py) * pass_stride;
                uint8_t*       dst = raw_full.data() + size_t(fy) * full_stride;
                for (uint32_t px = 0; px < Wp; px++) {
                    uint32_t fx = a.xs + px * a.xstep;
                    memcpy(dst + fx * pxb, src + px * pxb, pxb);
                }
            }
        } else if (bits == 4) {
            for (uint32_t py = 0; py < Hp; py++) {
                uint32_t fy = a.ys + py * a.ystep;
                const uint8_t* src = pass_raws[p].data() + size_t(py) * pass_stride;
                uint8_t*       dst = raw_full.data() + size_t(fy) * full_stride;
                for (uint32_t px = 0; px < Wp; px++) {
                    uint32_t fx = a.xs + px * a.xstep;
                    uint8_t v = (px & 1) ? (src[px >> 1] & 0xf) : (src[px >> 1] >> 4);
                    if (fx & 1) dst[fx >> 1] = uint8_t((dst[fx >> 1] & 0xf0) | v);
                    else        dst[fx >> 1] = uint8_t((dst[fx >> 1] & 0x0f) | (v << 4));
                }
            }
        }
    }
}
static void interlace_from_raw(uint32_t W, uint32_t H, uint8_t ct, uint8_t bd,
                               const std::vector<uint8_t>& raw_full,
                               std::vector<std::vector<uint8_t>>& pass_raws)
{
    uint32_t bits = samples_per_pixel(ct) * bd;
    uint32_t full_stride = (W * bits + 7) / 8;
    for (int p = 0; p < 7; p++) {
        uint32_t Wp, Hp; pass_dims(p, W, H, Wp, Hp);
        if (Wp == 0 || Hp == 0) { pass_raws[p].clear(); continue; }
        uint32_t pass_stride = (Wp * bits + 7) / 8;
        pass_raws[p].assign(size_t(Hp) * pass_stride, 0);
        const A7Pass& a = A7P[p];
        if (bits >= 8) {
            uint32_t pxb = bits / 8;
            for (uint32_t py = 0; py < Hp; py++) {
                uint32_t fy = a.ys + py * a.ystep;
                uint8_t*       dst = pass_raws[p].data() + size_t(py) * pass_stride;
                const uint8_t* src = raw_full.data() + size_t(fy) * full_stride;
                for (uint32_t px = 0; px < Wp; px++) {
                    uint32_t fx = a.xs + px * a.xstep;
                    memcpy(dst + px * pxb, src + fx * pxb, pxb);
                }
            }
        } else if (bits == 4) {
            for (uint32_t py = 0; py < Hp; py++) {
                uint32_t fy = a.ys + py * a.ystep;
                uint8_t*       dst = pass_raws[p].data() + size_t(py) * pass_stride;
                const uint8_t* src = raw_full.data() + size_t(fy) * full_stride;
                for (uint32_t px = 0; px < Wp; px++) {
                    uint32_t fx = a.xs + px * a.xstep;
                    uint8_t v = (fx & 1) ? (src[fx >> 1] & 0xf) : (src[fx >> 1] >> 4);
                    if (px & 1) dst[px >> 1] = uint8_t((dst[px >> 1] & 0xf0) | v);
                    else        dst[px >> 1] = uint8_t((dst[px >> 1] & 0x0f) | (v << 4));
                }
            }
        }
    }
}

// unp → raw_full + per-row filter bytes (concatenated for Adam7).
static bool unp_to_raw(uint32_t W, uint32_t H, uint8_t ct, uint8_t bd,
                       uint8_t interlace, const std::vector<uint8_t>& unp,
                       std::vector<uint8_t>& raw_full,
                       std::vector<uint8_t>& filters)
{
    uint32_t bits = samples_per_pixel(ct) * bd;
    uint32_t bpp_f = (bits + 7) / 8;
    if (interlace == 0) {
        uint32_t stride = (W * bits + 7) / 8;
        size_t expected = size_t(H) * (1 + stride);
        if (unp.size() != expected)
            return err("unp size %zu != expected %zu", unp.size(), expected);
        return defilter(unp.data(), H, stride, bpp_f, raw_full, filters);
    }
    if (interlace != 1) return err("unknown interlace %u", interlace);

    std::vector<std::vector<uint8_t>> pass_raws(7);
    filters.clear();
    const uint8_t* p = unp.data();
    const uint8_t* end = unp.data() + unp.size();
    for (int pass = 0; pass < 7; pass++) {
        uint32_t Wp, Hp; pass_dims(pass, W, H, Wp, Hp);
        if (Wp == 0 || Hp == 0) continue;
        uint32_t stride_p = (Wp * bits + 7) / 8;
        size_t need = size_t(Hp) * (1 + stride_p);
        if (size_t(end - p) < need) return err("unp truncated in Adam7 pass %d", pass);
        std::vector<uint8_t> pf;
        if (!defilter(p, Hp, stride_p, bpp_f, pass_raws[pass], pf)) return false;
        filters.insert(filters.end(), pf.begin(), pf.end());
        p += need;
    }
    deinterlace_to_raw(W, H, ct, bd, pass_raws, raw_full);
    return true;
}

static bool raw_to_unp(uint32_t W, uint32_t H, uint8_t ct, uint8_t bd,
                       uint8_t interlace, const std::vector<uint8_t>& raw_full,
                       const std::vector<uint8_t>& filters,
                       std::vector<uint8_t>& unp)
{
    uint32_t bits = samples_per_pixel(ct) * bd;
    uint32_t bpp_f = (bits + 7) / 8;
    if (interlace == 0) {
        uint32_t stride = (W * bits + 7) / 8;
        if (filters.size() != H) return err("filter count %zu != H %u", filters.size(), H);
        refilter(raw_full.data(), H, stride, bpp_f, filters.data(), unp);
        return true;
    }
    if (interlace != 1) return err("unknown interlace %u", interlace);

    std::vector<std::vector<uint8_t>> pass_raws(7);
    interlace_from_raw(W, H, ct, bd, raw_full, pass_raws);
    unp.clear();
    size_t fpos = 0;
    for (int pass = 0; pass < 7; pass++) {
        uint32_t Wp, Hp; pass_dims(pass, W, H, Wp, Hp);
        if (Wp == 0 || Hp == 0) continue;
        uint32_t stride_p = (Wp * bits + 7) / 8;
        std::vector<uint8_t> pu;
        refilter(pass_raws[pass].data(), Hp, stride_p, bpp_f,
                 filters.data() + fpos, pu);
        unp.insert(unp.end(), pu.begin(), pu.end());
        fpos += Hp;
    }
    return true;
}

/* ─── BMP I/O (from png2bmp) ─────────────────────────────────────────────── */

enum BmpMode { BMP_24, BMP_32, BMP_8PAL, BMP_4PAL };

static uint32_t bmp_bits_per_pixel(BmpMode m) {
    switch (m) {
        case BMP_4PAL: return 4; case BMP_8PAL: return 8;
        case BMP_24:   return 24; case BMP_32:  return 32;
    }
    return 0;
}
static inline bool bmp_is_paletted(BmpMode m) {
    return m == BMP_4PAL || m == BMP_8PAL;
}
static uint32_t bmp_bytes_per_row(uint32_t W, uint32_t bits) {
    return (W * bits + 7u) / 8u;
}
static uint32_t bmp_padded_row(uint32_t W, uint32_t bits) {
    return (bmp_bytes_per_row(W, bits) + 3u) & ~3u;
}

static bool write_bmp(BmpMode mode, uint32_t W, uint32_t H,
                      const std::vector<uint8_t>& pixels,
                      const std::vector<uint8_t>& palette,
                      std::vector<uint8_t>& out)
{
    uint32_t bits    = bmp_bits_per_pixel(mode);
    uint32_t bpr_raw = bmp_bytes_per_row(W, bits);
    uint32_t row_b   = bmp_padded_row(W, bits);
    uint32_t pad     = row_b - bpr_raw;
    uint32_t img_b   = row_b * H;
    bool     paletted = bmp_is_paletted(mode);
    uint32_t pal_b   = paletted ? uint32_t(palette.size()) : 0;
    uint32_t off     = 14 + 40 + pal_b;
    uint32_t total   = off + img_b;
    out.clear();
    out.reserve(total);
    out.push_back('B'); out.push_back('M');
    wr_le32(out, total);
    wr_le16(out, 0); wr_le16(out, 0);
    wr_le32(out, off);
    wr_le32(out, 40);
    wr_le32(out, W);
    wr_le32(out, H);
    wr_le16(out, 1);
    wr_le16(out, uint16_t(bits));
    wr_le32(out, 0);
    wr_le32(out, img_b);
    wr_le32(out, 2835); wr_le32(out, 2835);
    if (paletted) {
        wr_le32(out, uint32_t(palette.size() / 4));
        wr_le32(out, uint32_t(palette.size() / 4));
    } else { wr_le32(out, 0); wr_le32(out, 0); }
    if (paletted) out.insert(out.end(), palette.begin(), palette.end());
    uint8_t zeros[4] = {0, 0, 0, 0};
    if (pixels.size() != size_t(bpr_raw) * H)
        return err("BMP write: %zu bytes, expected %zu",
                   pixels.size(), size_t(bpr_raw) * H);
    for (int r = int(H) - 1; r >= 0; r--) {
        const uint8_t* row = pixels.data() + size_t(r) * bpr_raw;
        out.insert(out.end(), row, row + bpr_raw);
        if (pad) out.insert(out.end(), zeros, zeros + pad);
    }
    return true;
}

static bool read_bmp(const std::vector<uint8_t>& bmp,
                     uint32_t& W, uint32_t& H, uint32_t& bits,
                     std::vector<uint8_t>& palette,
                     std::vector<uint8_t>& pixels)
{
    if (bmp.size() < 54 || bmp[0] != 'B' || bmp[1] != 'M')
        return err("BMP read: bad header");
    uint32_t off    = rd_le32(bmp.data() + 10);
    uint32_t infosz = rd_le32(bmp.data() + 14);
    if (infosz < 40) return err("BMP info header too small");
    int32_t  ws = int32_t(rd_le32(bmp.data() + 18));
    int32_t  hs = int32_t(rd_le32(bmp.data() + 22));
    uint16_t bc = rd_le16(bmp.data() + 28);
    uint32_t cp = rd_le32(bmp.data() + 30);
    if (cp != 0) return err("BMP read: only BI_RGB supported");
    if (ws <= 0) return err("BMP read: non-positive width");
    bool top_down = (hs < 0);
    if (hs < 0) hs = -hs;
    if (hs <= 0) return err("BMP read: zero height");
    if (bc != 4 && bc != 8 && bc != 24 && bc != 32)
        return err("BMP read: unsupported bit count %u", bc);
    W = uint32_t(ws); H = uint32_t(hs); bits = bc;
    palette.clear();
    if (bc == 4 || bc == 8) {
        uint32_t nclr = rd_le32(bmp.data() + 46);
        if (nclr == 0) nclr = (bc == 4) ? 16 : 256;
        uint32_t pal_off = 14 + infosz;
        if (pal_off + nclr * 4 > bmp.size()) return err("BMP palette truncated");
        palette.assign(bmp.data() + pal_off, bmp.data() + pal_off + nclr * 4);
    }
    uint32_t bpr_raw = bmp_bytes_per_row(W, bits);
    uint32_t row_b   = bmp_padded_row(W, bits);
    pixels.assign(size_t(bpr_raw) * H, 0);
    for (uint32_t r = 0; r < H; r++) {
        uint32_t src_row = top_down ? r : (H - 1 - r);
        size_t src = off + size_t(src_row) * row_b;
        if (src + bpr_raw > bmp.size()) return err("BMP pixel data truncated");
        memcpy(pixels.data() + size_t(r) * bpr_raw, bmp.data() + src, bpr_raw);
    }
    return true;
}

/* ─── PNG raw ↔ BMP pixel layout ─────────────────────────────────────────── */

static bool png_raw_to_bmp(uint8_t ct, uint8_t bd,
                           uint32_t W, uint32_t H,
                           const std::vector<uint8_t>& plte,
                           const std::vector<uint8_t>& raw,
                           BmpMode& mode,
                           std::vector<uint8_t>& bmp_pixels,
                           std::vector<uint8_t>& bmp_palette)
{
    size_t N = size_t(W) * H;
    bmp_palette.clear();
    switch (ct) {
    case 0:
        if (bd == 4) {
            mode = BMP_4PAL;
            bmp_pixels = raw;
            bmp_palette.resize(16 * 4);
            for (int i = 0; i < 16; i++) {
                uint8_t v = uint8_t(i * 17);
                bmp_palette[i*4+0]=v; bmp_palette[i*4+1]=v;
                bmp_palette[i*4+2]=v; bmp_palette[i*4+3]=0;
            }
            return true;
        }
        mode = BMP_8PAL;
        bmp_pixels = raw;
        bmp_palette.resize(256 * 4);
        for (int i = 0; i < 256; i++) {
            bmp_palette[i*4+0]=uint8_t(i); bmp_palette[i*4+1]=uint8_t(i);
            bmp_palette[i*4+2]=uint8_t(i); bmp_palette[i*4+3]=0;
        }
        return true;
    case 2:
        mode = BMP_24;
        bmp_pixels.resize(N * 3);
        for (size_t i = 0; i < N; i++) {
            bmp_pixels[i*3+0]=raw[i*3+2];
            bmp_pixels[i*3+1]=raw[i*3+1];
            bmp_pixels[i*3+2]=raw[i*3+0];
        }
        return true;
    case 3: {
        if (plte.empty()) return err("palette PNG with no PLTE");
        mode = (bd == 4) ? BMP_4PAL : BMP_8PAL;
        bmp_pixels = raw;
        uint32_t n = uint32_t(plte.size() / 3);
        bmp_palette.resize(size_t(n) * 4);
        for (uint32_t i = 0; i < n; i++) {
            bmp_palette[i*4+0]=plte[i*3+2];
            bmp_palette[i*4+1]=plte[i*3+1];
            bmp_palette[i*4+2]=plte[i*3+0];
            bmp_palette[i*4+3]=0;
        }
        return true;
    }
    case 4:
        mode = BMP_32;
        bmp_pixels.resize(N * 4);
        for (size_t i = 0; i < N; i++) {
            uint8_t g = raw[i*2+0]; uint8_t a = raw[i*2+1];
            bmp_pixels[i*4+0]=g; bmp_pixels[i*4+1]=g;
            bmp_pixels[i*4+2]=g; bmp_pixels[i*4+3]=a;
        }
        return true;
    case 6:
        mode = BMP_32;
        bmp_pixels.resize(N * 4);
        for (size_t i = 0; i < N; i++) {
            bmp_pixels[i*4+0]=raw[i*4+2];
            bmp_pixels[i*4+1]=raw[i*4+1];
            bmp_pixels[i*4+2]=raw[i*4+0];
            bmp_pixels[i*4+3]=raw[i*4+3];
        }
        return true;
    default: return err("unsupported color type %u", ct);
    }
}

static bool bmp_to_png_raw(uint8_t ct, uint8_t bd, uint32_t W, uint32_t H,
                           uint32_t bmp_bits,
                           const std::vector<uint8_t>& bmp_pixels,
                           std::vector<uint8_t>& raw)
{
    size_t N = size_t(W) * H;
    auto check_bits = [&](uint32_t want) {
        if (bmp_bits != want)
            return err("BMP %u-bit doesn't match PNG ct=%u bd=%u (need %u-bit)",
                       bmp_bits, ct, bd, want);
        return true;
    };
    switch (ct) {
    case 0:
        if (bd == 4) { if (!check_bits(4)) return false; raw = bmp_pixels; return true; }
        if (!check_bits(8)) return false; raw = bmp_pixels; return true;
    case 2:
        if (!check_bits(24)) return false;
        raw.resize(N * 3);
        for (size_t i = 0; i < N; i++) {
            raw[i*3+0]=bmp_pixels[i*3+2];
            raw[i*3+1]=bmp_pixels[i*3+1];
            raw[i*3+2]=bmp_pixels[i*3+0];
        }
        return true;
    case 3:
        if (!check_bits(bd == 4 ? 4 : 8)) return false;
        raw = bmp_pixels; return true;
    case 4:
        if (!check_bits(32)) return false;
        raw.resize(N * 2);
        for (size_t i = 0; i < N; i++) {
            raw[i*2+0]=bmp_pixels[i*4+0];
            raw[i*2+1]=bmp_pixels[i*4+3];
        }
        return true;
    case 6:
        if (!check_bits(32)) return false;
        raw.resize(N * 4);
        for (size_t i = 0; i < N; i++) {
            raw[i*4+0]=bmp_pixels[i*4+2];
            raw[i*4+1]=bmp_pixels[i*4+1];
            raw[i*4+2]=bmp_pixels[i*4+0];
            raw[i*4+3]=bmp_pixels[i*4+3];
        }
        return true;
    default: return err("unsupported color type %u", ct);
    }
}

static bool check_supported(const Ihdr& h) {
    uint8_t ct = h.color_type, bd = h.bit_depth;
    if (samples_per_pixel(ct) == 0) return err("unknown color type %u", ct);
    if (h.width == 0 || h.height == 0)
        return err("zero-sized PNG (%ux%u)", h.width, h.height);
    bool ok = (ct == 0 && (bd == 4 || bd == 8))
           || (ct == 3 && (bd == 4 || bd == 8))
           || ((ct == 2 || ct == 4 || ct == 6) && bd == 8);
    if (!ok) return err("unsupported color_type=%u bit_depth=%u", ct, bd);
    return true;
}

/* ─── PLTE lookup ────────────────────────────────────────────────────────── */

static void find_plte(const std::vector<ChunkEntry>& entries,
                      std::vector<uint8_t>& plte)
{
    plte.clear();
    for (auto& e : entries)
        if (e.kind == KIND_RAW && memcmp(e.type, "PLTE", 4) == 0)
            plte = e.data;
}

/* ─── output filename templating ─────────────────────────────────────────── */

static int suffix_width(uint32_t n) {
    if (n <= 1) return 0;
    uint32_t m = n - 1; int d = 1;
    while (m >= 10) { m /= 10; d++; }
    return d < 2 ? 2 : d;
}
// "foo.bmp", n=3 → "foo0.bmp", "foo1.bmp", "foo2.bmp" (width=2 → "foo00..")
// "foo.bmp", n=1 → "foo.bmp"
static std::string frame_bmp_name(const std::string& tmpl, uint32_t i, uint32_t n) {
    if (n <= 1) return tmpl;
    fs::path p(tmpl);
    std::string stem = p.stem().string();
    std::string ext  = p.extension().string();
    fs::path dir = p.parent_path();
    int w = suffix_width(n);
    char buf[32]; snprintf(buf, sizeof(buf), "%0*u", w, i);
    std::string nm = stem + buf + ext;
    return dir.empty() ? nm : (dir / nm).string();
}

/* ─── encode ─────────────────────────────────────────────────────────────── */

static bool encode(const std::string& png_path,
                   const std::string& bmp_template,
                   const std::string& meta_path)
{
    std::vector<uint8_t> png;
    if (!read_file(png_path, png)) return err("cannot read %s", png_path.c_str());

    std::vector<ChunkEntry> entries;
    std::vector<StreamSplit> streams;
    std::vector<uint8_t> trailer;
    Ihdr ihdr{};
    if (!dissect_png(png, entries, streams, trailer, ihdr)) return false;
    if (!check_supported(ihdr)) return false;

    std::vector<uint8_t> plte;
    find_plte(entries, plte);

    uint32_t n = (uint32_t)streams.size();

    // Per-stream: preflate decode, then unp→raw_full→BMP. The default stream
    // (i=0) uses IHDR dims; for APNG fcTL-driven streams the dims come from
    // dissect_png's stream_dims fill.
    std::vector<std::vector<uint8_t>> hifs(n);
    std::vector<std::vector<uint8_t>> filters_all(n);
    std::vector<std::vector<uint8_t>> bmp_blobs(n);
    std::vector<std::pair<uint32_t,uint32_t>> dims(n);

    for (uint32_t i = 0; i < n; i++) {
        uint32_t W = streams[i].W, H = streams[i].H;
        dims[i] = {W, H};
        if (streams[i].deflate.empty()) {
            // No payload — write empty BMP marker? Skip — meta will indicate.
            continue;
        }
        std::vector<unsigned char> unpacked, recon;
        if (!preflate_decode(unpacked, recon, streams[i].deflate))
            return err("preflate_decode failed on stream %u", i);
        hifs[i] = std::move(recon);
        std::vector<uint8_t> unp(unpacked.begin(), unpacked.end());

        // unp → raw_full + filters. fdAT streams (i>=1) are non-interlaced
        // per the APNG spec, even when IHDR declares Adam7.
        uint8_t stream_interlace = (i == 0) ? ihdr.interlace : 0;
        std::vector<uint8_t> raw_full, filters;
        if (!unp_to_raw(W, H, ihdr.color_type, ihdr.bit_depth, stream_interlace,
                        unp, raw_full, filters))
            return false;
        filters_all[i] = std::move(filters);

        // raw_full → BMP
        BmpMode mode;
        std::vector<uint8_t> bmp_pixels, bmp_palette;
        if (!png_raw_to_bmp(ihdr.color_type, ihdr.bit_depth, W, H,
                            plte, raw_full, mode, bmp_pixels, bmp_palette))
            return false;
        if (!write_bmp(mode, W, H, bmp_pixels, bmp_palette, bmp_blobs[i]))
            return false;
    }

    // Write BMPs.
    for (uint32_t i = 0; i < n; i++) {
        if (bmp_blobs[i].empty()) continue;
        std::string p = frame_bmp_name(bmp_template, i, n);
        if (!write_file(p, bmp_blobs[i]))
            return err("cannot write %s", p.c_str());
    }

    // Build meta.
    std::vector<uint8_t> pngdump_blob;
    build_pngdump_blob(entries, streams, trailer, pngdump_blob);

    std::vector<uint8_t> meta;
    meta.insert(meta.end(), PNGTOOL_MAGIC, PNGTOOL_MAGIC + 8);
    wr_le32(meta, n);
    wr_le32(meta, (uint32_t)pngdump_blob.size());
    meta.insert(meta.end(), pngdump_blob.begin(), pngdump_blob.end());
    for (uint32_t i = 0; i < n; i++) {
        wr_le32(meta, dims[i].first);
        wr_le32(meta, dims[i].second);
        wr_le32(meta, (uint32_t)hifs[i].size());
        meta.insert(meta.end(), hifs[i].begin(), hifs[i].end());
        wr_le32(meta, (uint32_t)filters_all[i].size());
        meta.insert(meta.end(), filters_all[i].begin(), filters_all[i].end());
    }
    if (!write_file(meta_path, meta))
        return err("cannot write %s", meta_path.c_str());

    fprintf(stdout,
            "encoded: %s -> %u stream%s, meta=%zu bytes\n",
            png_path.c_str(), n, n == 1 ? "" : "s", meta.size());
    return true;
}

/* ─── decode ─────────────────────────────────────────────────────────────── */

static bool decode(const std::string& bmp_template,
                   const std::string& meta_path,
                   const std::string& png_path)
{
    std::vector<uint8_t> meta;
    if (!read_file(meta_path, meta)) return err("cannot read %s", meta_path.c_str());
    if (meta.size() < 16 || memcmp(meta.data(), PNGTOOL_MAGIC, 8) != 0)
        return err("bad pngtool meta magic in %s", meta_path.c_str());
    size_t p = 8;
    uint32_t n = rd_le32(meta.data() + p); p += 4;
    uint32_t blob_len = rd_le32(meta.data() + p); p += 4;
    if (p + blob_len > meta.size()) return err("meta blob truncated");
    std::vector<uint8_t> blob(meta.begin() + p, meta.begin() + p + blob_len);
    p += blob_len;

    std::vector<ChunkEntry> entries;
    uint32_t n2 = 0;
    std::vector<std::vector<uint8_t>> hdrs, adls;
    std::vector<uint8_t> trailer;
    Ihdr ihdr{};
    if (!parse_pngdump_blob(blob, entries, n2, hdrs, adls, trailer, ihdr)) return false;
    if (n != n2) return err("meta stream count mismatch (%u vs blob %u)", n, n2);
    if (!check_supported(ihdr)) return false;

    std::vector<uint8_t> plte;
    find_plte(entries, plte);

    std::vector<std::vector<uint8_t>> deflates(n);

    for (uint32_t i = 0; i < n; i++) {
        if (p + 16 > meta.size()) return err("meta stream %u header truncated", i);
        uint32_t W = rd_le32(meta.data() + p); p += 4;
        uint32_t H = rd_le32(meta.data() + p); p += 4;
        uint32_t hif_len = rd_le32(meta.data() + p); p += 4;
        if (p + hif_len > meta.size()) return err("meta stream %u hif truncated", i);
        std::vector<uint8_t> hif(meta.begin() + p, meta.begin() + p + hif_len);
        p += hif_len;
        if (p + 4 > meta.size()) return err("meta stream %u filters len missing", i);
        uint32_t flt_len = rd_le32(meta.data() + p); p += 4;
        if (p + flt_len > meta.size()) return err("meta stream %u filters truncated", i);
        std::vector<uint8_t> filters(meta.begin() + p, meta.begin() + p + flt_len);
        p += flt_len;

        if (hdrs[i].empty()) continue;     // empty stream

        // Load BMP
        std::string bp = frame_bmp_name(bmp_template, i, n);
        std::vector<uint8_t> bmp_bytes;
        if (!read_file(bp, bmp_bytes)) return err("cannot read %s", bp.c_str());
        uint32_t bw = 0, bh = 0, bbits = 0;
        std::vector<uint8_t> bmp_pal, bmp_pixels;
        if (!read_bmp(bmp_bytes, bw, bh, bbits, bmp_pal, bmp_pixels)) return false;
        if (bw != W || bh != H)
            return err("BMP %u is %ux%u, expected %ux%u", i, bw, bh, W, H);

        std::vector<uint8_t> raw_full;
        if (!bmp_to_png_raw(ihdr.color_type, ihdr.bit_depth, W, H, bbits,
                            bmp_pixels, raw_full)) return false;

        std::vector<uint8_t> unp;
        uint8_t stream_interlace = (i == 0) ? ihdr.interlace : 0;
        if (!raw_to_unp(W, H, ihdr.color_type, ihdr.bit_depth, stream_interlace,
                        raw_full, filters, unp)) return false;

        std::vector<unsigned char> deflate_raw;
        std::vector<unsigned char> recon(hif.begin(), hif.end());
        std::vector<unsigned char> unp_uc(unp.begin(), unp.end());
        if (!preflate_reencode(deflate_raw, recon, unp_uc))
            return err("preflate_reencode failed on stream %u", i);
        deflates[i].assign(deflate_raw.begin(), deflate_raw.end());
    }

    // Now reconstruct PNG using entries + per-stream (hdr + deflate + adler).
    std::vector<std::vector<uint8_t>> stream_bytes(n);
    std::vector<size_t> off(n, 0);
    for (uint32_t i = 0; i < n; i++) {
        if (hdrs[i].empty()) continue;
        stream_bytes[i].reserve(hdrs[i].size() + deflates[i].size() + 4);
        stream_bytes[i].insert(stream_bytes[i].end(), hdrs[i].begin(), hdrs[i].end());
        stream_bytes[i].insert(stream_bytes[i].end(),
                               deflates[i].begin(), deflates[i].end());
        stream_bytes[i].insert(stream_bytes[i].end(), adls[i].begin(), adls[i].end());
    }

    std::vector<uint8_t> out;
    out.insert(out.end(), PNG_SIG, PNG_SIG + 8);
    for (auto& e : entries) {
        if (e.kind == KIND_RAW) {
            uint32_t crc = chunk_crc(e.type, e.data.data(), e.length);
            wr_be32(out, e.length);
            out.insert(out.end(), e.type, e.type + 4);
            out.insert(out.end(), e.data.begin(), e.data.end());
            wr_be32(out, crc);
        } else if (e.kind == KIND_IDAT) {
            uint32_t sidx = e.stream_idx;
            if (sidx >= n || off[sidx] + e.length > stream_bytes[sidx].size())
                return err("IDAT slice out of range (stream %u)", sidx);
            uint8_t type[4] = {'I','D','A','T'};
            const uint8_t* dp = stream_bytes[sidx].data() + off[sidx];
            uint32_t crc = chunk_crc(type, dp, e.length);
            wr_be32(out, e.length);
            out.insert(out.end(), type, type + 4);
            out.insert(out.end(), dp, dp + e.length);
            wr_be32(out, crc);
            off[sidx] += e.length;
        } else {
            uint32_t sidx = e.stream_idx;
            if (sidx >= n || off[sidx] + e.length > stream_bytes[sidx].size())
                return err("fdAT slice out of range (stream %u)", sidx);
            uint8_t type[4] = {'f','d','A','T'};
            std::vector<uint8_t> payload;
            payload.reserve(4 + size_t(e.length));
            wr_be32(payload, e.sequence);
            payload.insert(payload.end(),
                           stream_bytes[sidx].data() + off[sidx],
                           stream_bytes[sidx].data() + off[sidx] + e.length);
            uint32_t total_len = (uint32_t)payload.size();
            uint32_t crc = chunk_crc(type, payload.data(), total_len);
            wr_be32(out, total_len);
            out.insert(out.end(), type, type + 4);
            out.insert(out.end(), payload.begin(), payload.end());
            wr_be32(out, crc);
            off[sidx] += e.length;
        }
    }
    out.insert(out.end(), trailer.begin(), trailer.end());
    for (uint32_t i = 0; i < n; i++) {
        if (off[i] != stream_bytes[i].size())
            return err("stream %u: consumed %zu of %zu bytes",
                       i, off[i], stream_bytes[i].size());
    }
    if (!write_file(png_path, out)) return err("cannot write %s", png_path.c_str());

    fprintf(stdout, "decoded: %u stream%s -> %s (%zu bytes)\n",
            n, n == 1 ? "" : "s", png_path.c_str(), out.size());
    return true;
}

/* ─── main ───────────────────────────────────────────────────────────────── */

static void usage() {
    fprintf(stderr,
        "pngtool - integrated PNG recompression (pngdump + png2bmp + preflate)\n"
        "\n"
        "  pngtool c input.png output.bmp output.meta\n"
        "  pngtool d input.bmp input.meta output.png\n"
        "\n"
        "For multi-frame PNG/APNG: the BMP path becomes a template — a\n"
        "zero-padded index is inserted before the extension, so \"out.bmp\"\n"
        "with 3 frames becomes out00.bmp, out01.bmp, out02.bmp.\n");
}

int main(int argc, char** argv) {
    if (argc != 5) { usage(); return 1; }
    std::string mode = argv[1];
    if (mode == "c") return encode(argv[2], argv[3], argv[4]) ? 0 : 1;
    if (mode == "d") return decode(argv[2], argv[3], argv[4]) ? 0 : 1;
    usage();
    return 1;
}
