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
| t24.bmp | 320×240×24 | 230454 | **97984** | 100130 | +2.2% |
| t32.bmp | 320×240×32 | 307254 | 115062 | **109353** | −5.0% |
| t8g / t8p | 8bpp | 77878 | 46941 / 46589 | 53452 / 53385 | fallback |

mrpc wins on 32bpp and loses slightly on 24bpp. That split is the two
designs showing their shapes: bmpc's strength is its residual coder, seven
context models mixed per bit, and t24 is photographic where that pays;
mrpc's strength is the predictor, and t32 has the structure a per-region
linear fit can actually capture.

**Encode time is the price, and decode time is the prize.** On t24:

| | encode | decode |
|---|---|---|
| bmpc | 2.9 s | 1.5 s |
| mrpc | 36 s | **0.06 s** |

Encoding is 12× slower because the coefficient search rescans every pixel
of a class for each candidate pair, and the class search predicts every
pixel of a block under every class. Decoding is **25× faster** for exactly
the reason the models are static: a decoded pixel costs one dot product
and one rangecoder step, and there is no adaptation to replay. bmpc's
decoder has to redo every update its encoder did; mrpc's does not.

## 6. Parameters

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
| `MAX_ITER` / `EXTRA_ITER` | 20 / 4 | iterations of each loop |
| `OPT_PRED` | 1 | the coefficient search (16%, and most of the time) |

`MRP_CLASS` is worth checking per image: MRP's formula gives 21 for these,
and both 12 (224508) and 32 (223258) are clearly worse than 21 (209483).

## 7. Three bugs worth naming

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
