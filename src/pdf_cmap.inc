// pdf_cmap.inc -- CMap bfchar / bfrange blocks as binary (whole .cmap class file).
//
// "N beginbfchar <src> <dst> ... endbfchar" and the bfrange equivalent ("<lo> <hi> <dst>")
// are mostly arithmetic sequences that xz sees as hex text. A block body in uniform format
// (fixed source width, one case, the same separator between fields and the same line break
// between entries) is replaced in the text by a marker; the entries go to a binary buffer:
// source as zigzag(src - expected), range width, destination as zigzag(dst - expected) where it
// fits 64 bits, else raw bytes. Anything irregular stays text; the encoder verifies the inverse.
//
// Skeleton bytes: 0x00 X = literal X (X <= 3), 0x01 = block body from the binary buffer.
// Block record: u8 kind (0 bfchar, 1 bfrange), u8 flags (bit0 lower case), var entries,
// var source digits, var lengths + bytes of: leading text, field separator, line break,
// trailing text; then per entry: zigzag src delta, [range width], var destination digits,
// destination (zigzag delta if <= 16 digits, else raw bytes).

struct CmapEntry {
  uint64_t src = 0, hi = 0;
  std::string dst;  // hex digits as written
};

// parses "<hex>" at p; returns end or 0
static size_t cmapHex(const uint8_t* d, size_t n, size_t p, std::string& hex) {
  if (p >= n || d[p] != '<') return 0;
  size_t q = p + 1;
  while (q < n && hexVal(d[q]) >= 0) q++;
  if (q >= n || d[q] != '>' || q == p + 1) return 0;
  hex.assign((const char*)d + p + 1, q - p - 1);
  return q + 1;
}

static uint64_t cmapVal(const std::string& h) {
  uint64_t v = 0;
  for (char c : h) v = v << 4 | (uint64_t)hexVal((uint8_t)c);
  return v;
}

static bool cmapCaseOf(const std::string& h, int& lower) {
  for (char c : h) {
    if (c >= 'a' && c <= 'f') { if (lower == 0) return false; lower = 1; }
    if (c >= 'A' && c <= 'F') { if (lower == 1) return false; lower = 0; }
  }
  return true;
}

static void cmapPutHex(Bytes& out, uint64_t v, size_t digits, bool lower) {
  static const char* U = "0123456789ABCDEF";
  static const char* L = "0123456789abcdef";
  const char* hx = lower ? L : U;
  for (size_t k = digits; k-- > 0;) {
    unsigned nib = (unsigned)(v >> (4 * k)) & 15u;  // (a local: GCC's UBSan misreports the shift inline)
    out.push_back((uint8_t)hx[nib]);
  }
}

// body [a, b) of one block; true and record on success
static bool cmapEncodeBlock(const uint8_t* d, size_t a, size_t b, int kind, Bytes& rec) {
  size_t p = a;
  while (p < b && isWS(d[p])) p++;
  Bytes lead(d + a, d + p), sep, eol;
  bool haveSep = false, haveEol = false;
  std::vector<CmapEntry> ents;
  int lower = -1;
  size_t srcDigits = 0;
  size_t lastEnd = p;
  while (p < b && d[p] == '<') {
    CmapEntry e;
    std::string h;
    int fields = kind == 1 ? 3 : 2;
    for (int f = 0; f < fields; f++) {
      if (f) {
        size_t q = p;
        while (q < b && isWS(d[q])) q++;
        Bytes s(d + p, d + q);
        if (s.empty()) return false;
        if (!haveSep) { sep = s; haveSep = true; }
        else if (s != sep) return false;
        p = q;
      }
      size_t q = cmapHex(d, b, p, h);
      if (!q || !cmapCaseOf(h, lower)) return false;
      if (f == 0) {
        if (srcDigits == 0) srcDigits = h.size();
        if (h.size() != srcDigits || h.size() > 16) return false;
        e.src = cmapVal(h);
      } else if (f == 1 && kind == 1) {
        if (h.size() != srcDigits) return false;
        e.hi = cmapVal(h);
      } else {
        e.dst = h;
      }
      p = q;
    }
    ents.push_back(e);
    lastEnd = p;
    size_t q = p;
    while (q < b && isWS(d[q])) q++;
    if (q < b && d[q] == '<') {
      Bytes s(d + p, d + q);
      if (s.empty()) return false;
      if (!haveEol) { eol = s; haveEol = true; }
      else if (s != eol) return false;
    }
    p = q;
  }
  if (ents.empty() || p != b) return false;
  Bytes trail(d + lastEnd, d + b);
  rec.push_back((uint8_t)kind);
  rec.push_back((uint8_t)(lower == 1 ? 1 : 0));
  putVarTo(rec, ents.size());
  putVarTo(rec, srcDigits);
  putLenBytesTo(rec, lead.data(), lead.size());
  putLenBytesTo(rec, sep.data(), sep.size());
  putLenBytesTo(rec, eol.data(), eol.size());
  putLenBytesTo(rec, trail.data(), trail.size());
  uint64_t nextSrc = 0, nextDst = 0;
  for (auto& e : ents) {
    putVarTo(rec, zigzag((int64_t)(e.src - nextSrc)));
    uint64_t w = 0;
    if (kind == 1) {
      w = e.hi - e.src;
      putVarTo(rec, zigzag((int64_t)w));
    }
    putVarTo(rec, e.dst.size());
    if (e.dst.size() <= 16) {
      uint64_t v = cmapVal(e.dst);
      putVarTo(rec, zigzag((int64_t)(v - nextDst)));
      nextDst = v + w + 1;
    } else {
      rec.insert(rec.end(), e.dst.begin(), e.dst.end());
    }
    nextSrc = (kind == 1 ? e.hi : e.src) + 1;
  }
  return true;
}

static bool cmapDecodeBlock(RBuf& r, Bytes& out) {
  int kind = r.u8();
  bool lower = r.u8() & 1;
  uint64_t n = r.var(), srcDigits = r.var();
  if (srcDigits < 1 || srcDigits > 16 || n > ((uint64_t)1 << 32)) return false;
  Bytes lead, sep, eol, trail;
  r.bytes(r.len(), lead);
  r.bytes(r.len(), sep);
  r.bytes(r.len(), eol);
  r.bytes(r.len(), trail);
  out.insert(out.end(), lead.begin(), lead.end());
  uint64_t nextSrc = 0, nextDst = 0;
  for (size_t k = 0; k < n; k++) {
    if (k) out.insert(out.end(), eol.begin(), eol.end());
    uint64_t src = nextSrc + (uint64_t)unzigzag(r.var()), w = 0;
    out.push_back('<');
    cmapPutHex(out, src, srcDigits, lower);
    out.push_back('>');
    if (kind == 1) {
      w = (uint64_t)unzigzag(r.var());
      out.insert(out.end(), sep.begin(), sep.end());
      out.push_back('<');
      cmapPutHex(out, src + w, srcDigits, lower);
      out.push_back('>');
    }
    out.insert(out.end(), sep.begin(), sep.end());
    uint64_t dd = r.var();
    if (dd == 0 || dd > ((uint64_t)1 << 24)) return false;
    out.push_back('<');
    if (dd <= 16) {
      uint64_t v = nextDst + (uint64_t)unzigzag(r.var());
      cmapPutHex(out, v, dd, lower);
      nextDst = v + w + 1;
    } else {
      r.appendTo((size_t)dd, out);
    }
    out.push_back('>');
    nextSrc = src + w + 1;
  }
  out.insert(out.end(), trail.begin(), trail.end());
  return true;
}

static void cmapForward(const Bytes& in, Bytes& skel, Bytes& bin) {
  const uint8_t* d = in.data();
  size_t n = in.size(), pos = 0;
  skel.clear();
  bin.clear();
  auto copy = [&](size_t a, size_t b) {
    for (size_t k = a; k < b; k++) liftLit(skel, d[k]);
  };
  static const char* const kw[2][2] = {{"beginbfchar", "endbfchar"}, {"beginbfrange", "endbfrange"}};
  size_t p = 0;
  while (p < n) {
    // next block start
    size_t best = n;
    int kind = -1;
    for (int k = 0; k < 2; k++) {
      const uint8_t* f = memfind(d + p, n - p, kw[k][0], strlen(kw[k][0]));
      if (f && (size_t)(f - d) < best) { best = (size_t)(f - d); kind = k; }
    }
    if (kind < 0) break;
    size_t a = best + strlen(kw[kind][0]);
    const uint8_t* e = memfind(d + a, n - a, kw[kind][1], strlen(kw[kind][1]));
    if (!e) break;
    size_t b = (size_t)(e - d);
    Bytes rec;
    if (cmapEncodeBlock(d, a, b, kind, rec)) {
      copy(pos, a);
      skel.push_back(1);
      bin.insert(bin.end(), rec.begin(), rec.end());
      pos = b;
    }
    p = b;
  }
  copy(pos, n);
}

static bool cmapInverse(const Bytes& skel, const Bytes& bin, Bytes& out) {
  RBuf r(bin);
  out.clear();
  for (size_t i = 0; i < skel.size(); i++) {
    uint8_t c = skel[i];
    if (c == 0) {
      if (++i >= skel.size()) return false;
      out.push_back(skel[i]);
    } else if (c == 1) {
      if (!cmapDecodeBlock(r, out)) return false;
    } else {
      out.push_back(c);
    }
  }
  return r.p == bin.size();
}

static bool cmapApply(const Bytes& in, Bytes& skel, Bytes& bin) {
  try {
    cmapForward(in, skel, bin);
    if (bin.empty()) return false;
    Bytes back;
    return cmapInverse(skel, bin, back) && back == in;
  } catch (const Error&) {
    return false;
  }
}
