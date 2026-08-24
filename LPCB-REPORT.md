# Where gralic beats BMF on LPCB, and what to do about it

**Corpus.** 107 BMP images, `LPCB-bmp.tar.bz2` (3.46 GB raw). 25 `PIA*` and 20 `STA*`
NASA/ESA frames, plus 62 camera photographs from four bodies
(`canon_eos_1100d` ×11, `fujifilm_finepix_x100` ×12, `olympus_xz1` ×27, `sony_a55` ×12).

**Standings.**

| | bytes | bpp (corpus) |
|---|---:|---:|
| gralic 1.1 | 971 986 179 | |
| BMF (`claude/bmf-codec-build-test-nghv6u` @ `706fa6f`) | 997 191 732 | |
| **gap** | **+25 205 553** | **+2.593 %** |

Everything below was produced by running the codec. The build reproduces the reported
sizes exactly (`PIA13833` → 1 068 248 bytes, matching `lpcbs.txt` to the byte), so the
numbers in `bmfv/` and `lpcbs.txt` and the numbers measured here are the same coder.

Two changes were implemented and measured, not just proposed. Together they account for
roughly **20 of the 25 MB**:

| fix | what it is | measured |
|---|---|---|
| **A** | give alt‑P2's **sign** and **zero** decisions the sub‑pixel remainder that `pred = (run4+7)>>4` throws away | **−2.12 %** on full-size photographs (one per camera), **−1.10 %** over all 107 files on crops → **12–16 MB**; patch included, lossless |
| **B** | let alt‑P1/alt‑P2 **renumber a sparse value set**, as the slow model already does | **−11 % to −15 %** on the comb-histogram NASA files; free elsewhere |

The 2 MB regression has a single cause, isolated by a 25-revision bisect: commit
`12b7be4`, the adaptive probability map on alt‑P2's class decision, which was tuned on one
705×800 synthetic image and is a **net loss on every real photograph**. §7.

---

## 1. The corpus is two problems and a null

Segmenting by `remapgain_min` — the bits/pixel that densely renumbering the *used* value
set would save on the MED residual, from bmgstat's "value set and tone curve" block — cuts
the corpus almost perfectly:

| seg | definition | n | gralic | Δ vs gralic | rel | share of gap | BMF wins |
|---|---|---:|---:|---:|---:|---:|---:|
| **A** | camera photographs | 62 | 680.58 MB | **+15.887 MB** | +2.33 % | 63.0 % | 1 / 62 |
| **B** | NASA, `remapgain_min ≤ −0.05` (comb histogram) | 9 | 134.92 MB | **+9.001 MB** | +6.67 % | 35.7 % | 2 / 9 |
| **C** | NASA, clean 8-bit value set | 36 | 156.48 MB | +0.317 MB | +0.20 % | 1.3 % | 21 / 36 |

Segment C is parity — BMF wins 21 of 36 and the net is two tenths of a per cent. There is
nothing there to fix. A byte-weighted CART on `delta_bpp` picks the same variable first and
takes 51.7 % of the weighted variance in one cut at `remapgain_min ≤ −0.2142`.

By camera group:

| group | n | gralic | Δ | rel |
|---|---:|---:|---:|---:|
| olympus_xz1 | 27 | 275.90 MB | +8.384 MB | **+3.04 %** |
| PIA | 25 | 75.95 MB | +4.901 MB | **+6.45 %** |
| STA | 20 | 215.45 MB | +4.417 MB | +2.05 % |
| sony_a55 | 12 | 184.50 MB | +3.306 MB | +1.79 % |
| fujifilm_x100 | 12 | 107.54 MB | +2.838 MB | +2.64 % |
| canon_1100d | 11 | 112.64 MB | +1.360 MB | +1.21 % |

### The shape of the photographic loss

Within a camera the loss is close to a **fixed per-pixel tax**; across cameras the rate
changes threefold. The olympus set is the clean evidence: 27 frames, all exactly
10 156 800 px, `bmf_bpp` spanning 6.00 → 13.15 (2.2×), yet `delta_bpp` only 0.167 → 0.362,
cv 0.16.

| model of `delta_bpp` (n=62) | R² |
|---|---:|
| constant 0.1768 bpp | 0.000 |
| proportional to `bmf_bpp` | 0.123 |
| proportional to MED noise `med_bpp` | 0.211 |
| `a + b·std_mean` (noise level) | **0.002** |
| fixed fraction of total modelling gain `h0−bmf` | −0.255 |
| camera-group dummies | **0.580** |

So it is not proportional to output size, not proportional to noise, and not a fixed
fraction of the modelling gain. Noise level does not predict the loss at all.

---

## 2. Where BMF's bits actually go

The range coder was instrumented to attribute every coded bit to its call site
(`−log2 p` accumulated per site, dumped at `flush()`; the instrumented build produces
byte-identical output). alt‑P2 codes 93.6 % of the corpus, and on photographs its bits
split like this — full images, final encode, not trials:

| site | olympus_16 | canon_02 | sony_06 | fuji_03 | machinery it has |
|---|---:|---:|---:|---:|---|
| zero / non-zero | 45.8 % | 41.8 % | 51.5 % | 58.0 % | 4-input mixer **+ APM** |
| **residual sign** | **26.6 %** | **26.6 %** | **26.4 %** | **24.4 %** | **one frequency pair, nothing** |
| magnitude level (log bucket) | 25.9 % | 28.9 % | 21.2 % | 16.3 % | **one strip counter, nothing** |
| magnitude low bits | 1.7 % | 2.7 % | 0.9 % | 1.3 % | binary tree |

Cost per symbol of the sign bit: **0.9572 / 0.9694 / 0.9370 / 0.8867 bits**. A quarter of
everything BMF spends on this corpus goes on a decision it is coding at 89–97 % of a fair
coin — and it is the one decision in the codec that received none of the modern machinery.
IMPROVEMENTS.md §3's APM went on the *zero* decision only; §2's mixer likewise.

---

## 3. Fix A — the sign bit is not a fair coin

### The mechanism

`predict()` carries the NLMS prediction as `run4` in 16× fixed point.
`seat_symbol_context` (`alt_p2.inc:586`) rounds it once:

```c
const int32_t ctx_idx = clamp32((run4+7)>>4, 0, 255);
```

and everything downstream sees only the rounded byte. If `run4/16 = 100.9`, the coded
prediction is 101 and the residual `sample − 101` leans **negative** — by construction. The
four bits `(run4+7) & 15` state which way the prediction leaned inside the pixel it was
rounded to, and they are discarded.

They are discarded *everywhere*, which was checked rather than assumed:

* `ctx_delta[ctx_idx]` and `ctx_delta[ctx_idx+4]` index the **rounded** value.
* `ctx_w[0].sel = idx_bound(MakeRun4Sel(-run4), 3)` is a three-way **brightness** bucket
  with edges at `run4` = 272 and 1840, i.e. pixel values 17 and 115 (`IDX/bmf-P2.idx:497`).
* `ctx_w[1..4]` are band tests of `run4 − neighbour` at `band_lo/band_hi`, and neighbour
  ternary signs — all coarse.
* The five bias banks are keyed on quantised activity and 11 sign bits of differential
  tests.

Nothing in the codec is a function of `run4 mod 16`.

### The measurement, before touching the codec

Dumping `(frac, code, context, …)` for every coded sample of a 1024×1024×3 crop and running
a **sequential Krichevsky–Trofimov code length** (order-independent, learning cost included)
over the sign bit says a naive context split loses to dilution — as IMPROVEMENTS.md §3
predicted for exactly this reason. Refining a *coarse secondary stage* on `frac` is what
works. A real adaptive-coder simulator (counter → 2-input logistic mixer → interpolated APM)
gives, on the sign bit:

| crop | baseline (ctx only) | + `frac` stage | gain | **placebo** (same machinery, activity context, no `frac`) |
|---|---:|---:|---:|---:|
| fujifilm_x100_03 | 0.95583 | 0.84980 | **−11.32 %** | +0.20 % |
| sony_a55_06 | 0.98426 | 0.91050 | **−7.66 %** | +0.02 % |
| STA13845 | 0.99149 | 0.93255 | **−5.95 %** | −0.04 % |
| canon_1100d_02 | 0.99093 | 0.93720 | **−5.42 %** | +0.06 % |
| olympus_xz1_16 | 0.99830 | 0.93303 | **−6.54 %** | −0.18 % |
| PIA13912 | 0.98045 | 0.95012 | **−3.09 %** | +0.11 % |

The placebo — identical mixer and APM, secondary context = activity class × quantised
west-neighbour magnitude, same table size — moves nothing. The effect is the sub-pixel
fraction, not the extra stage.

### The implementation, and what it actually saves

`signfix.patch` (153 lines against `alt_p2.inc`, mostly comment) does two things:

1. keeps `(run4+7) & 15` and passes the **sign** decision's probability through a new
   interpolated `Apm` keyed on `(phase, coarse activity)` — 128 of the map's 256 contexts,
   exactly the shape IMPROVEMENTS.md §3 already uses for the zero decision;
2. adds the phase to the **zero** decision's existing map, replacing
   `apm_context() = MakeCellMap(ctx)` with `(phase, activity)` — which is what turns that
   map from a net loss (§7) into a net gain.

On 5 crops (3 photographs + the 2 largest comb-histogram NASA frames), against HEAD's
4 482 680 bytes: sign map alone 4 426 688 (−1.249 %), **both 4 423 008 (−1.331 %)**.

**Full-size LPCB files, byte-measured, one per camera:**

| file | gralic | BMF HEAD | BMF + fix A | vs HEAD | vs gralic |
|---|---:|---:|---:|---:|---:|
| canon_1100d_02 | 10 321 388 | 10 240 716 | 10 103 652 | −1.338 % | **−2.110 %** |
| fujifilm_x100_03 | 7 382 712 | 7 401 244 | 7 150 488 | −3.388 % | **−3.146 %** |
| olympus_xz1_16 | 7 364 246 | 7 621 760 | 7 490 140 | −1.727 % | +1.710 % |
| sony_a55_06 | 11 077 283 | 11 245 828 | 10 990 444 | −2.271 % | **−0.784 %** |
| **total** | 36 145 629 | 36 509 548 | **35 734 724** | **−2.122 %** | **−1.137 %** (was +1.007 %) |
| STA13453 | 9 262 192 | 8 758 720 | 8 196 244 | −6.421 % | −11.51 % |

The group goes from 1.0 % **behind** gralic to 1.1 % **ahead**, and three of the four files
now beat gralic outright.

**All 107 files, 1024² crops** (sign map only — the weaker of the two halves):

| group | n | HEAD | + fix A | Δ |
|---|---:|---:|---:|---:|
| STA | 20 | 15 886 304 | 15 538 572 | **−2.189 %** |
| fujifilm_x100 | 12 | 9 910 624 | 9 734 304 | −1.779 % |
| canon_1100d | 11 | 10 151 928 | 10 030 236 | −1.199 % |
| sony_a55 | 12 | 12 712 196 | 12 599 632 | −0.885 % |
| olympus_xz1 | 27 | 30 088 464 | 29 885 336 | −0.675 % |
| PIA | 25 | 14 874 720 | 14 810 680 | −0.431 % |
| **all** | **107** | 93 624 236 | 92 598 760 | **−1.095 %** |

Applying each file's crop ratio to its full size projects **−11.6 MB**, taking the corpus
from +2.593 % to **+1.403 %** against gralic. That is a floor on three counts: the crops
under-state (the same four files give −2.12 % at full size against −1.35 % on their crops),
this run used the sign map alone, and the crop sample is a 1 Mpx window of a 10–46 Mpx
image. The honest range is **12–16 MB**.

The fix costs bytes on **5 of 107** crops; the worst is sony_a55_11 at +1 244 bytes
(+0.06 %), then STA13456 +504, PIA13812 +100, olympus_xz1_21 +92, olympus_xz1_20 +80.

Verification: lossless round-trip on all ten `testfiles/` images and on the LPCB crops
tested. `signfix.patch` applies cleanly to a pristine `706fa6f` checkout and reproduces
these sizes to the byte.

Cost: one interpolated table lookup and update per non-zero residual (61 % of samples) — the
same per-symbol cost as the zero-decision map that already ships — plus 16.9 KB of table.
Decode measured at **+4 % to +13 %** over four paired runs on a loaded machine; the numbers
are noisy but the ceiling is set by the work added, and IMPROVEMENTS.md measured the
*existing* zero-decision map at 3.5 % of decode. Either way this is far inside the 10×
budget §0 of IMPROVEMENTS.md sets.

Context-shape sweep for the sign map (1024² crops, 11 files):

| APM context | all | photographic subset |
|---|---:|---:|
| phase alone (16) | −0.974 % | −1.410 % |
| phase × activity (256) | −1.030 % | −1.535 % |
| phase × plane × coarse activity (256) | −0.976 % | −1.413 % |
| **phase × coarse activity (128)** | **−1.035 %** | **−1.543 %** |

The choice barely matters; the phase itself is doing the work.

---

## 4. Fix B — the alt models code a sparse alphabet as if it were dense

Nine NASA frames have been through a global tone curve or a quantiser: 74–244 of the 256
levels used, spaced almost evenly with gaps of 2–3, fitted gamma 0.59–1.02. They carry
**+9.0 MB, 35.7 % of the gap.**

| file | remapgain_sum | min distinct | gaps | Δ | rel | models |
|---|---:|---:|---|---:|---:|---|
| PIA13812 | −1.869 | 120 | 2..3 | +0.350 MB | **+26.80 %** | p1, refs+p2, slow |
| PIA13882 | −1.119 | 177 | 1..8 | +0.286 MB | +11.72 % | slow |
| PIA13912 | −0.669 | 205 | 1..3 | **+3.653 MB** | +15.05 % | p2, refs+p2 |
| STA13845 | −0.257 | 228 | 1..2 | **+3.334 MB** | +8.15 % | p2 |
| PIA13785 | −0.283 | 159 | 1..4 | +0.234 MB | +13.20 % | p2, refs+p2 |
| STA13843 | −0.119 | 244 | 1..2 | +0.435 MB | +0.88 % | p2, refs+p2 |
| PIA13915 | −0.032 | 234 | 1..5 | +0.847 MB | +6.24 % | slow |
| PIA13815 | −0.468 | 83 | 1..4 | −0.076 MB | −6.15 % | slow |
| PIA13799 | −0.103 | 74 | 1..31 | −0.063 MB | −41.85 % | refs+slow, slow |

The cause is structural: the slow model renumbers the value set before coding
(`reduce_alphabet`, `model.inc:318`, called from `code_plane_slow`, and shipped as gap codes
by `reduce_narrow_alphabet`, `model.inc:214`) — **alt‑P1 and alt‑P2 never call it.** They run
on raw 8-bit values, so their contexts, their quantiser ladders and their magnitude buckets
are all spread over a comb.

(`alphabet_reduced` at `bmf_state.inc:24` is *not* this. It is an update-work gate for the
cost estimator, set at `codec.inc:299`, and suppresses alt‑P2's neighbour/mirror counter
updates during trials.)

Renumbering each component densely outside BMF and re-encoding (1024² crops), against the
same crops through the unmodified codec:

| crop | HEAD | + renumber | + fix A | + both |
|---|---:|---:|---:|---:|
| PIA13912 | 879 252 | −14.85 % | −0.48 % | **−16.25 %** |
| STA13845 | 982 704 | −11.33 % | −1.22 % | **−13.16 %** |
| PIA13785 | 1 433 216 | −3.00 % | −0.02 % | −3.09 % |
| canon_1100d_02 (control) | 988 524 | **+0.00 %** | −1.03 % | −1.03 % |
| olympus_xz1_16 (control) | 883 792 | **−0.00 %** | −1.27 % | −1.28 % |

The two fixes are additive to slightly super-additive, and renumbering is **exactly free**
where the histogram is dense — so it can be applied unconditionally, with the value set on
the wire (50–90 bytes; the slow model's gap coder already exists).

The `-v` trial logs localise it to the alt models. A separate 2000² crop of PIA13912,
per-plane trial costs, as-is → renumbered:

```
slot0 slow      1 997 104 -> 1 997 084   (-0.001 %)   <- slow already renumbers internally
slot0 p1        1 845 280 -> 1 689 164   (-8.46 %)
slot0 p2        1 806 360 -> 1 626 936   (-9.93 %)
slot1 refs+p2   1 466 529 -> 1 351 513   (-7.84 %)
slot2 refs+p2   1 124 151 -> 1 034 527   (-7.97 %)
```

There is a **second-order** effect worth naming. When the penalty is large enough the alt
models *lose the trial* to `slow`, which has no spatial prediction at all — it models
equality structure, not numeric value. PIA13882 and PIA13915 both end up entirely on `slow`
and then lose 6–12 % to gralic for that downstream reason. On the PIA13915 crop, renumbering
flips the choice from `joint alt-P1` (9.011 bpp) to `joint alt-P2 + refs` (8.633 bpp).

Projected from the measured per-file percentages, fix B is worth **≈ 8.8 MB, ≈ 35 % of the
gap**, and closes segment B essentially completely.

---

## 5. The structural gap: BMF selects where gralic mixes

This is the finding behind the residual photographic loss that fix A does not take.

Mining all 107 `-v` transcripts (321 per-plane searches on the adopted representation):

```
Σ_slots (second_best − best) = 30 838 922 B = 3.093 % of the coded corpus
                  gralic gap = 25 205 553 B = 2.528 %
```

**The amount by which BMF's runner-up model disagrees with its winner is 22 % larger than
the entire gap to gralic**, and 24.05 MB of it is alt‑P2 against alt‑P1 *on the same plane
with the same reference set*. The selection logic is not at fault — the printed `<<` is the
cheapest printed trial in all 648 slot searches the transcripts contain — the architecture
that forces a choice at all is what costs.

The per-file evidence is unusually clean. Define `gap% = Σ(second − best) / Σ(best)` over a
file's three plane searches:

| set | pearson(gap%, rel gap vs gralic) | partial, controlling `bmf_bpp` |
|---|---:|---:|
| all 107 | **−0.712** | −0.676 |
| 62 photographs | −0.693 | −0.710 |
| 45 NASA | −0.701 | −0.671 |
| each camera separately | −0.67 … −0.77 | |

Monotone across quintiles of the homogeneous camera subset:

| quintile of gap% | n | mean gap% | byte-weighted loss to gralic |
|---|---:|---:|---:|
| Q1 (tightest decisions) | 12 | 1.41 | **+3.38 %** |
| Q3 | 13 | 2.04 | +2.49 % |
| Q5 (widest decisions) | 13 | 3.15 | **+1.49 %** |

**Where BMF's two best models are nearly tied, BMF loses to gralic; where one model clearly
dominates, BMF is competitive.** A near-tie is exactly the condition under which a mixture
beats a selection. Only 44 of 321 decisions are within 1 %, so a *better chooser* is worth
nothing; the mass sits in the 1–5 % band.

### But size the prize from per-pixel costs, not from trial margins

The 30.8 MB figure is the wrong unit and it flatters the proposal — it is a difference of two
*totals*, and one model can dominate the other pixel-by-pixel while the totals differ by 3 %.
So the models were instrumented to emit exact per-pixel bit costs and the pooling was computed
directly, over 13 crops × 3 planes:

| per-pixel cost correlation | corr(p1,p2) | corr(slow,p2) | corr(slow,p1) |
|---|---:|---:|---:|
| olympus_xz1_21 p0 | 0.794 | 0.434 | 0.471 |
| STA13453 p1 | 0.506 | 0.299 | 0.498 |
| PIA13882 | — | — | 0.770 |

| what you build | measured gain |
|---|---:|
| region / row-level model switching (per-row oracle) | **−0.01 % to −0.56 %** |
| fixed-weight per-plane linear pool, `p1` + `p2` | −0.82 % |
| the same with references, `refs+p1` + `refs+p2` | **−1.50 %** (NASA −1.88 %) |
| three-model fixed pool | −1.54 % to −2.27 % |
| **per-pixel oracle (the ceiling for any pooling)** | **−14.7 % to −16.1 %** |

Three things follow. The models' errors *are* substantially independent — correlations of
0.30–0.80 and a 15 % oracle ceiling — so IMPROVEMENTS.md §2's claim is right. Its *evidence*
(trial margins) does not support it, and its *estimate* of "5–15 % overall" is high by
roughly an order of magnitude for the mechanism proposed: a defensible band for a real
context-conditioned logistic mixer is **2–5 %**, i.e. 30–80 % of the gralic gap on cameras.
And **region-adaptive model selection is refuted outright at ≤ 0.56 %** — do not build
tiling or block-level switching.

The cheap first step is not the full mixer. It is a **per-plane fixed-weight linear pool of
alt‑P1 and alt‑P2**, the two models `search_planes` already trial-encodes on nearly every
plane: `−log2(w·P₁(v) + (1−w)·P₂(v))` needs only each model's probability for the symbol it
is about to code, which both already compute; the weight is a few bits in the plane
descriptor (`codec.inc:1695` has 6 bits today with room for more). **−0.82 % / −1.50 %
measured**, no binarisation rewrite, and the mode search gets *smaller* because it no longer
has to choose. Two individual cases show what is on the table: PIA13894 p1 goes
slow 348 740 / p1 310 362 / p2 296 688 → 3-way pool **244 164 (−17.7 %)**, oracle −49 %.

One more thing worth knowing: `CtxModel` already contains **the codec's only logistic mixer**
(`Mix5`, `apm.inc:148`) **and its only match model** — both behind a gate no LPCB image can
pass (§7). Lifting `Mix5` into the alt models is worth more than anything done to `CtxModel`
itself.

---

## 6. Smaller defects, all confirmed in the source

**6.0 — Where the colour transform earns nothing, BMF loses most.** Nine files end with
**no reference planes at all** (5 STA, 4 PIA). They are the worst group in the corpus:
**+5.88 %** against gralic on 86.2 MB, versus +2.26 % for the 86 files with two referenced
planes. Two things are *not* the cause. The weight fit is essentially optimal — sweeping the
shipped `w=(112,−11)` on an olympus crop finds a best point only **32 bytes (0.0036 %)**
below what the encoder chose, so the order-0 gradient-domain fit, the coordinate descent and
the 1/128 quantisation are all fine. And a fixed RCT is much worse (§9). What is left is
*which* plane goes in *which* slot — decided by the order-0 gradient proxy on margins under
0.1 % on **39 of 107** files and under 1 % on **86 of 107**, and never checked by a trial
encode — and what the transform cannot express at all (6.1).

**6.1 — Slot 1's colour gain is hardwired to 1.0.** The slot‑2 plane gets fitted weights
(`plane_mix2`, `codec.inc:119`; `seed2 = (w1·O1 + w0·O0) >> 3`, `codec.inc:181`), but slot 1
is always `O1 − dc − O0`:

* external transform: `code_colour_plane` / `plane_transform`, `codec.inc:417`, `codec.inc:854`
* joint alt‑P1: `codec.inc:112`
* joint alt‑P2: `codec.inc:176` — `seed1 = 16*out[...]`

Over the 321 (target, reference) rows bmgstat reports, the best single-reference scale k in
1/128ths is **not 128 on 143 of the 278 scaled rows — 51.4 %** (k=32:17, 64:54, 96:66,
128:135, 160:6). `PlaneDesc` already carries `weight0/1/2` on the wire (`records.inc:71`) and
slot 1 leaves them unused. This is a small, contained change with an existing wire format.

**6.2 — The joint mode is adopted unmeasured on 57 of 107 files.** `codec.inc:1464`: when
every plane already chose alt‑P2 with refs, joint alt‑P2 is taken with no trial encode. Those
57 files are 53 of the 62 photographs and carry +13.69 MB of the gap. Consequence: all
per-plane model and reference decisions on them were made by *planar* trials and then applied
to a *joint* pass that was never priced. Direct evidence this is not free: on the 20 files
where the `joint alt-P2 + refs` variant *was* offered, turning refs back on won 12/20 and
recovered 322 412 B — the planar search under-assigns references for joint coding half the
time it gets the chance, and **there is no trial in the other direction at all**.

(This is also the whole explanation of the estimator's apparent +1.5 % corpus bias. Where the
winner was actually measured, the estimate is exact — median +0.15 %, one part in 700. The
+2.47 % on the unmeasured cohort is the size of the unmeasured joint gain, not error.
`pearson(err%, gap vs gralic) = +0.038`: **the estimator is not why BMF loses.**)

**6.3 — The transpose search costs about half the encode time for 0.110 % of the corpus.**
Tried unconditionally (`codec.inc:1344`, no guard); adopted on 43/107; `|transposed − planar|`
is under 0.1 % on 46 files. Worth keeping, but it is not where anything is. (`ALGORITHMS.md:296`
describes an early abort in this path that does not exist — no transcript shows one, and all
107 transposed blocks print all three slots.)

**6.4 — The degenerate-weight slack is a non-issue.** A degenerate colour form is taken on
19/107 files, on 16 of which it is strictly worse than the descended fit and wins only through
the 2048-byte slack — total estimated cost of all 16 takeovers, **8 564 bytes**, in a currency
that is not coded bytes. 14 of the 16 fitted weights were already within a few units of
`(128,0)`. Write it off.

**6.5 — YCoCg‑R is inert here.** The order-0 gate refuses the trial on 105/107 files (median
`y_est/cur_est` = 1.2254). Tried twice, adopted once (`PIA12811`, a file BMF already wins).

**6.6 — Pruning is not the problem.** `try_p2` is pruned on 34 of 648 slot searches and
`try_mode0` on 378, but alt‑P2 codes 93.6 % of the corpus anyway.

**6.7 — The model-choice trials measure a deliberately weakened alt‑P2.**
`choose_plane_coding` sets `alphabet_reduced = 1` (`codec.inc:299`) and it is cleared only
at `codec.inc:617`, *after* `search_filter` returns. Every `p2` / `refs+p2` trial therefore
runs with alt‑P2's bank diffusion disabled, while `slow`, `p1`, `refs+p1` and `refs+slow`
trials are unaffected — nothing else reads the flag. Removing the gate makes the trial
numbers land on the real coded size exactly (`t8g` 43 524.8 → 42 888.8 against a coded body
of 42 892; `x_ep` 329 587.0 → 328 455.0 against 328 456) with **byte-identical final output**
on all ten test files, so it is purely an estimation bug. The per-plane bias is 0.1–1.4 %
and up to 10.4 % on one plane, always against alt‑P2 — which is the model that wins 93.6 %
of this corpus. Correcting the bias flips 9 plane-slot decisions and is worth ~0.23 MB
directly; its larger value is that every downstream decision (transpose, planar-vs-joint,
YCoCg) is currently taken on a mis-priced alt‑P2. Fix: clear the flag before
`search_planes`.

**6.8 — The activity quantiser saturates on exactly this corpus.** `ctx15`'s 16-class
ladder spans a mean |residual| of 0 to about 0.81 pixel levels (`P2_magsum_cap = 960` over a
total tap weight of 74, in 16× units). The LPCB camera groups run 1.98–2.76 bits/sample,
which for a Laplacian is a mean |residual| of roughly 1.2–2.0 levels — above the top of the
ladder for the busy parts of those images. Measured occupancy on a high-rate plane: 73.1 %
of samples in class 15, 89.9 % in classes 14–15. The activity context, which is an axis of
the frequency lattice, the magnitude strip and both maps, is a constant over most of the
pixels that matter. A naive equal-population ladder is *not* the fix (it loses on
mid-activity content); a per-image fitted one, transmitted, would be — `nb_ctx[]` is already
rebuilt per plane at `alt_p2.inc:1123`.

**6.7b — The slow model's only value-conditioned context tier is dead on this corpus.**
`id3` is gated at `alphabet < MB_small_alphabet = 32` (`model.inc:716`), and the minimum
post-reduction alphabet on a non-reference LPCB plane is **67**. The slow model therefore has
no magnitude-aware context anywhere on this corpus. Its tables are only 3–25 % occupied
(`ctx_id1` capped at 13 600, peaks at 4 672; `ctx_id2` capped at 44 500, peaks at 14 884) —
there is no capacity pressure, the features are the limit. Raising the gate, or replacing
"the left symbol" with a coarse bucket of it, is the cheapest structural change available to
the slow model and it targets the escape path that eats 48 % of pixels on PIA13882.

**6.7c — The joint alt‑P1 driver is dead code here.** `alt_model_p1` (`codec.inc:56`) plus
~100 lines of cross-plane selector logic in `ctx_of` (`alt_p1.inc:132`) never execute in any
shipped stream on this corpus. Per-plane alt‑P1 beats alt‑P2 by 7–11 % on the comb files
while *joint* alt‑P1 loses to joint alt‑P2 on the one file it was measured on, which suggests
the joint variant's cross-plane selectors are worse than the single-plane ones they replace.
If the mixing work needs budget, this is the first thing to retire or fix.

**6.9 — The bias cascade is a net loss on the highest-rate content.** Disabling all five
banks costs +1.38 % on a mid-rate photographic proxy and +3.50 % on `x_ep`, but **gains
0.22 %** on the highest-rate test image. The read-out is an integrator whose gain anneals
only on quiet pixels (`|res| < 38` in 16× units = 2.4 levels); in a noisy region the
countdown never decrements, the rate stays at 2⁻⁵ and the bias tracks the noise. The olympus
group — the corpus's largest single loss at +8.38 MB and its highest-rate group — is exactly
that regime. Gating the read-out or the anneal on activity class is the obvious experiment.

---

## 7. The 2 MB regression: one commit, and why nothing caught it

Building `49a0a08` (the commit that *adds* IMPROVEMENTS.md, i.e. before any of the work it
proposed landed) and comparing against `706fa6f` on 1024² crops:

| group | n | HEAD vs 49a0a08 |
|---|---:|---:|
| NASA (PIA+STA) | 45 | −0.006 % |
| canon_1100d | 11 | **+0.127 %** |
| fujifilm_x100 | 9 | **+0.137 %** |

HEAD is larger on 46 of the 65 crops measured. Same direction, same class as the 2 MB you
measured on the full corpus.

### Which commit

25 revisions between `49a0a08` and HEAD were built and run over three photographic crops
(canon_1100d_02 + fujifilm_x100_03 + olympus_xz1_16, 2 618 104 bytes at the base):

| revision | | total | Δ |
|---|---|---:|---:|
| `49a0a08` … `3c39da6` | orientation, palette, YCoCg, mirrors, estimator fix | 2 618 104 | — |
| **`12b7be4`** | **an adaptive probability map on alt‑P2's class decision** | **2 620 688** | **+2 584** |
| `3e95d7b` … `eb2d6c9` | APM elsewhere, alt‑P1 split, candidate map, context model ×4 | 2 620 688 | 0 |
| `7281bdb` | mix two coarse views of alt‑P2's class decision | 2 620 616 | −72 |
| `c26ef88` | price the context model's neighbourhood | 2 620 616 | 0 |
| **`ff2d3dd`** | **dual-rate counter for alt‑P2** | **2 620 724** | **+108** |
| `8f036a3` … `bf6bcaf` (19 commits) | strips, state machine, **and the whole IDX/MOD port** | 2 620 724 | **0** |
| `706fa6f` (HEAD) | | 2 620 724 | 0 |

**`12b7be4` accounts for 98.6 % of the regression.** Two things follow that are worth saying
plainly:

* The IDX/MOD parameter-extraction series — 26 commits, the single largest chunk of work —
  is **exactly byte-neutral**, to the byte, on real photographs. That claim holds.
* Everything between `ca8f7d2` and `eb2d6c9` that is about small-alphabet planes, palettes,
  orientation or YCoCg is likewise exactly zero here, because none of it can fire (below).

### Why the APM commit loses

Ablating the shipped refinement stack on the same three crops:

| build | total | vs HEAD |
|---|---:|---:|
| HEAD | 2 620 724 | — |
| HEAD − the zero-decision APM | 2 618 056 | **−2 668** |
| HEAD − APM and mixer both | 2 618 096 | −2 628 |
| `49a0a08` (pre-IMPROVEMENTS) | 2 618 104 | −2 620 |

Deleting the APM does not merely undo the regression, it lands slightly *under* the
pre-IMPROVEMENTS build. IMPROVEMENTS.md §3 reports the map as "1 444 bytes on `x_ep`,
−0.44 %" — and `x_ep` is a 705×800 32-bit synthetic image. Its context search ranged over
the *coarseness* of one axis ({16, 64, 256, 1024} activity classes) and never over **which
axis**. On continuous tone the activity class is already an axis of the 15 552-cell
frequency lattice the map is correcting, so the map divides evidence without adding
information — exactly the failure mode §3 itself warns about, applied to the one workload
that was not in the test set.

**The fix is not to revert it.** It is to give the map the axis it is missing: fix A above
replaces `apm_context() = MakeCellMap(ctx)` with `(phase, activity)` and adds a second map
on the sign. Same machinery, same cost, opposite sign of result: −0.1 % becomes −1.6 %.

### Why nothing caught it

**Almost none of the improvement work can fire on LPCB at all**, so the corpus only ever
sees the changes' side effects:

| feature added | gain claimed on `testfiles/` | files on LPCB where it can apply |
|---|---:|---:|
| direct context model for ≤16-value planes (§7) | **−4.9 % of that corpus** | **0 of 107** |
| — four mixed context orders, tree-bit APM, count-adaptive counters | (all inside it) | 0 |
| — match model as a fifth mixer input | (inside it) | 0 |
| palette coded instead of stored raw (§5.1) | −1.5 % on `t8p` | **0** (no palettised file) |
| luminance palette ordering | trial | 0 |
| YCoCg‑R lifting | gated trial | 2 tried, 1 adopted |
| mirror search | 0.04 % | 0 (removed from the path) |
| APM on alt‑P2's zero decision (§3) | −0.44 % of that corpus | 107 — **the only one that reaches this workload** |

This was checked, not assumed. The **minimum** distinct-value count over all components of
all 107 LPCB files is **74** (`PIA13799`); no file has any component with ≤ 16 or even ≤ 64
distinct values; none is palettisable; and the string `ctx` appears in **zero** of the 107
`-v` transcripts. The single largest improvement in the series — worth −4.9 % on the
development corpus — is never even offered on a single LPCB image.

Meanwhile `testfiles/` is ten images, of which `t1` (320×240 bilevel), `t8g`/`t8p` (320×240),
`t24`/`t32` (320×240), `f05_200` (2-value), `x_ai` (8-value) and `x_ci` (4-value) are small
or small-alphabet or both. The parameters and the design decisions are fitted to that set.
LPCB is 62 ten-to-sixteen-megapixel continuous-tone photographs with 200+ levels per channel.
A −0.44 % measured on ten images, of which one is `+0.17 %`, does not survive transfer to a
workload none of them resembles.

The same trap caught this investigation. One line of work here reached the sub-pixel-phase
idea independently and measured it on *constructed* proxy images with the right bit rate but
synthetic iid noise. It predicted **−0.15 %**. On the real LPCB photographs the same change
measures **−2.0 %** — thirteen times larger. Proxies with the right entropy but the wrong
spatial texture do not stand in for this corpus.

**The actionable conclusion is not "revert something".** It is that the codec has no
validation corpus that represents its own benchmark. Adding one is cheap: 1024×1024 centre
crops of all 107 LPCB files compress in ~10 s each and reproduce every qualitative
conclusion in this report (they under-state fix A by about 25 %, and they are exact on
which files use which model). That set is 340 MB and runs end to end in about 20 minutes.

---

## 8. What to do, in order

| # | change | expected | effort | risk | evidence |
|---|---|---:|---|---|---|
| 1 | **Fix A** — sub-pixel phase on the sign and zero maps (`signfix.patch`) | **−12 to −16 MB** | done: 153-line diff, one file | very low; 5 of 107 crops lose, worst +0.06 % | **measured end-to-end on all 107** |
| 2 | **Fix B** — alphabet renumbering for alt‑P1/alt‑P2 | **≈ −8.8 MB** | moderate: per-plane value set on the wire, decoder mirror; `reduce_narrow_alphabet` already exists | low; exactly free on dense histograms | **measured** (external bijection) |
| 3 | Build the LPCB crop corpus into `check.sh` and re-tune | unknown, plausibly ≥ −1 % | low | none | see §7 |
| 4 | Give the **magnitude level code** the same treatment as §3 gave the zero decision: binarise it and put a map on each bit | some of the 21–29 % it spends | moderate; it is shared `code_symbol_tree` machinery | medium | bit accounting §2 |
| 5 | Fitted weight on the **slot‑1** plane (§6.1) | small but broad | low; wire format already carries it | low | 51.4 % of rows want k ≠ 128 |
| 6 | Price the **joint** mode, and offer reference removal as well as addition (§6.2) | ~0.3 MB shown, likely more | low | low | 12/20 adoptions won 322 KB |
| 7 | **Per-plane fixed-weight pool of alt‑P1 and alt‑P2** | **−0.82 % / −1.50 %** ≈ −8 to −13 MB | moderate; both models already compute the probability, weight goes in the descriptor | medium; 2× decode on the final pass | **measured** per-pixel, §5 |
| 8 | Context-conditioned logistic mixer (the real IMPROVEMENTS §2) | 2–5 % projected; oracle ceiling is 15 % | large; needs the shared binarisation first | high, and the decode budget is the constraint | §5 |
| — | ~~region / tile-level model switching~~ | **≤ 0.56 %** | — | — | **refuted**, §5 |

Items 1–3 are days of work for the large majority of the gap. Items 7–8 are the structural
answer; 7 is affordable now and 8 is the one IMPROVEMENTS.md correctly puts last.

---

## 9. What is not the problem

Worth stating explicitly, because several are plausible and all were tested:

* **Model selection.** The `<<` is the cheapest printed trial in 648 of 648 slot searches.
* **The cost estimator.** Where it measured the winner it is accurate to one part in 700,
  and its error does not correlate with the gralic gap (r = +0.038).
* **The colour transform, of the fixed kind.** Pre-applying a reversible RCT (B−G, G, R−G)
  to 2000² crops and re-encoding *costs* +2.17 % (olympus), +3.80 % (fuji), +6.35 % (sony),
  +9.71 % (canon). BMF's own fitted `refs` transform beats a fixed RCT on every camera.
* **Noise level.** `delta_bpp` against `std_mean` over the 62 photographs: R² = 0.002.
* **Container overhead, palettes, alpha, near-lossless.** All 107 files are 24 bpp / 3 planes,
  none takes the ≤4 bpp short path, none is palettised, none has alpha. `bmf − body == 20`
  on all 107 rows exactly; container plus every scrap of side information is
  107×20 + 597 B of descriptors + ≈1 100 B of range-coder framing ≈ **3.8 KB, 0.00039 %** of
  the corpus — one part in 6 600 of the gap.
* **The colour weight fit, the coordinate descent and the 1/128 quantisation** — 32 bytes
  from the coder's own optimum on the one image where it was swept directly (§6.0).
* **The `kCtxId3Limit` cliff** and the other 1999-era table caps: measured at 4 bytes on the
  file that hits them. The slow model's tables run 3–25 % occupied on this corpus.
* **Region / tile / row-level model switching.** A per-row oracle over all three models is
  worth −0.01 % to −0.56 %. The per-plane choice the encoder already makes is close to the
  per-row optimum.
* **A run mode.** Ablated at ≤ 1.5 %; it is a decode-speed feature, not a ratio feature.
* **A match model.** Only 11 of 107 files have any duplicate rows, and 8 of those 11 are files
  BMF already wins.
* **Widening the magnitude strip's context.** Three widenings were built with the table
  enlarged so nothing collides: +2 flatness bits, +quantised |e_W|,|e_N|, +the residual's
  sign. All are a wash or a loss on realistic photographic magnitudes. A strip has no parent
  and no mixing partner, so a new axis must pay its own learning cost immediately — which is
  the mechanical reason the way in is mixing, not a bigger table.

---

## 10. Open questions, and what would settle them

* **Which commit costs the photographic 0.13 %.** A 27-revision sweep over three
  photographic crops is running; see the table appended below when it completes.
* **Why olympus specifically.** Camera identity explains 58 % of the photographic variance
  and nothing measured explains camera identity. `ctgain_bpp` (residual inter-channel
  redundancy) orders the four camera means at r = +0.854, but the *within*-camera partial
  correlation is −0.175 — the wrong sign. Treat it as a lead, not a finding. One suggestive
  detail: olympus is the only camera where the fitted two-reference weight collapses to
  `w=(128,0)`, on 9 of 27 files (canon 0/11, sony 0/12, fuji 1/12).
* **`STA13454`** (+0.541 MB, +3.92 %) is the largest loss with no explanation at all: clean
  256-level planes, `remapgain` 0, no unusual spatial or colour statistic. Same for STA13452,
  STA13455, STA13457.
* **Whether fix B survives at full scale.** Four of the six renumbering measurements are on
  crops that encode at 7.6–8.5 bpp against 6.0–9.8 bpp for the full files. `PIA13785` was
  measured whole (−7.39 %); `PIA13912` and `STA13845` at full size are 110 MB and 139 MB BMPs
  and were not run.
* **gralic's actual mechanism.** Inferred here only from where it wins. Nothing in this
  report depends on a claim about gralic's internals.

---

## Appendix — reproducing this

```
# corpus
curl -O https://fish.seedhost.eu/shelwien/downloads/1/LPCB-bmp.tar.bz2
tar -xf LPCB-bmp.tar.bz2

# codec
git clone -b claude/bmf-codec-build-test-nghv6u <repo> bmf && cd bmf && ./gc.sh

# fix A
git apply ../signfix.patch && ./gc.sh
```

`crop.py` builds the 1024² validation corpus; `bench.sh` runs a binary over it.
Bit accounting is a 40-line patch to `rangecoder.inc` (`−log2 p` per call site, keyed by a
scoped `bmf_site`); it does not change the coded stream.
