// pdf_tiff.inc -- streams that match a TIFF subformat, stored as a baseline TIFF file.
//
// The stream bytes become the single strip of a big-endian TIFF; nothing is re-coded, so the
// layer is exact by construction and needs no recon: restore reads StripOffsets /
// StripByteCounts back from the IFD. Subformats:
//   CCITT fax data   compression 4 (G4: K < 0, not byte-aligned), 3 (G3 with EOLs, 1D or 2D,
//                    T4Options fill bits when byte-aligned), 2 (1D, byte-aligned, no EOLs)
//   left encoded     8 (Flate), 5 (LZW, EarlyChange 1), 32773 (RunLength = PackBits), with
//                    Predictor 1 or 2 matching the image
//   raw samples      1 (images BMP cannot hold: 2 or 16 bits, CMYK, 2-bit palette, ...)
// Layout: header, IFD, out-of-line values, strip.

static inline uint32_t be32at(const uint8_t* p) { return (uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]; }
static inline uint32_t be16at(const uint8_t* p) { return (uint32_t)(p[0] << 8 | p[1]); }

struct TiffSpec {
  uint32_t width = 0, height = 0;
  int bps = 1, spp = 1;
  int photometric = 1;   // 0 WhiteIsZero, 1 BlackIsZero, 2 RGB, 3 palette, 5 separated (CMYK)
  int compression = 1;   // 1, 2, 3, 4, 5, 8, 32773
  int predictor = 1;     // 1 or 2
  uint32_t t4 = 0;       // T4Options (compression 3)
  std::vector<uint32_t> palette;  // 0x00RRGGBB, 2^bps entries (photometric 3)
};

static bool tiffWrap(const TiffSpec& sp, const Bytes& strip, Bytes& out) {
  if (!sp.width || !sp.height || sp.spp < 1 || sp.spp > 8 || strip.size() >= 0xFFFFFF00u) return false;
  if (sp.photometric == 3 && sp.palette.size() != ((size_t)1 << sp.bps)) return false;
  struct E { uint16_t tag, type; uint32_t count; Bytes v; };
  std::vector<E> es;
  auto shorts = [&](uint16_t tag, const std::vector<uint32_t>& vs) {
    E e{tag, 3, (uint32_t)vs.size(), {}};
    for (uint32_t x : vs) { e.v.push_back((uint8_t)(x >> 8)); e.v.push_back((uint8_t)x); }
    es.push_back(std::move(e));
  };
  auto longv = [&](uint16_t tag, uint32_t x) {
    E e{tag, 4, 1, {}};
    for (int k = 3; k >= 0; k--) e.v.push_back((uint8_t)(x >> (8 * k)));
    es.push_back(std::move(e));
  };
  auto rational = [&](uint16_t tag, uint32_t a, uint32_t b) {
    E e{tag, 5, 1, {}};
    for (uint32_t x : {a, b})
      for (int k = 3; k >= 0; k--) e.v.push_back((uint8_t)(x >> (8 * k)));
    es.push_back(std::move(e));
  };
  longv(256, sp.width);
  longv(257, sp.height);
  shorts(258, std::vector<uint32_t>((size_t)sp.spp, (uint32_t)sp.bps));
  shorts(259, {(uint32_t)sp.compression});
  shorts(262, {(uint32_t)sp.photometric});
  longv(273, 0);  // StripOffsets, set below
  shorts(277, {(uint32_t)sp.spp});
  longv(278, sp.height);
  longv(279, (uint32_t)strip.size());
  rational(282, 72, 1);
  rational(283, 72, 1);
  shorts(284, {1});
  if (sp.compression == 3) longv(292, sp.t4);
  if (sp.compression == 4) longv(293, 0);
  shorts(296, {2});
  if (sp.predictor == 2) shorts(317, {2});
  if (sp.photometric == 3) {
    std::vector<uint32_t> cm;
    for (int ch = 2; ch >= 0; ch--)
      for (uint32_t c : sp.palette) cm.push_back(((c >> (8 * ch)) & 0xFF) * 257);
    shorts(320, cm);
  }
  if (sp.photometric == 5) shorts(332, {1});
  size_t ifd = 8, extra = ifd + 2 + 12 * es.size() + 4, x = extra;
  for (auto& e : es)
    if (e.v.size() > 4) x += (e.v.size() + 1) & ~(size_t)1;
  for (auto& e : es)
    if (e.tag == 273)
      for (int k = 0; k < 4; k++) e.v[k] = (uint8_t)(x >> (8 * (3 - k)));
  out.clear();
  out.reserve(x + strip.size());
  const uint8_t hdr[8] = {'M', 'M', 0, 42, 0, 0, 0, 8};
  out.insert(out.end(), hdr, hdr + 8);
  out.push_back((uint8_t)(es.size() >> 8));
  out.push_back((uint8_t)es.size());
  Bytes ext;
  for (auto& e : es) {
    out.push_back((uint8_t)(e.tag >> 8)); out.push_back((uint8_t)e.tag);
    out.push_back((uint8_t)(e.type >> 8)); out.push_back((uint8_t)e.type);
    for (int k = 3; k >= 0; k--) out.push_back((uint8_t)(e.count >> (8 * k)));
    if (e.v.size() <= 4) {
      out.insert(out.end(), e.v.begin(), e.v.end());
      out.insert(out.end(), 4 - e.v.size(), 0);
    } else {
      size_t off = extra + ext.size();
      for (int k = 3; k >= 0; k--) out.push_back((uint8_t)(off >> (8 * k)));
      ext.insert(ext.end(), e.v.begin(), e.v.end());
      if (ext.size() & 1) ext.push_back(0);
    }
  }
  out.insert(out.end(), 4, 0);  // no next IFD
  out.insert(out.end(), ext.begin(), ext.end());
  out.insert(out.end(), strip.begin(), strip.end());
  return out.size() == x + strip.size();
}

// one inline SHORT/LONG value of tag in the first IFD; false if absent or not of that shape
static bool tiffTag(const Bytes& in, uint16_t tag, uint32_t& v) {
  if (in.size() < 8 || in[0] != 'M' || in[1] != 'M' || be16at(&in[2]) != 42) return false;
  uint32_t ifd = be32at(&in[4]);
  if (ifd > in.size() - 2) return false;
  uint32_t n = be16at(&in[ifd]);
  if (n > (in.size() - ifd - 2) / 12) return false;
  for (uint32_t k = 0; k < n; k++) {
    const uint8_t* e = &in[ifd + 2 + 12 * k];
    if (be16at(e) != tag) continue;
    uint32_t type = be16at(e + 2), cnt = be32at(e + 4);
    if (cnt != 1) return false;
    if (type == 3) { v = be16at(e + 8); return true; }
    if (type == 4) { v = be32at(e + 8); return true; }
    return false;
  }
  return false;
}

// TIFF file -> stream bytes (layerEncode for L_TIFW)
static bool tifwEncodeLayer(const Bytes& recon, const Bytes& in, Bytes& out) {
  uint32_t off, cnt;
  if (!recon.empty() || !tiffTag(in, 273, off) || !tiffTag(in, 279, cnt)) return false;
  if (off > in.size() || cnt > in.size() - off) return false;
  out.assign(in.begin() + off, in.begin() + off + cnt);
  return true;
}
