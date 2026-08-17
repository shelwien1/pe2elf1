# How the Philips DST encoder works, and how ours differs

Notes from reading a decompilation of `DstEncCtrl.dll` (the encoder behind
`DstEncUi 4.0.3`, which produced the reference file this project is measured
against), and what it says about the choices made in `src/dstenc.h`.

Nothing was copied. This is an analysis of what the reference encoder does and
why; the implementation here remains written from the ISO/IEC 14496-3 Part 3
Subpart 10 format description and FFmpeg's LGPL decoder. Where the two encoders
agree, it is because the format constrains the answer or because both arrived at
the same idea, not because anything was transcribed.

## What the DLL contains

More than expected. The name suggests a control layer, and much of it is indeed
MFC, threads, version strings and progress reporting — but the signal processing
is in there too: the autocorrelator, the predictor solver, the probability table
builder and the coding loop.

It is organised as a **six stage pipeline** (`GetNumberFunctionSteps` returns
seven, one per stage plus the caller) with a thread per stage, so frames flow
through the stages concurrently. The first stage's name is stored in the binary:
`"Auto Correlator"`.

The buffer sizes identify the layout immediately: 774 floats (129 lags × 6
channels), 768 (128 taps × 6 channels), 28224 bytes (4704 × 6, one frame of
packed DSD), 225792 bytes (37632 × 6, one *byte per sample*).

## Stage by stage

### 1. Autocorrelation

The same insight this project uses: for ±1 mapped bits the product is just a bit
comparison, so the autocorrelation is a popcount of a shifted XOR.

The implementation is of its era. There was no `POPCNT` instruction, so it
counts bits through a 2048-entry table, three lookups per 32-bit word (11 + 11 +
10 bits). The loop is unrolled **32 lags deep**, so each loaded word is XORed
against 32 different delayed words and accumulated into 32 registers before
moving on — the same register-blocking idea, arrived at because loads were the
scarce resource.

Ours does the identical arithmetic with hardware popcount, or with the AVX2
nibble-shuffle popcount over four 64-bit words at a time.

### 2. Predictor design

This is where the two encoders genuinely differ, and it is worth spelling out
because it explains measurements taken earlier in this project.

```
r = autocorrelation                       (129 lags)
order = levinson_durbin(r, max_order)     with early termination
coeff = step_up(order)                    reflection -> direct form
if max|coeff| > 10:
    r[0] *= 1.0000041                     white noise correction
    redo levinson and step_up
    if max|coeff| > 10:
        order = min(order * 8/9, 80)      give up some order
        redo step_up
coeff[i] = (coeff[i-1] + 2*coeff[i] + coeff[i+1]) / 4      smoothing
coeff[i] = floor(coeff[i] / max|coeff| * 255 + 0.5)        quantisation
```

Four things stand out.

**Order selection that does not actually fire.** Levinson-Durbin stops on either
of two conditions: the residual falling below **0.3% of `r[0]`**, or the
reflection coefficient reaching magnitude one. Read quickly, the first looks
like adaptive order selection and a likely explanation for their advantage on
quiet frames. It is not. On DSD the residual never approaches 0.3% of `r[0]` -
the signal is one-bit and the predictor, though it gets the *sign* right 87% of
the time, leaves a large residual in the least squares sense - so the second
condition is the operative one, and it is exactly the reflection coefficient
test this encoder already applies. Their filters come out 128 taps as well,
which the dumped coefficient tables confirm. Implemented and measured here: a
0.3% threshold changes nothing, and so does 3%.

**Conditional white noise correction.** `r[0]` is inflated by a factor of
1.0000041, but *only* when the first solve produced coefficients larger than
10.0 — a badly conditioned frame. This project measured that applying white
noise correction unconditionally costs about 2% and removed it; the reference
encoder agrees, and keeps it as a repair for the pathological case rather than a
default.

**Coefficient smoothing.** A `[1 2 1]/4` pass over the direct-form coefficients.
This is the direct explanation for something measured earlier here: dumping the
reference encoder's coefficients for a frame and comparing them with ours showed
theirs to be visibly smoother and more spread out, ours peakier and faster
decaying. That was this smoothing pass. It also makes the coefficients cheaper
to transmit, which showed up as the reference spending 374 bits per frame on the
filter table against our 411.

**Identical quantisation.** Scale so the largest tap is 255, round half up. Bit
for bit what `quantise_filter()` does here, arrived at independently by
reasoning that it maximises resolution within the 9-bit field.

### 3. Prediction and statistics

Identical in structure to the decoder, and to ours: the 128-tap sign-based FIR
is evaluated as sixteen lookups into 256-entry `int16` tables indexed by bytes
of the shift register. Per sample it writes the probability bin,
`min(|prediction| >> 3, 63)`, and packs a "the prediction was right" flag into a
bit array.

One difference: the loop is specialised by how many bytes of history the filter
actually spans, with separate cases for 1–4 bytes, 5–8 bytes and so on. The
format permits short filters even though this material never produces one, so on
DSD it always lands in the widest case and the specialisation costs nothing and
buys nothing.

### 4. Probability tables

```
p = (errors + total + errors * 512) / (2 * total)     = round(256.5 * errors/total)
p = clamp(p, 1, 128)
empty bins take the value of the next higher bin
length = trim trailing empty bins, then the trailing run of equal values
```

The quantisation is ours almost exactly — `round(256 * error_rate)` clamped to
1..128 — with one curiosity: their scale factor works out to **256.5**, not 256.
A sweep of that constant here found the optimum was slightly above 256 (259–262
was marginally better than 256, worth about 0.01%), so the reference encoder's
half-step is in the same direction and for the same reason: the arithmetic
coder's interval approximation makes the effective probability slightly less
than `p/256`.

The table *length* is chosen structurally rather than by cost: drop trailing
empty bins, then drop the trailing run of bins that all ended up with the same
probability, since the decoder clamps the index to the last entry anyway. Ours
searches lengths against an estimated coded size, which is why our tables come
out longer (14–21 entries against their 11).

Empty bins are filled from the neighbour *above*; ours fills from the neighbour
below. Neither is obviously right — an empty bin costs nothing either way, it
only affects how cheaply the table codes.

### 5–6. Coding and assembly

Arithmetic coding of the flags with those probabilities, the coefficient and
probability tables coded with the format's fixed predictors and Golomb residuals,
then frame assembly, with a check that falls back if a frame fails to compress.

## Side by side

| | Philips `DstEncUi 4.0.3` | dff2dsf |
|---|---|---|
| structure | 6-stage pipeline, thread per stage | one thread per frame, off by default |
| autocorrelation | popcount-XOR, 11-bit tables, 32-lag unroll | popcount-XOR, hardware/AVX2 |
| predictor | Levinson-Durbin | Levinson-Durbin |
| order | stops at 0.3% residual, which DSD never reaches | fixed 128, same in practice |
| conditioning | white noise correction only if max\|c\| > 10 | none |
| coefficient smoothing | `[1 2 1]/4` | `[1 2 1]/4`, kept as one of two candidates |
| refinement | **none, feed-forward** | **12 maximum-likelihood gradient steps, from two starts** |
| quantisation | peak → 255, round half up | same |
| prediction | 16 × 256 byte-indexed tables | same (AVX2: 4-tap nibble tables) |
| bins | `min(\|predict\|>>3, 63)` | same |
| probability | `round(256.5 · e/n)`, clamp 1..128 | same |
| empty bins | filled from above | filled from below |
| table length | trim empty and equal-run | cost-based search |

## What this explains

**Why their filter beat our first one.** Substituting the reference encoder's
coefficients into this encoder produced output within 0.01% of its own files,
proving the coder and tables here were already equivalent and the whole gap was
filter design. At that point ours was plain Levinson-Durbin at fixed order, and
theirs was Levinson plus order selection, a conditioning guard and smoothing —
three refinements ours did not have.

**Why the logistic refinement then overtook it.** Their pipeline is entirely
feed-forward: the filter is designed from the autocorrelation and never revisited
against what it actually costs to code. Least squares is the wrong objective for
a predictor whose sign is all that matters, and twelve maximum-likelihood steps
close that gap and a little more — which is where the 0.070% overall win comes
from.

**Why we lose on quiet frames.** The first guess was adaptive order, and it was
wrong - see above, their order is 128 as well. Comparing the two encoders frame
by frame over the quiet intro shows something more useful: the loss is not
concentrated in the near-silent frames at all (there is one such frame, and it
accounts for 80 bytes of a 5000 byte gap), but spread across ordinary frames,
and it is two-sided. On some frames we are 250 bytes worse, on others 90 bytes
better.

That is the signature of a local search landing in different basins, not of a
systematic modelling deficit - and it is what motivated refining from two
starting points, which recovers a good part of it.

## What was taken from this, and what it gave

Measured on 200 quiet frames and 400 loud ones, against DST payload bytes.

**Coefficient smoothing — adopted, and it is the substantial one.** A single
`[1 2 1]/4` pass over the Levinson solution. Least squares fits every wrinkle of
the measured correlation, including the part that is estimation noise rather
than signal; smoothing takes that off, and the result both predicts slightly
better and codes cheaper because neighbouring coefficients differ by less.

It does not win on every frame, though - on roughly a third the unsmoothed
design is better - so rather than replacing the design, both are now refined and
the cheaper result kept. The refinement is a local search and the two designs
sit in different basins, which is why following both is worth more than picking
the better start:

| | quiet 200 | loud 400 |
|---|---|---|
| before | 769,747 | 1,609,998 |
| smoothed only | 769,125 | 1,609,797 |
| better of the two starts, then refine | 769,035 | 1,609,797 |
| refine both, keep the cheaper | **768,843** | **1,609,458** |

**Probability scale of 256.5 — adopted.** Worth about 0.01%, and it replaces a
swept constant with a justified one.

**Order selection — measured, no effect.** See above: their threshold does not
fire on DSD, and neither does ours when implemented.

**The conditioning guard — not adopted.** There is still no protection here
against an ill-conditioned frame producing enormous coefficients. It has never
been observed on the test material, but the reference encoder carries the check
for a reason, and this is the most defensible thing left on the list.

**Threading — adopted, but across frames rather than stages.** Their six stage
pipeline is one way to use more cores, but it is bounded by the slowest stage.
Frames here are independent, so `--threads N` simply encodes N frames at once,
which is both simpler and scales with the core count rather than with the number
of stages: 3.6x on four cores, against the 6x their pipeline could reach at best.

The cost of following two candidates is that encoding takes twice as long, since
the refinement now runs twice per frame. That is the whole of the extra time -
everything before the refinement is unchanged.
