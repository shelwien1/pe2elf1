// Dense GEMV kernels over the qmat.h row-major arenas (AVX2, Zen 2 tuned).
// All kernels produce int32 dots that are bit-identical to the naive scalar
// reference (kernels.cpp dot_i8 on the signed ints); the fp32 epilogues use
// exactly the single-rounding formulas documented per function.
//
// Shapes supported (d_out x d_in): any d_out; d_in in {64, 192, 224, 768}.
// Activations: u8, 32B-aligned, zero-padded to m.stride (biased qa+128 or raw
// [0,127] depending on the arena's corr convention — see qmat.h).
#pragma once

#include "qmat.h"

namespace fx2 {
namespace opt {

// (a) out[r] = float(dot[r]) * scale[r]      for r in [0, rows_padded)
void qgemv_f32(const QDense& m, const uint8_t* act, float* out);

// (b) out[r] = out[r] + float(dot[r]) * scale[r]
//     (multiply rounded first, then the add — matches "residual += y" of the
//      naive model exactly; do NOT fuse into an FMA)
void qgemv_add(const QDense& m, const uint8_t* act, float* out);

// (c) mlp.up fused relu^2 + activation-quantize epilogue (d_in must be 192):
//       y = float(dot[r]) * scale[r]
//       y = max(y, 0);  h = y * y                       (fp32)
//       t = h / s_next  (IEEE div);  t = min(t, 127.0f)
//       q = cvt round-half-even(t)                      (in [0,127])
//     q_out[r] = q (u8, raw); idx_out receives the indices r with q != 0
//     (ascending); returns nnz. idx_out needs rows_padded + 8 entries of
//     room (written in 16 B blocks; slack entries are in-range indices).
//     Bit-identical to the naive path: qmatvec -> h=(y>0?y*y:0) ->
//     quantize_i8(h, s_next) reinterpreted as u8.
int qgemv_relu2q(const QDense& m, const uint8_t* act, float s_next,
                 uint8_t* q_out, uint16_t* idx_out);

// (d) low-rank gate chain epilogue (d_in must be 192; d_out 64):
//       y = float(dot[r]) * scale[r]
//       t = y / s_next;  t = clamp(t, -128.0f, 127.0f)
//       q = cvt round-half-even(t);  q_out[r] = u8(q + 128)   (BIASED u8)
//     Bit-identical to naive quantize_i8(y, s_next) + 128.
void qgemv_quant_bias(const QDense& m, const uint8_t* act, float s_next,
                      uint8_t* q_out);

// -------- dynamic per-token activation scales (cmix-lex-transformer's TF_ACTQ_DYN; here chosen at load
// from config.actq_dynamic_mm, OptModel::dyn) --------
// The arena's scale[r] then holds the weight row scale only (arena_build folds no activation scale), and
// every epilogue applies the token's activation scale s as ONE extra fp32 multiply (bit-identical to the
// naive chain kernels.cpp qmatvec -> scale_vec -> ...):
// (e) out[r] = (float(dot[r]) * scale[r]) * s
void qgemv_f32s(const QDense& m, const uint8_t* act, float s, float* out);
// (f) out[r] = out[r] + (float(dot[r]) * scale[r]) * s        (mul, mul, then add -- no FMA)
void qgemv_adds(const QDense& m, const uint8_t* act, float s, float* out);
// (g) mlp.up with DYNAMIC quantization of the relu^2 output (d_in 192, d_out <= 768):
//       y = (float(dot[r]) * scale[r]) * s_in;  h = max(y, 0)^2            (fp32, h >= 0)
//       s_h = max(max_r h[r], 1e-12f) / 127.0f                              (returned through *s_next)
//       q = cvt round-half-even(min(h / s_h, 127.0f))  (IEEE div, in [0,127]); q_out / idx_out / return as (c)
int qgemv_relu2q_dyn(const QDense& m, const uint8_t* act, float s_in, uint8_t* q_out, uint16_t* idx_out,
                     float* s_next);
// (h) low-rank gate chain with DYNAMIC quantization of the 64-dim intermediate (d_in 192, d_out <= 768):
//       y = (float(dot[r]) * scale[r]) * s_in;  s_y = max(max_r |y[r]|, 1e-12f) / 127.0f   (returned)
//       q_out[r] = u8(clamp(cvt rne(y / s_y), -128, 127) + 128)                            (BIASED u8)
float qgemv_quant_bias_dyn(const QDense& m, const uint8_t* act, float s_in, uint8_t* q_out);

// test/debug epilogue: the exact int32 dots (correction already subtracted)
void qgemv_i32(const QDense& m, const uint8_t* act, int32_t* out);

// benchmark ablation variant of (a) with software prefetch disabled
void qgemv_f32_nopf(const QDense& m, const uint8_t* act, float* out);

// -------- packed int4 alternative (epilogue (a) + test i32 only) --------
// act is SIGNED int8, zero-padded to m.stride4; corr7 = 7 * sum_i qa[i].
void qgemv_packed_f32(const QPacked& m, const int8_t* act, int32_t corr7,
                      float* out);
void qgemv_packed_i32(const QPacked& m, const int8_t* act, int32_t corr7,
                      int32_t* out);

}  // namespace opt
}  // namespace fx2
