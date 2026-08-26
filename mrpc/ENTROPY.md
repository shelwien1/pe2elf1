# mrpc's entropy coding: where the bits actually go

A measured answer to two questions: would a more precise rangecoder buy
anything, and would PAQ-like model components?

**Short answer.** The rangecoder is already exact — it loses 1.7 bits over
230,400 symbols, and no change to it can recover more than a couple of bytes
per image. The static model is also already at its own ceiling: mrpc's
two-pass optimiser has found the best generalized-Gaussian parameters
available to it, and mixing in neighbouring σ or other shapes measures at
−0.7% to +0.9%. Everything that is left is **adaptivity**, and it is worth
between nothing and 12% depending entirely on the image: ~0% on residuals
that really are generalized-Gaussian, 3–12% on paletted and synthetic ones.

### Everything measured, in one table

Percentages are of the coded image (side information excluded), charged on
exactly the symbols the real coder saw. `+` means smaller than mrpc today.

| | `t24` | `t24_pal` | `t24_gray` | `PIA` | `PIA_pal` | `PIA_gray` | `20000171A` |
| --- | --- | --- | --- | --- | --- | --- | --- |
| bits/symbol now | 3.316 | 3.755 | 5.432 | 4.846 | 5.311 | 5.460 | 2.590 |
| rangecoder loss | 0.0002% | — | 0.0016% | 0.0001% | — | — | — |
| frequency tables | 0.05% | 0.09% | 0.002% | 0.001% | 0.005% | 0.003% | 0.001% |
| mix σ neighbours | +0.09% | +0.94% | −0.10% | −0.07% | −0.02% | −0.05% | +0.09% |
| mix all 16 shapes | −0.32% | +0.44% | −0.69% | −0.28% | −0.57% | −0.29% | −0.24% |
| **32-band APM** | **+3.24%** | **+10.54%** | **+1.12%** | −0.12% | **+3.47%** | −0.12% | **+0.15%** |
| full adaptive mix | +6.58% | +12.62% | +1.61% | −0.06% | +2.87% | −0.10% | +0.20% |

---

## 1. What the coder is

`sh_v2f.inc` is a bitwise carry-counting rangecoder.

* `range` is renormalised one bit at a time (`while(range<0x80000000) ...`),
  so it never leaves `[2^31, 2^32)`.
* The interval boundaries are computed as `a*range/tot` directly
  (`mulRdiv`), not as `(range/tot)*a`. That is the difference between one
  rounding and two, and it is why the low end stays exact.
* Carries are counted rather than propagated (`FFNum`, `Cache`), so `low` is
  a plain 32-bit register with a carry bit rather than a big integer.
* Frequencies come from tables totalling `MAX_TOTFREQ = 2^20`, which leaves
  2048× headroom under `range`.

The quantisation in `mulRdiv` is at most 1 part in `range ≥ 2^31`, so the
distortion of any symbol's probability is bounded by 2⁻³¹.

### Measured

Charging every symbol against the model's own table and comparing with the
bytes the coder actually emitted for the same span:

| image | symbols | model wants | coder emits | loss |
| --- | --- | --- | --- | --- |
| `t24.bmp` | 230,400 | 764,054 bits | 764,056 bits | **+1.7 bits** (0.0002%) |
| `PIA13882_crop256.bmp` | 196,608 | 952,841 bits | 952,840 bits | −1.3 bits (−0.0001%) |
| `t24_gray.bmp` | 76,800 | 417,153 bits | 417,160 bits | +6.6 bits (0.0016%) |

7×10⁻⁶ bits per symbol. **There is nothing here to recover.** A more precise
rangecoder — 64-bit `low`, carryless renormalisation, a binary coder with
12-bit probabilities, anything — cannot win more than the ~2 bytes above, and
most of those variants would lose rather than win. This part of the codec is
finished.

---

## 2. What the model is

Per pixel and component, mrpc codes one symbol from a 256-wide window of a
511-entry table:

* **shape**: a generalized Gaussian, `exp(-(βx)^s)`, with `s` one of 16
  candidates, chosen per (component, activity group) and transmitted;
* **scale**: σ from a fixed 16-step geometric ladder (`SIGMA[]`, 0.15 …
  30.89, ratio ≈ 1.38), selected by an activity group; the *thresholds* that
  map local activity to a group are chosen by dynamic programming against
  exact code length and transmitted;
* **position**: the prediction to ⅛ of a pixel, via `NUM_SUBPM = 8`
  pre-shifted copies of each table;
* **quantisation**: frequencies scaled to a `2^20` total with `MIN_FREQ = 1`.

The essential property: **nothing adapts while coding.** Every parameter is
decided in the two-pass optimisation and sent. The decoder rebuilds the same
tables and reads them.

---

## 3. Three things that are not costing anything

Each measured by re-charging *exactly the symbols the real coder saw* under
an alternative model, so predictor, class map and group thresholds are held
identical.

### Frequency-table quantisation

The unquantised pdf against the `2^20` integer table:

| image | table cost |
| --- | --- |
| `t24.bmp` | 0.051% |
| `t24_pal.bmp` | 0.093% |
| `PIA13882_crop256.bmp` | 0.001% |
| `20000171A.bmp` | 0.001% |

Raising `MAX_TOTFREQ` to 2²², 2²⁴ or 2²⁶ moves the output by ±0.7% in *both*
directions — that is the optimiser landing in a different local optimum, not
a coding gain. **2²⁰ is enough.**

### Is the σ ladder too coarse?

A Bayesian mixture of the chosen group's table with its two neighbours —
no counts to learn, no side information, weights depending only on symbols
already coded, so the decoder can run it:

| image | gain |
| --- | --- |
| `t24.bmp` | +0.09% |
| `t24_pal.bmp` | +0.94% |
| `t24_gray.bmp` | −0.10% |
| `PIA13882_crop256.bmp` | −0.07% |

### Is one shape per (component, group) too rigid?

The same mixture over all 16 shapes at the chosen group:

| image | gain |
| --- | --- |
| `t24.bmp` | −0.32% |
| `t24_pal.bmp` | +0.44% |
| `t24_gray.bmp` | −0.69% |
| `PIA13882_crop256.bmp` | −0.28% |

Both are a wash, and that is a *positive* result about mrpc: the group DP and
the shape search have already found the best static parameters in the family.
There is no static modelling headroom left. Anything that helps has to be
something the two-pass optimiser structurally cannot do.

---

## 4. What does help: adaptivity

Mix the static pmf with a model learned online from the symbols already
coded, `q = (1-w)·p_static + w·p_adaptive`, with `w` adapted per context by
one gradient step on the log loss. This is decodable: everything it depends
on is already in the stream.

| image | bits/sym now | best context | gain |
| --- | --- | --- | --- |
| `t24_pal.bmp` 320x240 palette | 3.755 | (comp, group, subpixel), recency | **+12.62%** |
| `t24.bmp` 320x240 RGB | 3.316 | (comp, group, subpixel), recency | **+6.58%** |
| `PIA13882_crop256_pal.bmp` | 5.311 | (comp, group, subpixel) | +2.87% |
| `t24_gray.bmp` | 5.432 | (comp, group) | +1.61% |
| `20000171A.bmp` 4096x512 RGBA | 2.590 | (comp, group, sign of left) | +0.20% |
| `PIA13882_crop256.bmp` | 4.846 | — | −0.06% |
| `PIA13882_crop256_gray.bmp` | 5.460 | — | −0.10% |

Three things worth noting about the shape of that table.

**The mixture beats both of its parts.** The best possible *static* histogram
at the model's own context — measured as the plug-in conditional entropy plus
an MDL penalty for learning it — is only +1.09% on `t24.bmp`. The mixture
gets +6.58%. It wins because it uses the generalized Gaussian's smooth tails
where counts are sparse and the histogram's accurate peak where they are not.
Neither component alone does that.

**It is not mainly non-stationarity.** Halving the counts every 4096 symbols
to favour recent statistics adds +0.04% on `t24.bmp` and +0.9% on
`t24_pal.bmp`. So the gain is a genuine mismatch between the residual
distribution and the generalized-Gaussian family, not drift across the image.

**The gain tracks how well the family fits.** `PIA13882` is quantised space
imagery whose residual really is generalized-Gaussian, and adaptivity buys
nothing — mrpc is already optimal there. Palette indices are labels rather
than magnitudes, no smooth unimodal family fits them, and adaptivity buys
12%. Photographic RGB sits between.

---

## 5. The component worth building: a 32-band correction

A full adaptive histogram means 511 counts per context and a Fenwick tree per
symbol in the decoder. PAQ's APM/SSE idea in the shape this alphabet wants is
cheaper: keep the static pmf, learn **one multiplicative correction per band
of the residual**, 32 bands log-spaced in magnitude with sign.

```
p(r) ∝ p_static(r) · c[ctx][band(r)]
c[ctx][b] = (observed[ctx][b] + A) / (static expected[ctx][b] + A)
```

32 numbers per context instead of 511, and the cumulative the decoder needs
is 32 lookups into the static `cumfreq` table it already has.

| image | 32-band APM | full adaptive | APM captures |
| --- | --- | --- | --- |
| `t24_pal.bmp` | **+10.54%** | +12.62% | 84% |
| `PIA13882_crop256_pal.bmp` | **+3.47%** | +2.87% | 121% |
| `t24.bmp` | **+3.24%** | +6.58% | 49% |
| `t24_gray.bmp` | **+1.12%** | +1.61% | 70% |
| `20000171A.bmp` | +0.15% | +0.20% | 75% |
| `PIA13882_crop256.bmp` | −0.12% | −0.06% | — |
| `PIA13882_crop256_gray.bmp` | −0.12% | −0.10% | — |

On `PIA13882_crop256_pal.bmp` the constrained version *beats* the full
histogram: 32 parameters generalise where 511 overfit. That is the usual
argument for secondary estimation over raw counting.

---

## 6. The catch, and it is a real one

mrpc's optimiser never runs the coder. `CalcCost`, the OpenCL cost cube, the
quadtree prefix sums, the coefficient sweep and the group DP all depend on
one property: **a symbol's cost does not depend on when it is coded.** That is
what makes a block's cost under a class a sum of independent per-pixel terms,
and therefore what makes the whole two-pass search — and its OpenCL port —
possible.

An adaptive coder breaks that property. Two ways out:

1. **Optimise with the static model, code with the mixture.** The search stays
   exactly as it is; only the final coding pass mixes. The optimiser is then
   minimising a slightly wrong objective, but nothing else changes and the
   OpenCL path is untouched. *Every number in sections 4 and 5 above was
   measured this way* — the symbols being charged are the ones the static
   optimiser chose — so those gains are what this option actually delivers,
   not an upper bound on it.

2. **Make the correction two-pass and transmit it.** Then it is static, the
   optimiser stays exact, and you can even optimise against it. But you pay
   side information, and you lose the local adaptation that section 4 shows
   is most of the gain.

Option 1 is the one to build.

**Cost.** Decoding is 276 ns/symbol today (`20000171A.bmp`, 8.4M symbols in
2.32 s), and it is dominated by the predictor's gather and `DecSym`'s binary
search. A 32-band APM adds ~32 table lookups and a normalisation per symbol
in both encoder and decoder — call it 1.5–2× on the decode side, and
proportionally less on the encode, where the optimisation loop dominates and
would not change at all.

---

## 7. Recommendations, ranked

1. **Leave the rangecoder alone.** 0.0002%. It is exact and there is nothing
   to win.
2. **Leave the frequency tables alone.** 2²⁰ costs 0.001–0.09%.
3. **Do not add static experts** — σ neighbours, extra shapes, finer ladders.
   Measured at ±0.9%, and the optimiser has already solved that problem.
4. **Build the 32-band APM** on `(component, group, subpixel)`, mixed with the
   static pmf, under option 1 above. Measured across seven images it ranges
   from **+10.5%** (palette indices) through +3.2% (small photographic RGB)
   and +1.1% (grey) down to +0.15% (the 4096×512 RGBA image) and −0.12%
   where the generalized Gaussian already fits. The downside is bounded at
   about a tenth of a percent and the upside is large, so it is worth having
   — but it is worth having *for paletted and small images*, and it will do
   almost nothing for a big well-behaved photograph. Cost is ~2× decode.
5. **Look outside the entropy coder first if you want the bigger win.** The
   class-count heuristic is `10.4e-5·W·H + 13.8`, which gives `t24.bmp` 21
   classes where the cap is 63. Whole files, `-DMRP_CLASS=63`:

   | image | default | 63 classes | | time |
   | --- | --- | --- | --- | --- |
   | `t24.bmp` | 99,245 | 85,406 | **−13.94%** | 4.2 s → 6.1 s |
   | `t24_pal.bmp` | 44,801 | 41,894 | **−6.49%** | 3.0 s → 3.4 s |
   | `PIA13882_crop256.bmp` | 121,423 | 122,776 | +1.11% | 4.0 s → 7.4 s |

   On the two images where the entropy coder had the most to give, the class
   count gives more, for about 1.5× the encode time — and on the third it
   costs a little, which is why this is a heuristic worth *retuning* rather
   than a constant worth raising. Both searches are linear in the class
   count, so the time is predictable. This is a one-line change and it should
   be measured before any entropy-coding work is started.

---

## Appendix: how this was measured

An instrumented encoder (`MRPC_MIX=1`) charges each alternative model on
exactly the symbols the real coder saw, inside `CodeImage`, after the
optimisation has finished. Nothing goes into the bitstream. Every alternative
is a decodable model: the adaptive counts, mixture weights and band
corrections all depend only on symbols already coded, so a decoder can
reproduce them.

* Coder loss is `(bytes emitted between two positions) × 8` minus
  `Σ −log₂(freq[s]/window total)` over the symbols in that span.
* Table-quantisation cost keeps the unquantised pdf beside `freq[]` and
  charges both.
* Expert mixtures use `w ← w·p(x)/Σ`, floored so the mixture can switch back.
* The adaptive histogram keeps counts in a Fenwick tree over the 511 residual
  positions and renormalises over the current 256-wide window.
* The "best static histogram" bound is the plug-in conditional entropy plus
  `(k−1)/2·log₂ N` per context for learning it.

Images: `t24.bmp` (320×240 RGB), `PIA13882_crop256.bmp` (256×256 RGB, a crop
of quantised space imagery), `20000171A.bmp` (4096×512 RGBA), and 8bpp grey
and median-cut palette versions of the two small ones.
