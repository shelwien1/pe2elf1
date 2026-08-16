# mrpc — MRP's scheme, extended to colour

`mrpc.cpp` is a second compressor beside `bmpc`, on entirely different
ground. bmpc is one-pass and adaptive: everything it knows it learns while
coding. MRP (Matsuda et al., *Minimum Rate Predictors*, mrp-0.5) is
two-pass — it segments the image, fits predictors to the segments,
measures the resulting code length, and iterates the whole arrangement
against that measurement before transmitting any of it.

```
./gc.sh mrpc
CODER=./mrpc ./t.sh testfiles/*.bmp
```

---

## 1. What MRP does, and what had to change for colour

MRP is a greyscale coder. Its four moving parts:

1. a **class map** over a quadtree, up to 63 classes;
2. one **linear predictor per class**, fitted by weighted least squares,
   quantised to 6 fractional bits and transmitted;
3. a **generalized-Gaussian probability model** per activity group, where
   activity is a distance-weighted sum of neighbouring errors and the
   group thresholds are chosen by a trellis;
4. all of it **iterated against measured code length**.

Extending that to RGB/RGBA is not "run it three times". Three choices:

**One class map, shared.** The segmentation is a property of the image —
where the edges and the flat regions are — not of a plane. Three maps
would spend three times the side information describing the same edges.
Each class then carries its own coefficient vector *per component*.

**Taps that reach across components.** For component *k* the predictor
sees `PRD_ORDER` causal neighbours of component *k*, `XPRD_ORDER` causal
neighbours of every *other* component, and the components of the current
pixel already coded. That last group is offset zero — the same pixel — and
it is the whole colour story:

```
prd_R  =  G(0,0)  +  R(0,-1)  -  G(0,-1)
```

is expressible in these taps (coefficients +1, +1, −1, summing to 1, so
the predictor stays shift-invariant), and it is exactly what a per-plane
coder cannot say. The measurement below shows it is worth almost a third
of the file.

**Activity per component**, with this pixel's earlier components folded
in: if red just missed by 30, green is about to as well.

## 2. What the colour extension is worth

Corpus of three crops (24bpp, 32bpp, and a 32bpp screenshot), total bytes:

| | | |
|---|---|---|
| no colour taps at all — three grey coders | 174271 | |
| `XCUR` only (current pixel's other components) | 167216 | −4.0% |
| `XPRD_ORDER=6` only (spatial cross taps) | 179157 | **+2.8%** |
| both | **119450** | **−31.5%** |

The middle two rows are the interesting ones. **Neither half is worth
anything alone, and the spatial cross taps are actively harmful by
themselves.** The reason is in the formula above: the cross-component
prediction is a *difference*, `G(0,0) − G(0,-1)`. The current-pixel tap
supplies the level and the spatial cross tap supplies the reference to
subtract from it; with only one of them the predictor can add a term
proportional to a green sample but cannot cancel it, and it has spent
coefficients to do nothing. Together they are worth 31.5%.

`XPRD_ORDER` past 6 gives it back: 4 → 126542, 6 → 119450, 12 → 120790.

## 3. Everything else is MRP

Kept as it is in mrp-0.5, because measuring said so and because these are
the parts that make the scheme what it is:

* the value is coded directly from a **shifted pmf window** — the model
  spans `2·maxval+1` symbols and is indexed at `base + value`, where
  `base` comes from the prediction at 1/8-pixel accuracy (`PM_ACC=3`).
  The window `[base, base+maxval]` is renormalised on the fly, so the
  range constraint costs nothing and no error mapping is needed at all.
  (MRP's `e2E` only exists on its `pm_accuracy < 0` path. Its fractional
  indexing is the same idea that was worth −1.26% in bmpc, §7 there.)
* least squares **weighted by 1/σ²(group)**;
* the **paired coefficient search** — move weight between two taps keeping
  their sum, take the best of 11×3 shifts. This is worth **16%** on its
  own (`OPT_PRED=0`: 133797 against 112560), and it is most of the encode
  time.
* the **trellis** over activity thresholds, per class *and component*;
* the **quadtree** class search, splitting wherever a split pays for its
  own flag.

The one deliberate change: MRP draws the two taps of the coefficient
search at random. mrpc sweeps them instead — every tap paired with the one
a fixed distance away, the distance rotating with the iteration. Same
pairs, no worse convergence, and an encode that is reproducible: with the
random draw the same file compresses to sizes ~1% apart on the seed alone,
which is enough noise to hide most of what one is trying to measure.

## 4. What it does not share with bmpc

The BMP front end and the rangecoder, and nothing else. In particular
**no `Counter`**: nothing here runs a `ParamUpdater` per bit, or per
anything. The probability models are static tables built once from the
generalized-Gaussian family, so a pixel costs a table lookup and a
rangecoder step. That is not a compromise for speed alone — it is what
makes the scheme possible. `CalcCost` evaluates the code length of a
region without coding it, and the optimiser calls it once per candidate
class per block per iteration. With an adaptive model there is no such
function: you would have to run the coder.

Non-24/32bpp input (8bpp, RLE, non-BMP) falls back to a plain order-1 byte
model — adaptive binary probabilities with a shift update, still no
optimiser.

## 5. Results

| file | | raw | bmpc | **mrpc** | |
|---|---|---|---|---|---|
| t24.bmp | 320×240×24 | 230454 | 97984 | **96368** | −1.6% |
| t32.bmp | 320×240×32 | 307254 | 115062 | **105350** | −8.4% |
| x_ep.bmp | 705×800×32 | 2256054 | 340266 | **321162** | −5.6% |
| t8g / t8p | 8bpp | 77878 | 46941 / 46589 | 53452 / 53385 | fallback |

At `MRP_EFFORT=1`, the default. Effort 2 takes another 7% off all three.

mrpc wins on 32bpp — by more the bigger the image gets — and, since the
model work in §5, on 24bpp as well: t24 was mrpc's one loss at +1.5% and
is now a 1.6% win. The margin still has the shape of the two designs.
bmpc's strength is its residual coder, seven context models mixed per bit,
and t24 is photographic, which is where that pays. mrpc's strength is the
predictor, and it needs samples: a class is fitted from the pixels
assigned to it, so the larger the image the better the fit it can afford,
while an adaptive coder has already converged and gets nothing more.

**Encode time is the price, and decode time is the prize.**

| | bmpc enc | mrpc enc | bmpc dec | mrpc dec |
|---|---|---|---|---|
| t24 | 2.9 s | 21 s | 1.5 s | **0.07 s** |
| t32 | 9.8 s | 31 s | 2.1 s | **0.06 s** |
| 20000171A (8 MB) | 120 s | 521 s | 43.9 s | **0.60 s** |

The 8 MB image took over half an hour when it was first run, and printed
nothing while it did. It is 9 minutes now (§7), and the phase that
remains is the class search: 27 s of every 28 s iteration, because the
quadtree evaluates all 63 classes over the same pixels at each of its five
levels. Pruning the candidate list at the deeper levels is the next thing
worth measuring.

The decode column is why the whole arrangement is worth its encode: **73×
faster than bmpc on that file**, 0.60 s against 43.9 s. bmpc's decoder has
to replay every `ParamUpdater` step its encoder took; mrpc's reads a
transmitted model.

Encoding is slower because the coefficient search visits every pixel of a
class for each candidate pair, and the class search predicts every pixel
of a block under every class — both linear in the class count, which grows
with the image. Decoding is **9–25× faster** for exactly the reason the
models are static: a decoded pixel costs one dot product and one
rangecoder step, and there is no adaptation to replay. bmpc's decoder has
to redo every update its encoder did; mrpc's does not.

### Where the encoder's time went

AVX2 and one index, measured on a 4096×128 crop, two iterations of each
loop (85 s → 46 s overall, output byte-identical at every step):

| | before | after | |
|---|---|---|---|
| order trial | 14.3 s | **5.4 s** | vector normal equations |
| coefficient search | 17 s | **6 s** | class index, then vector candidates |
| class search | 11 s | **6 s** | vector prediction and activity |
| fit | 1 s | 1 s | |

* **The coefficient search was finding its pixels by scanning the image.**
  Once per (class, component, tap pair) — ten thousand full-image passes to
  reach a sixty-third of the pixels each time. A counting sort into a
  per-class pixel list, built once per iteration, took 17 s to 12 s before
  a single intrinsic was involved. The lesson is the usual one: the vector
  work was never the problem, the addressing was.
* **`org` is interleaved, so eight consecutive pixels of one component are
  a stride-`nc` gather.** Keeping a de-interleaved byte plane per component
  makes them one 8-byte load, and the 41-tap dot product becomes
  `cvtepu8_epi32` + `mullo` + `add` eight pixels at a time.
* **A pixel's error values are adjacent in `errB`**, so the twelve-tap
  activity sum computes *all* components in one vector rather than one per
  component — the class search's second-largest cost, 11 s to 6 s together
  with the above.
* **The 11×3 candidate grid is five vectors**, its predictions an affine
  function of two lane-constant patterns, and its two table lookups
  SIMD-computed indices into one contiguous cost block.
* **AVX2's `vgather` lost to eight ordinary loads** off those indices, 8 s
  against 6 s. It is a microcoded loop, and the addresses were already in
  registers. `-DMRP_GATHER=1` restores it.
* **Four accumulator chains, not one.** `mullo_epi32` is ten cycles of
  latency against one per cycle of throughput, so a single chain over 41
  taps waits about five times longer than the multiplies take.

### The decoder, and what did not work

Decoding is strictly sequential — pixel *x* needs pixel *x−1* — so it
cannot batch over pixels the way the encoder's searches do. Ablation (a
build with the symbol decode stubbed out) split the 0.77 s it then took on
the 8 MB image into roughly **45% prediction and bookkeeping, 55% symbol
decode**. It is 0.60 s now; the two changes below are why.

Two thirds of a prediction's taps read rows that are already *finished*,
so that part of every prediction in a row can be computed before the row
starts, eight pixels at a time out of the planes — the same trick that
worked so well in the encoder, and integer addition is associative so the
split is bit-exact. It was **slower**: 0.88 s against 0.77 s. Writing an
accumulator row and reading it back costs more than the 28 taps saved,
and the interleaved `org` was already giving several taps per cache line
where the planes spread them over four. Reverted.

(Along the way it did produce a real find. Row 0 is special: its taps
reach into the top border, which `BorderTopCol` is still filling in *as
row 0 is coded*, so precomputing that row reads cells that have not
happened yet. A self-check comparing the split against `Predict` —
`-DMRP_CHECK=1` — found it in one run.)

What did pay is the search inside `DecSym`. The window is always 256
symbols wide and the binary search over it is eight loads, each of which
must finish before the next address is known. Counting instead of
searching breaks the chain: eight *independent* probes pick a block of 32,
then that block is compared eight at a time. Same answer, no load waiting
on another load — 0.77 s → **0.74 s**.

### The rangecoder: `sh_rcm.inc`

That left the coder itself, and it was worth doing. `sh_v2f.inc` is
bit-oriented — it renormalises **one bit at a time**, `inpbit`/`outbit`
assembling the bytes by hand — and every multi-symbol operation costs
**three 64-bit divisions**: one in `rc_GetFreq` and two in `rc_Process`,
which cannot share a divide because their numerators differ. At ~2.6 bits
a symbol that is 2.6 trips through a bit loop plus three divisions.

`sh_rcm.inc` says the same thing the way `sh_rcv7.inc` does: the range
renormalises **a byte at a time** (`sTOP = 2^24`), so the loop runs about
a third of a time per symbol and each trip is a `getc`/`putc`; and there
is **one division per symbol** instead of three, with none at all on the
binary path — multiply and shift. It is a separate file because
`sh_v2f.inc` is shared with coder0 and bmpc, whose streams must not move.

Measured on a 4096×128 crop, with both builds predicting the *same* code
length (767513 bytes) so the comparison is the coder and nothing else:

| | actual | overhead | decode |
|---|---|---|---|
| `sh_v2f.inc` | 767815 | +302 B | 210 ms |
| `sh_rcm.inc` | 767815 | +302 B | **168 ms** |

**20% faster and not one byte larger.** Two things had to be got right:

* The naive `r = range/tot; range = r*freq` gives each symbol an interval
  of `r·freq` where an exact coder gives `range·freq/tot`, so every symbol
  is short by up to `tot/range`. With `tot` at 2²⁰ and the byte-renorm
  floor at 2²⁴ that cost **2110 bytes**, seven times the coder's entire
  overhead. Handing the leftover interval to the top symbol recovers
  *nine* of them — the loss is not the wasted interval, it is the
  rescaling of every symbol. Carrying 16 more bits in `r` (`RCM_PREC`,
  still one division, 64-bit multiplies) recovers all of it for 2 ms.
* With those bits the decoder's inverse is no longer `(code<<16)/r`.
  `Span()` truncates, so the inverse has to round the other way, or the
  symbol landing exactly on an interval boundary decodes one too low. It
  fails on rasters and not on the binary path, because the binary path
  never lands on one.

Two properties of `sh_rcv7.inc`'s flush are worth knowing if you read the
file: it settles `low` to the shortest value still inside
`[low, low+range)` rather than flushing all four bytes, and it stops as
soon as `low` is all-ones — so **the decoder must supply `0xFF` past the
end of the stream**. Returning 0 there costs the last few symbols of every
stream, which is exactly how it first failed.

### And two approximations, because the clock is a result too

Vectorising bought 2.2×; these bought the rest, and they are the reason a
large image is minutes rather than an hour.

* **`COEF_MAX`** — the candidate sweep samples at most 4096 pixels of a
  class instead of all of them, scaling the sum back up before the side
  cost is added. It is choosing the argmin of 33 numbers; four thousand
  samples decide that as well as thirty thousand do. **coef 6 s → 2 s**,
  and on t24+t32 it is not merely free but *smaller* than the exact
  version (208091 against 208595): the classes are uneven, so the large
  ones get sampled, and the sampling breaks ties the exact sum does not.
* **`MIN_GAIN`** — a loop stops once an iteration improves the cost by
  less than 1/4096 of it. MRP runs a fixed count; past a point the
  iterations are buying hundredths of a percent at a minute each.

Set `-DCOEF_MAX=0 -DMIN_GAIN=100000000` for the exact search.

### Two things the transmitted model was leaving on the table

Both come from the same observation: the generalized-Gaussian family is a
*prior*, and a two-pass coder has no reason to stop at a prior when it can
measure the thing and send the difference.

**σ was quantised to the group it landed in.** Each activity group took
its σ from the ladder rung and chose only the *shape*; the ladder steps by
×1.38, so the rung can be 16% away from what the residuals in that group
actually want. `NUM_SIGV=3` offers ±17.5% around the rung and codes the
choice alongside the shape — two extra bits per (component, group), 96 in
all. The whole price is the selection pass, which reads one cost table per
candidate per (pixel, component), so it runs in two stages: sixteen shapes
at the rung's own σ, then the three σ of whichever shape won. 19 reads
instead of 48, and the exhaustive form is worth another 0.35% for 3% more
time (`-DSIGV_2STAGE=0`) — inside the noise band described below.

**And the family is still only a family.** After the loops settle,
`MeasurePmFix` compares, per (component, group), how many residuals landed
in each of fifteen signed log-magnitude buckets with how many the chosen
member of the family expected to, and transmits the ratio quantised to
eighths of an octave. The window is `[base, base+MAXVAL]` and `base` *is*
the prediction, so table index *i* is residual *i*−`MAXVAL` whatever
`base` happens to be — which is what makes the correction a function of
the index alone, and so something to bake into the frequency table once
rather than evaluate per symbol. The rebuild is integer throughout and
reads only the transmitted levels, so both sides land on the same
frequencies; the thresholds are then re-run against the corrected costs so
the group boundaries agree with what is coded.

It costs `nc`·16·15 small numbers of side information, zig-zagged so that
"no correction" is the cheapest symbol, and it is gated on measured gain —
if it does not pay for itself it comes back out and the whole thing costs
one bit. **−1.46% across the corpus** (t32 alone −3.5%), which is more
than the residuals-are-not-quite-Gaussian story deserved.

One caveat both of these inherit. The decoder does not receive the
frequency tables, it *rebuilds* them — `set_freqtable` calls `exp`, `pow`
and `lngammaf`, and the results pass through truncations like
`uint(norm·pdf)+MIN_FREQ`. A last-ulp difference in `pow` between C
runtimes can flip one truncation, change one `freq`, and desynchronise the
whole stream. mrpc is built with `-Ofast`, which is exactly the setting
that makes this possible. Measured on this platform: clang and g++ encode
`t24` to the same 96368 bytes, and each decodes the other's stream
correctly — so the hazard is not currently biting. It is still a hazard,
and the fix is to build those tables from fixed-point or polynomial
constructions the way `sh_common.inc`'s `fexp2f`/`flog2f` already do for
the hot path — speed is irrelevant there, it runs once, only bit-identity
matters. Until then, treat a decode on a *different* C runtime as untested
rather than guaranteed.

### The iteration count is a compression parameter, not a tolerance

The first loop alternates a weighted-least-squares fit with a class
reassignment. The fit minimises squared error; the thing being measured is
code length. They are not the same objective, so the loop does not
descend — on a 320×240 image it went

```
135K  122K  147K  140K  147K  132K bytes
```

Only the best iterate is kept, and the second loop starts from it, so the
whole file rides on which iterate happened to land low. Two consequences,
and the second is the more useful one.

**More iterations are worth real bytes**, because they are more samples of
a noisy process rather than further steps of a descent. Five images,
encode time for all five:

| `MRP_EFFORT` | `MAX_ITER`/`EXTRA_ITER` | bytes | time |
|---|---|---|---|
| 0 | 12 / 3 | | |
| 1 | 20 / 4 | 325919 | 55.5 s |
| | 24 / 6 | 322181 | 63.2 s |
| | 40 / 10 | 308019 | 93.8 s |
| 2 | 60 / 20 | **302590** | 139.3 s |

7.2% for 2.5× the time. The default stays at 1 because the complaint that
started the speed work was about an 8 MB image taking an hour; `-DMRP_EFFORT=2`
is there for when the bytes matter more than the wall clock. Decode time
does not move with it at all.

**And small deltas cannot be read off a single encode.** The border
consistency fix — refreshing `errB`'s margins during optimisation so the
optimiser's activity is the one `CodeImage` actually codes with — is
correct, and it measured 2.6% *worse* across the corpus. Two `UPEL_DIST`
columns out of 320 cannot move a file 2.6%; what moved was which iterate
of the first loop won. It is `-DBORDER_FIX=1`, off, with the number
written next to it, and the same is true of `-DINIT_METRIC=1`. Anything
claiming less than a percent needs either the whole corpus or a frozen
optimiser to be believed.

## 6. Watching it work

A two-pass coder that says nothing for an hour is indistinguishable from a
hung one, so any image big enough to take a while (`MRP_PROGMIN` pixels,
default 200k) reports on stderr as it goes:

```
mrpc: 4096x512x4  63 classes  taps 20+3*6+cur
mrpc: order trial ........................ order 3 2 0 1  (14.1s)
mrpc: -DMRP_CLASS=n and -DMAX_ITER=n are the time dials; ...
mrpc: [1: 0] fit 5s group 1s class........ 17s = 2868374 B  [38s] *
mrpc: [1: 1] fit 5s group 1s class........ 17s = 2826921 B  [61s] *
mrpc: [1: 2] fit 5s group 1s class........ 17s = 2822063 B  [84s] *
mrpc: [1: 3] fit 5s group 1s class........ 17s = 2871484 B  [107s]
```

`[loop:iteration]`, then each phase with its own time, then the code
length the iteration reached and the total elapsed; `*` marks an
iteration that improved on the best so far, which is the one that will be
kept. The dots inside `class` are eighths of the class search, the long
pole — it is the phase that predicts every pixel of every block under
every class. `-DMRP_PROGRESS=2` reports on small images too, `=0` never.

The report is also how you decide whether to wait: the per-iteration time
is stable, so three lines tell you what the run will cost.

## 7. Parameters

All of them are `#define`s at the top of `mrpc.cpp`, overridable with
`-D`; `./mrpsweep.sh 'PRD_ORDER=30' 'XPRD_ORDER=12'` builds and totals a
corpus.

One caution learned the hard way: **crops mislead about the tap count.**
On the 96-row crop corpus `PRD_ORDER` wants to be 36 (112199 against
119835 at 20); on the full images it wants 20 (209483 against 218090 at
36). A crop has the same class count over a quarter of the pixels, so
every class is fitted from a quarter of the samples, and the point where
extra taps stop paying moves a long way. The shipped value is the
full-image one.

| | | |
|---|---|---|
| `PRD_ORDER` | 20 | causal taps on the component being predicted |
| `XPRD_ORDER` | 6 | causal taps on each other component |
| `XCUR` | 1 | tap the current pixel's earlier components |
| `XUPEL` | 1 | fold them into the activity measure too |
| `MRP_CLASS` | 0 | 0 = MRP's own `10.4e-5·pixels + 13.8` |
| `MRP_GROUP` | 16 | activity groups |
| `NUM_PMODEL` / `PM_ACC` | 16 / 3 | shapes, and bits of the fraction |
| `NUM_SIGV` | 3 | σ variants per group offered beside the shape |
| `PMFIX` | 1 | transmit a measured correction to each group's pmf |
| `MRP_EFFORT` | 1 | 0 fast, 1 default, 2 maximum compression |
| `MAX_ITER` / `EXTRA_ITER` | from `MRP_EFFORT` | iterations of each loop |
| `OPT_PRED` | 1 | the coefficient search (16%, and most of the time) |
| `TRIAL_PIX` / `TRIAL_BANDS` | 262144 / 6 | what the order trial fits on |
| `COEF_MAX` | 4096 | pixels the candidate sweep samples (0 = all) |
| `MIN_GAIN` | 4096 | stop when an iteration gains under 1/this |
| `DP_SHRINK` | 1 | run the threshold trellis only as far as the data goes |
| `MRP_GATHER` | 0 | 1 = use AVX2 vgather; measured slower |
| `BORDER_FIX` / `INIT_METRIC` | 0 / 0 | tried, measured negative, kept as toggles |

`MRP_CLASS` is worth checking per image: MRP's formula gives 21 for these,
and both 12 (224508) and 32 (223258) are clearly worse than 21 (209483).

The component-order trial fits one global predictor per candidate order,
and there are `n_colors!` of them — 24 for RGBA. It runs on `TRIAL_PIX`
pixels spread over `TRIAL_BANDS` bands rather than the whole image: on the
8 MB image that is 14 s instead of minutes, and it picks the same order
(209483 either way on t24+t32). **One** band is not enough — a single
32-row stripe of a 4096-wide image chose an order worth 4.7% more.

`MRP_CLASS` is also the knob that sets the encode time, because both
expensive searches are linear in it: the class search predicts every pixel of a
block under every class, and the coefficient search runs once per (class,
tap). MRP's formula asks for 72 classes on a 705×800 image and gets the
63 cap, and an encode at that size runs in tens of minutes. `-DMRP_CLASS`
is the dial; decode time does not move with it.

## 8. Three bugs worth naming

All three were found the same way — by making the encoder and the decoder
print checksums of the state they had just agreed on, and seeing which one
diverged first.

* **The quadtree map has to be saved and restored with the class map it
  describes.** Keeping the best iteration's classes but the last
  iteration's tree emits flags for a segmentation that is no longer there,
  and the decoder dutifully fills blocks the encoder never coded.
* **The per-class prediction buffer belongs to the enclosing 32×32 block**,
  so a sub-block of the quadtree indexes it *modulo* the buffer size, not
  from its own origin. This one never broke the stream — the final pass
  recomputes everything from the coefficients — it just fed the class
  search another block's numbers. Worth 10%.
* **MRP threads a narrowed `width` through the quadtree recursion**, and it
  is not cosmetic: the bottom-right child of a split must not read the
  context cell that lies in the *next* parent, which the decoder has not
  reached yet. Read it and encoder and decoder disagree about the
  probability of a flag, and everything after it is noise.

## 9. The optimisation notes, item by item

`mrpc_optimization_1.md` lists ~30 ideas across speed and compression.
This is where each one stands. "Identical" means the output was checked
byte-for-byte against the build before the change — the standard for
anything claiming to be a pure speed win.

**Landed, output identical**

| | | |
|---|---|---|
| 4.1 | fit from the class lists, not a class scan | |
| 4.2 | branch-and-bound `FindClass`, and return the winner's cost | |
| 4.7 | stable merge sort in `InitClass` | |
| 3.1 | the coefficient sweep threaded over (class, component) | |
| 3.2 | `SetPrdBuf` threaded over classes | |
| 3.3 | `FitPredictor` threaded over (class, component) | |
| 3.4 | the group histogram, the pmodel selection and the trellis | |
| 3.5 | `PredictRegion` over rows | |

3.4 and 3.5 are the ones in this round. The histogram and selection passes
went *component-outer* rather than pixel-outer, which is what makes them
both threadable and bit-identical: every bin they touch is indexed by `k`,
so the components own disjoint memory and each bin still receives its
pixels in raster order. The activity gets recomputed per component instead
of once for all of them — `nc` times the work spread over `nc` threads,
so the same wall time for that part, while the sixteen-group loop that
actually dominates goes `nc`-wide. Verified identical at 1, 2 and 4
threads.

**Landed, output moves — and measured**

| | | |
|---|---|---|
| 7.1 | σ variants beside the shape | −0.3% |
| 7.2 | transmitted pmf correction | **−1.46%** |
| 5.3 | trellis shrunk to the observed activity range | faster, and smaller side info |
| 7.10 | more iterations, as `MRP_EFFORT` | −7.2% at effort 2 |
| 4.6 | two-stage candidate sampling, applied to 7.1's grid | 19 table reads instead of 48 |

**Tried, measured negative, kept as documented toggles**

`BORDER_FIX` (7.7) and `INIT_METRIC` (7.9). 7.7 is the interesting one and
it is discussed above: it is *correct* and it costs 2.6%, which is how the
noise floor of these measurements got established in the first place.

**Not attempted, and why**

* **4.3, `ChooseOrder` without the per-trial `LoadOrg`.** The plane side is
  easy — `BuildTaps` already separates "which plane" from "which offset" —
  but the interleaved side is not: `CalcCost` reads `p[k]`, the activity
  reads `e[uoff+k]`, and the scalar `Predict` indexes `org` directly. Every
  one of those would need the permutation threaded through it, in the
  decoder's hot loop among others. Real win, invasive change.
* **4.4 int16 cost tables, 4.5 `madd_epi16` over 16 pixels, 5.1 `CalcCost`
  with error planes, 5.2 the histogram's table layout, 5.4 sweep
  interleave, 5.5 AVX-512.** Speed only, and after 7.10 the encoder's time
  is dominated by *how many* iterations it runs rather than the cost of
  one. These are the right next things if effort 2 is to become the
  default.
* **3.2's tile-based parallel `FindClass`.** The class walk is sequential
  by construction — each block's MTF context is the blocks before it — so
  this needs a tiling whose determinism argument is more than "fixed
  partition", and it is the largest remaining parallel fraction.
* **6 and 4.8, the decoder's reciprocal tables and `DecSym` bucket index.**
  Decode is already 0.6 s on the 8 MB image against bmpc's 43.9 s. Nothing
  here is complaining.
* **7.3 truly adaptive final pass.** The notes say do 7.2 first; 7.2 is
  done, and 7.3 would desynchronise the optimiser's cost model from the
  coder to buy the same thing again.
* **7.4 intercept term.** One virtual tap ≡ 1 per (class, component).
  Cheap in principle, but the sweep applies coefficient moves
  incrementally as `prd += p[o1]·i + p[o2]·j`, so a tap whose value is not
  read from the image needs a special case in `Predict`, `Predict8`,
  `FitPredictor` and `OptimizeCoef` — four hot loops for an estimated
  0–0.3%.
* **7.5 error-feedback taps.** The notes flag it themselves: error taps'
  values change when coefficients change, which breaks the sweep's
  incremental update.
* **7.6 side-information modelling.** On the images here the side
  information is 3.4 KB of a 96 KB file; worth revisiting on small images.
* **7.8 deterministic table construction.** Measured rather than fixed —
  see the note above. The hazard is real, it is not currently biting on
  this platform, and the fix is mechanical when cross-runtime decode has to
  be guaranteed.
