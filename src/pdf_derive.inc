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
//   N F  ObjStm /N /First                   W H  image /Width /Height, /DecodeParms /Columns /Rows (BMP / TIFF header, JPEG SOF)
//   C c  EmbeddedFile /Params /CheckSum: MD5 of the decoded data, upper / lower case hex digits
//   S  startxref value = start of the latest xref section
//   P  /Prev = start of the xref section before the current one
//   Z  trailer /Size = end of the latest classic xref section (max start + count)
//   X  /XRefStm = start of the latest XRef stream object
//   e  encrypted string, stored as plaintext   f  forward slot   G  signature blob (.p7s)
//   B  base64 blob (.b64, .b64r)

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
  bool bmp = false, tif = false;
  uint32_t tw, th;
  for (auto& l : sd.layers) bmp |= l.type == L_BMP || l.type == L_RGBA, tif |= l.type == L_TIFW;
  if (bmp && stored.size() >= 26 && stored[0] == 'B' && stored[1] == 'M') {
    f.w = rd32(&stored[18]);
    f.h = (int32_t)rd32(&stored[22]);
  } else if (tif && tiffTag(stored, 256, tw) && tiffTag(stored, 257, th)) {
    f.w = tw;
    f.h = th;
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
  for (size_t k = sd.layers.size(); k-- > 0 && isPseudoLayerType(sd.layers[k].type);) {
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
    if (!st.empty() && !st.back().dict) return st.back().path;  // "/Key[]" for elements of a keyed array
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
      std::string path = curPath();
      st.push_back({false, path.empty() ? std::string() : path + "[]", "", false});
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

struct DerivEdit {
  size_t a, b;  // replaced token range
  Bytes rep;    // placeholder (0x04, code, payload)
  bool operator<(const DerivEdit& o) const { return a < o.a; }
};

// encoder: text with the chosen tokens replaced and literal 0x04 escaped
static void derivEncode(const uint8_t* p, size_t n, std::vector<DerivEdit> edits, Bytes& out) {
  std::sort(edits.begin(), edits.end());
  out.clear();
  size_t k = 0;
  for (size_t i = 0; i < n;) {
    if (k < edits.size() && edits[k].a == i) {
      out.insert(out.end(), edits[k].rep.begin(), edits[k].rep.end());
      i = edits[k].b;
      k++;
      continue;
    }
    if (p[i] == kDerivEsc) out.push_back(kDerivEsc);
    out.push_back(p[i++]);
  }
}

// ---- encrypted strings (plan 8) --------------------------------------------------------------
// Placeholder 0x04 'e' u8 info, var length, plaintext [, 16-byte IV for AES]. info: bits 0-2
// spelling policy (0-3 literal, 4 hex upper case, 5 hex lower case), bits 4-5 cipher.
// Literal policies escape ( ) \ and: 0 nothing else; 1 CR as \r; 2 CR, LF as \r \n;
// 3 every byte < 32 or >= 127 as 3-digit octal.

struct DerivCrypt {
  const Bytes* fileKey = nullptr;
  uint32_t num = 0, gen = 0;
};

static void strRender(const Bytes& c, int policy, Bytes& out) {
  static const char* U = "0123456789ABCDEF";
  static const char* L = "0123456789abcdef";
  if (policy >= 4) {
    out.push_back('<');
    for (uint8_t b : c) {
      out.push_back((uint8_t)(policy == 5 ? L : U)[b >> 4]);
      out.push_back((uint8_t)(policy == 5 ? L : U)[b & 15]);
    }
    out.push_back('>');
    return;
  }
  out.push_back('(');
  for (uint8_t b : c) {
    if (b == '(' || b == ')' || b == '\\') { out.push_back('\\'); out.push_back(b); }
    else if (policy == 3 && (b < 32 || b >= 127)) {
      out.push_back('\\');
      out.push_back((uint8_t)('0' + (b >> 6)));
      out.push_back((uint8_t)('0' + ((b >> 3) & 7)));
      out.push_back((uint8_t)('0' + (b & 7)));
    } else if ((policy == 1 || policy == 2) && b == '\r') { out.push_back('\\'); out.push_back('r'); }
    else if (policy == 2 && b == '\n') { out.push_back('\\'); out.push_back('n'); }
    else out.push_back(b);
  }
  out.push_back(')');
}

// string token ranges of an object's text
static void stringTokens(const uint8_t* p, size_t n, std::vector<std::pair<size_t, size_t>>& out) {
  size_t i = 0;
  while (i < n) {
    uint8_t c = p[i];
    if (c == '%') { while (i < n && p[i] != '\r' && p[i] != '\n') i++; continue; }
    if (c == '(') {
      size_t a = i++;
      int depth = 1;
      while (i < n && depth) {
        if (p[i] == '\\') { i += 2; continue; }
        if (p[i] == '(') depth++;
        else if (p[i] == ')') depth--;
        i++;
      }
      if (!depth && i <= n) out.push_back({a, i});
      continue;
    }
    if (c == '<') {
      if (i + 1 < n && p[i + 1] == '<') { i += 2; continue; }
      size_t a = i++;
      while (i < n && p[i] != '>') i++;
      if (i < n) out.push_back({a, ++i});
      continue;
    }
    i++;
  }
}

// encoder: placeholders for the encrypted strings of one object
static void cryptStrings(const uint8_t* p, size_t n, uint8_t cipher, const DerivCrypt& dc, std::vector<DerivEdit>& edits) {
  std::vector<std::pair<size_t, size_t>> toks;
  stringTokens(p, n, toks);
  size_t taken = edits.size();
  for (auto& t : toks) {
    bool overlap = false;
    for (size_t k = 0; k < taken; k++) overlap |= edits[k].a < t.second && t.first < edits[k].b;
    if (overlap) continue;
    Lexer lx(p, t.second, t.first);
    Obj o;
    if (!lx.parse(o) || o.t != Obj::STR || lx.pos != t.second) continue;
    Bytes ct(o.s.begin(), o.s.end()), pt;
    uint8_t iv[16] = {};
    if (!cipherDecrypt(cipher, *dc.fileKey, dc.num, dc.gen, ct.data(), ct.size(), pt, iv)) continue;
    Bytes re;
    cipherEncrypt(cipher, *dc.fileKey, dc.num, dc.gen, iv, pt, re);
    if (re != ct) continue;
    int policy = -1;
    for (int pol = o.hex ? 4 : 0; pol < (o.hex ? 6 : 4) && policy < 0; pol++) {
      Bytes r;
      strRender(ct, pol, r);
      if (r.size() == t.second - t.first && memcmp(r.data(), p + t.first, r.size()) == 0) policy = pol;
    }
    if (policy < 0) continue;
    DerivEdit e{t.first, t.second, {}};
    e.rep.push_back(kDerivEsc);
    e.rep.push_back('e');
    e.rep.push_back((uint8_t)(policy | (cipher << 4)));
    putLenBytesTo(e.rep, pt.data(), pt.size());
    if (cipher == CI_AES128 || cipher == CI_AES256) e.rep.insert(e.rep.end(), iv, iv + 16);
    edits.push_back(std::move(e));
  }
}

// ---- signature blobs (plan M10) -----------------------------------------------------------------
// A hex /Contents string holding one DER SEQUENCE followed only by zero bytes (the space reserved
// in a signature dictionary): placeholder 0x04 'G' u8 case (0 upper, 1 lower case hex digits),
// var count of zero bytes after the DER element. The DER element itself goes to the .p7s aux file
// (elements concatenated in file order; each one is self-delimiting).

// size of the DER SEQUENCE at b, 0 if there is none that fits in n bytes
static size_t derLength(const uint8_t* b, size_t n) {
  if (n < 2 || b[0] != 0x30) return 0;
  size_t hl = 2, len = b[1];
  if (len & 0x80) {
    size_t k = len & 0x7F;
    if (k == 0 || k > 4 || n < 2 + k) return 0;
    len = 0;
    for (size_t i = 0; i < k; i++) len = len << 8 | b[2 + i];
    hl += k;
  }
  if (len > n - hl) return 0;
  return hl + len;
}

struct SigBlobs {
  const Bytes* data = nullptr;
  size_t pos = 0;
};

// encoder: placeholders for the signature blobs of one object; DER elements appended to p7s
static void sigStrings(const uint8_t* p, size_t n, std::vector<DerivEdit>& edits, Bytes& p7s) {
  std::vector<std::pair<size_t, size_t>> toks;
  stringTokens(p, n, toks);
  for (auto& t : toks) {
    if (p[t.first] != '<' || t.second - t.first < 2 + 64) continue;
    size_t k = t.first;
    while (k > 0 && isWS(p[k - 1])) k--;
    if (k < 9 || memcmp(p + k - 9, "/Contents", 9) != 0) continue;
    size_t a = t.first + 1, b = t.second - 1;
    if ((b - a) % 2) continue;
    bool upper = false, lower = false, ok = true;
    Bytes v;
    v.reserve((b - a) / 2);
    for (size_t i = a; i < b && ok; i += 2) {
      int h = hexVal(p[i]), l = hexVal(p[i + 1]);
      if (h < 0 || l < 0) ok = false;
      for (size_t j = i; j < i + 2; j++) {
        lower |= p[j] >= 'a' && p[j] <= 'f';
        upper |= p[j] >= 'A' && p[j] <= 'F';
      }
      v.push_back((uint8_t)(h * 16 + l));
    }
    if (!ok || (upper && lower)) continue;
    size_t der = derLength(v.data(), v.size());
    if (!der) continue;
    size_t z = der;
    while (z < v.size() && v[z] == 0) z++;
    if (z != v.size()) continue;
    DerivEdit e{t.first, t.second, {kDerivEsc, 'G', (uint8_t)(lower ? 1 : 0)}};
    putVarTo(e.rep, v.size() - der);
    edits.push_back(std::move(e));
    p7s.insert(p7s.end(), v.begin(), v.begin() + der);
  }
}

// ---- base64 blobs in object text ------------------------------------------------------------------
// A run of >= 1024 base64 characters (not all hex digits) that decodes and re-encodes to itself
// (JavaScript data, data: URIs, ...) and either decodes to a zlib stream or sits in quotes
// ("...", '...', or after the comma of a data: URI): placeholder 0x04 'B' u8 flags; bit0 = the decoded bytes were
// a zlib/deflate stream, stored inflated (L_FLATE recon in .b64r), bit1 = no '=' padding. Data of
// 64 KB or more gets a file of its own (<P>b%07X.<ext>, ext by signature: elf, jpg, ...), smaller
// data goes to the .b64 aux file. .b64r holds per blob: var data length, u8 own file, [ext with
// its length,] [Flate recon with its length].
static const size_t kBlobFileMin = 64 * 1024;

static const char kB64Alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static int b64Val(uint8_t c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}

static void b64Encode(const uint8_t* p, size_t n, bool pad, Bytes& out) {
  size_t i = 0;
  for (; i + 3 <= n; i += 3) {
    uint32_t v = (uint32_t)p[i] << 16 | (uint32_t)p[i + 1] << 8 | p[i + 2];
    for (int k = 18; k >= 0; k -= 6) out.push_back((uint8_t)kB64Alpha[(v >> k) & 63]);
  }
  if (n - i == 1) {
    uint32_t v = (uint32_t)p[i] << 16;
    out.push_back((uint8_t)kB64Alpha[v >> 18]);
    out.push_back((uint8_t)kB64Alpha[(v >> 12) & 63]);
    if (pad) { out.push_back('='); out.push_back('='); }
  } else if (n - i == 2) {
    uint32_t v = (uint32_t)p[i] << 16 | (uint32_t)p[i + 1] << 8;
    out.push_back((uint8_t)kB64Alpha[v >> 18]);
    out.push_back((uint8_t)kB64Alpha[(v >> 12) & 63]);
    out.push_back((uint8_t)kB64Alpha[(v >> 6) & 63]);
    if (pad) out.push_back('=');
  }
}

// n alphabet characters (no '='); false if n % 4 == 1 or the unused low bits are not zero
static bool b64Decode(const uint8_t* p, size_t n, Bytes& out) {
  if (n % 4 == 1) return false;
  out.clear();
  out.reserve(n / 4 * 3 + 2);
  uint32_t acc = 0;
  int bits = 0;
  for (size_t i = 0; i < n; i++) {
    acc = acc << 6 | (uint32_t)b64Val(p[i]);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      out.push_back((uint8_t)(acc >> bits));
      acc &= (1u << bits) - 1;
    }
  }
  return acc == 0;
}

struct B64Blobs {
  const Bytes* data = nullptr;
  const Bytes* rec = nullptr;
  size_t dpos = 0, rpos = 0, index = 0;
  const std::vector<Bytes>* fastZ = nullptr;  // encoder verify: the original zlib bytes per blob
  const CodecCtx* ctx = nullptr;
  std::function<Bytes(uint32_t, const std::string&)> file;  // blob file k with its extension
  uint32_t nextFile = 0;
};

// encoder: placeholders for the base64 runs of one object; data and recon appended
static void b64Strings(const CodecCtx& ctx, const uint8_t* p, size_t n, std::vector<DerivEdit>& edits, Bytes& data,
                       Bytes& rec, std::vector<Bytes>* orig, uint64_t& blobBytes, std::vector<Bytes>& files,
                       std::vector<std::string>& fileExt) {
  auto put = [&](Bytes& v) {
    putVarTo(rec, v.size());
    blobBytes += v.size();
    if (v.size() >= kBlobFileMin) {
      const char* e = sniffExt(v.data(), v.size());
      std::string ext = e ? e : "bin";
      rec.push_back(1);
      putLenBytesTo(rec, (const uint8_t*)ext.data(), ext.size());
      fileExt.push_back(ext);
      files.push_back(std::move(v));
    } else {
      rec.push_back(0);
      data.insert(data.end(), v.begin(), v.end());
    }
  };
  size_t taken = edits.size();
  size_t i = 0;
  while (i < n) {
    if (b64Val(p[i]) < 0) { i++; continue; }
    size_t a = i;
    bool hexOnly = true;
    while (i < n && b64Val(p[i]) >= 0) {
      hexOnly &= hexVal(p[i]) >= 0;
      i++;
    }
    size_t b = i, len = b - a;
    size_t eq = 0;
    while (i < n && p[i] == '=' && eq < 2) { i++; eq++; }
    if (len < 1024 || hexOnly) continue;
    if (eq && (len + eq) % 4) continue;
    bool overlap = false;
    for (size_t k = 0; k < taken; k++) overlap |= edits[k].a < i && a < edits[k].b;
    if (overlap) continue;
    Bytes dec, back;
    if (!b64Decode(p + a, len, dec)) continue;
    bool pad = eq > 0 || len % 4 == 0;
    b64Encode(dec.data(), dec.size(), pad, back);
    if (back.size() != i - a || memcmp(back.data(), p + a, back.size()) != 0) continue;
    uint8_t flags = pad ? 0 : 2;
    Bytes out, recon;
    if (dec.size() >= 2 && zlibHeaderValid(dec[0], dec[1]) &&
        layerDecode(ctx, L_FLATE, FilterParams(), dec, out, recon)) {
      flags |= 1;
      put(out);
      putLenBytesTo(rec, recon.data(), recon.size());
      if (orig) orig->push_back(std::move(dec));
    } else {
      // not a zlib stream: only quoted runs ("...", '...', data:...;base64,...") count as base64;
      // a run of name characters such as a /CharSet string (/A/B/C...) also matches the alphabet
      uint8_t prev = a ? p[a - 1] : 0, next = i < n ? p[i] : 0;
      if (!(prev == '"' || prev == '\'' || prev == ',') || !(next == '"' || next == '\'')) continue;
      put(dec);
      if (orig) orig->push_back(Bytes());
    }
    edits.push_back({a, i, {kDerivEsc, 'B', flags}});
  }
}

// ---- forward slots (plan 3.6, M9) ---------------------------------------------------------------
// Positions that point at later parts of the file (linearization dictionary, first-page trailer and
// xref): placeholder 0x04 'f' u8 width, u8 mode, var target, zigzag delta. The decoder writes
// `width` zeros and patches them once every item position is known. mode bit0: 0 = start of the
// target item + delta, 1 = length of the target item + delta; bit1: zero-padded. target 0 = end of
// file, k + 1 = item k.
struct FwdPatch {
  uint64_t pos;
  uint8_t width, mode;
  uint64_t target;
  int64_t delta;
};

static bool fwdApply(Bytes& out, const std::vector<uint64_t>& itemStart, const std::vector<FwdPatch>& patches,
                     uint64_t bias = 0) {
  for (auto& p : patches) {
    uint64_t v;
    if (p.target == 0) v = out.size();
    else {
      uint64_t k = p.target - 1;
      if (k >= itemStart.size()) return false;
      if (p.mode & 1) {
        uint64_t e = k + 1 < itemStart.size() ? itemStart[k + 1] : out.size();
        v = e - itemStart[k];
      } else {
        v = itemStart[k] - bias;
      }
    }
    if (p.target == 0) v -= bias;
    v += (uint64_t)p.delta;
    char buf[32];
    int l = (p.mode & 2) ? snprintf(buf, sizeof(buf), "%0*llu", p.width, (unsigned long long)v)
                         : snprintf(buf, sizeof(buf), "%llu", (unsigned long long)v);
    if (l != p.width || p.pos + l > out.size()) return false;
    memcpy(out.data() + p.pos, buf, l);
  }
  return true;
}

// decoder: placeholders resolved
static bool derivDecode(const uint8_t* p, size_t n, const StreamFacts& f, const DeriveState& ds, uint32_t objNum,
                        Bytes& out, const DerivCrypt* dc = nullptr, std::vector<FwdPatch>* fwd = nullptr,
                        SigBlobs* sig = nullptr, B64Blobs* b64 = nullptr) {
  out.clear();
  for (size_t i = 0; i < n; i++) {
    if (p[i] != kDerivEsc) { out.push_back(p[i]); continue; }
    if (++i >= n) return false;
    if (p[i] == kDerivEsc) { out.push_back(kDerivEsc); continue; }
    if (p[i] == 'f') {
      if (!fwd) return false;
      Bytes rest(p + i + 1, p + n);
      RBuf r(rest);
      FwdPatch fp;
      fp.width = r.u8();
      fp.mode = r.u8();
      fp.target = r.var();
      fp.delta = unzigzag(r.var());
      if (fp.width == 0 || fp.width > 20) return false;
      fp.pos = out.size();  // relative to this text; the caller rebases
      out.insert(out.end(), fp.width, '0');
      fwd->push_back(fp);
      i += r.p;
      continue;
    }
    if (p[i] == 'B') {
      if (!b64 || !b64->data || !b64->rec || i + 1 >= n) return false;
      uint8_t flags = p[++i];
      if (flags > 3) return false;
      RBuf rr(*b64->rec);
      rr.p = b64->rpos;
      uint64_t dl = rr.var();
      Bytes v;
      if (rr.u8()) {
        Bytes ext;
        rr.bytes(rr.len(), ext);
        if (!b64->file) return false;
        v = b64->file(b64->nextFile++, std::string(ext.begin(), ext.end()));
        if (v.size() != dl) return false;
      } else {
        if (dl > b64->data->size() - b64->dpos) return false;
        v.assign(b64->data->begin() + b64->dpos, b64->data->begin() + b64->dpos + dl);
        b64->dpos += dl;
      }
      if (flags & 1) {
        Bytes rc, z;
        rr.bytes(rr.len(), rc);
        if (b64->fastZ && b64->index < b64->fastZ->size() && !(*b64->fastZ)[b64->index].empty())
          z = (*b64->fastZ)[b64->index];
        else if (!b64->ctx || !layerEncode(*b64->ctx, L_FLATE, rc, v, z))
          return false;
        v.swap(z);
      }
      b64->rpos = rr.p;
      b64->index++;
      b64Encode(v.data(), v.size(), !(flags & 2), out);
      continue;
    }
    if (p[i] == 'G') {
      if (!sig || !sig->data || i + 1 >= n) return false;
      int lowerCase = p[++i];
      if (lowerCase > 1) return false;
      Bytes rest(p + i + 1, p + std::min(n, i + 12));
      RBuf r(rest);
      uint64_t pad = r.var();
      const Bytes& s = *sig->data;
      size_t der = derLength(s.data() + sig->pos, s.size() - sig->pos);
      if (!der || pad > ((uint64_t)1 << 30)) return false;
      Bytes v(s.begin() + sig->pos, s.begin() + sig->pos + der);
      sig->pos += der;
      v.insert(v.end(), (size_t)pad, 0);
      strRender(v, 4 + lowerCase, out);
      i += r.p;
      continue;
    }
    if (p[i] == 'e') {
      if (!dc || !dc->fileKey || dc->fileKey->empty() || i + 1 >= n) return false;
      uint8_t info = p[++i];
      uint8_t cipher = info >> 4;
      int policy = info & 7;
      if (cipher < CI_RC4 || cipher > CI_AES256 || policy > 5) return false;
      Bytes rest(p + i + 1, p + n);
      RBuf r(rest);
      Bytes pt;
      r.bytes(r.len(), pt);
      uint8_t iv[16] = {};
      if (cipher != CI_RC4) {
        Bytes ivb;
        r.bytes(16, ivb);
        memcpy(iv, ivb.data(), 16);
      }
      Bytes ct;
      cipherEncrypt(cipher, *dc->fileKey, dc->num, dc->gen, iv, pt, ct);
      strRender(ct, policy, out);
      i += r.p;
      continue;
    }
    std::string v;
    if (!derivValue(p[i], f, ds, objNum, v)) return false;
    out.insert(out.end(), v.begin(), v.end());
  }
  return true;
}

// encoder: anchors for forward slots (item starts in file order, file size)
struct FwdAnchors {
  std::vector<uint64_t> starts;  // item starts, ascending
  uint64_t fileSize = 0;
  uint64_t bias = 0;             // values are file offsets minus this
  // placeholder for position value v of a token with `width` digits; false if no anchor
  bool position(uint64_t v, size_t width, bool zeroPad, Bytes& rep) const {
    uint64_t target;
    int64_t delta;
    uint64_t pos = v + bias;  // position in the file
    if (pos == fileSize) { target = 0; delta = 0; }
    else {
      auto it = std::upper_bound(starts.begin(), starts.end(), pos);
      if (it == starts.begin()) return false;
      size_t k = (size_t)(it - starts.begin()) - 1;
      target = k + 1;
      delta = (int64_t)(pos - starts[k]);
      if (delta > 64) return false;
    }
    rep = {kDerivEsc, 'f', (uint8_t)width, (uint8_t)(zeroPad ? 2 : 0)};
    putVarTo(rep, target);
    putVarTo(rep, zigzag(delta));
    return true;
  }
  bool length(uint64_t itemPos, uint64_t v, size_t width, Bytes& rep) const {
    itemPos += bias;
    auto it = std::lower_bound(starts.begin(), starts.end(), itemPos);
    if (it == starts.end() || *it != itemPos) return false;
    size_t k = (size_t)(it - starts.begin());
    uint64_t e = k + 1 < starts.size() ? starts[k + 1] : fileSize;
    int64_t delta = (int64_t)(v - (e - itemPos));
    if (delta < -64 || delta > 64) return false;
    rep = {kDerivEsc, 'f', (uint8_t)width, 1};
    putVarTo(rep, k + 1);
    putVarTo(rep, zigzag(delta));
    return true;
  }
};

// encoder: forward slots for the linearization dictionary and forward-pointing trailer values
static void fwdChoose(const uint8_t* p, size_t n, int kind, const FwdAnchors& fa, std::vector<DerivEdit>& edits) {
  std::vector<DerivTok> toks;
  derivWalk(p, n, toks);
  bool lin = memfind(p, n, "/Linearized", 11) != nullptr;
  auto taken = [&](const DerivTok& t) {
    for (auto& e : edits)
      if (e.a == t.a) return true;
    return false;
  };
  uint64_t hintPos = UINT64_MAX;
  for (auto& t : toks) {
    if (t.kind != DerivTok::INT || taken(t) || t.b - t.a > 20) continue;
    bool want = false;
    if (lin && kind == 1 && (t.path == "/L" || t.path == "/E" || t.path == "/T")) want = true;
    if ((kind == 0 || kind == 2) && (t.path == "/Prev" || t.path == "startxref" || t.path == "/XRefStm")) want = true;
    if (lin && kind == 1 && t.path == "/H[]") {
      if (hintPos == UINT64_MAX) { hintPos = t.v; want = true; }
      else {
        Bytes rep;
        if (fa.length(hintPos, t.v, t.b - t.a, rep)) edits.push_back({t.a, t.b, rep});
        hintPos = UINT64_MAX - 1;
        continue;
      }
    }
    if (!want || t.v == 0) continue;
    Bytes rep;
    if (fa.position(t.v, t.b - t.a, false, rep)) {
      // the spelling must be the plain decimal of the value
      if (decStr(t.v).size() == t.b - t.a) edits.push_back({t.a, t.b, rep});
    }
  }
}

// encoder: choose placeholders for one item text. kind: 0 text item, 1 object item, 2 stream prefix
static void derivChoose(const uint8_t* p, size_t n, int kind, uint8_t cls, const StreamFacts& f, const DeriveState& ds,
                        uint32_t objNum, std::vector<DerivEdit>& edits) {
  std::vector<DerivTok> toks;
  derivWalk(p, n, toks);
  auto tokStr = [&](const DerivTok& t) { return std::string((const char*)p + t.a, t.b - t.a); };
  auto tryCodes = [&](const DerivTok& t, std::initializer_list<uint8_t> codes) {
    std::string s = tokStr(t), v;
    for (uint8_t c : codes)
      if (derivValue(c, f, ds, objNum, v) && v == s) {
        edits.push_back({t.a, t.b, Bytes{kDerivEsc, c}});
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
        else if (t.path == "/DecodeParms/Columns" || t.path == "/DecodeParms[]/Columns") tryCodes(t, {'W'});
        else if (t.path == "/DecodeParms/Rows" || t.path == "/DecodeParms[]/Rows") tryCodes(t, {'H'});
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
