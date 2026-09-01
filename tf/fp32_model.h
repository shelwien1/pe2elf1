// fp32_model: the same transformer as model_opt, but running entirely in fp32.
//
// The weights file stores int4 weights with per-row scales and a static scale
// per activation quantizer.  This loader dequantizes every weight ONCE, at
// load time, into plain fp32 (w = q * row_scale) and then never quantizes
// anything again: there are no activation quantization steps in the forward
// pass, and no int arithmetic.  Compared with the quantized engine that costs
// throughput (the weight stream is 23.6 MB per token instead of 2.9 MB), and
// buys two things: the arithmetic is ordinary floating point, and every
// parameter of the model is a plain float in one contiguous block, so it can
// be read, written, backed up and restored with a memcpy - which is what a
// backward pass through the body needs.
//
// The forward pass follows cpp_infer/SPEC.md and KIMI_SEMANTICS.md, with the
// fake-quantization removed:
//   * a quantized matmul y[o] = s_act*s_w[o]*<q(x), q(w[o])> becomes the plain
//     dot product y[o] = <x, w[o]> on the dequantized row;
//   * vanilla attention keeps its KV cache in fp32 and scores with
//     0.125*<q_h, k_h> (the sq*sk factor existed only to undo int scaling);
//   * everything else - rms_norm, the causal conv + SiLU, the KDA recurrence,
//     the gated norm, RoPE, the logit softcap and the softmax - was already
//     fp32 and is unchanged.
#pragma once

namespace fx2 {
namespace f32 {

struct ModelImpl;

struct Transformer32 {
  // weights_path == nullptr initializes the weights in memory from init_seed
  // (weights_init.cpp), exactly as the quantized engine does.
  explicit Transformer32(const char* weights_path, size_t rope_rows = 0,
                         uint64_t init_seed = 0);
  ~Transformer32();
  Transformer32(const Transformer32&) = delete;
  Transformer32& operator=(const Transformer32&) = delete;

  // resets the KV rings, the KDA states and the conv histories
  void begin_article(int64_t rope_position_offset = 0);
  // feed a token and its 205-element prior, get the distribution over the
  // next token
  void step(uint8_t token, const float* prior205, float* probs_out);

  const float* last_logits() const;      // 205, post-softcap
  const float* last_final_norm() const;  // 192, the unembedding's input
  const float* unembed_rows() const;     // 205 x 192, row-major

  // Every parameter of the model, contiguous.  Copy it to back the model up,
  // copy it back to restore - there is nothing else to save, no packed arena
  // to rebuild and no scales to recompute.
  float* weights();
  const float* weights() const;
  size_t weight_count() const;

  // ---- training ----------------------------------------------------------
  // Allocates the gradient buffer, the optimizer moments and the forward tape,
  // and makes step() record what backward() needs.  Roughly 4x the weight
  // block in memory (gradients + two AdamW moments).
  void enable_training();
  bool training() const;

  // Backpropagates dL/d(capped logits) - 205 values, the derivative of the
  // caller's loss with respect to the post-softcap logits of the LAST step -
  // through the whole model, accumulating into the gradient buffer.  Every
  // parameter gets a gradient: the twelve blocks, both attention kinds, the
  // convolutions, the gates, the embeddings.
  //
  // The recurrent state is truncated at one step: the KDA state and the
  // attention KV ring as they stood BEFORE this token are treated as
  // constants, so what is computed is the exact gradient of this token's loss
  // with respect to this token's computation.  Contributions that would flow
  // back into earlier tokens' forward passes are dropped, which is the same
  // trade the LSTM made with its finite BPTT horizon, at horizon 1.
  void backward(const float* dcap205);

  // AdamW over every parameter, then zeroes the gradients.
  void adam_step(float lr, float weight_decay, long t);

  // The recurrent state - KDA states, conv histories, the attention KV rings,
  // the skip store and the position counter.  Save it, run a step, restore it
  // and the same step runs again identically; that is what makes the truncated
  // gradient checkable against a finite difference, and it is also what a
  // checkpoint of a running context would need.
  size_t state_bytes() const;
  void save_state(void* dst) const;
  void load_state(const void* src);
  void zero_grads();
  const float* grads() const;
  float* grads();

 private:
  std::unique_ptr<ModelImpl> impl;
};

}  // namespace f32
}  // namespace fx2
