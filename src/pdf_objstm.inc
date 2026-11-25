// pdf_objstm.inc -- object stream header derivation (L_OBJSTM layer).
//
// A decoded object stream is "n1 o1 n2 o2 ... nN oN" followed, at /First, by the member
// objects. The layer keeps only the members. The offsets are predicted by lexing: member k
// starts where member k-1 ends plus the whitespace after it (the decoder runs the same lexer on
// the same bytes); mismatches are stored. Object numbers are stored as deltas from prev+1.
//
// Recon: var N, u8 separator inside a pair, u8 separator between pairs, var tail length + tail
// (bytes after the last offset up to /First), N varints zigzag(n - prev - 1), u8 offsets-exact,
// [N varints zigzag(off - predicted)].

static void objstmPredict(const uint8_t* m, size_t n, size_t k, const std::vector<uint64_t>& offs, uint64_t& pred) {
  if (k == 0) { pred = 0; return; }
  size_t st = (size_t)std::min<uint64_t>(offs[k - 1], n);
  Lexer lx(m, n, st);
  Obj o;
  if (!lx.parse(o)) { pred = st; return; }
  size_t e = lx.pos;
  while (e < n && isWS(m[e])) e++;
  pred = e;
}

static void objstmHeader(const std::vector<uint64_t>& nums, const std::vector<uint64_t>& offs, uint8_t sepA, uint8_t sepB,
                         const Bytes& tail, Bytes& out) {
  char buf[24];
  for (size_t k = 0; k < nums.size(); k++) {
    if (k) out.push_back(sepB);
    int l = snprintf(buf, sizeof(buf), "%llu", (unsigned long long)nums[k]);
    out.insert(out.end(), buf, buf + l);
    out.push_back(sepA);
    l = snprintf(buf, sizeof(buf), "%llu", (unsigned long long)offs[k]);
    out.insert(out.end(), buf, buf + l);
  }
  out.insert(out.end(), tail.begin(), tail.end());
}

static bool objstmDecodeLayer(const Bytes& in, int64_t N, int64_t First, Bytes& out, Bytes& recon) {
  if (N <= 0 || First <= 0 || (uint64_t)First > in.size() || N > First) return false;
  // parse the header strictly: digits, one separator byte, ...
  std::vector<uint64_t> nums, offs;
  size_t p = 0, f = (size_t)First;
  int sepA = -1, sepB = -1;
  for (int64_t k = 0; k < N; k++) {
    for (int part = 0; part < 2; part++) {
      if (k || part) {
        if (p >= f || !isWS(in[p])) return false;
        int& sep = part ? sepA : sepB;
        if (sep < 0) sep = in[p];
        else if (sep != in[p]) return false;
        p++;
      }
      size_t st = p;
      uint64_t v = 0;
      while (p < f && in[p] >= '0' && in[p] <= '9' && p - st < 18) v = v * 10 + (in[p++] - '0');
      if (p == st || (p - st > 1 && in[st] == '0')) return false;
      (part ? offs : nums).push_back(v);
    }
  }
  if (sepB < 0) sepB = ' ';
  Bytes tail(in.begin() + p, in.begin() + f);
  for (uint8_t c : tail)
    if (!isWS(c)) return false;
  const uint8_t* m = in.data() + f;
  size_t mn = in.size() - f;
  std::vector<int64_t> corr((size_t)N);
  bool exact = true;
  for (size_t k = 0; k < (size_t)N; k++) {
    uint64_t pred;
    objstmPredict(m, mn, k, offs, pred);
    corr[k] = (int64_t)(offs[k] - pred);
    exact &= corr[k] == 0;
  }
  recon.clear();
  putVarTo(recon, (uint64_t)N);
  recon.push_back((uint8_t)sepA);
  recon.push_back((uint8_t)sepB);
  putLenBytesTo(recon, tail.data(), tail.size());
  uint64_t prev = (uint64_t)-1;
  for (uint64_t v : nums) {
    putVarTo(recon, zigzag((int64_t)(v - prev - 1)));
    prev = v;
  }
  recon.push_back(exact ? 1 : 0);
  if (!exact)
    for (int64_t c : corr) putVarTo(recon, zigzag(c));
  out.assign(in.begin() + f, in.end());
  return true;
}

static bool objstmEncodeLayer(const Bytes& recon, const Bytes& in, Bytes& out) {
  RBuf r(recon);
  size_t N = r.len();
  if (N > in.size() + 1) return false;
  uint8_t sepA = r.u8(), sepB = r.u8();
  Bytes tail;
  r.bytes(r.len(), tail);
  std::vector<uint64_t> nums(N), offs(N);
  uint64_t prev = (uint64_t)-1;
  for (auto& v : nums) {
    v = prev + 1 + (uint64_t)unzigzag(r.var());
    prev = v;
  }
  bool exact = r.u8() != 0;
  for (size_t k = 0; k < N; k++) {
    uint64_t pred;
    objstmPredict(in.data(), in.size(), k, offs, pred);
    offs[k] = exact ? pred : pred + (uint64_t)unzigzag(r.var());
  }
  out.clear();
  objstmHeader(nums, offs, sepA, sepB, tail, out);
  out.insert(out.end(), in.begin(), in.end());
  return true;
}

// ---- XRef stream rows (L_XREFROWS) ------------------------------------------------------------
// Decoded XRef stream rows (fields of /W widths, objects listed by /Index) are replaced by
// residuals against predictions: type 1 -> offset and generation of the latest definition of
// that object written so far (CodecCtx::objPos, tracked identically on both sides); type 2 ->
// container = previous type-2 container, index = previous index + 1 in the same container;
// other types -> 0. Output per row: the type byte (when /W[0] > 0), zigzag varint residuals.
//
// Recon: u8 W0 W1 W2, var number of /Index ranges, (start, count) pairs, var tail length + tail.

static bool xrefRowsGeometry(const std::vector<uint64_t>& w) {
  return w.size() == 3 && w[0] <= 1 && w[1] <= 8 && w[2] <= 8 && w[0] + w[1] + w[2] > 0;
}

static void xrefRowsPredict(const ObjPositions* op, uint64_t num, uint64_t type, uint64_t f2, int64_t& lastC,
                            int64_t& lastI, bool second, uint64_t& pred) {
  if (type == 1) {
    uint64_t off = 0;
    uint32_t gen = 0;
    if (op) op->get(num, off, gen);
    pred = second ? gen : off;
  } else if (type == 2) {
    if (!second) pred = lastC < 0 ? 0 : (uint64_t)lastC;
    else pred = ((int64_t)f2 == lastC && lastI >= 0) ? (uint64_t)(lastI + 1) : 0;
  } else {
    pred = 0;
  }
}

static bool xrefRowsDecodeLayer(const CodecCtx& ctx, const Bytes& in, const std::vector<uint64_t>& W,
                                const std::vector<std::pair<uint64_t, uint64_t>>& index, Bytes& out, Bytes& recon) {
  if (!xrefRowsGeometry(W)) return false;
  size_t rl = (size_t)(W[0] + W[1] + W[2]);
  uint64_t rows = 0;
  for (auto& r : index) rows += r.second;
  if (rows == 0 || rows > in.size() / rl) return false;
  out.clear();
  size_t p = 0;
  int64_t lastC = -1, lastI = -1;
  for (auto& rg : index)
    for (uint64_t k = 0; k < rg.second; k++) {
      uint64_t f[3] = {W[0] ? 0u : 1u, 0, 0};
      for (int i = 0; i < 3; i++)
        if (W[i]) {
          uint64_t v = 0;
          for (uint64_t b = 0; b < W[i]; b++) v = v << 8 | in[p++];
          f[i] = v;
        }
      uint64_t num = rg.first + k, pred;
      if (W[0]) out.push_back((uint8_t)f[0]);
      xrefRowsPredict(ctx.objPos, num, f[0], f[1], lastC, lastI, false, pred);
      putVarTo(out, zigzag((int64_t)(f[1] - pred)));
      xrefRowsPredict(ctx.objPos, num, f[0], f[1], lastC, lastI, true, pred);
      putVarTo(out, zigzag((int64_t)(f[2] - pred)));
      if (f[0] == 2) { lastC = (int64_t)f[1]; lastI = (int64_t)f[2]; }
    }
  recon.clear();
  for (int i = 0; i < 3; i++) recon.push_back((uint8_t)W[i]);
  putVarTo(recon, index.size());
  for (auto& r : index) { putVarTo(recon, r.first); putVarTo(recon, r.second); }
  putLenBytesTo(recon, in.data() + p, in.size() - p);
  return true;
}

static bool xrefRowsEncodeLayer(const CodecCtx& ctx, const Bytes& recon, const Bytes& in, Bytes& out) {
  RBuf r(recon);
  std::vector<uint64_t> W(3);
  for (auto& w : W) w = r.u8();
  if (!xrefRowsGeometry(W)) return false;
  size_t ni = r.len();
  std::vector<std::pair<uint64_t, uint64_t>> index(ni);
  for (auto& x : index) { x.first = r.var(); x.second = r.var(); }
  RBuf q(in);
  out.clear();
  int64_t lastC = -1, lastI = -1;
  for (auto& rg : index) {
    if (rg.second > in.size()) return false;
    for (uint64_t k = 0; k < rg.second; k++) {
      uint64_t f[3] = {1, 0, 0}, num = rg.first + k, pred;
      if (W[0]) f[0] = q.u8();
      xrefRowsPredict(ctx.objPos, num, f[0], 0, lastC, lastI, false, pred);
      f[1] = pred + (uint64_t)unzigzag(q.var());
      xrefRowsPredict(ctx.objPos, num, f[0], f[1], lastC, lastI, true, pred);
      f[2] = pred + (uint64_t)unzigzag(q.var());
      if (f[0] == 2) { lastC = (int64_t)f[1]; lastI = (int64_t)f[2]; }
      for (int i = 0; i < 3; i++)
        for (uint64_t b = W[i]; b-- > 0;) out.push_back((uint8_t)(f[i] >> (8 * b)));
    }
  }
  if (q.p != in.size()) return false;
  r.appendTo(r.len(), out);
  return true;
}
