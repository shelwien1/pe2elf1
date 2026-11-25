// pdf_bmp.inc -- raw images as BMP3 (BITMAPFILEHEADER + BITMAPINFOHEADER, BI_RGB, bottom-up).
//
// Restore reads width, height, bit count and bfOffBits from the BMP itself and ignores the
// palette and every other header field, so the palette is the encoder's free choice.

static inline void le16(Bytes& b, uint32_t v) { b.push_back((uint8_t)v); b.push_back((uint8_t)(v >> 8)); }
static inline void le32(Bytes& b, uint32_t v) { for (int i = 0; i < 4; i++) b.push_back((uint8_t)(v >> (8 * i))); }
static inline uint32_t rd32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
static inline uint32_t rd16(const uint8_t* p) { return p[0] | (p[1] << 8); }

struct BmpSpec {
  uint32_t width = 0, height = 0;
  int bpp = 0;                    // 1, 4, 8, 24, 32
  std::vector<uint32_t> palette;  // 0x00RRGGBB, 2^bpp entries for bpp <= 8
};

static size_t bmpPdfRow(uint32_t w, int bpp) { return ((size_t)w * bpp + 7) / 8; }

// raw PDF samples -> BMP file + recon (the tail after the expected sample bytes)
static bool bmpWrap(const BmpSpec& sp, const Bytes& raw, Bytes& bmp, Bytes& recon) {
  if (!sp.width || !sp.height || sp.width > 0x7FFFFFFF || sp.height > 0x7FFFFFFF) return false;
  size_t prow = bmpPdfRow(sp.width, sp.bpp);
  size_t brow = (prow + 3) & ~(size_t)3;
  if (sp.height > raw.size() / prow) return false;
  size_t need = prow * sp.height;
  // a long tail means the image parameters do not describe the data: leave it raw
  if (raw.size() - need > 256) return false;
  size_t npal = sp.bpp <= 8 ? ((size_t)1 << sp.bpp) : 0;
  uint64_t off = 14 + 40 + 4 * npal;
  uint64_t total = off + (uint64_t)brow * sp.height;
  if (total >= 0xFFFFFFFFull) return false;
  bmp.clear();
  bmp.reserve((size_t)total);
  bmp.push_back('B');
  bmp.push_back('M');
  le32(bmp, (uint32_t)total);
  le32(bmp, 0);
  le32(bmp, (uint32_t)off);
  le32(bmp, 40);
  le32(bmp, sp.width);
  le32(bmp, sp.height);
  le16(bmp, 1);
  le16(bmp, sp.bpp);
  le32(bmp, 0);
  le32(bmp, (uint32_t)(brow * sp.height));
  le32(bmp, 2835);
  le32(bmp, 2835);
  le32(bmp, (uint32_t)npal);
  le32(bmp, 0);
  for (size_t i = 0; i < npal; i++) le32(bmp, i < sp.palette.size() ? sp.palette[i] : 0);
  for (size_t y = 0; y < sp.height; y++) {
    const uint8_t* src = raw.data() + (sp.height - 1 - y) * prow;
    size_t st = bmp.size();
    bmp.insert(bmp.end(), src, src + prow);
    if (sp.bpp == 24)
      for (size_t x = 0; x + 2 < prow; x += 3) std::swap(bmp[st + x], bmp[st + x + 2]);
    bmp.insert(bmp.end(), brow - prow, 0);
  }
  recon.clear();
  putLenBytesTo(recon, raw.data() + need, raw.size() - need);
  return true;
}

// BMP file + recon -> raw PDF samples (layerEncode for L_BMP)
static bool bmpEncodeLayer(const Bytes& recon, const Bytes& in, Bytes& out) {
  if (in.size() < 54 || in[0] != 'B' || in[1] != 'M') return false;
  uint32_t off = rd32(&in[10]);
  uint32_t w = rd32(&in[18]);
  int32_t h = (int32_t)rd32(&in[22]);
  int bpp = (int)rd16(&in[28]);
  if (h <= 0 || w == 0 || w > 0x7FFFFFFF || (bpp != 1 && bpp != 4 && bpp != 8 && bpp != 24 && bpp != 32)) return false;
  size_t prow = bmpPdfRow(w, bpp);
  size_t brow = (prow + 3) & ~(size_t)3;
  if (off > in.size() || (size_t)h > (in.size() - off) / brow) return false;
  out.clear();
  out.reserve(prow * h);
  for (int32_t y = h - 1; y >= 0; y--) {
    const uint8_t* src = in.data() + off + (size_t)y * brow;
    size_t st = out.size();
    out.insert(out.end(), src, src + prow);
    if (bpp == 24)
      for (size_t x = 0; x + 2 < prow; x += 3) std::swap(out[st + x], out[st + x + 2]);
  }
  RBuf r(recon);
  r.appendTo(r.len(), out);
  return r.ok();
}

// ---- RGB image + 8-bit /SMask as one 32 bpp BMP (plan M10, opt-in -a) -------------------------
// The image's stored form becomes a BGRA BMP (layer L_RGBA, recon as for L_BMP); the mask keeps no
// data of its own (layer L_ALPHA: var index of the image's stream, then the L_BMP recon) and is
// restored from the alpha bytes once the image's stored form is known (the mask is deferred).

// 24 bpp image BMP + 8 bpp mask BMP of the same size -> 32 bpp BGRA BMP; false if they don't match
static bool rgbaMerge(const Bytes& rgb, const Bytes& a, Bytes& out) {
  if (rgb.size() < 54 || a.size() < 54 || rd16(&rgb[28]) != 24 || rd16(&a[28]) != 8) return false;
  uint32_t w = rd32(&rgb[18]), h = rd32(&rgb[22]);
  if (rd32(&a[18]) != w || rd32(&a[22]) != h || !w || !h || (int32_t)h < 0) return false;
  size_t so = rd32(&rgb[10]), ao = rd32(&a[10]);
  size_t srow = ((size_t)w * 3 + 3) & ~(size_t)3, arow = ((size_t)w + 3) & ~(size_t)3;
  if (so > rgb.size() || ao > a.size() || (rgb.size() - so) / srow < h || (a.size() - ao) / arow < h) return false;
  uint64_t total = 54 + 4 * (uint64_t)w * h;
  if (total >= 0xFFFFFFFFull) return false;
  out.clear();
  out.reserve((size_t)total);
  out.push_back('B');
  out.push_back('M');
  le32(out, (uint32_t)total);
  le32(out, 0);
  le32(out, 54);
  le32(out, 40);
  le32(out, w);
  le32(out, h);
  le16(out, 1);
  le16(out, 32);
  le32(out, 0);
  le32(out, (uint32_t)(total - 54));
  le32(out, 2835);
  le32(out, 2835);
  le32(out, 0);
  le32(out, 0);
  for (size_t y = 0; y < h; y++) {
    const uint8_t* s = rgb.data() + so + y * srow;
    const uint8_t* m = a.data() + ao + y * arow;
    for (size_t x = 0; x < w; x++) {
      out.push_back(s[3 * x]);
      out.push_back(s[3 * x + 1]);
      out.push_back(s[3 * x + 2]);
      out.push_back(m[x]);
    }
  }
  return true;
}

static bool bmp32Geometry(const Bytes& in, uint32_t& w, uint32_t& h, size_t& off) {
  if (in.size() < 54 || in[0] != 'B' || in[1] != 'M' || rd16(&in[28]) != 32) return false;
  w = rd32(&in[18]);
  int32_t sh = (int32_t)rd32(&in[22]);
  off = rd32(&in[10]);
  if (sh <= 0 || w == 0 || w > 0x3FFFFFFF || off > in.size()) return false;
  h = (uint32_t)sh;
  return (in.size() - off) / 4 / w >= h;
}

// BGRA BMP -> the image's raw RGB samples + tail (layerEncode for L_RGBA)
static bool rgbaEncodeLayer(const Bytes& recon, const Bytes& in, Bytes& out) {
  uint32_t w, h;
  size_t off;
  if (!bmp32Geometry(in, w, h, off)) return false;
  out.clear();
  out.reserve((size_t)w * h * 3);
  for (int64_t y = (int64_t)h - 1; y >= 0; y--) {
    const uint8_t* s = in.data() + off + (size_t)y * w * 4;
    for (size_t x = 0; x < w; x++) {
      out.push_back(s[4 * x + 2]);
      out.push_back(s[4 * x + 1]);
      out.push_back(s[4 * x]);
    }
  }
  RBuf r(recon);
  r.appendTo(r.len(), out);
  return r.ok();
}

// alpha bytes of the image's BGRA BMP -> the mask's raw samples + tail (layerEncode for L_ALPHA)
static bool alphaEncodeLayer(const CodecCtx& ctx, const Bytes& recon, Bytes& out) {
  RBuf r(recon);
  uint64_t k = r.var();
  const Bytes* src = ctx.alphaSource ? ctx.alphaSource(k) : nullptr;
  uint32_t w, h;
  size_t off;
  if (!src || !bmp32Geometry(*src, w, h, off)) return false;
  out.clear();
  out.reserve((size_t)w * h);
  for (int64_t y = (int64_t)h - 1; y >= 0; y--) {
    const uint8_t* s = src->data() + off + (size_t)y * w * 4;
    for (size_t x = 0; x < w; x++) out.push_back(s[4 * x + 3]);
  }
  r.appendTo(r.len(), out);
  return r.ok();
}
