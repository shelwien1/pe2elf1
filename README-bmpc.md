# bmpc — a lossless LPC compressor for BMP, on coder0's machinery

`bmpc.cpp` predicts every raster byte with an adaptive linear mix over the
8×8 pixel block above and left of it, and codes the residual with the
order-0 form of coder0's counter. The mix weights and the counter's
`wr`/`mw`/`K` are both driven by coder0's `ParamUpdater`.

```
./gc.sh                 # builds coder0 and bmpc
./t.sh                  # coder0 roundtrip on book1
CODER=./bmpc ./t.sh testfiles/*.bmp
```

---

## 1. What is predicted from what

The block is 8×8 **pixels**, with the pixel being coded in the middle of
the bottom row and the three pixels to its right still unknown:

```
       x-4 x-3 x-2 x-1  x  x+1 x+2 x+3
 y-7    .   .   .   .   .   .   .   .      '.' known -> n_colors inputs
 y-6    .   .   .   .   .   .   .   .      '?' unknown
  :     .   .   .   .   .   .   .   .
 y-1    .   .   .   .   .   .   .   .
 y      .   .   .   .   ?   ?   ?   ?
```

7·8 + 4 = **60 known pixels**, every colour component of each one an input,
so **n_colors·60 taps** per predicted byte — 60 for 8bpp, 180 for 24bpp,
240 for 32bpp — and one such mix per colour component. Outside the image
the block reads a replicated border (the row's own edge pixel sideways,
128 above row 0), so the gather is branch-free.

The residual goes through a 255-node binary decomposition coded by
coder0's `Counter`. That started as the order-0 version of what coder0
does — one tree, no byte context — and §4 is what replaced it. Header,
palette and trailer bytes, and any file that is not a plain uncompressed
BMP raster (RLE, bitfields, <8bpp, truncated pixel array, not a BMP at
all), fall back to coder0's order-1 model, so the tool is lossless for
any input.

## 2. The two things that had to be right before anything worked

Both were found by measurement, and both are worth more than every model
variant in §3 put together.

**The curvature handed to `ParamUpdater`.** The loss reaches the weights
only through `dot = w·x`, so its Hessian is `hdot · x xᵀ` — rank one, and
almost entirely *off*-diagonal. Feeding the diagonal `h_i = hdot·x_i²`
gives each of 240 mutually correlated taps its own full Newton step, and
the predictions overshoot by roughly the tap count. Feeding
`h_i = hdot·‖x‖²` instead makes what `ParamUpdater` emits,

```
        NW · D_i           NW · gdot · x_i
  Δw = ---------  ->  Δw = ---------------      i.e.  Δdot = -NW·gdot/hdot
        R_i + inc          hdot·‖x‖² + inc
```

the exact rank-1 Newton step, damped by `NW` — which is NLMS. **+33.5%**
for the diagonal form over this one.

**The operating point.** With inputs measured from 128, `‖x‖²` swings
about 1000:1 within a single image (x_ep: median 0.07, p99 52) and every
tap spikes together in dark or bright regions. Measuring inputs *and*
output from a neighbouring pixel instead — `pred = base + w·x`, the same
affine family reparameterised — is worth **−12.2%**. Which neighbour
barely matters once the mix adapts: W and (W+N)/2 are within 0.2% of each
other, MED (JPEG-LS) is no better.

A corollary: `momentum_D` and `momentum_R` both want to be **0**. An EMA
denominator cannot track a 1000:1 input-energy swing, and gradient
momentum mixes gradients taken at one input scale into a step normalised
at another (`momentum_D` = 0.9 costs **+9.7%**). The Newton denominator
is the instantaneous `hdot·‖x‖²`, and `inc` is NLMS's ε — the one thing
that keeps a flat region from dividing by nothing (`inc` = 0: **+19.3%**).

## 3. The predictor-formula experiments

Measured on a 6-file corpus (`sweep.py`): the four `testfiles` rasters,
`x_ep.bmp`, and a 128-row crop of the 8 MB 4096×512 32bpp BMP. `geo` is
the geometric mean of the per-file size ratios — the total is dominated by
whichever file is biggest, so `geo` is what a decision was steered by.

| experiment | result | kept |
|---|---|---|
| **rank-1 vs diagonal curvature** | +33.5% for diagonal | rank-1 |
| **operating point** none / W / N / (W+N)/2 / MED | +12.2 / +0.44 / +0.91 / 0 / +0.49 % | (W+N)/2 |
| **Huber loss** `gdot = clip(e,HUB)` vs L2 | **−2.4%** | yes, HUB = 6 byte levels |
| **L1 loss** `gdot = HUB·sign(e)` | +0.8% (+16.5% at high NW) | no |
| **adaptive Huber knee** = HUBA·EMA(\|e\|) | +1.7% at best HUBA | no — a *fixed* absolute knee bounds what one edge can do; a relative one lets a noisy image take huge steps |
| **logistic mix** — inputs stretched, output squashed, cross-entropy loss, `hdot = p(1-p)` | +1.3% at its own best NW, +4.3% at the linear optimum | no |
| **weights in the logistic domain** — `w = LO + SPAN·σ(z)`, optimise `z`, `f' = SPAN·s(1-s)`, `f'' = f'(1-2s)` (coder0's `OPT_MWLGT` chain) | −0.07% at NW=64, +1.05% at NW=224, +1.8% at the final NW | no — the box was never binding (±4 and ±16 give identical output), so the map only distorts the metric |
| **bias, as a constant-1 tap** | +0.36% | no — with a neighbour as the operating point the offset is already there |
| **bias, per-context** — EMA of the residual indexed by quantised (W−NW, NW−N, N−NE), JPEG-LS style | +2.0% at the gentlest rate tried, monotone worse as the rate rises | no — a 60-pixel adaptive mix already has the local structure this corrects for; the table only adds noise |
| **two mixes at different rates**, combined convexly with `λ = σ(z)`, `z` driven by `ParamUpdater` — *this* is where a logistic-domain weight paid | **−0.70%** (every file improves) | yes |
| residual: **zigzag fold** `|e|·2−(e<0)` | −0.42% | yes |
| residual: **one order-0 model per colour plane** | −4.4% | yes |
| inputs: **already-coded components of this pixel** (+n_colors−1 taps) | **−9.6%** | yes |

The last one is the single biggest model win and the one place the input
set departs from a literal reading of "60 pixels": when the G byte of a
pixel is coded, its B byte is already known, and a 24bpp raster is
B,G,R interleaved. It is still a linear mix of colour components of
adjacent pixels, with the adjacent pixel being the one being decoded.

### Final shape

```
pred = base + dotA + σ(z)·(dotB − dotA)          base = (W+N)/2, per component
dotA = wA·x   (fast mix)     dotB = wB·x   (slow mix)
x    = the 60 pixels' components, minus base, plus this pixel's coded ones
each mix:  Δw = −NW · clip(e,HUB) · x / (‖x‖² + inc)      via ParamUpdater
z:         driven by the combined error, in the logistic domain
residual:  (v − pred) folded to a zigzag, order-0 counter tree per plane
```

## 4. The residual coder

With the predictor exhausted, the order-0 residual model was what was
left between bmpc and a real image compressor, so it became a context-mixing
stage: seven `Counter` trees over different contexts, mixed in the logistic
domain by weights `ParamUpdater` drives with the cross-entropy gradient.
The models, and what each was worth when it was added:

| # | context | |
|---|---|---|
| 0 | plane | the original order-0 tree |
| 1 | plane × activity — \|e\| at W+N+NE, 8 buckets | how noisy it is here |
| 2 | plane × signed eW+eN, 17 buckets | which way the mix just missed |
| 3 | plane × signed error already made on the **previous component of this pixel** | −1.46% |
| 4 | plane × joint (qsigned eW, qsigned eN), 5×5 | −0.19% |
| 5 | plane × **the predicted byte**, 256 buckets | −0.97% at 16 buckets, −3.14% at 256 |
| 6 | plane × activity × **the fraction of the prediction**, 8 buckets | −1.26% (§7) |

Models 0–2 together are −2.9% over plain order-0; the stack is **−10.1%**.
Mixer weights are indexed by (plane, activity, depth in the bit tree) —
the models' relative worth is very different for the top bits of the
residual and for the noise in the bottom ones (−0.20%).

Two things the measurements insisted on, both echoing §2:

* **the mixer is fed `hdot = 1`, not the true cross-entropy `p(1-p)`.**
  The true curvature collapses to ~0 exactly where the model is confident,
  which is most bits, and the normalised step then blows up there.
* **it wants a much smaller `NW` than the pixel mix** (1024/4096 with a
  large ε). At the pixel mix's rate the residual mixer is +40%.

An SSE/APM stage on top — interpolated over `stretch(p)`, per plane and
tree node — is implemented (`RES_SSE=1`) and measured: worth −0.6% over a
*broken* mixer, +0.1% over a working one. A mixer that already has seven
opinions has nothing left for it to correct. It stays off.

`-DRES_NM=1 -DRES_MD=0` recovers the plain order-0 coder of §1.

## 5. Colour component order

Before anything else, the raster is stored **permuted**: coding position j
holds component `ord[j]`, one order for the whole image, chosen from all
`n_colors!` of them. Nothing else changes — it is a permutation, so it
inverts trivially and the stream carries only the order.

What it buys is entirely in the models. The mix's intra-pixel taps let
position j see positions 0..j-1 of the same pixel (§3, worth −9.6% on its
own), and residual model 3 keys on the error just made on position j−1 —
which for position 0 has no previous. Both want the component that best
explains the others coded **first**.

### Choosing it: everything cheap is degenerate

| criterion | corpus |
|---|---|
| LMS on the raw values, `sum_j Var(v_j | v_0..v_{j-1})` | **+0.13%** |
| the same in the gradient domain | **+0.15%** |
| trial-code a sample under each order | **−0.85%** (geo −1.55%) |

The first two lose, and they had to. Reordering changes no plane's
*content*, so any sum of conditional **log** variances is `log det(Cov)` —
provably identical for every order. (Measured: all six orders of a 3-plane
image score 19.0517 and the winner is decided by float noise.) That leaves
the LMS sum, which does vary — but it varies with *variance*, while the
order only matters through the things variance cannot see.

So the third option measures the actual objective. `Process()` puts the
model in a mode where it predicts and adapts exactly as it would but adds
`-log2 p(bit)` to a counter instead of emitting anything, and the encoder
codes one 32-row band under each of the `n_colors!` orders and keeps the
cheapest. Coding `t32` exhaustively under all 24 orders spans
117353..126763 — a **4.8% spread** — and LMS picks 124436.

Two details the sample needs:

* **contiguous bands, not scattered rows.** The mix reaches seven rows up;
  scattered rows measure a model that never sees a vertical neighbour.
  A 16-row band is already too short — it picks the identity everywhere
  and the gain vanishes.
* **one band is enough, and the cost is then bounded.** `CT_FRAC` asks for
  1/32 of the rows but never less than one band, so the trial costs
  `n_colors!` × 32 rows however big the image is. Sampling 1/8 instead
  picks the same order everywhere and costs several times the encode.

It is not free: the encoder does ~24 extra band-encodes, which is 1.5×–4×
its old time depending on the image. The **decoder is untouched** — it
reads the order.

## 6. Per-block statistics

For every 64×64 block (edge = `2**B0_BLKLW` × `2**B0_BLKLH`) and every
colour component, four numbers: **min**, **max**, the **AND** of all the
block's bytes (bits that are always 1) and their **OR** (whose complement
is the bits that are always 0). Those four are four small pictures — one
pixel per block, `4·n_colors` components — so a second `Raster`, with the
same mix and the same residual CM, codes them ahead of the raster itself.

Together they name the set of byte values the block can still hold:

```
legal(v)  =  min <= v <= max  &&  (v & mask1) == mask1  &&  (v & mask0) == 0
```

carried into the alphabet the residual tree actually codes — a 256-bit
rotation by the prediction (`r = (v-pred)&255`), then the zigzag fold as a
bit permutation (Morton spread of the low half, plus the reversed high
half onto the odd positions; ~30 instructions rather than 256 bit tests).
At each tree node, if only one of the two subtrees still holds a legal
symbol, **that bit is not coded and not modelled at all** — it carries no
information. A block whose component is constant costs nothing per pixel.
The prediction is also clamped into `[min,max]`, which is free.

### The four values are not independent

min and max are themselves values of the block, so

```
mask1 ⊆ min & max                         (BLK_DEP)
mask0 ⊆ ~(min | max)
  => mask0 & mask1 == 0                   follows; never coded separately
min == max  <=>  mask1 == ~mask0,  and then all four are the same byte
```

which is the same exclusion machinery pointed at the statistic picture
itself. After min and max, the masks have only `popcount(min & max)` and
`popcount(~(min | max))` free bits between them — and **none at all** for a
constant block. Worth −0.07% by itself, but the real effect is that it
made *small* blocks affordable: at 32×32 the statistics went from +0.14%
(a net loss) to −0.09%.

`max >= min` is in there too, and isolating it says it is worth **21
bytes** across the seven test rasters (+0.001%). That is not a bug in the
constraint, it is the same ceiling the whole idea runs into: `min` is a
direct intra-pixel tap for the mix and `max−min` is small and smooth, so
the counters had already put almost no probability below `min`. Making
that exactly zero recovers the rounding and nothing else. The masks paid
because the masks are the part that is *not* well predicted.

The obvious-looking alternative — coding the planes packed against each
other as plain numbers (`max−min`, `mask1` vs `min&max`, …) — is exact and
makes every number smaller, and it **loses** (`BLK_PACK`, +0.02%). They are
*pictures*: min and max are smooth, and the mix and the CM predict them
from neighbouring blocks. The transform trades that structure for
magnitudes the coder was not paying much for. Constraining them costs the
structure nothing; rewriting them costs it everything.

### Two of the four often do not pay

On photographs the masks pin down almost nothing that `[min,max]` had not
already pinned down, and coding them is a net loss even at their
constrained price. On data with structure in the low bits — quantised,
palette-mapped, 16-bit halves — they are the whole story. The encoder can
tell which it is holding: the masks' benefit is the alphabet they remove
from every pixel of the block, their cost is exactly the free bits the
constraints leave, and both are countable before anything is coded. So
`BLK_MASKS=2` decides per image and spends one flagged byte saying so.

| | corpus geo | corpus total | `quant.bmp` |
|---|---|---|---|
| no statistics | 0 | 0 | 101105 |
| min/max/mask0/mask1, always | −0.37% | −0.19% | **73035** |
| min/max only | −0.39% | −0.22% | 101105 |
| **decided per image** | **−0.39%** | **−0.22%** | **73035** |

`quant.bmp` (`tools/mkcorpus.sh`) is a synthetic raster whose low three
bits are always zero and which sets one more bit in half its blocks. The
flag pays for itself 27.8% there and costs one byte on everything else.

### Block size

| edge | geo | total | 8 MB image |
|---|---|---|---|
| 16×16 | **−0.95%** | −0.18% | 2651982 |
| 32×32 | −0.69% | **−0.29%** | 2644872 |
| 64×64 | −0.39% | −0.22% | **2643530** |
| 128×128 | — | — | 2643732 |

The statistic picture costs `n_planes·n_colors` bytes per block whatever
the block holds, and the constraint tightens as the block shrinks, so
where the two cross depends on how compressible the raster is rather than
on how big it is: a noisy 320×240 keeps wanting smaller blocks, a smooth
4096×512 does not. `B0_BLKLW`/`B0_BLKLH` are frozen out of `IDX/opt.pl` —
its corpus is crops, so it would answer that question with a bias it
cannot see — and stay at the 64×64 the brief started from, which is now
also the best on the 8 MB image (before the inter-plane constraints,
128×128 was).

The exclusion is worth less than the analysis says it should be (a median
block admits 22–39 of 256 values), and the reason is that the residual CM
already knew most of it: with the activity, the neighbour errors and the
predicted byte in its context, nearly all its probability mass was already
inside the legal set. What the statistics add is the difference between
*nearly* certain and free.

## 7. What MRP had to give

MRP-0.5 (Matsuda et al., *Minimum Rate Predictors*) is a two-pass image
coder built on entirely different ground: it segments the image into
classes over a quadtree, fits a separate linear predictor to each by
weighted normal equations, and iterates the whole thing against measured
code length, transmitting the coefficients. bmpc is one-pass and adaptive.
Four of its ideas port anyway; two paid.

| MRP idea | in bmpc | |
|---|---|---|
| **fraction of the prediction** (`bconv`/`fconv`, `PM_ACCURACY`) | residual model 6 | **−1.26%** |
| **distance-weighted activity** (`calc_uenc`, `ctx_weight`) | `RES_CTXW=1` | **−0.46%** on top |
| range-aware error map (`e2E`/`E2e`) | `RES_ZIGZAG=2` | +0.23% |
| per-class predictors (quadtree + `design_predictor`) | `MIX_CLS`, `MIX_LCLS`, `MIX_LOSS=4` | +0.7% … +9.6% |

**The fraction is the big one.** The mix produces a float; the coder is
told an integer. MRP keeps the fraction and indexes its probability model
by it, because 137.1 and 137.9 are not the same statement about the
residual even though both round to 137. bmpc was throwing it away at
`pb = int(f)`. Adding one more model — plane × activity × the fraction in
8 buckets, `nc·8·8` rows of 256 counters, about 50 KB — is worth −1.26%,
which is more than models 4 and 5 were worth put together. 4 buckets is
worse, 16 is the same. It is worth exactly *one* model: a second one on a
different context is +0.02%, and keying the **mixer's** weight set on the
fraction as well is +0.69%.

**The activity measure was too short.** bmpc summed |e| over W, N and NE.
MRP sums over all 12 causal neighbours within distance 3, weighted
`round(64/distance)`, and shifts by 6 — two noisy pixels beside each other
and two three away are not the same amount of local texture, and a
three-tap sum cannot tell them apart. Same context, same bucket count,
−0.46% (`RES_CTXW=1`; the `>>7` variant that keeps the old numeric scale
is only −0.03%, so it is the extra neighbours doing the work, not the
rescaling). Doing the same to the *signed* context of model 2 does not
pay (+0.16%) — direction averages away over 12 pixels in a way magnitude
does not.

**The range-aware error map loses**, which is a statement about bmpc, not
about MRP. `e2E` maps a value known to lie in [lo,hi] onto [0,hi−lo]:
inside ±th of the nearer bound both signs are possible and it is the plain
zigzag, outside it only one is, so the tail continues upward instead of
interleaving a bit that carries nothing. bmpc has better bounds than MRP
does — the per-block min/max of §6 rather than 0..255 — and it still
loses, for two reasons. The bit that carries nothing was already free:
§6's exclusion machinery walks the legal set down the bit tree and does
not code a node whose two subtrees are not both reachable. And bmpc's fold
is *modular*: a residual of +200 against a prediction of 250 wraps to +6
and costs almost nothing, where `e2E` calls it 255. Saturated predictions
followed by a jump to the other end of the range are common enough in real
images that the wrap is worth more than the alphabet compaction.
`RES_ZIGZAG=2` implements it and stays off.

**Per-class predictors do not survive the port.** This is MRP's headline
idea, and the reason it does not carry is that bmpc's predictor is already
local: a mix under `ParamUpdater` tracks whatever the last few hundred
pixels were doing, which is most of what a class label would have told it.
Splitting the weights by activity class (`MIX_CLS=4`) makes it **+9.6%** on
t24 — 240 taps need data, and each class gets a quarter of it, arriving in
bursts so its adaptation state is stale every time it is asked. The cheap
half — both mixes keep every pixel, and only the `MIX_DUAL` blend
coordinate is learned per class (`MIX_LCLS`) — is a wash: +0.7% on t24,
−0.14% on x_ep. MRP also weights its normal equations by `1/sigma[gr]^2`,
the residual spread of the activity group; the same statement here is a
Huber knee tracked per (component, activity) rather than per component
(`MIX_LOSS=4`), and that is +7% on t24. The knee is doing a different job
than the weighting is: it decides what is an *outlier*, and an outlier is
outlier-shaped relative to the whole component, not to its own bucket.

The two that paid cost nothing at decode beyond 12 byte loads and one more
counter per bit: **−1.72%** on the corpus together (−1.38% geo), and
2643708 → 2631749 on the 8 MB image. Per file, shipped build against
shipped build: t32 −1.88%, t24 −1.21%, x_ep −4.91%, and the two 8bpp files
**+0.16%** — at `n_colors=1` the mix is 60 taps rather than 240, its output
lands on a coarser grid, and there is correspondingly less in the fraction
to read. 35 bytes each; not worth making either toggle depend on depth.

## 8. Results

Sizes in bytes; `coder0` is the order-1 baseline on the same file.

| file | | raw | bzip2 -9 | xz -9e | coder0 | **bmpc** | bpc |
|---|---|---|---|---|---|---|---|
| t8g.bmp | 320×240×8 | 77878 | 52454 | 56108 | 51784 | **46941** | 4.82 |
| t8p.bmp | 320×240×8 pal | 77878 | 52092 | 55964 | 51439 | **46589** | 4.79 |
| t24.bmp | 320×240×24 | 230454 | 225644 | 188344 | 182265 | **97984** | 3.40 |
| t32.bmp | 320×240×32 | 307254 | 307692 | 271776 | 272514 | **115062** | 3.00 |
| x_ep.bmp | 705×800×32 | 2256054 | 497718 | 498560 | 709320 | **340266** | 1.21 |
| 20000171A.bmp | 4096×512×32 | 8388662 | — | — | 4057586 | **2631309** | 2.51 |
| x_ai.bmp | RLE8 | 887278 | — | — | 285807 | 286423 | fallback |
| x_ci.bmp | RLE8 | 3278170 | — | — | 1046957 | 1047507 | fallback |

Every one of these round-trips (`CODER=./bmpc ./t.sh testfiles/*.bmp`), as
do an empty file, a 2-byte `BM`, 3 KB of `/dev/urandom`, a truncated BMP, a
top-down BMP with non-zero row padding and a trailer, and book1.

The two RLE8 files are not rasters, so they take the order-1 fallback,
which is coder0's coder byte for byte — except that bmpc builds it with
`FASTM=1`, the polynomial `exp2`/`log2` of §9. That costs 0.2% here
(286423 against 285807) and next to nothing on the raster path (+0.08% on
x_ep, ±5 bytes on t24 and t32), which is why it is on; `-DFASTM=0`
reproduces coder0 to the byte.

On the 8 MB target image bmpc codes **2631309** bytes: 35.1% smaller than
coder0's 4057586, and 2.4% smaller than the 2694740 of `bmf`, which is
what this was being measured against.

Where it got there, on that file:

| | size | |
|---|---|---|
| order-1 (coder0) | 4057586 | |
| + the LPC mix, order-0 residual | 2912183 | −28.2% |
| + the 6-model residual mix | 2646464 | −9.1% |
| + a final `IDX/opt.pl` pass | 2646067 | |
| + per-block statistics (64×64) | 2643530 | −0.10% |
| + trial-chosen component order | 2643708 | +0.007% here; −4.8% on t32 |
| + MRP's fraction and activity models (§7) | 2631749 | −0.45% |
| + a fifth `IDX/opt.pl` pass, for the new mixer | **2631309** | |
| bmf, for reference | 2694740 | |

## 9. Speed

Best-of-N timings (`./bench.sh`), clang 18, `-Ofast -march=haswell -flto`:

| | before | after | |
|---|---|---|---|
| x_ep decode | 11.48s | **7.90s** | 1.45× |
| x_ep encode | 28.48s | **20.13s** | 1.41× |
| big_crop decode | 13.46s | **9.43s** | 1.43× |
| big_crop encode | 112.74s | **78.27s** | 1.44× |

Ablation says where the time is, and it is not where SIMD usually helps:
turning off the counter's three `ParamUpdater`s takes decode from 9.84s to
3.39s, so **66% of decode is the adaptive parameter machinery**, and
shrinking residual model 5's table from 25 MB to 1.5 MB (`RES_VQ=16`) saves
0.13s — it is **not** memory bound. What worked:

* **Stop computing things twice** (−7%, *bit-identical*). `Predict()` and
  `C_Update()` are always called as a pair on the same counter and the
  second recomputed two exponentials, a logarithm and the mix that the
  first had already produced. Carrying six floats across is the same
  arithmetic, once.
* **The rank-1 curvature makes the Newton denominator loop-invariant**
  (−9%). With `momentum_D = momentum_R = 0` — which is where the tuning
  landed (§2) — the per-tap `D` and `R` are pure temporaries, and since
  `h` is the same for every tap, `k = NW/(R+inc)` leaves the loop. The
  mix update collapses from three interleaved arrays with a divide per tap
  to one array and `w = clamp(w - clip(k·clip(g,Dc),step))` — a clean
  8-wide AVX2 FMA loop (`MIX_LEAN`). The same collapse on the residual
  mixer takes `RES_NM` divides *per bit* down to one.
* **The input gather had an integer division per input** (`base[j % nc]`,
  240 per pixel) and a 256-entry table lookup. With linear inputs the
  byte→float map is affine, so the bytes convert directly and the
  operating point folds into the same FMA against a pre-replicated base;
  `‖x‖²` accumulates in the same pass. `8·n_colors` is a multiple of 8 for
  every depth, so the row runs vectorise whole.
* **Fast `exp`/`log`** (−11%, +0.09% size). Exponent-field constructions
  with a minimax polynomial, ~1e-5 relative. Consistency is what the codec
  needs, not correct rounding — both sides run the same instructions.
  `-DFASTM=0` restores libm.
* **Prefetch** the first three tree nodes of all seven models at the top of
  `CodeRes` (they are in seven tables up to 25 MB apart), the next raster
  row in the gather, and ahead of the weight vector in the dot product.
  Worth little on its own, consistent with not being memory bound.

Two things measured and **rejected**: rewriting the eight RTRL trace
updates as an `R[]`/`C[]` vector pair was *slower* (9.42 → 9.73s) because
building the two vectors in memory costs more than the FMAs it saves —
clang already keeps those eight in registers. And vectorising across the
the models, the one place with real 6-wide parallelism, needs a gather per
field from six scattered tables; on Haswell (no AVX-512 scatter, ~15-cycle
gathers) the transpose costs about what the scalar work does.

`CT_KEEP=8` screens the component orders on a quarter band before scoring
the survivors in full — ~1.9× encode for 0.12–0.37% size. Off by default;
the screen is a weak signal and this project has been trading the other
way throughout.

## 10. Caveats

* **`-Ofast` makes the folded and unfolded builds differ slightly.** With
  a knob pinned by `-DB0_NW_w=1024` the compiler folds it and reassociates;
  read from its `mapping` object it cannot. Both are correct and both
  round-trip, but they are not bit-identical streams, and per-file sizes
  move by a few tenths of a percent either way (totals within 0.03%).
  IDX-FORMAT.md §12 asks the two builds to agree; under `-ffast-math`
  they agree on value, not on rounding. `sweep.py` numbers are therefore
  the `-D` build's; §4 is the shipped build's.
* 8bpp **palette** images are predicted on the index, which is only
  meaningful when the palette is ordered. t8p (paletted) and t8g
  (greyscale ramp) happen to land within 1% of each other here.
* It is slow: `n_colors·60` multiply-adds plus that many weight updates
  per byte, twice over (`MIX_DUAL`), and then seven `Counter` predict/update
  pairs and a mixer step per *bit* — about 220 KB/s decode (§9). `-DMIX_DUAL=0` costs 0.7% and buys back ~30% of the
  pixel-mix time; `-DRES_NM=3` costs ~4% and roughly halves the residual
  time.
* Memory is dominated by residual model 5: `n_colors·256·256` counters,
  ~25 MB at 32bpp, plus the statistic picture's own model (its 4·n_colors
  components would be 100 MB at the raster's bucket count, so `BLK_SVQ`
  cuts it to 16).
* The encoder now holds the whole pixel array in memory: the block
  statistics have to be measured before anything is coded.
* The encoder also trial-codes `n_colors!` × 32 rows to pick the component
  order, which is 1.5×–4× its old time. `-DCT_ON=0` removes it. The
  decoder is unaffected either way.
* A **static colour transform** — fit a linear predictor of each component
  from the ones before it and store residuals — was built and dropped.
  It is exactly invertible for any quantised weights (a lifting scheme),
  and it loses badly, up to **+44%**: LMS minimises global variance, but
  the codec is paid in *local gradient*, and those disagree. On x_ep it
  took plane 2's σ from 15.0 to 1.8 while its mean |dx| went 1.79 → 0.87
  (a real win), and plane 3's σ from 75.2 to 42.8 while its mean |dx| went
  1.38 → **8.51** (a rout). Scoring candidate transforms on the gradient
  instead did turn it into a small net win (−0.33%), but the reorder alone
  is worth −0.85% and costs nothing at decode, so the transform is gone.

## 11. Reproducing

Model shape (the `#define`s at the top of `bmpc.cpp`) was chosen with
`sweep.py`, which rebuilds with `-D` overrides and reports the corpus total
and `geo`:

```
./sweep.py 'MIX_LOSS=0' 'MIX_LOSS=1 B0_HUB=48' ...   # any -D config
DETAIL=1 ./sweep.py ...                              # per-file breakdown
CORPUS=big ./sweep.py ...                            # add the full 8 MB BMP
```

The B0 constants are then hill-climbed by the project's own optimizer,
which flips the bits of the `!MAP!` pattern strings inside the executable —
no rebuild, no source edit:

```
tools/bmpcrop.py testfiles/x_ep.bmp work/opt/ep64.bmp 64   # build the corpus
cp bmpc out/bmpc_opt
perl IDX/opt.pl opt.lst out/bmpc_opt                       # writes export.!!!
```

`IDX/opt.pl` is `IDX/auto_opt2.pl` with a corpus list and a configurable
target. Four rounds of it were worth −0.8%, −1.3%, −0.5% and −0.6% on the
full corpus, and a fifth after §7 changed the mixer's shape found −0.20%
on its own corpus (−0.02% on the full one) by moving exactly two knobs:
the mixer's weight seed `BMW0` down from 257/1024 to 193/1024, and its
per-weight gradient clip `DB` up from 6.97 to 7.52 — a seventh model each
weight has to share the output with, and a heavier tail to clip. A sixth
round found nothing further — it keeps finding things, and it found one the hand sweeps
missed entirely: `D_clip`, which nothing suggested would bind, wants to be
~1/1000 of its seed and is worth −0.7% by itself. Two things it needs from the source side:

* **the corpus has to cover every depth.** 8, 24 and 32bpp all take
  different paths through `n_colors`; whichever is missing drifts while
  nothing measures it. `opt.lst` carries one of each, cropped by
  `tools/bmpcrop.py` so a pass is seconds rather than minutes.
* **coder0's constants are frozen out of it.** `sh_counter0.inc` is
  included with `FREEZE_C0`, which redirects `pdesc` to `mdesc` for the
  C0 block: same objects, same values, no `!MAP!` marker, so the scan
  cannot see them (IDX-FORMAT.md §10). They are shared with `coder0.cpp`
  and tuned on book1 — an image corpus must not move them. The B0 knobs
  that only feed a switched-off toggle are frozen the same way, which
  takes the search space from 1387 bits to 215. `sh_mapping.inc` gained
  the three `m*` macros the format doc describes; this tree did not have
  them.
