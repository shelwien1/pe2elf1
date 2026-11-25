// pdf_t1.inc -- Type 1 font decryption layer (L_T1).
//
// Removes the eexec encryption (r = 55665) of the private part and, inside it, the charstring
// encryption (r = 4330) of every "<len> RD <binary>" blob. Both ciphers are deterministic given
// the plaintext (the random lead bytes are part of the plaintext), so re-encryption reproduces
// the original exactly. The result compresses far better than the encrypted font, but it is not
// a loadable font any more: it is written with the extension .t1d.
//
// Recon record: var start of the encrypted part, var its length, u8 flags (bit0: charstrings
// decrypted, bit1: charstrings split into pools), u8 count + RD token names (length-prefixed);
// with bit1: var lenIV, skeleton length, lead-byte pool length, operator pool length, length of
// the part after the encrypted section.

static void t1Decrypt(uint8_t* p, size_t n, uint16_t r) {
  for (size_t i = 0; i < n; i++) {
    uint8_t c = p[i];
    p[i] = (uint8_t)(c ^ (r >> 8));
    r = (uint16_t)((c + r) * 52845u + 22719u);
  }
}
static void t1Encrypt(uint8_t* p, size_t n, uint16_t r) {
  for (size_t i = 0; i < n; i++) {
    uint8_t c = (uint8_t)(p[i] ^ (r >> 8));
    p[i] = c;
    r = (uint16_t)((c + r) * 52845u + 22719u);
  }
}

// Charstring blobs "<len> <rd> <binary>" in the decrypted private part. Only bytes outside
// recognized blobs are examined, so the scan gives the same result on either side as long as
// every blob header is recognized; layerDecode verifies that through t1EncodeLayer.
static void t1FindBlobs(const uint8_t* p, size_t n, const std::vector<std::string>& rd,
                        std::vector<std::pair<size_t, size_t>>& blobs, bool bodies = true) {
  blobs.clear();
  size_t i = 0;
  while (i < n) {
    if (p[i] < '0' || p[i] > '9' || (i > 0 && isRegular(p[i - 1]) && p[i - 1] != ']')) { i++; continue; }
    size_t j = i;
    uint64_t L = 0;
    while (j < n && p[j] >= '0' && p[j] <= '9' && j - i < 9) L = L * 10 + (p[j++] - '0');
    if (j >= n || p[j] != ' ') { i = j > i ? j : i + 1; continue; }
    size_t t = j + 1, te = t;
    while (te < n && isRegular(p[te])) te++;
    if (te >= n || p[te] != ' ' || te == t) { i = j; continue; }
    bool ok = false;
    for (auto& r : rd)
      if (r.size() == te - t && memcmp(p + t, r.data(), r.size()) == 0) ok = true;
    if (!ok || (bodies && te + 1 + L > n)) { i = j; continue; }
    blobs.push_back({te + 1, (size_t)L});
    i = te + 1 + (bodies ? (size_t)L : 0);
  }
}

// ---- charstring split ---------------------------------------------------------------------
// With the charstrings decrypted, the private part becomes: text with the charstring bodies
// removed, then pools: lead bytes (lenIV per charstring), operators (each followed by the
// number of operands before it; 255 + trailing operand count ends a charstring), operands as
// zigzag varints. Only charstrings whose numbers all use the shortest encoding are split.

static bool t1CsNumber(const uint8_t* c, size_t n, size_t& i, int32_t& v) {
  uint8_t b = c[i];
  if (b <= 246) { v = (int32_t)b - 139; i += 1; return true; }
  if (b <= 254) {
    if (i + 1 >= n) return false;
    v = b <= 250 ? (b - 247) * 256 + c[i + 1] + 108 : -(b - 251) * 256 - c[i + 1] - 108;
    i += 2;
    return true;
  }
  if (i + 4 >= n) return false;
  v = (int32_t)(((uint32_t)c[i + 1] << 24) | ((uint32_t)c[i + 2] << 16) | ((uint32_t)c[i + 3] << 8) | c[i + 4]);
  i += 5;
  return true;
}
static void t1CsPutNumber(Bytes& out, int32_t v) {
  if (v >= -107 && v <= 107) out.push_back((uint8_t)(v + 139));
  else if (v >= 108 && v <= 1131) { out.push_back((uint8_t)(247 + (v - 108) / 256)); out.push_back((uint8_t)((v - 108) % 256)); }
  else if (v <= -108 && v >= -1131) { out.push_back((uint8_t)(251 + (-v - 108) / 256)); out.push_back((uint8_t)((-v - 108) % 256)); }
  else { out.push_back(255); for (int k = 3; k >= 0; k--) out.push_back((uint8_t)((uint32_t)v >> (8 * k))); }
}

// one decrypted charstring -> pools; false if it cannot be rebuilt exactly
static bool t1CsSplit(const uint8_t* c, size_t n, size_t lenIV, Bytes& iv, Bytes& ops, Bytes& nums) {
  if (n < lenIV) return false;
  iv.insert(iv.end(), c, c + lenIV);
  size_t i = lenIV, cnt = 0, opsStart = ops.size(), numsStart = nums.size();
  Bytes chk;
  while (i < n) {
    if (c[i] >= 32) {
      size_t st = i;
      int32_t v;
      if (!t1CsNumber(c, n, i, v)) return false;
      chk.clear();
      t1CsPutNumber(chk, v);
      if (chk.size() != i - st || memcmp(chk.data(), c + st, chk.size()) != 0) return false;
      putVarTo(nums, zigzag(v));
      cnt++;
    } else {
      if (cnt > 254) return false;
      ops.push_back(c[i]);
      if (c[i] == 12) {
        if (i + 1 >= n) return false;
        ops.push_back(c[i + 1]);
        i++;
      }
      i++;
      ops.push_back((uint8_t)cnt);
      cnt = 0;
    }
  }
  if (cnt > 254) return false;
  ops.push_back(255);
  ops.push_back((uint8_t)cnt);
  (void)opsStart;
  (void)numsStart;
  return true;
}

static bool t1CsJoin(RBuf& iv, RBuf& ops, RBuf& nums, size_t lenIV, size_t len, Bytes& out) {
  size_t st = out.size();
  for (size_t k = 0; k < lenIV; k++) out.push_back(iv.u8());
  for (;;) {
    uint8_t op = ops.u8();
    if (op == 255) {
      size_t cnt = ops.u8();
      for (size_t k = 0; k < cnt; k++) t1CsPutNumber(out, (int32_t)unzigzag(nums.var()));
      break;
    }
    uint8_t op2 = op == 12 ? ops.u8() : 0;
    size_t cnt = ops.u8();
    for (size_t k = 0; k < cnt; k++) t1CsPutNumber(out, (int32_t)unzigzag(nums.var()));
    out.push_back(op);
    if (op == 12) out.push_back(op2);
  }
  return out.size() - st == len;
}

static bool t1EncodeLayer(const Bytes& recon, const Bytes& in, Bytes& out) {
  RBuf r(recon);
  size_t s = (size_t)r.var(), n = (size_t)r.var();
  uint8_t flags = r.u8();
  std::vector<std::string> rd(r.u8());
  for (auto& x : rd) {
    Bytes b;
    r.bytes(r.len(), b);
    x.assign(b.begin(), b.end());
  }
  Bytes joined;
  const Bytes* src = &in;
  if (flags & 2) {
    // rebuild the private part from skeleton + pools; n is its length after the rebuild
    size_t lenIV = (size_t)r.var(), skelLen = (size_t)r.var(), ivLen = (size_t)r.var(), opsLen = (size_t)r.var();
    size_t tailLen = (size_t)r.var();
    if (s > in.size() || skelLen > in.size() - s || ivLen > in.size() || opsLen > in.size() || tailLen > in.size())
      return false;
    size_t poolStart = s + skelLen;
    if (poolStart + ivLen + opsLen > in.size() - tailLen) return false;
    Bytes ivB(in.begin() + poolStart, in.begin() + poolStart + ivLen);
    Bytes opsB(in.begin() + poolStart + ivLen, in.begin() + poolStart + ivLen + opsLen);
    Bytes numB(in.begin() + poolStart + ivLen + opsLen, in.end() - tailLen);
    RBuf ivr(ivB), opr(opsB), nr(numB);
    const uint8_t* sk = in.data() + s;
    std::vector<std::pair<size_t, size_t>> heads;
    t1FindBlobs(sk, skelLen, rd, heads, false);
    joined.assign(in.begin(), in.begin() + s);
    size_t pos = 0;
    for (auto& h : heads) {
      joined.insert(joined.end(), sk + pos, sk + h.first);
      if (!t1CsJoin(ivr, opr, nr, lenIV, h.second, joined)) return false;
      pos = h.first;
    }
    joined.insert(joined.end(), sk + pos, sk + skelLen);
    if (ivr.p != ivB.size() || opr.p != opsB.size() || nr.p != numB.size()) return false;
    if (joined.size() - s != n) return false;
    joined.insert(joined.end(), in.end() - tailLen, in.end());
    src = &joined;
  }
  if (s > src->size() || n > src->size() - s) return false;
  out = *src;
  uint8_t* p = out.data() + s;
  if (flags & 1) {
    std::vector<std::pair<size_t, size_t>> blobs;
    t1FindBlobs(p, n, rd, blobs);
    for (auto& b : blobs) t1Encrypt(p + b.first, b.second, 4330);
  }
  t1Encrypt(p, n, 55665);
  return true;
}

static bool t1DecodeLayer(const Bytes& in, Bytes& out, Bytes& recon, bool charstrings) {
  const uint8_t* d = in.data();
  size_t n = in.size();
  const uint8_t* e = memfind(d, n, "eexec", 5);
  if (!e) return false;
  size_t s = (size_t)(e - d) + 5;
  while (s < n && (d[s] == '\r' || d[s] == '\n' || d[s] == ' ' || d[s] == '\t')) s++;
  if (s + 4 > n) return false;
  bool hex = true;
  for (int k = 0; k < 4; k++) hex &= hexVal(d[s + k]) >= 0;
  if (hex) return false;  // hex eexec: not handled
  // encrypted part ends before the trailer of zeros + cleartomark, if there is one
  size_t j = n;
  const uint8_t* ctm = nullptr;
  for (const uint8_t* q = d; (q = memfind(q, d + n - q, "cleartomark", 11)); q++) ctm = q;
  if (ctm && (size_t)(ctm - d) > s) {
    size_t z = (size_t)(ctm - d), zeros = 0;
    while (z > s && (d[z - 1] == '0' || isWS(d[z - 1]))) { zeros += d[z - 1] == '0'; z--; }
    if (zeros >= 64) j = z;
  }
  out = in;
  uint8_t* p = out.data() + s;
  size_t len = j - s;
  t1Decrypt(p, len, 55665);
  // RD token names: RD, -| and any name bound to {string currentfile exch readstring pop}
  std::vector<std::string> rd = {"RD", "-|"};
  for (const uint8_t* q = p; (q = memfind(q, p + len - q, "readstring", 10)); q += 10) {
    const uint8_t* b = q;
    int back = 0;
    while (b > p && *b != '/' && back < 80) { b--; back++; }
    if (*b != '/') continue;
    const uint8_t* ne = b + 1;
    while (ne < q && isRegular(*ne) && *ne != '{') ne++;
    std::string name((const char*)b + 1, ne - b - 1);
    if (!name.empty() && name.size() < 16 && std::find(rd.begin(), rd.end(), name) == rd.end()) rd.push_back(name);
  }
  bool csDecrypt = charstrings;
  if (const uint8_t* li = memfind(p, len, "/lenIV", 6)) {
    Lexer lx(p, len, (size_t)(li - p) + 6);
    Obj v;
    if (lx.parse(v) && v.t == Obj::INT && v.i < 0) csDecrypt = false;
  }
  if (csDecrypt) {
    std::vector<std::pair<size_t, size_t>> blobs;
    t1FindBlobs(p, len, rd, blobs);
    if (blobs.empty()) csDecrypt = false;
    for (auto& b : blobs) t1Decrypt(p + b.first, b.second, 4330);
  }
  // split charstrings into pools (only with decrypted charstrings and every blob splittable)
  bool split = false;
  Bytes skel, ivP, opsP, numP;
  size_t lenIV = 4;
  if (csDecrypt) {
    if (const uint8_t* li = memfind(p, len, "/lenIV", 6)) {
      Lexer lx(p, len, (size_t)(li - p) + 6);
      Obj v;
      if (lx.parse(v) && v.t == Obj::INT && v.i >= 0 && v.i < 64) lenIV = (size_t)v.i;
    }
    std::vector<std::pair<size_t, size_t>> blobs;
    t1FindBlobs(p, len, rd, blobs);
    split = !blobs.empty();
    size_t pos = 0;
    for (auto& b : blobs) {
      if (!split) break;
      skel.insert(skel.end(), p + pos, p + b.first);
      split = t1CsSplit(p + b.first, b.second, lenIV, ivP, opsP, numP);
      pos = b.first + b.second;
    }
    if (split) skel.insert(skel.end(), p + pos, p + len);
  }
  recon.clear();
  putVarTo(recon, s);
  putVarTo(recon, len);
  recon.push_back((uint8_t)((csDecrypt ? 1 : 0) | (split ? 2 : 0)));
  recon.push_back((uint8_t)rd.size());
  for (auto& x : rd) putLenBytesTo(recon, (const uint8_t*)x.data(), x.size());
  if (split) {
    size_t tail = out.size() - (s + len);
    putVarTo(recon, lenIV);
    putVarTo(recon, skel.size());
    putVarTo(recon, ivP.size());
    putVarTo(recon, opsP.size());
    putVarTo(recon, tail);
    Bytes o2(out.begin(), out.begin() + s);
    o2.insert(o2.end(), skel.begin(), skel.end());
    o2.insert(o2.end(), ivP.begin(), ivP.end());
    o2.insert(o2.end(), opsP.begin(), opsP.end());
    o2.insert(o2.end(), numP.begin(), numP.end());
    o2.insert(o2.end(), out.begin() + s + len, out.end());
    out.swap(o2);
  }
  return true;
}
