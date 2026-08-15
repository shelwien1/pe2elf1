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
coder0's `Counter`, with coder0's order-1 byte context dropped — i.e. the
order-0 version of what coder0 does. Header, palette and trailer bytes,
and any file that is not a plain uncompressed BMP raster (RLE, bitfields,
<8bpp, truncated pixel array, not a BMP at all), fall back to coder0's
order-1 model, so the tool is lossless for any input.

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

## 4. Results

Sizes in bytes; `coder0` is the order-1 baseline on the same file.

| file | | raw | bzip2 -9 | xz -9e | coder0 | **bmpc** | bpc |
|---|---|---|---|---|---|---|---|
| t8g.bmp | 320×240×8 | 77878 | 52454 | 56108 | 51784 | **49968** | 5.13 |
| t8p.bmp | 320×240×8 pal | 77878 | 52092 | 55964 | 51439 | **49624** | 5.10 |
| t24.bmp | 320×240×24 | 230454 | 225644 | 188344 | 182265 | **122585** | 4.26 |
| t32.bmp | 320×240×32 | 307254 | 307692 | 271776 | 272514 | **147775** | 3.85 |
| x_ep.bmp | 705×800×32 | 2256054 | 497718 | 498560 | 709320 | **408606** | 1.45 |
| 20000171A.bmp | 4096×512×32 | 8388662 | — | — | 4057586 | **2912183** | 2.78 |
| x_ai.bmp | RLE8 | 887278 | — | — | 285808 | 285808 | fallback |
| x_ci.bmp | RLE8 | 3278170 | — | — | 1046958 | 1046958 | fallback |

Every one of these round-trips (`CODER=./bmpc ./t.sh testfiles/*.bmp`), as
do an empty file, a 2-byte `BM`, 3 KB of `/dev/urandom`, a truncated BMP, a
top-down BMP with non-zero row padding and a trailer, and book1.

The two RLE8 files are not rasters, so they take the order-1 fallback and
match coder0 exactly (+1 byte for the mode flag).

On the 8 MB target image bmpc codes 2912183 bytes against coder0's
4057586 — 28.2% smaller — at 2.78 bits per byte.

## 5. Caveats

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
* The mix is `n_colors·60` multiply-adds plus the same number of
  `ParamUpdater` steps per byte, twice over (`MIX_DUAL`) — about 4 MB/s
  each way. `-DMIX_DUAL=0` costs 0.7% and buys back ~30%.

## 6. Reproducing

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
target. Two things it needs from the source side:

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
