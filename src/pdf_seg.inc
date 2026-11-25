// pdf_seg.inc -- file segmenter: items, stream extents incl. recovery, raw regions.
//
// Segmentation is a sequential scan from offset 0, never xref-driven. Items tile the file:
// every byte belongs to exactly one item, so any segmentation is lossless; a wrong guess only
// costs compactness.

struct Item {
  enum Kind : uint8_t { TEXT, RAW, OBJ, STREAM, XREF, KIND_COUNT };
  Kind kind = TEXT;
  size_t start = 0, end = 0;           // [start, end)
  size_t dataStart = 0, dataEnd = 0;   // STREAM payload [dataStart, dataEnd)
  uint32_t num = 0, gen = 0;
  int obj = -1;                        // index into Segmentation::objs
  uint8_t extent = 0;                  // STREAM: how the extent was found (kExtentName)
  int xref = -1;                       // XREF: index into Segmentation::xrefs
};
static const char* const kExtentName[] = {"Length", "Length-ref", "endstream-scan", "endobj-scan", "eof"};

struct ObjCand {
  size_t off;
  uint32_t num, gen;
};

struct Segmentation {
  bool isPdf = false;
  size_t hdrPos = 0;
  std::vector<Item> items;
  std::vector<Obj> objs;
  std::vector<Obj> trailers;  // classic trailer dicts in file order
  std::vector<XrefSection> xrefs;
  std::vector<ObjCand> cands;
  std::unordered_map<uint32_t, std::vector<size_t>> candByNum;  // objnum -> offsets
};

class Segmenter {
 public:
  Segmenter(const uint8_t* d, size_t n, Segmentation& s) : d_(d), n_(n), s_(s) {}

  void run() {
    s_.isPdf = false;
    size_t lim = std::min<size_t>(n_, 1024 + 5);
    const uint8_t* h = memfind(d_, lim, "%PDF-", 5);
    if (!h) return;
    s_.isPdf = true;
    s_.hdrPos = (size_t)(h - d_);
    scanCandidates();
    if (s_.hdrPos > 0) addRegion(0, s_.hdrPos, true);
    size_t pos = s_.hdrPos, textStart = pos, ci = 0;
    while (ci < s_.cands.size()) {
      if (s_.cands[ci].off < pos) { ci++; continue; }
      Item it;
      Obj v;
      if (parseObjectAt(s_.cands[ci].off, it, v)) {
        addRegion(textStart, it.start, false);
        it.obj = (int)s_.objs.size();
        s_.objs.push_back(std::move(v));
        s_.items.push_back(it);
        pos = textStart = it.end;
      } else {
        ci++;
      }
    }
    addRegion(textStart, n_, false);
  }

 private:
  const uint8_t* d_;
  size_t n_;
  Segmentation& s_;

  void scanCandidates() {
    const uint8_t* p = d_;
    for (;;) {
      p = memfind(p, n_ - (p - d_), "obj", 3);
      if (!p) break;
      size_t at = (size_t)(p - d_);
      p += 3;
      if (at + 3 < n_ && isRegular(d_[at + 3])) continue;
      size_t i = at;
      if (i == 0 || !isWS(d_[i - 1])) continue;
      while (i > 0 && isWS(d_[i - 1])) i--;
      size_t j = i;
      while (j > 0 && d_[j - 1] >= '0' && d_[j - 1] <= '9') j--;
      if (j == i || i - j > 5) continue;
      size_t k = j;
      if (k == 0 || !isWS(d_[k - 1])) continue;
      while (k > 0 && isWS(d_[k - 1])) k--;
      size_t m = k;
      while (m > 0 && d_[m - 1] >= '0' && d_[m - 1] <= '9') m--;
      if (m == k || k - m > 10) continue;
      if (m > 0 && isRegular(d_[m - 1])) continue;
      uint64_t num = 0, gen = 0;
      for (size_t q = m; q < k; q++) num = num * 10 + (d_[q] - '0');
      for (size_t q = j; q < i; q++) gen = gen * 10 + (d_[q] - '0');
      if (num > 0xFFFFFFFFull || gen > 65535) continue;
      s_.cands.push_back({m, (uint32_t)num, (uint32_t)gen});
      s_.candByNum[(uint32_t)num].push_back(m);
    }
  }

  bool texty(size_t a, size_t b) const {
    static const char* const kw[] = {"xref", "trailer", "startxref", "%PDF-", "%%EOF"};
    for (const char* k : kw)
      if (memfind(d_ + a, b - a, k, strlen(k))) return true;
    size_t bad = 0;
    for (size_t i = a; i < b; i++) {
      uint8_t c = d_[i];
      if (!((c >= 0x20 && c < 0x7F) || c == '\r' || c == '\n' || c == '\t' || c == '\f')) bad++;
    }
    return bad * 10 <= (b - a);
  }

  // text region: classic xref sections become items of their own
  void addRegion(size_t a, size_t b, bool forceRaw) {
    if (b <= a) return;
    if (!forceRaw) {
      const uint8_t* p = d_ + a;
      for (;;) {
        p = memfind(p, (d_ + b) - p, "xref", 4);
        if (!p) break;
        size_t at = (size_t)(p - d_);
        XrefSection x;
        if (parseXrefAt(d_, b, at, x) && x.end <= b) {
          addPlain(a, at, false);
          Item it;
          it.kind = Item::XREF;
          it.start = at;
          it.end = x.end;
          it.xref = (int)s_.xrefs.size();
          s_.xrefs.push_back(std::move(x));
          s_.items.push_back(it);
          a = it.end;
          p = d_ + a;
        } else {
          p += 4;
        }
      }
    }
    addPlain(a, b, forceRaw);
  }

  void addPlain(size_t a, size_t b, bool forceRaw) {
    if (b <= a) return;
    Item it;
    it.start = a;
    it.end = b;
    it.kind = (!forceRaw && texty(a, b)) ? Item::TEXT : Item::RAW;
    s_.items.push_back(it);
    // collect classic trailer dictionaries
    const uint8_t* p = d_ + a;
    for (;;) {
      p = memfind(p, (d_ + b) - p, "trailer", 7);
      if (!p) break;
      size_t at = (size_t)(p - d_);
      p += 7;
      if (at > 0 && isRegular(d_[at - 1])) continue;
      Lexer lx(d_, n_, at + 7);
      Obj t;
      if (lx.parse(t) && t.t == Obj::DICT) s_.trailers.push_back(std::move(t));
    }
  }

  size_t skipTrailingWS(size_t p) const {
    while (p < n_ && isWS(d_[p])) p++;
    return p;
  }

  bool validEnd(size_t e) const {
    if (e > n_) return false;
    size_t q = e;
    while (q < n_ && isWS(d_[q])) q++;
    return q + 9 <= n_ && memcmp(d_ + q, "endstream", 9) == 0;
  }

  // integer value of indirect object `num` at candidate offset `off`, if it is one
  bool intObjectAt(size_t off, uint32_t num, int64_t& v) const {
    Lexer lx(d_, n_, off);
    uint64_t a, g;
    if (!lx.parseUInt(a) || a != num || !lx.parseUInt(g) || !lx.eatKeyword("obj")) return false;
    Obj o;
    if (!lx.parse(o) || o.t != Obj::INT) return false;
    v = o.i;
    return true;
  }

  bool parseObjectAt(size_t off, Item& it, Obj& v) {
    Lexer lx(d_, n_, off);
    uint64_t num, gen;
    if (!lx.parseUInt(num) || !lx.parseUInt(gen) || !lx.eatKeyword("obj")) return false;
    it.start = off;
    it.num = (uint32_t)num;
    it.gen = (uint32_t)gen;
    lx.skipWS();
    if (lx.peekKeyword("endobj")) {
      v.t = Obj::NUL;
    } else {
      if (!lx.parse(v)) return false;
      if (v.t == Obj::KW) return false;
    }
    if (lx.eatKeyword("endobj")) {
      it.kind = Item::OBJ;
      it.end = skipTrailingWS(lx.pos);
      return true;
    }
    if (v.t == Obj::DICT && lx.eatKeyword("stream")) {
      it.kind = Item::STREAM;
      size_t p = lx.pos;
      size_t q = p;
      while (q < n_ && (d_[q] == ' ' || d_[q] == '\t')) q++;
      if (q < n_ && (d_[q] == '\r' || d_[q] == '\n')) p = q;
      if (p + 1 < n_ && d_[p] == '\r' && d_[p + 1] == '\n') p += 2;
      else if (p < n_ && (d_[p] == '\n' || d_[p] == '\r')) p += 1;
      size_t ds = p;
      it.dataStart = ds;
      bool found = false;
      const Obj* L = v.get("Length");
      if (L && L->t == Obj::INT && L->i >= 0 && validEnd(ds + (size_t)L->i)) {
        it.dataEnd = ds + (size_t)L->i;
        it.extent = 0;
        found = true;
      } else if (L && L->t == Obj::REF) {
        auto f = s_.candByNum.find((uint32_t)L->i);
        if (f != s_.candByNum.end()) {
          for (auto ri = f->second.rbegin(); ri != f->second.rend() && !found; ++ri) {
            int64_t len;
            if (intObjectAt(*ri, (uint32_t)L->i, len) && len >= 0 && validEnd(ds + (size_t)len)) {
              it.dataEnd = ds + (size_t)len;
              it.extent = 1;
              found = true;
            }
          }
        }
      }
      if (!found) {
        const uint8_t* e = memfind(d_ + ds, n_ - ds, "endstream", 9);
        it.extent = 2;
        if (!e) {
          e = memfind(d_ + ds, n_ - ds, "endobj", 6);
          it.extent = 3;
        }
        size_t pe = e ? (size_t)(e - d_) : n_;
        if (!e) it.extent = 4;
        if (e) {
          if (pe >= ds + 2 && d_[pe - 2] == '\r' && d_[pe - 1] == '\n') pe -= 2;
          else if (pe >= ds + 1 && (d_[pe - 1] == '\n' || d_[pe - 1] == '\r')) pe -= 1;
        }
        it.dataEnd = pe;
      }
      lx.pos = it.dataEnd;
      if (lx.eatKeyword("endstream")) lx.eatKeyword("endobj");
      else lx.eatKeyword("endobj");
      it.end = skipTrailingWS(std::max(lx.pos, it.dataEnd));
      return true;
    }
    // no endobj: object ends after its value
    it.kind = Item::OBJ;
    it.end = skipTrailingWS(lx.pos);
    return true;
  }
};
