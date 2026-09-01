// model_opt.h — TransformerOpt: the optimized fx2 transformer (SPEC section 6
// interface) assembled from the four kernel workstreams:
//   qmat  (src/opt/qmat*.{h,cpp})  — all int4x int8 matmuls, arena format
//   attn  (src/opt/attn.{h,cpp})   — vanilla sliding-window attention + KV
//   kda   (src/opt/kda.{h,cpp})    — fused Kimi linear-attention step
//   glue  (src/opt/glue.{h,cpp})   — norms/quants/rope/residual/head
// Numerics: bit-exact vs the naive src/model.cpp except the documented
// vector-math ulp divergences (attention softmax/PV reassociation ~1e-7 rel,
// KDA exp-family <= ~2 ulp/elem, head tanh/exp <= ~2 ulp) — expected
// whole-model probability drift <= ~1e-5 absolute.
// The vanilla-attention low-prob skip is hardwired OFF (exact softmax; at the
// SPEC-safe threshold 21 it never fires anyway).
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

namespace fx2 {
namespace opt {

// KV-cache variant (attn.h): outputs are bitwise identical; F32V is fastest,
// I8V saves 1.7 MB of L3 footprint (0.56 vs 2.81 MB KV total).
enum class AttnKind : int { KVF32 = 0, KVI8 = 1 };

struct TransformerOptImpl;

struct TransformerOpt {
  // weights_path == nullptr initializes the weights in memory from init_seed
  // instead of reading a file (weights_init.cpp) - deterministic, so an
  // encoder and a decoder passing the same seed get identical weights.
  // rope_rows caps the recomputed rope sin/cos tables (0 = the full 131072
  // positions, 32 MB).  Positions beyond the cap fall back to libm sin/cos,
  // which is deterministic and therefore still symmetric between an encoder
  // and a decoder running the same binary.
  explicit TransformerOpt(const char* weights_path,
                          AttnKind attn = AttnKind::KVF32,
                          size_t rope_rows = 0, uint64_t init_seed = 0);
  ~TransformerOpt();
  TransformerOpt(const TransformerOpt&) = delete;
  TransformerOpt& operator=(const TransformerOpt&) = delete;

  // resets KV rings, KDA states and conv histories; rope positions for the
  // article are rope_position_offset + local position (0-based)
  void begin_article(int64_t rope_position_offset = 0);

  // feed input token and its prior (205 float16 values); fills probs_out[205]
  // with the fp32 distribution over the NEXT token
  void step(uint8_t token, const uint16_t* prior_f16, float* probs_out);
  // same with an fp32 prior row (205 values)
  void step(uint8_t token, const float* prior205, float* probs_out);

  // the 205 post-softcap logits of the last step (valid until the next step)
  const float* last_logits() const;
  // the final rms_norm output of the last step (192 fp32) - the activation the
  // unembedding consumes, i.e. what a caller needs to run or train its own
  // output layer.  Valid until the next step.
  const float* last_final_norm() const;
  // the dequantized unembedding rows (205 x 192, row-major): weight q times
  // its per-row scale, without the activation scale.  Stable for the model's
  // lifetime; a caller training its own head starts from these.
  const float* unembed_rows() const;

  AttnKind attn_kind() const;

 private:
  std::unique_ptr<TransformerOptImpl> impl;
};

// ---- profiling hooks (functional only when model_opt.cpp is compiled with
// -DFX2_PROF; no-ops otherwise). Accumulators are raw-rdtsc ticks. ----
void prof_reset();
bool prof_enabled();
// Print the per-group / per-site table. tsc_to_core = core cycles per tsc
// tick (bench_common.h Calib::ratio()); tokens = steps accumulated since
// prof_reset. Overhead-corrects each section by the measured median
// back-to-back raw-rdtsc delta.
void prof_print(double tsc_to_core, long tokens);

}  // namespace opt
}  // namespace fx2
