// pdf_t1.inc -- Type 1 font decryption layer (L_T1).
//
// Removes the eexec encryption (r = 55665) of the private part and, inside it, the charstring
// encryption (r = 4330) of every "<len> RD <binary>" blob. Both ciphers are deterministic given
// the plaintext (the random lead bytes are part of the plaintext), so re-encryption reproduces
// the original exactly. The result compresses far better than the encrypted font, but it is not
// a loadable font any more: it is written with the extension .t1d.
//
// Recon record: var start of the encrypted part, var its length, u8 flags (bit0: charstrings
// decrypted), u8 count + RD token names (length-prefixed).

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
                        std::vector<std::pair<size_t, size_t>>& blobs) {
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
    if (!ok || te + 1 + L > n) { i = j; continue; }
    blobs.push_back({te + 1, (size_t)L});
    i = te + 1 + (size_t)L;
  }
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
  if (s > in.size() || n > in.size() - s) return false;
  out = in;
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
  recon.clear();
  putVarTo(recon, s);
  putVarTo(recon, len);
  recon.push_back(csDecrypt ? 1 : 0);
  recon.push_back((uint8_t)rd.size());
  for (auto& x : rd) putLenBytesTo(recon, (const uint8_t*)x.data(), x.size());
  return true;
}
