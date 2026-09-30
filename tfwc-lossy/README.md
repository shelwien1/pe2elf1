# Lossy compression of the 6m transformer weights

Goal: make `6m-q4-fp32-t1lambda1.tfwc2` (2,845,074 bytes) as small as possible
while `coder0 c book1wrt` gets at most 1% worse (169,223 → ≤ 170,915 bytes).

## Result

| weights | size (bytes) | book1wrt → | loss |
|---|---:|---:|---:|
| original `6m-q4-fp32-t1lambda1.tfwc2` | 2,845,074 | 169,223 | – |
| original weights, lossless in the `tfwz` container | 2,806,749 (−1.35%) | 169,223 | 0 |
| **`weights/6m-q4-t1lambda1-lossy.tfwz`** | **2,403,749 (−15.5%)** | **170,855** | **+0.964%** |
| the same lossy weights written as a plain tfwc2 | 2,450,993 (−13.9%) | 170,855 | +0.964% |

Verified end to end: `tfwz d` rebuilds the tfwc2, the unmodified `coder0`
(clang 18, `-O3 -fno-math-errno -ffp-contract=off -march=skylake`,
`TF_TRAIN=0 TF_FP32=0`, as in `gc.bat`) compresses book1wrt to 170,855 bytes and
decompresses it back byte-identical. book1000: 1,889 → 1,908.

The first 128 KB of book1wrt were used for calibration; the loss there is
+0.94%, on the remaining 72% of the file +0.98%.

```sh
cd tfwz && ./build.sh
./tfwz d ../weights/6m-q4-t1lambda1-lossy.tfwz 6m-lossy.tfwc2   # < 1 s
coder0 c book1wrt book1wrt.cmp 6m-lossy.tfwc2
```

## What the weights look like

5.87M int4 weights (97.5% of the file), 26K bf16 row scales, 28K fp32 values
(conv, dt_bias, beta, norm gains; all bf16-exact), 6.5 KB of names and shapes.

The int4 values are close to i.i.d. noise per tensor (±7 are over-represented,
the rows are clipped). Per-tensor order-0 entropy is 3.75 bits/weight; even an
unreachable per-row empirical entropy is only 1.7% lower, and no row, column or
neighbour context helps. So lossless modelling has ~1% to give, and the rest
has to come from changing weights where the model does not care.

The model is sensitive: requantizing everything with a 1.25× coarser step costs
+3.4% (1.5×: +4.2%, 2×: +13%, 3×: +17%). But sensitivity differs by more than
100× between components, which is what the method exploits.

## Your two ideas, measured

**Lossy image codecs** (each int4 tensor as an 8-bit grey image with levels ×18,
decoded and rounded back to the 15 levels; loss on the 128 KB prefix):

| codec | image bytes | weights changed | loss |
|---|---:|---:|---:|
| JXL 0.7, d=0.6 | 3,681,039 | 1.6% | +0.43% |
| JXL 0.7, d=1.0 | 3,118,978 | 10.5% | +3.4% |
| WebP 1.3.2, q=75 | 3,069,142 | 5.7% | +1.3% |

Settings anywhere near the 1% budget produce files bigger than the lossless
2.85 MB tfwc2. JXL only gets below it from d=1.5 on (2.66 MB), with 27% of the
weights changed and 2.6× the error of d=1.0, which already costs +3.4%. There is
no spatial redundancy for a transform codec to find, and at equal MSE plain
requantization is ~12% smaller than JXL (JXL d=2: 2.24 MB at MSE 0.55 q-units²;
requantization with a 2.5× step: 1.98 MB at the same MSE).

**Regenerating "pure noise" from an RNG** (seeded, matching the tensor's own
15-level histogram, row scales kept): no tensor is noise in the functional
sense — noise is never meaningfully better than simply zeroing the tensor, and
usually far worse. Layer 0, prefix loss:

| tensor | RNG noise | zeroed |
|---|---:|---:|
| value | +15.7% | +2.9% |
| key | +6.5% | +2.9% |
| query | +2.8% | +2.9% |
| gates | +6.3% | +6.0% |

(Zeroing any of q/k/v switches the attention off, hence the identical +2.9%.)
Dropping whole blocks costs ≥ +1.25% each (cheapest: layer 9 attention, 91 KB),
and there are no dead MLP units (importance spread q10/q90 is only 4–18×). The
only true noise are 97 rows with near-zero scales (vanilla q/k), worth ~2 KB.
`tfwz` still implements RNG regeneration (`noise` plan lines, per tensor or per
row) if you want to experiment with it.

What *does* hold from your intuition: required quality differs strongly per
component — see the table below.

## Method

1. **Calibration statistics.** A stats build of the fp32 engine
   (`patches/*-stats.patch`) runs the first 128 KB of book1wrt and records, per
   linear layer, the input second-moment matrix A = E[x xᵀ].

2. **GPTQ requantization.** A tensor is requantized to a coarser grid (row scale
   × k, still int4 with a bf16 scale, so the unmodified engine runs it) with GPTQ:
   columns in decreasing order of diag(A), each rounding error compensated in the
   not-yet-quantized columns through the Cholesky factor of A⁻¹ (damping 0.01).
   At the same step this cuts the actual loss 3–20× versus plain rounding
   (prefix loss at k=1.5: L0.up +187 → +31, L3.output +87 → +13, L11.value
   +67 → +3, unembedding +287 → +93).

3. **Sensitivity per component.** 84 groups (per layer: q, k, v, o, the four KDA
   gate matrices, mlp.up, mlp.down; plus the three embeddings). Each group was
   measured alone at GPTQ k=2 and k=3 on the prefix; the loss is modelled as
   ΔL_g = c_g · E_g, E_g = Σ_rows (w − ŵ)ᵀ A (w − ŵ).

4. **Allocation.** A Lagrangian multiple-choice knapsack picks k ∈ {1 (lossless),
   1.15, …, 4} per group to minimize the entropy under a loss budget. Final:
   46 groups requantized, 38 kept lossless.

5. **Per-row steps for MLP up-projections.** A hidden unit's importance is
   4·E[relu(h)²]·‖W_down[:,j]‖² (forward statistics only); row steps are
   k·importance^(-1/2), clipped to [1, 8]. At the same k this lowered both loss
   and rate (5–12%). The rows' step classes become coding contexts.

6. **fp32 side tensors** rounded to 4 mantissa bits (−10.8 KB, +11 bytes of
   loss on the prefix; 3 bits costs +84).

7. **`tfwz` container** (`tfwz/tfwz.cpp`): the tensor schema (names, shapes,
   order) is built in, as it is in the engine; one binary arithmetic coder with
   KT-estimator bit trees; one int4 model per tensor and row class, the encoder
   choosing the row classes by simulated cost; row scales as deltas from the
   tensor's median; bf16-exact fp32 as sign/exponent/mantissa trees. `tfwz d`
   reproduces the tfwc2 byte for byte.

Per component, final vs original (order-0 entropy of the int4 values):

| type | original | final | saved | lossless groups |
|---|---:|---:|---:|---:|
| mlp.up | 835,210 | 800,873 | 4.1% | 7/12 |
| mlp.down | 830,382 | 686,029 | 17.4% | 3/12 |
| attn output | 210,172 | 183,121 | 12.9% | 8/12 |
| attn value | 208,392 | 178,681 | 14.3% | 7/12 |
| attn query | 203,907 | 144,314 | 29.2% | 2/12 |
| attn key | 203,725 | 156,962 | 23.0% | 5/12 |
| KDA gates | 202,636 | 167,124 | 17.5% | 3/9 |
| embedding, prior_embedding, unembedding | 55,190 | 55,190 | 0 | 3/3 |

The final file is 2,351,690 bytes of int4 weights, 24,235 of bf16 scales and
27,815 of fp32 values.

## Also tried, rejected

- **RDO rounding** (penalizing expensive symbols inside GPTQ): −20 KB for +334
  bytes — the systematic shrinkage toward zero hurts far more than its error
  suggests.
- **Uniform absolute steps** per tensor (equal absolute error in every row):
  2.9% less rate at equal predicted error, but ~+190 bytes more real loss at
  equal size — large rows really are more sensitive.
- **Row weights from backward-pass gradients** for attention q/k/v: rate −15..36%
  but loss up to +86 vs +6 (the engine's backward is truncated at one step, which
  misrepresents k/v/gates). A forward-statistics proxy for value rows: no better
  than a coarser uniform step.
- **Rescaling MLP units** (up row ×α, down column ×α⁻², exact for relu²): 0–3%
  offline, and risky with the engine's fixed int8 activation scales.
- **GPTQ damping** 0.03/0.1 is clearly worse (+1970/+2291 on the final
  allocation); 0.0001–0.003 give +1572..+1680 against +1632 at 0.01 — the same
  within configuration variation, at 1.7–3 KB more.
- Sensitivity fits including plain-rounding measurements, or shrunk toward the
  per-type mean, allocated worse than the plain GPTQ fit.

## Caveats

- Calibration uses book1wrt itself (its first 128 KB): the activation statistics
  and the per-group measurements. Decisions are per component (84 step choices),
  never per weight from loss gradients, and nothing depends on which tokens
  book1wrt uses (all embedding rows are untouched), so the model stays general.
  The held-out 72% of the file loses the same as the calibration part.
- Effects of a configuration are systematic, not luck: over 35 full-file
  candidates, held-out loss tracks prefix loss with correlation 0.979
  (`runs/candidates.csv`). The final file is the smallest candidate within the
  limit, 60 bytes under it; the one after it (2,403,179 bytes) was +1.015%.
- All numbers come from my Linux build; the engine is meant to be bit-exact
  across compilers (tf/PORTING.md), so the Windows clang build should match.

## Reproducing

```sh
# X = this directory, S = the unpacked 003/ coder sources; needs python3 + numpy
cd $X && tfwz/build.sh
g++ -std=c++17 -O2 tools/wdump.cpp -o tools/wdump      # tfwc2 <-> flat dump for numpy
tools/wdump x $S/6m-q4-fp32-t1lambda1.tfwc2 tools/orig.wd

cd $S    # the eval build (TF_COSTLOG=file writes per-byte code lengths)
patch -p1 < $X/patches/coder0-costlog.patch
clang++ -std=c++17 -O3 -fno-math-errno -ffp-contract=off -march=skylake \
        -DTF_TRAIN=0 -DTF_FP32=0 coder0.cpp -o coder0e
# ... then the stats build: fp32 engine with statistics hooks
patch -p1 < $X/patches/coder0-stats.patch
patch -p1 < $X/patches/fp32_model-stats.patch
patch -p1 < $X/patches/transformer-stats.patch
clang++ -std=c++17 -O3 -fno-math-errno -ffp-contract=off -march=native \
        -DTF_TRAIN=0 -DTF_FP32=1 coder0.cpp -o coder0s
head -c 131072 book1wrt > b128k
TF_STATS=$X/runs/stats_b128k.bin ./coder0s c b128k /dev/null 6m-q4-fp32-t1lambda1.tfwc2

cd $X/tools    # requantize with the final allocation -> out.tfwz, out.tfwc2
python3 build.py ../runs/choice9_B370.json out
```

(`patches/weights_io-order.patch` adds the `WeightsFile::order` list that
`tfwc.cpp` also relies on; `tfwz/tf/` already contains it. With
`-DTF_TRAIN=3` instead of 0 the stats build also records backward-pass
statistics.)

`runs/` holds the measurements behind the allocation: the per-group sweeps
(`g15.txt` plain rounding k=1.5, `gp20.txt`/`gp30.txt` GPTQ k=2/3, `op.txt` at the
operating points), the rate/error tables, the fitted sensitivities (`c_v2.json`)
and the final allocation (`choice9_B370.json`). The pipeline that produced them:
`table.py`/`table_up.py` (rate/error tables) → `plan1.py`, `plan2.py` (fits) →
`alloc.py` (solver) → `build.py`. `ev.sh` runs candidates in parallel,
`heldout.py` splits their loss into prefix and held-out part.

## Where more could come from

The loss budget is spent almost entirely on second-order damage that GPTQ already
minimizes layer by layer. The next step would be recovering it by training:
distilling the requantized model toward the original on text the original model
generates itself (it needs no data from book1wrt), with the int4 grid kept by a
straight-through estimator. The fp32 engine has a (truncated) backward pass and
AdamW, so this is feasible, but it is a project of its own.
