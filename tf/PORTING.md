# tf/ — the fx2-cmix transformer inference engine

These files are taken verbatim from the `cpp_infer/src` tree of
`fx2-cmix-transformer` (Hutter-Prize submission by Kaido Orav / Byron Knoll et
al., transformer work on top of fx2-cmix), namely

    cpp_infer/src/weights_io.{h,cpp}
    cpp_infer/src/weights_io_compressed.cpp
    cpp_infer/src/opt/{qmat,qmat_dense,qmat_sparse,attn,kda,kda_math,glue,
                       vec_math,sparse_acts,arena_build,model_opt}.{h,cpp}

`cpp_infer/SPEC.md` and `cpp_infer/KIMI_SEMANTICS.md` in that submission
document the model (205 tokens, d_model 192, 12 layers — 9 Kimi linear-attention
layers and 3 sliding-window attention layers — int4 weights, int8 activations).

These are `.inc` files rather than `.cpp`: coder0 is built as a single
translation unit, so `coder0.cpp` includes `tf/tf_all.inc`, which includes
them. None of them carries a `#include <...>` of its own — every system header
the program needs is at the top of `coder0.cpp`.

Only seven changes were made, all mechanical:

1. **Include paths.** `arena_build.cpp` and `model_opt.cpp` referred to
   `../weights_io.h` / `../kernels.h`; the tree is flat here. (`kernels.h` is
   only pulled in under the `FX2_XCHECK_NAIVE_*` debug defines, which are never
   set, so the naive reference kernels are not part of this build.)

2. **`HugeBuf::alloc` is portable** (`arena_build.cpp`). The original used
   `mmap` + `madvise(MADV_HUGEPAGE)` unconditionally. That path is kept under
   `#if defined(__linux__)`; elsewhere (the Windows clang build) it falls back
   to an over-allocated `malloc` block aligned to 2 MB by hand. Nothing else in
   these sources is POSIX-specific.

3. **The RoPE tables can be shortened** (`weights_io.h/.cpp`,
   `weights_io_compressed.cpp`, `arena_build.{h,cpp}`, `model_opt.{h,cpp}`).
   `rope.sin` / `rope.cos` are not stored in the weights file: the decoder
   recomputes all 131072x32 entries of both from `rope.inv_freq`, which costs
   ~0.2 s and 32 MB of RAM. `fx2::g_rope_rows_limit` now caps how many rows are
   generated, `OptModel::load` takes the cap as a parameter and records the
   materialized length in `OptModel::rope_len`, and `model_opt.cpp` compares
   the position against that length instead of the compile-time constant. Rows
   are independent, so a prefix is exact; positions past the prefix already had
   a libm `sin`/`cos` fallback. coder0 caps the tables at the input size, so a
   4 KB file rebuilds 4096 rows instead of 131072.

4. **`TransformerOpt`'s constructor takes the cap** (`model_opt.h`) as a third,
   defaulted argument. The rest of the public interface — `begin_article`,
   the two `step` overloads, `last_logits` — is unchanged.

5. **No `std::` on the stdio calls.** `gc.bat` defines
   `_CRT_DISABLE_PERFCRIT_LOCKS`, which makes the UCRT define `fseek`, `ftell`,
   `fread` and `fclose` as macros expanding to `_fseek_nolock` and friends —
   so `std::fseek` becomes `std::_fseek_nolock`, which does not exist. All 33
   of these calls are now unqualified.

6. **Two anonymous-namespace name clashes resolved.** In separate translation
   units these were fine; in one they collide. `weights_io_compressed.inc`'s
   `die`, `dtype_size` and `Reader` became `die_wc`, `dtype_size_wc` and
   `ReaderWC` (`weights_io.inc` keeps the originals), and `arena_build.inc`'s
   `die` became `die_arena` (`qmat_dense.inc` keeps it).

7. **No dependence on infinity semantics.** `weights_io_compressed.inc` used
   `std::isinf` in its CUDA `sinf` port, and `attn.inc`/`glue.inc` used
   `-INFINITY` as a "no score here" sentinel. Under `-ffinite-math-only`
   (implied by `-ffast-math`) both are undefined — clang folds `isinf` to
   false. They are now a bit test and a large finite constant (`kNegHuge` in
   `qmat.h`), which behave identically under every flag set.

No numerical change: with the cap left at 0 the engine is bit-identical to the
original.

## fp32_model.cpp

`fp32_model.{h,cpp}` is not from the submission - it is a plain fp32
implementation of the same model, written against `cpp_infer/src/model.cpp`
(the naive reference forward), `SPEC.md` and `KIMI_SEMANTICS.md`. It
dequantizes every weight once at load (`w = q * row_scale`) and drops the
activation quantizers entirely, so the forward pass is ordinary floating point
and every parameter is a plain float in one contiguous arena. `weights()` /
`weight_count()` expose that arena: 5,897,145 floats, copy it to back the model
up and copy it back to restore.

Differences from the quantized path, all of them consequences of removing the
fake quantization:

* a matmul `y[o] = s_act*s_w[o]*<q(x), q(w[o])>` becomes `y[o] = <x, w[o]>` on
  the dequantized row (the folded `s_act` disappears with the quantizer);
* vanilla attention keeps its KV ring in fp32 and scores with
  `0.125*<q_h, k_h>` - the `sq*sk` factor existed only to undo int8 scaling;
* `log_baseline_decay_rate` is kept as the raw parameter and `-exp()` applied
  at use, rather than folded at load, so that every trainable number lives in
  the arena.

Everything else - `rms_norm`, the causal conv + SiLU, the KDA recurrence, the
gated norm, RoPE, the logit softcap and the softmax - was already fp32 in the
reference and is transcribed unchanged.

## Build flags

These translation units must **not** be compiled with `-ffast-math` / `-Ofast`
or with LTO against fast-math code:

* `weights_io_compressed.cpp` reproduces CUDA's `sinf`/`cosf` bit-exactly
  (Cody-Waite and Payne-Hanek reduction with `fmaf`) to regenerate the RoPE
  tables;
* the model's activation quantization is `clamp(round_half_even(x / s))` with
  IEEE division, and the attention softmax, the KDA exp/softplus chain and the
  logit softcap are all written against exact IEEE semantics.

This is why the single-TU build uses `-O3 -ffp-contract=off` rather than the
`-Ofast` coder0 was historically built with: one translation unit means one
flag set, and it has to be the one the transformer is correct under. Change 7
above removes the hard failure mode, but `-ffast-math` would still substitute
reciprocals for the divisions in the norm and quantization paths — measured at
13.6% relative on the output probabilities — so it stays off.

`-ffp-contract=off` is also passed. Clang's default already contracts nothing
in these sources (they use explicit FMA intrinsics), so it changes neither the
output nor the speed there; g++ defaults to `-ffp-contract=fast` and without it
produces a different model. With it, a clang and a g++ build of the engine are
bit-identical.

The kernels require AVX2, FMA, F16C and BMI2 — all present in `-march=haswell`,
which is what coder0 targets. (F16C is needed by `glue.cpp` alone, for the
float16 prior conversion.)

Two things that look like Windows risks and are not: `attn.cpp`'s hot-path GNU
extended asm (`pv_dense_i8`, hard-coded `%ymm0..%ymm13`) compiles for
`x86_64-pc-windows-msvc`, at the cost of the expected xmm6-xmm15 spills the
Win64 ABI requires; and mixing these strict objects with an LTO'd, `-ffast-math`
`coder0.o` is safe, because clang carries per-function FP attributes through
LTO. `attn.cpp`'s `pv_row()` remains an intrinsic-only fallback if the asm ever
misbehaves.

coder0 asks for `AttnKind::KVI8`, the variant cmix uses. It is bitwise
identical to `KVF32` and both faster and 1.7 MB smaller in cache footprint.
