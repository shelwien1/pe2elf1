# mrpc — compression improvements for the model

Where mrpc's *model* could compress better, read against BMF v2.01 as a
comparison point. Scope is what is modelled and how predictions and
probabilities are formed — not the rangecoder, which `ENTROPY.md` settles: it
loses 1.7 bits over 230,400 symbols, 7e-6 bits a symbol, and there is nothing
in it to win.

`ALGORITHM.md` describes what mrpc does today. `ALGORITHMS.md` and
`IMPROVEMENTS.md` (Shkarin's BMF, reconstructed) describe a codec built on
the opposite principle, which is why it is worth reading against.

**How claims are graded**, borrowing BMF's convention because it is the right
one:

| tag | meaning |
| --- | --- |
| **[measured]** | a number this session produced; the experiment is described |
| **[from the code]** | a structural fact, checkable by reading the named function |
| **[expected]** | a judgement from how comparable coders behave; no number here |

> **§1-§13 are the proposal, written before any of it was built. §14 is what
> happened when it was.** Three items shipped and are worth **-2.259 % mean**
> on the corpus; three were measured and refuted, one of them the item this
> document called the largest expected gain. Where §14 disagrees with the
> section above it, §14 has the numbers.

---

## 0. First, a name collision that has to be cleared up

BMF's corpus and mrpc's share filenames — `t24`, `t8g`, `t8p`, `x_ep` — and
three of the four are **not the same images**. Anyone comparing the two
documents' tables will otherwise conclude mrpc is 77 % worse than BMF, which
is not a real result.

**[measured].** My `t24.bmp` is 320x240x24, 230,454 bytes, matching BMF's
`t24` exactly in geometry and raw size. But:

| | my `t24.bmp` | BMF's `t24` |
| --- | --- | --- |
| coded | 95,602 (mrpc) | 53,924 |
| bits per component | 3.32 | 1.87 |
| order-0 entropy, per component | 6.00 / 8.00 / 8.00 | — |
| left-difference entropy | 6.03 / 5.71 / 5.72 | — |
| `xz -9e` | 188,344 | — |

An image whose left-difference entropy is 5.7 bits and whose optimised
residual has mean magnitude 9.19 (from the entropy dump) cannot be coded at
1.87 bits a component; that would need a mean residual near 0.6. And BMF's
`t8g` costs 4.47 bpp where the same-sized grey version of my `t24` costs
5.75. Two different pictures that happen to be 320x240. `t8g`/`t8p` follow,
since both corpora derive them from their own `t24`.

**`x_ep` is the same file** — 705x800x32, raw 2,256,054, an unusual enough
geometry to be conclusive — and it is the one honest cross-codec number
available:

| | mrpc, host (`-C`) | mrpc, device | BMF |
| --- | --- | --- | --- |
| `x_ep`, 705x800x32 | **321,332** | 322,454 | 328,476 (§9.1) / 330,660 (§1) |

mrpc is **2.2 % smaller** than the better of BMF's two published figures. Two
caveats on it: mrpc's number was measured before the tail axis of §10 and the
batched first loop, and BMF's own document reports `x_ep` at two different
sizes in two different tables. Neither moves the conclusion, which is only
that **the two codecs are in the same class** on the one file both have. Everything
below is about mrpc's own headroom, measured on mrpc, with BMF used to say
*where* to look rather than *how far*.

---

## 1. Baseline

Current build, 16-tile corpus (four 160x128 RGB tiles of `t24`, four 128x128
of `PIA13882`, and 128x96 grey and paletted tiles of each), all round-tripped:

| | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `t24_0` | 17,484 | `t24_1` | 25,513 | `t24_2` | 28,086 | `t24_3` | 32,777 |
| `pia_0` | 33,085 | `pia_1` | 32,084 | `pia_2` | 30,086 | `pia_3` | 27,892 |
| `t24g_0` | 9,249 | `t24g_1` | 9,284 | `t24p_0` | 8,059 | `t24p_1` | 8,454 |
| `piag_0` | 10,285 | `piag_1` | 9,741 | `piap_0` | 10,679 | `piap_1` | 9,987 |

Whole images: `t24.bmp` 95,602, `20000171A.bmp` 2,751,355, `PIA13882_crop256`
121,429.

---

## 2. The structural gap

**[from the code].** mrpc's probability model has, for the whole image:

```
  num_class x MRP_GROUP  =  63 x 16  =  1008 contexts
```

and every one of them is **transmitted**: the class map, the coefficients, the
thresholds, the model index per group. The decoder learns nothing.

BMF's alt-P1 has 32 x 3^9 = **629,856** contexts; alt-P2 has 1,088 NLMS filter
rows selected by a 1,920-value activity context, five banks of 32,768 SSE
contexts, and a 15,552-point residual lattice. It transmits **none** of them —
both sides derive identical statistics from reconstructed data.

That is the whole difference, and almost everything below is a consequence of
it. mrpc pays side information for every context it has, so it cannot afford
many; the two-pass search then spends its effort placing those few contexts
optimally. BMF pays nothing per context, so it has three orders of magnitude
more of them, and spends its effort on generalising between them.

The two are not in opposition. **The transmitted model and the learned model
compose**: a static parametric prediction refined by an adaptive correction is
strictly more general than either. That is what §3 and §4 are.

---

## 3. Adaptivity — the measured headroom

**Now [from the code].** Nothing in mrpc adapts while coding except the order-1
byte model on the head/tail blobs. Every pixel probability comes from a table
fixed before the first bit is written.

**[measured]**, from `ENTROPY.md`: mixing the static pmf with a model learned
online from symbols already coded, `q = (1-w)*p_static + w*p_adaptive` with `w`
adapted per context by one gradient step on the log loss — a decodable model,
since everything it depends on is already in the stream:

| image | bits/sym now | gain |
| --- | --- | --- |
| `t24_pal.bmp` | 3.755 | **+12.62 %** |
| `t24.bmp` | 3.316 | **+6.58 %** |
| `PIA13882_crop256_pal` | 5.311 | +2.87 % |
| `t24_gray.bmp` | 5.432 | +1.61 % |
| `20000171A.bmp` | 2.590 | +0.20 % |
| `PIA13882_crop256.bmp` | 4.846 | −0.06 % |

Three things that matter for what to build.

**The mixture beats both its parts.** The best possible *static* histogram at
the model's own context — plug-in conditional entropy plus an MDL penalty for
learning it — is +1.09 % on `t24.bmp`. The mixture gets +6.58 %, by using the
generalized Gaussian's smooth tails where counts are sparse and the histogram's
peak where they are not. This is the same argument BMF's §2 makes for a mixer
over selection, arrived at from the other side.

**It is not non-stationarity.** Halving the counts every 4,096 symbols adds
+0.04 % on `t24.bmp`. It is a mismatch between the residual distribution and
the parametric family, not drift.

**The gain tracks how badly the family fits.** `PIA13882` is quantised space
imagery whose residual really is generalized-Gaussian, and adaptivity buys
nothing. Palette indices are labels rather than magnitudes and it buys 12 %.

### The architectural obstacle, and the two ways past it

**[from the code].** `CalcCost`, the OpenCL cost cube, the quadtree prefix
sums, the coefficient sweep and the group DP all depend on one property: *a
symbol's cost does not depend on when it is coded*. That is what makes a
block's cost under a class a sum of independent per-pixel terms, and therefore
what makes the entire two-pass search — and its OpenCL port — possible. An
adaptive coder breaks it.

1. **Optimise static, code mixed.** The search is untouched; only the final
   coding pass mixes. The optimiser then minimises a slightly wrong objective.
   *Every number in the table above was measured this way*, so those are
   delivered gains, not upper bounds.
2. **Make the correction two-pass and transmit it.** Static, so the optimiser
   stays exact and can even optimise against it — but you pay side information
   and lose the local adaptation that is most of the gain.

Option 1 is the one to build, and it is the single largest item in this
document.

---

## 4. SSE / APM — the cheapest real win

**Now [from the code].** Nothing in mrpc maps a probability to a better
probability. BMF's five-bank cascade (`step_bank`) is the same idea applied one
stage earlier — it refines the predicted *value* before context selection, keyed
on an 11-bit over/undershoot pattern.

**Change.** Keep the static pmf and learn one multiplicative correction per
band of the residual: 32 bands, log-spaced in magnitude with sign.

```
  p(r)  ∝  p_static(r) * c[ctx][band(r)]
  c[ctx][b] = (observed[ctx][b] + A) / (static expected[ctx][b] + A)
```

32 numbers a context instead of 511, and the cumulative the decoder needs is
32 lookups into the `cumfreq` table it already has.

**[measured]:**

| image | 32-band APM | full adaptive | APM captures |
| --- | --- | --- | --- |
| `t24_pal.bmp` | **+10.54 %** | +12.62 % | 84 % |
| `PIA13882_crop256_pal` | **+3.47 %** | +2.87 % | 121 % |
| `t24.bmp` | **+3.24 %** | +6.58 % | 49 % |
| `t24_gray.bmp` | **+1.12 %** | +1.61 % | 70 % |
| `20000171A.bmp` | +0.15 % | +0.20 % | 75 % |
| `PIA13882_crop256` | −0.12 % | −0.06 % | — |

On one image the constrained version *beats* the full histogram: 32 parameters
generalise where 511 overfit. That is the standard argument for secondary
estimation, and it is the reason to build this one first — it is bounded below
at about a tenth of a percent, and it needs no binarisation, no mixer and no
change to the search.

**Cost.** ~32 table lookups and a normalisation per symbol, in both directions.
Decode is 276 ns a symbol today; call it 1.5-2x. The encoder's optimisation
loop does not change at all.

---

## 5. Adaptive prediction beside the transmitted one

**Now [from the code].** mrpc's predictor is a per-`(class, component)` linear
filter, fitted by weighted least squares, refined pairwise against code length,
quantised to `COEF_PREC = 6` bits and transmitted. It is fixed for the whole
image.

BMF's alt-P2 runs a 28-tap NLMS filter per pixel, with 1,088 weight sets
selected by an activity context, *two* adaptation rates blended by a running
covariance-over-variance confidence, and weight-vector (not prediction) mixing
across six neighbouring positions.

**The comparison is closer than it looks.** mrpc's classes are a quantised,
transmitted version of BMF's seated filter rows: both say "this region wants
that filter". mrpc's is chosen by rate-optimal segmentation and sent; BMF's is
chosen by an activity hash and learned. mrpc has 63 of them, BMF 1,088.

**Change [expected].** Keep the transmitted predictor as the centre and add a
small NLMS correction learned identically on both sides:

```
  v = clamp(Predict(k, p, coef[cl][k]))          the transmitted prediction
  v'= v + dot(w[ctx], features)                  the learned correction
  w[ctx] += rate * (actual - v') * features / (ms + floor)
```

with `ctx` a small activity/class context and `features` the same causal
neighbours the predictor already reads. Side information: none. This is the
prediction-domain twin of §4, and the reason to expect it to pay is the same:
the transmitted predictor is optimal *on average over the image* and the
correction is free to be local.

**Why this is the right shape for mrpc specifically.** The transmitted
predictor is exactly what an NLMS filter is bad at — a globally optimal
starting point that a per-pixel learner takes hundreds of samples to reach.
BMF seeds fresh rows from their neighbours at 0.78 gain for precisely this
reason. mrpc can seed from something better than a neighbour: the answer.

**Cost.** Linear in taps, both sides, on the hottest loop. This is the item
most likely to break the decode budget, and the one to size carefully — an
8-tap correction, not 28.

---

## 6. Coefficient precision bounds the predictor

**[measured].** Adding taps to mrpc makes it *worse*, and the reason is not the
tap count:

```
  t24.bmp, per (class, component) least squares
  base (32 taps)   RMSE unquantised 34.4999 -> quantised 34.5580   sum|b| 2.26
  x8   (36 taps)   RMSE unquantised 34.4792 -> quantised 34.8375   sum|b| 3.34
```

The extra cross-component taps buy 0.06 % of fit quality and cost 48 % more
coefficient L1 norm. Quantising to 1/64 with a ±2.0 clamp then costs 1.04 % of
RMSE instead of 0.17 % — six times more. Large cancelling coefficients do not
survive the quantiser. Widening `MAX_COEF` to ±8.0 recovers 40 % of the loss
(118,089 -> 111,183 at a fixed iteration count); `COEF_PREC = 8` alone is
catastrophic (240,981) because `set_spmodel` and the coefficient coder are tuned
for the current integer range.

**Change.** Retune the coefficient coder for 8-bit coefficients — the geometric
`SPMod` parameter search is already per-tap, so it is the *range* that needs
rescaling, not the model — and only then revisit tap count. This is an enabler,
not a gain in itself, and it gates both §5 (a correction with more taps) and any
future widening of the neighbourhood.

BMF has no equivalent problem: its filter weights are floats that are never
transmitted. This is a cost mrpc pays for being a two-pass codec, and the only
way to reduce it is to make the coefficients cheaper to send.

---

## 7. An orientation trial

**Now [from the code].** mrpc tries all `nc!` component orders (`ChooseOrder`)
and nothing else. BMF tries transposition, and `IMPROVEMENTS.md` §6 measures
2.4-16.6 % from extending it.

**[measured].** Transposing each corpus tile and re-encoding:

| | | | |
| --- | --- | --- | --- |
| `t24p_0` | **−5.82 %** | `t24_2` | −0.36 % |
| `t24_3` | **−3.94 %** | `piag_1` | −0.15 % |
| `pia_2` | −0.16 % | `t24_0` | **+28.77 %** |

As a **trial** — encode both, keep the smaller, one bit in the stream — it
cannot lose: **−0.69 % total, −0.73 % mean per image**, transposed smaller on
10 of 16, best single gain −5.82 %. Imposed rather than tried it would cost
28.77 % on one image.

**An honest caveat that does not apply to BMF.** BMF is a stable one-pass
coder, so its orientation gain is content: the image really does have
horizontal structure. mrpc's search is bimodal (`TUNING.md`), and `t24_0`
swings 30 % between adjacent class counts — so part of mrpc's transpose gain is
*variance reduction*, not orientation. Buying two lottery tickets and keeping
the better one is still a real gain in bytes, but it is a different mechanism,
and it means the number will not survive fixing §8.

**Cost.** 2x encode, nothing at decode. mrpc already has the trial machinery.

---

## 8. The instability is both a lever and a liability

**[measured].** mrpc's search is chaotic at a scale that swamps most of this
document:

* The same source compiled as one translation unit instead of two under
  `-march=native` codes `t24_gray.bmp` to 55,508 bytes instead of 55,576 — an
  ULP of floating-point contraction moves the optimiser somewhere else.
* Output against class count is **bimodal**, not smooth: on a 160x128 tile,
  12 and 14 classes give ~22 kB, 16 through 22 give ~17 kB, 24 through 28 give
  ~22 kB again.
* At 24 classes on that tile only **2 of 24** are ever used and the quadtree
  collapses to 2 leaves; at 20 classes it reaches 50 leaves.

The diagnosis is in `TUNING.md`: 24 classes x 34 taps over 20k pixels is 25
samples a coefficient, the per-class fits chase noise, the class search cannot
tell the classes apart, and the segmentation collapses. It is a small-image
problem — `20000171A.bmp` varies 0.27 % across a 3.7x range of class counts,
because at 2.1M pixels every class still has 850 samples a coefficient.

**Two ways to treat it, and they are different projects.**

*Exploit it.* Any trial that re-runs the search with a perturbation collects
part of the variance: the orientation trial above, a two-value class trial, a
different initial segmentation. Cheap, measured, and bounded by how much
variance there is.

*Fix it.* The collapse is a starved-fit problem, so the fix is to stop letting
classes starve — merge classes whose pixel count falls below a
samples-per-coefficient floor, rather than choosing the count up front. That is
the change `TUNING.md` could not make from outside: three different class-count
rules and a measuring trial were all worse on average than MRP's original,
because a rule that *moves* the count sometimes moves it across the boundary.
A rule that *removes starved classes during* the search never crosses it.

**[expected].** This is worth more than its own number suggests, because every
other item here is measured through the noise it creates.

---

## 9. What does not transfer, and what already exists

**Inter-plane decorrelation.** BMF chooses a plane coding order and per-plane
linear predictions from already-coded planes, with transmitted weights. mrpc's
cross-component taps do the same job *spatially* — `XPRD_ORDER = 6` taps on
each other component's causal neighbourhood, plus `XCUR` taps on this pixel's
already-coded components at offset 0. BMF's `IMPROVEMENTS.md` §8.1 identifies
its own version's limitation as "the transform sees one pixel; the model sees a
neighbourhood" and proposes making it spatial. **mrpc already is.** Nothing to
take here; if anything the traffic runs the other way.

**Two-pass transmitted parameters.** BMF's §12 proposes moving toward what mrpc
already is. The rate-optimal threshold DP and the rate-optimal quadtree
segmentation have no BMF equivalent.

**Palette index reordering [measured, and it lost badly].** BMF's §5 measures
1.5 % from modelling the palette itself, which mrpc could also do — it stores
the palette raw, in the head blob, through an order-1 model. The *reordering*
half was tested here and went the wrong way. Sorting a median-cut palette by
luma, so that neighbouring indices are neighbouring colours and the index
field becomes a magnitude:

| | as produced | sorted by luma | |
| --- | --- | --- | --- |
| `t24_pal.bmp` | 45,293 | 57,931 | **+27.9 %** |
| `PIA13882_crop256_pal.bmp` | 46,785 | 48,251 | **+3.1 %** |

Both worse, one of them badly. The intuition — that a predictor wants indices
to be magnitudes — is right about the *predictor* and wrong about the
*codec*: median cut already groups similar colours at adjacent indices within
each split, and sorting globally by luma destroys that local structure to
impose a one-dimensional order on three-dimensional data. If palette ordering
is ever revisited it should be as a **trial** with one bit in the stream, not
as an imposed transform, and the ordering to try is one that minimises coded
size rather than one that looks tidy.

**A run / skip mode.** BMF's §10.2 proposes one for its alt models; mrpc has
none either, and codes every pixel through the full machinery. On a plane where
one value covers most of the area this is pure waste. **[expected]**, untested
here, and cheap — but it is a bitstream change and it interacts with the
activity measure, which has no notion of "nothing happened".

**Long-range structure.** Neither codec has any. BMF measured four long taps as
a *loss* on its corpus (0.15-0.45 % worse on the two files that carry the bits)
for the same reason more taps hurt mrpc: more things for every filter row to
learn from the few hundred samples it sees. mrpc's version of that finding is
§6 — the cost is quantisation rather than sample starvation, but the shape is
the same. A transmitted vertical period is the cheap idea in both codecs and is
untested in both.

---

## 10. What is already settled and should not be proposed again

**[measured]**, all from `ENTROPY.md` and `TUNING.md`:

| | |
| --- | --- |
| a more precise rangecoder | 0.0002 % — it is exact |
| bigger frequency tables (2^22, 2^24, 2^26) | 0.001-0.09 %; the variation is search noise |
| mixing neighbouring sigmas | −0.10 % to +0.94 % |
| mixing all 16 shapes at a group | −0.69 % to +0.44 % |
| freeing sigma and shape per group | −0.85 % (worse) |
| a shift (bias) per group | −0.45 % — the residual is conditionally unbiased |
| an asymmetric scale per group | −0.72 % — it is symmetric |
| a ridge term on the least-squares fit | worse at every lambda over five orders of magnitude |
| capping the fit weighting ratio | noise |
| raising `MRP_MAXCLASS` past 63 | 1.7 % on one image inside a 13 % swing; nothing on the rest |
| any border fill other than edge replication | twelve tried, all worse, in proportion to how far they depart from the edge value |

### Borders: replication is right, and it is a local optimum in every direction

**[measured].** The border cells a predictor reads off the edge are currently
edge replication with a lag (`ALGORITHM.md` §2). Twelve alternatives were
costed against it — mirrors, gradients, constants, decays, and three that
change only the *error* plane — each on the same image with the model held
fixed (`tools/border_probe.patch`).

Every one of them loses. Ordered by how much:

| rule | `t24n0` 32x240 | `t24n2` 32x240 | `pian0` 32x256 | `bign0` 32x512 | `t24.bmp` |
| --- | --- | --- | --- | --- | --- |
| **0 replicate the edge** | **0** | **0** | **0** | **0** | **0** |
| 2 shear the top border | +0.04 % | −0.09 % | −0.01 % | +0.03 % | +0.00 % |
| 12 point-reflect the right edge | +0.84 % | +0.19 % | +0.03 % | +0.20 % | +0.05 % |
| 14 border errors halved | +0.88 % | +0.25 % | +0.05 % | +0.20 % | +0.05 % |
| 1 mirror the left and right edges | +1.36 % | +1.42 % | +0.18 % | −0.03 % | +0.05 % |
| 5 mirror left, point-reflect right | +1.72 % | +1.17 % | +0.18 % | +0.09 % | +0.06 % |
| 13 border errors zeroed | +1.72 % | +1.20 % | +0.19 % | +0.13 % | +0.08 % |
| 15 border errors doubled | +1.76 % | +1.19 % | +0.19 % | +0.13 % | +0.06 % |
| 11 decay to the plane mean | +5.98 % | +4.37 % | +0.18 % | +0.82 % | +0.48 % |
| 9 flat mid-grey (128) | +10.05 % | +7.96 % | +0.33 % | +23.63 % | +1.11 % |
| 10 decay to mid-grey | +14.38 % | +8.57 % | +0.12 % | +23.79 % | +0.84 % |
| **8 all zero** | **+23.90 %** | **+29.48 %** | +3.53 % | **+79.22 %** | +1.91 % |

The ranking is a monotone function of exactly one quantity: **how far the fill
departs from the value at the edge.** Replicate (zero departure) wins; a point
reflection on one edge departs a little and costs a little; a mirror on both
departs more; a decay to the plane's mean more again; flat grey more; zero
most, and zero is a catastrophe — +79 % on a strip of the 4096x512 image,
whose planes sit nowhere near zero.

**Why, and it is not subtle.** The predictor is `v = Σ cᵢ·p[o+tᵢ]`, and the
coefficients are fitted with a DC gain of about 1 — they have to be, or the
prediction darkens. For a pixel in column 0, **10 of the 20 same-component
taps reach outside**, including tap 0 = `(0,-1)`, the nearest neighbour and
the largest coefficient in any predictor. So roughly half the predictor's
weight lands on fabricated data. If those cells carry the edge value, their
joint contribution is `(Σ_outside cᵢ)·edge ≈ (half the gain)·edge`, and the
prediction still lands near the local value. If they carry anything a distance
δ away from it, the prediction is biased by `(Σ_outside cᵢ)·δ` — and the
coefficients cannot correct for it, because they are shared with the interior
pixels where the same taps carry real data.

A biased predictor is the most expensive thing this codec can have. That is
the whole result, and it says the search space is exhausted: any *fixed* rule
that departs from the edge value is worse in proportion to how far it departs,
so there is no better fixed rule to find.

**The regression numbers say the same thing from the other side.** Fitting
`p(x+j)` on `p(x)` over `PIA13882` gives slopes 0.90, 0.80, 0.71, 0.65, 0.60
for j = 1..5 — real shrinkage toward the mean, which is why "decay to the mean"
looks principled. It is principled for estimating *that pixel*. It is wrong
here, because the border is not being read on its own; it is being read through
coefficients whose gain is fixed by the interior.

**The error plane is not a separate lever either.** It feeds the activity
measure that picks the distribution's width, so it looked like somewhere the
border could help without touching prediction. Zeroing it costs +1.72 %,
doubling it +1.76 %, halving it +0.88 % — symmetric around the replicated
value, which is therefore about right.

**What is left.** Nothing that is a fixed function of the edge. Two things
that are not:

* **Real content, where it exists.** If mrpc ever codes an image in strips or
  tiles (`IMPROVEMENTS.md` §13 measures 5 % for region adaptivity once the
  model menu is wide enough to make regions disagree), the border between two
  strips should be the neighbouring strip's actual pixels, not a replication.
  That is the one border fill guaranteed to beat this one, and it only exists
  if the image is split.
* **The codec's own prediction, extended.** Filling the border with what the
  model would predict there — rather than with a copy — is the only
  non-constant rule that does not introduce a bias, because it is by
  construction the conditional mean. It is also circular to compute and would
  have to be approximated; untested, and the prior after the table above
  should be low.

And the honest scale: on a normally-shaped image the whole question is worth
**0.02 % to 0.06 %**. It only becomes a percent-scale question on strips narrow
enough for the border to cover a third of the width.

### A note on method, which applies to everything above

This is the first measurement in this session that resolved a sub-1 % model
change cleanly, and the technique is the reason. Encoding twice and comparing
sizes cannot do it: the search swings 10-30 % between adjacent class counts on
a small image, so the noise is two orders of magnitude larger than the signal.
The 16-tile corpus does not fix it either — mirrored borders measured
**+0.03 % to +0.75 % mean with a ±12 % spread** that way, which is
indistinguishable from nothing, and on 17 narrow strips it measured
**−0.82 % mean with a ±30 % spread**, which would have been indistinguishable
from a *win*.

Holding the model fixed and re-costing the same pixels removes the search from
the measurement. Any model change that can be expressed as "same symbols,
different probability or prediction" should be measured this way first, and only
then end-to-end.

The first six items say the same thing and it is a compliment to the codec: **the
two-pass search has already found the best static model available to it.**
Every remaining gain has to come from something the search structurally cannot
do — which is why §3, §4 and §5 are all about adding a *learned* component
beside the transmitted one, and why the one static parameter that did pay
(`NUM_TAIL`, −0.94 % mean, −3.67 % on `t24.bmp`) was one the family did not
have rather than one it had at the wrong value.

---

## 11. Suggested order

*Written before any of it was built. §14 says what each item actually cost
and bought, and the order below turned out to be roughly right about the
first two and wrong about the sixth.*

**Measured, bounded downside, no change to the search:**

1. **§4** — the 32-band APM. +0.15 % to +10.5 %, worst case −0.12 %, ~1.5-2x
   decode, and it needs nothing else built first. This is the item to build.
2. **§7** — the orientation trial. −0.69 % on the corpus, cannot lose, 2x
   encode, free at decode, and the machinery exists.

**Structural, in order:**

3. **§8, the fix half** — merge starved classes during the search instead of
   choosing a count up front. Everything else is measured through this noise.
4. **§6** — retune the coefficient coder for 8-bit coefficients. An enabler
   with no gain of its own, and it gates §5.
5. **§3** — the full adaptive mixture, once §4 has established the plumbing and
   the decode budget is known. +6.6 % where §4 gets +3.2 %.
6. **§5** — an 8-tap NLMS correction on top of the transmitted predictor. The
   largest expected gain and the largest risk to the decode budget.
7. **§9's run mode** — cheap, untested, and it buys decode time back for the
   items above.

**Not on the list:** everything in §10, and any further static parameter.

---

## 12. How to evaluate

The lesson of this session, at some cost:

* **Never on one image.** The search is bimodal and a compiler inlining
  difference moves the output 0.12 %. Use the 16-tile corpus, report all
  sixteen numbers, and score as the **mean of per-image percentages** — the
  total is dominated by the largest tiles and hides everything the small ones
  say.
* **Losslessness is absolute**, and `t.sh` checks it against the original, both
  code paths, every image.
* **Measure the alternative on the symbols the real coder saw.** The
  `tools/entropy_probe.patch` harness charges any candidate model inside
  `CodeImage`, after the optimisation has finished, so predictor, class map and
  thresholds are held identical and the difference is attributable. It also
  keeps the comparison *honest about decodability*: every alternative it charges
  depends only on symbols already coded.
* **Decode time is part of the result.** 276 ns a symbol-component today
  (`20000171A.bmp`, 8.4M symbols in 2.32 s). §4 is ~2x, §5 could be much worse,
  and neither is worth having if it puts the decoder in a different class.
* **Pre-test outside the codec where possible.** The density-family study (§10,
  rows 5-7) was an afternoon of offline fitting on a symbol dump and killed
  three proposals before any of them was built.

---

## 13. Sources

* `ALGORITHM.md` — what mrpc does.
* `ENTROPY.md` — where mrpc's bits go, measured; the rangecoder, the frequency
  tables, the adaptive and APM numbers in §3 and §4.
* `TUNING.md` — the class count and the model family; the bimodality in §8, the
  tap and coefficient measurements in §6, the settled list in §10.
* `ALGORITHMS.md`, `IMPROVEMENTS.md` — BMF v2.01 reconstructed, and its own
  improvement list. Used for structure and for where to look; its numbers
  describe its corpus, which except for `x_ep` is not this one.

---

## 14. What was built from this list, and what it measured

Everything below is on the 24-tile corpus, scored as the mean of per-image
percentages against the codec as it stood when §1-§13 were written, and
verified by round-tripping every image through both code paths.

Cost, since §12 insists it is part of the result. The default encoder is
**0.89-0.96x** the old one -- the model trial's seven extra coding passes
cost less than what the coefficient clamp saves the search -- and that holds
at scale: 705x800x4 goes from 251.5 s to 242.2 s.

The decoder is **2.09x** on that image, 0.383 s to 0.801 s over 2.26M
symbols. The small tiles measure 0.99-1.31x, but that is process startup
rather than the codec; 2.09x is the honest per-symbol number and it is the
price of the band stage -- a couple of dozen 64-bit multiplies, one
reciprocal and a gradient step per symbol, against a decoder that was doing
very little else. It is well inside a budget of 10x, and `ChooseSse` can
always answer "off" on an image where it is not worth paying.

`-t` is 2.2-2.4x to encode and free to decode.

**The default encoder is now -2.259 % mean, -1.043 % total.** Nineteen of
twenty-four tiles improve, by 0.09 % to 10.34 %; the other five rise by at
most 0.031 %, which is the model word in the header on an image whose model
the trial turned off. With `-t` it is **-2.654 % mean, -1.305 % total**.

| | | |
| --- | --- | --- |
| §4 the residual band correction | **shipped**, and it is most of the gain | -1.449 % mean at one fixed setting |
| §4b choosing the correction per image | **shipped** | it is a trial, so it cannot lose |
| §6 the coefficient clamp, per image | **shipped** | -4.2 % / -3.0 % on two palette tiles |
| §7 the orientation trial | **shipped behind `-t`** | a further -0.413 % mean, 2.4x encode |
| §3 more context for the correction | measured, **not shipped** | every axis that adds parameters loses |
| §8 merging starved classes | measured, **not shipped** (`MRPC_MINSAMP`) | +0.24 % mean, -0.13 % total |
| §5 an NLMS correction on the predictor | measured, **refuted** | there is nothing left to correct |
| §9's run mode | not built, and the reason is a result | the correction's zero band already is one |
| §9's palette ordering as a trial | not attempted, and why | every candidate is a full search |
| §9's vertical period | measured, **refuted** | best correlation over 64 lags is 0.08 |

### §4, and the shape it actually took

The proposal was a 32-band multiplicative correction on the residual. What
went in is that, with the observation that makes it *free*: `SseBand` is
monotone in the residual, so each band is one contiguous run of symbols, and

```
    p'(s) = P(band) * p(s | band)
```

factors exactly. The second factor is the static table untouched -- `EncSym`
over the band's own sub-range of the same `cumfreq` array, no new state, no
new arithmetic. Only the first factor is corrected, and it is a couple of
dozen numbers. With the correction at 1 the two stages cost exactly what the
one stage cost, so this is a strict generalisation of the old coder rather
than a replacement for it, and everything the two-pass search decided still
means what it meant.

The correction is learned by a gradient step on the log loss, in the log
domain and therefore multiplicatively, on integers, off the same quantised
probabilities the coder just used -- so the encoder and decoder walk it in
lockstep with no floating point in the loop.

The search is not told about any of this: it optimises the static model, and
the correction runs only in the final coding pass. That is option 1 of §3,
and the measured gains are delivered gains rather than upper bounds.

### §4b, the part that was not in the plan

Four separate attempts to give the correction more context all lost, and
they lost in the same way:

| | |
| --- | --- |
| the subpixel position of the prediction (8x contexts) | +0.093 % |
| the residual to the left, in four steps (4x) | worse on two of three probes |
| the residual to the left and above (9x) | worse still |
| 42 bands instead of 28 | +0.145 % |

...and yet on `big0`, the one 262k-symbol tile, the left-residual context
*wins* by 0.09 %. The axis these disagree on is not the model, it is the
image: a 61k-symbol tile cannot afford contexts a 262k-symbol one pays for,
and there is no rule from the pixel count that gets it right per picture --
`TUNING.md` spent a session establishing that for the class count.

So the encoder tries them. The coding loop is a small fraction of an encode
-- the optimisation above it runs twenty iterations of three searches over
the same pixels -- so `CodeImage` grew a measure mode that totals up the
exact code length without emitting anything, and `ChooseSse` runs it once per
candidate and keeps the best. Seven of them do not show above the run-to-run
noise: 5.51 s against 5.35 s and 5.46 s against 5.60 s on the same image,
either side of zero. Zero is one of them, so the correction cannot lose to
the codec without it, and the winner is three nibbles at the front of
`params`.

This is the same lesson as the palette ordering in §9 and it is worth stating
plainly: **on this codec, a trial beats a rule every time the trial is
affordable.** The rest of §14 is mostly about when it is not.

### §6, which turned out to be a gain and not an enabler

§6 predicted that widening the coefficient clamp was an enabler with no gain
of its own. It has one. A flat `+-4` measures -2.4 % to -4.0 % on four 8-bit
tiles, +0.2 % on two greyscale ones and **+5.2 %** on one 24-bit tile -- the
signature of a parameter that wants choosing per image, not setting.

The mechanism is not precision. It is that a single-component plane has no
cross-component taps to carry the level, so the twenty spatial taps have to
do all of it; and if the plane is palette *indices* rather than a picture,
the optimal filter has large cancelling coefficients that `+-2` throws away.

It could not be made a trial. The effect does not appear until the second
optimisation loop: loop 1's cost is **identical** across the three clamps on
two of the four tiles and ranks them backwards on a third, so there is no
cheap proxy, and a real trial is three full searches. What shipped is the
narrowest honest rule -- single-component images get `+-4` -- with the value
transmitted rather than compiled in, so a better rule needs no format change.
`MRPC_CRANGE` pins it for a measurement.

### §7, measured in full

| | | | |
| --- | --- | --- | --- |
| transposed is smaller on | 10 of 24 | best single gain | -4.81 % (`t24p_0`) |
| transposed is larger on | 14 of 24 | worst single loss | +38.69 % (`t24_0`) |
| imposed | **+1.589 % mean** | as a trial, measured alone | **-0.578 % mean** |
| | | as a trial, on top of section 4 | **-0.413 % mean** |

Confirming §7's own numbers and its caveat. It ships behind `-t` rather than
by default because the trial is a second full search: two encodes, and no
cheap proxy for the same reason §6 has none. It costs nothing at all to
decode, and the decoder needs no new code -- `LoadOrg` and `StoreOrg` are the
only two places the raster meets the codec's buffer, and transposing there is
a gather, not a copy.

### §5, refuted, and the reason is structural

This was "the largest expected gain". It is not a gain at all, and the
measurement that kills it is four lines of least squares rather than a codec
change.

An 8-tap integer NLMS correction on top of the transmitted predictor, keyed
on the component and on the activity group, seeded from zero and fed the
neighbouring signed residuals, **raised** the mean absolute error on
`t24_0` from 5.214 to 5.975 and the file from 15,423 to 19,280 bytes. That
is not a tuning problem. Dumping the residual plane and fitting it offline:

| | base | global LS | robust (IRLS) | oracle, refit per 32x32 |
| --- | --- | --- | --- | --- |
| `t24_0`, sum over components | 6.059 bits | 7.101 | 6.311 | 7.810 |
| `pia_0` | 15.908 | 15.926 | 15.896 | 15.910 |
| `big0` | 10.665 | 10.664 | 10.664 | 10.649 |
| `t24p_0` | 4.306 | 5.022 | 4.492 | 5.005 |

An **oracle** — a separate least-squares fit per 32x32 block, which no
online learner can beat — buys 0.016 bits on `big0` and loses on everything
else. Where least squares appears to win on the sum of squares it loses on
the entropy: `t24_0`'s green plane goes from RMSE 40.5 to 36.8 while its mean
absolute error goes *up*, because the correlation that exists is entirely in
a handful of outliers and the fit chases them.

Why: mrpc's weighted least squares already makes the residual orthogonal to
the taps, and with 20 same-component plus 6-per-other-component taps the tap
set very nearly spans the neighbours' residuals as well -- a neighbour's
residual is a linear combination of pixels that are mostly inside this
pixel's own neighbourhood. There is no information in the neighbouring
residuals that the transmitted predictor has not already used.

**§5's premise was that mrpc's classes are a transmitted version of BMF's
seated NLMS rows. They are; that is exactly why adding NLMS on top buys
nothing.** BMF's filter is learned online precisely because it has no
two-pass search to fit it. mrpc has one. The two solve the same problem, and
running them in series solves it twice.

The probe is `tools/nlms_probe.patch` and `tools/resid_fit.py`.

### §8, which stopped paying once the rest was in

`MergeStarved` folds any class holding fewer than `MRPC_MINSAMP` pixels per
coefficient into the surviving class whose predictor is closest, compacts the
labels, and lets the search continue -- so the count only ever falls, it
falls to something the image can fit, and no decision about it is made before
the segmentation has been seen. `CodeParams` moved to after the search so
that what goes in the stream is the count the segmentation ended up with.

Measured on the codec as it was, a floor of 16 samples a coefficient was
worth **-0.246 % mean, -0.297 % total**. Measured again with §4 and §6 in
place it is **+0.241 % mean, -0.125 % total** -- and the +-5 % per-image
spread never went away: six tiles pay 0.8-4.7 % while the other eighteen
gain 0.06-2.9 %. The merge is doing something the residual correction was
already doing.

It is off by default and reachable through `MRPC_MINSAMP`, because the total
still favours it and someone measuring on larger images should have it to
hand.

### §9's run mode, which the correction turned out to be

A run/skip mode was proposed for planes where one value covers most of the
area. It was not built, and the reason is a result rather than a schedule:
the correction's zero band **is** `residual == 0`, and its probability is
exactly what the correction learns and re-learns per context. Charged on its
own, before the coefficient clamp and the trial, the correction is worth
-9.35 % on `t24_0`, -4.34 % on `t24p_1` and -2.82 % on `piap_0` -- the
flattest planes in the corpus, and that is the mechanism. An explicit run
mode
would signal what this infers, at the cost of a bitstream change and an
interaction with an activity measure that has no notion of "nothing
happened".

### §9's palette ordering, and why it stayed a proposal

§9 measured an *imposed* luma sort as +27.9 % and +3.1 %, and proposed
revisiting it as a trial with one bit in the stream and an ordering chosen to
minimise coded size. It was not attempted, and the reason is the one that
also stopped §6 and §7 from becoming trials: cost. Every other trial here
re-runs something cheap -- a coding pass for §4b, a whole search for §7. A
palette ordering changes the *image*, so each candidate is a full search, and
there are 256! of them; even a greedy pairwise descent is dozens of encodes.

It is also not the codec's to make. mrpc receives the palette inside the
opaque `head` blob and never looks at it; the permutation and its inverse
belong to whatever wrote the BMP. That is a frontend change with a codec
budget attached, and it is the one item on this list that is still worth
someone's afternoon -- `t24p_0` and `piap_0` are the tiles that responded
most to everything else here, which is a hint about where the remaining
redundancy is.

### §9's vertical period, tested on the residual and not worth a tap

"A transmitted vertical period is the cheap idea in both codecs and is
untested in both." It is now tested here, and it is not there. Taking the
same residual dumps §5 used and correlating each plane with itself at every
vertical lag from 1 to 64:

| | largest \|rho\| over 64 lags | at |
| --- | --- | --- |
| `t24p_0` | 0.080 | p = 62 |
| `t24_0` | 0.038 | p = 63 |
| `big0` | 0.041 | p = 3 |
| `pia_0` | 0.043 | p = 2 |

A linear tap at correlation `rho` removes `rho^2` of the variance, so the best
of those is worth 0.6 % of the variance -- about five thousandths of a bit --
and it is the largest of sixty-four candidates on twelve thousand samples,
where the noise floor on `rho` is already 0.009. Nothing else clears 0.05.
BMF measured four long taps as a loss on its own corpus; this is the same
answer arrived at without building anything.

### What the whole exercise says

The six items §1-§6 said the same thing: the two-pass search has already
found the best static model available to it. That reading survives, and it
now has a sharper edge. Every gain here came from one of exactly two places:

1. **A learned component beside the transmitted one, in the one domain
   where the search structurally cannot look** -- the probability, not the
   prediction. §4 works and §5 does not, and the asymmetry is not an
   accident: least squares already solved the prediction problem, and
   nothing solved the "is this family the right shape here" problem.
2. **Letting the encoder choose instead of guessing.** §4b and §7 are both
   trials; §6 is a rule only because its trial is unaffordable, and it is
   transmitted so that it can stop being one.

And every attempt to add parameters -- finer bands, more context, more taps,
more classes -- lost, at every size in this corpus. That is not a statement
about these particular parameters. It is a statement about 61,440 symbols.

---
