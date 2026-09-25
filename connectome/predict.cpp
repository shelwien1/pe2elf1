// predict.cpp - causal predictor template: data.tsv -> pred.tsv
//
// usage: predict [-m mode] data.tsv pred.tsv
//   -m0 (default) dummy, features only: t0 = i0_p3, t1 = -i0_p20
//       (the best single features on other validation sequences, WP ~0.39)
//   -m1 "same as previous": t0,t1 of the previous row. This is a LEAK:
//       the platform never gives targets to the model; only shows how
//       smooth the targets are (lag-1 autocorrelation ~0.94).
//
// Rows are fed one at a time in file order, like the platform callback:
// state is reset when seq_ix changes and updated on every row (warm-up too);
// a prediction is written for every need_prediction row.
// Output columns: seq_ix step_in_seq t0 t1
#include "tsv.h"

struct Model {
  int mode = 0;
  float prev[2] = {0, 0}; // mode 1: targets of the previous row

  void reset() { prev[0] = prev[1] = 0; }

  // x = 112 features of the current row, in the official order
  // (x[0..10] i0_p0..10, x[11..21] i0_p11..21, x[22..43] i0_v*, ... x[104..111] a0..a7)
  void predict(const float* x, float* p) {
    if (mode == 1) { p[0] = prev[0]; p[1] = prev[1]; return; }
    p[0] = x[3];   // i0_p3
    p[1] = -x[20]; // i0_p20
  }

  // mode 1 only: targets become "known" after the row's prediction
  void observe_targets(const float* t) { prev[0] = t[0]; prev[1] = t[1]; }
};

int main(int argc, char** argv) {
  Model m;
  int a = 1;
  for (; a < argc && argv[a][0] == '-' && argv[a][1]; a++) {
    if (argv[a][1] == 'm') m.mode = atoi(argv[a] + 2);
    else die("unknown option %s", argv[a]);
  }
  if (argc - a != 2 || m.mode < 0 || m.mode > 1) {
    fprintf(stderr, "usage: predict [-m0|-m1] data.tsv pred.tsv\n"
                    "  -m0 dummy predictor (features only)\n"
                    "  -m1 previous row targets (leak, diagnostic only)\n");
    return 1;
  }
  const char* in = argv[a];
  const char* out = argv[a + 1];
  if (m.mode == 1) fprintf(stderr, "warning: -m1 uses targets, which are never available at test time\n");

  TSV d; d.load(in);
  int c_seq = d.need("seq_ix"), c_step = d.need("step_in_seq"), c_need = d.need("need_prediction");
  std::vector<std::string> fn = feature_names();
  int fc[N_FEATURES];
  for (int j = 0; j < N_FEATURES; j++) fc[j] = d.need(fn[j].c_str());
  int c_t[2] = {-1, -1};
  if (m.mode == 1) { c_t[0] = d.need("t0"); c_t[1] = d.need("t1"); }

  FILE* f = fopen(out, "wb"); if (!f) die("can't create %s", out);
  fprintf(f, "seq_ix\tstep_in_seq\tt0\tt1\n");
  float x[N_FEATURES], p[2];
  long long cur = 0, nout = 0;
  for (size_t r = 0; r < d.nrows; r++) {
    long long seq = d.i(r, c_seq);
    if (r == 0 || seq != cur) { m.reset(); cur = seq; }
    for (int j = 0; j < N_FEATURES; j++) x[j] = d.f(r, fc[j]);
    m.predict(x, p);
    if (d.b(r, c_need)) {
      char s0[32], s1[32];
      fmt_float(s0, p[0]); fmt_float(s1, p[1]);
      fprintf(f, "%lld\t%lld\t%s\t%s\n", seq, d.i(r, c_step), s0, s1);
      nout++;
    }
    if (m.mode == 1) { float t[2] = {d.f(r, c_t[0]), d.f(r, c_t[1])}; m.observe_targets(t); }
  }
  fclose(f);
  fprintf(stderr, "%s: %zu rows, %lld predictions -> %s\n", in, d.nrows, nout, out);
  return 0;
}
