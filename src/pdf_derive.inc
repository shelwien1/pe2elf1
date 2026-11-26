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
//   w  start of a /Widths or /W array whose widths come from the embedded font, stands for "[":
//      letters 'a' + kind, 'a' + map, 'a' + rule, then " <font> <FirstChar> " and for Type 1
//      " h<L_T1 recon in hex> ", for a CIDToGIDMap stream " <map> " (text, so that the number
//      lifter reads it as text and numbers and no payload byte opens a string or a comment).
//      A source is "o<ordinal>.<ext>" (own file) or "s<object number>" (a stream stored inline
//      in an earlier item).
//   v  the same as w, but after the "[" (stands for nothing): for "N G obj[" with no space, where
//      a placeholder in place of the "[" would hide the keyword from the decoder's header parse
//   x  a width of that array, as the font predicts it (see pdf_font.inc)
//   k  "N G R": the structure parent (/P) of this top-level object, from the /K of earlier objects
//   K  the same for ObjStm member k: " <k> " follows in decimal
//   n  start of a /ParentTree leaf's /Nums array, stands for "["; in ObjStm member k " <k> " follows
//   q  start of an array object that is a /Nums value, stands for "[": " <key> " follows, and in
//      ObjStm member k "<k> " (the decoder adds the /K of the members before k first); Q the same
//      after the "[" (see v)
//   r  "N G R" in it: the structure element of that MCID on the page whose /StructParents is the
//      key (array values), or the element whose OBJR names the object with that /StructParent

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
  const char* hx = lower ? L : U;  // (a local: GCC 13 UBSan builds miscompile "(c ? L : U)[i]")
  std::string s;
  for (int k = 0; k < 16; k++) {
    s += hx[m[k] >> 4];
    s += hx[m[k] & 15];
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

// which facts the placeholders of a text need: bit0 decoded data, bit1 md5. Every 0x04 is looked
// at without parsing the placeholders (a payload may hold or end in 0x04), so the answer can only
// ask for too much, never too little.
static int derivNeeds(const uint8_t* p, size_t n) {
  int need = 0;
  for (size_t i = 0; i + 1 < n; i++)
    if (p[i] == kDerivEsc) {
      uint8_t c = p[i + 1];
      if (c == 'D' || c == '3' || c == 'F') need |= 1;
      if (c == 'C' || c == 'c') need |= 3;
    }
  return need;
}

struct DerivEdit {
  size_t a, b;  // replaced token range
  Bytes rep;    // placeholder (0x04, code, payload)
  bool operator<(const DerivEdit& o) const { return a < o.a || (a == o.a && b < o.b); }  // insertions first
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
  const KeyBuf* fileKey = nullptr;
  uint32_t num = 0, gen = 0;
};

static void strRender(const Bytes& c, int policy, Bytes& out) {
  static const char* U = "0123456789ABCDEF";
  static const char* L = "0123456789abcdef";
  if (policy >= 4) {
    const char* hx = policy == 5 ? L : U;  // (a local: see md5Hex)
    out.push_back('<');
    for (uint8_t b : c) {
      out.push_back((uint8_t)hx[b >> 4]);
      out.push_back((uint8_t)hx[b & 15]);
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
    if (!cipherEncrypt(cipher, *dc.fileKey, dc.num, dc.gen, iv, pt, re) || re != ct) continue;
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

// ---- widths from embedded fonts (plan2 P8) ------------------------------------------------------
// streams the decoder keeps for width sources: stored inline (not own files), at most 64 KB, not
// object streams; the encoder makes the same choice when it names a source "s<num>"
static const size_t kInlineSourceMax = 65536;
static bool inlineSourceOk(const StreamDesc& sd, size_t size) {
  return (sd.storage == ST_BIN || sd.storage == ST_TXT) && sd.cls != SC_OBJSTM && size <= kInlineSourceMax;
}

static bool parseFontOf(int kind, const Bytes& b, const Bytes& t1recon, FontMetrics& m) {
  switch (kind) {
    case WK_TT: case WK_CID2: case WK_CID2M: return parseTrueType(b.data(), b.size(), m);
    case WK_CFF: return parseCff(b.data(), b.size(), m) && !m.cidKeyed;
    case WK_CID0: return parseCff(b.data(), b.size(), m) && m.cidKeyed;
    case WK_T1: return parseType1(b, t1recon, m);
  }
  return false;
}

// Font programs (and CIDToGIDMap streams) by source text; the metrics are parsed once.
struct DerivFonts {
  std::function<Bytes(uint32_t, const std::string&)> load;  // own file by ordinal and extension
  std::unordered_map<uint32_t, Bytes> inl;                   // inline streams seen so far
  FontCache cache;
  bool source(const std::string& src, Bytes& out) {
    if (src.size() < 2) return false;
    if (src[0] == 's') {
      uint64_t num = 0;
      for (size_t i = 1; i < src.size(); i++) {
        if (src[i] < '0' || src[i] > '9' || i > 10) return false;
        num = num * 10 + (src[i] - '0');
      }
      auto f = inl.find((uint32_t)num);
      if (f == inl.end() || num > 0xFFFFFFFFull) return false;
      out = f->second;
      return true;
    }
    if (src[0] != 'o' || !load) return false;
    size_t dot = src.find('.');
    if (dot == std::string::npos || dot < 2 || dot + 1 >= src.size() || dot > 11) return false;
    uint64_t ord = 0;
    for (size_t i = 1; i < dot; i++) {
      if (src[i] < '0' || src[i] > '9') return false;
      ord = ord * 10 + (src[i] - '0');
    }
    std::string ext = src.substr(dot + 1);
    for (char c : ext)
      if (!isalnum((unsigned char)c)) return false;
    if (ord > 0xFFFFFFFFull) return false;
    out = load((uint32_t)ord, ext);
    return true;
  }
  const FontMetrics* get(int kind, const std::string& font, const std::string& t1hex, const std::string& map) {
    std::string key = std::string(1, (char)('a' + kind)) + " " + font + " " + t1hex + " " + map;
    bool found;
    FontMetrics* e = cache.get(key, found);
    if (!found) {
      Bytes b, rec;
      if (kind == WK_T1) {
        if (t1hex.size() < 3 || t1hex[0] != 'h' || t1hex.size() % 2 == 0) return nullptr;
        for (size_t i = 1; i + 1 < t1hex.size(); i += 2) {
          int hi = hexVal((uint8_t)t1hex[i]), lo = hexVal((uint8_t)t1hex[i + 1]);
          if (hi < 0 || lo < 0) return nullptr;
          rec.push_back((uint8_t)(hi * 16 + lo));
        }
      }
      if (source(font, b) && parseFontOf(kind, b, rec, *e) && kind == WK_CID2M) {
        Bytes mb;
        if (!source(map, mb)) e->ok = false;
        else e->setCidMap(mb.data(), mb.size());
      }
    }
    return e->ok ? e : nullptr;
  }
};

struct WidthTok {
  int t;  // 0 number, '[' or ']'
  size_t a, b;
};

// tokens of the array that opens at p[a]; false unless it holds only numbers, brackets and spaces
static bool widthsTokens(const uint8_t* p, size_t n, size_t a, std::vector<WidthTok>& out) {
  out.clear();
  int depth = 0;
  for (size_t i = a; i < n;) {
    uint8_t c = p[i];
    if (isWS(c)) { i++; continue; }
    if (c == '[') { out.push_back({'[', i, i + 1}); depth++; i++; continue; }
    if (c == ']') {
      out.push_back({']', i, i + 1});
      i++;
      if (--depth == 0) return true;
      continue;
    }
    if (!widthNumStart(c)) return false;
    size_t j = i;
    while (j < n && isRegular(p[j])) j++;
    out.push_back({0, i, j});
    i = j;
  }
  return false;
}

// position of the value of `key` ("/Widths") in the outermost dictionary of an object's text;
// SIZE_MAX if absent. Strings, hex strings and comments are skipped.
static size_t topKeyValue(const uint8_t* p, size_t n, const char* key) {
  size_t kl = strlen(key);
  int dd = 0, ad = 0;
  for (size_t i = 0; i < n;) {
    uint8_t c = p[i];
    if (c == '(') {
      int depth = 1;
      for (i++; i < n && depth; i++) {
        if (p[i] == '\\') { i++; continue; }
        depth += p[i] == '(' ? 1 : p[i] == ')' ? -1 : 0;
      }
    } else if (c == '%') {
      while (i < n && p[i] != '\r' && p[i] != '\n') i++;
    } else if (c == '<' && i + 1 < n && p[i + 1] == '<') { dd++; i += 2; }
    else if (c == '>' && i + 1 < n && p[i + 1] == '>') { dd--; i += 2; }
    else if (c == '<') { while (i < n && p[i] != '>') i++; i++; }
    else if (c == '[') { ad++; i++; }
    else if (c == ']') { ad--; i++; }
    else if (c == '/') {
      size_t j = i + 1;
      while (j < n && isRegular(p[j])) j++;
      if (dd == 1 && ad == 0 && j - i == kl && memcmp(p + i, key, kl) == 0) {
        while (j < n && isWS(p[j])) j++;
        return j < n ? j : SIZE_MAX;
      }
      i = j;
    } else {
      i++;
    }
  }
  return SIZE_MAX;
}

// ---- structure parents (plan2 P8) ---------------------------------------------------------------
// child object number -> (parent number, parent generation), from every /K seen so far: a direct
// reference or the references in a /K array. Both sides add an object's /K after its own /P is
// done, top-level objects in item order and ObjStm members in member order.
struct StructMap {
  typedef std::pair<uint32_t, uint32_t> Ref;
  std::unordered_map<uint32_t, Ref> parent;
  // for /ParentTree: /StructParents -> page, /StructParent -> object, (page, MCID) -> element,
  // object named by an OBJR -> element
  std::unordered_map<uint32_t, uint32_t> spPage, spObj;
  std::unordered_map<uint64_t, Ref> mcElem;
  std::unordered_map<uint32_t, Ref> objrElem;
  bool parentTreeRef(bool arr, uint64_t key, uint64_t idx, Ref& r) const {
    if (key > 0xFFFFFFFFull) return false;
    if (arr) {
      auto pg = spPage.find((uint32_t)key);
      if (pg == spPage.end() || idx > 0xFFFFFFFFull) return false;
      auto e = mcElem.find((uint64_t)pg->second << 32 | idx);
      if (e == mcElem.end()) return false;
      r = e->second;
      return true;
    }
    auto o = spObj.find((uint32_t)key);
    if (o == spObj.end()) return false;
    auto e = objrElem.find(o->second);
    if (e == objrElem.end()) return false;
    r = e->second;
    return true;
  }
};

// text of one object value (no "N G obj" header)
static void structAddKids(const uint8_t* p, size_t n, uint32_t num, uint32_t gen, StructMap& sm) {
  bool any = false;
  for (const uint8_t* q = p; (q = memfind(q, n - (q - p), "/K", 2)) != nullptr; q += 2)
    if (q + 2 >= p + n || !isRegular(q[2])) { any = true; break; }
  bool sp = memfind(p, n, "/StructParent", 13) != nullptr;
  if (!any && !sp) return;
  Lexer lx(p, n, 0);
  Obj o;
  if (!lx.parse(o) || o.t != Obj::DICT) return;
  auto uint32Of = [](const Obj* v, uint32_t& x) {
    if (!v || v->t != Obj::INT || v->i < 0 || v->i > 0xFFFFFFFFll) return false;
    x = (uint32_t)v->i;
    return true;
  };
  uint32_t x;
  if (uint32Of(o.get("StructParents"), x)) sm.spPage[x] = num;
  if (uint32Of(o.get("StructParent"), x)) sm.spObj[x] = num;
  const Obj* k = o.get("K");
  if (!k) return;
  const Obj* pg = o.get("Pg");
  int64_t page = pg && pg->t == Obj::REF && pg->i >= 0 && pg->i <= 0xFFFFFFFFll ? pg->i : -1;
  auto add = [&](const Obj& r) {
    if (r.t == Obj::REF && r.i >= 0 && r.i <= 0xFFFFFFFFll) sm.parent[(uint32_t)r.i] = {num, gen};
    else if (r.t == Obj::INT && page >= 0 && r.i >= 0 && r.i <= 0xFFFFFFFFll)
      sm.mcElem[(uint64_t)page << 32 | (uint64_t)r.i] = {num, gen};
    else if (r.t == Obj::DICT) {
      const Obj* t = r.get("Type");
      const Obj* mcid = r.get("MCID");
      const Obj* p2 = r.get("Pg");
      int64_t pgx = p2 && p2->t == Obj::REF && p2->i >= 0 && p2->i <= 0xFFFFFFFFll ? p2->i : page;
      if (mcid && mcid->t == Obj::INT && mcid->i >= 0 && mcid->i <= 0xFFFFFFFFll && pgx >= 0 &&
          (!t || (t->t == Obj::NAME && t->s == "MCR")))
        sm.mcElem[(uint64_t)pgx << 32 | (uint64_t)mcid->i] = {num, gen};
      const Obj* ob = r.get("Obj");
      if (t && t->t == Obj::NAME && t->s == "OBJR" && ob && ob->t == Obj::REF && ob->i >= 0 && ob->i <= 0xFFFFFFFFll)
        sm.objrElem[(uint32_t)ob->i] = {num, gen};
    }
  };
  if (k->t == Obj::ARR) for (auto& e : k->a) add(e);
  else add(*k);
}

static std::string refStr(std::pair<uint32_t, uint32_t> r);

// Walks the /Nums array of a number tree leaf: key, value, key, value, ...; a value is "N G R",
// null, or an array of those. Encoder and decoder feed it the same tokens.
struct NumsWalk {
  int depth = 0, st = 0;  // st at depth 1: 0 expects a key, 1 a value
  int refPart = 0;        // inside "N G R": 1 after N, 2 after G
  uint64_t key = 0, idx = 0;
  bool done = false, bad = false;
  bool atElement() const { return refPart == 0 && (depth == 2 || (depth == 1 && st == 1)); }
  void elementDone() {
    if (depth == 2) idx++;
    else st = 0;
  }
  void open() {
    if (refPart) { bad = true; return; }
    depth++;
    if (depth == 1) return;
    if (depth == 2 && st == 1) { idx = 0; return; }
    bad = true;
  }
  void close() {
    if (refPart) { bad = true; return; }
    if (depth == 2) st = 0;
    else if (depth == 1 && st != 0) bad = true;
    if (--depth == 0) done = true;
    if (depth < 0) bad = true;
  }
  void integer(const uint8_t* t, size_t n) {
    if (depth == 1 && st == 0 && !refPart) {
      uint64_t v = 0;
      if (n == 0 || n > 10) { bad = true; return; }
      for (size_t i = 0; i < n; i++) {
        if (t[i] < '0' || t[i] > '9') { bad = true; return; }
        v = v * 10 + (t[i] - '0');
      }
      key = v;
      st = 1;
      return;
    }
    if (refPart == 0 && atElement()) refPart = 1;
    else if (refPart == 1) refPart = 2;
    else bad = true;
  }
  void word(const uint8_t* t, size_t n) {
    if (n == 1 && t[0] == 'R' && refPart == 2) { refPart = 0; elementDone(); return; }
    if (n == 4 && memcmp(t, "null", 4) == 0 && atElement()) { elementDone(); return; }
    bad = true;
  }
};

// encoder: placeholders for the /Nums array opening at v[a] (value text v, edits at base + offset);
// key >= 0: v[a] opens an array object that is the /Nums value of that key
static size_t numsChoose(const uint8_t* v, size_t n, size_t a, size_t base, const StructMap& sm,
                         std::vector<DerivEdit>& edits, int64_t key, long member, bool after = false) {
  NumsWalk w;
  if (key >= 0) {
    w.depth = 1;
    w.st = 1;
    w.key = (uint64_t)key;
  }
  std::vector<DerivEdit> ed;
  size_t refStart = 0;
  StructMap::Ref pred;
  bool havePred = false;
  for (size_t i = a; i < n && !w.done && !w.bad;) {
    uint8_t c = v[i];
    if (isWS(c)) { i++; continue; }
    if (c == '[') { w.open(); i++; continue; }
    if (c == ']') {
      w.close();
      i++;
      if (key >= 0 && w.depth == 1) w.done = true;
      continue;
    }
    if (!isRegular(c)) return 0;
    size_t j = i;
    while (j < n && isRegular(v[j])) j++;
    if (c >= '0' && c <= '9') {
      if (w.atElement()) {
        refStart = i;
        havePred = sm.parentTreeRef(w.depth == 2, w.key, w.idx, pred);
      }
      w.integer(v + i, j - i);
    } else {
      bool endRef = j - i == 1 && v[i] == 'R' && w.refPart == 2;
      w.word(v + i, j - i);
      if (endRef && havePred && refStr(pred) == std::string((const char*)v + refStart, j - refStart))
        ed.push_back({base + refStart, base + j, Bytes{kDerivEsc, 'r'}});
      if (endRef) havePred = false;
    }
    i = j;
  }
  if (!w.done || w.bad || ed.size() < 2) return 0;
  Bytes rep{kDerivEsc, (uint8_t)(key < 0 ? 'n' : after ? 'Q' : 'q')};
  std::string f = key < 0 ? (member >= 0 ? " " : "") : " " + decStr((uint64_t)key) + " ";
  if (member >= 0) f += decStr((uint64_t)member) + " ";
  rep.insert(rep.end(), f.begin(), f.end());
  if (after) edits.push_back({base + a + 1, base + a + 1, rep});
  else edits.push_back({base + a, base + a + 1, rep});
  edits.insert(edits.end(), ed.begin(), ed.end());
  return ed.size();
}

// "N G R" at p[a]; its end in e
static bool refAt(const uint8_t* p, size_t n, size_t a, uint64_t& num, uint64_t& gen, size_t& e) {
  size_t i = a;
  uint64_t v[2];
  for (int f = 0; f < 2; f++) {
    size_t d0 = i;
    v[f] = 0;
    while (i < n && p[i] >= '0' && p[i] <= '9' && i - d0 < 10) v[f] = v[f] * 10 + (p[i++] - '0');
    if (i == d0 || i >= n || p[i] != ' ') return false;
    i++;
  }
  if (i >= n || p[i] != 'R' || (i + 1 < n && isRegular(p[i + 1]))) return false;
  num = v[0];
  gen = v[1];
  e = i + 1;
  return true;
}

static std::string refStr(std::pair<uint32_t, uint32_t> r) {
  return decStr(r.first) + " " + decStr(r.second) + " R";
}

// encoder: placeholder for /P of object `num` when the map predicts it; v = value text (no
// header), base = its offset in the text the edit applies to. member < 0: top-level object.
static bool structChoose(const uint8_t* v, size_t n, size_t base, uint32_t num, long member, const StructMap& sm,
                         std::vector<DerivEdit>& edits) {
  auto f = sm.parent.find(num);
  if (f == sm.parent.end()) return false;
  size_t a = topKeyValue(v, n, "/P"), e;
  uint64_t pn, pg;
  if (a == SIZE_MAX || !refAt(v, n, a, pn, pg, e) || refStr(f->second) != std::string((const char*)v + a, e - a))
    return false;
  Bytes rep{kDerivEsc, (uint8_t)(member < 0 ? 'k' : 'K')};
  if (member >= 0) {
    std::string m = " " + decStr((uint64_t)member) + " ";
    rep.insert(rep.end(), m.begin(), m.end());
  }
  edits.push_back({base + a, base + e, rep});
  return true;
}

// ObjStm members: numbers from the L_OBJSTM recon, offsets predicted from the member text as the
// layer does. The decoder computes the offsets as the text is restored: member k's start is
// known once members before it are complete.
static bool objstmReconNums(const Bytes& recon, std::vector<uint64_t>& nums, bool& exact, std::vector<int64_t>& corr) {
  RBuf r(recon);
  size_t N = r.len();
  r.u8();
  r.u8();
  Bytes tail;
  r.bytes(r.len(), tail);
  nums.assign(N, 0);
  uint64_t prev = (uint64_t)-1;
  for (auto& v : nums) {
    v = prev + 1 + (uint64_t)unzigzag(r.var());
    if (v > 0xFFFFFFFFull) return false;
    prev = v;
  }
  exact = r.u8() != 0;
  corr.assign(N, 0);
  if (!exact)
    for (auto& c : corr) c = unzigzag(r.var());
  return r.ok();
}

struct ObjstmMembers {
  std::vector<uint64_t> nums, offs;
  std::vector<int64_t> corr;
  bool exact = true;
  size_t added = 0;  // members whose /K is in the map
  bool init(const StreamDesc& sd) {
    if (sd.layers.empty() || sd.layers.back().type != L_OBJSTM) return false;
    return objstmReconNums(sd.layers.back().recon, nums, exact, corr) && !nums.empty();
  }
  // start of member k in text t (members before k complete)
  bool start(const uint8_t* t, size_t n, size_t k, uint64_t& s) {
    while (offs.size() <= k) {
      uint64_t pred;
      objstmPredict(t, n, offs.size(), offs, pred);
      uint64_t o = pred + (uint64_t)corr[offs.size()];
      if (o > n || (!offs.empty() && o < offs.back())) return false;
      offs.push_back(o);
    }
    s = offs[k];
    return true;
  }
  // /K of members [added, k) into the map; final: k == count, the last member ends at n
  bool advance(const uint8_t* t, size_t n, size_t k, StructMap& sm) {
    if (k > nums.size()) return false;
    while (added < k) {
      uint64_t a, b = n;
      if (!start(t, n, added, a) || (added + 1 < nums.size() && !start(t, n, added + 1, b))) return false;
      structAddKids(t + a, (size_t)(b - a), (uint32_t)nums[added], 0, sm);
      added++;
    }
    return true;
  }
};

// ObjStm streams whose members both sides see as text
static bool objstmMembersOf(const StreamDesc& sd) {
  return sd.cls == SC_OBJSTM && sd.storage == ST_TXT && !sd.layers.empty() && sd.layers.back().type == L_OBJSTM;
}

static size_t objBodyStart(const uint8_t* p, size_t n) {
  Lexer lx(p, n, 0);
  uint64_t a, g;
  if (lx.parseUInt(a) && lx.parseUInt(g) && lx.eatKeyword("obj")) return lx.pos;
  return SIZE_MAX;
}

// what the member and structure placeholders need on the decoder side
struct DerivExtra {
  DerivFonts* fonts = nullptr;
  StructMap* sm = nullptr;
  ObjstmMembers* members = nullptr;  // decoding ObjStm member text
};

// encoder: placeholders for the /Widths or /W array opening at p[a]; the map and rule with the
// most exact predictions win. Returns the number of widths replaced.
static size_t widthsChoose(const uint8_t* p, size_t n, size_t a, int kind, const std::string& fields, uint32_t first,
                           const FontMetrics& m, std::vector<DerivEdit>& edits, bool after = false) {
  std::vector<WidthTok> tk;
  if (!widthsTokens(p, n, a, tk)) return 0;
  auto run = [&](int map, int rule, std::vector<size_t>* hitTok) -> size_t {
    WidthsWalk ww;
    ww.cid = kind == WK_CID2 || kind == WK_CID2M || kind == WK_CID0;
    ww.first = first;
    size_t hits = 0;
    std::string s;
    for (size_t t = 0; t < tk.size() && !ww.bad; t++) {
      const WidthTok& w = tk[t];
      if (w.t == '[') ww.open();
      else if (w.t == ']') ww.close();
      else {
        uint32_t code;
        if (ww.widthNext(code) && widthPredict(m, kind, map, rule, code, s) && s.size() == w.b - w.a &&
            memcmp(s.data(), p + w.a, s.size()) == 0) {
          hits++;
          if (hitTok) hitTok->push_back(t);
        }
        ww.number(p + w.a, w.b - w.a);
      }
    }
    return ww.bad || !ww.done ? 0 : hits;
  };
  int bestMap = 0, bestRule = 0;
  size_t best = 0;
  for (int map = 0; map < widthMaps(kind); map++)
    for (int rule = 0; rule < kWidthRules; rule++) {
      size_t h = run(map, rule, nullptr);
      if (h > best) { best = h; bestMap = map; bestRule = rule; }
    }
  // an array where few widths match costs more than it saves (the remaining numbers lose their
  // neighbours in the lifter's groups): at least 20% of the numbers, 70% for Type 1 (TeX writes
  // widths from TFM metrics with a decimal; the header also carries the L_T1 recon). 120-file
  // set, PDF-specific xz: any 3 hits 7733.7 KB, 30% 7727.7 KB, 20% with Type 1 at 70% 7725.6 KB,
  // Type 1 off 7725.6 KB
  size_t nnum = 0;
  for (auto& t : tk) nnum += t.t == 0;
  if (best < 3 || best * 10 < (size_t)(kind == WK_T1 ? 7 : 2) * nnum) return 0;
  std::vector<size_t> hit;
  run(bestMap, bestRule, &hit);
  Bytes rep{kDerivEsc, (uint8_t)(after ? 'v' : 'w'), (uint8_t)('a' + kind), (uint8_t)('a' + bestMap),
            (uint8_t)('a' + bestRule)};
  // fields: " <font> <FirstChar> [h<recon>] [<map>] "
  rep.insert(rep.end(), fields.begin(), fields.end());
  if (after) edits.push_back({a + 1, a + 1, rep});
  else edits.push_back({a, a + 1, rep});
  for (size_t t : hit) edits.push_back({tk[t].a, tk[t].b, Bytes{kDerivEsc, 'x'}});
  return hit.size();
}

// decoder: placeholders resolved
static bool derivDecode(const uint8_t* p, size_t n, const StreamFacts& f, const DeriveState& ds, uint32_t objNum,
                        Bytes& out, const DerivCrypt* dc = nullptr, std::vector<FwdPatch>* fwd = nullptr,
                        SigBlobs* sig = nullptr, B64Blobs* b64 = nullptr, DerivExtra* ex = nullptr) {
  DerivFonts* fonts = ex ? ex->fonts : nullptr;
  out.clear();
  for (size_t i = 0; i < n; i++) {
    if (p[i] != kDerivEsc) { out.push_back(p[i]); continue; }
    if (++i >= n) return false;
    if (p[i] == kDerivEsc) { out.push_back(kDerivEsc); continue; }
    if (p[i] == 'k' || p[i] == 'K') {
      if (!ex || !ex->sm) return false;
      uint32_t child = objNum;
      if (p[i] == 'K') {
        if (!ex->members || i + 1 >= n || p[i + 1] != ' ') return false;
        size_t j = i + 2, d0 = j;
        uint64_t k = 0;
        while (j < n && p[j] >= '0' && p[j] <= '9' && j - d0 < 10) k = k * 10 + (p[j++] - '0');
        if (j == d0 || j >= n || p[j] != ' ' || k >= ex->members->nums.size()) return false;
        if (!ex->members->advance(out.data(), out.size(), (size_t)k, *ex->sm)) return false;
        child = (uint32_t)ex->members->nums[(size_t)k];
        i = j;
      }
      auto f = ex->sm->parent.find(child);
      if (f == ex->sm->parent.end()) return false;
      std::string v = refStr(f->second);
      out.insert(out.end(), v.begin(), v.end());
      continue;
    }
    if (p[i] == 'n' || p[i] == 'q' || p[i] == 'Q') {
      if (!ex || !ex->sm) return false;
      bool arrObj = p[i] != 'n', after = p[i] == 'Q';
      i++;
      NumsWalk w;
      auto decimal = [&](uint64_t& v) {  // digits then a space
        size_t d0 = i;
        v = 0;
        while (i < n && p[i] >= '0' && p[i] <= '9' && i - d0 < 10) v = v * 10 + (p[i++] - '0');
        return i > d0 && i < n && p[i++] == ' ';
      };
      if (arrObj || ex->members) {
        if (i >= n || p[i++] != ' ') return false;
      }
      if (arrObj) {  // "<key> "
        uint64_t k;
        if (!decimal(k)) return false;
        w.depth = 1;
        w.st = 1;
        w.key = k;
      }
      if (ex->members) {  // "<member> ": the /K of the members before it first
        uint64_t m;
        if (!decimal(m) || m >= ex->members->nums.size() ||
            !ex->members->advance(out.data(), out.size(), (size_t)m, *ex->sm))
          return false;
      }
      w.open();
      if (!after) out.push_back('[');
      while (!w.done) {
        if (i >= n || w.bad) return false;
        uint8_t c = p[i];
        if (isWS(c)) { out.push_back(c); i++; }
        else if (c == '[') { w.open(); out.push_back(c); i++; }
        else if (c == ']') {
          w.close();
          out.push_back(c);
          i++;
          if (arrObj && w.depth == 1) w.done = true;
        }
        else if (c == kDerivEsc && i + 1 < n && p[i + 1] == 'r') {
          StructMap::Ref r;
          if (!w.atElement() || !ex->sm->parentTreeRef(w.depth == 2, w.key, w.idx, r)) return false;
          std::string v = refStr(r);
          out.insert(out.end(), v.begin(), v.end());
          w.elementDone();
          i += 2;
        } else if (isRegular(c) && c != kDerivEsc) {
          size_t j = i;
          while (j < n && isRegular(p[j]) && p[j] != kDerivEsc) j++;
          if (c >= '0' && c <= '9') w.integer(p + i, j - i);
          else w.word(p + i, j - i);
          out.insert(out.end(), p + i, p + j);
          i = j;
        } else {
          return false;
        }
      }
      if (w.bad) return false;
      i--;
      continue;
    }
    if (p[i] == 'w' || p[i] == 'v') {
      bool after = p[i] == 'v';
      if (!fonts || i + 3 >= n || p[i + 1] < 'a' || p[i + 2] < 'a' || p[i + 3] < 'a') return false;
      int kind = p[i + 1] - 'a', map = p[i + 2] - 'a', rule = p[i + 3] - 'a';
      if (kind >= WK_COUNT || map >= widthMaps(kind) || rule >= kWidthRules) return false;
      i += 4;
      // " <font> <FirstChar> [h<recon>] [<map>] "
      int nf = 2 + (kind == WK_T1) + (kind == WK_CID2M);
      std::string fld[4];
      for (int f = 0; f < nf; f++) {
        if (i >= n || p[i++] != ' ') return false;
        size_t f0 = i;
        while (i < n && p[i] != ' ' && i - f0 < 4096) i++;
        if (i == f0) return false;
        fld[f].assign((const char*)p + f0, i - f0);
      }
      if (i >= n || p[i++] != ' ') return false;
      uint64_t first = 0;
      for (char c : fld[1]) {
        if (c < '0' || c > '9' || first > 0xFFFFFF) return false;
        first = first * 10 + (c - '0');
      }
      if (first > 0xFFFFFF) return false;
      const FontMetrics* m = fonts->get(kind, fld[0], kind == WK_T1 ? fld[2] : std::string(),
                                        kind == WK_CID2M ? fld[2] : std::string());
      if (!m) return false;
      WidthsWalk ww;
      ww.cid = kind == WK_CID2 || kind == WK_CID2M || kind == WK_CID0;
      ww.first = (uint32_t)first;
      ww.open();
      if (!after) out.push_back('[');
      std::string s;
      while (!ww.done) {
        if (i >= n || ww.bad) return false;
        uint8_t c = p[i];
        if (isWS(c)) { out.push_back(c); i++; }
        else if (c == '[') { ww.open(); out.push_back(c); i++; }
        else if (c == ']') { ww.close(); out.push_back(c); i++; }
        else if (c == kDerivEsc && i + 1 < n && p[i + 1] == 'x') {
          uint32_t code;
          if (!ww.widthNext(code) || !widthPredict(*m, kind, map, rule, code, s)) return false;
          out.insert(out.end(), s.begin(), s.end());
          ww.number((const uint8_t*)s.data(), s.size());
          i += 2;
        } else if (widthNumStart(c)) {
          size_t j = i;
          while (j < n && isRegular(p[j]) && p[j] != kDerivEsc) j++;
          ww.number(p + i, j - i);
          out.insert(out.end(), p + i, p + j);
          i = j;
        } else {
          return false;
        }
      }
      if (ww.bad) return false;
      i--;
      continue;
    }
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
        if (ext.empty() || ext.size() > 8) return false;
        for (uint8_t c : ext)
          if (!isalnum(c)) return false;  // part of a file name
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
      if (!cipherEncrypt(cipher, *dc->fileKey, dc->num, dc->gen, iv, pt, ct) || !r.ok())
        return fail("bad encrypted string in item text");
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
