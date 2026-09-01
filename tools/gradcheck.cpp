// gradcheck: checks the analytic backward pass of the trainable stack in
// transformer.inc (the extra block and the output layer) against central
// finite differences of the loss it is supposed to be the gradient of.
//
//   build:  clang++ -O2 -std=c++17 -march=haswell -I. -I tf \
//             tools/gradcheck.cpp -o gradcheck -lm
//   run:    ./gradcheck
//
// It drives the real code: TF_GRADCHECK makes TF_AdamW::Row record the
// gradients the backward pass produces instead of applying them, so the
// weights stay fixed and both sides are evaluated at the same point.
#include "tool_prelude.inc"

typedef uint32_t uint;
typedef uint64_t qword;
typedef float* __restrict prfloat;
typedef const float* __restrict cprfloat;
#define ALIGN(n) __attribute__((aligned(n)))
#define NOINLINE __attribute__((noinline))

static inline float DotProduct(cprfloat a, cprfloat b, uint n) {
  float s = 0;
  for (uint i = 0; i < n; i++) s += a[i] * b[i];
  return s;
}
static inline float SumOfAbs(cprfloat a, uint n) {
  float s = 0;
  for (uint i = 0; i < n; i++) s += std::fabs(a[i]);
  return s;
}
// UnifiedModel is never instantiated here, but it is not a template on this
// type, so the members it names still have to exist.
struct ppmd_Model {
  uint sqp[256];
  void Init(int, int, int, uint) {}
  void ppmd_UpdateByte(uint) {}
  void ppmd_PrepareByte() {}
};

#define TF_TRAIN 2
#define TF_GRADCHECK 1
#include "transformer.inc"

namespace {

uint64_t st = 12345;
float urand() {
  uint64_t z = (st += 0x9E3779B97F4A7C15ull);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
  z ^= z >> 31;
  return float(z >> 40) * (1.0f / 16777216.0f) * 2.0f - 1.0f;
}

Transformer T;
const int kTokens = 12, kTarget = 5;

double loss() {
  T.StackForward();
  return -std::log(double(T.out_[T.tok_list_[kTarget]]) + 1e-300);
}

}  // namespace

int main() {
  T.n_tokens_ = kTokens;
  for (int i = 0; i < kTokens; i++) T.tok_list_[i] = i * 7 + 3;  // scattered ids

  for (uint i = 0; i < Transformer::TF_D; i++) T.prev_xn_[i] = urand();
  for (uint i = 0; i < Transformer::TF_V * Transformer::TF_D; i++)
    T.head_w_[i] = 0.1f * urand();
  for (uint i = 0; i < TF_ADAPTER_D * Transformer::TF_D; i++)
    T.up_w_[i] = 0.3f * urand();
  // a non-zero down projection, or the up gradient would be zero by
  // construction and the test would pass without checking anything
  for (uint i = 0; i < Transformer::TF_D * TF_ADAPTER_D; i++)
    T.dn_w_[i] = 0.2f * urand();

  // analytic gradients, in the order TF_AdamW::Row sees them:
  //   head rows (n_tokens x D), then down rows (D x A), then up rows (A x D)
  const uint n_head = uint(kTokens) * Transformer::TF_D;
  const uint n_down = Transformer::TF_D * TF_ADAPTER_D;
  const uint n_up = TF_ADAPTER_D * Transformer::TF_D;
  static float g[1 << 22];
  loss();
  tf_gc_capture = g;
  tf_gc_n = 0;
  T.StackUpdate(T.tok_list_[kTarget]);
  tf_gc_capture = 0;
  if (tf_gc_n != n_head + n_down + n_up) {
    std::printf("FAIL: captured %u gradients, expected %u\n", tf_gc_n,
                n_head + n_down + n_up);
    return 1;
  }

  struct Site {
    const char* name;
    float* w;
    uint base, rows, row_len;
    bool head;  // the head's rows are indexed by token id, not by list position
  } sites[3] = {
      {"head", T.head_w_, 0, uint(kTokens), Transformer::TF_D, true},
      {"mlp.down", T.dn_w_, n_head, Transformer::TF_D, TF_ADAPTER_D, false},
      {"mlp.up", T.up_w_, n_head + n_down, TF_ADAPTER_D, Transformer::TF_D,
       false},
  };

  // Directional derivative: perturb the whole site along a random direction v
  // and compare (L(w+hv) - L(w-hv)) / 2h with dot(grad, v).  Per-element finite
  // differences drown in float noise here - the forward pass is fp32, so a
  // single weight's contribution to the loss is near the precision floor -
  // whereas a direction that touches every weight of the site at once has a
  // signal thousands of times larger than the noise.
  static float v[1 << 22];
  int bad = 0;
  for (const Site& s : sites) {
    const uint n = s.rows * s.row_len;
    for (uint i = 0; i < n; i++) v[i] = urand();

    double best_rel = 1e30;
    double shown_num = 0, shown_ana = 0, shown_h = 0;
    for (float h : {1e-2f, 3e-3f, 1e-3f}) {
      double lp, lm;
      for (int sign = 0; sign < 2; sign++) {
        const float d = sign ? -h : h;
        for (uint r = 0; r < s.rows; r++) {
          const uint wr = s.head ? uint(T.tok_list_[r]) : r;
          for (uint c = 0; c < s.row_len; c++)
            s.w[wr * s.row_len + c] += d * v[r * s.row_len + c];
        }
        (sign ? lm : lp) = loss();
        for (uint r = 0; r < s.rows; r++) {  // restore exactly
          const uint wr = s.head ? uint(T.tok_list_[r]) : r;
          for (uint c = 0; c < s.row_len; c++)
            s.w[wr * s.row_len + c] -= d * v[r * s.row_len + c];
        }
      }
      const double num = (lp - lm) / (2.0 * h);
      double ana = 0;
      for (uint i = 0; i < n; i++) ana += double(g[s.base + i]) * v[i];
      const double rel =
          std::fabs(num - ana) / (std::fabs(num) + std::fabs(ana) + 1e-12);
      if (rel < best_rel) {
        best_rel = rel;
        shown_num = num;
        shown_ana = ana;
        shown_h = h;
      }
    }
    const bool ok = best_rel < 1e-3;
    if (!ok) bad++;
    std::printf("%-9s directional: numeric %+.6f  analytic %+.6f  "
                "rel %.2e (h=%.0e)  %s\n",
                s.name, shown_num, shown_ana, best_rel, shown_h,
                ok ? "ok" : "FAIL");
  }

  std::printf("%s\n", bad ? "GRADCHECK FAILED" : "gradcheck passed");
  return bad ? 1 : 0;
}
