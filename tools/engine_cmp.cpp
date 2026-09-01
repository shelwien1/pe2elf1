// engine_cmp: checks the fp32 engine against the quantized one, and checks
// that the fp32 weight block really is the whole model.
//
//   build: clang++ -O3 -std=c++17 -march=haswell -I tf tools/engine_cmp.cpp \
//            <tf objects> -o engine_cmp
//   run:   ./engine_cmp models/6m-q4-fp32.tfwc2
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "fp32_model.h"
#include "model_opt.h"

int main(int argc, char** argv) {
  const char* path = argc > 1 ? argv[1] : "models/6m-q4-fp32.tfwc2";
  const int kSteps = 400, V = 205;

  // a repeatable pseudo-text token stream and a peaked prior on each token,
  // which is the shape the model was trained to refine
  std::vector<uint8_t> toks(kSteps);
  uint64_t s = 99;
  for (int i = 0; i < kSteps; i++) {
    s = s * 6364136223846793005ull + 1442695040888963407ull;
    toks[i] = uint8_t(8 + (s >> 33) % 70);
  }
  auto prior_for = [&](int i, float* p) {
    for (int j = 0; j < V; j++) p[j] = 1e-4f;
    p[toks[i]] = 0.5f;                       // ppmd usually likes the truth
    p[(toks[i] + 1) % V] = 0.3f;
    float sum = 0;
    for (int j = 0; j < V; j++) sum += p[j];
    for (int j = 0; j < V; j++) p[j] /= sum;
  };

  fx2::f32::Transformer32 f32(path, kSteps + 1, 0);
  fx2::opt::TransformerOpt q(path, fx2::opt::AttnKind::KVI8, kSteps + 1, 0);
  std::printf("fp32 parameters: %zu floats (%.2f MB)\n", f32.weight_count(),
              f32.weight_count() * 4.0 / (1 << 20));

  std::vector<float> pf(V), pq(V), pr(V);
  double max_abs = 0, sum_abs = 0, nll_f = 0, nll_q = 0;
  f32.begin_article(0);
  q.begin_article(0);
  for (int i = 0; i + 1 < kSteps; i++) {
    prior_for(i, pr.data());
    f32.step(toks[i], pr.data(), pf.data());
    q.step(toks[i], pr.data(), pq.data());
    for (int j = 0; j < V; j++) {
      const double d = std::fabs(double(pf[j]) - double(pq[j]));
      if (d > max_abs) max_abs = d;
      sum_abs += d;
    }
    nll_f -= std::log(std::max(double(pf[toks[i + 1]]), 1e-30));
    nll_q -= std::log(std::max(double(pq[toks[i + 1]]), 1e-30));
  }
  const int n = kSteps - 1;
  std::printf("fp32 vs quantized: max |dp| %.3e, mean |dp| %.3e per element\n",
              max_abs, sum_abs / (double(n) * V));
  std::printf("mean -log p: fp32 %.6f, quantized %.6f (delta %+.6f)\n",
              nll_f / n, nll_q / n, (nll_f - nll_q) / n);

  // the weight block is the entire model: save it, corrupt it, restore it,
  // and the same input must give bit-identical output again
  std::vector<float> backup(f32.weight_count());
  std::memcpy(backup.data(), f32.weights(), backup.size() * sizeof(float));

  f32.begin_article(0);
  std::vector<float> ref(V);
  prior_for(0, pr.data());
  f32.step(toks[0], pr.data(), ref.data());

  for (size_t i = 0; i < f32.weight_count(); i++) f32.weights()[i] *= 1.03f;
  f32.begin_article(0);
  prior_for(0, pr.data());
  f32.step(toks[0], pr.data(), pf.data());
  double moved = 0;
  for (int j = 0; j < V; j++) moved += std::fabs(pf[j] - ref[j]);
  if (moved < 1e-6) {
    std::printf("FAIL: perturbing the weight block did not change the output\n");
    return 1;
  }

  std::memcpy(f32.weights(), backup.data(), backup.size() * sizeof(float));
  f32.begin_article(0);
  prior_for(0, pr.data());
  f32.step(toks[0], pr.data(), pf.data());
  int bad = 0;
  for (int j = 0; j < V; j++)
    if (std::memcmp(&pf[j], &ref[j], 4) != 0) bad++;
  std::printf("weight backup/restore: perturbation moved output by %.4f L1, "
              "restore gave %s\n", moved,
              bad ? "a DIFFERENT result" : "a bit-identical result");
  return bad ? 1 : 0;
}
