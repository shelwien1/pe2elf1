// pdf_xref.inc -- classic cross-reference tables (I_XREF items).
//
// An xref section is "xref" EOL, then subsections "start count" EOL followed by 20-byte
// entries "oooooooooo ggggg n" + 2-byte EOL. In-use entries are predicted from the positions of
// the objects already written (latest definition of that number), which the decoder tracks as
// it writes; only mismatches and free entries are stored. Sections that are not in this exact
// form stay plain text.
//
// Coding (channel S: u8 subsection separator, 2 bytes entry EOL; N: lengths, varints; T: EOLs):
//   T: text after "xref" up to the first subsection; per section the text after each count
//   N: subsection count; per subsection zigzag(start - previous end), count; then per entry
//      a kind varint (+ fields), see writeXref.

struct XrefEntry {
  uint64_t off;
  uint32_t gen;
  char type;
};
struct XrefSection {
  size_t start = 0, end = 0;  // [start, end) in the file
  Bytes eol1;                 // after "xref"
  uint8_t sep = ' ';          // between subsection start and count
  std::vector<Bytes> eol2;    // after each subsection count
  uint8_t eole[2] = {' ', '\n'};
  std::vector<std::pair<uint64_t, uint64_t>> subs;
  std::vector<XrefEntry> entries;
};

static bool parseXrefAt(const uint8_t* d, size_t n, size_t pos, XrefSection& x) {
  if (pos + 4 > n || memcmp(d + pos, "xref", 4) != 0 || (pos > 0 && isRegular(d[pos - 1]))) return false;
  size_t p = pos + 4;
  size_t e1 = p;
  while (e1 < n && isWS(d[e1])) e1++;
  if (e1 == p) return false;
  x = XrefSection();
  x.start = pos;
  x.eol1.assign(d + p, d + e1);
  p = e1;
  bool firstEntry = true;
  while (p < n && d[p] >= '0' && d[p] <= '9') {
    uint64_t st = 0, cnt = 0;
    size_t q = p;
    while (q < n && d[q] >= '0' && d[q] <= '9' && q - p < 12) st = st * 10 + (d[q++] - '0');
    if ((q - p > 1 && d[p] == '0') || q >= n || d[q] != ' ') return false;
    q++;
    size_t c0 = q;
    while (q < n && d[q] >= '0' && d[q] <= '9' && q - c0 < 12) cnt = cnt * 10 + (d[q++] - '0');
    if (q == c0 || (q - c0 > 1 && d[c0] == '0')) return false;
    size_t e2 = q;
    while (e2 < n && isWS(d[e2])) e2++;
    if (e2 == q || cnt > (n - e2) / 20) return false;
    x.subs.push_back({st, cnt});
    x.eol2.emplace_back(d + q, d + e2);
    p = e2;
    for (uint64_t k = 0; k < cnt; k++, p += 20) {
      const uint8_t* e = d + p;
      uint64_t off = 0;
      uint32_t gen = 0;
      for (int i = 0; i < 10; i++) {
        if (e[i] < '0' || e[i] > '9') return false;
        off = off * 10 + (e[i] - '0');
      }
      if (e[10] != ' ' || e[16] != ' ' || (e[17] != 'n' && e[17] != 'f')) return false;
      for (int i = 11; i < 16; i++) {
        if (e[i] < '0' || e[i] > '9') return false;
        gen = gen * 10 + (e[i] - '0');
      }
      if (firstEntry) {
        x.eole[0] = e[18];
        x.eole[1] = e[19];
        firstEntry = false;
      } else if (e[18] != x.eole[0] || e[19] != x.eole[1]) {
        return false;
      }
      if (!isWS(e[18]) || !isWS(e[19])) return false;
      x.entries.push_back({off, gen, (char)e[17]});
    }
  }
  if (x.subs.empty()) return false;
  // the bytes after the last count that were taken as its EOL must not run into the next token:
  // the section ends after the last entry
  x.end = p;
  return true;
}

// Object positions known to both sides: latest definition of each object number so far.
struct ObjPositions {
  std::unordered_map<uint32_t, std::pair<uint64_t, uint32_t>> pos;  // num -> (offset, gen)
  void add(uint32_t num, uint32_t gen, uint64_t off) { pos[num] = {off, gen}; }
  bool get(uint64_t num, uint64_t& off, uint32_t& gen) const {
    auto it = pos.find((uint32_t)num);
    if (num > 0xFFFFFFFFull || it == pos.end()) return false;
    off = it->second.first;
    gen = it->second.second;
    return true;
  }
};

// "N G obj" at the start of an object item's text
static bool objHeaderOf(const uint8_t* p, size_t n, uint32_t& num, uint32_t& gen) {
  Lexer lx(p, n, 0);
  uint64_t a, g;
  if (!lx.parseUInt(a) || !lx.parseUInt(g) || !lx.eatKeyword("obj") || a > 0xFFFFFFFFull || g > 65535) return false;
  num = (uint32_t)a;
  gen = (uint32_t)g;
  return true;
}

// next free object number after entry i (0 after the last), over the whole section
static std::vector<uint64_t> xrefNextFree(const std::vector<std::pair<uint64_t, uint64_t>>& subs,
                                          const std::vector<char>& types) {
  std::vector<uint64_t> nums;
  for (auto& sb : subs)
    for (uint64_t k = 0; k < sb.second; k++) nums.push_back(sb.first + k);
  std::vector<uint64_t> next(types.size(), 0);
  uint64_t nf = 0;
  for (size_t i = types.size(); i-- > 0;) {
    next[i] = nf;
    if (types[i] == 'f') nf = nums[i];
  }
  return next;
}

// entry kinds: 0 in use as predicted; 1 in use + zigzag(offset - predicted) + gen;
// 2 free, offset = next free object, gen 65535; 3 free + offset + gen; 4 free, offset 0, gen 65535
// kind 5: in use, pointing forward at item `target` (delta-coded against the previous one), gen
static void writeXref(ChanWriter& w, const XrefSection& x, const ObjPositions& op,
                      const std::function<int64_t(uint64_t)>* itemAt = nullptr) {
  int64_t prevTarget = -1;
  w.lenBytes(CH_N, CH_T, x.eol1);
  w.u8(CH_S, x.sep);
  w.u8(CH_S, x.eole[0]);
  w.u8(CH_S, x.eole[1]);
  w.var(CH_N, x.subs.size());
  std::vector<char> types;
  for (auto& e : x.entries) types.push_back(e.type);
  std::vector<uint64_t> nextFree = xrefNextFree(x.subs, types);
  uint64_t prevEnd = 0;
  for (size_t s = 0; s < x.subs.size(); s++) {
    w.svar(CH_N, (int64_t)(x.subs[s].first - prevEnd));
    w.var(CH_N, x.subs[s].second);
    w.lenBytes(CH_N, CH_T, x.eol2[s]);
    prevEnd = x.subs[s].first + x.subs[s].second;
  }
  size_t ei = 0;
  for (size_t s = 0; s < x.subs.size(); s++)
    for (uint64_t k = 0; k < x.subs[s].second; k++, ei++) {
      const XrefEntry& e = x.entries[ei];
      uint64_t num = x.subs[s].first + k, poff = 0;
      uint32_t pgen = 0;
      bool known = op.get(num, poff, pgen);
      if (e.type == 'n' && known && e.off == poff && e.gen == pgen) {
        w.var(CH_N, 0);
      } else if (e.type == 'n' && !known && itemAt && (*itemAt)(e.off) >= 0) {
        int64_t t = (*itemAt)(e.off);
        w.var(CH_N, 5);
        w.svar(CH_N, t - prevTarget - 1);
        w.var(CH_N, e.gen);
        prevTarget = t;
      } else if (e.type == 'n') {
        w.var(CH_N, 1);
        w.svar(CH_N, (int64_t)(e.off - poff));
        w.var(CH_N, e.gen);
      } else if (e.off == nextFree[ei] && e.gen == 65535) {
        w.var(CH_N, 2);
      } else if (e.off == 0 && e.gen == 65535) {
        w.var(CH_N, 4);
      } else {
        w.var(CH_N, 3);
        w.var(CH_N, e.off);
        w.var(CH_N, e.gen);
      }
    }
}

static int64_t xrefEnd(const std::vector<std::pair<uint64_t, uint64_t>>& subs) {
  int64_t e = -1;
  for (auto& s : subs) e = std::max<int64_t>(e, (int64_t)(s.first + s.second));
  return e;
}

static void readXref(ChanReader& r, const ObjPositions& op, Bytes& out, int64_t* endOut = nullptr,
                     std::vector<std::pair<uint64_t, int64_t>>* fwd = nullptr) {
  int64_t prevTarget = -1;
  std::vector<int64_t> targets;
  out.insert(out.end(), {'x', 'r', 'e', 'f'});
  Bytes t;
  r.lenBytes(CH_N, CH_T, t);
  out.insert(out.end(), t.begin(), t.end());
  uint8_t sep = r.u8(CH_S), e0 = r.u8(CH_S), e1 = r.u8(CH_S);
  size_t ns = r.len(CH_N, 1 << 30);
  std::vector<std::pair<uint64_t, uint64_t>> subs(ns);
  std::vector<Bytes> eol2(ns);
  uint64_t prevEnd = 0, total = 0;
  for (size_t s = 0; s < ns; s++) {
    subs[s].first = prevEnd + (uint64_t)r.svar(CH_N);
    subs[s].second = r.var(CH_N);
    r.lenBytes(CH_N, CH_T, eol2[s]);
    prevEnd = subs[s].first + subs[s].second;
    total += subs[s].second;
    if (total > ((uint64_t)1 << 32)) fail("bad xref section");
  }
  std::vector<XrefEntry> ents;
  std::vector<char> types;
  std::vector<uint8_t> kinds;
  for (size_t s = 0; s < ns; s++)
    for (uint64_t k = 0; k < subs[s].second; k++) {
      uint64_t num = subs[s].first + k, poff = 0;
      uint32_t pgen = 0;
      op.get(num, poff, pgen);
      uint64_t kind = r.var(CH_N);
      XrefEntry e{0, 0, 'n'};
      if (kind == 0) { e.off = poff; e.gen = pgen; }
      else if (kind == 1) { e.off = poff + (uint64_t)r.svar(CH_N); e.gen = (uint32_t)r.var(CH_N); }
      else if (kind == 2) { e.type = 'f'; e.gen = 65535; }
      else if (kind == 3) { e.type = 'f'; e.off = r.var(CH_N); e.gen = (uint32_t)r.var(CH_N); }
      else if (kind == 4) { e.type = 'f'; e.gen = 65535; }
      else if (kind == 5) {
        if (!fwd) fail("forward xref entry without patch list");
        int64_t t = prevTarget + 1 + r.svar(CH_N);
        prevTarget = t;
        e.gen = (uint32_t)r.var(CH_N);
        targets.resize(ents.size() + 1, -1);
        targets[ents.size()] = t;
      }
      else fail("bad xref entry kind");
      ents.push_back(e);
      types.push_back(e.type);
      kinds.push_back((uint8_t)kind);
    }
  std::vector<uint64_t> nextFree = xrefNextFree(subs, types);
  if (endOut) *endOut = xrefEnd(subs);
  char buf[48];
  size_t ei = 0;
  for (size_t s = 0; s < ns; s++) {
    int l = snprintf(buf, sizeof(buf), "%llu%c%llu", (unsigned long long)subs[s].first, sep,
                     (unsigned long long)subs[s].second);
    out.insert(out.end(), buf, buf + l);
    out.insert(out.end(), eol2[s].begin(), eol2[s].end());
    for (uint64_t k = 0; k < subs[s].second; k++, ei++) {
      XrefEntry& e = ents[ei];
      if (kinds[ei] == 2) e.off = nextFree[ei];
      if (kinds[ei] == 5) fwd->push_back({out.size(), targets[ei]});
      if (e.off > 9999999999ull || e.gen > 99999) fail("bad xref entry");
      l = snprintf(buf, sizeof(buf), "%010llu %05u %c", (unsigned long long)e.off, e.gen, e.type);
      out.insert(out.end(), buf, buf + l);
      out.push_back(e0);
      out.push_back(e1);
    }
  }
}
