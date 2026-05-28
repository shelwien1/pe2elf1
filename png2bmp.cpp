/* png2bmp - PNG dump set ↔ plain BMP, companion to pngdump.
 *
 * Pipeline:
 *   PNG --[pngdump]--> meta + dumpNN.bin   (raw zlib streams)
 *       --[inflate]--> meta + dumpNN.unp   (filter byte + scanline rows)
 *       --[png2bmp]--> out.bmp + out.bmp.meta2
 *
 * c mode (compose):
 *   png2bmp c <meta-path> <output.bmp>
 *   Defilters stream 0 → raw pixels → plain BMP. Side-channel meta2
 *   holds the original meta verbatim plus per-row filter types and any
 *   extra APNG streams (dumpNN.unp for N>0) verbatim.
 *
 * d mode (decompose):
 *   png2bmp d <input.bmp> <meta-path>
 *   Reads <input.bmp> + <input.bmp>.meta2. Re-filters BMP pixels with
 *   the stored filter types → dump00.unp; restores meta and any extra
 *   dumpNN.unp byte-exactly.
 *
 * BMP form (BITMAPINFOHEADER 40B, BI_RGB, bottom-up, no extensions):
 *   PNG ct=0 bd=4 (gray 4-bit)      → 4-bit BMP, grayscale palette (0,17,..,255)
 *   PNG ct=0 bd=8 (gray 8-bit)      → 8-bit BMP, grayscale palette
 *   PNG ct=2 bd=8 (RGB 8-bit)       → 24-bit BGR
 *   PNG ct=3 bd=4 (palette 4-bit)   → 4-bit BMP, PLTE as palette
 *   PNG ct=3 bd=8 (palette 8-bit)   → 8-bit BMP, PLTE as palette
 *   PNG ct=4 bd=8 (gray+alpha 8-bit)→ 32-bit BGRA (B=G=R=g)
 *   PNG ct=6 bd=8 (RGBA 8-bit)      → 32-bit BGRA
 * Unsupported combos (1/2-bit, 16-bit, etc.): error out cleanly.
 *
 * To get dumpNN.unp from dumpNN.bin:
 *   python3 -c "import zlib,sys;\
 *     open(sys.argv[2],'wb').write(zlib.decompress(open(sys.argv[1],'rb').read()))" \
 *     dump00.bin dump00.unp
 *
 * Build: c++ -O2 -std=c++17 png2bmp.cpp -o png2bmp
 */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

/* ─── magic ──────────────────────────────────────────────────────────────── */

static const uint8_t PNGDUMP_MAGIC[8]  = {'P','N','G','D','U','M','P', 0x02};
static const uint8_t P2BMETA2_MAGIC[8] = {'P','2','B','M','E','T','A','2'};

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

/* ─── err ────────────────────────────────────────────────────────────────── */

static bool err(const char* fmt, ...) {
    fprintf(stderr, "png2bmp: ");
    va_list ap; va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    return false;
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

/* ─── pngdump meta parsing ───────────────────────────────────────────────── */

struct Ihdr {
    uint32_t width = 0, height = 0;
    uint8_t  bit_depth = 0, color_type = 0;
    uint8_t  interlace = 0;           // 0 = none, 1 = Adam7
};

struct Meta {
    Ihdr     ihdr;
    uint32_t n_streams = 0;
    std::vector<uint8_t> plte;   // RGB triplets, may be empty
    bool     has_trns = false;
};

static bool parse_meta(const std::vector<uint8_t>& m, Meta& out) {
    if (m.size() < 16 || memcmp(m.data(), PNGDUMP_MAGIC, 8) != 0)
        return err("not a pngdump meta file (bad magic)");
    size_t p = 8;
    out.n_streams = rd_le32(m.data() + p); p += 4;
    uint32_t n_chunks = rd_le32(m.data() + p); p += 4;
    // v2 per-stream block: [1] hlen, [hlen] zlib header, [4] adler32 — one
    // entry per stream. hlen=0 marks an empty stream (no header, no adler).
    for (uint32_t i = 0; i < out.n_streams; i++) {
        if (p + 1 > m.size()) return err("meta truncated at stream %u header", i);
        uint8_t hlen = m[p++];
        if (hlen == 0) continue;
        if (hlen != 2 && hlen != 6) return err("stream %u: invalid hlen %u", i, hlen);
        if (p + size_t(hlen) + 4 > m.size())
            return err("meta truncated at stream %u body", i);
        p += size_t(hlen) + 4;
    }
    bool ihdr_done = false;
    for (uint32_t i = 0; i < n_chunks; i++) {
        if (p + 5 > m.size()) return err("meta truncated at chunk %u", i);
        uint8_t kind = m[p++];
        uint32_t length = rd_le32(m.data() + p); p += 4;
        if (kind == KIND_RAW) {
            if (p + 4 + length > m.size()) return err("meta truncated in RAW %u", i);
            const uint8_t* type = m.data() + p; p += 4;
            const uint8_t* data = m.data() + p; p += length;
            if (memcmp(type, "IHDR", 4) == 0) {
                if (length != 13) return err("IHDR not 13 bytes");
                out.ihdr.width      = rd_be32(data);
                out.ihdr.height     = rd_be32(data + 4);
                out.ihdr.bit_depth  = data[8];
                out.ihdr.color_type = data[9];
                out.ihdr.interlace  = data[12];
                ihdr_done = true;
            } else if (memcmp(type, "PLTE", 4) == 0) {
                if (length % 3 != 0) return err("PLTE length not /3");
                out.plte.assign(data, data + length);
            } else if (memcmp(type, "tRNS", 4) == 0) {
                out.has_trns = true;
            }
        } else if (kind == KIND_IDAT) {
            if (p + 4 > m.size()) return err("meta truncated in IDAT %u", i);
            p += 4;
        } else if (kind == KIND_FDAT) {
            if (p + 8 > m.size()) return err("meta truncated in fdAT %u", i);
            p += 8;
        } else {
            return err("unknown chunk kind %u at %u", kind, i);
        }
    }
    if (!ihdr_done) return err("meta has no IHDR");
    return true;
}

/* ─── png stride / bpp ───────────────────────────────────────────────────── */

static uint32_t samples_per_pixel(uint8_t ct) {
    switch (ct) {
        case 0: return 1;   // gray
        case 2: return 3;   // RGB
        case 3: return 1;   // palette index
        case 4: return 2;   // gray + alpha
        case 6: return 4;   // RGBA
        default: return 0;
    }
}

// Bytes per scanline (excluding filter byte). Handles sub-byte packing.
static uint32_t png_stride(const Ihdr& h) {
    uint32_t bits = samples_per_pixel(h.color_type) * h.bit_depth;
    return (h.width * bits + 7) / 8;
}

// bpp for filter math: ceil(bits_per_pixel / 8). For 8-bit color types this
// equals samples_per_pixel.
static uint32_t png_filter_bpp(const Ihdr& h) {
    uint32_t bits = samples_per_pixel(h.color_type) * h.bit_depth;
    return (bits + 7) / 8;
}

/* ─── PNG filters: defilter & refilter ───────────────────────────────────── */

static inline uint8_t paeth(uint8_t a, uint8_t b, uint8_t c) {
    int p  = int(a) + int(b) - int(c);
    int pa = std::abs(p - int(a));
    int pb = std::abs(p - int(b));
    int pc = std::abs(p - int(c));
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

// unp [H * (1 + stride)] → raw [H * stride] + filters [H]
static bool defilter(const uint8_t* unp, uint32_t W, uint32_t H,
                     uint32_t stride, uint32_t bpp,
                     std::vector<uint8_t>& raw,
                     std::vector<uint8_t>& filters)
{
    (void)W;
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
            uint8_t b =  prior        ?  prior[i]            : 0;
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

// raw [H * stride] + filters [H] → unp [H * (1 + stride)]
static void refilter(const uint8_t* raw, uint32_t W, uint32_t H,
                     uint32_t stride, uint32_t bpp,
                     const uint8_t* filters,
                     std::vector<uint8_t>& unp)
{
    (void)W;
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
            uint8_t b =  prior        ?  prior[i]            : 0;
            uint8_t c = (prior && i >= bpp) ? prior[i - bpp] : 0;
            uint8_t f;
            switch (ft) {
                case 0: f = x; break;
                case 1: f = uint8_t(x - a); break;
                case 2: f = uint8_t(x - b); break;
                case 3: f = uint8_t(x - uint8_t((unsigned(a) + unsigned(b)) / 2)); break;
                case 4: f = uint8_t(x - paeth(a, b, c)); break;
                default: f = x; break;  // unreachable: defilter rejects bad types
            }
            out[i] = f;
        }
    }
}

/* ─── Adam7 interlacing ──────────────────────────────────────────────────── */

struct A7Pass { uint8_t xs, ys, xstep, ystep; };
static const A7Pass A7P[7] = {
    {0, 0, 8, 8}, {4, 0, 8, 8}, {0, 4, 4, 8},
    {2, 0, 4, 4}, {0, 2, 2, 4}, {1, 0, 2, 2}, {0, 1, 1, 2}
};

static void pass_dims(int p, uint32_t W, uint32_t H,
                      uint32_t& Wp, uint32_t& Hp)
{
    const A7Pass& a = A7P[p];
    Wp = (W > a.xs) ? (W - a.xs + a.xstep - 1) / a.xstep : 0;
    Hp = (H > a.ys) ? (H - a.ys + a.ystep - 1) / a.ystep : 0;
}

// Total filter-byte count across all non-empty passes.
static size_t interlace_total_rows(const Ihdr& h) {
    size_t s = 0;
    for (int p = 0; p < 7; p++) {
        uint32_t Wp, Hp; pass_dims(p, h.width, h.height, Wp, Hp);
        if (Wp > 0 && Hp > 0) s += Hp;
    }
    return s;
}

// Total expected dump00.unp size when interlaced (sum of pass unp sizes).
static size_t interlace_total_unp(const Ihdr& h) {
    uint32_t bits = samples_per_pixel(h.color_type) * h.bit_depth;
    size_t s = 0;
    for (int p = 0; p < 7; p++) {
        uint32_t Wp, Hp; pass_dims(p, h.width, h.height, Wp, Hp);
        if (Wp > 0 && Hp > 0)
            s += size_t(Hp) * (1 + ((Wp * bits + 7) / 8));
    }
    return s;
}

// Scatter the 7 pass pixel grids into one full-image raw buffer.
static void deinterlace_to_raw(const Ihdr& h,
                               const std::vector<std::vector<uint8_t>>& pass_raws,
                               std::vector<uint8_t>& raw_full)
{
    uint32_t W = h.width, H = h.height;
    uint32_t bits = samples_per_pixel(h.color_type) * h.bit_depth;
    uint32_t full_stride = png_stride(h);
    raw_full.assign(size_t(H) * full_stride, 0);

    for (int p = 0; p < 7; p++) {
        if (pass_raws[p].empty()) continue;
        uint32_t Wp, Hp; pass_dims(p, W, H, Wp, Hp);
        uint32_t pass_stride = (Wp * bits + 7) / 8;
        const A7Pass& a = A7P[p];

        if (bits >= 8) {                              // whole-byte pixels
            uint32_t pxb = bits / 8;
            for (uint32_t py = 0; py < Hp; py++) {
                uint32_t fy = a.ys + py * a.ystep;
                const uint8_t* src = pass_raws[p].data() + size_t(py) * pass_stride;
                uint8_t*       dst = raw_full.data()      + size_t(fy) * full_stride;
                for (uint32_t px = 0; px < Wp; px++) {
                    uint32_t fx = a.xs + px * a.xstep;
                    memcpy(dst + fx * pxb, src + px * pxb, pxb);
                }
            }
        } else if (bits == 4) {                        // 4-bit packed nibbles
            for (uint32_t py = 0; py < Hp; py++) {
                uint32_t fy = a.ys + py * a.ystep;
                const uint8_t* src = pass_raws[p].data() + size_t(py) * pass_stride;
                uint8_t*       dst = raw_full.data()      + size_t(fy) * full_stride;
                for (uint32_t px = 0; px < Wp; px++) {
                    uint32_t fx = a.xs + px * a.xstep;
                    uint8_t v = (px & 1) ? (src[px >> 1] & 0xf)
                                          : (src[px >> 1] >> 4);
                    if (fx & 1) dst[fx >> 1] = uint8_t((dst[fx >> 1] & 0xf0) | v);
                    else        dst[fx >> 1] = uint8_t((dst[fx >> 1] & 0x0f) | (v << 4));
                }
            }
        }
        // (other sub-byte depths: not currently supported by check_supported)
    }
}

// Gather: extract pass pixel grids from the full-image raw buffer.
static void interlace_from_raw(const Ihdr& h,
                               const std::vector<uint8_t>& raw_full,
                               std::vector<std::vector<uint8_t>>& pass_raws)
{
    uint32_t W = h.width, H = h.height;
    uint32_t bits = samples_per_pixel(h.color_type) * h.bit_depth;
    uint32_t full_stride = png_stride(h);

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
                const uint8_t* src = raw_full.data()      + size_t(fy) * full_stride;
                for (uint32_t px = 0; px < Wp; px++) {
                    uint32_t fx = a.xs + px * a.xstep;
                    memcpy(dst + px * pxb, src + fx * pxb, pxb);
                }
            }
        } else if (bits == 4) {
            for (uint32_t py = 0; py < Hp; py++) {
                uint32_t fy = a.ys + py * a.ystep;
                uint8_t*       dst = pass_raws[p].data() + size_t(py) * pass_stride;
                const uint8_t* src = raw_full.data()      + size_t(fy) * full_stride;
                for (uint32_t px = 0; px < Wp; px++) {
                    uint32_t fx = a.xs + px * a.xstep;
                    uint8_t v = (fx & 1) ? (src[fx >> 1] & 0xf)
                                          : (src[fx >> 1] >> 4);
                    if (px & 1) dst[px >> 1] = uint8_t((dst[px >> 1] & 0xf0) | v);
                    else        dst[px >> 1] = uint8_t((dst[px >> 1] & 0x0f) | (v << 4));
                }
            }
        }
    }
}

/* ─── unp ↔ raw wrappers (handle non-interlaced + Adam7) ─────────────────── */

// dump00.unp → defiltered full-image raw + per-row filter types.
// For interlaced, filters holds concatenated pass filter bytes (sum of Hp).
static bool unp_to_raw(const Ihdr& h, const std::vector<uint8_t>& unp,
                       std::vector<uint8_t>& raw_full,
                       std::vector<uint8_t>& filters)
{
    uint32_t bpp_f = png_filter_bpp(h);
    if (h.interlace == 0) {
        uint32_t stride = png_stride(h);
        size_t expected = size_t(h.height) * (1 + stride);
        if (unp.size() != expected)
            return err("dump00.unp size %zu != expected %zu "
                       "(H=%u, stride=%u, +1 filter byte/row)",
                       unp.size(), expected, h.height, stride);
        return defilter(unp.data(), h.width, h.height,
                        stride, bpp_f, raw_full, filters);
    }
    if (h.interlace != 1)
        return err("unknown interlace method %u", h.interlace);

    size_t expected = interlace_total_unp(h);
    if (unp.size() != expected)
        return err("dump00.unp size %zu != expected %zu (Adam7, W=%u H=%u)",
                   unp.size(), expected, h.width, h.height);

    std::vector<std::vector<uint8_t>> pass_raws(7);
    filters.clear();
    uint32_t bits = samples_per_pixel(h.color_type) * h.bit_depth;
    const uint8_t* p = unp.data();
    for (int pass = 0; pass < 7; pass++) {
        uint32_t Wp, Hp; pass_dims(pass, h.width, h.height, Wp, Hp);
        if (Wp == 0 || Hp == 0) { pass_raws[pass].clear(); continue; }
        uint32_t stride_p = (Wp * bits + 7) / 8;
        std::vector<uint8_t> pf;
        if (!defilter(p, Wp, Hp, stride_p, bpp_f, pass_raws[pass], pf))
            return false;
        filters.insert(filters.end(), pf.begin(), pf.end());
        p += size_t(Hp) * (1 + stride_p);
    }
    deinterlace_to_raw(h, pass_raws, raw_full);
    return true;
}

// Inverse: full-image raw + filter types → dump00.unp.
static bool raw_to_unp(const Ihdr& h, const std::vector<uint8_t>& raw_full,
                       const std::vector<uint8_t>& filters,
                       std::vector<uint8_t>& unp)
{
    uint32_t bpp_f = png_filter_bpp(h);
    if (h.interlace == 0) {
        if (filters.size() != h.height)
            return err("filter count %zu != height %u",
                       filters.size(), h.height);
        refilter(raw_full.data(), h.width, h.height,
                 png_stride(h), bpp_f, filters.data(), unp);
        return true;
    }
    if (h.interlace != 1)
        return err("unknown interlace method %u", h.interlace);

    size_t need = interlace_total_rows(h);
    if (filters.size() != need)
        return err("filter count %zu != expected %zu (Adam7)",
                   filters.size(), need);

    std::vector<std::vector<uint8_t>> pass_raws(7);
    interlace_from_raw(h, raw_full, pass_raws);

    uint32_t bits = samples_per_pixel(h.color_type) * h.bit_depth;
    unp.clear();
    size_t fpos = 0;
    for (int pass = 0; pass < 7; pass++) {
        uint32_t Wp, Hp; pass_dims(pass, h.width, h.height, Wp, Hp);
        if (Wp == 0 || Hp == 0) continue;
        uint32_t stride_p = (Wp * bits + 7) / 8;
        std::vector<uint8_t> pu;
        refilter(pass_raws[pass].data(), Wp, Hp, stride_p, bpp_f,
                 filters.data() + fpos, pu);
        unp.insert(unp.end(), pu.begin(), pu.end());
        fpos += Hp;
    }
    return true;
}

/* ─── BMP write / read ───────────────────────────────────────────────────── */

enum BmpMode { BMP_24, BMP_32, BMP_8PAL, BMP_4PAL };

static uint32_t bmp_bits_per_pixel(BmpMode m) {
    switch (m) {
        case BMP_4PAL: return 4;
        case BMP_8PAL: return 8;
        case BMP_24:   return 24;
        case BMP_32:   return 32;
    }
    return 0;
}
static inline bool bmp_is_paletted(BmpMode m) {
    return m == BMP_4PAL || m == BMP_8PAL;
}
static uint32_t bmp_bytes_per_row(uint32_t W, uint32_t bits) {
    return (W * bits + 7u) / 8u;                  // unpadded
}
static uint32_t bmp_padded_row(uint32_t W, uint32_t bits) {
    return (bmp_bytes_per_row(W, bits) + 3u) & ~3u;
}

// Write a BMP with BITMAPINFOHEADER (40), BI_RGB, bottom-up.
// pixels: top-down, (bytes_per_row_raw * H) bytes in BMP byte order
// (BGR / BGRA / 8-bit index / packed 4-bit index — high nibble = leftmost).
// palette (for BMP_4PAL/BMP_8PAL): BGRX entries, must already be BMP-ordered.
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

    // BITMAPFILEHEADER (14)
    out.push_back('B'); out.push_back('M');
    wr_le32(out, total);
    wr_le16(out, 0); wr_le16(out, 0);   // reserved
    wr_le32(out, off);

    // BITMAPINFOHEADER (40)
    wr_le32(out, 40);
    wr_le32(out, W);
    wr_le32(out, H);                     // positive = bottom-up
    wr_le16(out, 1);                     // planes
    wr_le16(out, uint16_t(bits));        // bit count
    wr_le32(out, 0);                     // BI_RGB
    wr_le32(out, img_b);                 // biSizeImage
    wr_le32(out, 2835);                  // ~72 DPI
    wr_le32(out, 2835);
    if (paletted) {
        wr_le32(out, uint32_t(palette.size() / 4));   // biClrUsed
        wr_le32(out, uint32_t(palette.size() / 4));   // biClrImportant
    } else {
        wr_le32(out, 0); wr_le32(out, 0);
    }

    if (paletted)
        out.insert(out.end(), palette.begin(), palette.end());

    // Pixel data: bottom-up. Source `pixels` is top-down, bpr_raw bytes/row.
    uint8_t zeros[4] = {0, 0, 0, 0};
    if (pixels.size() != size_t(bpr_raw) * H)
        return err("BMP write: pixel buffer %zu bytes, expected %zu (%u rows x %u)",
                   pixels.size(), size_t(bpr_raw) * H, H, bpr_raw);
    for (int r = int(H) - 1; r >= 0; r--) {
        const uint8_t* row = pixels.data() + size_t(r) * bpr_raw;
        out.insert(out.end(), row, row + bpr_raw);
        if (pad) out.insert(out.end(), zeros, zeros + pad);
    }
    return true;
}

// Parse a BMP we (or something compatible) wrote. Returns the pixel array
// in top-down order, packed at bytes_per_row_raw bytes per row, in BMP byte
// order (BGR / BGRA / 8-bit index / packed 4-bit indices).
static bool read_bmp(const std::vector<uint8_t>& bmp,
                     uint32_t& W, uint32_t& H, uint32_t& bits,
                     std::vector<uint8_t>& palette,
                     std::vector<uint8_t>& pixels)
{
    if (bmp.size() < 54 || bmp[0] != 'B' || bmp[1] != 'M')
        return err("BMP read: bad header");
    uint32_t off    = rd_le32(bmp.data() + 10);
    uint32_t infosz = rd_le32(bmp.data() + 14);
    if (infosz < 40) return err("BMP read: info header too small (%u)", infosz);
    int32_t  ws = int32_t(rd_le32(bmp.data() + 18));
    int32_t  hs = int32_t(rd_le32(bmp.data() + 22));
    uint16_t bc = rd_le16(bmp.data() + 28);
    uint32_t cp = rd_le32(bmp.data() + 30);
    if (cp != 0) return err("BMP read: only BI_RGB supported (got %u)", cp);
    if (ws <= 0) return err("BMP read: non-positive width");
    bool top_down = (hs < 0);
    if (hs < 0) hs = -hs;
    if (hs <= 0) return err("BMP read: zero height");
    if (bc != 4 && bc != 8 && bc != 24 && bc != 32)
        return err("BMP read: unsupported bit count %u", bc);
    W    = uint32_t(ws);
    H    = uint32_t(hs);
    bits = bc;

    palette.clear();
    if (bc == 4 || bc == 8) {
        uint32_t nclr = rd_le32(bmp.data() + 46);
        if (nclr == 0) nclr = (bc == 4) ? 16 : 256;
        uint32_t pal_off = 14 + infosz;
        if (pal_off + nclr * 4 > bmp.size())
            return err("BMP read: palette truncated");
        palette.assign(bmp.data() + pal_off, bmp.data() + pal_off + nclr * 4);
    }

    uint32_t bpr_raw = bmp_bytes_per_row(W, bits);
    uint32_t row_b   = bmp_padded_row(W, bits);
    pixels.assign(size_t(bpr_raw) * H, 0);
    for (uint32_t r = 0; r < H; r++) {
        uint32_t src_row = top_down ? r : (H - 1 - r);
        size_t src = off + size_t(src_row) * row_b;
        if (src + bpr_raw > bmp.size())
            return err("BMP read: pixel data truncated at row %u", r);
        memcpy(pixels.data() + size_t(r) * bpr_raw, bmp.data() + src, bpr_raw);
    }
    return true;
}

/* ─── PNG raw ↔ BMP pixel layout conversion ──────────────────────────────── */

// PNG raw (top-down, PNG byte order per color type) → BMP-ordered pixels.
// Outputs the BMP mode used and (for palette/grayscale) the BMP palette.
static bool png_raw_to_bmp(const Ihdr& h, const std::vector<uint8_t>& plte,
                           const std::vector<uint8_t>& raw,
                           BmpMode& mode,
                           std::vector<uint8_t>& bmp_pixels,
                           std::vector<uint8_t>& bmp_palette)
{
    uint32_t W = h.width, H = h.height;
    size_t N = size_t(W) * H;
    bmp_palette.clear();

    switch (h.color_type) {
    case 0: {  // grayscale → 8-bit or 4-bit BMP, grayscale palette
        if (h.bit_depth == 4) {
            mode = BMP_4PAL;
            bmp_pixels = raw;         // identical nibble packing (high nib = leftmost)
            bmp_palette.resize(16 * 4);
            for (int i = 0; i < 16; i++) {
                uint8_t v = uint8_t(i * 17);   // 0,17,34,...,255
                bmp_palette[i*4 + 0] = v;
                bmp_palette[i*4 + 1] = v;
                bmp_palette[i*4 + 2] = v;
                bmp_palette[i*4 + 3] = 0;
            }
            return true;
        }
        // bit_depth == 8
        mode = BMP_8PAL;
        bmp_pixels = raw;             // 1 byte per pixel
        bmp_palette.resize(256 * 4);
        for (int i = 0; i < 256; i++) {
            bmp_palette[i*4 + 0] = uint8_t(i);
            bmp_palette[i*4 + 1] = uint8_t(i);
            bmp_palette[i*4 + 2] = uint8_t(i);
            bmp_palette[i*4 + 3] = 0;
        }
        return true;
    }
    case 2: {  // RGB → 24-bit BGR
        mode = BMP_24;
        bmp_pixels.resize(N * 3);
        for (size_t i = 0; i < N; i++) {
            bmp_pixels[i*3 + 0] = raw[i*3 + 2];  // B
            bmp_pixels[i*3 + 1] = raw[i*3 + 1];  // G
            bmp_pixels[i*3 + 2] = raw[i*3 + 0];  // R
        }
        return true;
    }
    case 3: {  // palette → 4-bit or 8-bit BMP, PLTE as palette
        if (plte.empty()) return err("palette PNG with no PLTE");
        mode = (h.bit_depth == 4) ? BMP_4PAL : BMP_8PAL;
        bmp_pixels = raw;             // packed indices, identical layout in BMP
        uint32_t n = uint32_t(plte.size() / 3);
        bmp_palette.resize(size_t(n) * 4);
        for (uint32_t i = 0; i < n; i++) {
            bmp_palette[i*4 + 0] = plte[i*3 + 2];  // B
            bmp_palette[i*4 + 1] = plte[i*3 + 1];  // G
            bmp_palette[i*4 + 2] = plte[i*3 + 0];  // R
            bmp_palette[i*4 + 3] = 0;
        }
        return true;
    }
    case 4: {  // gray + alpha → 32-bit BGRA  (B=G=R=g, A)
        mode = BMP_32;
        bmp_pixels.resize(N * 4);
        for (size_t i = 0; i < N; i++) {
            uint8_t g = raw[i*2 + 0];
            uint8_t a = raw[i*2 + 1];
            bmp_pixels[i*4 + 0] = g;
            bmp_pixels[i*4 + 1] = g;
            bmp_pixels[i*4 + 2] = g;
            bmp_pixels[i*4 + 3] = a;
        }
        return true;
    }
    case 6: {  // RGBA → 32-bit BGRA
        mode = BMP_32;
        bmp_pixels.resize(N * 4);
        for (size_t i = 0; i < N; i++) {
            bmp_pixels[i*4 + 0] = raw[i*4 + 2];  // B
            bmp_pixels[i*4 + 1] = raw[i*4 + 1];  // G
            bmp_pixels[i*4 + 2] = raw[i*4 + 0];  // R
            bmp_pixels[i*4 + 3] = raw[i*4 + 3];  // A
        }
        return true;
    }
    default:
        return err("unsupported PNG color type %u", h.color_type);
    }
}

// BMP-ordered pixels → PNG raw (top-down). Inverse of png_raw_to_bmp.
static bool bmp_to_png_raw(const Ihdr& h, uint32_t bmp_bits,
                           const std::vector<uint8_t>& bmp_pixels,
                           std::vector<uint8_t>& raw)
{
    uint32_t W = h.width, H = h.height;
    size_t N = size_t(W) * H;

    auto check_bits = [&](uint32_t want) {
        if (bmp_bits != want)
            return err("BMP %u-bit doesn't match PNG ct=%u bd=%u (expected %u-bit)",
                       bmp_bits, h.color_type, h.bit_depth, want);
        return true;
    };

    switch (h.color_type) {
    case 0:                                // gray
        if (h.bit_depth == 4) {
            if (!check_bits(4)) return false;
            raw = bmp_pixels;              // packed-nibble bytes identical
            return true;
        }
        if (!check_bits(8)) return false;
        raw = bmp_pixels;
        return true;
    case 2:                                // RGB
        if (!check_bits(24)) return false;
        raw.resize(N * 3);
        for (size_t i = 0; i < N; i++) {
            raw[i*3 + 0] = bmp_pixels[i*3 + 2];
            raw[i*3 + 1] = bmp_pixels[i*3 + 1];
            raw[i*3 + 2] = bmp_pixels[i*3 + 0];
        }
        return true;
    case 3:                                // palette
        if (!check_bits(h.bit_depth == 4 ? 4 : 8)) return false;
        raw = bmp_pixels;
        return true;
    case 4: {                              // gray+alpha
        if (!check_bits(32)) return false;
        raw.resize(N * 2);
        for (size_t i = 0; i < N; i++) {
            // B==G==R==g by construction; trust B (or pick any one channel).
            raw[i*2 + 0] = bmp_pixels[i*4 + 0];
            raw[i*2 + 1] = bmp_pixels[i*4 + 3];
        }
        return true;
    }
    case 6:                                // RGBA
        if (!check_bits(32)) return false;
        raw.resize(N * 4);
        for (size_t i = 0; i < N; i++) {
            raw[i*4 + 0] = bmp_pixels[i*4 + 2];
            raw[i*4 + 1] = bmp_pixels[i*4 + 1];
            raw[i*4 + 2] = bmp_pixels[i*4 + 0];
            raw[i*4 + 3] = bmp_pixels[i*4 + 3];
        }
        return true;
    default:
        return err("unsupported PNG color type %u", h.color_type);
    }
}

/* ─── dumpNN.unp filename ────────────────────────────────────────────────── */

static std::string unp_name(uint32_t i, uint32_t n_streams) {
    int w = 2;
    if (n_streams > 0) {
        uint32_t m = n_streams - 1;
        int d = 1;
        while (m >= 10) { m /= 10; d++; }
        if (d > w) w = d;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "dump%0*u.unp", w, i);
    return std::string(buf);
}

/* ─── validation helpers ─────────────────────────────────────────────────── */

static bool check_supported(const Ihdr& h) {
    uint8_t ct = h.color_type, bd = h.bit_depth;
    if (samples_per_pixel(ct) == 0)
        return err("unknown PNG color type %u", ct);
    if (h.width == 0 || h.height == 0)
        return err("zero-sized PNG (%ux%u)", h.width, h.height);
    // Supported (color_type, bit_depth):
    //   (0, 4)  grayscale 4-bit       → 4-bit BMP, grayscale palette
    //   (0, 8)  grayscale 8-bit       → 8-bit BMP, grayscale palette
    //   (2, 8)  RGB 8-bit             → 24-bit BMP
    //   (3, 4)  palette 4-bit         → 4-bit BMP, PLTE palette
    //   (3, 8)  palette 8-bit         → 8-bit BMP, PLTE palette
    //   (4, 8)  gray+alpha 8-bit      → 32-bit BMP
    //   (6, 8)  RGBA 8-bit            → 32-bit BMP
    bool ok = (ct == 0 && (bd == 4 || bd == 8)) ||
              (ct == 3 && (bd == 4 || bd == 8)) ||
              ((ct == 2 || ct == 4 || ct == 6) && bd == 8);
    if (!ok)
        return err("unsupported PNG combo color_type=%u bit_depth=%u", ct, bd);
    return true;
}

/* ─── c mode: compose ────────────────────────────────────────────────────── */

static bool compose(const std::string& meta_path, const std::string& bmp_path) {
    std::vector<uint8_t> meta1;
    if (!read_file(meta_path, meta1))
        return err("cannot read meta %s", meta_path.c_str());

    Meta info;
    if (!parse_meta(meta1, info)) return false;
    if (!check_supported(info.ihdr)) return false;

    fs::path dir = fs::path(meta_path).parent_path();
    if (dir.empty()) dir = ".";

    // Read all dumpNN.unp.
    std::vector<std::vector<uint8_t>> unps(info.n_streams);
    for (uint32_t i = 0; i < info.n_streams; i++) {
        fs::path p = dir / unp_name(i, info.n_streams);
        if (!read_file(p.string(), unps[i]))
            return err("cannot read %s", p.string().c_str());
    }

    // Defilter stream 0 (handles non-interlaced + Adam7).
    uint32_t W = info.ihdr.width;
    uint32_t H = info.ihdr.height;
    std::vector<uint8_t> raw, filters;
    if (!unp_to_raw(info.ihdr, unps[0], raw, filters))
        return false;

    // PNG raw → BMP-ordered pixels.
    BmpMode mode = BMP_24;
    std::vector<uint8_t> bmp_pixels, bmp_palette;
    if (!png_raw_to_bmp(info.ihdr, info.plte, raw, mode, bmp_pixels, bmp_palette))
        return false;

    // Write BMP.
    std::vector<uint8_t> bmp_bytes;
    if (!write_bmp(mode, W, H, bmp_pixels, bmp_palette, bmp_bytes))
        return false;
    if (!write_file(bmp_path, bmp_bytes))
        return err("cannot write %s", bmp_path.c_str());

    // Write meta2 = <bmp_path>.meta2
    std::string meta2_path = bmp_path + ".meta2";
    std::vector<uint8_t> meta2;
    meta2.reserve(meta1.size() + filters.size() + 32);
    meta2.insert(meta2.end(), P2BMETA2_MAGIC, P2BMETA2_MAGIC + 8);
    wr_le32(meta2, uint32_t(meta1.size()));
    meta2.insert(meta2.end(), meta1.begin(), meta1.end());
    wr_le32(meta2, uint32_t(filters.size()));
    meta2.insert(meta2.end(), filters.begin(), filters.end());
    wr_le32(meta2, info.n_streams - 1);     // extra streams
    for (uint32_t i = 1; i < info.n_streams; i++) {
        wr_le32(meta2, uint32_t(unps[i].size()));
        meta2.insert(meta2.end(), unps[i].begin(), unps[i].end());
    }
    if (!write_file(meta2_path, meta2))
        return err("cannot write %s", meta2_path.c_str());

    const char* mode_s = (mode == BMP_24)   ? "24-bit BGR"
                       : (mode == BMP_32)   ? "32-bit BGRA"
                       : (mode == BMP_4PAL) ? "4-bit palette"
                                            : "8-bit palette";
    fprintf(stdout,
            "composed: %ux%u %s%s, bmp=%zu, meta2=%zu (filters=%zu, %u extra stream%s)\n",
            W, H, mode_s,
            info.ihdr.interlace == 1 ? " Adam7" : "",
            bmp_bytes.size(), meta2.size(), filters.size(),
            info.n_streams - 1, info.n_streams - 1 == 1 ? "" : "s");
    return true;
}

/* ─── d mode: decompose ──────────────────────────────────────────────────── */

static bool decompose(const std::string& bmp_path, const std::string& meta_path) {
    std::vector<uint8_t> bmp_bytes;
    if (!read_file(bmp_path, bmp_bytes))
        return err("cannot read %s", bmp_path.c_str());
    std::string meta2_path = bmp_path + ".meta2";
    std::vector<uint8_t> meta2;
    if (!read_file(meta2_path, meta2))
        return err("cannot read %s", meta2_path.c_str());

    if (meta2.size() < 16 || memcmp(meta2.data(), P2BMETA2_MAGIC, 8) != 0)
        return err("bad meta2 magic in %s", meta2_path.c_str());
    size_t p = 8;
    uint32_t meta1_len = rd_le32(meta2.data() + p); p += 4;
    if (p + meta1_len > meta2.size()) return err("meta2: meta1 truncated");
    std::vector<uint8_t> meta1(meta2.begin() + p, meta2.begin() + p + meta1_len);
    p += meta1_len;
    if (p + 4 > meta2.size()) return err("meta2: filter length missing");
    uint32_t filt_len = rd_le32(meta2.data() + p); p += 4;
    if (p + filt_len > meta2.size()) return err("meta2: filters truncated");
    std::vector<uint8_t> filters(meta2.begin() + p, meta2.begin() + p + filt_len);
    p += filt_len;
    if (p + 4 > meta2.size()) return err("meta2: extras count missing");
    uint32_t n_extra = rd_le32(meta2.data() + p); p += 4;
    std::vector<std::vector<uint8_t>> extras(n_extra);
    for (uint32_t i = 0; i < n_extra; i++) {
        if (p + 4 > meta2.size()) return err("meta2: extra %u header missing", i);
        uint32_t L = rd_le32(meta2.data() + p); p += 4;
        if (p + L > meta2.size()) return err("meta2: extra %u truncated", i);
        extras[i].assign(meta2.begin() + p, meta2.begin() + p + L);
        p += L;
    }
    if (p != meta2.size())
        fprintf(stderr, "png2bmp: warning: %zu trailing bytes in meta2 ignored\n",
                meta2.size() - p);

    Meta info;
    if (!parse_meta(meta1, info)) return false;
    if (!check_supported(info.ihdr)) return false;
    if (info.n_streams != n_extra + 1)
        return err("meta1 says %u streams but meta2 has 1+%u extras",
                   info.n_streams, n_extra);

    // Read BMP.
    uint32_t bw = 0, bh = 0, bbits = 0;
    std::vector<uint8_t> bmp_pal, bmp_pixels;
    if (!read_bmp(bmp_bytes, bw, bh, bbits, bmp_pal, bmp_pixels)) return false;
    if (bw != info.ihdr.width || bh != info.ihdr.height)
        return err("BMP is %ux%u, meta1 IHDR is %ux%u",
                   bw, bh, info.ihdr.width, info.ihdr.height);

    // BMP pixels → PNG raw.
    std::vector<uint8_t> raw;
    if (!bmp_to_png_raw(info.ihdr, bbits, bmp_pixels, raw)) return false;

    // Refilter → dump00.unp (handles non-interlaced + Adam7).
    std::vector<uint8_t> unp0;
    if (!raw_to_unp(info.ihdr, raw, filters, unp0)) return false;

    // Write outputs.
    fs::path mp = meta_path;
    fs::path dir = mp.parent_path();
    if (dir.empty()) dir = ".";
    std::error_code ec;
    fs::create_directories(dir, ec);

    if (!write_file(meta_path, meta1))
        return err("cannot write %s", meta_path.c_str());

    fs::path unp0_path = dir / unp_name(0, info.n_streams);
    if (!write_file(unp0_path.string(), unp0))
        return err("cannot write %s", unp0_path.string().c_str());

    for (uint32_t i = 0; i < n_extra; i++) {
        fs::path up = dir / unp_name(i + 1, info.n_streams);
        if (!write_file(up.string(), extras[i]))
            return err("cannot write %s", up.string().c_str());
    }

    fprintf(stdout,
            "decomposed: meta1=%zu, dump00.unp=%zu, %u extra stream%s\n",
            meta1.size(), unp0.size(), n_extra, n_extra == 1 ? "" : "s");
    return true;
}

/* ─── main ───────────────────────────────────────────────────────────────── */

static void usage() {
    fprintf(stderr,
        "png2bmp - PNG dump ↔ plain BMP companion to pngdump\n"
        "\n"
        "  png2bmp c <meta-path> <output.bmp>\n"
        "      compose: defilter dumpNN.unp → BMP + <output.bmp>.meta2\n"
        "\n"
        "  png2bmp d <input.bmp> <meta-path>\n"
        "      decompose: BMP + <input.bmp>.meta2 → meta + dumpNN.unp\n"
        "\n"
        "dumpNN.unp files = inflated dumpNN.bin (raw deflate output: filter\n"
        "byte per row + scanline pixels). Get them with any inflate tool.\n"
        "Supported PNGs: grayscale 4/8-bit, palette 4/8-bit, RGB / gray+alpha / RGBA 8-bit.\n");
}

int main(int argc, char** argv) {
    if (argc != 4) { usage(); return 1; }
    std::string mode = argv[1];
    if (mode == "c")  return compose(argv[2],   argv[3]) ? 0 : 1;
    if (mode == "d")  return decompose(argv[2], argv[3]) ? 0 : 1;
    usage();
    return 1;
}
