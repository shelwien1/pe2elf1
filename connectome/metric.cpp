// metric.cpp - Global Weighted Pearson (WP), same as utils.GlobalAccumulator
// of the wnn_connectome starter pack (see METRIC.md).
//
// usage: metric [-dN] data.tsv pred.tsv [data2.tsv pred2.tsv ...]
//   -dN: print N decimals (default 9)
//
// data.tsv: needs seq_ix, need_prediction, t0, t1 and (optionally) is_scored;
//           without is_scored every need_prediction row is scored.
// pred.tsv: needs t0, t1; one row per need_prediction row of data.tsv
//           (or one row per data row). If seq_ix/step_in_seq columns are
//           present, they are checked against data.tsv.
// All pairs are pooled into one global score (TOTAL), like on the platform.
#include "tsv.h"

static const float CLIP = 2.0f;
static int decimals = 9;

// Weighted moments of one target, merged per sequence (Chan et al.)
struct WP {
  double W = 0, my = 0, mp = 0, m2y = 0, m2p = 0, cxy = 0;

  void add(const std::vector<double>& y, const std::vector<double>& p) {
    size_t n = y.size();
    double w = 0, sy = 0, sp = 0;
    for (size_t i = 0; i < n; i++) { double wi = fabs(y[i]); w += wi; sy += wi * y[i]; sp += wi * p[i]; }
    if (w == 0) return;
    double mean_y = sy / w, mean_p = sp / w, a = 0, b = 0, c = 0;
    for (size_t i = 0; i < n; i++) {
      double wi = fabs(y[i]), yc = y[i] - mean_y, pc = p[i] - mean_p;
      a += wi * yc * yc; b += wi * pc * pc; c += wi * yc * pc;
    }
    double total = W + w, dy = mean_y - my, dp = mean_p - mp, corr = W * w / total;
    m2y += a + dy * dy * corr;
    m2p += b + dp * dp * corr;
    cxy += c + dy * dp * corr;
    my += dy * w / total;
    mp += dp * w / total;
    W = total;
  }
  void merge(const WP& o) { // same formula, block = another accumulator
    if (o.W == 0) return;
    double total = W + o.W, dy = o.my - my, dp = o.mp - mp, corr = W * o.W / total;
    m2y += o.m2y + dy * dy * corr;
    m2p += o.m2p + dp * dp * corr;
    cxy += o.cxy + dy * dp * corr;
    my += dy * o.W / total;
    mp += dp * o.W / total;
    W = total;
  }
  double result() const {
    if (W < 1e-8) return 0;
    double sy = sqrt(fmax(0.0, m2y / W)), sp = sqrt(fmax(0.0, m2p / W));
    if (!(sy > 1e-8 && sp > 1e-8)) return 0;
    double r = cxy / W / (sy * sp);
    return r < -1 ? -1 : r > 1 ? 1 : r;
  }
};

struct Score {
  WP t[2];
  long long seqs = 0, need = 0, scored = 0;
  void merge(const Score& o) {
    for (int k = 0; k < 2; k++) t[k].merge(o.t[k]);
    seqs += o.seqs; need += o.need; scored += o.scored;
  }
  void print(const char* name) const {
    double r0 = t[0].result(), r1 = t[1].result();
    printf("%-24s t0=%.*f t1=%.*f WP=%.*f  (seqs=%lld need=%lld scored=%lld)\n",
           name, decimals, r0, decimals, r1, decimals, (r0 + r1) / 2, seqs, need, scored);
  }
};

static void score_pair(const char* dfn, const char* pfn, Score& s) {
  TSV d, p;
  d.load(dfn); p.load(pfn);
  int c_seq = d.need("seq_ix"), c_step = d.col("step_in_seq"), c_need = d.need("need_prediction");
  int c_sc = d.col("is_scored"), c_t[2] = {d.need("t0"), d.need("t1")};
  int p_t[2] = {p.need("t0"), p.need("t1")}, p_seq = p.col("seq_ix"), p_step = p.col("step_in_seq");
  if (c_sc < 0) fprintf(stderr, "note: %s has no is_scored column, scoring all need_prediction rows\n", dfn);

  size_t nneed = 0;
  for (size_t r = 0; r < d.nrows; r++) nneed += d.b(r, c_need);
  bool all_rows = p.nrows == d.nrows; // prediction for every data row (warm-up ones ignored)
  if (!all_rows && p.nrows != nneed)
    die("%s has %zu rows, expected %zu (need_prediction rows of %s) or %zu", pfn, p.nrows, nneed, dfn, d.nrows);
  if (p_step >= 0 && c_step < 0) die("%s: no column 'step_in_seq'", dfn);

  std::vector<double> y[2], q[2];
  size_t pr = 0;
  long long cur = 0;
  for (size_t r = 0; r <= d.nrows; r++) {
    long long seq = r < d.nrows ? d.i(r, c_seq) : 0;
    if (r > 0 && (r == d.nrows || seq != cur)) { // end of sequence block
      for (int k = 0; k < 2; k++) { s.t[k].add(y[k], q[k]); y[k].clear(); q[k].clear(); }
      s.seqs++;
    }
    if (r == d.nrows) break;
    cur = seq;
    float t[2] = {d.f(r, c_t[0]), d.f(r, c_t[1])};
    if (!std::isfinite(t[0]) || !std::isfinite(t[1])) die("%s: row %zu: nonfinite target", dfn, r + 1);
    bool need = d.b(r, c_need);
    if (!need && !all_rows) continue;
    size_t pi = pr++;
    if (!need) continue;
    s.need++;
    if (p_seq >= 0 && p.i(pi, p_seq) != seq)
      die("%s: row %zu: seq_ix %lld, expected %lld", pfn, pi + 1, p.i(pi, p_seq), seq);
    if (p_step >= 0 && p.i(pi, p_step) != d.i(r, c_step))
      die("%s: row %zu: step_in_seq %lld, expected %lld", pfn, pi + 1, p.i(pi, p_step), d.i(r, c_step));
    float v[2] = {p.f(pi, p_t[0]), p.f(pi, p_t[1])};
    if (!std::isfinite(v[0]) || !std::isfinite(v[1])) die("%s: row %zu: nonfinite prediction", pfn, pi + 1);
    if (c_sc >= 0 && !d.b(r, c_sc)) continue;
    s.scored++;
    for (int k = 0; k < 2; k++) {
      y[k].push_back(fmin(fmax(t[k], -CLIP), CLIP));
      q[k].push_back(fmin(fmax(v[k], -CLIP), CLIP));
    }
  }
}

int main(int argc, char** argv) {
  int a = 1;
  for (; a < argc && argv[a][0] == '-' && argv[a][1]; a++) {
    if (argv[a][1] == 'd') decimals = atoi(argv[a] + 2);
    else die("unknown option %s", argv[a]);
  }
  if (argc - a < 2 || (argc - a) % 2 || decimals < 0 || decimals > 20) {
    fprintf(stderr,
      "usage: metric [-dN] data.tsv pred.tsv [data2.tsv pred2.tsv ...]\n"
      "Global Weighted Pearson over need_prediction&is_scored rows of all pairs.\n"
      "  -dN  print N decimals (default 9)\n");
    return 1;
  }
  Score total;
  int npairs = (argc - a) / 2;
  for (int i = a; i < argc; i += 2) {
    Score s;
    score_pair(argv[i], argv[i + 1], s);
    if (npairs > 1) {
      const char* name = argv[i];
      for (const char* q = name; *q; q++) if (*q == '/' || *q == '\\') name = q + 1;
      s.print(name);
    }
    total.merge(s);
  }
  if (total.scored == 0) die("no rows selected by the scoring mask");
  total.print("TOTAL");
  return 0;
}
