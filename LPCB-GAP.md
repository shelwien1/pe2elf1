# Where GraLIC beats BMF on LPCB

A study of the 107-image LPCB corpus, the 25 MB GraLIC is ahead by, which files
that gap lives in, and what in BMF is producing it.

Five findings:

1. **The gap is two unrelated problems wearing one number.** 99 continuous-tone
   images lose a uniform +0.134 bpp; 8 tone-curved images lose +0.475 bpp and
   carry a third of the total deficit between them (§2).
2. **The larger problem is a misallocated model, not a missing one.** Between
   50% and 63% of BMF's output bits are produced by frequency counters that none
   of the codec's own adaptive machinery is attached to — measured, not inferred
   (§3).
3. **The regression you noticed is real and I reproduced it**: the IMPROVEMENTS
   work cost 0.163% on LPCB, about +1.6 MB corpus-scale. **The sign patch has
   already more than reversed it** — the current codec is 1.03% *smaller* than
   the pre-IMPROVEMENTS build on the same files (§7).
4. **Two plausible-looking explanations are wrong**, and both would otherwise be
   the obvious next thing to build (§5).
5. **Renumbering as first built had a failure mode worth more than the feature.**
   On `PIA13882`, two rows of caption — 0.135% of the pixels — cost 12.3% of the
   file, because a rank map over the used values de-regularises a step-2 lattice
   to seat them. A two-tier map fixes it: −12.13% on that file, which turns an
   11.07% loss to gralic into a 2.40% win (§4.1).

> **Revised against `claude/bmf-lpcb-performance-hdzfzy`.** That branch's
> `LPCB-REPORT.md` is a deeper investigation of the same question, and it
> refutes two things below by building them. The corrections are marked
> **[corrected]** where they occur; in summary:
>
> * **§3's split between the sign and the magnitude is wrong.** My
>   instrumentation totals every coder pass, and BMF makes 4.33 of them per
>   sample — so the figures are dominated by trial encodes rather than by the
>   stream that ships. Attributed to the final encode only, the sign is ~26%
>   and the magnitude ~26%, not the ~20/50 split I reported. The headline the
>   section was written to support survives unchanged: about half the output
>   comes from decisions with no mixing and no secondary estimation.
> * **§6's A2 — widening `CtxPair` — is refuted.** All three axes I proposed
>   (the flatness flags, neighbour magnitudes, the residual sign) were built
>   with the table enlarged so nothing collides, and all are a wash or a loss.
>   The reason is mechanical and is the useful part: a frequency strip has no
>   parent and no mixing partner, so a new axis has to repay its own learning
>   cost immediately. The way in is mixing, not a bigger table.
> * **§5's dismissal of cross-component structure was tested on the wrong
>   thing.** My correlation asks whether a *nonlinear value-domain map* would
>   pay, and the answer to that is still no. But splitting photographs into
>   single planes and running both codecs on each shows BMF at parity with
>   gralic on one real plane, with **71–84% of each photograph's outcome
>   decided in the cross-plane channel**. The prize is a cross-plane predictor
>   that sees a *neighbourhood*, which is not what I tested.
>
> Confirmed rather than corrected: §4's renumbering proposal, measured there at
> **5.37 MB** across all six candidates at full size — and it needs a trial
> gate, without which `PIA13915` loses 3.24%. §5's acquittal of the
> plane-coding search (648 of 648 slot searches pick the cheapest trial). And
> §7.1's regression, bisected there to a single commit, `12b7be4`, the adaptive
> probability map on alt-P2's class decision — with the IDX/MOD port measured
> byte-neutral on real photographs across all 26 commits.

---

## 1. The numbers

Sizes as supplied in `lpcbs.txt` (`gra11` vs `bmf18`), against the 107 images:

| | bytes | bits/pixel |
|---|---:|---:|
| MED residual floor (order-0, from `bmgstat`) | — | 9.590 |
| GraLIC | 971,986,179 | 6.737 |
| BMF | 997,191,732 | 6.912 |
| **gap** | **+25,205,553 (+2.59%)** | **+0.175** |

BMF is larger on **83 of the 107 files**.

Per family:

| family | n | BMF loses on | GraLIC bpp | BMF bpp | Δ bpp | Δ bytes | Δ % |
|---|---:|---:|---:|---:|---:|---:|---:|
| olympus_xz1 | 27 | **27** | 8.049 | 8.293 | +0.245 | +8,383,691 | +3.04% |
| PIA | 25 | 8 | 4.802 | 5.111 | +0.310 | +4,901,110 | +6.45% |
| STA | 20 | 14 | 6.240 | 6.368 | +0.128 | +4,417,017 | +2.05% |
| sony_a55 | 12 | **12** | 7.610 | 7.746 | +0.136 | +3,305,795 | +1.79% |
| fujifilm_finepix_x100 | 12 | **12** | 5.796 | 5.949 | +0.153 | +2,838,172 | +2.64% |
| canon_eos_1100d | 11 | 10 | 6.686 | 6.767 | +0.081 | +1,359,768 | +1.21% |

Three of the six camera families lose on *every single file*. That uniformity is
the first useful signal: a loss that never varies is not a model that sometimes
mispredicts, it is a stage that is always weaker.

---

## 2. The gap is two problems, not one

`bmgstat` reports a "remapped gain" per component — how much MED-residual entropy
falls if the used values are renumbered `0..N-1` before predicting. It is non-zero
on only **8 of the 107 files**, and splitting the corpus on it separates the gap
cleanly:

| class | n | GraLIC bpp | BMF bpp | Δ bpp | Δ bytes | share of gap |
|---|---:|---:|---:|---:|---:|---:|
| continuous tone | 99 | 6.691 | 6.826 | +0.134 | +17,051,663 | 68% |
| tone-curved / sparse value set | 8 | 7.076 | 7.551 | **+0.475** | +8,153,890 | **32%** |

**7.5% of the files carry a third of the deficit**, at 3.5× the per-pixel rate.
These two classes need different fixes and are treated separately below.

Correlations against the per-file gap, over all 107 (Pearson):

| feature | r vs Δ bpp | r vs Δ % |
|---|---:|---:|
| `bmgstat` remapped gain | **+0.577** | +0.356 |
| output bpp | +0.389 | +0.382 |
| fraction of MED floor recovered | −0.394 | −0.496 |
| mean run length | −0.100 | **−0.660** |
| order-0 entropy H0 | +0.166 | +0.403 |
| pixel equals north | −0.199 | −0.385 |
| **nonlinear cross-component headroom** | **−0.045** | −0.151 |

---

## 3. Deficit A — the bits go where the model isn't

This is the 68%, and it is the one worth fixing first.

### What alt-P2 actually codes

A residual leaves `code_three_way` as up to four decisions:

1. **zero / non-zero** — a ternary `P2Freq` estimate, mixed with three other
   predictions through `Mix4`, then refined through an `Apm`.
2. **sign** — a `P2Freq` ratio refined through `sign_apm` (added by the recent
   sign patch).
3. **magnitude bucket** — an 8-ary arithmetic symbol drawn from one raw frequency
   row, `model_strip(ctx_pair[idx&1])`. No mixing. No SSE. No APM.
4. **bits within the bucket** — a binary tree of `FreqPair` counters coded with
   `encode_bit(fa, f1, go)` straight off the counts. No mixing. No SSE. No APM.

Stages 1 and 2 get the whole apparatus. Stages 3 and 4 get a counter and a
halving rule.

### Where the bits actually land

I built an instrumented copy that accumulates `−log2(p)` at each of the four
sites and ran it on three files. (The totals cover the plane-selection trial
passes as well as the final encode; every pass uses the same coder, so the
proportions are what to read.)

| file | zero flag | sign | **magnitude bucket** | **within-bucket** | unmodelled share |
|---|---:|---:|---:|---:|---:|
| `olympus_xz1_24` | 17.1% | 19.8% | **50.5%** | **12.6%** | **63.1%** |
| `PIA13785` | 10.3% | 16.4% | **52.3%** | **21.0%** | **73.3%** |
| `STA13782` | 20.1% | 16.5% | **46.3%** | **17.1%** | **63.4%** |
| `canon_eos_1100d_01` | 42.8% | 26.0% | **30.1%** | **1.1%** | **31.2%** |

**[corrected] These percentages count every coder pass, not the one that ships.**
BMF makes 4.33 passes per sample (§5), so a stage's share here is its share of
all trial encoding, and the trials use different predictors from the winner.
`claude/bmf-lpcb-performance-hdzfzy` instruments the final encode only and gets,
on four photographs: zero flag 42–58%, **sign 24–27%**, **magnitude level
16–29%**, low bits 1–3%. So the magnitude is about a quarter of the shipped
stream, not half, and the sign is as large as it is — which is why the sign
patch (§7.2) moved as much as it did. What the table was written to show is
unaffected: sign plus magnitude plus low bits is still about half the output,
and none of it had mixing or an APM before that patch.

Cost per event tells the same story from the other side. On `olympus_xz1_24` the
magnitude bucket costs 1.868 bits per event and the within-bucket bits cost 0.898
— the latter is 10% off incompressible.

The one file where the modelled stages dominate, `canon_eos_1100d_01`, is also
the file with the smallest family deficit (canon, +1.21%). The correspondence
runs the whole way down: **the share of the budget BMF models well predicts how
close BMF gets.**

### Why the magnitude is under-conditioned

`ctx_pair` is built by `MakeCtxPair(ctx_delta[...], ctx15)`, and `CtxDelta` is
`(plane, lvl)`:

```
Index CtxDelta          Index CtxPair
 ADD 4:  plane           ADD %M%CtxDelta_Volume: delta
 ADD 16: lvl             ADD 16: ctx15
```

Volume 4 × 16 × 16 = **1024 contexts**: the plane, a 16-level quantisation of the
run-derived delta, and a 16-level activity class. `GL_model_table_bytes` is
520,192, which is exactly 1024 × 254 × 2 — the arena is sized to the nose of the
context and no further.

Meanwhile `seat_symbol_context` has *already computed*, for the zero flag:

* `flat_a`, `flat_b` — two flatness flags over the causal neighbourhood,
* `mixer_fwd(ctx_w, 5) >> kP2MixShift` — the NLMS prediction's own confidence,
* the full `ctx` that `MakeP2Ctx` builds out of all of the above.

The stage carrying 50% of the bits sees none of it. It is not that the
information is missing; it is computed, used for the cheap decision, and dropped
before the expensive one.

---

## 4. Deficit B — tone-curved and sparse value sets

The eight files with a non-zero remapped gain are images that have been through a
tone curve or a quantiser: they use 120–210 of the 256 levels per component, with
gaps of 2 and 3 between adjacent used values.

`PIA13812` is the extreme case and BMF's worst relative loss on the corpus:

| | |
|---|---|
| size | 1188 × 1015, 1.2 Mpx |
| distinct values | B 120, G 138, R 133 of 256; min gap 2 |
| MED floor | 13.551 bpp |
| **after renumbering** | **11.682 bpp** (−1.869) |
| GraLIC | 8.675 bpp |
| BMF | 11.000 bpp (**+26.80%**) |

BMF is 2.325 bpp behind, and **1.869 bpp of that is available from renumbering
alone** — 80% of the deficit on this file, from a transform that costs one
histogram pass and 256 bytes of header per component.

The mechanism is direct: when only every second or third level is used, every
residual is a multiple of 2 or 3, and BMF spends the bucket and within-bucket
bits of §3 coding a low bit that is always the same. The counters do eventually
learn it, but they learn it separately in each of the 1024 contexts, and it is
still one arithmetic decision per sample instead of none.

The other seven: `PIA13882` (+11.72%, 1.119 remap), `PIA13912` (+15.05%, 0.669),
`PIA13815` (−6.15%, 0.468), `PIA13785` (+13.20%, 0.283), `STA13845` (+8.24%,
0.257), `STA13843` (+0.94%, 0.119), `PIA13799` (−41.85%, 0.103).

### 4.1 The rank map's failure mode — two rows of caption

Renumbering as first built maps a value to its **rank among the used values**.
That is right when every used value carries weight, and it fails hard when a
handful do not.  `PIA13882` is the case, and it explains the whole of that
file's deficit.

The image is 1533 × 1484.  Rows 0–1481 use **100 / 95 / 85** values per plane,
on a step-2 lattice.  The last two rows are an off-lattice colour bar, and they
carry 82 / 91 / 92 values of their own:

| coded extent | size | bpp | distinct per plane |
|---|---|---|---|
| rows 0–1481 | 2,379,876 | 8.380 | 100 / 95 / 85 |
| rows 0–1482 | 2,553,556 | 8.986 | 143 / 129 / 118 |
| rows 0–1483 (whole) | 2,714,592 | 9.546 | 182 / 186 / 177 |

**0.135% of the pixels cost 12.3% of the file.**  The mechanism is the map, not
the strip's own content: once the strip's values are interleaved into the
lattice, a step of 2 in the original becomes a step of 1, 2 or 3 depending on
where it lands, so the map de-regularises 99.87% of the image in order to seat
0.13% of it.  Model selection then also flips from alt-P2 to slow.

The same defect shows up as a **whole-versus-tiled inversion**, which is what
made it visible.  Coding the image as 16 independent tiles — each paying its own
header and restarting its models — beats coding it whole:

| | whole | sum of 4×4 tiles | whole − sum |
|---|---|---|---|
| bmf | 2,714,592 | 2,245,292 | **+469,300 (+20.90%)** |
| gralic | 2,444,091 | 2,398,955 | +45,136 (+1.88%) |

gralic pays the 1.9% a model restart costs.  BMF pays 20.9%, because tiling
isolates the strip into one tile and leaves the other fifteen their lattice.
Nearly all of it is the two-piece split: top ¾ + bottom ¼ coded separately is
2,252,028, within 0.3% of the 4×4 sum, so the penalty is **not** a function of
window size.

**The fix, built:** the map is now two-tier.  Values above a frequency floor
take the low, contiguous ranks; the rest of the used set follows above them.
The floor is chosen per plane from a ladder of `npix>>10` .. `npix>>16`, ranked
by the KT cost of the plane's MED residual under the candidate map — one pass
over the plane, not a model encode — with the plain rank map as incumbent and
`kKeepShift` as the margin, so a plane with no light tail keeps the plain map.
On `PIA13882` the probe cuts the dense sets at 100 / 93 / 85 values:

| | size | bpp | vs gralic |
|---|---|---|---|
| before | 2,714,592 | 9.546 | +11.07% |
| **after** | **2,385,216** | **8.387** | **−2.40%** |

−12.13%, and the file flips from BMF's second-worst relative loss on the corpus
to a win.  The reach is wider than one file: at a 1/10,000 floor, five of the
eighteen locally held LPCB frames drop below three quarters of their used-value
count — `STA13456` (740→397), `PIA13882` (545→279), `PIA13872` (743→536),
`sony_a55_07` (532→355), `STA13900` (768→612) — and the MED probe picks the
two-tier map on `PIA13785`, `PIA13915` and `STA13845` as well.

Measured against a `HEAD`-built reference over the frames held locally, with
every new stream decoded back and compared pixel for pixel:

| file | before | after | |
|---|---|---|---|
| `PIA13799` | 87,756 | 87,756 | — |
| `PIA13913` | 403,216 | 403,216 | — |
| `STA13456` | 1,093,604 | 1,093,604 | — |
| `PIA13872` | 190,144 | 190,144 | — |
| `STA13782` | 935,760 | 935,760 | — |
| `PIA13812` | 1,294,036 | 1,294,036 | — |
| `STA13900` | 1,910,720 | 1,910,720 | — |
| `PIA13785` | 1,855,980 | **1,797,872** | −3.13% |
| `PIA13882` | 2,714,592 | **2,385,216** | −12.13% |

The seven unchanged files are the point as much as the two that move: the
incumbent margin means a plane with no light tail in its histogram keeps the
plain rank map, so `PIA13812`, which gains 21.9% from that map, is untouched.

#### The rung the probe picks is not the rung that codes best — unresolved

The floor ladder is ranked by an order-0 KT measure of the MED residual, and on
`PIA13882` that measure is demonstrably wrong.  Forcing the `>>10` rung — dense
sets of 96 / 89 / 85 rather than the probe's 100 / 93 / 85 — gives **2,240,400
bytes, 7.878 bpp**: another −6.07%, and 8.33% *ahead* of gralic rather than
2.40%.  The probe ranks that map 0.86% *worse*.

Two things make this hard to fix, and both are worth recording:

* **A real trial would not find it either.** The trial ledger scores the winning
  map at 2,425,690 bytes against the shipped map's 2,385,062 — it prefers the
  same wrong one — and then the file it actually produces is 2,240,400.  The
  ledger and the final encode disagree by 8%, because the map tips the
  downstream plane-coding search onto a different branch.  So the signal that
  distinguishes these maps is the final encoded size and nothing cheaper.
* **The direction does not generalise.** Biasing the ladder toward the more
  aggressive rung was built and measured.  Loosely (any rung within 1/16, ladder
  extended to `>>8`) it cuts at 1/256 and gives 2,591,364 — worse than shipping.
  Tightly (within 1/64, ladder unchanged) it lands exactly on 96 / 89 / 85 and
  takes `PIA13882` to 2,240,400 — but it takes `PIA13785` from 1,797,872 to
  1,842,636, +2.49%.  One file each way is a coin flip fitted to one file, so it
  was reverted rather than shipped.

What is left on the table is about 6% of `PIA13882` and an unknown amount
elsewhere, behind a selector problem that neither the probe nor the existing
trial machinery can answer.

---

## 5. What does *not* explain the gap

Two hypotheses that look compelling from a single file and die on the corpus.
Both are recorded because each would otherwise be the obvious next thing to build.

**Nonlinear cross-component structure.** `bmgstat` reports both `H(T|S)` (the
floor for reading one component out of another) and the best affine residual.
Their difference is what a *linear* cross-plane model cannot reach. On
`PIA13812` it is large: `H(B|G) = 3.65` against an affine residual of 5.53. On
`PIA13785`, `H(B|G) = 4.53` against 6.08. But summed over components and
correlated across the corpus it is **r = −0.045** against the gap. The eight
files with the most nonlinear headroom include BMF's largest win on the corpus
(`PIA13799`, −41.85%, 8.296 bpp of headroom) and its largest loss (`PIA13812`,
+26.79%, 5.081 bpp). GraLIC is not beating BMF by exploiting a nonlinearity BMF
cannot express.

**[corrected] That acquits the wrong defendant.** The correlation above asks
whether a nonlinear *value-domain* map between components would pay, and the
answer to that is still no. It says nothing about the cross-plane *channel*.
`claude/bmf-lpcb-performance-hdzfzy` settles that directly: split photographs
into single 8-bit planes and run both codecs on each plane and on the colour
image. On one real photographic plane BMF is at parity with gralic — mean gap
B +0.006%, R −0.256%, G +1.091% — while **71–84% of each photograph's outcome is
decided in the cross-plane channel**, and that channel sets the sign of the
result. Segment A's deficit is roughly 11–13 MB cross-plane against 2.5–4.6 MB
inside a plane. What is missing is a cross-plane predictor that sees a
*neighbourhood* rather than the co-located pixel; the fitted colour transform
itself is within 32 bytes of the coder's own optimum, and stacking a fixed RCT
on top costs 2–10%. C1 below is aimed at this and is badly undersized: it is
the largest item on the list, not the last.

**The plane-coding search.** BMF encodes the image three times to choose between
planar, transposed, and joint coding. On `STA13845` the three trials come in at
45,829,014 / 45,791,558 / 44,237,350 bytes — a 3.5% spread, and the best is
chosen. GraLIC codes it in 40,893,000. **The best decision BMF can make is still
8% behind**, so the search is not choosing wrongly; the thing being chosen among
is not good enough.

The instrumented build puts a number on what that search costs. On
`olympus_xz1_24` the zero/non-zero decision was coded 132,038,400 times over
10,156,800 × 3 = 30,470,400 samples, and on `canon_eos_1100d_01` 159,279,120
times over 36,756,720 — **4.33 coding passes per sample in both cases**. One of
those passes becomes the shipping stream; **77% of the arithmetic coding done at
encode time is discarded.** That is the budget §6 draws on.

`refs+p2` losing to plain `p2` on all three planes of `STA13845` is the same
point from another angle — the linear cross-plane taps are not paying for
themselves on continuous-tone material.

---

## 6. What to fix, in order

### A1 — Put the magnitude bucket through the mixer and the APM

**Reach: ~50% of the bit budget on the losing files. The single largest item.**

Replace the 8-ary arithmetic symbol with a binary decomposition, and give each
node the treatment stage 1 already has: a `Mix4` over the frequency estimate plus
adaptive companions, then an `Apm` keyed on the activity class. The bucket is
1.87 bits/event on `olympus_xz1_24`, so this is 2–3 binary decisions per sample,
against the ~1 that stage 1 already pays for.

Cost: decode roughly +40–60%. Well inside the stated budget (decode must not
become ~10× slower); the §5 finding that two-thirds of encode time buys a 3.5%
selection spread says where to reclaim it from if needed.

### A2 — Widen `CtxPair` to the context that already exists — **[corrected] refuted**

**This does not work, and the reason it does not is the useful part.**
`claude/bmf-lpcb-performance-hdzfzy` built all three of the axes proposed below
— the two flatness bits, quantised `|e_W|`/`|e_N|`, and the residual's sign —
each with the table enlarged so nothing collides, and every one is a wash or a
loss on realistic photographic magnitudes. A frequency strip has no parent
distribution and no mixing partner, so an added axis must repay its own learning
cost from its own counts, immediately. Dilution beats the extra information.
That is a general result about this coder, not a fact about these three axes:
**the way into the magnitude is mixing, not a bigger table**, which makes A1 the
item and A2 not a cheaper route to it. The rest of this subsection is left as
written because the diagnosis it starts from — that the stage is
under-conditioned — is still right; the proposed remedy is what fails.

Reach: same stage as A1.

`flat_a == 0`, `flat_b == 0`, and 2–3 bits of `mixer_fwd(ctx_w, 5) >> kP2MixShift`
are computed one statement above the point where `ctx_pair` is built. Adding them
turns 1024 contexts into 8192–32768:

```
Index CtxPair
 ADD %M%CtxDelta_Volume: delta
 ADD 16: ctx15
 ADD 2:  flat_a0
 ADD 2:  flat_b0
 ADD 8:  conf
```

`GL_model_table_bytes` must grow with `CtxPair_Volume` — it is already an IDX
parameter, and `kModelStrips` already derives from it, so this is a declaration
change plus an arena resize. Cost at decode: one wider index, no extra coding
work. **This is the cheapest item on the list and should be tried first**, both
because it is a few lines and because it calibrates how much of A1's benefit is
really about conditioning rather than about mixing.

Do A2 before A1: if widening the context alone closes a large part of the gap,
A1's design (which inputs the mixer should take) follows from what A2's contexts
turn out to be worth.

### A3 — An SSE on the within-bucket bits

**Reach: 12–21% of the budget, currently at 0.90–0.93 bits/event.**

The `FreqPair` tree in `code_symbol_tree` codes each bit straight off two counts.
An APM keyed on `(lvl, node, ctx15)` is one table lookup and one interpolation per
bit. Even a few per cent off a fifth of the budget is worth more than most of what
is left in the parameter space.

### B1 — Value-set renumbering as a pre-pass

**Reach: 32% of the corpus gap, concentrated in 8 files; up to 1.87 bpp on the
worst. [corrected] Measured at 5.37 MB** on `claude/bmf-lpcb-performance-hdzfzy`,
across all six candidates at full size — and **it must be a gated trial**:
without the gate `PIA13915` loses 3.24%, which is the most useful thing anyone
learned about it. The structural cause is named there too: the slow model calls
`reduce_alphabet`, and alt-P1 and alt-P2 never do.

Per component, scan the image, build the used-value set, and if `|S| < 256`
renumber to `0..|S|−1` before prediction. Ship the set as a 256-bit mask in the
plane header (32 bytes). Guard it on measured benefit — the histogram pass gives
the exact post-renumbering MED entropy, so the decision is made on a real number
rather than a heuristic, in the same style as the existing plane-coding trials.

Decode cost: one 256-byte lookup per sample on the way out, and none at all on
files where the mask is full. This is self-contained, does not touch the coder,
and is the only item here that can be validated against a figure `bmgstat`
already prints.

### C1 — Cross-plane reference as *context*, not as a predictor — **[corrected] this is item 1**

**Reach: 11–13 MB, the largest single item here.** It is placed last below
because §5 mis-scoped it; see the correction there. The plane-split experiment
puts 71–84% of every photograph's outcome in the cross-plane channel, and the
camera families carry 63% of the corpus deficit. Nothing in this list is worth
as much, and nothing in it has been costed as little.

BMF uses the reference plane through a least-squares affine fit and through
linear NLMS taps, both of which cap out at the affine residual. Feeding a
quantised reference value (16–32 buckets of the co-located reference pixel, or of
the reference plane's own residual) into `CtxPair` instead reaches `H(T|S)`
rather than the affine bound. Given §5, this is not a corpus-wide win and should
be scoped to the files where `bmgstat` shows the two diverging — but on those it
is 1.5–1.9 bpp, and with A2 in place it is another `ADD` line in the same Index.

---

## 7. The supplied numbers are already out of date

Two things happened to BMF after `lpcbs.txt` was measured, and one of them is the
regression you noticed.

### 7.1 The IMPROVEMENTS work did regress LPCB

I built the last commit before `IMPROVEMENTS.md` (`773b305`) and ran it against
ten of the files:

| file | pre-IMPROVEMENTS | `bmf18` (as supplied) | current | IMPROVEMENTS | current vs pre |
|---|---:|---:|---:|---:|---:|
| `sony_a55_07` | 11,648,956 | 11,678,856 | 11,429,704 | +0.257% | -1.882% |
| `canon_eos_1100d_01` | 9,521,964 | 9,541,004 | 9,382,732 | +0.200% | -1.462% |
| `olympus_xz1_24` | 11,613,220 | 11,627,768 | 11,589,720 | +0.125% | -0.202% |
| `PIA13785` | 2,003,528 | 2,004,844 | 2,003,904 | +0.066% | +0.019% |
| `STA13900` | 1,959,664 | 1,960,688 | 1,959,412 | +0.052% | -0.013% |
| `PIA13812` | 1,657,248 | 1,657,984 | 1,658,096 | +0.044% | +0.051% |
| `STA13456` | 1,104,496 | 1,104,940 | 1,105,444 | +0.040% | +0.086% |
| `STA13782` | 964,340 | 964,512 | 957,012 | +0.018% | -0.760% |
| `PIA13799` | 87,912 | 87,756 | 87,756 | -0.177% | -0.177% |
| `PIA13872` | 224,376 | 223,944 | 190,316 | -0.193% | -15.180% |
| **total** | **40,785,704** | **40,852,296** | **40,364,096** | **+0.163%** | **-1.034%** |

**The IMPROVEMENTS work cost 0.163% on this sample.** Scaled to the 997 MB
corpus that is about +1.6 MB — the regression you measured at ~2 MB, confirmed.

It is diffuse rather than structural. Diffing `bmf -v` on `STA13900` between the
two builds shows *identical* model selection (planar, then transposed; the same
predictor chosen for each slot) and per-plane trial costs that move by under 0.1%
in both directions — some planes better, more of them worse. Nothing chose
wrongly; the coder simply got marginally worse on large noisy planes.

**[corrected] It has a single cause, and it has been isolated.**
`claude/bmf-lpcb-performance-hdzfzy` bisected 25 revisions and attributes 98.6%
of the regression to `12b7be4`, the adaptive probability map on alt-P2's class
decision — tuned on one 705×800 synthetic image, and a net loss on every real
photograph. Deleting it lands slightly *under* the pre-IMPROVEMENTS build. The
same bisect measures the whole IDX/MOD port, all 26 commits, as **exactly
byte-neutral** on real photographs, which is the claim that series was built to
support. And the fix is not a revert: the sign patch of §7.2 is the same
machinery given the axis the map was missing.

The mechanism below is what I inferred before that bisect existed; it is
consistent with it but the bisect is the evidence. Those changes
added adaptive stages — APMs, a mixer — to the *zero flag* and to the tree bits
of the other models. On the internal 81-image corpus, where those stages carry
most of the budget, that won 7.1%. On a 10-megapixel camera photo the zero flag
is 17% of the budget and the new stages have to learn against near-stationary
noise: the indirection costs about what it saves, and the residual comes out
slightly negative.

### 7.2 The sign patch more than reverses it

| file | GraLIC | BMF (`bmf18`) | BMF now | sign patch | gap now |
|---|---:|---:|---:|---:|---:|
| `STA13845` | 40,892,954 | 44,226,712 | 43,756,032 | -1.06% | +7.00% |
| `PIA13812` | 1,307,611 | 1,657,984 | 1,658,096 | +0.01% | +26.80% |
| `olympus_xz1_24` | 11,267,251 | 11,627,768 | 11,589,720 | -0.33% | +2.86% |
| `PIA13785` | 1,771,064 | 2,004,844 | 2,003,904 | -0.05% | +13.15% |
| `sony_a55_07` | 11,291,367 | 11,678,856 | 11,429,704 | -2.13% | +1.23% |
| `canon_eos_1100d_01` | 9,252,904 | 9,541,004 | 9,382,732 | -1.66% | +1.40% |
| `STA13900` | 1,897,471 | 1,960,688 | 1,959,412 | -0.07% | +3.26% |
| `STA13456` | 1,070,622 | 1,104,940 | 1,105,444 | +0.05% | +3.25% |
| `STA13782` | 941,647 | 964,512 | 957,012 | -0.78% | +1.63% |
| `PIA13799` | 150,914 | 87,756 | 87,756 | +0.00% | -41.85% |
| `PIA13872` | 262,584 | 223,944 | 190,316 | -15.02% | -27.52% |
| **total** | **80,106,389** | **85,079,008** | **84,120,128** | **-1.13%** | **+5.01%** |

Against the pre-IMPROVEMENTS build the current codec is **−1.03%** on the ten-file
sample, so the regression is gone and then some.

The pattern is §3 again: the patch improves the sign, so it pays where the sign
is a large share of the budget (`canon_eos_1100d_01`, sign 26.0%, −1.66%;
`sony_a55_07`, −2.13%) and barely at all where the sign is small and already
near-incompressible (`olympus_xz1_24`, sign 19.8% at 0.986 bits/event, −0.33%).

That is the argument for §6 in miniature — **attaching adaptive machinery to a
stage moves the output roughly in proportion to that stage's share of the bits.**
It is also why §6 ranks the 50% stage above everything else: two consecutive
rounds of work have now landed on stages worth 17–26% of the budget on this
corpus, and one of them came out negative.

Caveat on the scaling: these eleven files were chosen as outliers plus family
representatives, not as a random sample, so the per-file figures are measured and
the corpus-scale extrapolations are estimates.

---

## 8. Method

* Sizes from the supplied `lpcbs.txt` (`gra11`, `bmf18`), 107 rows.
* Image statistics parsed from the 107 supplied `bmgstat` reports: value sets and
  remapped gain, `H(T|S)` and affine residuals, run lengths, neighbour equality,
  MED entropies, period strength.
* Model selection read from the 107 supplied `bmf -v` reports.
* Bit-budget figures from an instrumented build (accumulating `−log2(p)` at the
  four coding sites of `code_three_way` and `code_symbol_tree`) run on four
  images from the corpus tarball. The instrumentation lives outside the tree and
  is not part of the shipping codec.
* §4.1 measured directly: eighteen LPCB frames were retrieved and re-encoded
  with `./mk.sh release` before and after the two-tier map, and gralic 1.11's
  own demo binary was run on the same pixels under wine (PNM input) rather than
  read out of `lpcbs.txt` — it reproduces both the `lpcbs.txt` figure for
  `PIA13882` and the 256-crop figure, so the comparison is like for like.
* Re-encodes in §7 with the current `./mk.sh release` build, and with
  `773b305` (the last commit before `IMPROVEMENTS.md`) built in a detached
  worktree.
* Figures marked **[corrected]** come from `LPCB-REPORT.md` on
  `claude/bmf-lpcb-performance-hdzfzy`, which measures on the full corpus where
  this report measures on a sample, and which built three of the proposals here
  rather than costing them. Where the two disagree, that branch is right and
  this one says so at the point of disagreement.

Eleven of the 107 images were retrieved from the corpus tarball, so §7 is a
measured sample and not a corpus figure; §1, §2, §4 and §5 are over all 107.
