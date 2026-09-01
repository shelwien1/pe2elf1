// gradcheck_full: checks the backward pass of the WHOLE fp32 model against
// finite differences, group by group.
//
// The gradient backward() computes is truncated at one step: the KDA state and
// the attention KV ring as they stood before this token are constants.  So the
// finite difference has to be taken with the same semantics - save the
// recurrent state, perturb the weights, restore the state, and re-run only the
// step being differentiated.  save_state/load_state make that exact.
//
// Per-element differences are useless in fp32 (one weight's effect on the loss
// sits at the precision floor), so each group is checked along a random
// direction: (L(w+hv) - L(w-hv)) / 2h against <grad, v>.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "fp32_model.h"

namespace {
constexpr int V = 205;
uint64_t rs = 7;
float urand() {
  uint64_t z = (rs += 0x9E3779B97F4A7C15ull);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
  z ^= z >> 31;
  return float(z >> 40) * (1.0f / 16777216.0f) * 2.0f - 1.0f;
}
}  // namespace

int main(int argc, char** argv) {
  const char* path = argc > 1 ? argv[1] : nullptr;  // nullptr = fresh init
  const int WARM = argc > 2 ? std::atoi(argv[2]) : 6;
  const int TARGET = 41;

  fx2::f32::Transformer32 m(path, 4096, 12345);
  m.enable_training();
  const size_t NW = m.weight_count();
  std::printf("parameters: %zu, warmup steps: %d\n", NW, WARM);

  std::vector<float> prior(V), probs(V);
  auto make_prior = [&](int i) {
    for (int j = 0; j < V; j++) prior[j] = 1e-4f;
    prior[(i * 13 + 9) % V] = 0.6f;
    float s = 0;
    for (int j = 0; j < V; j++) s += prior[j];
    for (int j = 0; j < V; j++) prior[j] /= s;
  };
  auto tok = [&](int i) { return uint8_t((i * 17 + 5) % V); };

  // warm the recurrent state up, then snapshot it
  m.begin_article(0);
  for (int i = 0; i < WARM; i++) {
    make_prior(i);
    m.step(tok(i), prior.data(), probs.data());
  }
  std::vector<uint8_t> state(m.state_bytes());
  m.save_state(state.data());

  // the step under test; loss = -log softmax(capped logits)[TARGET]
  auto run_step = [&]() -> double {
    make_prior(WARM);
    m.step(tok(WARM), prior.data(), probs.data());
    return -std::log(std::max(double(probs[TARGET]), 1e-300));
  };
  auto restore = [&]() { m.load_state(state.data()); };

  restore();
  const double L0 = run_step();
  std::vector<float> dcap(V);
  for (int i = 0; i < V; i++) dcap[i] = probs[i] - (i == TARGET ? 1.0f : 0.0f);
  m.zero_grads();
  m.backward(dcap.data());
  std::printf("loss at the checked step: %.6f\n", L0);

  // the weight block is laid out in load order, so contiguous ranges are
  // meaningful groups; check the whole block plus a few slices of it
  struct Grp { const char* name; size_t off, n; };
  std::vector<Grp> groups = {
      {"whole model", 0, NW},
      {"embedding+prior+unembed", 0, size_t(205 * 192) * 3},
      {"first kimi block", size_t(205 * 192) * 3 + 6 + 24, 199747},
      {"a vanilla block", NW - 294912 - 147456, 147456},
      {"last mlp", NW - 294912, 294912},
  };

  std::vector<float> w0(NW), v(NW);
  std::memcpy(w0.data(), m.weights(), NW * sizeof(float));

  int bad = 0;
  for (const Grp& gp : groups) {
    if (gp.off + gp.n > NW) continue;
    for (size_t i = 0; i < NW; i++) v[i] = 0.0f;
    for (size_t i = gp.off; i < gp.off + gp.n; i++) v[i] = urand();

    double ana = 0;
    for (size_t i = 0; i < NW; i++) ana += double(m.grads()[i]) * v[i];

    double best = 1e30, shown_n = 0, shown_h = 0;
    for (float h : {3e-3f, 1e-3f, 3e-4f}) {
      double lp, lm;
      for (int sgn = 0; sgn < 2; sgn++) {
        const float d = sgn ? -h : h;
        for (size_t i = 0; i < NW; i++) m.weights()[i] = w0[i] + d * v[i];
        restore();
        (sgn ? lm : lp) = run_step();
      }
      std::memcpy(m.weights(), w0.data(), NW * sizeof(float));
      const double num = (lp - lm) / (2.0 * h);
      const double rel =
          std::fabs(num - ana) / (std::fabs(num) + std::fabs(ana) + 1e-12);
      if (rel < best) {
        best = rel;
        shown_n = num;
        shown_h = h;
      }
    }
    const bool ok = best < 2e-3;
    if (!ok) bad++;
    std::printf("%-26s numeric %+.6f  analytic %+.6f  rel %.2e (h=%.0e)  %s\n",
                gp.name, shown_n, ana, best, shown_h, ok ? "ok" : "FAIL");
  }
  std::printf("%s\n", bad ? "FULL GRADCHECK FAILED" : "full gradcheck passed");
  return bad ? 1 : 0;
}
