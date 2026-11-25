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
// Skeleton bytes: 0x00 X = literal byte X (X <= 5), 0x01 = number, 0x02 = hex string with
// upper-case (or no) letters, 0x03 = hex string with lower-case letters, 0x04 = inline image data
// whose filters were undone (content-stream mode; record in the inline buffer: u8 layer count,
// per layer u8 type + recon, then the decoded data, lengths as varints), 0x05 = literal string
// (content-stream mode; bytes as written, escapes kept, in the string buffer).
// Number value (plain form): zigzag(mantissa) * 8 + code; code 0 = integer, 1..6 = "I.F" with that
// many fraction digits, 7 = escape followed by varint(scale * 4 + form), form bit0 = no integer
// part (".5"), bit1 = trailing dot ("5."). Numbers file: the group headers (see "per-group number
// coding"), then the groups' values in group order.

enum LiftMode { LIFT_CS, LIFT_OBJ };

struct LiftNum {
  int64_t m;
  uint32_t code, ext;
};

static inline bool liftCtl(uint8_t c) { return c <= 5; }

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

// false for a value the lifter never produces (corrupt numbers file): more than 18 fraction digits
static bool liftSpellNum(const LiftNum& v, Bytes& out) {
  uint64_t a = v.m < 0 ? 0 - (uint64_t)v.m : (uint64_t)v.m;
  size_t scale = 0;
  bool omitInt = false, trail = false;
  if (v.code >= 1 && v.code <= 6) scale = v.code;
  else if (v.code == 7) {
    scale = v.ext / 4;
    omitInt = v.ext & 1;
    trail = (v.ext & 2) != 0;
  }
  if (scale > 18) return false;
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
  return true;
}

// ---- grouping lexer (runs on the skeleton, identical on both sides) ---------------------------
// Besides the group of every number marker it reports, in content-stream mode, the operators whose
// operands the walk below predicts: path construction and Td/TD. Strings come from the string and
// hex buffers, which both sides have before the numbers.

enum LiftOp : uint8_t { LOP_m, LOP_l, LOP_c, LOP_v, LOP_y, LOP_re, LOP_h, LOP_Td, LOP_COUNT };
static const uint8_t kLiftOpArgs[LOP_COUNT] = {2, 2, 6, 4, 4, 4, 0, 2};

struct LiftEvent {
  uint8_t op;
  bool ok;         // operands are exactly the expected number markers
  uint32_t first;  // marker index of the first operand
  int32_t tdKey;   // Td/TD: table slot of (font, text shown since the last positioning), -1 if none
  bool tdHit;      // that slot was filled before
};

struct LiftGroups {
  std::vector<uint32_t> ofMarker;  // group id per number marker
  uint32_t count = 0;
  std::vector<LiftEvent> events;   // content-stream mode
  uint32_t tdKeys = 0;
};

struct LiftStrings {  // strings by marker order: literal strings (marker 5), hex strings (2, 3)
  std::vector<std::pair<const uint8_t*, size_t>> lit, hex;
};

static int liftOpOf(const uint8_t* t, size_t l) {
  if (l == 1) {
    switch (t[0]) {
      case 'm': return LOP_m;
      case 'l': return LOP_l;
      case 'c': return LOP_c;
      case 'v': return LOP_v;
      case 'y': return LOP_y;
      case 'h': return LOP_h;
    }
    return -1;
  }
  if (l == 2 && t[0] == 'r' && t[1] == 'e') return LOP_re;
  if (l == 2 && t[0] == 'T' && (t[1] == 'd' || t[1] == 'D')) return LOP_Td;
  return -1;
}

static void liftGroup(LiftMode mode, const uint8_t* s, size_t n, LiftGroups& g, const LiftStrings* strs = nullptr) {
  std::unordered_map<std::string, uint32_t> ids;
  struct Pend { size_t idx; uint8_t slot; };
  std::vector<Pend> pend;
  std::string key;  // OBJ: last name at array depth 0
  int arr = 0, operands = 0;
  size_t markers = 0, litK = 0, hexK = 0;
  g.ofMarker.clear();
  g.events.clear();
  // content-stream text state for the Td table
  std::string lastName, font, shown, opText;
  bool opHasText = false;
  std::unordered_map<std::string, int32_t> tdIds;
  std::vector<char> tdSeen;
  auto addText = [&](const uint8_t* p, size_t l) {
    opText.append((const char*)p, l);
    opHasText = true;
  };
  auto flush = [&](const uint8_t* t, size_t tl) {
    char extra = 0;
    if (mode == LIFT_CS && t) {
      int op = liftOpOf(t, tl);
      if (op >= 0) {
        LiftEvent e{(uint8_t)op, false, pend.empty() ? (uint32_t)markers : (uint32_t)pend[0].idx, -1, false};
        e.ok = !arr && operands == kLiftOpArgs[op] && pend.size() == kLiftOpArgs[op];
        if (op == LOP_Td && e.ok && !shown.empty()) {
          std::string k = font;
          k.push_back('\0');
          k += shown;
          auto f = tdIds.find(k);
          if (f == tdIds.end()) {
            f = tdIds.emplace(k, (int32_t)tdSeen.size()).first;
            tdSeen.push_back(0);
          }
          e.tdKey = f->second;
          e.tdHit = tdSeen[(size_t)e.tdKey] != 0;
          tdSeen[(size_t)e.tdKey] = 1;
          if (e.tdHit) extra = 'h';
        }
        g.events.push_back(e);
      }
      // text shown since the last positioning operator (the Td table key)
      bool show = (tl == 2 && t[0] == 'T' && (t[1] == 'j' || t[1] == 'J')) || (tl == 1 && (t[0] == '\'' || t[0] == '"'));
      bool pos = (tl == 2 && ((t[0] == 'T' && (t[1] == 'd' || t[1] == 'D' || t[1] == 'm' || t[1] == '*')) ||
                              (t[0] == 'B' && t[1] == 'T') || (t[0] == 'E' && t[1] == 'T')));
      if (show && opHasText) {
        shown += opText;
        if (shown.size() > 64) shown.erase(0, shown.size() - 64);
      } else if (pos) {
        shown.clear();
      }
      if (tl == 2 && t[0] == 'T' && t[1] == 'f') font = lastName;
      opText.clear();
      opHasText = false;
    }
    for (auto& p : pend) {
      std::string k;
      if (mode == LIFT_OBJ) k = key;
      k.push_back((char)p.slot);
      if (t) k.append((const char*)t, tl);
      if (extra) k.push_back(extra);
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
    if (c == 2 || c == 3 || c == 5) {
      if (!arr) operands++;
      const std::vector<std::pair<const uint8_t*, size_t>>* v = strs ? (c == 5 ? &strs->lit : &strs->hex) : nullptr;
      size_t& k = c == 5 ? litK : hexK;
      if (v && k < v->size()) addText((*v)[k].first, (*v)[k].second);
      k++;
      i++;
      continue;
    }
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
        lastName.assign((const char*)s + i, j - i);
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
  g.tdKeys = (uint32_t)tdSeen.size();
}

// ---- value walk: predictions from the path's current point and the Td table -------------------
// Values in units of 1e-6 ("v6"). An operand is predicted when the state it depends on is known;
// path operators keep the current point, Td/TD fill a table keyed by font and the text shown
// since the last positioning operator.
struct LiftWalk {
  int64_t cx = 0, cy = 0, sx = 0, sy = 0;
  bool valid = false;
  std::vector<std::array<int64_t, 2>> td;
  explicit LiftWalk(uint32_t tdKeys) : td(tdKeys) {}
  // prediction for operand i of event e given the event's earlier operand values
  bool pred(const LiftEvent& e, int i, const int64_t* v, int64_t& p) const {
    switch (e.op) {
      case LOP_m: case LOP_l: case LOP_re:
        if (i > 1 || !valid) return false;
        p = i ? cy : cx;
        return true;
      case LOP_c: case LOP_v: case LOP_y:
        if (i < 2) { if (!valid) return false; p = i ? cy : cx; return true; }
        p = v[i - 2];
        return true;
      case LOP_Td:
        if (!e.tdHit) return false;
        p = td[(size_t)e.tdKey][(size_t)i];
        return true;
    }
    return false;
  }
  // state after event e; known = every operand value representable
  void update(const LiftEvent& e, const int64_t* v, bool known) {
    switch (e.op) {
      case LOP_m: case LOP_re:
        if (known) { cx = sx = v[0]; cy = sy = v[1]; }
        valid = known;
        break;
      case LOP_l: case LOP_c: case LOP_v: case LOP_y: {
        int k = kLiftOpArgs[e.op] - 2;
        if (known) { cx = v[k]; cy = v[k + 1]; }
        valid = known;
        break;
      }
      case LOP_h:
        if (valid) { cx = sx; cy = sy; }
        break;
      case LOP_Td:
        if (e.tdKey >= 0 && known) td[(size_t)e.tdKey] = {v[0], v[1]};
        break;
    }
  }
};

// runs the walk over the events in order. get(k, hasPred, pred, v6) returns whether marker k's
// value is representable at scale 6 and stores it in v6; the decoder decodes the value there.
template <class Get>
static void liftRunWalk(const LiftGroups& g, Get get) {
  LiftWalk w(g.tdKeys);
  for (auto& e : g.events) {
    int na = kLiftOpArgs[e.op];
    int64_t v[6] = {};
    bool kn[6] = {};
    bool known = e.ok;
    if (e.ok)
      for (int i = 0; i < na; i++) {
        int64_t p = 0;
        bool hp = false;
        bool curve = e.op == LOP_c || e.op == LOP_v || e.op == LOP_y;
        if (!(curve && i >= 2) || kn[i - 2]) hp = w.pred(e, i, v, p);
        kn[i] = get(e.first + i, hp, p, v[i]);
        known = known && kn[i];
      }
    w.update(e, v, known);
  }
}

// ---- per-group number coding -----------------------------------------------------------------
// Group header: u8 mode; bits 0-1 = 0 varint, 1 varint of delta, 2 fixed width, 3 fixed width of
// delta; bits 2-5 = width in bytes for the fixed modes; bit 6 = unit form, then u8 (S | fixed << 3
// | noLeadingZero << 4); bit 7 = residual form, then u8 k.
// Plain form: a value is zigzag(m or m - prev) * 8 + code, followed by varint(ext) when code is 7.
// Unit form: every value is an integer v in units of 10^-S; its spelling is recomputed: S fraction
// digits ("fixed") or S minus the trailing zeros of v ("canonical", 0 -> "0"); a zero integer part
// is written or omitted (".5") as the group's flag says. Residual form (unit form only): the coded
// value is (v * 10^(6-S) - prediction) / 10^k, prediction 0 where the walk has none.

static uint64_t liftWord(const LiftNum& v, int64_t prev, bool delta) {
  return zigzag(delta ? v.m - prev : v.m) * 8 + v.code;
}

static void liftEncodeGroup(const std::vector<LiftNum>& v, uint8_t mode, Bytes& out) {
  bool delta = mode & 1, fixed = (mode & 2) != 0;
  int w = (mode >> 2) & 15;
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

static void liftEncodeInts(const std::vector<int64_t>& v, uint8_t mode, Bytes& out) {
  bool delta = mode & 1, fixed = (mode & 2) != 0;
  int w = (mode >> 2) & 15;
  int64_t prev = 0;
  for (int64_t x : v) {
    uint64_t word = zigzag(delta ? (int64_t)((uint64_t)x - (uint64_t)prev) : x);
    prev = x;
    if (fixed)
      for (int k = w - 1; k >= 0; k--) out.push_back((uint8_t)(word >> (8 * k)));
    else
      putVarTo(out, word);
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

// best of the four modes for a group (plain values or integers); returns the mode, fills out
template <class T, class Enc>
static uint8_t liftChooseModeT(const std::vector<T>& v, LiftEntropy& est, Enc enc, uint64_t (*word)(const T&, int64_t, bool),
                               int64_t (*valOf)(const T&), Bytes& out, double& bitsOut) {
  int w[2] = {1, 1};
  for (int d = 0; d < 2; d++) {
    int64_t prev = 0;
    for (auto& x : v) {
      uint64_t wd = word(x, prev, d != 0);
      prev = valOf(x);
      while (w[d] < 8 && (wd >> (8 * w[d]))) w[d]++;
    }
  }
  uint8_t best = 0;
  double bestBits = 0;
  for (uint8_t m = 0; m < 4; m++) {
    if (v.size() < 32 && m) break;
    uint8_t mode = (uint8_t)(m | ((m & 2) ? w[m & 1] << 2 : 0));
    Bytes b;
    enc(v, mode, b);
    double h = est.bits(b);
    if (m == 0 || h < bestBits) { bestBits = h; best = mode; out.swap(b); }
  }
  bitsOut = bestBits;
  return best;
}
static uint64_t liftWordNum(const LiftNum& x, int64_t prev, bool d) { return liftWord(x, prev, d); }
static int64_t liftValNum(const LiftNum& x) { return x.m; }
static uint64_t liftWordInt(const int64_t& x, int64_t prev, bool d) {
  return zigzag(d ? (int64_t)((uint64_t)x - (uint64_t)prev) : x);
}
static int64_t liftValInt(const int64_t& x) { return x; }

static const int64_t kPow10[19] = {1LL, 10LL, 100LL, 1000LL, 10000LL, 100000LL, 1000000LL, 10000000LL, 100000000LL,
                                   1000000000LL, 10000000000LL, 100000000000LL, 1000000000000LL, 10000000000000LL,
                                   100000000000000LL, 1000000000000000LL, 10000000000000000LL, 100000000000000000LL,
                                   1000000000000000000LL};
static const int64_t kLiftV6Max = (int64_t)1 << 59;  // |v6| bound for the walk (no overflow in sums)

// fraction digits and spelling flags of a plain value; false for "5." (no place in unit form)
static bool liftScale(const LiftNum& x, int& scale, bool& omitInt) {
  omitInt = false;
  if (x.code == 0) { scale = 0; return true; }
  if (x.code <= 6) { scale = (int)x.code; return true; }
  if (x.ext & 2) return false;
  scale = (int)(x.ext / 4);
  omitInt = x.ext & 1;
  return true;
}

// value at scale 6 (v6); false if not representable
static bool liftV6(const LiftNum& x, int64_t& v6) {
  int sc;
  bool oi;
  if (!liftScale(x, sc, oi) || sc > 6) return false;
  int64_t f = kPow10[6 - sc];
  if (x.m > kLiftV6Max / f || x.m < -kLiftV6Max / f) return false;
  v6 = x.m * f;
  return true;
}

struct LiftUnit {
  int S = 0;
  bool fixed = false, noLead = false;
  uint8_t byte() const { return (uint8_t)(S | (fixed ? 8 : 0) | (noLead ? 16 : 0)); }
};

// spelling of the unit-form value v
static LiftNum liftUnitSpell(const LiftUnit& u, int64_t v) {
  int s = u.S;
  if (!u.fixed) {
    if (v == 0) s = 0;
    else {
      int64_t t = v;
      while (s > 0 && t % 10 == 0) { t /= 10; s--; }
    }
  }
  LiftNum x;
  x.m = v / kPow10[u.S - s];
  x.ext = 0;
  uint64_t a = x.m < 0 ? 0 - (uint64_t)x.m : (uint64_t)x.m;
  if (s == 0) x.code = 0;
  else if (u.noLead && a < (uint64_t)kPow10[s]) { x.code = 7; x.ext = (uint32_t)(s * 4 + 1); }
  else x.code = (uint32_t)s;
  return x;
}

// unit form for a group's values: true and the integers if every value fits one rule
static bool liftUnitOf(const std::vector<LiftNum>& v, LiftUnit& u, std::vector<int64_t>& ints) {
  if (v.empty()) return false;
  int S = 0, lead = -1;  // lead: 1 written, 0 omitted, -1 unknown
  for (auto& x : v) {
    int sc;
    bool oi;
    if (!liftScale(x, sc, oi) || sc > 6) return false;
    S = std::max(S, sc);
    uint64_t a = x.m < 0 ? 0 - (uint64_t)x.m : (uint64_t)x.m;
    if (sc > 0 && a < (uint64_t)kPow10[sc]) {
      int l = oi ? 0 : 1;
      if (lead >= 0 && lead != l) return false;
      lead = l;
    } else if (oi) {
      return false;
    }
  }
  u.S = S;
  u.noLead = lead == 0;
  ints.resize(v.size());
  bool canon = true, fixedOk = true;
  for (size_t k = 0; k < v.size(); k++) {
    int sc;
    bool oi;
    liftScale(v[k], sc, oi);
    int64_t f = kPow10[S - sc];
    if (v[k].m > kLiftV6Max / f || v[k].m < -kLiftV6Max / f) return false;
    ints[k] = v[k].m * f;
    if (sc != S) fixedOk = false;
    int cs = S;
    if (ints[k] == 0) cs = 0;
    else {
      int64_t t = ints[k];
      while (cs > 0 && t % 10 == 0) { t /= 10; cs--; }
    }
    if (cs != sc) canon = false;
  }
  if (!canon && !fixedOk) return false;
  u.fixed = !canon;  // canonical when both hold
  return true;
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
  return r.ok();
}

// ---- forward / inverse -----------------------------------------------------------------------
static inline void liftLit(Bytes& out, uint8_t c) {
  if (liftCtl(c)) out.push_back(0);
  out.push_back(c);
}

// string buffers (.csh, .css): var size of the length section, the lengths (varints), the bytes
static void liftPackStrings(const Bytes& lens, const Bytes& data, Bytes& out) {
  out.clear();
  if (lens.empty()) return;
  putVarTo(out, lens.size());
  out.insert(out.end(), lens.begin(), lens.end());
  out.insert(out.end(), data.begin(), data.end());
}
static bool liftUnpackStrings(const Bytes& in, std::vector<std::pair<const uint8_t*, size_t>>& out) {
  out.clear();
  if (in.empty()) return true;
  RBuf hh(in);
  size_t ln = hh.len();
  size_t dp = hh.p + ln;
  RBuf lr(in);
  lr.p = hh.p;
  while (lr.p < hh.p + ln) {
    uint64_t l = lr.var();
    if (lr.p > hh.p + ln || l > in.size() - dp) return false;
    out.push_back({in.data() + dp, (size_t)l});
    dp += (size_t)l;
  }
  return hh.ok() && lr.ok() && dp == in.size();
}

// per group: coding chosen by the encoder, values as the decoder sees them
struct LiftGroupCode {
  uint8_t mode = 0;
  LiftUnit unit;
  int k = 0;  // residual: power of ten of the coded unit
};

static void liftForward(LiftMode mode, const Bytes& in, Bytes& skel, Bytes& nums, Bytes& hex, Bytes& str,
                        const CodecCtx* ctx = nullptr, Bytes* inl = nullptr) {
  const uint8_t* p = in.data();
  size_t n = in.size(), i = 0;
  skel.clear();
  nums.clear();
  hex.clear();
  str.clear();
  Bytes hexLen, hexData, strLen;  // lengths kept apart from the bytes (xz: about -10% on .csh)
  std::vector<std::pair<size_t, size_t>> litAt, hexAt;  // (offset, length) in in / hexData
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
      if (mode == LIFT_CS && depth == 0 && j <= n) {
        // literal string: bytes as written (escapes kept) to the string buffer
        skel.push_back(5);
        putVarTo(strLen, j - 1 - (i + 1));
        litAt.push_back({i + 1, j - 1 - (i + 1)});
        i = j;
        continue;
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
        putVarTo(hexLen, nd / 2);
        hexAt.push_back({hexData.size(), nd / 2});
        for (size_t k = i + 1; k < j; k += 2) hexData.push_back((uint8_t)(hexVal(p[k]) * 16 + hexVal(p[k + 1])));
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
  Bytes strData;
  for (auto& a : litAt) strData.insert(strData.end(), p + a.first, p + a.first + a.second);
  liftPackStrings(hexLen, hexData, hex);
  liftPackStrings(strLen, strData, str);
  LiftStrings ls;
  for (auto& a : litAt) ls.lit.push_back({p + a.first, a.second});
  for (auto& a : hexAt) ls.hex.push_back({hexData.data() + a.first, a.second});
  LiftGroups g;
  liftGroup(mode, skel.data(), skel.size(), g, &ls);
  if (g.ofMarker.size() != vals.size()) {  // cannot happen; the caller verifies anyway
    nums.clear();
    return;
  }
  // predictions from the walk, with the true values
  std::vector<int64_t> v6(vals.size()), pred(vals.size(), 0);
  std::vector<char> has6(vals.size()), hasPred(vals.size(), 0);
  for (size_t k = 0; k < vals.size(); k++) has6[k] = liftV6(vals[k], v6[k]);
  liftRunWalk(g, [&](size_t k, bool hp, int64_t pv, int64_t& out) {
    hasPred[k] = hp;
    pred[k] = hp ? pv : 0;
    out = v6[k];
    return has6[k] != 0;
  });
  std::vector<std::vector<uint32_t>> members(g.count);
  for (size_t k = 0; k < vals.size(); k++) members[g.ofMarker[k]].push_back((uint32_t)k);
  Bytes head, data;
  LiftEntropy est;
  for (auto& mem : members) {
    std::vector<LiftNum> gv;
    for (uint32_t k : mem) gv.push_back(vals[k]);
    Bytes bestB, b;
    double bestBits, bits;
    uint8_t mode0 = liftChooseModeT(gv, est, liftEncodeGroup, liftWordNum, liftValNum, bestB, bestBits);
    Bytes hdr{mode0};
    LiftUnit u;
    std::vector<int64_t> ints;
    if (liftUnitOf(gv, u, ints)) {
      uint8_t m1 = liftChooseModeT(ints, est, liftEncodeInts, liftWordInt, liftValInt, b, bits);
      if (bits + 8 < bestBits) {
        bestBits = bits + 8;
        bestB.swap(b);
        hdr = {(uint8_t)(m1 | 0x40), u.byte()};
      }
      // residual form: only where the walk predicts something
      bool anyPred = false, all6 = true;
      for (uint32_t k : mem) { anyPred |= hasPred[k] != 0; all6 &= has6[k] != 0; }
      if (anyPred && all6) {
        std::vector<int64_t> r(mem.size());
        for (size_t t = 0; t < mem.size(); t++) r[t] = v6[mem[t]] - pred[mem[t]];
        int k10 = 0;
        while (k10 < 12) {
          bool ok = true;
          for (int64_t x : r) if (x % kPow10[k10 + 1]) { ok = false; break; }
          if (!ok) break;
          k10++;
        }
        for (auto& x : r) x /= kPow10[k10];
        uint8_t m2 = liftChooseModeT(r, est, liftEncodeInts, liftWordInt, liftValInt, b, bits);
        if (bits + 16 < bestBits) {
          bestBits = bits + 16;
          bestB.swap(b);
          hdr = {(uint8_t)(m2 | 0xC0), u.byte(), (uint8_t)k10};
        }
      }
    }
    head.insert(head.end(), hdr.begin(), hdr.end());
    data.insert(data.end(), bestB.begin(), bestB.end());
  }
  nums.swap(head);
  nums.insert(nums.end(), data.begin(), data.end());
}

static bool liftInverse(LiftMode mode, const Bytes& skel, const Bytes& nums, const Bytes& hex, const Bytes& str,
                        Bytes& out, const CodecCtx* ctx = nullptr, const Bytes* inl = nullptr) {
  Bytes noInl;
  RBuf ir(inl ? *inl : noInl);
  LiftStrings ls;
  if (!liftUnpackStrings(hex, ls.hex) || !liftUnpackStrings(str, ls.lit)) return false;
  LiftGroups g;
  liftGroup(mode, skel.data(), skel.size(), g, &ls);
  size_t nm = g.ofMarker.size();
  std::vector<size_t> cnt(g.count, 0);
  for (uint32_t id : g.ofMarker) cnt[id]++;
  RBuf nr(nums);
  std::vector<LiftGroupCode> gc(g.count);
  for (auto& c : gc) {
    c.mode = nr.u8();
    if (c.mode & 0x40) {
      uint8_t ub = nr.u8();
      c.unit.S = ub & 7;
      c.unit.fixed = (ub & 8) != 0;
      c.unit.noLead = (ub & 16) != 0;
      if (c.unit.S > 6 || ub > 31) return false;
    } else if (c.mode & 0x80) {
      return false;  // residual form needs the unit form
    }
    if (c.mode & 0x80) {
      c.k = nr.u8();
      if (c.k > 12) return false;
    }
    int w = (c.mode >> 2) & 15;
    if ((c.mode & 2) && (w < 1 || w > 8)) return false;
  }
  // coded values per group, in marker order within the group
  std::vector<std::vector<LiftNum>> plain(g.count);
  std::vector<std::vector<int64_t>> coded(g.count);
  for (uint32_t id = 0; id < g.count; id++) {
    const LiftGroupCode& c = gc[id];
    bool delta = c.mode & 1, fixed = (c.mode & 2) != 0;
    int w = (c.mode >> 2) & 15;
    int64_t prev = 0;
    auto word = [&]() -> uint64_t {
      uint64_t x = 0;
      if (fixed)
        for (int k = 0; k < w; k++) x = x << 8 | nr.u8();
      else
        x = nr.var();
      return x;
    };
    if (c.mode & 0x40) {
      coded[id].resize(cnt[id]);
      for (auto& v : coded[id]) {
        uint64_t x = word();
        v = delta ? (int64_t)((uint64_t)prev + (uint64_t)unzigzag(x)) : unzigzag(x);
        prev = v;
      }
    } else {
      plain[id].resize(cnt[id]);
      for (auto& v : plain[id]) {
        uint64_t x = word();
        v.code = (uint32_t)(x & 7);
        v.m = (int64_t)((delta ? (uint64_t)prev : 0) + (uint64_t)unzigzag(x >> 3));
        prev = v.m;
        v.ext = v.code == 7 ? (uint32_t)nr.var() : 0;
      }
    }
  }
  if (nr.p != nums.size()) return false;
  // values per marker
  std::vector<uint32_t> pos(nm);
  {
    std::vector<uint32_t> next(g.count, 0);
    for (size_t k = 0; k < nm; k++) pos[k] = next[g.ofMarker[k]]++;
  }
  std::vector<int64_t> unitVal(nm, 0);
  std::vector<char> done(nm, 0);
  bool bad = false;
  // residual form: v6 = coded * 10^k + prediction; unit value = v6 / 10^(6-S)
  auto resolve = [&](size_t k, bool hp, int64_t pv) {
    const LiftGroupCode& c = gc[g.ofMarker[k]];
    int64_t x = coded[g.ofMarker[k]][pos[k]];
    int64_t f = kPow10[c.k];
    if (x > (INT64_MAX / 4) / f || x < -(INT64_MAX / 4) / f) { bad = true; return; }
    int64_t v6 = x * f + (hp ? pv : 0);
    int64_t d = kPow10[6 - c.unit.S];
    if (v6 % d) { bad = true; return; }
    unitVal[k] = v6 / d;
    done[k] = 1;
  };
  liftRunWalk(g, [&](size_t k, bool hp, int64_t pv, int64_t& v6) -> bool {
    uint32_t id = g.ofMarker[k];
    const LiftGroupCode& c = gc[id];
    if (c.mode & 0x80) {
      resolve(k, hp, pv);
      if (bad) return false;
      int64_t f = kPow10[6 - c.unit.S];
      if (unitVal[k] > kLiftV6Max / f || unitVal[k] < -kLiftV6Max / f) return false;
      v6 = unitVal[k] * f;
      return true;
    }
    if (c.mode & 0x40) {
      int64_t v = coded[id][pos[k]], f = kPow10[6 - c.unit.S];
      if (v > kLiftV6Max / f || v < -kLiftV6Max / f) return false;
      v6 = v * f;
      return true;
    }
    return liftV6(plain[id][pos[k]], v6);
  });
  if (bad) return false;
  for (size_t k = 0; k < nm; k++)
    if ((gc[g.ofMarker[k]].mode & 0x80) && !done[k]) {
      resolve(k, false, 0);
      if (bad) return false;
    }
  // spelling
  out.clear();
  out.reserve(skel.size() * 2);
  size_t mk = 0, hk = 0, sk = 0;
  static const char* U = "0123456789ABCDEF";
  static const char* L = "0123456789abcdef";
  for (size_t i = 0; i < skel.size(); i++) {
    uint8_t c = skel[i];
    if (c == 0) {
      if (++i >= skel.size()) return false;
      out.push_back(skel[i]);
    } else if (c == 1) {
      if (mk >= nm) return false;
      size_t k = mk++;
      const LiftGroupCode& gcd = gc[g.ofMarker[k]];
      LiftNum v;
      if (gcd.mode & 0x80) v = liftUnitSpell(gcd.unit, unitVal[k]);
      else if (gcd.mode & 0x40) v = liftUnitSpell(gcd.unit, coded[g.ofMarker[k]][pos[k]]);
      else v = plain[g.ofMarker[k]][pos[k]];
      if (!liftSpellNum(v, out)) return false;
    } else if (c == 4) {
      if (!ctx || !liftInlineRestore(*ctx, ir, out)) return false;
    } else if (c == 2 || c == 3) {
      if (hk >= ls.hex.size()) return false;
      const char* hx = c == 3 ? L : U;
      out.push_back('<');
      for (size_t k = 0; k < ls.hex[hk].second; k++) {
        uint8_t b = ls.hex[hk].first[k];
        out.push_back((uint8_t)hx[b >> 4]);
        out.push_back((uint8_t)hx[b & 15]);
      }
      out.push_back('>');
      hk++;
    } else if (c == 5) {
      if (sk >= ls.lit.size()) return false;
      out.push_back('(');
      out.insert(out.end(), ls.lit[sk].first, ls.lit[sk].first + ls.lit[sk].second);
      out.push_back(')');
      sk++;
    } else {
      out.push_back(c);
    }
  }
  return mk == nm && hk == ls.hex.size() && sk == ls.lit.size() && ir.ok() && nr.ok() && ir.p == ir.b.size();
}

// forward + verify; false if the transform does not reproduce the input
// check: run the inverse and compare (the encoder skips it when its final verify will run and
// redoes the lifts with the check if that verify fails)
static bool liftApply(LiftMode mode, const Bytes& in, Bytes& skel, Bytes& nums, Bytes& hex, Bytes& str,
                      const CodecCtx* ctx = nullptr, Bytes* inl = nullptr, bool check = true) {
  return softly([&] {
    liftForward(mode, in, skel, nums, hex, str, ctx, inl);
    if (!check) return true;
    Bytes back;
    return liftInverse(mode, skel, nums, hex, str, back, ctx, inl) && back == in;
  });
}
