// quantize.cpp - data.tsv + pred.tsv <-> one integer table (fixed-point),
// stored as TSV text or as little-endian packed binary
//
// usage: quantize c|bc [-dN] seq.tsv pred.tsv int_seqpred.(tsv|bin)
//        quantize d|bd int_seqpred.(tsv|bin) seq_out.tsv pred_out.tsv
//
// c/bc: every float column is stored as round(v * 10^N) (default N=4, max
//    abs error 0.5e-4); seq_ix, step_in_seq, need_prediction, is_scored stay
//    as they are. The predictions are the first two columns pred_t0, pred_t1
//    (0 on rows without need_prediction). pred.tsv may have one row per
//    need_prediction row or per data row.
// d/bd: restore seq_out.tsv (same columns as seq.tsv) and pred_out.tsv
//    (seq_ix step_in_seq t0 t1, one row per need_prediction row).
//    Values are written as exact decimals, e.g. 12345/10000 -> 1.2345.
//
// Text format (c): header with the scale in the column name, e.g. "i0_p0/10000".
//
// Binary format (bc), all integers little-endian:
//   char[4] magic "CQB1"
//   u32     ncols
//   u32     nrows
//   ncols x column descriptor:
//     u8    name length L, then L bytes of name (no terminator)
//     u8    decimals N (value = stored / 10^N; 0 for integer columns)
//     u8    width W in bytes, 1..8 (smallest that holds the column's range)
//   nrows x record: for each column, W bytes, signed two's complement
//   (record size = sum of W; rows in file order)
#include "tsv.h"

static bool is_int_col(const std::string& n) {
  return n == "seq_ix" || n == "step_in_seq" || n == "need_prediction" || n == "is_scored";
}

// integer table: names without scale suffix, decimals per column, row-major values
struct Table {
  std::vector<std::string> names;
  std::vector<int> dec;
  std::vector<long long> v;
  size_t ncols = 0, nrows = 0;
  long long at(size_t r, size_t c) const { return v[r * ncols + c]; }
  int col(const char* n) const {
    for (size_t c = 0; c < ncols; c++) if (names[c] == n) return int(c);
    return -1;
  }
};

static long long pow10ll(int k) { long long s = 1; while (k-- > 0) s *= 10; return s; }

static long long quant(float v, double scale, const TSV& t, size_t r, int c) {
  if (!std::isfinite(v)) die("%s: row %zu col %s: nonfinite value", t.fname.c_str(), r + 1, t.names[c].c_str());
  double x = v * scale;
  if (fabs(x) > 9e18) die("%s: row %zu col %s: value too large", t.fname.c_str(), r + 1, t.names[c].c_str());
  return llround(x);
}

// q / 10^k as a plain decimal string ("-1.2345", "2", "0.05")
static void put_fixed(FILE* f, long long q, int k) {
  if (k == 0) { fprintf(f, "%lld", q); return; }
  unsigned long long scale = pow10ll(k);
  unsigned long long a = q < 0 ? 0ULL - (unsigned long long)q : (unsigned long long)q;
  unsigned long long ip = a / scale, fp = a % scale;
  if (q < 0) fputc('-', f);
  if (!fp) { fprintf(f, "%llu", ip); return; }
  char s[32]; snprintf(s, sizeof s, "%0*llu", k > 19 ? 19 : k, fp); // k <= 18
  int n = k; while (s[n - 1] == '0') n--;
  s[n] = 0;
  fprintf(f, "%llu.%s", ip, s);
}

// seq.tsv + pred.tsv -> Table
static void quantize(int dec, const char* sfn, const char* pfn, Table& t) {
  double scale = double(pow10ll(dec));
  TSV d, p;
  d.load(sfn); p.load(pfn);
  int c_seq = d.need("seq_ix"), c_step = d.col("step_in_seq"), c_need = d.need("need_prediction");
  int p_t[2] = {p.need("t0"), p.need("t1")}, p_seq = p.col("seq_ix"), p_step = p.col("step_in_seq");
  size_t nneed = 0;
  for (size_t r = 0; r < d.nrows; r++) nneed += d.b(r, c_need);
  bool all_rows = p.nrows == d.nrows;
  if (!all_rows && p.nrows != nneed)
    die("%s has %zu rows, expected %zu (need_prediction rows of %s) or %zu", pfn, p.nrows, nneed, sfn, d.nrows);
  if (p_step >= 0 && c_step < 0) die("%s: no column 'step_in_seq'", sfn);

  std::vector<bool> isint(d.ncols);
  t.names = {"pred_t0", "pred_t1"};
  t.dec = {dec, dec};
  for (size_t c = 0; c < d.ncols; c++) {
    isint[c] = is_int_col(d.names[c]);
    t.names.push_back(d.names[c]);
    t.dec.push_back(isint[c] ? 0 : dec);
  }
  t.ncols = t.names.size(); t.nrows = d.nrows;
  t.v.resize(t.ncols * t.nrows);

  size_t pr = 0;
  for (size_t r = 0; r < d.nrows; r++) {
    long long* row = &t.v[r * t.ncols];
    bool need = d.b(r, c_need);
    row[0] = row[1] = 0;
    if (need || all_rows) {
      size_t pi = pr++;
      if (need) {
        if (p_seq >= 0 && p.i(pi, p_seq) != d.i(r, c_seq))
          die("%s: row %zu: seq_ix %lld, expected %lld", pfn, pi + 1, p.i(pi, p_seq), d.i(r, c_seq));
        if (p_step >= 0 && p.i(pi, p_step) != d.i(r, c_step))
          die("%s: row %zu: step_in_seq %lld, expected %lld", pfn, pi + 1, p.i(pi, p_step), d.i(r, c_step));
        for (int k = 0; k < 2; k++) row[k] = quant(p.f(pi, p_t[k]), scale, p, pi, p_t[k]);
      }
    }
    for (size_t c = 0; c < d.ncols; c++) {
      if (isint[c]) {
        const char* n = d.names[c].c_str();
        row[2 + c] = (!strcmp(n, "need_prediction") || !strcmp(n, "is_scored")) ? d.b(r, c) : d.i(r, c);
      } else row[2 + c] = quant(d.f(r, c), scale, d, r, c);
    }
  }
  fprintf(stderr, "%s + %s: %zu rows, %zu predictions, %d decimals\n", sfn, pfn, d.nrows, nneed, dec);
}

static void write_text(const Table& t, const char* fn) {
  FILE* f = fopen(fn, "wb"); if (!f) die("can't create %s", fn);
  for (size_t c = 0; c < t.ncols; c++) {
    fprintf(f, "%s%s", c ? "\t" : "", t.names[c].c_str());
    if (!is_int_col(t.names[c])) fprintf(f, "/%lld", pow10ll(t.dec[c]));
  }
  fputc('\n', f);
  for (size_t r = 0; r < t.nrows; r++)
    for (size_t c = 0; c < t.ncols; c++) fprintf(f, "%lld%c", t.at(r, c), c + 1 < t.ncols ? '\t' : '\n');
  fclose(f);
}

static void read_text(const char* fn, Table& t) {
  TSV q; q.load(fn);
  t.ncols = q.ncols; t.nrows = q.nrows;
  t.names.resize(t.ncols); t.dec.assign(t.ncols, 0);
  for (size_t c = 0; c < t.ncols; c++) {
    const std::string& n = q.names[c];
    size_t s = n.rfind('/');
    t.names[c] = n.substr(0, s);
    if (s == std::string::npos) continue;
    const char* p = n.c_str() + s + 1;
    if (p[0] != '1') die("%s: column %s: scale must be a power of 10", fn, n.c_str());
    for (p++; *p; p++) {
      if (*p != '0' || t.dec[c] >= 18) die("%s: column %s: scale must be a power of 10", fn, n.c_str());
      t.dec[c]++;
    }
  }
  t.v.resize(t.ncols * t.nrows);
  for (size_t r = 0; r < t.nrows; r++)
    for (size_t c = 0; c < t.ncols; c++) t.v[r * t.ncols + c] = q.i(r, c);
}

static void put_le(std::vector<unsigned char>& b, unsigned long long x, int w) {
  for (int i = 0; i < w; i++) b.push_back((unsigned char)(x >> (8 * i)));
}

static void write_bin(const Table& t, const char* fn) {
  if (t.nrows > 0xFFFFFFFFu || t.ncols > 0xFFFFFFFFu) die("table too large for the binary format");
  std::vector<int> w(t.ncols);
  std::vector<unsigned char> b;
  b.insert(b.end(), {'C', 'Q', 'B', '1'});
  put_le(b, t.ncols, 4); put_le(b, t.nrows, 4);
  for (size_t c = 0; c < t.ncols; c++) {
    long long lo = 0, hi = 0;
    for (size_t r = 0; r < t.nrows; r++) { long long x = t.at(r, c); if (x < lo) lo = x; if (x > hi) hi = x; }
    w[c] = 1;
    while (w[c] < 8 && (lo < -(1LL << (8 * w[c] - 1)) || hi > (1LL << (8 * w[c] - 1)) - 1)) w[c]++;
    if (t.names[c].size() > 255) die("column name too long: %s", t.names[c].c_str());
    b.push_back((unsigned char)t.names[c].size());
    b.insert(b.end(), t.names[c].begin(), t.names[c].end());
    b.push_back((unsigned char)t.dec[c]);
    b.push_back((unsigned char)w[c]);
  }
  size_t hdr = b.size();
  for (size_t r = 0; r < t.nrows; r++)
    for (size_t c = 0; c < t.ncols; c++) put_le(b, (unsigned long long)t.at(r, c), w[c]);
  FILE* f = fopen(fn, "wb"); if (!f) die("can't create %s", fn);
  if (fwrite(b.data(), 1, b.size(), f) != b.size()) die("can't write %s", fn);
  fclose(f);
  fprintf(stderr, "%s: header %zu bytes, record %zu bytes x %zu rows\n", fn, hdr,
          t.nrows ? (b.size() - hdr) / t.nrows : 0, t.nrows);
}

static void read_bin(const char* fn, Table& t) {
  FILE* f = fopen(fn, "rb"); if (!f) die("can't open %s", fn);
  std::vector<unsigned char> b;
  unsigned char buf[1 << 16]; size_t n;
  while ((n = fread(buf, 1, sizeof buf, f)) > 0) b.insert(b.end(), buf, buf + n);
  fclose(f);
  size_t pos = 0;
  auto need = [&](size_t k) { if (b.size() - pos < k) die("%s: truncated file", fn); };
  auto get = [&](int w) {
    need(w);
    unsigned long long x = 0;
    for (int i = 0; i < w; i++) x |= (unsigned long long)b[pos++] << (8 * i);
    return x;
  };
  need(4);
  if (memcmp(&b[0], "CQB1", 4)) die("%s: not a CQB1 file", fn);
  pos = 4;
  t.ncols = get(4); t.nrows = get(4);
  std::vector<int> w(t.ncols);
  t.names.resize(t.ncols); t.dec.resize(t.ncols);
  for (size_t c = 0; c < t.ncols; c++) {
    int L = int(get(1)); need(L);
    t.names[c].assign((const char*)&b[pos], L); pos += L;
    t.dec[c] = int(get(1)); w[c] = int(get(1));
    if (w[c] < 1 || w[c] > 8 || t.dec[c] > 18) die("%s: bad descriptor for column %s", fn, t.names[c].c_str());
  }
  t.v.resize(t.ncols * t.nrows);
  for (size_t r = 0; r < t.nrows; r++)
    for (size_t c = 0; c < t.ncols; c++) {
      unsigned long long x = get(w[c]);
      if (w[c] < 8 && (x >> (8 * w[c] - 1))) x |= ~0ULL << (8 * w[c]); // sign-extend
      t.v[r * t.ncols + c] = (long long)x;
    }
  if (pos != b.size()) die("%s: %zu extra bytes at the end", fn, b.size() - pos);
}

// Table -> seq_out.tsv + pred_out.tsv
static void dequantize(const Table& t, const char* src, const char* sfn, const char* pfn) {
  int c_seq = t.col("seq_ix"), c_step = t.col("step_in_seq"), c_need = t.col("need_prediction");
  int c_p[2] = {t.col("pred_t0"), t.col("pred_t1")};
  if (c_seq < 0 || c_step < 0 || c_need < 0 || c_p[0] < 0 || c_p[1] < 0)
    die("%s: needs seq_ix, step_in_seq, need_prediction, pred_t0, pred_t1", src);
  FILE* fs = fopen(sfn, "wb"); if (!fs) die("can't create %s", sfn);
  FILE* fp = fopen(pfn, "wb"); if (!fp) die("can't create %s", pfn);
  std::vector<int> cols; // data columns, in file order
  for (size_t c = 0; c < t.ncols; c++) if (int(c) != c_p[0] && int(c) != c_p[1]) cols.push_back(c);
  for (size_t j = 0; j < cols.size(); j++) fprintf(fs, "%s%c", t.names[cols[j]].c_str(), j + 1 < cols.size() ? '\t' : '\n');
  fprintf(fp, "seq_ix\tstep_in_seq\tt0\tt1\n");
  size_t np = 0;
  for (size_t r = 0; r < t.nrows; r++) {
    for (size_t j = 0; j < cols.size(); j++) {
      put_fixed(fs, t.at(r, cols[j]), t.dec[cols[j]]);
      fputc(j + 1 < cols.size() ? '\t' : '\n', fs);
    }
    if (t.at(r, c_need)) {
      fprintf(fp, "%lld\t%lld\t", t.at(r, c_seq), t.at(r, c_step));
      put_fixed(fp, t.at(r, c_p[0]), t.dec[c_p[0]]); fputc('\t', fp);
      put_fixed(fp, t.at(r, c_p[1]), t.dec[c_p[1]]); fputc('\n', fp);
      np++;
    }
  }
  fclose(fs); fclose(fp);
  fprintf(stderr, "%s -> %s + %s: %zu rows, %zu predictions\n", src, sfn, pfn, t.nrows, np);
}

int main(int argc, char** argv) {
  const char* cmd = argc > 1 ? argv[1] : "";
  bool bin = cmd[0] == 'b';
  const char* op = cmd + bin;
  Table t;
  if (!strcmp(op, "c")) {
    int dec = 4, a = 2;
    for (; a < argc && argv[a][0] == '-' && argv[a][1]; a++) {
      if (argv[a][1] == 'd') dec = atoi(argv[a] + 2);
      else die("unknown option %s", argv[a]);
    }
    if (argc - a == 3 && dec >= 0 && dec <= 9) {
      quantize(dec, argv[a], argv[a + 1], t);
      if (bin) write_bin(t, argv[a + 2]); else write_text(t, argv[a + 2]);
      return 0;
    }
  } else if (!strcmp(op, "d") && argc == 5) {
    if (bin) read_bin(argv[2], t); else read_text(argv[2], t);
    dequantize(t, argv[2], argv[3], argv[4]);
    return 0;
  }
  fprintf(stderr,
    "usage: quantize c|bc [-dN] seq.tsv pred.tsv int_seqpred.(tsv|bin)\n"
    "       quantize d|bd int_seqpred.(tsv|bin) seq_out.tsv pred_out.tsv\n"
    "  c/d: integer TSV; bc/bd: little-endian packed binary (format in quantize.cpp)\n"
    "  -dN  keep N decimal digits: floats are stored as round(v*10^N) (default 4, max 9)\n");
  return 1;
}
