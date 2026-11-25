// pdf_ccitt.inc -- CCITTFaxDecode as an exactly invertible layer (L_CCITT, plan M10, opt-in -g).
//
// Group 4 (K < 0) and Group 3 one-dimensional (K = 0) data is decoded to 1 bit per pixel rows
// (PDF sample convention: with BlackIs1 false, 0 = black) and re-encoded with the standard T.4/T.6
// procedure: 2D mode selection pass if b2 < a1, vertical if |a1 - b1| <= 3, else horizontal;
// run lengths as makeup + terminating codes. The recon record keeps the coding options found in
// the data (EOLs, byte alignment, end-of-block, padding) and any tail; the layer is used only if
// re-encoding reproduces the input.
//
// Recon: u8 K class (0 = G3 1D, 1 = G4), var columns, var rows, u8 flags (bit0 EOL before each
// row, bit1 rows byte-aligned, bit2 end-of-block present, bit3 BlackIs1), u8 padding bit count,
// u8 padding value, var tail length + tail.

struct CcittCode {
  uint16_t code;
  uint8_t len;
};
static const char* const kWhiteTerm[64] = {
    "00110101", "000111", "0111", "1000", "1011", "1100", "1110", "1111", "10011", "10100", "00111", "01000",
    "001000", "000011", "110100", "110101", "101010", "101011", "0100111", "0001100", "0001000", "0010111",
    "0000011", "0000100", "0101000", "0101011", "0010011", "0100100", "0011000", "00000010", "00000011",
    "00011010", "00011011", "00010010", "00010011", "00010100", "00010101", "00010110", "00010111", "00101000",
    "00101001", "00101010", "00101011", "00101100", "00101101", "00000100", "00000101", "00001010", "00001011",
    "01010010", "01010011", "01010100", "01010101", "00100100", "00100101", "01011000", "01011001", "01011010",
    "01011011", "01001010", "01001011", "00110010", "00110011", "00110100"};
static const char* const kBlackTerm[64] = {
    "0000110111", "010", "11", "10", "011", "0011", "0010", "00011", "000101", "000100", "0000100", "0000101",
    "0000111", "00000100", "00000111", "000011000", "0000010111", "0000011000", "0000001000", "00001100111",
    "00001101000", "00001101100", "00000110111", "00000101000", "00000010111", "00000011000", "000011001010",
    "000011001011", "000011001100", "000011001101", "000001101000", "000001101001", "000001101010",
    "000001101011", "000011010010", "000011010011", "000011010100", "000011010101", "000011010110",
    "000011010111", "000001101100", "000001101101", "000011011010", "000011011011", "000001010100",
    "000001010101", "000001010110", "000001010111", "000001100100", "000001100101", "000001010010",
    "000001010011", "000000100100", "000000110111", "000000111000", "000000100111", "000000101000",
    "000001011000", "000001011001", "000000101011", "000000101100", "000001011010", "000001100110",
    "000001100111"};
// makeup codes for 64, 128, ..., 1728
static const char* const kWhiteMakeup[27] = {
    "11011", "10010", "010111", "0110111", "00110110", "00110111", "01100100", "01100101", "01101000",
    "01100111", "011001100", "011001101", "011010010", "011010011", "011010100", "011010101", "011010110",
    "011010111", "011011000", "011011001", "011011010", "011011011", "010011000", "010011001", "010011010",
    "011000", "010011011"};
static const char* const kBlackMakeup[27] = {
    "0000001111", "000011001000", "000011001001", "000001011011", "000000110011", "000000110100",
    "000000110101", "0000001101100", "0000001101101", "0000001001010", "0000001001011", "0000001001100",
    "0000001001101", "0000001110010", "0000001110011", "0000001110100", "0000001110101", "0000001110110",
    "0000001110111", "0000001010010", "0000001010011", "0000001010100", "0000001010101", "0000001011010",
    "0000001011011", "0000001100100", "0000001100101"};
// extended makeup codes for 1792, 1856, ..., 2560 (both colours)
static const char* const kExtMakeup[13] = {"00000001000",  "00000001100",  "00000001101",  "000000010010",
                                           "000000010011", "000000010100", "000000010101", "000000010110",
                                           "000000010111", "000000011100", "000000011101", "000000011110",
                                           "000000011111"};

struct CcittTables {
  // decode: map (len, code) -> run length; built once
  std::map<std::pair<int, int>, int> white, black;
  CcittCode wt[64], bt[64], wm[40], bm[40];  // makeup index k = run / 64 - 1 (0..39)
  CcittTables() {
    auto cc = [](const char* s) {
      CcittCode c{0, (uint8_t)strlen(s)};
      for (const char* p = s; *p; p++) c.code = (uint16_t)(c.code << 1 | (*p - '0'));
      return c;
    };
    for (int i = 0; i < 64; i++) {
      wt[i] = cc(kWhiteTerm[i]);
      bt[i] = cc(kBlackTerm[i]);
      white[{wt[i].len, wt[i].code}] = i;
      black[{bt[i].len, bt[i].code}] = i;
    }
    for (int i = 0; i < 27; i++) {
      wm[i] = cc(kWhiteMakeup[i]);
      bm[i] = cc(kBlackMakeup[i]);
      white[{wm[i].len, wm[i].code}] = 64 * (i + 1);
      black[{bm[i].len, bm[i].code}] = 64 * (i + 1);
    }
    for (int i = 0; i < 13; i++) {
      CcittCode c = cc(kExtMakeup[i]);
      wm[27 + i] = bm[27 + i] = c;
      white[{c.len, c.code}] = 1792 + 64 * i;
      black[{c.len, c.code}] = 1792 + 64 * i;
    }
  }
};
static const CcittTables& ccittTables() {
  static CcittTables t;
  return t;
}

struct BitReader {
  const uint8_t* p;
  size_t nbits, pos = 0;
  BitReader(const uint8_t* d, size_t n) : p(d), nbits(n * 8) {}
  int bit() {
    if (pos >= nbits) return -1;
    int b = (p[pos >> 3] >> (7 - (pos & 7))) & 1;
    pos++;
    return b;
  }
  bool peek(int n, uint32_t& v) const {
    if (pos + n > nbits) return false;
    v = 0;
    for (int k = 0; k < n; k++) v = v << 1 | ((p[(pos + k) >> 3] >> (7 - ((pos + k) & 7))) & 1);
    return true;
  }
};
struct BitWriter {
  Bytes& out;
  uint32_t acc = 0;
  int n = 0;
  explicit BitWriter(Bytes& o) : out(o) {}
  void put(uint32_t v, int len) {
    for (int k = len - 1; k >= 0; k--) {
      acc = acc << 1 | ((v >> k) & 1);
      if (++n == 8) { out.push_back((uint8_t)acc); acc = 0; n = 0; }
    }
  }
  void code(const CcittCode& c) { put(c.code, c.len); }
  int fill() const { return (8 - n) & 7; }
};

static const uint32_t kEOL = 1;  // 000000000001, 12 bits

// one run (makeup codes + terminating code); -1 on error
static int ccittReadRun(BitReader& br, bool black) {
  const auto& map = black ? ccittTables().black : ccittTables().white;
  int total = 0;
  for (;;) {
    int code = 0, len = 0, run = -1;
    while (len < 13) {
      int b = br.bit();
      if (b < 0) return -1;
      code = code << 1 | b;
      len++;
      auto it = map.find({len, code});
      if (it != map.end()) { run = it->second; break; }
    }
    if (run < 0) return -1;
    total += run;
    if (run < 64) return total;
  }
}

static void ccittWriteRun(BitWriter& bw, int run, bool black) {
  const CcittTables& t = ccittTables();
  while (run >= 2624) { bw.code(t.wm[39]); run -= 2560; }  // 2560 makeup (shared code)
  if (run >= 64) {
    int k = run / 64 - 1;
    bw.code(black ? t.bm[k] : t.wm[k]);
    run -= 64 * (k + 1);
  }
  bw.code(black ? t.bt[run] : t.wt[run]);
}

// changing elements of a row of pixels (1 = black)
static void ccittChanges(const std::vector<uint8_t>& px, std::vector<int>& ch) {
  ch.clear();
  uint8_t c = 0;
  for (size_t i = 0; i < px.size(); i++)
    if (px[i] != c) { ch.push_back((int)i); c = px[i]; }
}

static void ccittB1B2(const std::vector<int>& ref, int a0, int color, int cols, int& b1, int& b2) {
  // b1: first change on the reference line right of a0 into the colour opposite to `color`
  // (changes into black have even index); a0 = -1 at the start of a row
  size_t i = color ? 1 : 0;
  while (i < ref.size() && ref[i] <= a0) i += 2;
  b1 = i < ref.size() ? ref[i] : cols;
  b2 = i + 1 < ref.size() ? ref[i + 1] : cols;
}

// decode one 2D row given the reference changes; fills cur changes. false on error.
static bool ccittDecode2D(BitReader& br, const std::vector<int>& ref, std::vector<int>& cur, int cols) {
  cur.clear();
  int a0 = -1, color = 0;
  while (a0 < cols) {
    int b1, b2;
    ccittB1B2(ref, a0, color, cols, b1, b2);
    // mode code
    int b = br.bit();
    if (b < 0) return false;
    if (b == 1) {  // V0
      if (b1 > cols) return false;
      cur.push_back(b1);
      a0 = b1;
      color ^= 1;
      continue;
    }
    int b2b = br.bit(), b3 = br.bit();
    if (b2b < 0 || b3 < 0) return false;
    if (b2b == 1) {  // 01x: VR1 (011) / VL1 (010)
      int a1 = b3 ? b1 + 1 : b1 - 1;
      if (a1 < 0 || a1 > cols || a1 < a0) return false;
      cur.push_back(a1);
      a0 = a1;
      color ^= 1;
      continue;
    }
    if (b3 == 1) {  // 001: horizontal
      int r1 = ccittReadRun(br, color), r2 = ccittReadRun(br, !color);
      if (r1 < 0 || r2 < 0) return false;
      int s = a0 < 0 ? 0 : a0;
      int a1 = s + r1, a2 = a1 + r2;
      if (a2 > cols) return false;
      cur.push_back(a1);
      cur.push_back(a2);
      a0 = a2;
      continue;
    }
    int b4 = br.bit();
    if (b4 < 0) return false;
    if (b4 == 1) {  // 0001: pass
      if (b2 > cols) return false;
      a0 = b2;
      continue;
    }
    int b5 = br.bit(), b6 = br.bit();
    if (b5 < 0 || b6 < 0) return false;
    if (b5 == 1) {  // 00001x: VR2 (000011) / VL2 (000010)
      int a1 = b6 ? b1 + 2 : b1 - 2;
      if (a1 < 0 || a1 > cols || a1 < a0) return false;
      cur.push_back(a1);
      a0 = a1;
      color ^= 1;
      continue;
    }
    if (b6 == 1) {  // 000001x: VR3 / VL3
      int b7 = br.bit();
      if (b7 < 0) return false;
      int a1 = b7 ? b1 + 3 : b1 - 3;
      if (a1 < 0 || a1 > cols || a1 < a0) return false;
      cur.push_back(a1);
      a0 = a1;
      color ^= 1;
      continue;
    }
    return false;  // extensions / EOL inside a row: not supported
  }
  return true;
}

static void ccittEncode2D(BitWriter& bw, const std::vector<int>& ref, const std::vector<int>& cur, int cols) {
  int a0 = -1, color = 0;
  size_t ci = 0;
  while (a0 < cols) {
    while (ci < cur.size() && cur[ci] <= a0) ci++;
    int a1 = ci < cur.size() ? cur[ci] : cols;
    int a2 = ci + 1 < cur.size() ? cur[ci + 1] : cols;
    int b1, b2;
    ccittB1B2(ref, a0, color, cols, b1, b2);
    if (b2 < a1) {
      bw.put(1, 4);  // pass 0001
      a0 = b2;
    } else if (a1 - b1 >= -3 && a1 - b1 <= 3) {
      static const uint32_t vc[7] = {2, 2, 2, 1, 3, 3, 3};  // VL3..VR3 codes
      static const int vl[7] = {7, 6, 3, 1, 3, 6, 7};
      int d = a1 - b1 + 3;
      bw.put(vc[d], vl[d]);
      a0 = a1;
      color ^= 1;
      ci++;
    } else {
      bw.put(1, 3);  // horizontal 001
      int s = a0 < 0 ? 0 : a0;
      ccittWriteRun(bw, a1 - s, color);
      ccittWriteRun(bw, a2 - a1, !color);
      a0 = a2;
      ci += 2;
    }
  }
}

static bool ccittDecode1D(BitReader& br, std::vector<int>& cur, int cols) {
  cur.clear();
  int a = 0, color = 0;
  while (a < cols) {
    int r = ccittReadRun(br, color);
    if (r < 0 || a + r > cols) return false;
    a += r;
    if (a < cols) cur.push_back(a);
    color ^= 1;
  }
  return true;
}

static void ccittEncode1D(BitWriter& bw, const std::vector<int>& cur, int cols) {
  int a = 0, color = 0;
  size_t i = 0;
  while (a < cols) {
    int next = i < cur.size() ? cur[i] : cols;
    ccittWriteRun(bw, next - a, color);
    a = next;
    color ^= 1;
    i++;
  }
}

struct CcittParams {
  int k = 0, cols = 1728, rows = 0;
  bool eol = false, align = false, eob = true, black1 = false;
};

static void ccittRowBits(const std::vector<int>& ch, int cols, bool black1, uint8_t* dst) {
  size_t rb = ((size_t)cols + 7) / 8;
  memset(dst, black1 ? 0x00 : 0xFF, rb);  // white everywhere
  int color = 0, a = 0;
  for (size_t i = 0; i <= ch.size(); i++) {
    int e = i < ch.size() ? std::min(ch[i], cols) : cols;
    if (color)
      for (int x = a; x < e; x++) {
        if (black1) dst[x >> 3] |= (uint8_t)(0x80 >> (x & 7));
        else dst[x >> 3] &= (uint8_t)~(0x80 >> (x & 7));
      }
    a = e;
    color ^= 1;
  }
}

static void ccittRowChanges(const uint8_t* row, int cols, bool black1, std::vector<int>& ch) {
  ch.clear();
  int c = 0;
  for (int x = 0; x < cols; x++) {
    int bit = (row[x >> 3] >> (7 - (x & 7))) & 1;
    int black = black1 ? bit : !bit;
    if (black != c) { ch.push_back(x); c = black; }
  }
}

// encode rows + options -> bytes (shared by verify and restore)
static bool ccittEncodeAll(const CcittParams& p, bool g4, const Bytes& rows, int nrows, int padCount, int padVal,
                           Bytes& out) {
  size_t rb = ((size_t)p.cols + 7) / 8;
  if (rows.size() != rb * (size_t)nrows) return false;
  out.clear();
  BitWriter bw(out);
  std::vector<int> ref, cur;
  for (int r = 0; r < nrows; r++) {
    if (p.align && bw.n) bw.put(0, bw.fill());
    if (p.eol) {
      if (p.align && !g4) {
        // EOL ends on a byte boundary: fill zeros before it
        int f = (8 - ((bw.n + 12) & 7)) & 7;
        bw.put(0, f);
      }
      bw.put(kEOL, 12);
    }
    ccittRowChanges(rows.data() + rb * r, p.cols, p.black1, cur);
    if (g4) ccittEncode2D(bw, ref, cur, p.cols);
    else ccittEncode1D(bw, cur, p.cols);
    ref.swap(cur);
  }
  if (p.eob) {
    if (g4) { bw.put(kEOL, 12); bw.put(kEOL, 12); }
    else for (int k = 0; k < 6; k++) bw.put(kEOL, 12);
  }
  if (padCount) bw.put((uint32_t)padVal, padCount);
  return bw.n == 0;
}

static bool ccittDecodeLayer(const CcittParams& p0, const Bytes& in, Bytes& out, Bytes& recon) {
  CcittParams p = p0;
  if (p.k > 0 || p.cols < 1 || p.cols > 1 << 16) return false;
  bool g4 = p.k < 0;
  size_t rb = ((size_t)p.cols + 7) / 8;
  BitReader br(in.data(), in.size());
  std::vector<int> ref, cur;
  out.clear();
  int nrows = 0;
  bool eob = false, sawEol = false;
  for (;;) {
    if (p.rows > 0 && nrows >= p.rows) break;
    if (p.align) br.pos = (br.pos + 7) & ~(size_t)7;
    uint32_t v;
    // EOL (possibly preceded by fill zeros)
    size_t save = br.pos;
    while (br.peek(12, v) && v == 0 && br.pos < br.nbits) br.pos++;  // skip fill bits
    if (br.peek(12, v) && v == kEOL) {
      br.pos += 12;
      uint32_t v2;
      // end of block: a second EOL (G4) or a run of EOLs (G3 RTC)
      if (br.peek(12, v2) && v2 == kEOL) { eob = true; break; }
      sawEol = true;
    } else {
      br.pos = save;
    }
    if (br.pos >= br.nbits) break;
    size_t rowStart = br.pos;
    bool ok = g4 ? ccittDecode2D(br, ref, cur, p.cols) : ccittDecode1D(br, cur, p.cols);
    if (!ok) { br.pos = rowStart; break; }
    out.resize(out.size() + rb);
    ccittRowBits(cur, p.cols, p.black1, out.data() + out.size() - rb);
    ref.swap(cur);
    nrows++;
    if (out.size() > kMaxDecoded) return false;
  }
  if (nrows == 0) return false;
  // after an end-of-block marker: skip the remaining EOLs of it
  if (eob) {
    uint32_t v;
    while (br.peek(12, v) && v == kEOL) br.pos += 12;
  }
  p.eol = sawEol;
  p.eob = eob;
  // padding to the byte boundary, then tail bytes
  int padCount = (int)((8 - (br.pos & 7)) & 7);
  uint32_t padVal = 0;
  if (padCount && !br.peek(padCount, padVal)) return false;
  br.pos += padCount;
  size_t tailStart = br.pos / 8;
  // try the coding options: as detected, then with the opposite EOL / alignment choice
  for (int attempt = 0; attempt < 4; attempt++) {
    CcittParams q = p;
    if (attempt & 1) q.align = !q.align;
    if (attempt & 2) q.eol = !q.eol;
    Bytes re;
    if (!ccittEncodeAll(q, g4, out, nrows, padCount, (int)padVal, re)) continue;
    if (re.size() != tailStart || memcmp(re.data(), in.data(), re.size()) != 0) continue;
    recon.clear();
    recon.push_back(g4 ? 1 : 0);
    putVarTo(recon, (uint64_t)q.cols);
    putVarTo(recon, (uint64_t)nrows);
    recon.push_back((uint8_t)((q.eol ? 1 : 0) | (q.align ? 2 : 0) | (q.eob ? 4 : 0) | (q.black1 ? 8 : 0)));
    recon.push_back((uint8_t)padCount);
    recon.push_back((uint8_t)padVal);
    putLenBytesTo(recon, in.data() + tailStart, in.size() - tailStart);
    return true;
  }
  return false;
}

static bool ccittEncodeLayer(const Bytes& recon, const Bytes& in, Bytes& out) {
  RBuf r(recon);
  bool g4 = r.u8() != 0;
  CcittParams p;
  p.cols = (int)r.var();
  int nrows = (int)r.var();
  uint8_t f = r.u8();
  p.eol = f & 1;
  p.align = (f & 2) != 0;
  p.eob = (f & 4) != 0;
  p.black1 = (f & 8) != 0;
  int padCount = r.u8(), padVal = r.u8();
  if (p.cols < 1 || p.cols > 1 << 16 || padCount > 7) return false;
  if (!ccittEncodeAll(p, g4, in, nrows, padCount, padVal, out)) return false;
  r.appendTo(r.len(), out);
  return true;
}

static bool ccittDecodeFP(const FilterParams& fp, const Bytes& in, Bytes& out, Bytes& recon) {
  CcittParams p;
  p.k = fp.ccittK;
  p.cols = fp.ccittCols;
  p.rows = fp.ccittRows;
  p.eol = fp.ccittEol;
  p.align = fp.ccittAlign;
  p.eob = fp.ccittEob;
  p.black1 = fp.ccittBlack1;
  return ccittDecodeLayer(p, in, out, recon);
}
