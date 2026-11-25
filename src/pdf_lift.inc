// pdf_lift.inc -- number lifting for PDF text (content streams, object text).
//
// xz models digits as text and cannot see that the numbers after "Td" or inside "/Widths" form
// their own population. The lift transform replaces every canonically spelled number with a
// marker byte and stores the values as varints, grouped by context, in a separate buffer; in
// content-stream mode canonical hex strings move to a binary buffer as well.
//
// The grouping is computed by lexing the *skeleton* (text with markers), which both sides have,
// so encoder and decoder always agree on it; the lexer only has to be good, not right. The
// encoder runs the inverse and compares before using the result.
//
// Skeleton bytes: 0x00 X = literal byte X (X <= 4), 0x01 = number, 0x02 = hex string with
// upper-case (or no) letters, 0x03 = hex string with lower-case letters, 0x04 = inline image data
// whose filters were undone (content-stream mode; record in the inline buffer: u8 layer count,
// per layer u8 type + recon, then the decoded data, lengths as varints).
// Number value: zigzag(mantissa) * 8 + code; code 0 = integer, 1..6 = "I.F" with that many
// fraction digits, 7 = escape followed by varint(scale * 4 + form), form bit0 = no integer part
// (".5"), bit1 = trailing dot ("5."). Numbers file: one mode byte per group (see liftEncodeGroup),
// then the groups' values in group order.

enum LiftMode { LIFT_CS, LIFT_OBJ };

struct LiftNum {
  int64_t m;
  uint32_t code, ext;
};

static inline bool liftCtl(uint8_t c) { return c <= 4; }

// canonical number spelling -> value; false if the token is not lifted
static bool liftParseNum(const uint8_t* p, size_t n, LiftNum& v) {
  size_t i = 0;
  bool neg = false;
  if (i < n && p[i] == '-') { neg = true; i++; }
  size_t is = i;
  while (i < n && p[i] >= '0' && p[i] <= '9') i++;
  size_t ie = i;
  bool dot = false;
  size_t fs = i, fe = i;
  if (i < n && p[i] == '.') {
    dot = true;
    fs = ++i;
    while (i < n && p[i] >= '0' && p[i] <= '9') i++;
    fe = i;
  }
  if (i != n) return false;
  size_t il = ie - is, fl = fe - fs;
  if (il == 0 && fl == 0) return false;
  if (il > 1 && p[is] == '0') return false;  // leading zeros
  if (il + fl > 18) return false;
  int64_t m = 0;
  for (size_t k = is; k < ie; k++) m = m * 10 + (p[k] - '0');
  for (size_t k = fs; k < fe; k++) m = m * 10 + (p[k] - '0');
  if (neg) {
    if (m == 0) return false;  // "-0", "-0.0": sign not representable
    m = -m;
  }
  v.m = m;
  v.ext = 0;
  if (!dot) v.code = 0;
  else if (il > 0 && fl >= 1 && fl <= 6) v.code = (uint32_t)fl;
  else {
    v.code = 7;
    v.ext = (uint32_t)(fl * 4 + (il == 0 ? 1 : 0) + (fl == 0 ? 2 : 0));
  }
  return true;
}

static void liftSpellNum(const LiftNum& v, Bytes& out) {
  uint64_t a = v.m < 0 ? (uint64_t)(-v.m) : (uint64_t)v.m;
  size_t scale = 0;
  bool omitInt = false, trail = false;
  if (v.code >= 1 && v.code <= 6) scale = v.code;
  else if (v.code == 7) {
    scale = v.ext / 4;
    omitInt = v.ext & 1;
    trail = (v.ext & 2) != 0;
  }
  char digits[24];
  int nd = 0;
  do { digits[nd++] = (char)('0' + a % 10); a /= 10; } while (a);
  while ((size_t)nd <= scale) digits[nd++] = '0';  // at least one integer digit
  if (v.m < 0) out.push_back('-');
  int intDigits = nd - (int)scale;
  if (!(omitInt && intDigits == 1 && digits[nd - 1] == '0'))
    for (int k = nd - 1; k >= (int)scale; k--) out.push_back((uint8_t)digits[k]);
  if (scale || trail || v.code != 0) {
    if (v.code != 0) out.push_back('.');
    for (int k = (int)scale - 1; k >= 0; k--) out.push_back((uint8_t)digits[k]);
  }
}

// ---- grouping lexer (runs on the skeleton, identical on both sides) ---------------------------
struct LiftGroups {
  std::vector<uint32_t> ofMarker;  // group id per number marker
  uint32_t count = 0;
};

static void liftGroup(LiftMode mode, const uint8_t* s, size_t n, LiftGroups& g) {
  std::unordered_map<std::string, uint32_t> ids;
  struct Pend { size_t idx; uint8_t slot; };
  std::vector<Pend> pend;
  std::string key;  // OBJ: last name at array depth 0
  int arr = 0, operands = 0;
  size_t markers = 0;
  g.ofMarker.clear();
  auto flush = [&](const uint8_t* t, size_t tl) {
    for (auto& p : pend) {
      std::string k;
      if (mode == LIFT_OBJ) k = key;
      k.push_back((char)p.slot);
      k.append((const char*)t, tl);
      auto it = ids.find(k);
      uint32_t id;
      if (it == ids.end()) { id = (uint32_t)ids.size(); ids.emplace(k, id); }
      else id = it->second;
      if (g.ofMarker.size() <= p.idx) g.ofMarker.resize(p.idx + 1);
      g.ofMarker[p.idx] = id;
    }
    pend.clear();
  };
  auto marker = [&]() {
    uint8_t slot = mode == LIFT_CS ? (uint8_t)(arr ? 7 : std::min(operands, 6)) : (uint8_t)(arr ? 1 : 0);
    pend.push_back({markers++, slot});
    if (!arr) operands++;
  };
  // skips over a region, still accounting for any unescaped number marker inside it
  auto skipTo = [&](size_t& i, size_t end) {
    while (i < end) {
      if (s[i] == 0) { i += 2; continue; }
      if (s[i] == 1) marker();
      i++;
    }
    if (i > end) i = end;
  };
  size_t i = 0;
  while (i < n) {
    uint8_t c = s[i];
    if (c == 0) { i += 2; continue; }
    if (c == 1) { marker(); i++; continue; }
    if (c == 2 || c == 3) { if (!arr) operands++; i++; continue; }
    if (c == 4) { i++; continue; }
    if (isWS(c)) { i++; continue; }
    if (c == '%') {
      size_t j = i;
      while (j < n && s[j] != '\r' && s[j] != '\n') j += s[j] == 0 ? 2 : 1;
      skipTo(i, std::min(j, n));
      continue;
    }
    if (c == '(') {
      size_t j = i + 1;
      int depth = 1;
      while (j < n && depth) {
        if (s[j] == 0) { j += 2; continue; }
        if (s[j] == '\\') { j += 2; continue; }
        if (s[j] == '(') depth++;
        else if (s[j] == ')') depth--;
        j++;
      }
      skipTo(i, std::min(j, n));
      if (!arr) operands++;
      continue;
    }
    if (c == '<') {
      if (i + 1 < n && s[i + 1] == '<') {
        if (mode == LIFT_OBJ) flush(s + i, 2);
        i += 2;
        continue;
      }
      size_t j = i + 1;
      while (j < n && s[j] != '>') j += s[j] == 0 ? 2 : 1;
      skipTo(i, std::min(j + 1, n));
      if (!arr) operands++;
      continue;
    }
    if (c == '>') {
      size_t l = (i + 1 < n && s[i + 1] == '>') ? 2 : 1;
      if (mode == LIFT_OBJ) flush(s + i, l);
      i += l;
      continue;
    }
    if (c == '[') {
      if (!arr) operands++;
      arr++;
      i++;
      continue;
    }
    if (c == ']') {
      if (mode == LIFT_OBJ) flush(s + i, 1);
      arr = std::max(0, arr - 1);
      i++;
      continue;
    }
    if (c == '{' || c == '}' || c == ')') { i++; continue; }
    if (c == '/') {
      size_t j = i + 1;
      while (j < n && isRegular(s[j]) && !liftCtl(s[j])) j++;
      if (mode == LIFT_OBJ && !arr) {
        flush((const uint8_t*)"/", 1);
        key.assign((const char*)s + i, j - i);
      } else if (!arr) {
        operands++;
      }
      i = j;
      continue;
    }
    // regular run
    size_t j = i;
    while (j < n && isRegular(s[j]) && !liftCtl(s[j])) j++;
    bool numeric = (c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.';
    if (numeric) {
      if (!arr) operands++;
    } else {
      flush(s + i, j - i);
      if (mode == LIFT_CS) {
        operands = 0;
        arr = 0;
        if (j - i == 2 && s[i] == 'I' && s[i + 1] == 'D') {
          // lifted inline image data: whitespace, marker, then EI
          size_t w = j < n && s[j] == 0 ? j + 2 : j + 1;
          if (w < n && s[w] == 4) { i = w + 1; continue; }
          // inline image data up to whitespace "EI" (whitespace or end)
          size_t k = j + 1;
          while (k + 2 <= n && !(s[k] == 'E' && s[k + 1] == 'I' && isWS(s[k - 1]) && (k + 2 == n || isWS(s[k + 2]))))
            k += s[k] == 0 ? 2 : 1;
          i = j;
          skipTo(i, std::min(k, n));
          continue;
        }
      }
    }
    i = j;
  }
  flush(nullptr, 0);
  g.ofMarker.resize(markers);
  g.count = (uint32_t)ids.size();
}

// ---- per-group number coding -----------------------------------------------------------------
// Group mode byte: bits 0-1 = 0 varint, 1 varint of delta, 2 fixed width, 3 fixed width of delta;
// bits 2-5 = width in bytes for the fixed modes. A value is zigzag(m or m - prev) * 8 + code,
// followed by varint(ext) when code is 7. The mode is chosen by an order-1 entropy estimate.

static uint64_t liftWord(const LiftNum& v, int64_t prev, bool delta) {
  return zigzag(delta ? v.m - prev : v.m) * 8 + v.code;
}

static void liftEncodeGroup(const std::vector<LiftNum>& v, uint8_t mode, Bytes& out) {
  bool delta = mode & 1, fixed = (mode & 2) != 0;
  int w = mode >> 2;
  int64_t prev = 0;
  for (auto& x : v) {
    uint64_t word = liftWord(x, prev, delta);
    prev = x.m;
    if (fixed)
      for (int k = w - 1; k >= 0; k--) out.push_back((uint8_t)(word >> (8 * k)));
    else
      putVarTo(out, word);
    if (x.code == 7) putVarTo(out, x.ext);
  }
}

struct LiftEntropy {
  std::vector<uint32_t> pair, ctx;
  std::vector<uint32_t> touched;
  LiftEntropy() : pair(65536, 0), ctx(256, 0) {}
  // order-1 entropy estimate of b, in bits
  double bits(const Bytes& b) {
    if (b.size() < 2) return 8.0 * b.size();
    for (size_t i = 1; i < b.size(); i++) {
      uint32_t k = (uint32_t)b[i - 1] << 8 | b[i];
      if (!pair[k]++) touched.push_back(k);
      ctx[b[i - 1]]++;
    }
    double h = 0;
    for (uint32_t k : touched) h -= pair[k] * std::log2((double)pair[k] / ctx[k >> 8]);
    for (uint32_t k : touched) { pair[k] = 0; ctx[k >> 8] = 0; }
    touched.clear();
    return h + 8;
  }
};

static uint8_t liftChooseMode(const std::vector<LiftNum>& v, LiftEntropy& est) {
  if (v.size() < 32) return 0;
  int w[2] = {1, 1};
  for (int d = 0; d < 2; d++) {
    int64_t prev = 0;
    for (auto& x : v) {
      uint64_t word = liftWord(x, prev, d != 0);
      prev = x.m;
      while (w[d] < 8 && (word >> (8 * w[d]))) w[d]++;
    }
  }
  uint8_t best = 0;
  double bestBits = 0;
  for (uint8_t m = 0; m < 4; m++) {
    uint8_t mode = (uint8_t)(m | ((m & 2) ? w[m & 1] << 2 : 0));
    Bytes b;
    liftEncodeGroup(v, mode, b);
    double h = est.bits(b);
    if (m == 0 || h < bestBits) { bestBits = h; best = mode; }
  }
  return best;
}

// ---- inline images with filters ---------------------------------------------------------------
// BI <dict> ID <data> EI: when every filter of the dict can be undone exactly, the data is
// replaced by the decoded bytes and the layers' recon records.

static bool liftInlineImage(const CodecCtx& ctx, const uint8_t* dict, size_t dn, const uint8_t* data, size_t len,
                            Bytes& rec) {
  Lexer lx(dict, dn, 0);
  Obj filt, parms;
  for (;;) {
    lx.skipWS();
    if (lx.eof()) break;
    Obj k, v;
    if (!lx.parse(k) || k.t != Obj::NAME || !lx.parse(v)) return false;
    if (k.s == "F" || k.s == "Filter") filt = v;
    else if (k.s == "DP" || k.s == "DecodeParms") parms = v;
  }
  std::vector<std::pair<std::string, const Obj*>> fl;
  if (filt.t == Obj::NAME) fl.push_back({normFilter(filt.s), parms.t == Obj::DICT ? &parms : nullptr});
  else if (filt.t == Obj::ARR)
    for (size_t k = 0; k < filt.a.size(); k++) {
      if (filt.a[k].t != Obj::NAME) return false;
      const Obj* pp = parms.t == Obj::ARR && k < parms.a.size() && parms.a[k].t == Obj::DICT ? &parms.a[k] : nullptr;
      fl.push_back({normFilter(filt.a[k].s), pp});
    }
  if (fl.empty()) return false;
  Bytes cur(data, data + len), out, recon;
  std::vector<Layer> layers;
  for (auto& f : fl) {
    uint8_t lt = layerOfFilter(f.first);
    if (!lt) return false;
    FilterParams fp;
    if (f.second) {
      auto geti = [&](const char* key, int& v) {
        const Obj* o = f.second->get(key);
        if (o && o->t == Obj::INT && o->i >= 0 && o->i < (1 << 30)) v = (int)o->i;
      };
      geti("Predictor", fp.predictor);
      geti("Colors", fp.colors);
      geti("BitsPerComponent", fp.bpc);
      geti("Columns", fp.columns);
      geti("EarlyChange", fp.earlyChange);
    }
    if (!layerDecode(ctx, lt, fp, cur, out, recon)) return false;
    layers.push_back({lt, recon});
    cur.swap(out);
    if ((lt == L_FLATE || lt == L_LZW) && fp.predictor > 1) {
      uint8_t pl = fp.predictor == 2 ? L_TIFF : fp.predictor >= 10 ? L_PNG : L_NONE;
      if (!pl || !layerDecode(ctx, pl, fp, cur, out, recon)) return false;
      layers.push_back({pl, recon});
      cur.swap(out);
    }
  }
  rec.clear();
  rec.push_back((uint8_t)layers.size());
  for (auto& l : layers) {
    rec.push_back(l.type);
    putLenBytesTo(rec, l.recon.data(), l.recon.size());
  }
  putLenBytesTo(rec, cur.data(), cur.size());
  return true;
}

static bool liftInlineRestore(const CodecCtx& ctx, RBuf& r, Bytes& out) {
  size_t nl = r.u8();
  std::vector<Layer> layers(nl);
  for (auto& l : layers) {
    l.type = r.u8();
    if (l.type == L_NONE || l.type >= L_COUNT || isPseudoLayerType(l.type)) return false;
    r.bytes(r.len(), l.recon);
  }
  Bytes data, tmp;
  r.bytes(r.len(), data);
  for (size_t k = nl; k-- > 0;) {
    if (!layerEncode(ctx, layers[k].type, layers[k].recon, data, tmp)) return false;
    data.swap(tmp);
  }
  out.insert(out.end(), data.begin(), data.end());
  return true;
}

// ---- forward / inverse -----------------------------------------------------------------------
static inline void liftLit(Bytes& out, uint8_t c) {
  if (liftCtl(c)) out.push_back(0);
  out.push_back(c);
}

static void liftForward(LiftMode mode, const Bytes& in, Bytes& skel, Bytes& nums, Bytes& hex,
                        const CodecCtx* ctx = nullptr, Bytes* inl = nullptr) {
  const uint8_t* p = in.data();
  size_t n = in.size(), i = 0;
  skel.clear();
  nums.clear();
  hex.clear();
  skel.reserve(n);
  std::vector<LiftNum> vals;
  size_t biPos = SIZE_MAX;
  if (inl) inl->clear();
  auto copy = [&](size_t a, size_t b) {
    for (size_t k = a; k < b; k++) liftLit(skel, p[k]);
  };
  while (i < n) {
    uint8_t c = p[i];
    if (isWS(c)) { liftLit(skel, c); i++; continue; }
    if (c == '%') {
      size_t j = i;
      while (j < n && p[j] != '\r' && p[j] != '\n') j++;
      copy(i, j);
      i = j;
      continue;
    }
    if (c == '(') {
      size_t j = i + 1;
      int depth = 1;
      while (j < n && depth) {
        if (p[j] == '\\') { j += 2; continue; }
        if (p[j] == '(') depth++;
        else if (p[j] == ')') depth--;
        j++;
      }
      j = std::min(j, n);
      copy(i, j);
      i = j;
      continue;
    }
    if (c == '<') {
      if (i + 1 < n && p[i + 1] == '<') { skel.push_back('<'); skel.push_back('<'); i += 2; continue; }
      size_t j = i + 1;
      bool ok = true, upper = false, lower = false;
      while (j < n && p[j] != '>') {
        int v = hexVal(p[j]);
        if (v < 0) ok = false;
        if (p[j] >= 'a' && p[j] <= 'f') lower = true;
        if (p[j] >= 'A' && p[j] <= 'F') upper = true;
        j++;
      }
      if (j >= n) ok = false;
      size_t nd = j - i - 1;
      if (mode == LIFT_CS && ok && nd % 2 == 0 && !(upper && lower)) {
        skel.push_back(lower ? 3 : 2);
        putVarTo(hex, nd / 2);
        for (size_t k = i + 1; k < j; k += 2) hex.push_back((uint8_t)(hexVal(p[k]) * 16 + hexVal(p[k + 1])));
        i = j + 1;
        continue;
      }
      j = std::min(j + 1, n);
      copy(i, j);
      i = j;
      continue;
    }
    if (isDelim(c) && c != '/') { skel.push_back(c); i++; continue; }
    if (c == '/') {
      size_t j = i + 1;
      while (j < n && isRegular(p[j])) j++;
      copy(i, j);
      i = j;
      continue;
    }
    size_t j = i;
    while (j < n && isRegular(p[j])) j++;
    LiftNum v;
    if (liftParseNum(p + i, j - i, v)) {
      skel.push_back(1);
      vals.push_back(v);
      i = j;
      continue;
    }
    copy(i, j);
    if (mode == LIFT_CS && j - i == 2 && p[i] == 'B' && p[i + 1] == 'I') biPos = j;
    if (mode == LIFT_CS && j - i == 2 && p[i] == 'I' && p[i + 1] == 'D') {
      size_t k = j + 1;
      while (k + 2 <= n && !(p[k] == 'E' && p[k + 1] == 'I' && isWS(p[k - 1]) && (k + 2 == n || isWS(p[k + 2])))) k++;
      k = std::min(k, n);
      Bytes rec;
      if (ctx && inl && biPos != SIZE_MAX && j < k && isWS(p[j]) && k < n &&
          liftInlineImage(*ctx, p + biPos, i - biPos, p + j + 1, k - (j + 1), rec)) {
        liftLit(skel, p[j]);
        skel.push_back(4);
        inl->insert(inl->end(), rec.begin(), rec.end());
      } else {
        copy(j, k);
      }
      biPos = SIZE_MAX;
      i = k;
      continue;
    }
    i = j;
  }
  LiftGroups g;
  liftGroup(mode, skel.data(), skel.size(), g);
  if (g.ofMarker.size() != vals.size()) {  // cannot happen; the caller verifies anyway
    nums.clear();
    return;
  }
  std::vector<std::vector<LiftNum>> per(g.count);
  for (size_t k = 0; k < vals.size(); k++) per[g.ofMarker[k]].push_back(vals[k]);
  Bytes modes, data;
  LiftEntropy est;
  for (auto& v : per) {
    uint8_t mode = liftChooseMode(v, est);
    modes.push_back(mode);
    liftEncodeGroup(v, mode, data);
  }
  nums.swap(modes);
  nums.insert(nums.end(), data.begin(), data.end());
}

static bool liftInverse(LiftMode mode, const Bytes& skel, const Bytes& nums, const Bytes& hex, Bytes& out,
                        const CodecCtx* ctx = nullptr, const Bytes* inl = nullptr) {
  Bytes noInl;
  RBuf ir(inl ? *inl : noInl);
  LiftGroups g;
  liftGroup(mode, skel.data(), skel.size(), g);
  std::vector<size_t> cnt(g.count, 0);
  for (uint32_t id : g.ofMarker) cnt[id]++;
  std::vector<std::vector<LiftNum>> per(g.count);
  RBuf nr(nums);
  std::vector<uint8_t> modes(g.count);
  for (auto& m : modes) m = nr.u8();
  for (uint32_t id = 0; id < g.count; id++) {
    bool delta = modes[id] & 1, fixed = (modes[id] & 2) != 0;
    int w = modes[id] >> 2;
    if (fixed && (w < 1 || w > 8)) return false;
    int64_t prev = 0;
    per[id].resize(cnt[id]);
    for (auto& v : per[id]) {
      uint64_t x = 0;
      if (fixed)
        for (int k = 0; k < w; k++) x = x << 8 | nr.u8();
      else
        x = nr.var();
      v.code = (uint32_t)(x & 7);
      v.m = unzigzag(x >> 3) + (delta ? prev : 0);
      prev = v.m;
      v.ext = v.code == 7 ? (uint32_t)nr.var() : 0;
    }
  }
  if (nr.p != nums.size()) return false;
  std::vector<size_t> next(g.count, 0);
  RBuf hr(hex);
  out.clear();
  out.reserve(skel.size() * 2);
  size_t mk = 0;
  static const char* U = "0123456789ABCDEF";
  static const char* L = "0123456789abcdef";
  for (size_t i = 0; i < skel.size(); i++) {
    uint8_t c = skel[i];
    if (c == 0) {
      if (++i >= skel.size()) return false;
      out.push_back(skel[i]);
    } else if (c == 1) {
      if (mk >= g.ofMarker.size()) return false;
      uint32_t id = g.ofMarker[mk++];
      liftSpellNum(per[id][next[id]++], out);
    } else if (c == 4) {
      if (!ctx || !liftInlineRestore(*ctx, ir, out)) return false;
    } else if (c == 2 || c == 3) {
      size_t len = hr.len();
      const char* hx = c == 3 ? L : U;
      out.push_back('<');
      for (size_t k = 0; k < len; k++) {
        uint8_t b = hr.u8();
        out.push_back((uint8_t)hx[b >> 4]);
        out.push_back((uint8_t)hx[b & 15]);
      }
      out.push_back('>');
    } else {
      out.push_back(c);
    }
  }
  return mk == g.ofMarker.size() && hr.p == hex.size() && ir.p == ir.b.size();
}

// forward + verify; false if the transform does not reproduce the input
static bool liftApply(LiftMode mode, const Bytes& in, Bytes& skel, Bytes& nums, Bytes& hex,
                      const CodecCtx* ctx = nullptr, Bytes* inl = nullptr) {
  try {
    liftForward(mode, in, skel, nums, hex, ctx, inl);
    Bytes back;
    return liftInverse(mode, skel, nums, hex, back, ctx, inl) && back == in;
  } catch (const Error&) {
    return false;
  }
}
