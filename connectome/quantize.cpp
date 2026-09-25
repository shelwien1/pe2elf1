// quantize.cpp - data.tsv + pred.tsv <-> one integer TSV (fixed-point)
//
// usage: quantize c [-dN] seq.tsv pred.tsv int_seqpred.tsv
//        quantize d int_seqpred.tsv seq_out.tsv pred_out.tsv
//
// c: every float column is stored as round(v * 10^N) (default N=4, max abs
//    error 0.5e-4). The header keeps the scale in the column name,
//    e.g. "i0_p0/10000"; seq_ix, step_in_seq, need_prediction, is_scored
//    stay as they are. The predictions become the last two columns
//    pred_t0, pred_t1 (0 on rows without need_prediction).
//    pred.tsv may have one row per need_prediction row or per data row.
// d: restores seq_out.tsv (same columns as seq.tsv) and pred_out.tsv
//    (seq_ix step_in_seq t0 t1, one row per need_prediction row).
//    Values are written as exact decimals, e.g. 12345/10000 -> 1.2345.
#include "tsv.h"

static bool is_int_col(const std::string& n) {
  return n == "seq_ix" || n == "step_in_seq" || n == "need_prediction" || n == "is_scored";
}

static long long quant(float v, double scale, const TSV& t, size_t r, int c) {
  if (!std::isfinite(v)) die("%s: row %zu col %s: nonfinite value", t.fname.c_str(), r + 1, t.names[c].c_str());
  double x = v * scale;
  if (fabs(x) > 9e18) die("%s: row %zu col %s: value too large", t.fname.c_str(), r + 1, t.names[c].c_str());
  return llround(x);
}

// q / 10^k as a plain decimal string ("-1.2345", "2", "0.05")
static void put_fixed(FILE* f, long long q, int k, long long scale) {
  if (k == 0) { fprintf(f, "%lld", q); return; }
  unsigned long long a = q < 0 ? 0ULL - (unsigned long long)q : (unsigned long long)q;
  unsigned long long ip = a / scale, fp = a % scale;
  if (q < 0) fputc('-', f);
  if (!fp) { fprintf(f, "%llu", ip); return; }
  char s[32]; sprintf(s, "%0*llu", k, fp);
  int n = k; while (s[n - 1] == '0') n--;
  s[n] = 0;
  fprintf(f, "%llu.%s", ip, s);
}

static void compress(int dec, const char* sfn, const char* pfn, const char* ofn) {
  long long scale = 1; for (int i = 0; i < dec; i++) scale *= 10;
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
  for (size_t c = 0; c < d.ncols; c++) isint[c] = is_int_col(d.names[c]);
  FILE* f = fopen(ofn, "wb"); if (!f) die("can't create %s", ofn);
  for (size_t c = 0; c < d.ncols; c++)
    fprintf(f, isint[c] ? "%s\t" : "%s/%lld\t", d.names[c].c_str(), scale);
  fprintf(f, "pred_t0/%lld\tpred_t1/%lld\n", scale, scale);

  size_t pr = 0;
  for (size_t r = 0; r < d.nrows; r++) {
    for (size_t c = 0; c < d.ncols; c++) {
      if (isint[c]) {
        const char* n = d.names[c].c_str();
        long long v = (!strcmp(n, "need_prediction") || !strcmp(n, "is_scored")) ? d.b(r, c) : d.i(r, c);
        fprintf(f, "%lld\t", v);
      } else fprintf(f, "%lld\t", quant(d.f(r, c), double(scale), d, r, c));
    }
    bool need = d.b(r, c_need);
    long long q[2] = {0, 0};
    if (need || all_rows) {
      size_t pi = pr++;
      if (need) {
        if (p_seq >= 0 && p.i(pi, p_seq) != d.i(r, c_seq))
          die("%s: row %zu: seq_ix %lld, expected %lld", pfn, pi + 1, p.i(pi, p_seq), d.i(r, c_seq));
        if (p_step >= 0 && p.i(pi, p_step) != d.i(r, c_step))
          die("%s: row %zu: step_in_seq %lld, expected %lld", pfn, pi + 1, p.i(pi, p_step), d.i(r, c_step));
        for (int k = 0; k < 2; k++) q[k] = quant(p.f(pi, p_t[k]), double(scale), p, pi, p_t[k]);
      }
    }
    fprintf(f, "%lld\t%lld\n", q[0], q[1]);
  }
  fclose(f);
  fprintf(stderr, "%s + %s -> %s: %zu rows, %zu predictions, scale %lld\n", sfn, pfn, ofn, d.nrows, nneed, scale);
}

static void decompress(const char* ifn, const char* sfn, const char* pfn) {
  TSV q; q.load(ifn);
  std::vector<std::string> name(q.ncols);
  std::vector<int> dec(q.ncols, 0);
  std::vector<long long> scale(q.ncols, 1);
  for (size_t c = 0; c < q.ncols; c++) {
    const std::string& n = q.names[c];
    size_t s = n.rfind('/');
    name[c] = n.substr(0, s);
    if (s == std::string::npos) continue;
    const char* t = n.c_str() + s + 1;
    if (t[0] != '1') die("%s: column %s: scale must be a power of 10", ifn, n.c_str());
    for (t++; *t; t++) {
      if (*t != '0' || dec[c] >= 18) die("%s: column %s: scale must be a power of 10", ifn, n.c_str());
      dec[c]++; scale[c] *= 10;
    }
  }
  int c_seq = -1, c_step = -1, c_need = -1, c_p[2] = {-1, -1};
  for (size_t c = 0; c < q.ncols; c++) {
    if (name[c] == "seq_ix") c_seq = c;
    if (name[c] == "step_in_seq") c_step = c;
    if (name[c] == "need_prediction") c_need = c;
    if (name[c] == "pred_t0") c_p[0] = c;
    if (name[c] == "pred_t1") c_p[1] = c;
  }
  if (c_seq < 0 || c_step < 0 || c_need < 0 || c_p[0] < 0 || c_p[1] < 0)
    die("%s: needs seq_ix, step_in_seq, need_prediction, pred_t0, pred_t1", ifn);

  FILE* fs = fopen(sfn, "wb"); if (!fs) die("can't create %s", sfn);
  FILE* fp = fopen(pfn, "wb"); if (!fp) die("can't create %s", pfn);
  std::vector<int> cols; // data columns, in file order
  for (size_t c = 0; c < q.ncols; c++) if (int(c) != c_p[0] && int(c) != c_p[1]) cols.push_back(c);
  for (size_t j = 0; j < cols.size(); j++) fprintf(fs, "%s%c", name[cols[j]].c_str(), j + 1 < cols.size() ? '\t' : '\n');
  fprintf(fp, "seq_ix\tstep_in_seq\tt0\tt1\n");
  size_t np = 0;
  for (size_t r = 0; r < q.nrows; r++) {
    for (size_t j = 0; j < cols.size(); j++) {
      int c = cols[j];
      put_fixed(fs, q.i(r, c), dec[c], scale[c]);
      fputc(j + 1 < cols.size() ? '\t' : '\n', fs);
    }
    if (q.i(r, c_need)) {
      fprintf(fp, "%lld\t%lld\t", q.i(r, c_seq), q.i(r, c_step));
      put_fixed(fp, q.i(r, c_p[0]), dec[c_p[0]], scale[c_p[0]]); fputc('\t', fp);
      put_fixed(fp, q.i(r, c_p[1]), dec[c_p[1]], scale[c_p[1]]); fputc('\n', fp);
      np++;
    }
  }
  fclose(fs); fclose(fp);
  fprintf(stderr, "%s -> %s + %s: %zu rows, %zu predictions\n", ifn, sfn, pfn, q.nrows, np);
}

int main(int argc, char** argv) {
  int dec = 4, a = 2;
  if (argc > 1 && !strcmp(argv[1], "c")) {
    for (; a < argc && argv[a][0] == '-' && argv[a][1]; a++) {
      if (argv[a][1] == 'd') dec = atoi(argv[a] + 2);
      else die("unknown option %s", argv[a]);
    }
    if (argc - a == 3 && dec >= 0 && dec <= 9) { compress(dec, argv[a], argv[a + 1], argv[a + 2]); return 0; }
  } else if (argc == 5 && !strcmp(argv[1], "d")) {
    decompress(argv[2], argv[3], argv[4]);
    return 0;
  }
  fprintf(stderr,
    "usage: quantize c [-dN] seq.tsv pred.tsv int_seqpred.tsv\n"
    "       quantize d int_seqpred.tsv seq_out.tsv pred_out.tsv\n"
    "  -dN  keep N decimal digits: floats are stored as round(v*10^N) (default 4, max 9)\n");
  return 1;
}
