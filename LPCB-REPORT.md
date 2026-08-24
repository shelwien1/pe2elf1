# Where gralic beats BMF on LPCB, and what to do about it

**Corpus.** 107 BMP images, `LPCB-bmp.tar.bz2` (3.46 GB raw). 25 `PIA*` and 20 `STA*`
NASA/ESA frames, plus 62 camera photographs from four bodies
(`canon_eos_1100d` ×11, `fujifilm_finepix_x100` ×12, `olympus_xz1` ×27, `sony_a55` ×12).

**Standings.**

| | bytes | vs gralic |
|---|---:|---:|
| gralic 1.1 | 971 986 179 | — |
| BMF (`claude/bmf-codec-build-test-nghv6u` @ `706fa6f`) | 997 191 732 | +25 205 553 (**+2.593 %**) |
| BMF + `signfix.patch` (this branch) | **984 132 352** | +12 146 173 (**+1.250 %**) |

Everything below was produced by running the codec. The build reproduces the reported
sizes exactly (`PIA13833` → 1 068 248 bytes, matching `lpcbs.txt` to the byte), so the
numbers in `bmfv/` and `lpcbs.txt` and the numbers measured here are the same coder.

**Three headline results, all from running the codec rather than reading it.**

1. **A fix that is written, and measured on every file in the corpus.** alt‑P2 throws away
   the sub‑pixel remainder of its own prediction (`pred = (run4+7)>>4`) and then spends a
   quarter of the corpus coding a residual sign at 0.89–0.97 bits. Feeding those four bits
   back is worth **13.06 MB — the corpus goes 997 191 732 → 984 132 352, and the gap to
   gralic +2.593 % → +1.250 %.** BMF now wins 43 of 107 files instead of 24, and beats gralic
   outright on the STA and canon groups. 153 lines in one file, lossless, decode unchanged.
   §4.
2. **A second fix, measured at full size by an independent line of work.** alt‑P1 and
   alt‑P2 never renumber a sparse value set; the slow model does. Nine NASA frames have
   comb histograms and carry +9.0 MB. Renumbering recovers **5.37 MB** measured on all six
   candidates at full size — and one of them *inverts* without a trial gate, which is the
   most useful thing anyone learned about it. §5.
3. **Where the rest of it is.** Splitting five photographs into single 8-bit planes and
   running both codecs on each plane separately: on a single real plane **BMF is at parity
   with gralic** (mean gap B +0.006 %, R −0.256 %, G +1.091 %). **71–84 % of each
   photograph's outcome is decided in the cross-plane channel.** Segment A is not a 2‑D
   modelling problem. §6.

The 2 MB regression has a single cause, isolated by a 25-revision bisect: commit
`12b7be4`, the adaptive probability map on alt‑P2's class decision, tuned on one 705×800
synthetic image and a **net loss on every real photograph**. Fix A supersedes it — same
machinery, right context axis. §9.

Numbers are labelled by how they were obtained. **Full-size** means a whole LPCB file encoded
end to end; **crop** means a 1024² or 2048² centre window. Crops are a screen, not a source of
figures: of four crop-derived predictions for fix B, one undershot, one overshot by 2.5×, one
was close and **one had the wrong sign**; for fix A the 107-crop benchmark said −1.095 % where
the true corpus answer is −1.310 %. Fixes A and B, and the regression bisect's baseline, are
all full-size.

---

## 1. The corpus is two problems, a near-null, and a population nobody isolated

Segmenting by `remapgain_min` — the bits/pixel that densely renumbering the *used* value
set would save on the MED residual, from bmgstat's "value set and tone curve" block — cuts
the corpus almost perfectly:

| seg | definition | n | gralic | Δ vs gralic | rel | share of gap | BMF wins |
|---|---|---:|---:|---:|---:|---:|---:|
| **A** | camera photographs | 62 | 680.58 MB | **+15.887 MB** | +2.33 % | 63.0 % | 1 / 62 |
| **B** | NASA, `remapgain_min ≤ −0.05` (comb histogram) | 9 | 134.92 MB | **+9.001 MB** | +6.67 % | 35.7 % | 2 / 9 |
| **C** | NASA, clean 8-bit value set | 36 | 156.48 MB | +0.317 MB | +0.20 % | 1.3 % | 21 / 36 |

A byte-weighted CART on `delta_bpp` picks the same variable first and takes 51.7 % of the
weighted variance in one cut at `remapgain_min ≤ −0.2142`.

Two caveats on that table. **Segment C's "parity" is an averaging artifact**: the net
+0.317 MB is +1.622 MB of gross loss against −1.304 MB of gross win, 15 losers to 21
winners. The 1.62 MB of gross loss is 6.4 % of the deficit and is invisible in the net; no
statistic separates C's losers from its winners (`distinct_per_px` looked perfect on an
8-file subsample and gives r = −0.062 over all 36). And a **third population cuts across
the segmentation**: the seven files whose planes are coded by bare alt‑P1 lose **+6.18 %
byte-weighted** — worse than the p2-only (+6.05 %) or slow-only (+6.13 %) groups, and worse
than either segment. `PIA13833` is the sharp case: the only file under 2.5 bpp coded by bare
`p1`, losing +8.39 % at 1.016 bpp, while every low-rate neighbour uses `refs+slow` and wins
by 0.28 %–54 %. Refs were correctly declined by trial there and alt‑P2 lost on all three
planes, so this is alt‑P1 itself being the wrong tool, chosen correctly.

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

## 2. What gralic actually is, and why the speed defence does not hold

GraLIC 1.11 runs here: the original Win32 binary under wine reproduces the published LPCB
sizes **to the byte** on every file checked (PIA12811 → 231 133, PIA13803 → 5 062 879,
PIA13833 → 985 583, …) and decodes bit-exactly. So this is a runnable oracle, not a
reconstruction, and everything below is a measurement or a perturbation probe on it.

**The speed premise in IMPROVEMENTS.md §0 is inverted.** Over the LPCB corpus:

| | encode | decode | peak memory |
|---|---:|---:|---:|
| GraLIC 1.11 | **1 734 s** | **1 897 s** | 32 / 65 MB |
| `bmf -s` | 3 678 s | 3 039 s | 82 / 234 MB |
| `bmf -s -q9` (the shipped setting) | 11 725 s | 3 051 s | |

GraLIC is **2.12× faster to encode, 1.60× faster to decode, and uses ~3× less memory**, and
its entire model state is ≤2.58 MB of static `.bss`. An independent 2024 benchmark reports
the same shape. Two paired single-file runs on this machine, both reproducing their published
sizes to the byte:

```
olympus_xz1_16  10.2 Mpx photograph   gralic  enc 11.9 s  dec 14.2 s   7 364 246 B
                                      bmf     enc 93.9 s  dec 22.5 s   7 621 760 B
PIA13833         8.4 Mpx render       gralic  enc  5.8 s  dec  8.9 s     985 583 B
                                      bmf     enc 40.7 s  dec  1.6 s   1 068 248 B
```

— and gralic is running under wine, so those are pessimistic for it. On the photographs, the
population that carries 63 % of the deficit, gralic is **7.9× faster to encode and 1.58×
faster to decode** while producing a smaller file. (PIA13833 is the honest counter-example:
on a 1 bpp render BMF's cheap alt‑P1 path decodes 5.6× faster than gralic. BMF's decode cost
is model-dependent in a way gralic's is not.)

**No decode-speed budget explains the 2.6 % on photographs.** IMPROVEMENTS.md §0 rules out
several proposals on the grounds that decode "must stay in the same class it is now" — but
the coder that beats BMF is already in a *better* class on both axes there, with a fortieth
of the memory. The constraint is reasonable as a design preference; it is not what is costing
the bytes, and it should not by itself veto a proposal.

**What it does**, from perturbation probes: planar, three sequential per-plane segments; one
pass; **no trial search at all**, where BMF runs ~30 whole-image trial encodes per file; the
plane coding order is chosen from content and varies per image (G,R,B / G,B,R / B,R,G all
observed) yet the output is *exactly* invariant to channel permutation; inter-plane
decorrelation is worth 21 % to it but is **statistical, not exact** — on artificially
duplicated planes BMF wins; the encoder **fits and transmits per-plane parameters** (a
single-pixel flip relocates the first stream divergence into a segment header); no long-range
match model; near-isotropic without a transpose trial. Static analysis of the binary
(`.text` 397 kB, `.bss` 2.58 MB, constant fingerprint `0xfff`×1351, `0x7ff`×1394,
`shr r,12`×1628) says lpaq-style 12-bit logistic probability domain and a carryless range
coder, and `.data` holds **nine different hard-coded quantiser ladders** — parallel context
resolutions where BMF has one.

**Two findings that aim the work.**

*The deficit is in the low-residual regime.* On an olympus crop, adding ±1…±8 of noise makes
**BMF win**; smoothing the same crop makes BMF **6.5–11.9 % worse**. Whatever BMF is missing
is not entropy coding of noisy residuals — it is prediction where the residual is small.

*The gap is the size of one of gralic's own release increments.* GraLIC 1.9 → 1.11, three
months of work in 2011, bought 1.2–2.3 % on exactly the camera images while regressing 18 %
on a synthetic PIA crop. That is the same magnitude and the same population as BMF's current
deficit, which argues for targeted photographic refinement rather than a different
architecture.

**Where BMF actually stands.** On the STA group BMF is second best in the world (+1.9 %). On
PIA, **PAQ8im beats GraLIC by 8.0 %** and ZPAQ by 3.9 % — so BMF's +6.5 % there is a
missing-general-CM problem, not a gralic trick, and §5's alphabet fix addresses most of it.
The genuinely gralic-specific gap is the camera groups (olympus +2.9 %, fuji +2.5 %), where
**gralic beats everything, PAQ8im included**.

**The named mechanisms BMF lacks**, anchored on Rhatushnyak's one open-source descendant —
the JPEG XL weighted/self-correcting predictor, `libjxl/.../context_predict.h`, attributed to
him: four sub-predictors blended by **inverse-recent-error** weights; error fed back into the
predictors themselves; **fractional bits of prediction precision carried into the model**;
sign-conditional clamping to the neighbour range; and the predictor's own **`max_error` used
as a modelling context**. BMF has the error feedback (the five-bank cascade is exactly that,
and is its largest component) and has error-*energy* contexts. It does not have structurally
distinct sub-predictors, sign-conditional clamping, `max_error`, or — until fix A — any use
of its own fractional precision. **Fix A is one item from that list**, which is some evidence
the list is the right one.

*Not established:* the mixer and SSE attributions rest on constant fingerprints, not on a
decompiled loop. encode.su was unreachable (Cloudflare 403 to every tool), so the author's own
threads were not read.

---

## 3. Where BMF's bits actually go

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
everything BMF spends on this corpus goes on a decision it codes at 89–97 % of a fair coin,
out of a raw pair of counts. Add the magnitude level and **about half the corpus is coded by
the two decisions that got none of the modern machinery**: IMPROVEMENTS.md §3's APM went on
the *zero* decision only, and §2's mixer likewise. Fix A is what happens when the sign gets
the same treatment; the magnitude is still waiting (§10, item 7 territory).

Both stages sit *downstream* of the prediction, which is worth keeping in view: §2's
noise-versus-smoothing probe says BMF's deficit is in the **low-residual** regime, so a
better coder for large residuals was never going to be the answer. Fix A works because the
sub-pixel phase is a statement about the *prediction*, not about the residual.

---

## 4. Fix A — the sign bit is not a fair coin

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
   map from a net loss (§9) into a net gain.

On 5 crops (3 photographs + the 2 largest comb-histogram NASA frames), against HEAD's
4 482 680 bytes: sign map alone 4 426 688 (−1.249 %), **both 4 423 008 (−1.331 %)**.

**Every file in the corpus, encoded end to end.** Not a projection, not a crop:

| group | n | gralic | BMF HEAD | BMF + fix A | vs HEAD | HEAD vs gralic | **+fix A vs gralic** |
|---|---:|---:|---:|---:|---:|---:|---:|
| STA | 20 | 215 452 127 | 219 869 144 | 215 123 376 | **−2.158 %** | +2.050 % | **−0.153 %** |
| fujifilm_x100 | 12 | 107 541 044 | 110 379 216 | 108 025 808 | −2.132 % | +2.639 % | +0.451 % |
| canon_1100d | 11 | 112 639 020 | 113 998 788 | 112 479 188 | −1.333 % | +1.207 % | **−0.142 %** |
| sony_a55 | 12 | 184 498 637 | 187 804 432 | 185 926 516 | −1.000 % | +1.792 % | +0.774 % |
| olympus_xz1 | 27 | 275 904 889 | 284 288 580 | 282 225 920 | −0.726 % | +3.039 % | +2.291 % |
| PIA | 25 | 75 950 462 | 80 851 572 | 80 351 544 | −0.618 % | +6.453 % | +5.795 % |
| **all** | **107** | **971 986 179** | **997 191 732** | **984 132 352** | **−1.310 %** | **+2.593 %** | **+1.250 %** |

**13 059 380 bytes.** The corpus gap halves, from +2.593 % to +1.250 %; the photographs go
from +2.334 % to +1.186 %; BMF now wins **43 of 107 files instead of 24**, and beats gralic
outright on the STA and canon groups. Smaller on 95 files, larger on 7, identical on 5. The
worst regression in the corpus is **sony_a55_11 at +18 372 bytes (+0.059 %)**, then
olympus_xz1_21 +0.013 %; nothing else exceeds +0.05 %.

The group ordering is itself a result, and not the one I expected. The fix pays *least* on
olympus (−0.726 %), which is the group with the largest deficit (+3.04 %) and the highest bit
rate (8.29 bpp mean); it pays most on the low-rate groups. That is the same signal as gralic's
own noise-versus-smoothing probe (§2): **the sub-pixel phase carries information in proportion
to how well the pixel was predicted**, so it recovers the least exactly where the residuals
are largest. Fix A does not fix olympus, and nothing in this report does.

For the record, the crop benchmark predicted −1.095 % where the true answer is −1.310 %,
and got the group *ordering* right except for swapping fuji and STA. As a cheap screen it
works; as a source of numbers to quote it does not.

Verification: lossless round-trip on all ten `testfiles/` images and on the LPCB crops
tested. `signfix.patch` applies cleanly to a pristine `706fa6f` checkout and reproduces
these sizes to the byte.

Cost: one interpolated table lookup and update per non-zero residual (61 % of samples) — the
same per-symbol cost as the zero-decision map that already ships — plus 16.9 KB of table.
Decode on the full 10 Mpx `olympus_xz1_16`, three paired runs on a quiet machine:
**+2.0 %, −8.3 %, +3.8 %** — i.e. inside the noise, and bounded above by the work added
(IMPROVEMENTS.md measured the *existing* zero-decision map at 3.5 % of decode). Nowhere near
the 10× budget §0 of IMPROVEMENTS.md sets.

### Why it works, and how it ties to §6

If §6 is right that the photographic deficit lives in the cross-plane channel, then a fix
that recovers sub-pixel precision should pay *more* on colour than on isolated planes — because
the inter-plane seed `(w1·O1 + w0·O0) >> 3` produces a genuinely fractional prediction almost
every time, where a purely spatial predictor over integer neighbours often lands on or near an
integer. Rounding destroys information that exists *because* the prediction is a blend.

Extracting each 1024² crop's three components as independent 8-bit greyscale images and
running fix A on them, against the same pixels coded as colour:

| crop | Σ 3 planes, HEAD | + fix A | Δ | same pixels as RGB, HEAD | + fix A | Δ |
|---|---:|---:|---:|---:|---:|---:|
| olympus_xz1_16 | 1 090 044 | 1 085 928 | **−0.378 %** | 883 792 | 872 296 | **−1.301 %** |
| canon_1100d_02 | 1 093 104 | 1 086 172 | −0.634 % | 988 524 | 978 100 | −1.055 % |
| fujifilm_x100_03 | 980 936 | 971 776 | −0.934 % | 748 408 | 728 196 | −2.701 % |
| sony_a55_06 | 993 900 | 985 172 | −0.878 % | 864 204 | 849 636 | −1.686 % |

**Roughly 40–70 % of fix A's gain is cross-plane-specific** (mean ≈56 %), which is the
prediction the mechanism makes and a second, independent line of support for §6. It also
explains why the fix is worth so much less on the PIA group (−0.618 % measured at full size):
those files use references least.

Context-shape sweep for the sign map (1024² crops, 11 files):

| APM context | all | photographic subset |
|---|---:|---:|
| phase alone (16) | −0.974 % | −1.410 % |
| phase × activity (256) | −1.030 % | −1.535 % |
| phase × plane × coarse activity (256) | −0.976 % | −1.413 % |
| **phase × coarse activity (128)** | **−1.035 %** | **−1.543 %** |

The choice barely matters; the phase itself is doing the work.

---

## 5. Fix B — the alt models code a sparse alphabet as if it were dense

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

Renumbering each component densely outside BMF and re-encoding **every candidate at full
size** (two of the six independently reproduced byte-for-byte by a second run):

| file | baseline | renumbered | recovered | Δ | % of that file's gap |
|---|---:|---:|---:|---:|---:|
| PIA13912 | 27 921 832 | 24 586 376 | +3 335 456 | **−11.95 %** | 91.3 % |
| STA13845 | 44 226 712 | 42 446 336 | +1 780 376 | −4.03 % | 53.4 % |
| STA13843 | 49 708 880 | 49 147 112 | +561 768 | −1.13 % | 129.0 % |
| PIA13785 | 2 004 844 | 1 856 780 | +148 064 | −7.39 % | 63.3 % |
| PIA13882 | 2 730 536 | 2 714 496 | +16 040 | −0.59 % | 5.6 % |
| **PIA13915** | 14 421 652 | 14 888 888 | **−467 236** | **+3.24 %** | **−55.2 %** |
| **total** | | | **+5 374 468** | | **61.1 %** |

**5.37 MB, 21.3 % of the deficit** — not the 8.8 MB a crop-based projection suggested. Of
the four predictions made from crops, one undershot, one overshot by 2.5×, one was close,
and **PIA13915 had the wrong sign**. On 1024² crops renumbering is *exactly free* on dense
histograms (canon +0 bytes, olympus −20), so the harm is bounded on the other 98 files, but
the size of the win is not knowable from a crop.

**PIA13915's inversion is the most useful result here**, because it is not a failure of
renumbering — it is a failure of *plumbing*, and it names the gate that has to change with
it. The baseline codes all three planes with `slow` ("joint, no alt model", 14 421 630).
Renumbering makes the alt trials cheaper, which reorders `choose_plane_coding`'s
gradient-entropy ranking, which changes the slot assignment, which lands plane 0 in a slot
where `out.n_p2` was already set — and `codec.inc:1749` then **never offers `slow` at all**.
`p1` wins at 5 840 088 against a `slow` cost of about 5.05 M for the same plane. So
renumbering must be a *trial*, not an unconditional transform, and the `slow` suppression
(§8.6b) has to go with it.

One thing this overturns: on STA13845 renumbering *unlocks* the colour transform. The
baseline declined references (`joint alt-P2 + refs` 44 295 998 against 44 237 350 plain);
renumbered, refs win (42 466 414 against 42 474 765). The comb histogram was suppressing the
cross-plane path too, so "not a colour-transform failure" was wrong for that file.

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

Fix B is worth **5.37 MB measured, 21 % of the gap** — 5.4–6.0 MB once it is a gated trial
rather than an unconditional transform, since the gate turns PIA13915's −0.47 MB into zero.
It recovers 61 % of segment B, not all of it.

---

## 6. Segment A is a cross-plane problem, not a 2-D one

This is the single most informative experiment in the investigation, and it reverses the
obvious reading of §1.

Split a 2048² centre crop into its three 8-bit planes, run **both** codecs on each plane
separately and on the colour image, and compare. (The single-plane BMF runs were checked to
take the same code path as colour slot 0 — plain alt‑P2, no strips, no context model.)

| file | gap on RGB | gap on Σ 3 planes | BMF's cross-plane gain | gralic's | cross-plane share |
|---|---:|---:|---:|---:|---:|
| canon_1100d_02 | −133 518 (−3.43 %) | −33 643 (−0.82 %) | **−7.71 %** | −5.21 % | 74.8 % |
| canon_1100d_06 | +37 040 (+1.09 %) | +10 047 (+0.28 %) | −6.19 % | **−6.95 %** | 72.9 % |
| sony_a55_03 | +112 991 (+2.43 %) | +18 605 (+0.35 %) | −9.55 % | **−11.38 %** | 83.5 % |
| fujifilm_x100_05 | +114 627 (+3.16 %) | +30 554 (+0.67 %) | −19.00 % | **−20.96 %** | 73.3 % |
| olympus_xz1_24 | +161 366 (+3.46 %) | +46 438 (+0.79 %) | −18.35 % | **−20.46 %** | 71.2 % |
| STA13845 | +178 091 (+4.71 %) | **+407 253 (+10.94 %)** | −4.10 % | +1.60 % | −128.7 % |
| PIA13912 | +295 176 (+10.36 %) | **+380 156 (+10.78 %)** | −19.51 % | −19.21 % | −28.8 % |

**On a single real photographic plane BMF is at parity with gralic.** Mean single-plane gap
over the photographs: **B +0.006 %, R −0.256 %, G +1.091 %**. Seventy-one to eighty-four per
cent of each file's outcome is decided in the cross-plane channel, and that channel sets the
*sign*: on canon_02 — the one photograph BMF wins — BMF beats gralic on cross-plane by 2.5
points and wins the file; on the four losers it trails by 1.8–2.5 points.

The comb-histogram NASA files behave in exactly the opposite way: their single-plane gaps
(+7.5 % to +19.5 %) are *larger* than their colour gaps, and BMF's cross-plane machinery is
fine or better. That is the alphabet diagnosis of §5, confirmed mechanically from a
completely different direction.

So segment A's 15.9 MB splits roughly **11.3–13.4 MB cross-plane** against **2.5–4.6 MB
single-plane 2‑D**. Everything BMF does inside a plane is competitive; what it does
*between* planes is not.

**This is confirmed from the other direction by fix A.** §4 shows that recovering the
prediction's sub-pixel phase pays 40–70 % more on colour than on the same pixels as isolated
planes — because the inter-plane seed produces a fractional prediction almost every time and
a spatial predictor over integers does not. A fix aimed at the cross-plane channel behaves
like one; that is two independent measurements pointing at the same place.

Two things this does **not** mean. It is not that BMF's colour transform is mis-fitted — a
direct sweep of the shipped weight finds it 32 bytes from the coder's own optimum (§8.0), and
stacking a fixed RCT on top costs +2.2 % to +9.7 % (§11). And it is not the scalar gain on
slot 1 (§8.1, refuted by measurement). What is missing is a cross-plane predictor that sees a
*neighbourhood* rather than the co-located pixel — IMPROVEMENTS.md §8.1, which was written
off as "worth exactly nothing" on a 0.077 Mpx and a 0.56 Mpx synthetic image, scored as
order-0 entropy. On this evidence that dismissal should be reopened; IMPROVEMENTS §8.2 (a residual-domain
cross-plane context) was never assessed by anyone at all.

One detail nothing predicts: **G is consistently BMF's worst plane** on every photograph
(+0.78 % to +2.30 %) while B and R sit at parity. G is the plane BMF most often codes flat.

*Caveat:* gralic's standalone 8-bit path may not equal its in-colour per-plane path. The
STA13845 row — where gralic's colour mode is worse than its own Σ3 — suggests gralic's grey
path is if anything stronger, which would make its cross-plane advantage larger, not smaller.
The assumption-free statement stands on its own: on a single real photographic plane BMF is
within 0.3 % of gralic on B and R; on the same pixels as colour the gap is 1.1 %–3.5 %.

---

## 7. Where a mixer would and would not help

Mixing the models instead of choosing one is IMPROVEMENTS.md §2 and it is the recommendation
every line of this investigation converged on independently. It is also the one whose size
is most often overstated, including by me earlier in this report, so it gets measured twice
here: once the easy way, which is wrong, and once properly.

### The easy way, and why it flatters the proposal

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
context-conditioned logistic mixer is **2–5 %**, i.e. 30–80 % of the gralic gap on cameras —
and even that band is a projection, since the fixed-weight pools use post-hoc fitted weights
on 1024² crops of 13 files and require running both models on both sides. And
**region-adaptive model selection is refuted outright at ≤ 0.56 %** — do not build tiling or
block-level switching.

Set against §6, this whole avenue is also aimed at the wrong half of the problem: the pooling
gains are gains on *single-plane* modelling, which is where BMF is already at parity with
gralic. A mixer would help the NASA files (`refs+p1`+`refs+p2` is −1.88 % there) more than
the cameras that carry 63 % of the deficit.

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
pass (§9). Lifting `Mix5` into the alt models is worth more than anything done to `CtxModel`
itself.

---

## 8. Smaller defects, all confirmed in the source

**8.0 — Where the colour transform earns nothing, BMF loses most.** Nine files end with
**no reference planes at all** (5 STA, 4 PIA). They are the worst group in the corpus:
**+5.88 %** against gralic on 86.2 MB, versus +2.26 % for the 86 files with two referenced
planes. Two things are *not* the cause. The weight fit is essentially optimal — sweeping the
shipped `w=(112,−11)` on an olympus crop finds a best point only **32 bytes (0.0036 %)**
below what the encoder chose, so the order-0 gradient-domain fit, the coordinate descent and
the 1/128 quantisation are all fine. And a fixed RCT is much worse (§11). What is left is
*which* plane goes in *which* slot — decided by the order-0 gradient proxy on margins under
0.1 % on **39 of 107** files and under 1 % on **86 of 107**, and never checked by a trial
encode — and what the transform cannot express at all (7.1).

**8.1 — Slot 1's colour gain is hardwired to 1.0 — and fixing it is worth almost nothing.**
The structural fact is real. The slot‑2 plane gets fitted weights (`plane_mix2`,
`codec.inc:119`; `seed2`, `codec.inc:182`), but slot 1 is always a literal unit-gain
difference `side[i] = base[ofs] − dc − ref[ofs]` (`codec.inc:467`), and no `k ≠ 128` is
reachable for it in any path — external (`codec.inc:467`), joint alt‑P1 (`codec.inc:113`) or
joint alt‑P2 (`seed1 = 16*out[...]`, `codec.inc:176`). `PlaneDesc` carries `weight0/1/2` for
every plane (`records.inc:71`) and slot 1 leaves them unused.

bmgstat says the best single-reference scale k is **not 128 on 143 of 278 scaled rows**
(k=32:17, 64:54, 96:66, 128:135, 160:6), which makes this look like a multi-megabyte lever.
**It is not.** Patching `codec.inc:176` to make slot 1's gain sweepable (`w=128` reproduces
the stock stream byte-identically) and sweeping it in BMF's own currency, on the exact file
and channel pair carrying the largest predicted forfeit:

```
canon_eos_1100d_11 crop, G<-R, bmgstat k*=64
  w= 32  1,108,492     w= 96  1,103,052     w=128  1,102,772  (shipped)
  w= 64  1,104,828     w=112  1,102,792     w=136  1,102,700  <- best found
```

**The shipped w=128 is at the optimum**; bmgstat's k*=64 would cost **+2 056 B (+0.19 %)**,
and the best point anywhere in the sweep is −72 B (−0.0065 %). Same on `sony_a55_05`
(w=128 best). The reason bmgstat disagrees is a functional-form artifact: its parabola
`H(k) = H* + c(k−k*)²`, anchored on `(k*, H*)` and `(0, alone)`, is symmetric, so it returns
`H(128) = H(0)` identically whenever `k* = 64` — and 10 such rows carry 70 % of the apparent
prize. Worth **0.05–0.13 MB**, and only as a rider on a bitstream revision that is happening
anyway. The cross-plane deficit of §6 is real, but it is not this.

**8.2 — The joint mode is adopted unmeasured on 57 of 107 files.** `codec.inc:1464`: when
every plane already chose alt‑P2 with refs, joint alt‑P2 is taken with no trial encode. Those
57 files are 53 of the 62 photographs and 60.7 % of the corpus output. Consequence: all
per-plane model and reference decisions on them were made by *planar* trials and then applied
to a *joint* pass that was never priced. Direct evidence this is not free: on the 20 files
where the `joint alt-P2 + refs` variant *was* offered, turning refs back on won 12/20 and
recovered 322 412 B — the planar search under-assigns references for joint coding half the
time it gets the chance.

Two corrections to how this is easy to over-read. The cohort carries +13.69 MB of the gap,
but that is a *size*, not an attribution: at +2.311 % it loses **less** than the 50 measured
files (+3.032 %) and less than the corpus (+2.593 %). The measured value of pricing the joint
mode is about **0.12 MB**. And it is not true that nothing ever removes a reference —
`clear_flags_on_all()` (`codec.inc:451`) is the "joint, flags cleared" trial; it just never
runs on an alt‑P2 file, because it is gated `!ps.n_p2` (`codec.inc:1494`). It fires on 4 of
107 files and never wins. The conclusion survives; the claim needed narrowing.

(This is also the whole explanation of the estimator's apparent +1.5 % corpus bias. Where the
winner was actually measured, the estimate is exact — median +0.15 %, one part in 700. The
+2.47 % on the unmeasured cohort is the size of the unmeasured joint gain, not error.
`pearson(err%, gap vs gralic) = +0.038`: **the estimator is not why BMF loses.**)

**8.3 — The transpose search costs about half the encode time for 0.110 % of the corpus.**
Tried unconditionally (`codec.inc:1344`, no guard); adopted on 43/107; `|transposed − planar|`
is under 0.1 % on 46 files. Worth keeping, but it is not where anything is. (`ALGORITHMS.md:296`
describes an early abort in this path that does not exist — no transcript shows one, and all
107 transposed blocks print all three slots.)

**8.4 — The degenerate-weight slack is a non-issue.** A degenerate colour form is taken on
19/107 files, on 16 of which it is strictly worse than the descended fit and wins only through
the 2048-byte slack — total estimated cost of all 16 takeovers, **8 564 bytes**, in a currency
that is not coded bytes. 14 of the 16 fitted weights were already within a few units of
`(128,0)`. Write it off.

**8.5 — YCoCg‑R is inert here.** The order-0 gate refuses the trial on 105/107 files (median
`y_est/cur_est` = 1.2254). Tried twice, adopted once (`PIA12811`, a file BMF already wins).

**8.6 — Pruning is not the problem.** `try_p2` is pruned on 34 of 648 slot searches and
`try_mode0` on 378, but alt‑P2 codes 93.6 % of the corpus anyway.

**8.6b — The bare `slow` trial is suppressed for every later plane, stickily and in slot
order.** `codec.inc:1749`:

```c
if( out.n_p2 ) { best_cost = 0x7FFFFFFF; }        // slow is never probed
else           { best_cost = probe_plane(plane_i, try_mode0); ... }
```

Once *any* earlier-ordered slot has chosen alt‑P2, no later slot may try the slow model at
all. Whether plane P gets to try `slow` therefore depends on what a different plane picked,
and on the coding order the colour search happened to pick. It fires on **189 of 321 slots
across 99 files**. Mostly harmless — alt‑P2 wins nearly everywhere — but it is exactly the
gate that turned fix B's PIA13915 into a 3.24 % *loss* (§5), and it is one line.

**8.7 — The model-choice trials measure a deliberately weakened alt‑P2.**
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

**8.7b — BMF stops learning as the image grows; gralic does not.** A nested-prefix ladder on
`olympus_xz1_24` — the same 2048-px-wide data, truncated to 128, 256, 512, 1024, 2048 rows,
so content drift is controlled:

| rows | BMF bpp | gralic bpp | rel | G-plane BMF | G-plane gralic | rel |
|---:|---:|---:|---:|---:|---:|---:|
| 128 | 9.3287 | 9.1450 | +2.009 % | 3.5244 | 3.4630 | +1.773 % |
| 256 | 9.3171 | 9.0856 | +2.549 % | 3.5526 | 3.4895 | +1.808 % |
| 512 | 9.3314 | 9.0497 | +3.113 % | 3.6172 | 3.5537 | +1.787 % |
| 1024 | 9.3435 | 9.0383 | +3.377 % | 3.6668 | 3.5911 | +2.107 % |
| 2048 | 9.2847 | 8.9806 | **+3.387 %** | 3.6290 | 3.5488 | +2.259 % |

**BMF's rate is flat (9.329 → 9.344, +0.16 %) while gralic's falls monotonically
(9.145 → 9.038, −1.17 %) on identical nested data.** Both see the same content, so this is
not content drift, and it is not warm-up — warm-up would *shrink* the gap with size. It is
model-capacity saturation, and it is why the relative loss correlates with megapixels
(r = +0.867). About two thirds of the growth is in the joint path (RGB +1.38 points against
the G plane's +0.49), which points at the same place §6 does. Which table saturates first —
the 5×32768 bank counters, the 15 552-cell frequency lattice, or the 1088 `NbRow`s — was not
isolated; `IDX/bmf-P2.idx:312` fixes the bank radix as a literal, which blocks the obvious
enlargement test.

**8.8 — The activity quantiser saturates on exactly this corpus.** `ctx15`'s 16-class
ladder spans a mean |residual| of 0 to about 0.81 pixel levels (`P2_magsum_cap = 960` over a
total tap weight of 74, in 16× units). The LPCB camera groups run 1.98–2.76 bits/sample,
which for a Laplacian is a mean |residual| of roughly 1.2–2.0 levels — above the top of the
ladder for the busy parts of those images. Measured occupancy on a high-rate plane: 73.1 %
of samples in class 15, 89.9 % in classes 14–15. The activity context, which is an axis of
the frequency lattice, the magnitude strip and both maps, is a constant over most of the
pixels that matter. A naive equal-population ladder is *not* the fix (it loses on
mid-activity content); a per-image fitted one, transmitted, would be — `nb_ctx[]` is already
rebuilt per plane at `alt_p2.inc:1123`.

**8.10 — The slow model's only value-conditioned context tier is dead on this corpus.**
`id3` is gated at `alphabet < MB_small_alphabet = 32` (`model.inc:716`), and the minimum
post-reduction alphabet on a non-reference LPCB plane is **67**. The slow model therefore has
no magnitude-aware context anywhere on this corpus. Its tables are only 3–25 % occupied
(`ctx_id1` capped at 13 600, peaks at 4 672; `ctx_id2` capped at 44 500, peaks at 14 884) —
there is no capacity pressure, the features are the limit. Raising the gate, or replacing
"the left symbol" with a coarse bucket of it, is the cheapest structural change available to
the slow model and it targets the escape path that eats 48 % of pixels on PIA13882.

**8.11 — The joint alt‑P1 driver is dead code here.** `alt_model_p1` (`codec.inc:56`) plus
~100 lines of cross-plane selector logic in `ctx_of` (`alt_p1.inc:132`) never execute in any
shipped stream on this corpus. Per-plane alt‑P1 beats alt‑P2 by 7–11 % on the comb files
while *joint* alt‑P1 loses to joint alt‑P2 on the one file it was measured on, which suggests
the joint variant's cross-plane selectors are worse than the single-plane ones they replace.
If the mixing work needs budget, this is the first thing to retire or fix.

**8.12 — The bias cascade is a net loss on the highest-rate content.** Disabling all five
banks costs +1.38 % on a mid-rate photographic proxy and +3.50 % on `x_ep`, but **gains
0.22 %** on the highest-rate test image. The read-out is an integrator whose gain anneals
only on quiet pixels (`|res| < 38` in 16× units = 2.4 levels); in a noisy region the
countdown never decrements, the rate stays at 2⁻⁵ and the bias tracks the noise. The olympus
group — the corpus's largest single loss at +8.38 MB and its highest-rate group — is exactly
that regime. Gating the read-out or the anneal on activity class is the obvious experiment.

---

## 9. The 2 MB regression: one commit, and why nothing caught it

Building `49a0a08` (the commit that *adds* IMPROVEMENTS.md, i.e. before any of the work it
proposed landed) and comparing against `706fa6f` over all 107 crops:

| group | n | 49a0a08 | HEAD | Δ |
|---|---:|---:|---:|---:|
| fujifilm_x100 | 12 | 9 897 932 | 9 910 624 | **+0.128 %** |
| canon_1100d | 11 | 10 139 036 | 10 151 928 | **+0.127 %** |
| sony_a55 | 12 | 12 697 108 | 12 712 196 | +0.119 % |
| olympus_xz1 | 27 | 30 061 596 | 30 088 464 | +0.089 % |
| STA | 20 | 15 873 296 | 15 886 304 | +0.082 % |
| PIA | 25 | 14 889 584 | 14 874 720 | −0.100 % |
| **photographs** | **62** | 62 795 672 | 62 863 212 | **+0.108 %** |
| all | 107 | 93 558 552 | 93 624 236 | +0.070 % |

HEAD is larger on **88 of 107** crops, smaller on 19, identical on none. Projected to full
size that is **+0.96 MB**; crops under-state map effects by roughly 1.9× here (fix A is
−1.10 % on crops and −2.12 % at full size on the same files), which puts the true figure
around 1.8 MB — consistent with the ~2 MB you measured.

### Which commit

25 revisions between `49a0a08` and HEAD were built and run over three photographic crops
(canon_1100d_02 + fujifilm_x100_03 + olympus_xz1_16, 2 618 104 bytes at the base — a figure
that an independently rebuilt `49a0a08` in a separate tree reproduces to the byte):

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
| direct context model for ≤16-value planes (IMPROVEMENTS §7) | **−4.9 % of that corpus** | **0 of 107** |
| — four mixed context orders, tree-bit APM, count-adaptive counters | (all inside it) | 0 |
| — match model as a fifth mixer input | (inside it) | 0 |
| palette coded instead of stored raw (IMPROVEMENTS §5.1) | −1.5 % on `t8p` | **0** (no palettised file) |
| luminance palette ordering | trial | 0 |
| YCoCg‑R lifting | gated trial | 2 tried, 1 adopted |
| mirror search | 0.04 % | 0 (removed from the path) |
| APM on alt‑P2's zero decision (IMPROVEMENTS §3) | −0.44 % of that corpus | 107 — **the only one that reaches this workload** |

This was checked, not assumed. The **minimum** distinct-value count over all components of
all 107 LPCB files is **74** (`PIA13799`); no file has any component with ≤ 16 or even ≤ 64
distinct values; none is palettisable; and the string `ctx` appears in **zero** of the 107
`-v` transcripts. The single largest improvement in the series — worth −4.9 % on the
development corpus — is never even offered on a single LPCB image.

Meanwhile `testfiles/` is ten images, of which `t1` (320×240 bilevel), `t8g`/`t8p` (320×240),
`t24`/`t32` (320×240), `f05_200` (2-value), `x_ai` (8-value) and `x_ci` (4-value) are small
or small-alphabet or both. The *design decisions* are fitted to that set. LPCB is 62
ten-to-sixteen-megapixel continuous-tone photographs with 200+ levels per channel. A −0.44 %
measured on ten images, of which one is `+0.17 %`, does not survive transfer to a workload
none of them resembles.

**The constants are a different story, and the usual assumption about them is wrong.** They
were *never tuned at all* — not on `testfiles/`, not on anything. All 84 of the 84 float
parameters that carry their original value in an `.idx` comment still evaluate to it
bit-exactly; none of the 31 commits touching `IDX/*.idx` is an `import.pl` result; `opt.lst`
was created in `f201bf2` and never edited. They are Shkarin's BMF 2.01 values, recovered
faithfully by the port. IDX/MOD is infrastructure that was built, tested and never run for
gain.

The corpus it is *configured* to score on is six synthetic images — t1/t8g/t8p/t24/t32 plus
the bilevel `f05_200`: 4.42 Mpx total, of which **0.154 Mpx is truecolour**, against LPCB's
**1 122 Mpx on the alt-P2 path**. A factor of 7 284. `t24`, the only image in that list that
scores the model losing the 25 MB, has MED-residual entropy 5.75 bpp at lag-1 correlation
0.767; the LPCB photographs run 2.1–4.9 bpp at 0.94–0.998.

One hill-climb pass over 14 of the 518 parameters on four 512² LPCB crops moved five, and
isolating them splits the result cleanly: `AP_rate` 5→7, `AP_map_weight` 3→1 and
`P2_rate_reset` are **universal** — they improve the photographs *and* `opt.lst`, which proves
the shipped values are not at the optimum of even the development corpus. Together they give
**−0.117 % on 26 Mpx of full-size real LPCB** (all 12 test crops improve), ≈1.16 MB scaled.
Two of the five are a warning in the other direction: `AP_p_ceil` 65504→57312 gained **4
bytes** on a four-file corpus, was accepted by `opt.pl`'s tie-break, and costs **+7.6 %**
elsewhere. A retuning corpus has to span the *bit-rate* range, not just the content type.

(The two APM parameters in that set are the same mechanism fix A replaces, so their ≈1 MB is
*inside* fix A's measured gain, not additional to it. Two further findings for whoever runs
the pass: 480 of the 518 search patterns begin with `1`, so the reachable maximum is under 2×
the shipped value — one leading `0` per line fixes it — and 84.6 % of the 28 096 search bits
are 22 threshold ladders; freezing those with the `!` marker that no `.idx` line currently
uses takes a pass from 38 h to about 11 h.)

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

## 10. What to do, in order

Every "expected" below is bytes recovered on the full 107-file corpus against current HEAD.
Lines do not add: fix A **supersedes** damping the APM (they are the same mechanism), and
fix B's remaining segment-B files overlap slightly with fix A's NASA gains.

| # | change | expected | effort | speed | risk | evidence |
|---|---|---:|---|---|---|---|
| **1** | **Fix A** — sub-pixel phase on alt‑P2's sign and zero maps (`signfix.patch`) | **13.06 MB** | done: 153-line diff, one file | decode within noise | very low; 7 of 107 files lose, worst +0.059 % | **measured, all 107 files at full size** |
| **2** | **Fix B** — alphabet renumbering for the alt models, **as a gated trial** | **5.4–6.0 MB** | ~100 lines, one bitstream field ≤96 B/file; `reduce_narrow_alphabet` is the precedent | +5–8 % encode, <1 % decode | low, *given the gate* — without it PIA13915 loses 3.24 % | **measured at full size on all six candidates** |
| **3** | Delete the `codec.inc:1749` `slow` suppression (§8.6b) | small alone; **a precondition for 2** | one line | +encode | none | 189/321 slots; it is what breaks PIA13915 |
| **4** | Put a bit-rate-spanning LPCB corpus into `opt.lst` and run `IDX/opt.pl` for the first time | ≈1 MB shown from 3 of 518 parameters, most of it inside fix A; rest unknown | low engineer-hours, ~11 h machine after freezing the ladders | free | medium — `AP_p_ceil` gained 4 bytes on four files and costs +7.6 % elsewhere | §9 |
| **5** | Colour-transform candidate and coding order chosen by trial, not by the order-0 gradient proxy | ≈1 MB, **±2×** | ~60 lines | ≈2× encode — spends the whole budget | low in principle (a trial is the oracle); mis-sizing is the risk | 39/107 decided on <0.1 % margins; crop-derived, re-measure at full size first |
| **6** | Riders on whatever bitstream revision (2) forces: fitted slot‑1 weight (§8.1, ≈0.1), measured joint adoption (§8.2, ≈0.12), clear `alphabet_reduced` before the search (§8.7, 0.05–0.15) | ≈0.3 MB | ~60 lines, +8 bits/plane | free / +30 % encode | low | measured, all small |
| **7** | Gate the bias cascade and its diffusion on activity/occupancy (§8.8, §8.12) | unsized; aimed at olympus's +8.38 MB | ~40 lines + table sizing | free or negative decode | medium — helps big planes, hurts small ones unless gated | best-motivated unmeasured item |
| **8** | **A cross-plane predictor with a neighbourhood** (IMPROVEMENTS §8.1/§8.2, reopened) | the 11–13 MB of §6 is here | large | unknown | high | §6, the strongest evidence in this report |
| **9** | Fuse alt‑P1's prediction and error into alt‑P2's taps / `Mix4` / contexts | 8–15 MB *if* the pooling band holds; unproven | large | +15 % decode | high — prototype, don't schedule | §7 |
| — | ~~region / tile / row-level model switching~~ | ≤ 0.56 % | — | — | — | **refuted**, §7 |
| — | ~~mirrors, YCoCg gate, p2-skip gate, a fixed colour transform~~ | 0 | — | — | — | **refuted**, §8, §11 |
| — | ~~full N-model mixing, SSE chains, match model, learned context quantisation~~ | unbounded, unsized | very large | out of class | — | this is "be paq8px" — see below |

Items 1–3 are days of work. Item 8 is where the majority of the remaining gap actually is,
and nobody has costed it.

**On "just add paq-class modelling".** Three separate lines of analysis converge on it in
different clothes, and it should not be scheduled as a deliverable. IMPROVEMENTS.md §0
already prices most of it out by name. It is not sized — the 30.8 MB "disagreement mass"
measures the cost of picking the *loser*, which selection already avoids, and the only honest
measurements are the fixed-weight pools at −0.82 %/−1.50 %. And it targets the wrong regime:
PAQ8im beats gralic by 8 % on the PIA group, so paq-class modelling would help segment B —
which renumbering already closes for a fiftieth of the effort — while **on the camera groups
that carry 63 % of the deficit, gralic beats PAQ8im**. Whatever BMF is missing on olympus is
not paq.

**An honest total.** Items 1 and 2 are demonstrated at full size and come to **≈18.5 MB of
the 25.2 MB** — fix A's 13.06 MB measured on all 107 files, plus fix B's 5.37 MB measured on
all six candidates, less a small overlap on the comb files that fix A also improves. Beyond
them the attributed mechanisms come to roughly 1.5–2.5 MB.

What is left is **olympus**, and it is not small: +2.29 % after fix A, on the group carrying
the largest share of the remaining deficit. It is the highest-rate group, which is exactly
where fix A's mechanism has least to give, and §6 puts most of its loss in the cross-plane
channel, where nothing has been built or costed. Item 8 is aimed at it and is the honest
answer to "what next".

---

## 11. What is not the problem

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
  from the coder's own optimum on the one image where it was swept directly (§8.0), and the
  slot-1 unit gain is at its optimum too (§8.1).
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
* **The transpose decision on the two biggest transposed losers.** A transpose-disabled
  encoder makes STA13845 44 226 712 → 44 249 584 (+22 872) and STA13454 14 338 608 →
  14 355 544 (+16 936). The decision is correct on both.
* **The BMP reader.** All 107 are 24 bpp / BI_RGB / 40-byte v3 / offset 54 / bottom-up.
  42 files carry 1–3 bytes of row padding which `read_rows` seeks over and discards — every
  padding byte in the corpus was checked and is zero, and a round trip on a padded file is
  byte-identical. The internal stride is `width*planes`, so predictors see clean rows.
* **The range coder.** `encode_bit` allocates `f0*(range/(f0+f1))` and gives the remainder
  to bit 1, so the two intervals tile `[0,range)` exactly with no dead space; renormalisation
  keeps `range ∈ (2²³, 2³¹]`, making probability distortion ≤ 1/128 relative and its coding
  cost second-order.
* **The configuration.** `bmf.cpp:274` — *"compress, always with -S -Q9"*. The binary has
  only `c`/`d`; the published sizes are already the maximum setting.

---

## 12. Open questions, and what would settle them

* **Extend the per-plane decomposition.** §6 rests on five photographs. Run it on 12–16
  spanning all four cameras and report gap(RGB), gap(Σ3 planes) and each codec's cross-plane
  gain per file. If the 71–85 % band holds, segment A is settled as a cross-plane problem and
  everything else in it is second-order. 2048² crops track their full files within ±0.26
  points for sony/fuji/olympus; **canon must be run at full size** (its crop was off by
  +0.80/−2.65 points) and so must the NASA files (−3.4/−4.7). About an hour on crops.
* **Why G.** BMF's G plane is +0.78 % to +2.30 % worse than gralic on every photograph while
  B and R are at parity. Nothing in this report explains it.
* **Why olympus specifically.** Camera identity explains 58 % of the photographic variance
  and nothing measured explains camera identity. `ctgain_bpp` orders the four camera means at
  r = +0.854, but the *within*-camera partial correlation is −0.175 — the wrong sign. One
  suggestive detail: olympus is the only camera where the fitted two-reference weight
  collapses to `w=(128,0)`, on 9 of 27 files (canon 0/11, sony 0/12, fuji 1/12).
* **`STA13454`** (+0.541 MB, +3.92 %) is still the largest wholly unexplained single loss:
  clean 256-level planes, `remapgain` 0, transpose verified correct, no unusual spatial or
  colour statistic, and the plane decomposition was not run on it. Same for STA13452,
  STA13455, STA13457.
* **Which table saturates.** §8.7b shows BMF's rate flat against gralic's falling on nested
  prefixes of the same image. Whether that is the 5×32768 bank counters, the 15 552-cell
  frequency lattice or the 1088 `NbRow`s was not isolated — `IDX/bmf-P2.idx:312` fixes the
  bank radix as a literal, which blocks the obvious enlargement test.
* **Fix B's remaining files.** PIA13812 was never run renumbered at full size, and the four
  measured recoveries span 5.6 %–129 % of their files' gaps, so a per-file forecast is not
  available. The union-map and reference-plane-map variants (the value sets differ on every
  segment-B file; STA13845 recovered only 53 %) were not tried.
* **`sym_list.inc` and `counters.inc`** — 760 lines, never examined for loss. They serve the
  slow model (2.67 % of bytes) and alt‑P1 (3.70 %), which bounds the exposure, but §1's
  alt‑P1 population makes that bound ≈0.5 MB rather than nothing.
* **gralic's actual mechanism.** Inferred here only from where it wins; the mixer and SSE
  attributions in the literature rest on constant fingerprints, not on a decompiled loop.
  Nothing in this report depends on a claim about gralic's internals.

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

```
# the validation corpus this report argues for
for f in LPCB-bmp/*.bmp; do python3 crop.py "$f" crops/$(basename $f) 1024 1024; done
./bench.sh ./bmf head           # 107 encodes, ~20 min on 4 cores

# the per-plane decomposition of section 6
python3 plane.py crops/olympus_xz1_16.bmp p1.bmp 1     # 8-bit grey, one component

# gralic, for comparison (Win32 demo binary, runs under wine)
/usr/lib/wine/wine Gralic111d.exe c out.gra in.ppm
```

Bit accounting is a 40-line patch to `rangecoder.inc` — accumulate `−log2 p` in `encode` and
`encode_bit` into a bucket chosen by a scoped `bmf_site`, reset on `enc_init`, dump on
`flush`. It does not change the coded stream, which is the check that it is correct.

---

## Files in this branch

| file | what |
|---|---|
| `LPCB-REPORT.md` | this document |
| `signfix.patch` | fix A, against `alt_p2.inc` at `706fa6f` of `claude/bmf-codec-build-test-nghv6u`; applies cleanly to a pristine checkout |
| `crop.py` | builds the 1024² validation corpus from the LPCB BMPs |
| `plane.py` | extracts one component of a 24-bit BMP as an 8-bit greyscale BMP (§6) |
| `bench.sh` | runs a `bmf` binary over that corpus and prints `<name> <bytes>` |
| `lpcb.csv` | per-file join of gralic/BMF sizes, the bmgstat statistics and the `-v` choices |
| `results.csv` | every number behind §4 and §9, per file: gralic, BMF HEAD and **BMF + fix A at full size**, plus the 1024² crop at `49a0a08`, at HEAD and with fix A |
| `bisect-results.txt` | the 25-revision bisect, `<sha> <bytes over three photographic crops>` |
