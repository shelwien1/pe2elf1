# How mrpc works

mrpc is a lossless image codec on the scheme of MRP (Minimum Rate
Predictors, Matsuda et al.). It is a *two-pass* coder: the encoder spends
most of its time searching for a model, transmits that model, and then codes
the image under it in a single raster pass. The decoder does no searching at
all -- it reads the model and replays the raster pass.

This document describes the algorithm. It does not describe the rangecoder;
for present purposes that is a black box that codes a symbol against a
frequency table at its exact information content (measured loss: 7e-6 bits a
symbol -- see `ENTROPY.md`).

---

## 1. The shape of the thing

Every pixel is coded as a symbol drawn from a probability distribution. The
whole codec exists to make that distribution as sharp as possible, and it has
exactly three levers:

1. **A linear predictor** guesses the pixel from its causal neighbours. The
   distribution is centred on that guess.
2. **A class** says *which* predictor. The image is segmented, and each class
   carries its own predictor per colour component.
3. **An activity group** says how *wide* the distribution is. Local activity
   -- a weighted sum of neighbouring errors -- is quantised into groups, and
   each group carries a scale and a shape.

None of the three is derived by the decoder. All three are searched for by
the encoder and written into the stream:

```
  header    version, geometry, head/tail lengths
  params    class count, component order
  class map quadtree flags + a class per leaf
  coef      one linear predictor per (class, component)
  thresh    activity thresholds per (class, component), model index per group
  image     one symbol per pixel per component
```

## 2. Geometry and borders

The raster is copied into `org`, a buffer padded by `PADL = PADR = PADT =
UPEL_DIST+2 = 5` cells on the left, right and top, with row stride
`BS = W + PADL + PADR`. There is no bottom padding: nothing causal reaches
downwards.

Border cells are not clamped at read time. They are *materialised*, because
the optimiser recomputes predictions out of raster order and a cell must not
mean one thing during row *y* and another after it. The fill is edge
replication with a lag:

```
  left  (y, -j)      := pixel (y-1, 0)        j = 1..PADL
  right (y, W-1+j)   := pixel (y,   W-1)
  top   (-j, x)      := pixel (0, x-PADR-1)
```

The lag on the top row is what makes this work during the coding pass: the
earliest reader of cell `(-j, x)` is pixel `(0, x-PADR)`, which is coded
after `(0, x-PADR-1)`. So every border cell is filled from a pixel that is
already known when it is first read, in both directions. Cells with no
source (above and left of the origin) hold 128 with zero error.

For the encoder's inner loops `org` is also de-interleaved into `nc` separate
planes, so a tap on component *j* is a contiguous byte array indexed by a
plane offset rather than a stride-`nc` walk. The decoder does not build them
-- it predicts one pixel at a time and has nothing to batch.

## 3. The predictor

`DYX[]` is MRP's causal neighbourhood in distance order:

```
  (0,-1) (-1,0) (0,-2) (-1,-1) (-2,0) (-1,1) (0,-3) (-1,-2) ... 42 entries
```

A predictor for component *k* of class *c* is a dot product over three groups
of taps:

| taps | count | what they read |
| --- | --- | --- |
| same component | `PRD_ORDER` = 20 | `DYX[0..19]` of component *k* |
| cross component | `XPRD_ORDER` = 6 | `DYX[0..5]` of every *other* component |
| current pixel | `k` | components of *this* pixel already coded |

so `nt[k] = 20 + (nc-1)*6 + k`, and `NTMAX = 20 + 3*6 + 4 = 42` bounds it.
The third group is what makes a colour image cheaper than three grey ones:
when coding the third component of a pixel, the first two are already known
exactly, and offset 0 is a tap like any other.

Coefficients are integers in `COEF_PREC = 6` bits of fraction, clamped to
`MAX_COEF = ±2.0`. The prediction is

```
  v = clamp( sum_i org[o + toff[k][i]] * coef[c][k][i],  0, 255<<6 )
```

kept at `<<6` precision. `PRD_ORDER` above 30 is a build error: `DYX[30] =
(0,-6)` reaches further left than `PADL`.

### The error the model sees

Not the residual. MRP's `econv`:

```
  e = 2*org - (v >> 5);   err = e > 0 ? e-1 : -e
```

which is the doubled absolute error with a twist at zero, and is what the
activity measure and the group quantiser are calibrated on.

## 4. The probability model

Given a prediction `v` and a group `g`, the symbol is coded from a shifted
generalized Gaussian.

* `q = Qprd(v) = (255<<6 - v + 4) >> 3` is the prediction reversed and kept
  to `PM_ACC = 3` bits of fraction. `base = q>>3` is its integer part and
  `sub = q&7` its fraction.
* The table is `PMSIZE = 511` entries wide -- the full range of
  `pixel - prediction` -- and there are `NUM_SUBPM = 8` copies of it, one per
  sub-pixel position, so the distribution is positioned to 1/8 of a level.
* The symbol coded is `s = base + pixel`, over the window `[base, base+256)`,
  which is exactly the 256 values the pixel can take given `base`. The
  rangecoder is given `cumfreq[hi] - cumfreq[lo]` as the total, so the
  conditioning on the window is exact.
* The density is `exp(-(beta*x)^shape)` with `beta` set from `shape` and a
  scale `sigma`, sampled at 8 points per level and box-filtered by two, then
  quantised to a `2^20` total with `MIN_FREQ = 1`.

`sigma` comes from a fixed 16-step geometric ladder, `SIGMA[16]`, 0.15 to
30.89. **It is the only source of scale in the codec and it is not a free
parameter**: the threshold search chooses which activities land in which
group, but not what scale that group has.

`shape` and a tail weight are chosen per `(component, group)` and
transmitted, out of a family of `NUM_SHAPE * NUM_TAIL = 16 * 5` members. The
tail weight mixes a little uniform mass under the curve:

```
  freq[i] = (1-w) * norm * pdf[i] + w * budget/511 + MIN_FREQ
```

A generalized Gaussian's single shape parameter sets its peak and its tail
together; the tail weight unties them. It is the one parameter added to MRP's
model here; `TUNING.md` has what it is worth and why the others were not.

### Activity

```
  u = ( sum_{i<12} err[uoff[i] + k] * uw[i]  +  64 * sum_{j<k} err[j] ) >> 6
  u = min(u, MAX_UPARA = 512)
```

The first sum is over the 12 nearest causal neighbours weighted by `64/dist`.
The second is the errors already made on *this* pixel's earlier components --
the same idea as the current-pixel predictor taps, applied to the width
rather than the centre. `u` is then mapped to a group by `uq[c][k][u]`, a
lookup table built from the transmitted thresholds.

Every one of these reads is of a value the decoder already has.

## 5. What the encoder searches for

### Component order

Before anything else, `ChooseOrder` tries all `nc!` orderings of the colour
components (a Heap's-algorithm walk) and keeps the cheapest. Because the
cross-component and current-pixel taps are causal in *coding* order, the
order matters: whichever component is coded first has no cross taps at all.

The trial fits one predictor with no classes on a subset of rows --
`TRIAL_PIX = 262144` pixels spread over `TRIAL_BANDS = 6` bands rather than
one contiguous stripe, because one stripe of a 4096-wide image can be nothing
like the rest of it.

### The initial class map

`InitClass` sorts the `BASE_BSIZE = 8` blocks by variance and deals them into
`num_class` equal buckets in rank order. So the initial segmentation is "by
busyness", which is a reasonable prior for "wants the same predictor".

### The two loops

```
  loop 1 (MAX_ITER times, stop after EXTRA_ITER without gain):
      DesignPredictor    least squares per (class, component)
      OptimizeGroup      histogram + DP over the activity thresholds
      OptimizeClass      reassign every 8x8 block to its best class

  loop 2 (same stopping rule):
      OptimizePredictor  pairwise coefficient refinement
      OptimizeGroup      as above, plus the shape/tail choice per group
      OptimizeClass      as above, but 32x32 blocks over a quadtree
```

The state after every iteration is scored and the best is kept (`SAVE`/
`LOAD`); the quadtree map travels with the class map it describes, since
restoring one without the other would emit flags for a segmentation that is
no longer there.

Loop 1 scores the image cost only. Loop 2 adds the side cost -- coefficients,
thresholds, class map -- because that is when the segmentation gets fine
enough for it to matter.

### DesignPredictor -- least squares

For each `(class, component)`, accumulate the normal equations over the
pixels of that class and solve by Gauss-Jordan with partial pivoting. Samples
are weighted by `1/sigma[group]^2` (heteroscedastic least squares: a sample
from a noisy neighbourhood should not drag the predictor as hard as one from
a smooth one) except on the very first iteration, where the groups do not
mean anything yet and the fit is unweighted.

The real solution is then quantised to `COEF_PREC` bits by flooring, and the
accumulated rounding is handed back to the taps that lost the most of it --
largest-remainder apportionment, because a predictor whose coefficients sum
low darkens the whole class.

### OptimizeGroup -- the threshold DP

For every pixel this accumulates, into `cbuf[class][component][group][u]`,
the cost of coding that pixel if its activity `u` were assigned to `group`.
Then, per `(class, component)`:

* prefix-sum over `u`, so the cost of an activity *range* under a group is
  two lookups;
* a 16-stage shortest path over the `MAX_UPARA+2 = 514` possible threshold
  positions, minimising coded length plus the side cost of the thresholds
  themselves (`th_cost`, a geometric model over the gaps);
* traceback gives `th[class][component][0..15]`, and `uq` is rebuilt from it.

In loop 2 it also picks the shape and tail per `(component, group)` by
accumulating, for every pixel, what each of the `NUM_PMODEL` family members
would have charged, and taking the argmin.

### OptimizeClass -- the segmentation

Loop 1: for each `BASE_BSIZE = 8` block, try every class and take the cheapest,
counting `class_cost[mtf[c]]` -- the cost of *saying* the class. No quadtree.

Loop 2: blocks are `MAX_BSIZE = 32` and each is the root of a `QT_DEPTH = 4`
quadtree down to `MIN_BSIZE = 2`. `VbsClass` recurses:

```
  cost1 = cost of this block under its best single class + a "no split" flag
  cost2 = a "split" flag + the four children, recursively
  take the cheaper; record the choice in qtmap[level]
```

The recursion has to restore the move-to-front state before trying the split
branch, because the two branches see different class-code costs.

### OptimizePredictor -- pairwise refinement

Least squares minimises squared error; the codec pays *code length*. So loop
2 refines the coefficients against the real objective, but only in a way that
keeps the search tractable: pick two taps `p1, p2` and try an 11x3 grid of
shifts that move weight from one to the other, keeping their sum. Keeping the
sum is what preserves the predictor's DC gain while it searches.

Which pairs: on pass *n*, tap *t* is paired with `(t + 1 + n mod (nt-1))`, so
successive passes cover different pairings. Cost is evaluated over the class's
own pixel list, subsampled to at most `COEF_MAX = 4096` pixels and scaled back
up, with the side cost of the two coefficients added in absolute bits.

## 6. What goes in the stream

Everything below `params` is coded with `SPMod`, a parametric substitute for
an adaptive model: `freq[i] = 1024 * exp(-i/2^(m mod 8))`, with a variant for
`m >= 8` that gives symbol 0 all the mass the rest do not use. So sixteen
distributions -- eight decay rates, each with and without a spike at zero --
and the encoder tries all sixteen, keeps the cheapest, and sends `m` first.
There is nothing adaptive anywhere in the side information; the encoder knows
the whole histogram before it writes a bit, so it just names the model.

**Class count and component order** (`CodeParams`) -- flat over
`MRP_MAXCLASS+1` and `nc` respectively. Note this makes `MRP_MAXCLASS` part
of the format.

**Class map** (`CodeClass`). Walked in the same order the decoder will walk
it. Each quadtree node emits a split flag under a context built from whether
its three already-coded neighbours split (`QtCtx`: up, up-right, left, plus
the level), with one of seven quantised probabilities per context, chosen by
the encoder and sent.

Each leaf emits a class index, *move-to-front coded*. `MtfClass` promotes the
classes of three neighbours -- above, left, and above-right-by-one-block --
to the front of the list before the index is taken, so a block that agrees
with its neighbours codes a small number. The frequencies for those numbers
come from `qhist`, the histogram of the codes actually used, quantised to one
of `PMCLASS_LEVEL = 32` log-probability levels and transmitted.

`class_cost[]`, the per-code cost that the class search minimises against, is
read straight back out of that quantised model -- so the search is scoring
against the code it will actually pay.

**Coefficients** (`CodePredictor`). For each `(component, tap)`, all
`num_class` values are coded together with one geometric parameter chosen for
that tap, magnitude first and a raw sign bit if non-zero. Grouping by tap
rather than by class is what makes it cheap: tap 0 is large in every class,
tap 30 is near zero in every class, and one parameter fits each.

**Thresholds** (`CodeThreshold`). Coded as *gaps* between successive
thresholds, again with one geometric parameter for the whole image. Then the
model index per `(component, group)`, flat over `NUM_PMODEL`.

**The image** (`CodeImage`). One raster pass. Per pixel per component:
predict, compute activity from the errors so far, look up the group, look up
the table, code the symbol, store the error. This is the only part the
decoder also runs, and it is identical code with `dec` flipped -- which is
the property that keeps the two in step.

**Row padding and head/tail blobs** are the exception: they are coded with an
adaptive order-1 byte model -- a binary tree of eight adaptive bit
probabilities per context, the context being the previous byte. Nothing is
known about these bytes in advance, so adaptation is all there is. Padding is
not part of the raster but is rarely zero, and a file format's header and
trailer ride along so the caller gets its bytes back exactly.

The stream's own header (version, geometry, blob lengths) goes through the
same machinery with its own contexts, because the geometry has to come out
before there is anything allocated to hold it. Lengths are coded seven bits
at a time with a continuation bit, so a 37-pixel-wide image does not pay four
bytes to say so.

## 7. The decoder

```
  CodeParams(1)       class count, component order
  DecodeClassMap()    quadtree + MTF class per leaf
  DecodePredictor()   coefficients
  DecodeThreshold()   thresholds -> uq, and the model index per group
  SetPmodels()        rebuild only the tables the stream named
  CodeImage(1)        the raster pass
```

There is no search, no optimisation and no OpenCL. It is O(pixels x taps),
about 276 ns a pixel-component on a 2.1 GHz core, and single-threaded by
construction -- pixel *n*'s prediction and activity both depend on pixel
*n-1*'s decoded value.

The decoder builds one probability table per `(group, sub)` rather than the
encoder's `16 x 80 x 8`, since the stream tells it which family member each
group uses.

## 8. What runs on a device

The encoder's three searches are what the OpenCL path parallelises: the class
search (both loops), the coefficient sweep, and the group histogram and
threshold DP. Nothing that writes to the bitstream uses a device -- a decoder
that needs one to read a file is not a decoder. `README.md` has the details;
the short version is that the per-pixel costs are computed once for all
classes and the quadtree reads rectangle sums out of a prefix sum of them,
which is a fifth of the arithmetic before any of it is made parallel.

## 9. Fallbacks

`ncomp` outside 1..4, a zero dimension, or a stride below `width*ncomp` is
refused. A caller with a file it could not parse as an image passes no image
at all, and the whole thing is coded as head and tail with the order-1 model.
One component is a grey image or a field of palette indices; the codec
predicts whatever the bytes are, which is lossless either way, though indices
into an arbitrary palette are labels rather than magnitudes and there is not
much for a linear predictor to find in them.

## 10. Constants

| | | |
| --- | --- | --- |
| `PRD_ORDER` | 20 | same-component taps (max 30: `PADL`) |
| `XPRD_ORDER` | 6 | cross-component taps, each other component |
| `XCUR` | 1 | also tap this pixel's earlier components |
| `COEF_PREC` | 6 | fraction bits in a coefficient; range ±2.0 |
| `NUM_SHAPE` x `NUM_TAIL` | 16 x 5 | the model family |
| `PM_ACC` | 3 | sub-pixel positioning of the pmf |
| `MRP_GROUP` | 16 | activity groups |
| `SIGMA_LO/HI` | 0.15 / 30.89 | the scale ladder |
| `MAX_UPARA` | 512 | activity is capped here |
| `UPEL_DIST` | 3 | -> 12 activity neighbours, and 5 cells of padding |
| `MAX_BSIZE` / `MIN_BSIZE` | 32 / 2 | quadtree root and leaf |
| `BASE_BSIZE` | 8 | loop 1's flat block |
| `QT_DEPTH` | 4 | quadtree levels |
| `MRP_MAXCLASS` | 63 | class cap; part of the format |
| `MAX_ITER` / `EXTRA_ITER` | 20 / 4 | iterations, and patience |
| `MAX_TOTFREQ` | 2^20 | frequency table total |

`PRD_ORDER` and `XPRD_ORDER` are checked at compile time against the padding:
`DYX[30]` is `(0,-6)` and `DYX[36]` is `(-6,0)`, both of which reach outside
a 5-cell border, so anything above 30 is a static assertion rather than a
silent out-of-bounds read.

## 11. Where to read what

| | |
| --- | --- |
| `mrpc_lib.h` | the C API |
| `mrpc_lib.cpp` | everything above |
| `mrpc_cl.inc` | the OpenCL host layer |
| `mrpc_kernels.inc` | the kernels |
| `sh_v2f.inc` | the rangecoder |
| `README.md` | the device port |
| `ENTROPY.md` | where the bits go, measured |
| `TUNING.md` | the class count and the model family, measured |
