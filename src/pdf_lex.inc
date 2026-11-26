// pdf_lex.inc -- PDF lexer and read-only object parser.
//
// Used by the encoder only: segmentation, dictionary lookups, classification, ObjStm expansion.
// Nothing here has to re-serialize; before M5 every object is stored as raw text.

static inline bool isWS(uint8_t c) { return c == 0 || c == 9 || c == 10 || c == 12 || c == 13 || c == 32; }
static inline bool isDelim(uint8_t c) {
  return c == '(' || c == ')' || c == '<' || c == '>' || c == '[' || c == ']' || c == '{' || c == '}' || c == '/' ||
         c == '%';
}
static inline bool isRegular(uint8_t c) { return !isWS(c) && !isDelim(c); }
static inline int hexVal(uint8_t c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

struct Obj {
  enum Type : uint8_t { NONE, NUL, BOOL, INT, REAL, NAME, STR, ARR, DICT, REF, KW };
  Type t = NONE;
  bool hex = false;        // STR: was a hex string
  int64_t i = 0;           // INT, BOOL, REF (object number)
  uint32_t gen = 0;        // REF
  double r = 0;            // REAL
  lib::string s;           // NAME (decoded, no slash), STR (decoded bytes), KW
  lib::vector<Obj> a;      // ARR elements; DICT: key, value, key, value...

  bool isName(const char* n) const { return t == NAME && s == n; }
  bool isNum() const { return t == INT || t == REAL; }
  double num() const { return t == INT ? (double)i : r; }
  // DICT lookup; the last occurrence of a duplicate key wins.
  const Obj* get(const char* key) const {
    if (t != DICT) return nullptr;
    const Obj* r0 = nullptr;
    for (size_t k = 0; k + 1 < a.size(); k += 2)
      if (a[k].s == key) r0 = &a[k + 1];
    return r0;
  }
  const Obj* get2(const char* key, const char* abbrev) const {
    const Obj* o = get(key);
    return o ? o : get(abbrev);
  }
};

class Lexer {
 public:
  Lexer(const uint8_t* p, size_t n, size_t pos = 0) : p_(p), n_(n), pos(pos) {}
  const uint8_t* p_;
  size_t n_;
  size_t pos;

  bool eof() const { return pos >= n_; }

  void skipWS() {
    while (pos < n_) {
      uint8_t c = p_[pos];
      if (isWS(c)) {
        pos++;
      } else if (c == '%') {
        while (pos < n_ && p_[pos] != '\r' && p_[pos] != '\n') pos++;
      } else {
        break;
      }
    }
  }

  // Next regular-character run as a keyword-like token (does not skip whitespace first).
  bool peekKeyword(const char* kw) {
    size_t save = pos;
    skipWS();
    size_t l = strlen(kw);
    bool ok = pos + l <= n_ && memcmp(p_ + pos, kw, l) == 0 && (pos + l == n_ || !isRegular(p_[pos + l]));
    pos = save;
    return ok;
  }
  bool eatKeyword(const char* kw) {
    size_t save = pos;
    skipWS();
    size_t l = strlen(kw);
    if (pos + l <= n_ && memcmp(p_ + pos, kw, l) == 0 && (pos + l == n_ || !isRegular(p_[pos + l]))) {
      pos += l;
      return true;
    }
    pos = save;
    return false;
  }

  // Parses an unsigned integer token at the current position (after whitespace).
  bool parseUInt(uint64_t& v) {
    size_t save = pos;
    skipWS();
    size_t st = pos;
    v = 0;
    while (pos < n_ && p_[pos] >= '0' && p_[pos] <= '9' && pos - st < 19) v = v * 10 + (p_[pos++] - '0');
    if (pos == st || (pos < n_ && isRegular(p_[pos]))) {
      pos = save;
      return false;
    }
    return true;
  }

  // Parses one object. Returns false on syntax error (position then undefined).
  bool parse(Obj& o, int depth = 0) {
    if (depth > 200) return false;
    skipWS();
    if (pos >= n_) return false;
    uint8_t c = p_[pos];
    if (c == '/') {
      pos++;
      o.t = Obj::NAME;
      o.s.clear();
      while (pos < n_ && isRegular(p_[pos])) {
        uint8_t ch = p_[pos++];
        if (ch == '#' && pos + 1 < n_ && hexVal(p_[pos]) >= 0 && hexVal(p_[pos + 1]) >= 0) {
          ch = (uint8_t)(hexVal(p_[pos]) * 16 + hexVal(p_[pos + 1]));
          pos += 2;
        }
        o.s += (char)ch;
      }
      return true;
    }
    if (c == '(') return parseLiteral(o);
    if (c == '<') {
      if (pos + 1 < n_ && p_[pos + 1] == '<') {
        pos += 2;
        o.t = Obj::DICT;
        o.a.clear();
        for (;;) {
          skipWS();
          if (pos >= n_) return false;
          if (p_[pos] == '>' && pos + 1 < n_ && p_[pos + 1] == '>') {
            pos += 2;
            return true;
          }
          Obj k;
          if (!parse(k, depth + 1) || k.t != Obj::NAME) return false;
          Obj v;
          skipWS();
          if (pos < n_ && p_[pos] == '>' && pos + 1 < n_ && p_[pos + 1] == '>') {
            v.t = Obj::NUL;  // key without value: tolerate
          } else if (!parse(v, depth + 1)) {
            return false;
          }
          o.a.push_back(lib::move(k));
          o.a.push_back(lib::move(v));
        }
      }
      return parseHex(o);
    }
    if (c == '[') {
      pos++;
      o.t = Obj::ARR;
      o.a.clear();
      for (;;) {
        skipWS();
        if (pos >= n_) return false;
        if (p_[pos] == ']') {
          pos++;
          return true;
        }
        Obj v;
        if (!parse(v, depth + 1)) return false;
        o.a.push_back(lib::move(v));
      }
    }
    if (c == ')' || c == '>' || c == ']' || c == '{' || c == '}') {
      // stray delimiter: treat as a one-character keyword so callers can stop
      o.t = Obj::KW;
      o.s.assign(1, (char)c);
      pos++;
      return true;
    }
    // regular token: number, keyword, or reference
    size_t st = pos;
    while (pos < n_ && isRegular(p_[pos])) pos++;
    lib::string tok((const char*)p_ + st, pos - st);
    int kind = numberKind(tok);
    if (kind == 1) {
      o.t = Obj::INT;
      errno = 0;
      long long v = strtoll(tok.c_str(), nullptr, 10);
      if (errno == ERANGE) {
        o.t = Obj::REAL;
        o.r = strtod(tok.c_str(), nullptr);
        return true;
      }
      o.i = v;
      // reference: INT INT R
      if (v >= 0 && tok[0] != '+' && tok[0] != '-') {
        size_t save = pos;
        uint64_t g;
        if (parseUInt(g) && g <= 65535 && eatKeyword("R")) {
          o.t = Obj::REF;
          o.gen = (uint32_t)g;
          return true;
        }
        pos = save;
      }
      return true;
    }
    if (kind == 2) {
      o.t = Obj::REAL;
      o.r = strtod(tok.c_str(), nullptr);
      return true;
    }
    if (tok == "true" || tok == "false") {
      o.t = Obj::BOOL;
      o.i = tok == "true";
      return true;
    }
    if (tok == "null") {
      o.t = Obj::NUL;
      return true;
    }
    o.t = Obj::KW;
    o.s = tok;
    // object-level keywords cannot occur inside a container: the container is broken. Stopping
    // here keeps a damaged object from swallowing the rest of the file.
    if (depth > 0 && (tok == "obj" || tok == "endobj" || tok == "stream" || tok == "endstream")) return false;
    return true;
  }

 private:
  // 1 = integer, 2 = real, 0 = neither
  static int numberKind(const lib::string& t) {
    size_t i = 0;
    if (i < t.size() && (t[i] == '+' || t[i] == '-')) i++;
    size_t digits = 0, dots = 0;
    for (; i < t.size(); i++) {
      if (t[i] >= '0' && t[i] <= '9') digits++;
      else if (t[i] == '.') dots++;
      else return 0;
    }
    if (digits == 0 || dots > 1) return 0;
    return dots ? 2 : 1;
  }

  bool parseLiteral(Obj& o) {
    o.t = Obj::STR;
    o.hex = false;
    o.s.clear();
    pos++;  // '('
    int depth = 1;
    while (pos < n_) {
      uint8_t c = p_[pos++];
      if (c == '\\') {
        if (pos >= n_) return false;
        uint8_t e = p_[pos++];
        switch (e) {
          case 'n': o.s += '\n'; break;
          case 'r': o.s += '\r'; break;
          case 't': o.s += '\t'; break;
          case 'b': o.s += '\b'; break;
          case 'f': o.s += '\f'; break;
          case '\r':
            if (pos < n_ && p_[pos] == '\n') pos++;
            break;
          case '\n': break;
          default:
            if (e >= '0' && e <= '7') {
              int v = e - '0';
              for (int k = 0; k < 2 && pos < n_ && p_[pos] >= '0' && p_[pos] <= '7'; k++) v = v * 8 + (p_[pos++] - '0');
              o.s += (char)(v & 0xFF);
            } else {
              o.s += (char)e;
            }
        }
      } else if (c == '(') {
        depth++;
        o.s += '(';
      } else if (c == ')') {
        if (--depth == 0) return true;
        o.s += ')';
      } else if (c == '\r') {
        if (pos < n_ && p_[pos] == '\n') pos++;
        o.s += '\n';
      } else {
        o.s += (char)c;
      }
    }
    return false;
  }

  bool parseHex(Obj& o) {
    o.t = Obj::STR;
    o.hex = true;
    o.s.clear();
    pos++;  // '<'
    int hi = -1;
    while (pos < n_) {
      uint8_t c = p_[pos++];
      if (c == '>') {
        if (hi >= 0) o.s += (char)(hi << 4);
        return true;
      }
      if (isWS(c)) continue;
      int v = hexVal(c);
      if (v < 0) return false;
      if (hi < 0) {
        hi = v;
      } else {
        o.s += (char)(hi * 16 + v);
        hi = -1;
      }
    }
    return false;
  }
};

// Memory search helper.
static const uint8_t* memfind(const uint8_t* hay, size_t hn, const char* needle, size_t nn) {
  if (nn == 0 || hn < nn) return nullptr;
  const uint8_t* end = hay + hn - nn + 1;
  const uint8_t* p = hay;
  while (p < end) {
    p = (const uint8_t*)memchr(p, needle[0], end - p);
    if (!p) return nullptr;
    if (memcmp(p, needle, nn) == 0) return p;
    p++;
  }
  return nullptr;
}
