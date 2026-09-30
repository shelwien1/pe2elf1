# Lossy compression of the 6m transformer weights

Goal: make `6m-q4-fp32-t1lambda1.tfwc2` (2,845,074 bytes) as small as possible
while `coder0 c book1wrt` gets at most 1% worse (169,223 → ≤ 170,915 bytes).

## Result

| weights | size (bytes) | book1wrt → | loss |
|---|---:|---:|---:|
| original `6m-q4-fp32-t1lambda1.tfwc2` | 2,845,074 | 169,223 | – |
| original weights, lossless in the `tfwz` container | 2,806,749 (−1.35%) | 169,223 | 0 |
| requantization only (GPTQ + per-component allocation) | 2,403,749 (−15.5%) | 170,855 | +0.964% |
| **`weights/6m-q4-t1lambda1-lossy.tfwz`: + distillation** | **2,238,348 (−21.3%)** | **170,802** | **+0.933%** |

Verified end to end: `tfwz d` rebuilds the tfwc2, the unmodified `coder0`
(clang 18, `-O3 -fno-math-errno -ffp-contract=off -march=skylake`,
`TF_TRAIN=0 TF_FP32=0`, as in `gc.bat`) compresses book1wrt to 170,802 bytes and
decompresses it back byte-identical. (The same weights as a plain tfwc2 are
2,289,591 bytes.)

The requantization was calibrated on the first 128 KB of book1wrt; the
distillation used only other novels. On three novels never used for anything,
converted to book1wrt's format (128 KB each):

| | Tess of the d'Urbervilles | Great Expectations | Pride and Prejudice |
|---|---:|---:|---:|
| original weights | 47,714 | 44,389 | 42,080 |
| requantization only | 48,280 (+1.19%) | 44,935 (+1.23%) | 42,676 (+1.42%) |
| final | 48,426 (+1.49%) | 44,863 (+1.07%) | 42,738 (+1.57%) |

So on other text of the same kind the model loses 1.1–1.6%, not 0.93%: part of
the fit is specific to book1wrt (its prefix drove the allocation).

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
   1.15, …, 4} per group to minimize the entropy under a loss budget (in
   predicted bytes on the 128 KB prefix). Budget 370 — the largest that meets
   the limit without distillation — requantizes 46 groups; the final file uses
   budget 700: 60 groups requantized, 24 kept lossless.

5. **Per-row steps for MLP up-projections.** A hidden unit's importance is
   4·E[relu(h)²]·‖W_down[:,j]‖² (forward statistics only); row steps are
   k·importance^(-1/2), clipped to [1, 8]. At the same k this lowered both loss
   and rate (5–12%). The rows' step classes become coding contexts.

6. **fp32 side tensors** rounded to 4 mantissa bits (−10.8 KB, +11 bytes of
   loss on the prefix; 3 bits costs +84).

7. **Distillation** (`distill/`). With the loss recovered by training, the
   allocation can be pushed further: the final file is the allocation for a
   predicted budget of 700 (instead of 370), which alone would cost +1.52% on the
   book1wrt prefix and +0.933% on the whole file after distillation.
   * Data: seven Gutenberg novels (The Return of the Native, Jude the Obscure,
     The Mayor of Casterbridge, The Woodlanders, Under the Greenwood Tree,
     Middlemarch, David Copperfield; 655K tokens) run through cmix's WRT
     dictionary and fx2's permutation, which reproduces book1wrt's bytes exactly
     (`distill/wrt.cpp`, `english.dic` from cmix). The model's own generations
     are useless for this: from an empty PPMD state it produces loops or
     character soup.
   * Targets: the original model's 205-way distributions along those texts, with
     the same PPMD priors coder0 would give it (`patches/*-dump.patch`).
   * Training: the requantized model in the fp32 engine, KL(original‖student)
     back-propagated through its (one-step truncated) backward pass; Adam
     (lr 1e-5, batch 32) on latent fp32 copies of all 110 int4 matrices, every
     weight re-snapped to its row's grid after each step (straight-through
     estimator); the row scales are learned too (Adam at 3e-5 on log|scale|,
     rounded to bf16 every step). 2.3% of the weights end up on a different
     level; the file grows by 3.9 KB.
   * Learning rate matters: 1e-5 keeps improving; 3e-5 starts to drift after
     ~100K tokens and 5e-5/1e-4 diverge (roundings keep flipping back and
     forth). The 205-way KL on a held-out novel is a poor guide: for the
     budget-600 run it got slightly worse while compression improved a lot —
     the coder only uses the file's alphabet, mixed with PPMD.
   * With fixed scales this removes 42–47% of the loss of every allocation
     tried on the book1wrt prefix (budget 500: +1.21% → +0.64%, 600: +1.42% →
     +0.82%, 700: +1.52% → +0.86%). Full file with fixed scales: 500 → +0.774%
     (2,338,523 B), 600 → +0.931% (2,284,440 B), 700 → +1.032% (2,238,976 B, over
     the limit). Learning the scales brings budget 700 to +0.933% (the final
     file); budget 800 with learned scales reaches +1.284% (2,196,752 B).
     Continuing the fixed-scale 700 run on six more novels at lr 5e-6 with
     learned scales only got to +0.996%.

8. **`tfwz` container** (`tfwz/tfwz.cpp`): the tensor schema (names, shapes,
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

(This table is for the requantization-only file; the final one requantizes 14
more groups, 60 of 84 — see `runs/choiceD_B700.json`.) The final file is 2,185,934 bytes of
int4 weights, 24,590 of bf16 scales and 27,815 of fp32 values.

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
- **GPTQ damping** 0.03/0.1 is clearly worse (+1970/+2291 on the budget-370
  allocation); 0.0001–0.003 give +1572..+1680 against +1632 at 0.01 — the same
  within configuration variation, at 1.7–3 KB more.
- Sensitivity fits including plain-rounding measurements, or shrunk toward the
  per-type mean, allocated worse than the plain GPTQ fit.

## Caveats

- Calibration uses book1wrt itself (its first 128 KB): the activation statistics
  and the per-group measurements. Decisions are per component (84 step choices),
  never per weight from loss gradients, and nothing depends on which tokens
  book1wrt uses (all embedding rows are untouched). The held-out 72% of the file
  loses the same as the calibration part, but other novels lose 1.1–1.6% (table
  above), so some of the fit is book1-specific. The distillation step uses no
  book1wrt data at all; it adapts the model toward 19th-century English novels.
- Effects of a configuration are systematic, not luck: over 35 full-file
  candidates, held-out loss tracks prefix loss with correlation 0.979
  (`runs/candidates.csv`, requantization only). The final file is 113 bytes
  under the limit; the next larger allocation tried (budget 800, 2,196,752 bytes
  after distillation) was +1.284%.
- All numbers come from my Linux build; the engine is meant to be bit-exact
  across compilers (tf/PORTING.md), so the Windows clang build should match.

## Side finding: book1wrt is permuted twice

book1wrt already carries fx2's byte permutation (it contains `K O J L N` where
the text has `; ? : < >`, and none of the originals), and coder0 applies
`TF_Permute` to its input again. So on book1wrt these 2,742 punctuation bytes
reach the model as spare tokens instead of the ones it was trained on. Feeding
coder0 the un-permuted stream (apply `TF_Permute` once more to book1wrt, or skip
the permutation when preparing the file) gives **168,353 bytes instead of
169,223 (−0.51%)** with the original weights, round trip verified. Everything
above uses book1wrt as given.

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

cd $X/tools    # requantize -> out.tfwz, out.tfwc2
python3 build.py ../runs/choice9_B370.json out    # the requantization-only file
python3 build.py ../runs/choiceD_B700.json d700   # the final file's starting point
```

(`patches/weights_io-order.patch` adds the `WeightsFile::order` list that
`tfwc.cpp` also relies on; `tfwz/tf/` already contains it. With
`-DTF_TRAIN=3` instead of 0 the stats build also records backward-pass
statistics.)

`runs/` holds the measurements behind the allocation: the per-group sweeps
(`g15.txt` plain rounding k=1.5, `gp20.txt`/`gp30.txt` GPTQ k=2/3, `op.txt` at the
operating points), the rate/error tables, the fitted sensitivities (`c_v2.json`)
and the allocations (`choice9_B370.json` requantization only, `choiceD_B700.json`
for the final file). The pipeline that produced them:
`table.py`/`table_up.py` (rate/error tables) → `plan1.py`, `plan2.py` (fits) →
`alloc.py` (solver) → `build.py`. `ev.sh` runs candidates in parallel,
`heldout.py` splits their loss into prefix and held-out part.

## Distillation, reproducing

```sh
# the WRT transform needs cmix's dictionary: github.com/byronknoll/cmix
g++ -O2 distill/wrt.cpp $CMIX/src/preprocess/dictionary.cpp -I$CMIX/src/preprocess -o wrt
./wrt $CMIX/dictionary/english.dic novel.txt novel.wrt     # Gutenberg text, header/footer cut
# in 003/ with patches/*-dump.patch and fp32_model-linreg.patch applied:
#   dgen = coder0 built with -DTF_FP32=1; TF_DUMP=file records token, prior, target
TF_DUMP=novel.dump ./dgen c novel.wrt /dev/null 6m-q4-fp32-t1lambda1.tfwc2
#   common_head.inc = coder0.cpp up to "static const uint CNUM"
clang++ -std=c++17 -O3 -ffp-contract=off -march=native -DTF_FP32=1 -DTF_TRAIN=0 dtrain.cpp -o dtrain
LR=1e-5 SLR=3e-5 BATCH=32 ./dtrain requantized.tfwc2 out.q tr122.dump tr145.dump ...   # SLR: learn scales
python3 tools/qmerge.py requantized.tfwc2 out.q distilled.tfwc2 scales
```

The final run used, in this order, the first 64 KB of Gutenberg #122 and the
first 128 KB of #145, then 64 KB of #153, 128 KB of #766, 64 KB of #143, 128 KB
of #2662 and 64 KB of #482, starting from `runs/choiceD_B700.json` built with
`tools/build.py` (log: `runs/distill/D700s.log`).
