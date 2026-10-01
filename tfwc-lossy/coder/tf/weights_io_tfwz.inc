// weights_io_tfwz: decoder for TFWZ weights files (tfwc-lossy/tfwz/tfwz.cpp),
// so the coder can load them directly, like the FX2TFWC* files.
//
// TFWZ holds values only: the tensor set, order, dtypes and shapes are the 6m
// model's (205 tokens, d_model 192, 12 layers) and are built in below
// (kTfwzSchema, generated from 6m-q4-fp32.tfwc2).  Other architectures
// (cmix-lex-transformer's checkpoints) have no TFWZ form; keep them as tfwc2/3.
//
// The stream is one carryless binary arithmetic coder with KT-estimator bit
// models:
//   int4 tensors  optional noise rows (seed + 15-bin histogram, splitmix64),
//                 optional per-row classes (3-bit tree), then 4-bit trees over
//                 q+7, one model per row class
//   bf16 arrays   scalars: sign/exponent/mantissa trees; row scales: the
//                 median, a common mantissa shift, then Elias-gamma deltas in
//                 the ordered (sign, exponent, mantissa) domain
//   f32 / i32     a bf16-exact flag, then sign/exponent/mantissa trees (column
//                 context for [n][4]) or raw 32-bit words
//   rope.sin/cos  no payload: recomputed from rope.inv_freq, as for FX2TFWC2
// This must stay in exact sync with tfwz.cpp's encoder.

#include "weights_io.h"

namespace fx2 {

WeightsFile load_tfwz(const uint8_t* buf, size_t size, const char* path);

namespace tfwz_detail {

struct SchemaEntry {
  const char* name;
  uint8_t dtype;
  int ndim;
  uint32_t dims[2];
};
// schema.inc - the tensor set of the 6m model, in the order of the shipped
// 6m-q4-fp32 FX2TFWC2 file (generated from it by tools/mkschema.py)
// name, dtype, ndim, dims
static const SchemaEntry kTfwzSchema[] = {
  {"embedding.weight.q", DT_I8, 2, {205, 192}},
  {"embedding.weight.scale", DT_BF16, 1, {205, 0}},
  {"prior_embedding.weight.q", DT_I8, 2, {192, 205}},
  {"prior_embedding.weight.scale", DT_BF16, 1, {192, 0}},
  {"prior_embedding.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"skip_connection_weights.value", DT_F32, 1, {6, 0}},
  {"blocks.0.residual_stream_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.0.token_embedding_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.0.attention.log_baseline_decay_rate", DT_F32, 1, {3, 0}},
  {"blocks.0.attention.dt_bias", DT_F32, 1, {192, 0}},
  {"blocks.0.attention.query_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.0.attention.query_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.0.attention.query_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.0.attention.query_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.0.attention.key_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.0.attention.key_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.0.attention.key_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.0.attention.key_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.0.attention.value_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.0.attention.value_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.0.attention.value_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.0.attention.value_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.0.attention.forget_gate_projection.up.weight.q", DT_I8, 2, {64, 192}},
  {"blocks.0.attention.forget_gate_projection.up.weight.scale", DT_BF16, 1, {64, 0}},
  {"blocks.0.attention.forget_gate_projection.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.0.attention.forget_gate_projection.down.weight.q", DT_I8, 2, {192, 64}},
  {"blocks.0.attention.forget_gate_projection.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.0.attention.forget_gate_projection.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.0.attention.beta_projection.weight", DT_F32, 2, {3, 192}},
  {"blocks.0.attention.output_gate_projection.up.weight.q", DT_I8, 2, {64, 192}},
  {"blocks.0.attention.output_gate_projection.up.weight.scale", DT_BF16, 1, {64, 0}},
  {"blocks.0.attention.output_gate_projection.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.0.attention.output_gate_projection.down.weight.q", DT_I8, 2, {192, 64}},
  {"blocks.0.attention.output_gate_projection.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.0.attention.output_gate_projection.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.0.attention.output_fused_norm_gate.weight", DT_F32, 1, {64, 0}},
  {"blocks.0.attention.output_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.0.attention.output_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.0.attention.output_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.0.mlp.up.weight.q", DT_I8, 2, {768, 192}},
  {"blocks.0.mlp.up.weight.scale", DT_BF16, 1, {768, 0}},
  {"blocks.0.mlp.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.0.mlp.down.weight.q", DT_I8, 2, {192, 768}},
  {"blocks.0.mlp.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.0.mlp.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.1.residual_stream_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.1.token_embedding_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.1.attention.log_baseline_decay_rate", DT_F32, 1, {3, 0}},
  {"blocks.1.attention.dt_bias", DT_F32, 1, {192, 0}},
  {"blocks.1.attention.query_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.1.attention.query_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.1.attention.query_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.1.attention.query_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.1.attention.key_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.1.attention.key_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.1.attention.key_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.1.attention.key_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.1.attention.value_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.1.attention.value_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.1.attention.value_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.1.attention.value_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.1.attention.forget_gate_projection.up.weight.q", DT_I8, 2, {64, 192}},
  {"blocks.1.attention.forget_gate_projection.up.weight.scale", DT_BF16, 1, {64, 0}},
  {"blocks.1.attention.forget_gate_projection.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.1.attention.forget_gate_projection.down.weight.q", DT_I8, 2, {192, 64}},
  {"blocks.1.attention.forget_gate_projection.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.1.attention.forget_gate_projection.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.1.attention.beta_projection.weight", DT_F32, 2, {3, 192}},
  {"blocks.1.attention.output_gate_projection.up.weight.q", DT_I8, 2, {64, 192}},
  {"blocks.1.attention.output_gate_projection.up.weight.scale", DT_BF16, 1, {64, 0}},
  {"blocks.1.attention.output_gate_projection.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.1.attention.output_gate_projection.down.weight.q", DT_I8, 2, {192, 64}},
  {"blocks.1.attention.output_gate_projection.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.1.attention.output_gate_projection.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.1.attention.output_fused_norm_gate.weight", DT_F32, 1, {64, 0}},
  {"blocks.1.attention.output_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.1.attention.output_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.1.attention.output_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.1.mlp.up.weight.q", DT_I8, 2, {768, 192}},
  {"blocks.1.mlp.up.weight.scale", DT_BF16, 1, {768, 0}},
  {"blocks.1.mlp.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.1.mlp.down.weight.q", DT_I8, 2, {192, 768}},
  {"blocks.1.mlp.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.1.mlp.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.2.residual_stream_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.2.token_embedding_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.2.attention.log_baseline_decay_rate", DT_F32, 1, {3, 0}},
  {"blocks.2.attention.dt_bias", DT_F32, 1, {192, 0}},
  {"blocks.2.attention.query_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.2.attention.query_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.2.attention.query_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.2.attention.query_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.2.attention.key_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.2.attention.key_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.2.attention.key_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.2.attention.key_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.2.attention.value_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.2.attention.value_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.2.attention.value_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.2.attention.value_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.2.attention.forget_gate_projection.up.weight.q", DT_I8, 2, {64, 192}},
  {"blocks.2.attention.forget_gate_projection.up.weight.scale", DT_BF16, 1, {64, 0}},
  {"blocks.2.attention.forget_gate_projection.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.2.attention.forget_gate_projection.down.weight.q", DT_I8, 2, {192, 64}},
  {"blocks.2.attention.forget_gate_projection.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.2.attention.forget_gate_projection.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.2.attention.beta_projection.weight", DT_F32, 2, {3, 192}},
  {"blocks.2.attention.output_gate_projection.up.weight.q", DT_I8, 2, {64, 192}},
  {"blocks.2.attention.output_gate_projection.up.weight.scale", DT_BF16, 1, {64, 0}},
  {"blocks.2.attention.output_gate_projection.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.2.attention.output_gate_projection.down.weight.q", DT_I8, 2, {192, 64}},
  {"blocks.2.attention.output_gate_projection.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.2.attention.output_gate_projection.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.2.attention.output_fused_norm_gate.weight", DT_F32, 1, {64, 0}},
  {"blocks.2.attention.output_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.2.attention.output_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.2.attention.output_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.2.mlp.up.weight.q", DT_I8, 2, {768, 192}},
  {"blocks.2.mlp.up.weight.scale", DT_BF16, 1, {768, 0}},
  {"blocks.2.mlp.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.2.mlp.down.weight.q", DT_I8, 2, {192, 768}},
  {"blocks.2.mlp.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.2.mlp.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.3.residual_stream_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.3.token_embedding_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.3.attention.query_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.3.attention.query_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.3.attention.query_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.3.attention.key_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.3.attention.key_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.3.attention.key_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.3.attention.value_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.3.attention.value_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.3.attention.value_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.3.attention.quantize_queries.scale", DT_BF16, 1, {3, 0}},
  {"blocks.3.attention.quantize_keys.scale", DT_BF16, 1, {3, 0}},
  {"blocks.3.attention.quantize_values.scale", DT_BF16, 1, {3, 0}},
  {"blocks.3.attention.output_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.3.attention.output_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.3.attention.output_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.3.mlp.up.weight.q", DT_I8, 2, {768, 192}},
  {"blocks.3.mlp.up.weight.scale", DT_BF16, 1, {768, 0}},
  {"blocks.3.mlp.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.3.mlp.down.weight.q", DT_I8, 2, {192, 768}},
  {"blocks.3.mlp.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.3.mlp.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.4.residual_stream_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.4.token_embedding_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.4.attention.log_baseline_decay_rate", DT_F32, 1, {3, 0}},
  {"blocks.4.attention.dt_bias", DT_F32, 1, {192, 0}},
  {"blocks.4.attention.query_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.4.attention.query_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.4.attention.query_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.4.attention.query_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.4.attention.key_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.4.attention.key_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.4.attention.key_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.4.attention.key_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.4.attention.value_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.4.attention.value_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.4.attention.value_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.4.attention.value_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.4.attention.forget_gate_projection.up.weight.q", DT_I8, 2, {64, 192}},
  {"blocks.4.attention.forget_gate_projection.up.weight.scale", DT_BF16, 1, {64, 0}},
  {"blocks.4.attention.forget_gate_projection.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.4.attention.forget_gate_projection.down.weight.q", DT_I8, 2, {192, 64}},
  {"blocks.4.attention.forget_gate_projection.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.4.attention.forget_gate_projection.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.4.attention.beta_projection.weight", DT_F32, 2, {3, 192}},
  {"blocks.4.attention.output_gate_projection.up.weight.q", DT_I8, 2, {64, 192}},
  {"blocks.4.attention.output_gate_projection.up.weight.scale", DT_BF16, 1, {64, 0}},
  {"blocks.4.attention.output_gate_projection.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.4.attention.output_gate_projection.down.weight.q", DT_I8, 2, {192, 64}},
  {"blocks.4.attention.output_gate_projection.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.4.attention.output_gate_projection.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.4.attention.output_fused_norm_gate.weight", DT_F32, 1, {64, 0}},
  {"blocks.4.attention.output_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.4.attention.output_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.4.attention.output_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.4.mlp.up.weight.q", DT_I8, 2, {768, 192}},
  {"blocks.4.mlp.up.weight.scale", DT_BF16, 1, {768, 0}},
  {"blocks.4.mlp.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.4.mlp.down.weight.q", DT_I8, 2, {192, 768}},
  {"blocks.4.mlp.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.4.mlp.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.5.residual_stream_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.5.token_embedding_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.5.attention.log_baseline_decay_rate", DT_F32, 1, {3, 0}},
  {"blocks.5.attention.dt_bias", DT_F32, 1, {192, 0}},
  {"blocks.5.attention.query_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.5.attention.query_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.5.attention.query_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.5.attention.query_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.5.attention.key_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.5.attention.key_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.5.attention.key_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.5.attention.key_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.5.attention.value_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.5.attention.value_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.5.attention.value_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.5.attention.value_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.5.attention.forget_gate_projection.up.weight.q", DT_I8, 2, {64, 192}},
  {"blocks.5.attention.forget_gate_projection.up.weight.scale", DT_BF16, 1, {64, 0}},
  {"blocks.5.attention.forget_gate_projection.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.5.attention.forget_gate_projection.down.weight.q", DT_I8, 2, {192, 64}},
  {"blocks.5.attention.forget_gate_projection.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.5.attention.forget_gate_projection.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.5.attention.beta_projection.weight", DT_F32, 2, {3, 192}},
  {"blocks.5.attention.output_gate_projection.up.weight.q", DT_I8, 2, {64, 192}},
  {"blocks.5.attention.output_gate_projection.up.weight.scale", DT_BF16, 1, {64, 0}},
  {"blocks.5.attention.output_gate_projection.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.5.attention.output_gate_projection.down.weight.q", DT_I8, 2, {192, 64}},
  {"blocks.5.attention.output_gate_projection.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.5.attention.output_gate_projection.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.5.attention.output_fused_norm_gate.weight", DT_F32, 1, {64, 0}},
  {"blocks.5.attention.output_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.5.attention.output_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.5.attention.output_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.5.mlp.up.weight.q", DT_I8, 2, {768, 192}},
  {"blocks.5.mlp.up.weight.scale", DT_BF16, 1, {768, 0}},
  {"blocks.5.mlp.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.5.mlp.down.weight.q", DT_I8, 2, {192, 768}},
  {"blocks.5.mlp.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.5.mlp.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.6.residual_stream_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.6.token_embedding_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.6.attention.log_baseline_decay_rate", DT_F32, 1, {3, 0}},
  {"blocks.6.attention.dt_bias", DT_F32, 1, {192, 0}},
  {"blocks.6.attention.query_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.6.attention.query_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.6.attention.query_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.6.attention.query_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.6.attention.key_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.6.attention.key_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.6.attention.key_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.6.attention.key_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.6.attention.value_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.6.attention.value_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.6.attention.value_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.6.attention.value_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.6.attention.forget_gate_projection.up.weight.q", DT_I8, 2, {64, 192}},
  {"blocks.6.attention.forget_gate_projection.up.weight.scale", DT_BF16, 1, {64, 0}},
  {"blocks.6.attention.forget_gate_projection.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.6.attention.forget_gate_projection.down.weight.q", DT_I8, 2, {192, 64}},
  {"blocks.6.attention.forget_gate_projection.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.6.attention.forget_gate_projection.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.6.attention.beta_projection.weight", DT_F32, 2, {3, 192}},
  {"blocks.6.attention.output_gate_projection.up.weight.q", DT_I8, 2, {64, 192}},
  {"blocks.6.attention.output_gate_projection.up.weight.scale", DT_BF16, 1, {64, 0}},
  {"blocks.6.attention.output_gate_projection.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.6.attention.output_gate_projection.down.weight.q", DT_I8, 2, {192, 64}},
  {"blocks.6.attention.output_gate_projection.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.6.attention.output_gate_projection.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.6.attention.output_fused_norm_gate.weight", DT_F32, 1, {64, 0}},
  {"blocks.6.attention.output_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.6.attention.output_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.6.attention.output_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.6.mlp.up.weight.q", DT_I8, 2, {768, 192}},
  {"blocks.6.mlp.up.weight.scale", DT_BF16, 1, {768, 0}},
  {"blocks.6.mlp.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.6.mlp.down.weight.q", DT_I8, 2, {192, 768}},
  {"blocks.6.mlp.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.6.mlp.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.7.residual_stream_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.7.token_embedding_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.7.attention.query_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.7.attention.query_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.7.attention.query_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.7.attention.key_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.7.attention.key_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.7.attention.key_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.7.attention.value_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.7.attention.value_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.7.attention.value_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.7.attention.quantize_queries.scale", DT_BF16, 1, {3, 0}},
  {"blocks.7.attention.quantize_keys.scale", DT_BF16, 1, {3, 0}},
  {"blocks.7.attention.quantize_values.scale", DT_BF16, 1, {3, 0}},
  {"blocks.7.attention.output_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.7.attention.output_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.7.attention.output_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.7.mlp.up.weight.q", DT_I8, 2, {768, 192}},
  {"blocks.7.mlp.up.weight.scale", DT_BF16, 1, {768, 0}},
  {"blocks.7.mlp.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.7.mlp.down.weight.q", DT_I8, 2, {192, 768}},
  {"blocks.7.mlp.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.7.mlp.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.8.residual_stream_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.8.token_embedding_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.8.attention.log_baseline_decay_rate", DT_F32, 1, {3, 0}},
  {"blocks.8.attention.dt_bias", DT_F32, 1, {192, 0}},
  {"blocks.8.attention.query_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.8.attention.query_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.8.attention.query_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.8.attention.query_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.8.attention.key_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.8.attention.key_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.8.attention.key_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.8.attention.key_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.8.attention.value_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.8.attention.value_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.8.attention.value_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.8.attention.value_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.8.attention.forget_gate_projection.up.weight.q", DT_I8, 2, {64, 192}},
  {"blocks.8.attention.forget_gate_projection.up.weight.scale", DT_BF16, 1, {64, 0}},
  {"blocks.8.attention.forget_gate_projection.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.8.attention.forget_gate_projection.down.weight.q", DT_I8, 2, {192, 64}},
  {"blocks.8.attention.forget_gate_projection.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.8.attention.forget_gate_projection.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.8.attention.beta_projection.weight", DT_F32, 2, {3, 192}},
  {"blocks.8.attention.output_gate_projection.up.weight.q", DT_I8, 2, {64, 192}},
  {"blocks.8.attention.output_gate_projection.up.weight.scale", DT_BF16, 1, {64, 0}},
  {"blocks.8.attention.output_gate_projection.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.8.attention.output_gate_projection.down.weight.q", DT_I8, 2, {192, 64}},
  {"blocks.8.attention.output_gate_projection.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.8.attention.output_gate_projection.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.8.attention.output_fused_norm_gate.weight", DT_F32, 1, {64, 0}},
  {"blocks.8.attention.output_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.8.attention.output_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.8.attention.output_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.8.mlp.up.weight.q", DT_I8, 2, {768, 192}},
  {"blocks.8.mlp.up.weight.scale", DT_BF16, 1, {768, 0}},
  {"blocks.8.mlp.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.8.mlp.down.weight.q", DT_I8, 2, {192, 768}},
  {"blocks.8.mlp.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.8.mlp.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.9.residual_stream_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.9.token_embedding_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.9.attention.log_baseline_decay_rate", DT_F32, 1, {3, 0}},
  {"blocks.9.attention.dt_bias", DT_F32, 1, {192, 0}},
  {"blocks.9.attention.query_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.9.attention.query_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.9.attention.query_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.9.attention.query_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.9.attention.key_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.9.attention.key_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.9.attention.key_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.9.attention.key_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.9.attention.value_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.9.attention.value_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.9.attention.value_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.9.attention.value_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.9.attention.forget_gate_projection.up.weight.q", DT_I8, 2, {64, 192}},
  {"blocks.9.attention.forget_gate_projection.up.weight.scale", DT_BF16, 1, {64, 0}},
  {"blocks.9.attention.forget_gate_projection.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.9.attention.forget_gate_projection.down.weight.q", DT_I8, 2, {192, 64}},
  {"blocks.9.attention.forget_gate_projection.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.9.attention.forget_gate_projection.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.9.attention.beta_projection.weight", DT_F32, 2, {3, 192}},
  {"blocks.9.attention.output_gate_projection.up.weight.q", DT_I8, 2, {64, 192}},
  {"blocks.9.attention.output_gate_projection.up.weight.scale", DT_BF16, 1, {64, 0}},
  {"blocks.9.attention.output_gate_projection.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.9.attention.output_gate_projection.down.weight.q", DT_I8, 2, {192, 64}},
  {"blocks.9.attention.output_gate_projection.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.9.attention.output_gate_projection.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.9.attention.output_fused_norm_gate.weight", DT_F32, 1, {64, 0}},
  {"blocks.9.attention.output_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.9.attention.output_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.9.attention.output_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.9.mlp.up.weight.q", DT_I8, 2, {768, 192}},
  {"blocks.9.mlp.up.weight.scale", DT_BF16, 1, {768, 0}},
  {"blocks.9.mlp.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.9.mlp.down.weight.q", DT_I8, 2, {192, 768}},
  {"blocks.9.mlp.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.9.mlp.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.10.residual_stream_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.10.token_embedding_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.10.attention.log_baseline_decay_rate", DT_F32, 1, {3, 0}},
  {"blocks.10.attention.dt_bias", DT_F32, 1, {192, 0}},
  {"blocks.10.attention.query_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.10.attention.query_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.10.attention.query_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.10.attention.query_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.10.attention.key_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.10.attention.key_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.10.attention.key_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.10.attention.key_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.10.attention.value_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.10.attention.value_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.10.attention.value_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.10.attention.value_convolution.weight", DT_F32, 2, {192, 4}},
  {"blocks.10.attention.forget_gate_projection.up.weight.q", DT_I8, 2, {64, 192}},
  {"blocks.10.attention.forget_gate_projection.up.weight.scale", DT_BF16, 1, {64, 0}},
  {"blocks.10.attention.forget_gate_projection.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.10.attention.forget_gate_projection.down.weight.q", DT_I8, 2, {192, 64}},
  {"blocks.10.attention.forget_gate_projection.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.10.attention.forget_gate_projection.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.10.attention.beta_projection.weight", DT_F32, 2, {3, 192}},
  {"blocks.10.attention.output_gate_projection.up.weight.q", DT_I8, 2, {64, 192}},
  {"blocks.10.attention.output_gate_projection.up.weight.scale", DT_BF16, 1, {64, 0}},
  {"blocks.10.attention.output_gate_projection.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.10.attention.output_gate_projection.down.weight.q", DT_I8, 2, {192, 64}},
  {"blocks.10.attention.output_gate_projection.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.10.attention.output_gate_projection.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.10.attention.output_fused_norm_gate.weight", DT_F32, 1, {64, 0}},
  {"blocks.10.attention.output_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.10.attention.output_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.10.attention.output_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.10.mlp.up.weight.q", DT_I8, 2, {768, 192}},
  {"blocks.10.mlp.up.weight.scale", DT_BF16, 1, {768, 0}},
  {"blocks.10.mlp.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.10.mlp.down.weight.q", DT_I8, 2, {192, 768}},
  {"blocks.10.mlp.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.10.mlp.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.11.residual_stream_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.11.token_embedding_coefficient.value", DT_F32, 1, {1, 0}},
  {"blocks.11.attention.query_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.11.attention.query_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.11.attention.query_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.11.attention.key_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.11.attention.key_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.11.attention.key_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.11.attention.value_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.11.attention.value_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.11.attention.value_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.11.attention.quantize_queries.scale", DT_BF16, 1, {3, 0}},
  {"blocks.11.attention.quantize_keys.scale", DT_BF16, 1, {3, 0}},
  {"blocks.11.attention.quantize_values.scale", DT_BF16, 1, {3, 0}},
  {"blocks.11.attention.output_projection.weight.q", DT_I8, 2, {192, 192}},
  {"blocks.11.attention.output_projection.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.11.attention.output_projection.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.11.mlp.up.weight.q", DT_I8, 2, {768, 192}},
  {"blocks.11.mlp.up.weight.scale", DT_BF16, 1, {768, 0}},
  {"blocks.11.mlp.up.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"blocks.11.mlp.down.weight.q", DT_I8, 2, {192, 768}},
  {"blocks.11.mlp.down.weight.scale", DT_BF16, 1, {192, 0}},
  {"blocks.11.mlp.down.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"unembedding.weight.q", DT_I8, 2, {205, 192}},
  {"unembedding.weight.scale", DT_BF16, 1, {205, 0}},
  {"unembedding.quantize_activation.scale", DT_BF16, 1, {1, 0}},
  {"rope.inv_freq", DT_F32, 1, {32, 0}},
  {"rope.sin", DT_F32, 2, {131072, 32}},
  {"rope.cos", DT_F32, 2, {131072, 32}},
  {"config.ints", DT_I32, 1, {11, 0}},
  {"config.kimi", DT_I32, 1, {12, 0}},
};
static const int kTfwzSchemaN = 434;

[[noreturn]] inline void die_tz(const char* path, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "weights_io_tfwz: %s: ", path);
  std::vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  std::exit(1);
}

// carryless 32-bit binary arithmetic decoder, 16-bit probabilities
struct TzDecoder {
  const uint8_t *p, *end;
  uint32_t x1 = 0, x2 = 0xFFFFFFFFu, x = 0;
  TzDecoder(const uint8_t* b, size_t n) : p(b), end(b + n) {
    for (int i = 0; i < 4; i++) x = (x << 8) | byte();
  }
  uint8_t byte() { return p < end ? *p++ : 0; }
  int bit(uint32_t p1) {
    const uint32_t xm = x1 + uint32_t((uint64_t(x2 - x1) * p1) >> 16);
    const int y = x <= xm;
    if (y) x2 = xm; else x1 = xm + 1;
    while (((x1 ^ x2) & 0xFF000000u) == 0) {
      x1 <<= 8;
      x2 = (x2 << 8) | 255;
      x = (x << 8) | byte();
    }
    return y;
  }
};

// KT-estimator bit: P(1) = (n1 + 0.4) / (n0 + n1 + 0.8), counts halved at 2^16
struct TzBit {
  uint32_t n0 = 0, n1 = 0;
  uint32_t p1() const {
    const double p = (n1 + 0.4) / (n0 + n1 + 0.8);
    const uint32_t v = uint32_t(p * 65536.0 + 0.5);
    return v < 32 ? 32 : v > 65504 ? 65504 : v;
  }
  void update(int y) {
    if (y) n1++; else n0++;
    if (n0 + n1 >= 65536) { n0 = (n0 + 1) >> 1; n1 = (n1 + 1) >> 1; }
  }
};

struct TzCoder {
  TzDecoder* dec;
  int code(TzBit& b) {
    const int y = dec->bit(b.p1());
    b.update(y);
    return y;
  }
  uint32_t tree(TzBit* t, int nbits) {
    uint32_t node = 1;
    for (int k = 0; k < nbits; k++) node = (node << 1) | uint32_t(code(t[node]));
    return node - (1u << nbits);
  }
  uint32_t raw(int nbits) {
    uint32_t r = 0;
    for (int k = 0; k < nbits; k++) r = (r << 1) | uint32_t(dec->bit(32768));
    return r;
  }
};

struct TzSplitMix {
  uint64_t s;
  uint64_t next() {
    uint64_t z = (s += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
  }
};

inline void tz_noise_row(uint64_t seed, int tensor, int row, const uint32_t* hist,
                         int8_t* out, int n) {
  TzSplitMix r{seed ^ (uint64_t(tensor) << 40) ^ (uint64_t(row) << 16)};
  uint32_t cum[16];
  cum[0] = 0;
  for (int i = 0; i < 15; i++) cum[i + 1] = cum[i] + hist[i];
  for (int j = 0; j < n; j++) {
    const uint32_t u = uint32_t(r.next() >> 48);
    int s = 0;
    while (s < 14 && cum[s + 1] <= u) s++;
    out[j] = int8_t(s - 7);
  }
}

struct TzBf16Model {
  TzBit sign[8][2];
  TzBit exp[8][256];
  TzBit man[8][128];
};

inline uint16_t tz_bf16(TzCoder& c, TzBf16Model& m, int ctx) {
  const int s = c.code(m.sign[ctx][0]);
  const uint32_t e = c.tree(m.exp[ctx], 8);
  const uint32_t mm = c.tree(m.man[ctx], 7);
  return uint16_t((s << 15) | (e << 7) | mm);
}

struct TzIntModel {
  TzBit zero[4], sign[4], len[24], man[24][4];
};

inline int32_t tz_int(TzCoder& c, TzIntModel& m) {
  if (c.code(m.zero[0])) return 0;
  const int neg = c.code(m.sign[0]);
  int nb = 0;
  while (c.code(m.len[std::min(nb, 23)])) nb++;
  uint32_t r = 1;
  for (int b = nb - 1; b >= 0; b--) {
    const int pos = nb - 1 - b;
    r = (r << 1) | uint32_t(c.code(m.man[std::min(nb, 23)][std::min(pos, 3)]));
  }
  return neg ? -int32_t(r) : int32_t(r);
}

struct TzStream {
  TzCoder c;
  std::vector<TzBit> int4;
  std::unordered_map<std::string, std::unique_ptr<TzBf16Model>> bf;
  std::unordered_map<std::string, std::unique_ptr<TzIntModel>> im;
  TzBit is_bf16[4], noise_flag[4], rowflag[4], mode[4];
  std::vector<TzBit> clsm = std::vector<TzBit>(8);
  std::vector<TzBit> shiftm = std::vector<TzBit>(8);
  TzBf16Model& bfm(const std::string& k) {
    auto& p = bf[k];
    if (!p) p.reset(new TzBf16Model());
    return *p;
  }
  TzIntModel& imm(const std::string& k) {
    auto& p = im[k];
    if (!p) p.reset(new TzIntModel());
    return *p;
  }
};

// model kind of a tensor: the name without its layer number
inline std::string tz_kind(const std::string& name) {
  std::string s = name;
  if (s.rfind("blocks.", 0) == 0) s = s.substr(s.find('.', 7) + 1);
  return s;
}

inline size_t tz_esize(uint8_t d) { return d == DT_I8 ? 1 : d == DT_BF16 ? 2 : 4; }

// decodes tensor i of the schema into t (not rope.sin / rope.cos)
inline void tz_tensor(TzStream& S, int i, WTensor& t, const char* path) {
  const SchemaEntry& e = kTfwzSchema[i];
  TzCoder& c = S.c;
  const std::string name = e.name;
  t.dtype = e.dtype;
  t.shape.assign(e.dims, e.dims + e.ndim);
  t.numel = 1;
  for (uint32_t d : t.shape) t.numel *= d;
  t.data.assign(t.numel * tz_esize(e.dtype), 0);
  const size_t n = t.numel;

  if (e.dtype == DT_I8) {
    const int R = int(e.dims[0]), C = int(e.dims[1]);
    int8_t* q = reinterpret_cast<int8_t*>(t.data.data());
    std::vector<uint8_t> isnoise(R, 0);
    uint32_t hist[15] = {0};
    uint64_t seed = 0;
    if (c.code(S.noise_flag[0])) {
      const int all = c.code(S.noise_flag[1]);
      seed = c.raw(32);
      for (int k = 0; k < 15; k++) hist[k] = c.raw(17);
      for (int r = 0; r < R; r++) isnoise[r] = uint8_t(all ? 1 : c.code(S.rowflag[0]));
    }
    std::vector<int> cls(R, 0);
    if (c.code(S.mode[0])) {
      S.clsm.assign(8, TzBit());
      for (int r = 0; r < R; r++) cls[r] = int(c.tree(S.clsm.data(), 3));
    }
    S.int4.assign(16 * 8, TzBit());
    for (int r = 0; r < R; r++) {
      int8_t* row = q + size_t(r) * C;
      if (isnoise[r]) {
        tz_noise_row(seed, i, r, hist, row, C);
        continue;
      }
      TzBit* mc = S.int4.data() + 16 * cls[r];
      for (int j = 0; j < C; j++) {
        const uint32_t v = c.tree(mc, 4);
        if (v > 14) die_tz(path, "%s: int4 symbol out of range", e.name);
        row[j] = int8_t(int(v) - 7);
      }
    }
    return;
  }

  const std::string kind = tz_kind(name);
  if (e.dtype == DT_BF16) {
    uint16_t* v = reinterpret_cast<uint16_t*>(t.data.data());
    if (n < 8) {
      for (size_t k = 0; k < n; k++) v[k] = tz_bf16(c, S.bfm("scalar.bf16"), 0);
      return;
    }
    const uint16_t ref = tz_bf16(c, S.bfm("scale.ref"), 0);
    const int shift = int(c.tree(S.shiftm.data(), 3));
    TzIntModel& im = S.imm(kind + (shift ? ".s" : ""));
    auto red = [&](uint16_t b) {
      const int32_t u = int32_t((((b >> 7) & 0xFF) << (7 - shift)) | ((b & 0x7F) >> shift));
      return (b & 0x8000) ? -u - 1 : u;
    };
    auto unred = [&](int32_t k) {
      const bool neg = k < 0;
      const int32_t u = neg ? -k - 1 : k;
      const uint16_t b = uint16_t(((u >> (7 - shift)) << 7) | ((u & ((1 << (7 - shift)) - 1)) << shift));
      return uint16_t(neg ? (b | 0x8000) : b);
    };
    const int32_t rr = red(ref);
    for (size_t k = 0; k < n; k++) v[k] = unred(rr + tz_int(c, im));
    return;
  }

  TzBf16Model& m = S.bfm(kind);
  uint32_t* v = reinterpret_cast<uint32_t*>(t.data.data());
  const int exact = c.code(S.is_bf16[0]);
  const int cols = e.ndim == 2 ? int(e.dims[1]) : 1;
  for (size_t k = 0; k < n; k++) {
    if (exact)
      v[k] = uint32_t(tz_bf16(c, m, cols <= 8 ? int(k % cols) : 0)) << 16;
    else
      v[k] = c.raw(32);
  }
}

}  // namespace tfwz_detail

WeightsFile load_tfwz(const uint8_t* buf, size_t size, const char* path) {
  using namespace tfwz_detail;
  if (size < 5 || std::memcmp(buf, "TFWZ", 4) != 0)
    die_tz(path, "not a TFWZ file");
  if (buf[4] != 1) die_tz(path, "TFWZ version %u not supported", unsigned(buf[4]));
  TzDecoder dec(buf + 5, size - 5);
  TzStream S;
  S.c.dec = &dec;
  WeightsFile wf;
  wf.tensors.reserve(kTfwzSchemaN);
  for (int i = 0; i < kTfwzSchemaN; i++) {
    const SchemaEntry& e = kTfwzSchema[i];
    const std::string name = e.name;
    WTensor t;
    if (name == "rope.sin" || name == "rope.cos") {
      // recomputed from rope.inv_freq exactly as the FX2TFWC2 loader does,
      // honouring g_rope_rows_limit
      if (!wf.has("rope.inv_freq")) die_tz(path, "rope.inv_freq not decoded yet");
      const WTensor& inv = wf.get("rope.inv_freq");
      if (inv.dtype != DT_F32 || inv.numel != e.dims[1])
        die_tz(path, "rope.inv_freq mismatch");
      t.dtype = DT_F32;
      t.shape = {e.dims[0], e.dims[1]};
      if (g_rope_rows_limit != 0 && t.shape[0] > g_rope_rows_limit)
        t.shape[0] = uint32_t(g_rope_rows_limit);
      t.numel = size_t(t.shape[0]) * t.shape[1];
      t.data.resize(t.numel * sizeof(float));
      float* out = reinterpret_cast<float*>(t.data.data());
      const float* invf = inv.f32();
      const int cos_bias = name == "rope.cos" ? 1 : 0;
      for (size_t pos = 0; pos < t.shape[0]; pos++)
        for (size_t col = 0; col < t.shape[1]; col++)
          out[pos * t.shape[1] + col] = sincosf_cuda(float(pos) * invf[col], cos_bias);
    } else {
      tz_tensor(S, i, t, path);
    }
    if (!wf.tensors.emplace(name, std::move(t)).second)
      die_tz(path, "duplicate tensor %s", name.c_str());
  }
  wf.promote_small_bf16_to_f32();
  return wf;
}

}  // namespace fx2
