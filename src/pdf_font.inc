// pdf_font.inc -- glyph advances of embedded fonts, for derived /Widths and /W (plan2 P8).
//
// Only what the width prediction needs: TrueType head/maxp/hhea/hmtx and the (3,0) (1,0) (3,1)
// cmap subtables in formats 0, 4 and 6; CFF (FontFile3 /Type1C and CID-keyed /CIDFontType0C):
// charset, built-in encoding, FDArray/FDSelect, the width of every Type 2 charstring; Type 1
// (FontFile, from the .t1d stored form and its L_T1 recon): built-in encoding and the hsbw/sbw
// width of every charstring, a "div" in front of it included. Every read is bounds-checked: the
// decoder runs this on files of the dump.

static const int32_t kNoAdv = INT32_MIN;  // advance not known (the glyph gets no prediction)

// Everything at its maximum size: glyph ids, CIDs and cmap subtable lengths are 16-bit, a
// CIDToGIDMap covers 65536 CIDs. About 1 MB: allocate on the heap (the caches hold a few).
static const size_t kMaxGlyphs = 65536;
static const uint32_t kNoGlyph = 0xFFFFFFFFu;
struct FontMetrics {
  bool ok = false;
  bool cff = false;
  uint32_t upem = 1000;
  uint32_t nglyphs = 0;
  int32_t adv[kMaxGlyphs];    // per glyph id
  uint16_t den[kMaxGlyphs];   // per glyph divisor of adv (Type 1 "div"), with hasDen
  bool hasDen = false;
  uint8_t cmap[3][65536];     // TrueType: raw (3,0), (1,0), (3,1) subtables
  uint32_t cmapLen[3] = {};
  int32_t enc[256];           // CFF, Type 1: code -> glyph id, -1 none
  int32_t stdEnc[256];        // Type 1: StandardEncoding code -> glyph id
  bool cidKeyed = false;      // CID-keyed CFF: cidGid holds CID -> glyph id (kNoGlyph none)
  uint32_t cidGid[65536];
  uint8_t cidMap[131072];     // CIDFontType2 with a CIDToGIDMap stream: its first 65536 entries
  uint32_t cidMapLen = 0;
  void reset() {
    ok = cff = hasDen = cidKeyed = false;
    upem = 1000;
    nglyphs = 0;
    for (auto& l : cmapLen) l = 0;
    cidMapLen = 0;
    for (auto& e : enc) e = -1;
    for (auto& e : stdEnc) e = -1;
  }
  void setCidMap(const uint8_t* p, size_t n) {
    cidMapLen = (uint32_t)std::min(n, sizeof(cidMap));
    memcpy(cidMap, p, cidMapLen);
  }
};

static inline uint32_t fntU16(const uint8_t* p, size_t n, size_t o, bool& ok) {
  if (o > n || n - o < 2) { ok = false; return 0; }
  return (uint32_t)p[o] << 8 | p[o + 1];
}
static inline uint32_t fntU32(const uint8_t* p, size_t n, size_t o, bool& ok) {
  if (o > n || n - o < 4) { ok = false; return 0; }
  return (uint32_t)p[o] << 24 | (uint32_t)p[o + 1] << 16 | (uint32_t)p[o + 2] << 8 | p[o + 3];
}

// glyph id for code c in a raw cmap subtable; 0 = none
static uint32_t ttCmapLookup(const uint8_t* p, size_t n, uint32_t c) {
  bool ok = true;
  uint32_t fmt = fntU16(p, n, 0, ok);
  if (!ok) return 0;
  if (fmt == 0) return c < 256 && 6 + c < n ? p[6 + c] : 0;
  if (fmt == 6) {
    uint32_t first = fntU16(p, n, 6, ok), cnt = fntU16(p, n, 8, ok);
    if (!ok || c < first || c - first >= cnt) return 0;
    uint32_t g = fntU16(p, n, 10 + 2 * (size_t)(c - first), ok);
    return ok ? g : 0;
  }
  if (fmt == 4) {
    uint32_t segX2 = fntU16(p, n, 6, ok);
    if (!ok || segX2 < 2) return 0;
    size_t seg = segX2 / 2, endO = 14, startO = 16 + segX2, deltaO = 16 + 2 * (size_t)segX2,
           rangeO = 16 + 3 * (size_t)segX2;
    for (size_t s = 0; s < seg; s++) {
      uint32_t end = fntU16(p, n, endO + 2 * s, ok);
      if (!ok) return 0;
      if (c > end) continue;
      uint32_t start = fntU16(p, n, startO + 2 * s, ok), delta = fntU16(p, n, deltaO + 2 * s, ok),
               ro = fntU16(p, n, rangeO + 2 * s, ok);
      if (!ok || c < start) return 0;
      if (ro == 0) return (c + delta) & 0xFFFF;
      uint32_t g = fntU16(p, n, rangeO + 2 * s + ro + 2 * (size_t)(c - start), ok);
      return ok && g ? (g + delta) & 0xFFFF : 0;
    }
  }
  return 0;
}

static bool parseTrueType(const uint8_t* p, size_t n, FontMetrics& m) {
  m.reset();
  bool ok = true;
  uint32_t ver = fntU32(p, n, 0, ok), nt = fntU16(p, n, 4, ok);
  if (!ok || (ver != 0x00010000 && ver != 0x74727565) || nt == 0 || nt > 512) return false;
  size_t head = 0, maxp = 0, hhea = 0, hmtx = 0, cmap = 0, hmtxLen = 0, cmapLen = 0;
  for (uint32_t k = 0; k < nt; k++) {
    size_t r = 12 + 16 * (size_t)k;
    uint32_t tag = fntU32(p, n, r, ok), off = fntU32(p, n, r + 8, ok), len = fntU32(p, n, r + 12, ok);
    if (!ok) return false;
    if (off > n || len > n - off) continue;
    if (tag == 0x68656164) head = off;
    else if (tag == 0x6D617870) maxp = off;
    else if (tag == 0x68686561) hhea = off;
    else if (tag == 0x686D7478) { hmtx = off; hmtxLen = len; }
    else if (tag == 0x636D6170) { cmap = off; cmapLen = len; }
  }
  if (!head || !maxp || !hhea || !hmtx) return false;
  m.upem = fntU16(p, n, head + 18, ok);
  uint32_t ng = fntU16(p, n, maxp + 4, ok), nh = fntU16(p, n, hhea + 34, ok);
  if (!ok || m.upem < 16 || m.upem > 16384 || ng == 0 || nh == 0) return false;
  nh = std::min(nh, ng);
  if (hmtxLen < 4 * (size_t)nh) return false;
  m.nglyphs = ng;  // a u16: at most kMaxGlyphs - 1
  for (uint32_t g = 0; g < ng; g++) m.adv[g] = (int32_t)fntU16(p, n, hmtx + 4 * (size_t)std::min(g, nh - 1), ok);
  if (!ok) return false;
  if (cmap) {
    const uint8_t* c = p + cmap;
    size_t cn = cmapLen;
    uint32_t cnt = fntU16(c, cn, 2, ok);
    for (uint32_t k = 0; ok && k < cnt && k < 256; k++) {
      uint32_t pid = fntU16(c, cn, 4 + 8 * (size_t)k, ok), eid = fntU16(c, cn, 6 + 8 * (size_t)k, ok),
               off = fntU32(c, cn, 8 + 8 * (size_t)k, ok);
      int slot = pid == 3 && eid == 0 ? 0 : pid == 1 && eid == 0 ? 1 : pid == 3 && eid == 1 ? 2 : -1;
      if (!ok || slot < 0 || m.cmapLen[slot] || off >= cn) continue;
      bool ok2 = true;
      uint32_t fmt = fntU16(c, cn, off, ok2);
      size_t len = fmt == 0 ? 262 : fntU16(c, cn, off + 2, ok2);
      if (!ok2 || (fmt != 0 && fmt != 4 && fmt != 6)) continue;
      len = std::min(len, cn - off);
      len = std::min(len, sizeof(m.cmap[slot]));  // format 0: 262 bytes; 4 and 6: a u16 length
      memcpy(m.cmap[slot], c + off, len);
      m.cmapLen[slot] = (uint32_t)len;
    }
    ok = true;
  }
  m.ok = true;
  return true;
}

// ---- CFF ------------------------------------------------------------------------------------------
struct CffIndex {
  size_t count = 0, offSize = 0, offs = 0, data = 0, end = 0;
  bool read(const uint8_t* p, size_t n, size_t at) {
    bool ok = true;
    count = fntU16(p, n, at, ok);
    if (!ok) return false;
    if (count == 0) { end = at + 2; return true; }
    if (at + 3 > n) return false;
    offSize = p[at + 2];
    if (offSize < 1 || offSize > 4) return false;
    offs = at + 3;
    if ((count + 1) > (n - offs) / offSize) return false;
    data = offs + (count + 1) * offSize - 1;
    size_t last;
    if (!off(p, count, last) || last > n - data) return false;
    end = data + last;
    return true;
  }
  bool off(const uint8_t* p, size_t i, size_t& v) const {
    v = 0;
    for (size_t k = 0; k < offSize; k++) v = v << 8 | p[offs + i * offSize + k];
    return v >= 1;
  }
  bool item(const uint8_t* p, size_t i, size_t& a, size_t& b) const {
    size_t x, y;
    if (i >= count || !off(p, i, x) || !off(p, i + 1, y) || y < x || data + y > end) return false;
    a = data + x;
    b = data + y;
    return true;
  }
};

// DICT operands as doubles (reals parsed from their nibbles); calls op(code, operands) per operator.
// The operand stack holds at most 48 (Type 2 limit; a DICT needs far fewer).
typedef FixedVec<double, 48> CffStack;
template <class F>
static bool cffDict(const uint8_t* p, size_t a, size_t b, F op) {
  CffStack st;
  size_t i = a;
  while (i < b) {
    uint8_t c = p[i];
    if (c <= 21) {
      int code = c;
      if (c == 12) {
        if (i + 1 >= b) return false;
        code = 1200 + p[i + 1];
        i++;
      }
      i++;
      if (!op(code, st)) return false;
      st.clear();
      continue;
    }
    if (st.size() >= 48) return false;
    if (c >= 32 && c <= 246) { st.push_back(c - 139); i++; }
    else if (c >= 247 && c <= 250) { if (i + 1 >= b) return false; st.push_back((c - 247) * 256 + p[i + 1] + 108); i += 2; }
    else if (c >= 251 && c <= 254) { if (i + 1 >= b) return false; st.push_back(-(c - 251) * 256 - p[i + 1] - 108); i += 2; }
    else if (c == 28) { if (i + 2 >= b) return false; st.push_back((int16_t)(p[i + 1] << 8 | p[i + 2])); i += 3; }
    else if (c == 29) {
      if (i + 4 >= b) return false;
      st.push_back((int32_t)((uint32_t)p[i + 1] << 24 | (uint32_t)p[i + 2] << 16 | (uint32_t)p[i + 3] << 8 | p[i + 4]));
      i += 5;
    } else if (c == 30) {
      FixedStr<71> s;  // stops past 63 characters; one nibble adds at most 2
      i++;
      bool end = false;
      while (i < b && !end && s.size() < 64) {
        for (int h = 0; h < 2 && !end; h++) {
          int nib = h ? p[i] & 15 : p[i] >> 4;
          char d = (char)('0' + nib);
          if (nib <= 9) s.append(&d, 1);
          else if (nib == 10) s.append(".", 1);
          else if (nib == 11) s.append("E", 1);
          else if (nib == 12) s.append("E-", 2);
          else if (nib == 14) s.append("-", 1);
          else if (nib == 15) end = true;
          else return false;
        }
        i++;
      }
      if (!end) return false;
      char* e = nullptr;
      double v = strtod(s.c_str(), &e);
      if (s.empty() || *e) return false;
      st.push_back(v);
    } else {
      return false;
    }
  }
  return true;
}

// width of a Type 2 charstring: the first stack-clearing operator tells whether a width operand
// leads; a subroutine call or anything unusual before it gives kNoAdv
static int32_t t2Width(const uint8_t* c, size_t n, int32_t dflt, int32_t nominal) {
  int cnt = 0;
  int32_t first = 0;
  bool firstInt = true;
  size_t i = 0;
  while (i < n) {
    uint8_t b = c[i];
    if (b >= 32 || b == 28) {
      int32_t v;
      bool isInt = true;
      if (b == 28) { if (i + 3 > n) return kNoAdv; v = (int16_t)(c[i + 1] << 8 | c[i + 2]); i += 3; }
      else if (b <= 246) { v = b - 139; i++; }
      else if (b <= 250) { if (i + 2 > n) return kNoAdv; v = (b - 247) * 256 + c[i + 1] + 108; i += 2; }
      else if (b <= 254) { if (i + 2 > n) return kNoAdv; v = -(b - 251) * 256 - c[i + 1] - 108; i += 2; }
      else {
        if (i + 5 > n) return kNoAdv;
        uint32_t f = (uint32_t)c[i + 1] << 24 | (uint32_t)c[i + 2] << 16 | (uint32_t)c[i + 3] << 8 | c[i + 4];
        isInt = (f & 0xFFFF) == 0;
        v = (int32_t)f >> 16;
        i += 5;
      }
      if (cnt == 0) { first = v; firstInt = isInt; }
      if (++cnt > 48) return kNoAdv;
      continue;
    }
    bool hasW;
    switch (b) {
      case 1: case 3: case 18: case 23: case 19: case 20: hasW = (cnt & 1) != 0; break;
      case 21: hasW = cnt > 2; break;
      case 4: case 22: hasW = cnt > 1; break;
      case 14: hasW = cnt == 1 || cnt == 5; break;
      default: return kNoAdv;
    }
    if (!hasW) return dflt;
    if (!firstInt) return kNoAdv;
    return nominal + first;
  }
  return kNoAdv;
}

static uint16_t cffStdSid(int code) {
  static const uint8_t hi[95] = {
      96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 0,   111, 112, 113, 114, 0,   115, 116, 117,
      118, 119, 120, 121, 122, 0,   123, 0,   124, 125, 126, 127, 128, 129, 130, 131, 0,   132, 133, 0,   134, 135, 136, 137,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   138, 0,   139, 0,   0,   0,   0,   140,
      141, 142, 143, 0,   0,   0,   0,   0,   144, 0,   0,   0,   145, 0,   0,   146, 147, 148, 149, 0,   0,   0,   0};
  if (code >= 32 && code <= 126) return (uint16_t)(code - 31);
  if (code >= 161 && code <= 255) return hi[code - 161];
  return 0;
}

static bool parseCff(const uint8_t* p, size_t n, FontMetrics& m) {
  m.reset();
  m.cff = true;
  if (n < 4 || p[0] != 1) return false;
  CffIndex names, tops, strs, gsubrs;
  if (!names.read(p, n, p[2]) || !tops.read(p, n, names.end) || !strs.read(p, n, tops.end) || !gsubrs.read(p, n, strs.end))
    return false;
  size_t ta, tb;
  if (!tops.item(p, 0, ta, tb)) return false;
  double charsetOff = 0, encOff = 0, csOff = -1, privSize = -1, privOff = -1, fm = 0.001, fdArr = -1, fdSel = -1;
  bool cid = false, topFm = false;
  if (!cffDict(p, ta, tb, [&](int op, const CffStack& s) {
        if (op == 15 && s.size() == 1) charsetOff = s[0];
        else if (op == 16 && s.size() == 1) encOff = s[0];
        else if (op == 17 && s.size() == 1) csOff = s[0];
        else if (op == 18 && s.size() == 2) { privSize = s[0]; privOff = s[1]; }
        else if (op == 1207 && s.size() == 6) { fm = s[0]; topFm = true; }
        else if (op == 1230) cid = true;
        else if (op == 1236 && s.size() == 1) fdArr = s[0];
        else if (op == 1237 && s.size() == 1) fdSel = s[0];
        return true;
      }))
    return false;
  if (csOff < 0 || csOff >= n || fm <= 0) return false;
  // the Private DICT of a font DICT: default and nominal width
  auto privWidths = [&](double size, double off, double& dflt, double& nominal) {
    dflt = 0;
    nominal = 0;
    if (off < 0 || size <= 0 || off >= n || size > n - off) return true;
    return cffDict(p, (size_t)off, (size_t)(off + size), [&](int op, const CffStack& s) {
             if (op == 20 && s.size() == 1) dflt = s[0];
             else if (op == 21 && s.size() == 1) nominal = s[0];
             return true;
           }) &&
           dflt == std::floor(dflt) && nominal == std::floor(nominal) && std::fabs(dflt) <= 1e6 && std::fabs(nominal) <= 1e6;
  };
  CffIndex cs;
  if (!cs.read(p, n, (size_t)csOff) || cs.count == 0) return false;
  size_t ng = cs.count;
  double fdDflt[256], fdNom[256];  // per font DICT (at most 255)
  size_t nfd = 0;
  std::unique_ptr<uint8_t[]> fdOfP(new uint8_t[kMaxGlyphs]());  // per glyph (ng is a u16)
  uint8_t* fdOf = fdOfP.get();
  if (cid) {
    // CID-keyed: a Private DICT per font DICT of the FDArray, FDSelect picks one per glyph; the
    // scale is the Top DICT's FontMatrix times the font DICT's (one of them when only one is given)
    CffIndex fds;
    if (fdArr < 0 || fdArr >= n || !fds.read(p, n, (size_t)fdArr) || fds.count == 0 || fds.count > 255) return false;
    double fdFm = -1;
    for (size_t k = 0; k < fds.count; k++) {
      size_t fa, fb;
      double ps = -1, po = -1, f = -1, dd, nn;
      if (!fds.item(p, k, fa, fb) || !cffDict(p, fa, fb, [&](int op, const CffStack& s) {
            if (op == 18 && s.size() == 2) { ps = s[0]; po = s[1]; }
            else if (op == 1207 && s.size() == 6) f = s[0];
            return true;
          }))
        return false;
      if (k == 0) fdFm = f;
      else if (f != fdFm) return false;  // one scale for the whole font
      if (!privWidths(ps, po, dd, nn)) return false;
      fdDflt[nfd] = dd;
      fdNom[nfd++] = nn;
    }
    if (fdFm > 0) fm = topFm ? fm * fdFm : fdFm;
    if (fds.count > 1) {
      if (fdSel < 0 || fdSel >= n) return false;
      size_t o = (size_t)fdSel;
      int fmt = p[o++];
      bool ok = true;
      if (fmt == 0) {
        if (o + ng > n) return false;
        for (size_t g = 0; g < ng; g++) fdOf[g] = p[o + g];
      } else if (fmt == 3) {
        uint32_t nr = fntU16(p, n, o, ok);
        for (uint32_t k = 0; ok && k < nr; k++) {
          uint32_t first = fntU16(p, n, o + 2 + 3 * (size_t)k, ok), next = fntU16(p, n, o + 5 + 3 * (size_t)k, ok);
          if (!ok || o + 4 + 3 * (size_t)k >= n) return false;
          uint8_t fd = p[o + 4 + 3 * (size_t)k];
          for (uint32_t g = first; g < next && g < ng; g++) fdOf[g] = fd;
        }
        if (!ok) return false;
      } else {
        return false;
      }
      for (size_t g = 0; g < ng; g++)
        if (fdOf[g] >= fds.count) return false;
    }
  } else {
    double dd, nn;
    if (!privWidths(privSize, privOff, dd, nn)) return false;
    fdDflt[nfd] = dd;
    fdNom[nfd++] = nn;
  }
  double up = std::floor(1.0 / fm + 0.5);
  if (up < 16 || up > 16384 || std::fabs(up * fm - 1.0) > 1e-9) return false;
  m.upem = (uint32_t)up;
  if (ng >= kMaxGlyphs) return false;
  m.nglyphs = (uint32_t)ng;
  for (size_t g = 0; g < ng; g++) {
    size_t a, b;
    if (!cs.item(p, g, a, b)) return false;
    m.adv[g] = t2Width(p + a, b - a, (int32_t)fdDflt[fdOf[g]], (int32_t)fdNom[fdOf[g]]);
  }
  // charset: glyph id -> SID
  std::unique_ptr<uint32_t[]> sidP(new uint32_t[kMaxGlyphs]());
  uint32_t* sid = sidP.get();
  bool ok = true;
  if (charsetOff == 0) {
    for (size_t g = 0; g < ng; g++) sid[g] = g <= 228 ? (uint32_t)g : 0;
  } else if (charsetOff >= 3 && charsetOff < n) {
    size_t o = (size_t)charsetOff;
    int fmt = p[o++];
    size_t g = 1;
    if (fmt == 0) {
      for (; g < ng; g++, o += 2) sid[g] = fntU16(p, n, o, ok);
    } else if (fmt == 1 || fmt == 2) {
      while (g < ng && ok) {
        uint32_t s0 = fntU16(p, n, o, ok);
        uint32_t left = fmt == 1 ? (o + 2 < n ? p[o + 2] : (ok = false, 0)) : fntU16(p, n, o + 2, ok);
        o += fmt == 1 ? 3 : 4;
        for (uint32_t k = 0; k <= left && g < ng; k++) sid[g++] = s0 + k;
      }
    } else {
      return false;
    }
    if (!ok) return false;
  } else {
    return false;  // expert charsets
  }
  if (cid) {  // the charset holds CIDs
    for (auto& c : m.cidGid) c = kNoGlyph;
    for (size_t g = ng; g-- > 0;)
      if (sid[g] < 65536) m.cidGid[sid[g]] = (uint32_t)g;
    m.cidKeyed = true;
    m.ok = true;
    return true;
  }
  auto gidOfSid = [&](uint32_t s) -> int32_t {
    if (s == 0) return -1;
    for (size_t g = 1; g < ng; g++)
      if (sid[g] == s) return (int32_t)g;
    return -1;
  };
  if (encOff == 0) {
    int32_t bySid[150];  // StandardEncoding names have SIDs 1..149: first glyph of each
    for (auto& b : bySid) b = -1;
    for (size_t g = ng; g-- > 1;)
      if (sid[g] < 150) bySid[sid[g]] = (int32_t)g;
    for (int c = 0; c < 256; c++) {
      uint16_t sc = cffStdSid(c);
      m.enc[c] = sc ? bySid[sc] : -1;
    }
  } else if (encOff >= 2 && encOff < n) {
    size_t o = (size_t)encOff;
    int fmt = p[o] & 0x7F;
    bool sup = (p[o] & 0x80) != 0;
    o++;
    if (o >= n) return false;
    int cnt = p[o++];
    if (fmt == 0) {
      if (o + cnt > n) return false;
      for (int k = 0; k < cnt; k++)
        if ((size_t)(k + 1) < ng) m.enc[p[o + k]] = k + 1;
      o += cnt;
    } else if (fmt == 1) {
      if (o + 2 * (size_t)cnt > n) return false;
      int32_t g = 1;
      for (int k = 0; k < cnt; k++) {
        int first = p[o + 2 * k], left = p[o + 2 * k + 1];
        for (int c = first; c <= first + left && c < 256; c++, g++)
          if ((size_t)g < ng) m.enc[c] = g;
      }
      o += 2 * (size_t)cnt;
    } else {
      return false;
    }
    if (sup) {
      if (o >= n) return false;
      int ns = p[o++];
      for (int k = 0; k < ns; k++) {
        if (o + 3 > n) return false;
        int code = p[o];
        uint32_t s = (uint32_t)p[o + 1] << 8 | p[o + 2];
        o += 3;
        int32_t g = gidOfSid(s);
        if (g > 0) m.enc[code] = g;
      }
    }
  } else {
    return false;  // expert encoding
  }
  m.ok = true;
  return true;
}

// ---- Type 1 ---------------------------------------------------------------------------------------
static const char* const kStdEncLo =
    "space exclam quotedbl numbersign dollar percent ampersand quoteright parenleft parenright asterisk plus comma "
    "hyphen period slash zero one two three four five six seven eight nine colon semicolon less equal greater "
    "question at A B C D E F G H I J K L M N O P Q R S T U V W X Y Z bracketleft backslash bracketright asciicircum "
    "underscore quoteleft a b c d e f g h i j k l m n o p q r s t u v w x y z braceleft bar braceright asciitilde";
static const char* const kStdEncHi =
    "exclamdown cent sterling fraction yen florin section currency quotesingle quotedblleft guillemotleft "
    "guilsinglleft guilsinglright fi fl - endash dagger daggerdbl periodcentered - paragraph bullet quotesinglbase "
    "quotedblbase quotedblright guillemotright ellipsis perthousand - questiondown - grave acute circumflex tilde "
    "macron breve dotaccent dieresis - ring cedilla - hungarumlaut ogonek caron emdash - - - - - - - - - - - - - - - "
    "- AE - ordfeminine - - - - Lslash Oslash OE ordmasculine - - - - - ae - - - dotlessi - - lslash oslash oe "
    "germandbls - - - -";

// StandardEncoding glyph name of a code (in the table, len bytes); false if none
static bool t1StdName(int code, const char*& name, size_t& len) {
  const char* t = code >= 32 && code <= 126 ? kStdEncLo : code >= 161 && code <= 255 ? kStdEncHi : nullptr;
  if (!t) return false;
  int k = code >= 161 ? code - 161 : code - 32;
  const char* q = t;
  for (; k > 0; k--) q = strchr(q, ' ') + 1;
  const char* e = strchr(q, ' ');
  name = q;
  len = e ? (size_t)(e - q) : strlen(q);
  return !(len == 1 && q[0] == '-');
}

// width of a decrypted Type 1 charstring (lead bytes skipped): "sbx wx hsbw" or "sbx sby wx wy
// sbw", an "a b div" giving a fraction; false if anything else comes first
static bool t1Width(const uint8_t* c, size_t n, int64_t& num, uint32_t& den) {
  int64_t sn[24];
  uint32_t sd[24];
  int sp = 0;
  size_t i = 0;
  while (i < n) {
    if (c[i] >= 32) {
      int32_t v;
      if (!t1CsNumber(c, n, i, v) || sp >= 24) return false;
      sn[sp] = v;
      sd[sp++] = 1;
      continue;
    }
    if (c[i] == 13 && sp == 2) { num = sn[1]; den = sd[1]; return true; }
    if (c[i] == 12 && i + 1 < n) {
      if (c[i + 1] == 7 && sp == 4) { num = sn[2]; den = sd[2]; return true; }
      if (c[i + 1] == 12 && sp >= 2 && sd[sp - 1] == 1 && sd[sp - 2] == 1 && sn[sp - 1] > 0 && sn[sp - 1] <= 65535) {
        sd[sp - 2] = (uint32_t)sn[sp - 1];
        sp--;
        i += 2;
        continue;
      }
    }
    return false;
  }
  return false;
}

// glyph name -> glyph index: open addressing at twice the glyph maximum, names pointing into the
// font text (1.5 MB, on the heap)
struct T1NameTable {
  struct Slot {
    const uint8_t* p;
    uint32_t n;
    int32_t idx;
  };
  static const size_t kSlots = 2 * kMaxGlyphs;
  std::unique_ptr<Slot[]> tab{new Slot[kSlots]()};
  size_t count = 0;
  Slot& find(const uint8_t* q, size_t n) {
    uint64_t h = 1469598103934665603ull;
    for (size_t i = 0; i < n; i++) h = (h ^ q[i]) * 1099511628211ull;
    size_t i = (size_t)(h ^ (h >> 29)) & (kSlots - 1);
    while (tab[i].p && !(tab[i].n == n && memcmp(tab[i].p, q, n) == 0)) i = (i + 1) & (kSlots - 1);
    return tab[i];
  }
  int32_t get(const uint8_t* q, size_t n) {
    Slot& s = find(q, n);
    return s.p ? s.idx : -1;
  }
};

static bool parseType1(const Bytes& stored, const Bytes& recon, FontMetrics& m) {
  m.reset();
  Bytes t;
  size_t s, n, lenIV;
  uint8_t flags;
  RdNames rd;
  if (!softly([&] { return t1Plain(recon, stored, t, s, n, flags, rd, lenIV); }) || !(flags & 1)) return false;
  const uint8_t* p = t.data();
  // FontMatrix and Encoding in the cleartext part
  if (const uint8_t* q = memfind(p, s, "/FontMatrix", 11)) {
    Lexer lx(p, s, (size_t)(q - p) + 11);
    Obj a;
    if (!lx.parse(a) || a.t != Obj::ARR || a.a.empty()) return false;
    double f = a.a[0].t == Obj::INT ? (double)a.a[0].i : a.a[0].t == Obj::REAL ? a.a[0].r : 0;
    if (f <= 0) return false;
    double up = std::floor(1.0 / f + 0.5);
    if (up < 16 || up > 16384 || std::fabs(up * f - 1.0) > 1e-9) return false;
    m.upem = (uint32_t)up;
  }
  // charstrings: "/name len RD <body>"
  std::vector<std::pair<size_t, size_t>> blobs;
  t1FindBlobs(p + s, n, rd, blobs);
  T1NameTable byName;
  for (auto& b : blobs) {
    size_t h = b.first;  // back over "RD ", the length and the name
    size_t q = h;
    while (q > 0 && p[s + q - 1] != '/' && h - q < 64) q--;
    if (q == 0 || p[s + q - 1] != '/') continue;
    size_t ne = q;
    while (ne < h && isRegular(p[s + ne])) ne++;
    const uint8_t* name = p + s + q;
    size_t nameLen = ne - q;
    int64_t w;
    uint32_t d;
    if (b.second < lenIV || !t1Width(p + s + b.first + lenIV, b.second - lenIV, w, d)) {
      w = kNoAdv;
      d = 1;
    }
    T1NameTable::Slot& sl = byName.find(name, nameLen);
    if (sl.p) continue;  // a name defined twice: the first counts
    if (m.nglyphs == kMaxGlyphs - 1) break;  // (as CFF and TrueType: 65535 glyphs at most)
    sl = {name, (uint32_t)nameLen, (int32_t)m.nglyphs};
    m.adv[m.nglyphs] = w == kNoAdv || w < INT32_MIN + 1 || w > INT32_MAX ? kNoAdv : (int32_t)w;
    m.den[m.nglyphs++] = (uint16_t)std::min<uint32_t>(d, 65535);
  }
  if (m.nglyphs == 0) return false;
  m.hasDen = true;
  for (int c = 0; c < 256; c++) {
    const char* nm;
    size_t nl;
    if (t1StdName(c, nm, nl) && nl) m.stdEnc[c] = byName.get((const uint8_t*)nm, nl);
  }
  const uint8_t* e = memfind(p, s, "/Encoding", 9);
  if (e) {
    size_t i = (size_t)(e - p) + 9;
    while (i < s && isWS(p[i])) i++;
    if (i + 16 <= s && memcmp(p + i, "StandardEncoding", 16) == 0) {
      memcpy(m.enc, m.stdEnc, sizeof(m.enc));
    } else {
      // "dup <code> /<name> put" up to the "def" that ends the encoding
      const uint8_t* end = memfind(p + i, s - i, "readonly def", 12);
      if (!end) end = memfind(p + i, s - i, " def", 4);
      size_t lim = end ? (size_t)(end - p) : s;
      for (const uint8_t* q = p + i; (q = memfind(q, p + lim - q, "dup ", 4)) != nullptr; q += 4) {
        size_t k = (size_t)(q - p) + 4, d0 = k;
        uint32_t code = 0;
        while (k < lim && p[k] >= '0' && p[k] <= '9' && k - d0 < 3) code = code * 10 + (p[k++] - '0');
        if (k == d0 || code > 255 || k >= lim || p[k] != ' ' || k + 1 >= lim || p[k + 1] != '/') continue;
        size_t nb = k + 2, ne = nb;
        while (ne < lim && isRegular(p[ne])) ne++;
        int32_t g = byName.get(p + nb, ne - nb);
        if (g >= 0) m.enc[code] = g;
      }
    }
  }
  m.ok = true;
  return true;
}

// ---- width prediction -------------------------------------------------------------------------------
// kinds: simple TrueType (/Widths), simple CFF (/Widths), CIDFontType2 (/W; identity CIDToGIDMap
// or the map stream's bytes), Type 1 (/Widths), CID-keyed CFF (/W; CID -> glyph by the charset).
// TrueType code->glyph maps: 0 (3,0) at 0xF000+code then code, 1 (1,0), 2 (3,1) with the code as
// Unicode, 3 glyph id = code. Type 1 maps: 0 the font's own encoding, 1 StandardEncoding. A code
// without a glyph is predicted 0. Rules: 0 round, 1 floor, 2 ceil of advance * 1000 /
// unitsPerEm; 3, 4, 5 rounded to 1, 2, 3 decimals with trailing zeros dropped.
enum { WK_TT, WK_CFF, WK_CID2, WK_T1, WK_CID0, WK_CID2M, WK_COUNT };
static const int kWidthMaps = 4, kWidthRules = 6;
static int widthMaps(int kind) { return kind == WK_TT ? 4 : kind == WK_T1 ? 2 : 1; }

// a predicted width as written: an integer or up to 3 decimals
typedef FixedStr<47> WidthStr;
static bool widthPredict(const FontMetrics& m, int kind, int map, int rule, uint32_t code, WidthStr& s) {
  if (!m.ok || rule < 0 || rule >= kWidthRules) return false;
  int64_t adv = 0;
  uint64_t den = 1;
  uint32_t ng = m.nglyphs;
  if (kind == WK_TT) {
    uint32_t g = 0;
    if (map == 0) {
      g = code <= 0xFF ? ttCmapLookup(m.cmap[0], m.cmapLen[0], 0xF000 + code) : 0;
      if (!g) g = ttCmapLookup(m.cmap[0], m.cmapLen[0], code);
    } else if (map == 1) g = ttCmapLookup(m.cmap[1], m.cmapLen[1], code);
    else if (map == 2) g = ttCmapLookup(m.cmap[2], m.cmapLen[2], code);
    else if (map == 3) g = code;
    else return false;
    if (g && g < ng) adv = m.adv[g];
  } else if (kind == WK_CFF) {
    if (map != 0) return false;
    int32_t g = code < 256 ? m.enc[code] : -1;
    if (g > 0 && (uint32_t)g < ng) adv = m.adv[g];
  } else if (kind == WK_CID2) {
    if (map != 0) return false;
    if (code < ng) adv = m.adv[code];
  } else if (kind == WK_CID2M) {
    if (map != 0) return false;
    uint32_t g = 2 * (size_t)code + 1 < m.cidMapLen ? (uint32_t)m.cidMap[2 * code] << 8 | m.cidMap[2 * code + 1] : 0;
    if (g && g < ng) adv = m.adv[g];
  } else if (kind == WK_T1) {
    if (map > 1) return false;
    int32_t g = code < 256 ? (map ? m.stdEnc[code] : m.enc[code]) : -1;
    if (g >= 0 && (uint32_t)g < ng) {
      adv = m.adv[g];
      den = m.hasDen ? m.den[g] : 1;
    }
  } else if (kind == WK_CID0) {
    if (map != 0) return false;
    uint32_t g = m.cidKeyed && code < 65536 ? m.cidGid[code] : kNoGlyph;
    if (g < ng) adv = m.adv[g];
  } else {
    return false;
  }
  if (adv == kNoAdv || adv < 0 || den == 0) return false;
  uint64_t num = (uint64_t)adv * 1000, up = (uint64_t)m.upem * den;
  char buf[48];
  if (rule <= 2) {
    uint64_t v = rule == 0 ? (2 * num + up) / (2 * up) : rule == 1 ? num / up : (num + up - 1) / up;
    snprintf(buf, sizeof(buf), "%llu", (unsigned long long)v);
  } else {
    int k = rule - 2;
    uint64_t sc = k == 1 ? 10 : k == 2 ? 100 : 1000;
    uint64_t x = (2 * num * sc + up) / (2 * up);
    uint64_t ip = x / sc, fp = x % sc;
    if (!fp) snprintf(buf, sizeof(buf), "%llu", (unsigned long long)ip);
    else {
      int l = snprintf(buf, sizeof(buf), "%llu.%0*llu", (unsigned long long)ip, k, (unsigned long long)fp);
      while (l > 0 && buf[l - 1] == '0') buf[--l] = 0;
    }
  }
  s.set(buf);
  return true;
}

// Walks a /Widths or /W array token by token (encoder: the original text; decoder: the restored
// text) and says, before each number, whether it is a width and of which code or CID.
struct WidthsWalk {
  bool cid = false;    // /W
  uint32_t first = 0;  // /Widths: /FirstChar
  int depth = 0, st = 0;  // /W at depth 1: 0 expects a start CID, 1 after it, 2 after an end CID
  uint64_t c0 = 0, k = 0;
  bool done = false, bad = false;
  bool widthNext(uint32_t& code) const {
    uint64_t c;
    if (!cid) {
      if (depth != 1) return false;
      c = (uint64_t)first + k;
    } else if (depth == 2) c = c0 + k;
    else if (depth == 1 && st == 2) c = c0;
    else return false;
    if (c > 0xFFFFFF) return false;
    code = (uint32_t)c;
    return true;
  }
  void number(const uint8_t* t, size_t n) {
    if (!cid) { if (depth != 1) bad = true; k++; return; }
    if (depth == 2) { k++; return; }
    if (depth != 1) { bad = true; return; }
    if (st == 2) { st = 0; return; }
    uint64_t v = 0;
    if (n == 0 || n > 9) { bad = true; return; }
    for (size_t i = 0; i < n; i++) {
      if (t[i] < '0' || t[i] > '9') { bad = true; return; }
      v = v * 10 + (t[i] - '0');
    }
    if (st == 0) { c0 = v; st = 1; }
    else st = 2;
  }
  void open() {
    depth++;
    if (depth == 1) return;
    if (cid && depth == 2 && st == 1) { k = 0; return; }
    bad = true;
  }
  void close() {
    if (depth == 2) st = 0;
    if (--depth == 0) done = true;
    if (depth < 0) bad = true;
  }
};

static inline bool widthNumStart(uint8_t c) { return (c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.'; }

// Metrics of the last few font programs, by source key: a fixed set of slots (about 1 MB each,
// allocated on first use), reused round robin. What the widths come out as does not depend on
// what the cache holds, so the encoder and the decoder may hold different fonts.
struct FontCache {
  static const int kSlots = 8;
  struct Slot {
    std::string key;
    std::unique_ptr<FontMetrics> m;
  };
  Slot slot[kSlots];
  int next = 0;
  // the metrics for key; found = false: a reset slot for the caller to fill
  FontMetrics* get(const std::string& key, bool& found) {
    for (auto& s : slot)
      if (s.m && s.key == key) {
        found = true;
        return s.m.get();
      }
    Slot& s = slot[next];
    next = (next + 1) % kSlots;
    if (!s.m) s.m.reset(new FontMetrics());
    s.key = key;
    s.m->reset();
    found = false;
    return s.m.get();
  }
};
