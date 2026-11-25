// pdf_derive.inc -- derived fields (plan 2A / 3.5).
//
// Values that the decoder can compute are replaced in the item text by a placeholder: byte 0x04
// followed by a code byte; a literal 0x04 in item text is written as 0x04 0x04. The encoder only
// substitutes a token whose spelling is the canonical spelling of the value the decoder will
// compute, so the substitution is exact by construction (and the final verify checks it).
//
// Codes and values:
//   L  stream /Length = payload size        l  value of an indirect /Length object
//   D  decoded size (/DL, TrueType /Length1, EmbeddedFile /Params /Size)
//   1 2 3  Type 1 /Length1 /Length2 /Length3 (cleartext, encrypted part, trailer)
//   N F  ObjStm /N /First                   W H  image /Width /Height (BMP header, JPEG SOF)
//   C c  EmbeddedFile /Params /CheckSum: MD5 of the decoded data, upper / lower case hex digits
//   S  startxref value = start of the latest xref section
//   P  /Prev = start of the xref section before the current one
//   Z  trailer /Size = end of the latest classic xref section (max start + count)
//   X  /XRefStm = start of the latest XRef stream object

static const uint8_t kDerivEsc = 4;

struct StreamFacts {
  bool payloadKnown = false, decodedKnown = false;
  uint64_t payloadLen = 0, decodedLen = 0;
  int64_t objstmN = -1, objstmFirst = -1, t1s = -1, t1n = -1, w = -1, h = -1;
  bool md5Known = false;
  uint8_t md5[16];
};

// xref section bookkeeping shared by encoder and decoder (updated in item order)
struct DeriveState {
  std::vector<uint64_t> xrefPos;    // starts of classic sections and XRef stream objects
  int64_t lastXrefEnd = -1;         // max(start + count) of the latest classic section
  int64_t lastXRefStm = -1;         // start of the latest XRef stream object
  std::unordered_map<uint32_t, uint64_t> lenRef;  // object number -> payload size of the stream
                                                  // whose /Length refers to it
};

static bool isPseudoLayer(uint8_t t) { return t == L_BMP || t == L_T1 || t == L_OBJSTM || t == L_XREFROWS; }

// JPEG SOFn dimensions
static bool jpegDims(const Bytes& d, int64_t& w, int64_t& h) {
  if (d.size() < 4 || d[0] != 0xFF || d[1] != 0xD8) return false;
  size_t p = 2;
  while (p + 4 <= d.size()) {
    if (d[p] != 0xFF) return false;
    uint8_t m = d[p + 1];
    if (m == 0xFF) { p++; continue; }
    if (m == 0xD8 || m == 0x01 || (m >= 0xD0 && m <= 0xD7)) { p += 2; continue; }
    size_t len = ((size_t)d[p + 2] << 8) | d[p + 3];
    if (len < 2) return false;
    if (m >= 0xC0 && m <= 0xCF && m != 0xC4 && m != 0xC8 && m != 0xCC) {
      if (p + 9 > d.size()) return false;
      h = ((int64_t)d[p + 5] << 8) | d[p + 6];
      w = ((int64_t)d[p + 7] << 8) | d[p + 8];
      return true;
    }
    if (m == 0xDA) return false;
    p += 2 + len;
  }
  return false;
}

// Facts that depend only on the stored data and the layers (identical on both sides).
static void streamFacts(const CodecCtx& ctx, const StreamDesc& sd, const Bytes& stored, bool wantDecoded,
                        bool wantMd5, StreamFacts& f) {
  // image dimensions straight from the stored form
  bool bmp = false;
  for (auto& l : sd.layers) bmp |= l.type == L_BMP;
  if (bmp && stored.size() >= 26 && stored[0] == 'B' && stored[1] == 'M') {
    f.w = rd32(&stored[18]);
    f.h = (int32_t)rd32(&stored[22]);
  } else {
    jpegDims(stored, f.w, f.h);
  }
  for (auto& l : sd.layers) {
    if (l.type == L_T1) {
      RBuf r(l.recon);
      f.t1s = (int64_t)r.var();
      f.t1n = (int64_t)r.var();
    }
    if (l.type == L_OBJSTM) {
      RBuf r(l.recon);
      f.objstmN = (int64_t)r.var();
    }
  }
  if (!wantDecoded && !wantMd5 && f.objstmN < 0) return;
  // decoded data: stored form with the pseudo layers (innermost) undone
  Bytes cur = stored;
  for (size_t k = sd.layers.size(); k-- > 0 && isPseudoLayer(sd.layers[k].type);) {
    Bytes out;
    if (!layerEncode(ctx, sd.layers[k].type, sd.layers[k].recon, cur, out)) return;
    if (sd.layers[k].type == L_OBJSTM) f.objstmFirst = (int64_t)(out.size() - cur.size());
    cur.swap(out);
  }
  f.decodedLen = cur.size();
  f.decodedKnown = true;
  if (wantMd5) {
    md5(cur.data(), cur.size(), f.md5);
    f.md5Known = true;
  }
}

// ---- raw-text walker ----------------------------------------------------------------------------
// Reports values by key path: "/Length", "/Params/Size", "startxref" (token after that keyword),
// "@" (an integer that is the whole value of an indirect object). Values: INT token, REF (first
// number of "N G R"), HEX (digits of a hex string, without the brackets).
struct DerivTok {
  std::string path;
  enum Kind { INT, REF, HEX } kind;
  size_t a, b;  // token range (HEX: the digits)
  uint64_t v;   // INT value / REF object number
};

static void derivWalk(const uint8_t* p, size_t n, std::vector<DerivTok>& out) {
  struct Frame { bool dict; std::string path, key; bool expectKey; };
  std::vector<Frame> st;
  size_t i = 0;
  bool afterObj = false, afterStartxref = false;
  auto skipWS = [&]() {
    while (i < n) {
      if (isWS(p[i])) i++;
      else if (p[i] == '%') { while (i < n && p[i] != '\r' && p[i] != '\n') i++; }
      else break;
    }
  };
  auto valueDone = [&]() {
    if (!st.empty() && st.back().dict) st.back().expectKey = true;
  };
  auto curPath = [&]() -> std::string {
    if (st.empty() || !st.back().dict || st.back().expectKey) return "";
    return st.back().path + st.back().key;
  };
  auto intAt = [&](size_t a, size_t& b, uint64_t& v) {
    b = a;
    v = 0;
    while (b < n && p[b] >= '0' && p[b] <= '9' && b - a < 19) v = v * 10 + (p[b++] - '0');
    return b > a && (b >= n || !isRegular(p[b]));
  };
  while (true) {
    skipWS();
    if (i >= n) break;
    uint8_t c = p[i];
    if (c == '(') {
      int depth = 1;
      i++;
      while (i < n && depth) {
        if (p[i] == '\\') { i += 2; continue; }
        if (p[i] == '(') depth++;
        else if (p[i] == ')') depth--;
        i++;
      }
      valueDone();
      afterObj = afterStartxref = false;
      continue;
    }
    if (c == '<' && i + 1 < n && p[i + 1] == '<') {
      std::string path = (!st.empty() && !st.back().dict) ? std::string("[]") : curPath();
      st.push_back({true, path, "", true});
      i += 2;
      afterObj = afterStartxref = false;
      continue;
    }
    if (c == '<') {
      size_t a = i + 1, b = a;
      while (b < n && p[b] != '>') b++;
      std::string path = curPath();
      if (!path.empty() && b < n) out.push_back({path, DerivTok::HEX, a, b, 0});
      i = b < n ? b + 1 : n;
      valueDone();
      afterObj = afterStartxref = false;
      continue;
    }
    if (c == '>' && i + 1 < n && p[i + 1] == '>') {
      if (!st.empty()) st.pop_back();
      i += 2;
      valueDone();
      continue;
    }
    if (c == '[') {
      st.push_back({false, "", "", false});
      i++;
      continue;
    }
    if (c == ']') {
      if (!st.empty() && !st.back().dict) st.pop_back();
      i++;
      valueDone();
      continue;
    }
    if (c == '/') {
      size_t b = i + 1;
      while (b < n && isRegular(p[b])) b++;
      if (!st.empty() && st.back().dict && st.back().expectKey) {
        st.back().key.assign((const char*)p + i, b - i);
        st.back().expectKey = false;
      } else {
        valueDone();
      }
      i = b;
      afterObj = afterStartxref = false;
      continue;
    }
    if (!isRegular(c)) { i++; continue; }
    size_t b = i;
    while (b < n && isRegular(p[b])) b++;
    if (c >= '0' && c <= '9') {
      uint64_t v;
      size_t e;
      bool isInt = intAt(i, e, v) && e == b;
      std::string path = curPath();
      if (isInt) {
        // reference?
        size_t save = i;
        i = b;
        skipWS();
        size_t g0 = i, g1;
        uint64_t g;
        bool ref = false;
        if (intAt(g0, g1, g)) {
          i = g1;
          skipWS();
          if (i < n && p[i] == 'R' && (i + 1 >= n || !isRegular(p[i + 1]))) {
            ref = true;
            i++;
          }
        }
        if (ref) {
          if (!path.empty()) out.push_back({path, DerivTok::REF, save, b, v});
          valueDone();
          afterObj = afterStartxref = false;
          continue;
        }
        i = save;
        if (!path.empty()) out.push_back({path, DerivTok::INT, i, b, v});
        else if (afterStartxref) out.push_back({"startxref", DerivTok::INT, i, b, v});
        else if (afterObj && st.empty()) {
          // whole value of an indirect object if "endobj" follows
          size_t q = b;
          while (q < n && isWS(p[q])) q++;
          if (q + 6 <= n && memcmp(p + q, "endobj", 6) == 0) out.push_back({"@", DerivTok::INT, i, b, v});
        }
      }
      i = b;
      valueDone();
      afterObj = afterStartxref = false;
      continue;
    }
    afterObj = (b - i == 3 && memcmp(p + i, "obj", 3) == 0);
    afterStartxref = (b - i == 9 && memcmp(p + i, "startxref", 9) == 0);
    i = b;
    if (!afterObj && !afterStartxref) valueDone();
  }
}

// /Length N G R in a stream dictionary: the referenced object number
static bool lengthRefOf(const uint8_t* p, size_t n, uint32_t& num) {
  std::vector<DerivTok> toks;
  derivWalk(p, n, toks);
  for (auto& t : toks)
    if (t.path == "/Length" && t.kind == DerivTok::REF && t.v <= 0xFFFFFFFFull) {
      num = (uint32_t)t.v;
      return true;
    }
  return false;
}

static std::string decStr(uint64_t v) {
  char b[24];
  snprintf(b, sizeof(b), "%llu", (unsigned long long)v);
  return b;
}

static std::string md5Hex(const uint8_t m[16], bool lower) {
  static const char* U = "0123456789ABCDEF";
  static const char* L = "0123456789abcdef";
  std::string s;
  for (int k = 0; k < 16; k++) {
    s += (lower ? L : U)[m[k] >> 4];
    s += (lower ? L : U)[m[k] & 15];
  }
  return s;
}

// value a code stands for; false if not available
static bool derivValue(uint8_t code, const StreamFacts& f, const DeriveState& ds, uint32_t objNum, std::string& v) {
  switch (code) {
    case 'L': if (!f.payloadKnown) return false; v = decStr(f.payloadLen); return true;
    case 'D': if (!f.decodedKnown) return false; v = decStr(f.decodedLen); return true;
    case '1': if (f.t1s < 0) return false; v = decStr((uint64_t)f.t1s); return true;
    case '2': if (f.t1n < 0) return false; v = decStr((uint64_t)f.t1n); return true;
    case '3':
      if (f.t1s < 0 || !f.decodedKnown || (uint64_t)(f.t1s + f.t1n) > f.decodedLen) return false;
      v = decStr(f.decodedLen - (uint64_t)(f.t1s + f.t1n));
      return true;
    case 'N': if (f.objstmN < 0) return false; v = decStr((uint64_t)f.objstmN); return true;
    case 'F': if (f.objstmFirst < 0) return false; v = decStr((uint64_t)f.objstmFirst); return true;
    case 'W': if (f.w < 0) return false; v = decStr((uint64_t)f.w); return true;
    case 'H': if (f.h < 0) return false; v = decStr((uint64_t)f.h); return true;
    case 'C':
    case 'c': if (!f.md5Known) return false; v = md5Hex(f.md5, code == 'c'); return true;
    case 'S': if (ds.xrefPos.empty()) return false; v = decStr(ds.xrefPos.back()); return true;
    case 'P': if (ds.xrefPos.size() < 2) return false; v = decStr(ds.xrefPos[ds.xrefPos.size() - 2]); return true;
    case 'p': if (ds.xrefPos.empty()) return false; v = decStr(ds.xrefPos.back()); return true;
    case 'Z': if (ds.lastXrefEnd < 0) return false; v = decStr((uint64_t)ds.lastXrefEnd); return true;
    case 'X': if (ds.lastXRefStm < 0) return false; v = decStr((uint64_t)ds.lastXRefStm); return true;
    case 'l': {
      auto it = ds.lenRef.find(objNum);
      if (it == ds.lenRef.end()) return false;
      v = decStr(it->second);
      return true;
    }
  }
  return false;
}

// which facts the placeholders of a text need: bit0 decoded data, bit1 md5
static int derivNeeds(const uint8_t* p, size_t n) {
  int need = 0;
  for (size_t i = 0; i + 1 < n; i++)
    if (p[i] == kDerivEsc) {
      uint8_t c = p[i + 1];
      if (c == 'D' || c == '3' || c == 'F') need |= 1;
      if (c == 'C' || c == 'c') need |= 3;
      i++;
    }
  return need;
}

// encoder: text with the chosen tokens replaced and literal 0x04 escaped
static void derivEncode(const uint8_t* p, size_t n, std::vector<std::pair<std::pair<size_t, size_t>, uint8_t>> edits,
                        Bytes& out) {
  std::sort(edits.begin(), edits.end());
  out.clear();
  size_t k = 0;
  for (size_t i = 0; i < n;) {
    if (k < edits.size() && edits[k].first.first == i) {
      out.push_back(kDerivEsc);
      out.push_back(edits[k].second);
      i = edits[k].first.second;
      k++;
      continue;
    }
    if (p[i] == kDerivEsc) out.push_back(kDerivEsc);
    out.push_back(p[i++]);
  }
}

// decoder: placeholders resolved
static bool derivDecode(const uint8_t* p, size_t n, const StreamFacts& f, const DeriveState& ds, uint32_t objNum,
                        Bytes& out) {
  out.clear();
  for (size_t i = 0; i < n; i++) {
    if (p[i] != kDerivEsc) { out.push_back(p[i]); continue; }
    if (++i >= n) return false;
    if (p[i] == kDerivEsc) { out.push_back(kDerivEsc); continue; }
    std::string v;
    if (!derivValue(p[i], f, ds, objNum, v)) return false;
    out.insert(out.end(), v.begin(), v.end());
  }
  return true;
}

// encoder: choose placeholders for one item text. kind: 0 text item, 1 object item, 2 stream prefix
static void derivChoose(const uint8_t* p, size_t n, int kind, uint8_t cls, const StreamFacts& f, const DeriveState& ds,
                        uint32_t objNum, std::vector<std::pair<std::pair<size_t, size_t>, uint8_t>>& edits) {
  std::vector<DerivTok> toks;
  derivWalk(p, n, toks);
  auto tokStr = [&](const DerivTok& t) { return std::string((const char*)p + t.a, t.b - t.a); };
  auto tryCodes = [&](const DerivTok& t, std::initializer_list<uint8_t> codes) {
    std::string s = tokStr(t), v;
    for (uint8_t c : codes)
      if (derivValue(c, f, ds, objNum, v) && v == s) {
        edits.push_back({{t.a, t.b}, c});
        return;
      }
  };
  for (auto& t : toks) {
    if (t.kind == DerivTok::REF) continue;
    if (kind == 2) {
      if (t.kind == DerivTok::INT) {
        if (t.path == "/Length") tryCodes(t, {'L'});
        else if (t.path == "/DL") tryCodes(t, {'D'});
        else if (t.path == "/Length1") tryCodes(t, {'1', 'D'});
        else if (t.path == "/Length2") tryCodes(t, {'2'});
        else if (t.path == "/Length3") tryCodes(t, {'3'});
        else if (t.path == "/N" && cls == SC_OBJSTM) tryCodes(t, {'N'});
        else if (t.path == "/First" && cls == SC_OBJSTM) tryCodes(t, {'F'});
        else if (t.path == "/Width") tryCodes(t, {'W'});
        else if (t.path == "/Height") tryCodes(t, {'H'});
        else if (t.path == "/Params/Size") tryCodes(t, {'D'});
        else if (t.path == "/Prev" && cls == SC_XREF) tryCodes(t, {'p'});
      } else if (t.path == "/Params/CheckSum" && t.b - t.a == 32) {
        tryCodes(t, {'C', 'c'});
      }
    } else if (kind == 1) {
      if (t.kind == DerivTok::INT && t.path == "@") tryCodes(t, {'l'});
    } else {
      if (t.kind != DerivTok::INT) continue;
      if (t.path == "startxref") tryCodes(t, {'S'});
      else if (t.path == "/Prev") tryCodes(t, {'P'});
      else if (t.path == "/Size") tryCodes(t, {'Z'});
      else if (t.path == "/XRefStm") tryCodes(t, {'X'});
    }
  }
}
