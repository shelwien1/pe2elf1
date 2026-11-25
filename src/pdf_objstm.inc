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
