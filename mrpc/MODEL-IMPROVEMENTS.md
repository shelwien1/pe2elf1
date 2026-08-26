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

The first six say the same thing and it is a compliment to the codec: **the
two-pass search has already found the best static model available to it.**
Every remaining gain has to come from something the search structurally cannot
do — which is why §3, §4 and §5 are all about adding a *learned* component
beside the transmitted one, and why the one static parameter that did pay
(`NUM_TAIL`, −0.94 % mean, −3.67 % on `t24.bmp`) was one the family did not
have rather than one it had at the wrong value.

---

## 11. Suggested order

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
