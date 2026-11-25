// pdf_filt.inc -- filter engine: exact inversion of PDF filter layers.
//
// For every layer there are two functions sharing one recon record format:
//   layerDecode  (encoder only): PDF-side bytes -> decoded bytes + recon; false unless the layer
//                re-encodes byte-exactly (each decoder checks this itself, through layerEncode).
//   layerEncode  (both sides):   decoded bytes + recon -> PDF-side bytes.

enum LayerType : uint8_t { L_NONE, L_FLATE, L_LZW, L_AHX, L_A85, L_RL, L_PNG, L_TIFF, L_BMP, L_T1, L_OBJSTM, L_XREFROWS, L_COUNT };
static const char* const kLayerName[L_COUNT] = {"-", "Flate", "LZW", "AHx", "A85", "RL", "PNG", "TIFF", "BMP", "T1", "ObjStm", "XRefRows"};

struct Layer {
  uint8_t type = L_NONE;
  Bytes recon;
};

struct ObjPositions;
struct CodecCtx {
  uint8_t zlibHdr[2] = {0x78, 0x9C};
  const ObjPositions* objPos = nullptr;  // objects written so far (L_XREFROWS predictions)
  bool verify = true;  // encoder: re-encode each Flate layer and compare (off with -n)
};

// Predictor / filter parameters from /DecodeParms (encoder side only).
struct FilterParams {
  int predictor = 1, colors = 1, bpc = 8, columns = 1, earlyChange = 1;
  bool t1Charstrings = true;  // L_T1: also remove charstring encryption
  int64_t objstmN = 0, objstmFirst = 0;  // L_OBJSTM: /N and /First
  std::vector<uint64_t> xrefW;                     // L_XREFROWS: /W
  std::vector<std::pair<uint64_t, uint64_t>> xrefIndex;  // L_XREFROWS: /Index ranges
};

// recon record reader
struct RBuf {
  const Bytes& b;
  size_t p = 0;
  explicit RBuf(const Bytes& v) : b(v) {}
  uint8_t u8() {
    if (p >= b.size()) fail("recon record truncated");
    return b[p++];
  }
  uint64_t var() {
    uint64_t v = 0;
    for (int sh = 0;; sh += 7) {
      uint8_t c = u8();
      if (sh > 63) fail("bad varint in recon record");
      v |= (uint64_t)(c & 0x7F) << sh;
      if (!(c & 0x80)) return v;
    }
  }
  size_t len() {
    uint64_t v = var();
    if (v > b.size() - p) fail("recon record truncated");
    return (size_t)v;
  }
  void bytes(size_t n, Bytes& out) {
    if (n > b.size() - p) fail("recon record truncated");
    out.assign(b.begin() + p, b.begin() + p + n);
    p += n;
  }
  void appendTo(size_t n, Bytes& out) {
    if (n > b.size() - p) fail("recon record truncated");
    out.insert(out.end(), b.begin() + p, b.begin() + p + n);
    p += n;
  }
};
static inline void putLenBytesTo(Bytes& r, const uint8_t* p, size_t n) {
  putVarTo(r, n);
  r.insert(r.end(), p, p + n);
}

// ---------------------------------------------------------------------------------------------
// Flate via preflate.inc. pdfdump owns the zlib wrapper, preflate owns the raw deflate stream.

struct SpanIn : public ::InputStream {
  const uint8_t* p;
  size_t n, pos = 0;
  SpanIn(const uint8_t* p_, size_t n_) : p(p_), n(n_) {}
  bool eof() const override { return pos >= n; }
  size_t read(byte* buf, const size_t sz) override {
    size_t k = std::min(sz, n - pos);
    if (k) memcpy(buf, p + pos, k);
    pos += k;
    return k;
  }
};
// Largest decoded stream the encoder will hold (decompression-bomb guard). Bigger streams keep
// their filter layer.
static const size_t kMaxDecoded = (size_t)1 << 30;

struct VecOut : public ::OutputStream {
  Bytes& v;
  size_t limit;
  explicit VecOut(Bytes& v_, size_t lim = SIZE_MAX) : v(v_), limit(lim) {}
  size_t write(const byte* buf, const size_t sz) override {
    if (sz > limit - std::min(limit, v.size())) throw Error("decoded stream exceeds size limit");
    v.insert(v.end(), buf, buf + sz);
    return sz;
  }
};

static uint32_t adler32(const uint8_t* p, size_t n) {
  uint32_t a = 1, b = 0;
  while (n) {
    size_t k = std::min<size_t>(n, 5552);
    n -= k;
    while (k--) { a += *p++; b += a; }
    a %= 65521;
    b %= 65521;
  }
  return (b << 16) | a;
}

static bool zlibHeaderValid(uint8_t cmf, uint8_t flg) {
  return (cmf & 0x0F) == 8 && (cmf >> 4) <= 7 && ((cmf << 8) | flg) % 31 == 0 && !(flg & 0x20);
}

static const size_t kMetaBlockSize = (size_t)1 << 26;

// encoder-side timing (seconds) and volume, reported by the info mode
struct FlateProf {
  double dec = 0, enc = 0;
  uint64_t unpacked = 0, streams = 0;
};
static FlateProf gFlateProf;
static double nowSec() {
  return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

static bool flateDecode(const CodecCtx& ctx, const Bytes& in, Bytes& out, Bytes& recon) {
  size_t off;
  uint8_t mode;
  if (in.size() >= 2 && zlibHeaderValid(in[0], in[1])) {
    off = 2;
    mode = (in[0] == ctx.zlibHdr[0] && in[1] == ctx.zlibHdr[1]) ? 0 : 1;
  } else {
    off = 0;
    mode = 2;  // raw deflate, no wrapper
  }
  if (in.size() <= off) return false;
  out.clear();
  std::vector<byte> diff;
  uint64_t dsize = 0;
  bool terminated = false;  // synthetic final block appended (stream ends after a sync flush)
  double t0 = nowSec();
  struct ProfGuard {
    double t0;
    Bytes& out;
    ~ProfGuard() { gFlateProf.dec += nowSec() - t0; gFlateProf.unpacked += out.size(); gFlateProf.streams++; }
  } guard{t0, out};
  {
    SpanIn si(in.data() + off, in.size() - off);
    VecOut vo(out, kMaxDecoded);
    if (!preflate_decode(vo, diff, dsize, si, [] {}, 0, kMetaBlockSize)) {
      // Unterminated deflate (e.g. Acrobat): ends with an empty stored block 00 00 FF FF and no
      // final block. Append an empty final static block (03 00) at the flush point and retry.
      size_t c = in.size();
      while (c > off && (in[c - 1] == '\r' || in[c - 1] == '\n') && in.size() - c < 2) c--;
      if (c < off + 4 || memcmp(in.data() + c - 4, "\x00\x00\xFF\xFF", 4) != 0) return false;
      Bytes buf(in.begin() + off, in.begin() + c);
      buf.push_back(0x03);
      buf.push_back(0x00);
      SpanIn s2(buf.data(), buf.size());
      out.clear();
      diff.clear();
      VecOut v2(out, kMaxDecoded);
      if (!preflate_decode(v2, diff, dsize, s2, [] {}, 0, kMetaBlockSize) || dsize != buf.size()) return false;
      Bytes re;
      VecOut ro(re);
      SpanIn ui(out.data(), out.size());
      if (!preflate_reencode(ro, diff, ui, out.size(), [] {}) || re != buf) return false;
      dsize = c - off;
      terminated = true;
    }
  }
  if (dsize == 0 || dsize > in.size() - off) return false;
  if (!terminated && ctx.verify) {
    Bytes re;
    VecOut ro(re);
    double t1 = nowSec();
    SpanIn ui(out.data(), out.size());  // not the vector overload: its MemStream memcpy()s from null for empty data
    bool rok = preflate_reencode(ro, diff, ui, out.size(), [] {});
    gFlateProf.enc += nowSec() - t1;
    gFlateProf.dec -= nowSec() - t1;
    if (!rok) return false;
    if (re.size() != dsize || memcmp(re.data(), in.data() + off, dsize) != 0) return false;
  }
  recon.clear();
  recon.push_back(mode);
  if (mode == 1) { recon.push_back(in[0]); recon.push_back(in[1]); }
  putLenBytesTo(recon, diff.data(), diff.size());
  const uint8_t* tail = in.data() + off + dsize;
  size_t tn = in.size() - off - dsize;
  if (terminated) {
    recon.push_back(2);  // strip the synthetic final block, then explicit tail
    putLenBytesTo(recon, tail, tn);
    return true;
  }
  if (mode != 2 && tn >= 4 &&
      (((uint32_t)tail[0] << 24) | ((uint32_t)tail[1] << 16) | ((uint32_t)tail[2] << 8) | tail[3]) ==
          adler32(out.data(), out.size())) {
    recon.push_back(0);  // derived Adler-32, then extra bytes
    putLenBytesTo(recon, tail + 4, tn - 4);
  } else {
    recon.push_back(1);  // explicit tail
    putLenBytesTo(recon, tail, tn);
  }
  return true;
}

static bool flateEncode(const CodecCtx& ctx, const Bytes& recon, const Bytes& in, Bytes& out) {
  RBuf r(recon);
  uint8_t mode = r.u8();
  out.clear();
  if (mode == 0) { out.push_back(ctx.zlibHdr[0]); out.push_back(ctx.zlibHdr[1]); }
  else if (mode == 1) { out.push_back(r.u8()); out.push_back(r.u8()); }
  else if (mode != 2) fail("bad flate recon");
  Bytes diffB;
  r.bytes(r.len(), diffB);
  std::vector<byte> diff(diffB.begin(), diffB.end());
  VecOut vo(out);
  SpanIn ui(in.data(), in.size());
  if (!preflate_reencode(vo, diff, ui, in.size(), [] {})) return false;
  uint8_t tmode = r.u8();
  if (tmode == 2) {
    if (out.size() < 2) return false;
    out.resize(out.size() - 2);
  } else if (tmode == 0) {
    uint32_t a = adler32(in.data(), in.size());
    out.push_back((uint8_t)(a >> 24)); out.push_back((uint8_t)(a >> 16));
    out.push_back((uint8_t)(a >> 8)); out.push_back((uint8_t)a);
  } else if (tmode != 1) {
    fail("bad flate recon tail");
  }
  r.appendTo(r.len(), out);
  return true;
}

// ---------------------------------------------------------------------------------------------
// PNG predictors (Predictor >= 10): per-row tag byte + filtered row.

static inline uint8_t paeth(int a, int b, int c) {
  int p = a + b - c, pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);
  if (pa <= pb && pa <= pc) return (uint8_t)a;
  if (pb <= pc) return (uint8_t)b;
  return (uint8_t)c;
}

static void pngFilterRow(int tag, const uint8_t* cur, const uint8_t* prev, size_t n, size_t bpp, uint8_t* dst) {
  for (size_t i = 0; i < n; i++) {
    int a = i >= bpp ? cur[i - bpp] : 0, b = prev[i], c = i >= bpp ? prev[i - bpp] : 0;
    uint8_t pr = 0;
    switch (tag) {
      case 1: pr = (uint8_t)a; break;
      case 2: pr = (uint8_t)b; break;
      case 3: pr = (uint8_t)((a + b) >> 1); break;
      case 4: pr = paeth(a, b, c); break;
    }
    dst[i] = (uint8_t)(cur[i] - pr);
  }
}

static bool pngGeometry(int colors, int bpc, int columns, size_t& bpp, size_t& rowlen) {
  if (colors < 1 || colors > 256 || columns < 1 || columns > (1 << 28)) return false;
  if (bpc != 1 && bpc != 2 && bpc != 4 && bpc != 8 && bpc != 16) return false;
  bpp = std::max<size_t>(1, ((size_t)colors * bpc + 7) / 8);
  rowlen = ((size_t)colors * bpc * columns + 7) / 8;
  return true;
}

static int pngHeuristicTag(const uint8_t* cur, const uint8_t* prev, size_t n, size_t bpp, Bytes& tmp) {
  tmp.resize(n);
  int best = 0;
  uint64_t bestSum = UINT64_MAX;
  for (int t = 0; t < 5; t++) {
    pngFilterRow(t, cur, prev, n, bpp, tmp.data());
    uint64_t s = 0;
    for (size_t i = 0; i < n; i++) s += tmp[i] < 128 ? tmp[i] : 256 - tmp[i];
    if (s < bestSum) { bestSum = s; best = t; }
  }
  return best;
}

static bool pngDecode(const FilterParams& fp, const Bytes& in, Bytes& out, Bytes& recon) {
  size_t bpp, rowlen;
  if (!pngGeometry(fp.colors, fp.bpc, fp.columns, bpp, rowlen)) return false;
  size_t nrows = in.size() / (rowlen + 1);
  if (nrows == 0) return false;
  out.assign(nrows * rowlen, 0);
  Bytes tags(nrows), zero(rowlen, 0);
  for (size_t r = 0; r < nrows; r++) {
    const uint8_t* src = in.data() + r * (rowlen + 1);
    uint8_t tag = src[0];
    if (tag > 4) return false;
    tags[r] = tag;
    uint8_t* cur = out.data() + r * rowlen;
    const uint8_t* prev = r ? cur - rowlen : zero.data();
    src++;
    for (size_t i = 0; i < rowlen; i++) {
      int a = i >= bpp ? cur[i - bpp] : 0, b = prev[i], c = i >= bpp ? prev[i - bpp] : 0;
      uint8_t pr = 0;
      switch (tag) {
        case 1: pr = (uint8_t)a; break;
        case 2: pr = (uint8_t)b; break;
        case 3: pr = (uint8_t)((a + b) >> 1); break;
        case 4: pr = paeth(a, b, c); break;
      }
      cur[i] = (uint8_t)(src[i] + pr);
    }
  }
  recon.clear();
  putVarTo(recon, fp.colors);
  putVarTo(recon, fp.bpc);
  putVarTo(recon, fp.columns);
  bool constant = true;
  for (size_t r = 1; r < nrows; r++) constant &= tags[r] == tags[0];
  if (constant) {
    recon.push_back(0);
    recon.push_back(tags[0]);
  } else {
    bool heur = true;
    Bytes tmp;
    for (size_t r = 0; r < nrows && heur; r++) {
      const uint8_t* cur = out.data() + r * rowlen;
      heur = pngHeuristicTag(cur, r ? cur - rowlen : zero.data(), rowlen, bpp, tmp) == tags[r];
    }
    if (heur) {
      recon.push_back(1);
    } else {
      recon.push_back(2);
      putLenBytesTo(recon, tags.data(), tags.size());
    }
  }
  putLenBytesTo(recon, in.data() + nrows * (rowlen + 1), in.size() - nrows * (rowlen + 1));
  return true;
}

static bool pngEncode(const Bytes& recon, const Bytes& in, Bytes& out) {
  RBuf r(recon);
  int colors = (int)r.var(), bpc = (int)r.var(), columns = (int)r.var();
  size_t bpp, rowlen;
  if (!pngGeometry(colors, bpc, columns, bpp, rowlen)) fail("bad PNG recon");
  if (in.size() % rowlen) return false;
  size_t nrows = in.size() / rowlen;
  uint8_t mode = r.u8();
  Bytes tags;
  if (mode == 0) tags.assign(nrows, r.u8());
  else if (mode == 2) { r.bytes(r.len(), tags); if (tags.size() != nrows) return false; }
  else if (mode != 1) fail("bad PNG recon mode");
  out.assign(nrows * (rowlen + 1), 0);
  Bytes zero(rowlen, 0), tmp;
  for (size_t i = 0; i < nrows; i++) {
    const uint8_t* cur = in.data() + i * rowlen;
    const uint8_t* prev = i ? cur - rowlen : zero.data();
    int tag = mode == 1 ? pngHeuristicTag(cur, prev, rowlen, bpp, tmp) : tags[i];
    uint8_t* dst = out.data() + i * (rowlen + 1);
    dst[0] = (uint8_t)tag;
    pngFilterRow(tag, cur, prev, rowlen, bpp, dst + 1);
  }
  r.appendTo(r.len(), out);
  return true;
}

// TIFF predictor 2, 8 and 16 bits per component.
static bool tiffDecode(const FilterParams& fp, const Bytes& in, Bytes& out, Bytes& recon) {
  size_t bpp, rowlen;
  if (!pngGeometry(fp.colors, fp.bpc, fp.columns, bpp, rowlen) || (fp.bpc != 8 && fp.bpc != 16)) return false;
  size_t nrows = in.size() / rowlen;
  if (nrows == 0) return false;
  out.assign(in.begin(), in.begin() + nrows * rowlen);
  for (size_t r = 0; r < nrows; r++) {
    uint8_t* row = out.data() + r * rowlen;
    if (fp.bpc == 8) {
      for (size_t i = fp.colors; i < rowlen; i++) row[i] = (uint8_t)(row[i] + row[i - fp.colors]);
    } else {
      size_t st = 2 * fp.colors;
      for (size_t i = st; i + 1 < rowlen; i += 2) {
        unsigned v = ((row[i] << 8) | row[i + 1]) + ((row[i - st] << 8) | row[i - st + 1]);
        row[i] = (uint8_t)(v >> 8);
        row[i + 1] = (uint8_t)v;
      }
    }
  }
  recon.clear();
  putVarTo(recon, fp.colors);
  putVarTo(recon, fp.bpc);
  putVarTo(recon, fp.columns);
  putLenBytesTo(recon, in.data() + nrows * rowlen, in.size() - nrows * rowlen);
  return true;
}

static bool tiffEncode(const Bytes& recon, const Bytes& in, Bytes& out) {
  RBuf r(recon);
  int colors = (int)r.var(), bpc = (int)r.var(), columns = (int)r.var();
  size_t bpp, rowlen;
  if (!pngGeometry(colors, bpc, columns, bpp, rowlen) || (bpc != 8 && bpc != 16)) fail("bad TIFF recon");
  if (in.size() % rowlen) return false;
  size_t nrows = in.size() / rowlen;
  out = in;
  for (size_t rr = 0; rr < nrows; rr++) {
    const uint8_t* src = in.data() + rr * rowlen;
    uint8_t* row = out.data() + rr * rowlen;
    if (bpc == 8) {
      for (size_t i = colors; i < rowlen; i++) row[i] = (uint8_t)(src[i] - src[i - colors]);
    } else {
      size_t st = 2 * colors;
      for (size_t i = st; i + 1 < rowlen; i += 2) {
        unsigned v = ((src[i] << 8) | src[i + 1]) - ((src[i - st] << 8) | src[i - st + 1]);
        row[i] = (uint8_t)(v >> 8);
        row[i + 1] = (uint8_t)v;
      }
    }
  }
  r.appendTo(r.len(), out);
  return true;
}

// ---------------------------------------------------------------------------------------------
// ASCIIHexDecode. Canonical writer: hex pairs, a line break of fixed form every W digits,
// optional EOL before '>', '>' terminator, verbatim tail.

static void ahxWrite(const Bytes& in, bool lower, size_t W, const Bytes& eol, bool odd, bool eolBeforeGT, bool gt,
                     Bytes& out) {
  static const char* U = "0123456789ABCDEF";
  static const char* L = "0123456789abcdef";
  const char* hx = lower ? L : U;
  out.clear();
  size_t digits = in.size() * 2 - (odd && !in.empty() ? 1 : 0), col = 0, k = 0;
  for (size_t i = 0; i < in.size(); i++) {
    for (int h = 0; h < 2; h++) {
      if (k == digits) break;
      if (W && col == W) { out.insert(out.end(), eol.begin(), eol.end()); col = 0; }
      out.push_back((uint8_t)hx[h ? (in[i] & 15) : (in[i] >> 4)]);
      col++;
      k++;
    }
  }
  if (eolBeforeGT) out.insert(out.end(), eol.begin(), eol.end());
  if (gt) out.push_back('>');
}

static bool ahxDecode(const Bytes& in, Bytes& out, Bytes& recon) {
  out.clear();
  int hi = -1;
  size_t i = 0, ndig = 0, lowerCnt = 0, upperCnt = 0;
  bool gt = false;
  for (; i < in.size(); i++) {
    uint8_t c = in[i];
    if (c == '>') { gt = true; i++; break; }
    if (isWS(c)) continue;
    int v = hexVal(c);
    if (v < 0) return false;
    if (c >= 'a' && c <= 'f') lowerCnt++;
    if (c >= 'A' && c <= 'F') upperCnt++;
    ndig++;
    if (hi < 0) hi = v;
    else { out.push_back((uint8_t)(hi * 16 + v)); hi = -1; }
  }
  bool odd = hi >= 0;
  if (odd) out.push_back((uint8_t)(hi << 4));
  size_t bodyEnd = gt ? i - 1 : i;  // position of '>' or end
  // infer line width and EOL from the first break
  size_t W = 0;
  Bytes eol;
  size_t j = 0, col = 0;
  for (; j < bodyEnd; j++) {
    if (isWS(in[j])) {
      size_t e = j;
      while (e < bodyEnd && isWS(in[e])) e++;
      eol.assign(in.begin() + j, in.begin() + e);
      W = col;
      break;
    }
    col++;
  }
  bool eolBeforeGT = false;
  if (!eol.empty() && bodyEnd >= eol.size() && memcmp(in.data() + bodyEnd - eol.size(), eol.data(), eol.size()) == 0) {
    // a break right before '>' (the second attempt below tries the opposite choice)
    size_t q = bodyEnd - eol.size();
    if (q == 0 || !isWS(in[q - 1])) eolBeforeGT = true;
  }
  if (W >= ndig) W = 0;
  bool lower = lowerCnt > upperCnt;
  Bytes tail(in.begin() + i, in.end());
  for (int attempt = 0; attempt < 2; attempt++) {
    bool ebg = attempt == 0 ? eolBeforeGT : !eolBeforeGT;
    if (ebg && eol.empty()) continue;
    Bytes re;
    ahxWrite(out, lower, W, eol, odd, ebg, gt, re);
    re.insert(re.end(), tail.begin(), tail.end());
    if (re == in) {
      recon.clear();
      recon.push_back((uint8_t)((lower ? 1 : 0) | (gt ? 2 : 0) | (odd ? 4 : 0) | (ebg ? 8 : 0)));
      putVarTo(recon, W);
      putLenBytesTo(recon, eol.data(), eol.size());
      putLenBytesTo(recon, tail.data(), tail.size());
      return true;
    }
  }
  return false;
}

static bool ahxEncode(const Bytes& recon, const Bytes& in, Bytes& out) {
  RBuf r(recon);
  uint8_t f = r.u8();
  size_t W = (size_t)r.var();
  Bytes eol;
  r.bytes(r.len(), eol);
  ahxWrite(in, f & 1, W, eol, (f & 4) != 0, (f & 8) != 0, (f & 2) != 0, out);
  r.appendTo(r.len(), out);
  return true;
}

// ---------------------------------------------------------------------------------------------
// ASCII85Decode. Canonical writer: optional "<~", groups with optional 'z', a fixed line break
// every W characters, optional EOL before "~>", terminator, verbatim tail.

static void a85Write(const Bytes& in, bool prefix, bool useZ, size_t W, const Bytes& eol, bool eolBeforeEnd, bool end,
                     Bytes& out) {
  out.clear();
  if (prefix) { out.push_back('<'); out.push_back('~'); }
  size_t col = 0;
  auto emit = [&](uint8_t c) {
    if (W && col == W) { out.insert(out.end(), eol.begin(), eol.end()); col = 0; }
    out.push_back(c);
    col++;
  };
  size_t i = 0;
  for (; i + 4 <= in.size(); i += 4) {
    uint32_t v = ((uint32_t)in[i] << 24) | ((uint32_t)in[i + 1] << 16) | ((uint32_t)in[i + 2] << 8) | in[i + 3];
    if (v == 0 && useZ) { emit('z'); continue; }
    char c[5];
    for (int k = 4; k >= 0; k--) { c[k] = (char)('!' + v % 85); v /= 85; }
    for (int k = 0; k < 5; k++) emit((uint8_t)c[k]);
  }
  size_t rem = in.size() - i;
  if (rem) {
    uint32_t v = 0;
    for (size_t k = 0; k < 4; k++) v = (v << 8) | (k < rem ? in[i + k] : 0);
    char c[5];
    for (int k = 4; k >= 0; k--) { c[k] = (char)('!' + v % 85); v /= 85; }
    for (size_t k = 0; k <= rem; k++) emit((uint8_t)c[k]);
  }
  if (eolBeforeEnd) out.insert(out.end(), eol.begin(), eol.end());
  if (end) { out.push_back('~'); out.push_back('>'); }
}

static bool a85Decode(const Bytes& in, Bytes& out, Bytes& recon) {
  out.clear();
  size_t i = 0;
  bool prefix = in.size() >= 2 && in[0] == '<' && in[1] == '~';
  if (prefix) i = 2;
  uint32_t grp[5];
  int g = 0;
  bool end = false, sawZ = false, sawZeroGroup = false;
  size_t bodyStart = i;
  for (; i < in.size(); i++) {
    uint8_t c = in[i];
    if (isWS(c)) continue;
    if (c == '~') {
      if (i + 1 < in.size() && in[i + 1] == '>') { end = true; i += 2; break; }
      return false;
    }
    if (c == 'z') {
      if (g) return false;
      sawZ = true;
      for (int k = 0; k < 4; k++) out.push_back(0);
      continue;
    }
    if (c < '!' || c > 'u') return false;
    grp[g++] = c - '!';
    if (g == 5) {
      uint64_t v = 0;
      for (int k = 0; k < 5; k++) v = v * 85 + grp[k];
      if (v > 0xFFFFFFFFull) return false;
      if (v == 0) sawZeroGroup = true;
      for (int k = 3; k >= 0; k--) out.push_back((uint8_t)(v >> (8 * k)));
      g = 0;
    }
  }
  if (g == 1) return false;
  if (g) {
    for (int k = g; k < 5; k++) grp[k] = 84;
    uint64_t v = 0;
    for (int k = 0; k < 5; k++) v = v * 85 + grp[k];
    if (v > 0xFFFFFFFFull) return false;
    for (int k = 0; k < g - 1; k++) out.push_back((uint8_t)(v >> (8 * (3 - k))));
  }
  if (sawZ && sawZeroGroup) return false;
  size_t bodyEnd = end ? i - 2 : i;
  size_t W = 0, col = 0;
  Bytes eol;
  for (size_t j = bodyStart; j < bodyEnd; j++) {
    if (isWS(in[j])) {
      size_t e = j;
      while (e < bodyEnd && isWS(in[e])) e++;
      eol.assign(in.begin() + j, in.begin() + e);
      W = col;
      break;
    }
    col++;
  }
  Bytes tail(in.begin() + i, in.end());
  for (int attempt = 0; attempt < 4; attempt++) {
    bool ebe = (attempt & 1) != 0;
    size_t w = (attempt & 2) ? 0 : W;
    if (ebe && eol.empty()) continue;
    Bytes re;
    a85Write(out, prefix, sawZ, w, eol, ebe, end, re);
    re.insert(re.end(), tail.begin(), tail.end());
    if (re == in) {
      recon.clear();
      recon.push_back((uint8_t)((prefix ? 1 : 0) | (sawZ ? 2 : 0) | (ebe ? 4 : 0) | (end ? 8 : 0)));
      putVarTo(recon, w);
      putLenBytesTo(recon, eol.data(), eol.size());
      putLenBytesTo(recon, tail.data(), tail.size());
      return true;
    }
  }
  return false;
}

static bool a85Encode(const Bytes& recon, const Bytes& in, Bytes& out) {
  RBuf r(recon);
  uint8_t f = r.u8();
  size_t W = (size_t)r.var();
  Bytes eol;
  r.bytes(r.len(), eol);
  a85Write(in, f & 1, (f & 2) != 0, W, eol, (f & 4) != 0, (f & 8) != 0, out);
  r.appendTo(r.len(), out);
  return true;
}

// ---------------------------------------------------------------------------------------------
// RunLengthDecode. Packet boundaries are the only freedom: the recon record is either a greedy
// encoder variant id or the explicit list of packet header bytes.

// Packet headers produced by a greedy encoder; minRun = shortest run coded as a repeat packet.
static void rlGreedyHeaders(const Bytes& d, int minRun, Bytes& hdrs) {
  hdrs.clear();
  size_t i = 0, n = d.size();
  while (i < n) {
    size_t run = 1;
    while (i + run < n && run < 128 && d[i + run] == d[i]) run++;
    if ((int)run >= minRun) {
      hdrs.push_back((uint8_t)(257 - run));
      i += run;
      continue;
    }
    size_t st = i;
    while (i < n && i - st < 128) {
      size_t r2 = 1;
      while (i + r2 < n && r2 < (size_t)minRun && d[i + r2] == d[i]) r2++;
      if ((int)r2 >= minRun) break;
      i++;
    }
    hdrs.push_back((uint8_t)(i - st - 1));
  }
}

static bool rlDecode(const Bytes& in, Bytes& out, Bytes& recon) {
  out.clear();
  Bytes hdrs;
  size_t i = 0;
  bool eod = false;
  while (i < in.size()) {
    uint8_t h = in[i];
    if (h == 128) { eod = true; i++; break; }
    if (h < 128) {
      if (i + 1 + h + 1 > in.size()) return false;
      out.insert(out.end(), in.begin() + i + 1, in.begin() + i + 2 + h);
      i += 2 + h;
    } else {
      if (i + 2 > in.size() || out.size() > kMaxDecoded) return false;
      out.insert(out.end(), (size_t)(257 - h), in[i + 1]);
      i += 2;
    }
    hdrs.push_back(h);
  }
  recon.clear();
  Bytes g;
  int variant = 0;
  for (int mr = 2; mr <= 3 && !variant; mr++) {
    rlGreedyHeaders(out, mr, g);
    if (g == hdrs) variant = mr;
  }
  recon.push_back((uint8_t)variant);
  if (!variant) putLenBytesTo(recon, hdrs.data(), hdrs.size());
  recon.push_back(eod ? 1 : 0);
  putLenBytesTo(recon, in.data() + i, in.size() - i);
  return true;
}

static bool rlEncode(const Bytes& recon, const Bytes& in, Bytes& out) {
  RBuf r(recon);
  uint8_t variant = r.u8();
  Bytes hdrs;
  if (variant) rlGreedyHeaders(in, variant, hdrs);
  else r.bytes(r.len(), hdrs);
  out.clear();
  size_t p = 0;
  for (uint8_t h : hdrs) {
    out.push_back(h);
    if (h < 128) {
      if (p + h + 1 > in.size()) return false;
      out.insert(out.end(), in.begin() + p, in.begin() + p + h + 1);
      p += h + 1;
    } else if (h > 128) {
      if (p >= in.size()) return false;
      out.push_back(in[p]);
      p += 257 - h;
    } else {
      return false;
    }
  }
  if (p != in.size()) return false;
  if (r.u8()) out.push_back(128);
  r.appendTo(r.len(), out);
  return true;
}

// ---------------------------------------------------------------------------------------------
// LZWDecode. Greedy encoder; the recon record carries EarlyChange, the decoded offsets at which
// Clear codes occur, EOD presence, the padding bits and the tail.

static bool lzwDecode(const FilterParams& fp, const Bytes& in, Bytes& out, Bytes& recon) {
  int early = fp.earlyChange ? 1 : 0;
  out.clear();
  std::vector<uint32_t> estart(4096), elen(4096);
  std::vector<uint64_t> clears;
  size_t bitpos = 0, nbits = in.size() * 8;
  int width = 9, next = 258, prev = -1;
  bool eod = false;
  auto rd = [&](int w) -> int {
    int v = 0;
    for (int k = 0; k < w; k++) {
      v = (v << 1) | ((in[bitpos >> 3] >> (7 - (bitpos & 7))) & 1);
      bitpos++;
    }
    return v;
  };
  size_t prevLen = 0;
  for (;;) {
    if (nbits - bitpos < (size_t)width) break;
    int code = rd(width);
    if (code == 256) {
      clears.push_back(out.size());
      width = 9;
      next = 258;
      prev = -1;
      continue;
    }
    if (code == 257) { eod = true; break; }
    size_t st = out.size();
    if (st > kMaxDecoded) return false;
    if (prev < 0) {
      if (code > 255) return false;
      out.push_back((uint8_t)code);
      prevLen = 1;
      prev = code;
      continue;
    }
    size_t curLen;
    if (code < 256) {
      out.push_back((uint8_t)code);
      curLen = 1;
    } else if (code < next) {
      size_t s0 = estart[code];
      curLen = elen[code];
      for (size_t k = 0; k < curLen; k++) { uint8_t c = out[s0 + k]; out.push_back(c); }
    } else if (code == next && next < 4096) {
      size_t s0 = st - prevLen;
      for (size_t k = 0; k < prevLen; k++) { uint8_t c = out[s0 + k]; out.push_back(c); }
      uint8_t c = out[s0];
      out.push_back(c);
      curLen = prevLen + 1;
    } else {
      return false;
    }
    if (next < 4096) {
      estart[next] = (uint32_t)(st - prevLen);
      elen[next] = (uint32_t)(prevLen + 1);
      next++;
      if (next + early >= (1 << width) && width < 12) width++;
    }
    prevLen = curLen;
    prev = code;
  }
  size_t padCount = (8 - (bitpos & 7)) & 7;
  int padVal = padCount ? rd((int)padCount) : 0;
  recon.clear();
  recon.push_back((uint8_t)early);
  putVarTo(recon, clears.size());
  uint64_t last = 0;
  for (uint64_t c : clears) { putVarTo(recon, c - last); last = c; }
  recon.push_back(eod ? 1 : 0);
  recon.push_back((uint8_t)padCount);
  recon.push_back((uint8_t)padVal);
  putLenBytesTo(recon, in.data() + bitpos / 8, in.size() - bitpos / 8);
  return true;
}

static bool lzwEncode(const Bytes& recon, const Bytes& in, Bytes& out) {
  RBuf r(recon);
  int early = r.u8() ? 1 : 0;
  size_t nc = r.len();
  std::vector<uint64_t> clears(nc);
  uint64_t acc = 0;
  for (size_t k = 0; k < nc; k++) { acc += r.var(); clears[k] = acc; }
  bool eod = r.u8() != 0;
  int padCount = r.u8(), padVal = r.u8();
  out.clear();
  uint64_t bitbuf = 0;
  int bitcnt = 0;
  auto put = [&](uint32_t v, int w) {
    bitbuf = (bitbuf << w) | v;
    bitcnt += w;
    while (bitcnt >= 8) { out.push_back((uint8_t)(bitbuf >> (bitcnt - 8))); bitcnt -= 8; }
  };
  std::vector<uint16_t> child((size_t)4096 * 256, 0);
  std::vector<uint32_t> used;
  int encNext = 258, decNext = 258, codesSinceClear = 0;
  auto width = [&]() {
    int w = 9;
    while (w < 12 && decNext + early >= (1 << w)) w++;
    return w;
  };
  auto reset = [&]() {
    for (uint32_t u : used) child[u] = 0;
    used.clear();
    encNext = decNext = 258;
    codesSinceClear = 0;
  };
  auto emitCode = [&](int code) {
    put((uint32_t)code, width());
    if (codesSinceClear > 0 && decNext < 4096) decNext++;
    codesSinceClear++;
  };
  size_t ci = 0, i = 0, n = in.size();
  while (i < n) {
    while (ci < clears.size() && clears[ci] == i) {
      put(256, width());
      reset();
      ci++;
    }
    if (ci < clears.size() && clears[ci] < i) return false;
    size_t limit = ci < clears.size() ? (size_t)clears[ci] : n;
    int code = in[i];
    size_t j = i + 1;
    while (j < limit) {
      uint16_t c = child[(size_t)code * 256 + in[j]];
      if (!c) break;
      code = c;
      j++;
    }
    emitCode(code);
    if (j < limit && encNext < 4096) {
      size_t idx = (size_t)code * 256 + in[j];
      child[idx] = (uint16_t)encNext++;
      used.push_back((uint32_t)idx);
    }
    i = j;
  }
  while (ci < clears.size() && clears[ci] == n) {
    put(256, width());
    reset();
    ci++;
  }
  if (ci != clears.size()) return false;
  if (eod) put(257, width());
  if (padCount) put((uint32_t)padVal, padCount);
  if (bitcnt) return false;
  r.appendTo(r.len(), out);
  return true;
}

// LZW encoding with an initial Clear and EOD (used only by the self-test).
static bool lzwTestEncode(const Bytes& in, Bytes& out, int early) {
  for (int pc = 0; pc < 8; pc++) {
    Bytes r;
    r.push_back((uint8_t)early);
    putVarTo(r, 1);
    putVarTo(r, 0);
    r.push_back(1);
    r.push_back((uint8_t)pc);
    r.push_back(0);
    putVarTo(r, 0);
    if (lzwEncode(r, in, out)) return true;
  }
  return false;
}

// ---------------------------------------------------------------------------------------------

static bool bmpEncodeLayer(const Bytes& recon, const Bytes& in, Bytes& out);  // pdf_bmp.inc
static bool t1EncodeLayer(const Bytes& recon, const Bytes& in, Bytes& out);   // pdf_t1.inc
static bool t1DecodeLayer(const Bytes& in, Bytes& out, Bytes& recon, bool charstrings);
static bool objstmEncodeLayer(const Bytes& recon, const Bytes& in, Bytes& out);  // pdf_objstm.inc
static bool objstmDecodeLayer(const Bytes& in, int64_t N, int64_t First, Bytes& out, Bytes& recon);
static bool xrefRowsEncodeLayer(const CodecCtx& ctx, const Bytes& recon, const Bytes& in, Bytes& out);
static bool xrefRowsDecodeLayer(const CodecCtx& ctx, const Bytes& in, const std::vector<uint64_t>& W,
                                const std::vector<std::pair<uint64_t, uint64_t>>& index, Bytes& out, Bytes& recon);

static bool layerEncode(const CodecCtx& ctx, uint8_t type, const Bytes& recon, const Bytes& in, Bytes& out) {
  switch (type) {
    case L_FLATE: return flateEncode(ctx, recon, in, out);
    case L_LZW: return lzwEncode(recon, in, out);
    case L_AHX: return ahxEncode(recon, in, out);
    case L_A85: return a85Encode(recon, in, out);
    case L_RL: return rlEncode(recon, in, out);
    case L_PNG: return pngEncode(recon, in, out);
    case L_TIFF: return tiffEncode(recon, in, out);
    case L_BMP: return bmpEncodeLayer(recon, in, out);
    case L_T1: return t1EncodeLayer(recon, in, out);
    case L_OBJSTM: return objstmEncodeLayer(recon, in, out);
    case L_XREFROWS: return xrefRowsEncodeLayer(ctx, recon, in, out);
  }
  fail("unknown layer type %d", type);
}

// Encoder side. Each decoder is responsible for exactness; this wrapper re-checks through
// layerEncode (the decoder's own path) so no layer can be committed unverified.
static bool layerDecode(const CodecCtx& ctx, uint8_t type, const FilterParams& fp, const Bytes& in, Bytes& out,
                        Bytes& recon) {
  bool ok = false;
  try {
    switch (type) {
      case L_FLATE: ok = flateDecode(ctx, in, out, recon); break;  // verified internally
      case L_LZW: ok = lzwDecode(fp, in, out, recon); break;
      case L_AHX: ok = ahxDecode(in, out, recon); break;
      case L_A85: ok = a85Decode(in, out, recon); break;
      case L_RL: ok = rlDecode(in, out, recon); break;
      case L_PNG: ok = pngDecode(fp, in, out, recon); break;
      case L_TIFF: ok = tiffDecode(fp, in, out, recon); break;
      case L_T1: ok = t1DecodeLayer(in, out, recon, fp.t1Charstrings); break;
      case L_OBJSTM: ok = objstmDecodeLayer(in, fp.objstmN, fp.objstmFirst, out, recon); break;
      case L_XREFROWS: ok = xrefRowsDecodeLayer(ctx, in, fp.xrefW, fp.xrefIndex, out, recon); break;
      default: return false;
    }
    if (!ok) return false;
    if (type == L_FLATE) return true;
    Bytes re;
    return layerEncode(ctx, type, recon, out, re) && re == in;
  } catch (const Error&) {
    return false;
  }
}
