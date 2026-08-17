# DST, decoded and encoded

Direct Stream Transfer is the lossless coder Philips puts inside DSDIFF (`.dff`)
files, specified in ISO/IEC 14496-3 Part 3 Subpart 10. This document describes
what it does, in the order the code does it: decoding first, because encoding is
defined entirely in terms of what the decoder will do with the result.

Section numbers in parentheses are the specification's, quoted where
`src/dst.hpp` quotes them. The decoder here is a port of FFmpeg's
`libavcodec/dstdec.c` and is LGPL; the encoder is original work but predicts
through the decoder's own filter evaluation, so the two agree by construction.

## Contents

- [The material](#the-material)
- [Decoding](#decoding)
  - [Frame syntax](#frame-syntax)
  - [Tables](#tables)
  - [The filter](#the-filter)
  - [The arithmetic decoder](#the-arithmetic-decoder)
  - [The sample loop](#the-sample-loop)
- [Encoding](#encoding)
  - [What has to be chosen](#what-has-to-be-chosen)
  - [Autocorrelation](#autocorrelation)
  - [The initial filter](#the-initial-filter)
  - [Analysis](#analysis)
  - [The cost model](#the-cost-model)
  - [Refinement](#refinement)
  - [The probability table](#the-probability-table)
  - [Assembling the frame](#assembling-the-frame)
  - [The arithmetic encoder](#the-arithmetic-encoder)
  - [Fallbacks and trimming](#fallbacks-and-trimming)
  - [Threads](#threads)
- [Constants](#constants)
- [Where this lives in the source](#where-this-lives-in-the-source)

## The material

DSD is one bit per sample per channel at a very high rate: DSD64 is 64 × 44100 =
2822400 samples per second per channel. A DST frame holds 1/75 of a second,

    frame_bits = 588 × (dsd_rate / 44100)

which is 37632 bits — 4704 bytes — per channel at DSD64. Bits run most
significant first, and in the container channels are interleaved a byte at a
time. Frames are entirely independent: nothing carries from one to the next,
which is what makes seeking possible, and what lets the encoder use as many
cores as it likes.

Throughout, a DSD bit `b` is read as the value `s(b) = 2b - 1`, so a 1 is +1 and
a 0 is −1. Every arithmetic below is in those terms.

Where frames come from — the `FRM8` form, `PROP`/`SND`, `DST `/`FRTE`/`DSTF`,
and the optional `DSTI`, `DSTC` and `COMT` chunks — is container business, and
is described in `README.md` instead.

## Decoding

A frame decodes to `frame_bits × channels` DSD bits and nothing else: there is no
inter-frame state to carry, no filter to keep, no probability history. The
decoder rebuilds everything from the frame's own header.

### Frame syntax

The frame is a bit stream, MSB first. Reading past its end yields zeroes, and
that is load-bearing — the encoder strips trailing zero bytes, so a decoder that
faults instead of returning zeroes would reject valid frames.

| bits | field | meaning |
|---|---|---|
| 1 | coded | 0 = raw DSD follows, 1 = DST coded (10.3) |
| 1 | same segmentation | must be 1 |
| 1 | same segmentation for all channels | must be 1 |
| 1 | end of channel segmentation | must be 1 |
| 1 | same mapping | filters and probability tables share one channel map |
| … | filter map | see below (10.7–10.9) |
| … | probability map | absent when *same mapping* is 1 |
| 1 each | half probability | one per channel (10.10) |
| … | filter coefficient sets | length 7 bits, coefficients 9 bits signed (10.12) |
| … | probability tables | length 6 bits, values 7 bits with offset 1 (10.13) |
| 1 | coder reset | must be 0 |
| 12 | arithmetic coder start | initial offset `c` (10.11) |
| … | arithmetic coded samples | one symbol per sample per channel |

**A raw frame** is the degenerate case: the first byte is zero (a 0 bit, a
skipped bit, then six bits that must be zero) and the rest of the chunk is
interleaved DSD, copied out as is. Encoders emit this only when coding would
have made the frame larger.

**Segmentation** would allow a frame to switch filters partway through, and the
maps would allow each channel its own filter and probability table. Real
encoders emit neither, and the reference decoder rejects the segmentation forms
outright, so the three segmentation bits must all be 1.

**The map** assigns each channel an element index. Channel 0 always maps to
element 0. If the first bit is 1, so does every other channel — one filter for
the whole frame. Otherwise each later channel reads an index in
`ilog2(elements) + 1` bits, and an index exactly equal to the current element
count declares a new element; anything larger is malformed. That gives a compact
code that grows only as elements are actually introduced.

### Tables

Filter coefficient sets and probability tables are coded the same way, differing
only in field widths and in whether the values are signed:

| | length field | value field | signedness | offset |
|---|---|---|---|---|
| filter coefficients | 7 bits | 9 bits | signed | 0 |
| probability values | 6 bits | 7 bits | unsigned | +1 |

The stored length is one less than the real one, so lengths run 1..128 and 1..64.
Then one bit says how the values themselves are stored:

- **Verbatim** — every value in its field width.
- **Predicted** — a two-bit *method* selects order 1, 2 or 3 (method 3 is
  invalid). The first `method + 1` values are stored verbatim as seeds, a
  three-bit `lsb_size` follows, and each remaining value is sent as the residual
  from a fixed linear prediction of its predecessors, Golomb coded.

The predictors are fixed by the format, in eighths:

```
filter coefficients            probability values
  order 1:  -8                   order 1:  -8
  order 2: -16,  8               order 2: -16,   8
  order 3:  -9, -5, 6            order 3: -24,  24, -8
```

The prediction is formed in eighths and rounded away from zero, which the decoder
undoes by adding it back:

```
x = Σ pred[method][k] · value[j-k-1]                 for k = 0 … method
c = golomb(lsb_size)
value[j] = x ≥ 0 ? c - (x + 4)/8 : c + (-x + 3)/8
```

The Golomb code is JPEG-LS unsigned with escape length zero — a unary quotient,
a stop bit, `lsb_size` remainder bits — followed by a sign bit when the value is
non-zero. Unsigned tables are range checked as they are read, since a corrupt
probability outside 1..128 would otherwise poison the coder.

### The filter

The predictor is a 128-tap FIR over the ±1 mapped history:

    P[i] = Σ coeff[t] · s(x[i-1-t])        t = 0 … length-1

evaluated in 16-bit arithmetic, and only its **sign** is ever used. That last
point is the single most important fact about DST, and the whole of the
encoder's filter design follows from it.

Because every input is ±1, the sum over any eight consecutive taps depends only
on those eight history bits — 256 possibilities. So the filter is expanded once
per frame into a lookup table

    lut[j][k] = Σ s(bit l of k) · coeff[8j + l]      l = 0 … 7

of 16 rows by 256 entries, and the prediction becomes sixteen table lookups.
Taps beyond the sent length are zero, so short filters simply give rows of
zeroes. A partial sum that will not fit `int16_t` is rejected, though with
coefficients bounded by ±256 it cannot happen: eight taps reach ±2048.

The history itself is a 128-bit shift register per channel, held as two 64-bit
words with the newest bit in bit 0 of the low word. Byte `j` of that register is
exactly the byte that indexes row `j`, so evaluation is:

```
predict = Σ lut[j][ byte j of status ]       j = 0 … 15
```

At the start of every frame the register is filled with `0xAA` — alternating
bits — which is what makes frames independent.

### The arithmetic decoder

DST codes one binary symbol per sample per channel with a 12-bit range coder
(10.11). The state is an interval width `a`, starting at 4095, and an offset `c`
into it, read from the frame as 12 bits.

Decoding one symbol with probability `p` (in 1..128, a probability out of 256):

```
k   = (a >> 8) | ((a >> 7) & 1)      // floor(a/256), bit 0 set if the remainder ≥ 128
q   = k · p                          // ≈ a · p/256
a_q = a - q

e = (c < a_q)                        // 1 selects the wide sub-interval
if e:  a  = a_q
else:  a  = q,  c -= a_q

while a < 2048:                      // renormalise, refilling c from the stream
    n = 11 - ilog2(a)
    a <<= n
    c = (c << n) | get_bits(n)
```

So `e = 1` — the symbol worth `a - q` of the interval — is the likely one, and
`p/256` is the probability of `e = 0`. In DST terms `e = 1` means *the filter
predicted this bit correctly*, and the table therefore holds **error rates**.

That `k` is a cheap approximation of `a/256` matters more than it looks:
averaged over the renormalised range `a ∈ [2048, 4095]`, `256k/a` comes to
0.980, and it is below one about three quarters of the time. The interval given
to `e = 0` is therefore about 2% narrower than `p/256` would suggest, so the
probability that actually minimises the coded size is a shade *above* the
measured error rate — which is where the encoder's scale factor of 256.5, and
the 259–262 an empirical sweep preferred, both come from.

### The sample loop

Before the samples, one symbol is decoded and discarded, with probability
`(reverse_bits(coeff[0][0] & 127) >> 1) + 1` — a quirk of the format that costs
a fraction of a bit and exists to give the coder a defined start.

Then, for each sample index `i`, for each channel in order:

```
predict = Σ lut[j][byte j of status]                     // 16-bit

if half_prob[ch] and i < filter_length:
    p = 128                                              // even odds
else:
    bin = min(|predict| >> 3, prob_length - 1)
    p   = prob_table[bin]

e = arith_decode(p)
x = (predict >> 15) ^ e                                  // the DSD bit
status = (status << 1) | x
emit x
```

Three details are worth spelling out:

- **The bin** is the prediction magnitude in steps of 8, saturating at the table
  length. A confident prediction lands in a high bin with a low error rate; an
  uncertain one lands near zero, where the rate approaches a half.
- **`predict >> 15`** is the sign bit broadcast: 0 when the prediction is
  non-negative and 1 when negative. XOR with `e` turns "was the prediction right"
  back into the bit. A non-negative prediction therefore predicts a 1.
- **Half probability** covers the first `filter_length` samples, whose history is
  still partly the fixed `0xAA` pattern rather than signal. The encoder decides
  per channel whether those are cheaper coded at even odds or through the table.

Output bits are packed MSB first and interleaved by channel, one byte per channel
at a time, which is exactly the DSDIFF layout.

## Encoding

The format fixes the decoder; everything above is the contract. The encoder's
freedom is in three choices per frame — the filter coefficients, the probability
table, and how each table is coded — and its job is to make the frame small.

### What has to be chosen

The coded size of a frame is roughly

    filter table + probability table + Σ over samples of -log2 P(symbol)

and the three terms fight each other. A longer, more finely fitted filter
predicts better but costs more to send; a longer probability table models the
uncertain bins better but costs more to send; and both are of the order of a few
hundred bits against a payload of tens of thousands, so neither can be ignored.

The pipeline, per frame and across all channels together:

```
1. autocorrelate the ±1 mapped bits
2. Levinson-Durbin  →  128-tap predictor, then quantise to 9-bit coefficients
3. refine it against what DST actually charges, from two starting points
4. measure per-bin error rates  →  the probability table
5. arithmetic code the "prediction was right" flags
```

Steps 3–5 predict bit for bit as the decoder does, sharing the filter lookup
table from `dst.hpp`. One filter and one probability table are used for all
channels: per-channel versions were measured and cost more to send than they
save.

### Autocorrelation

With samples mapped to ±1, a product is a bit comparison: `s(x)·s(y)` is +1 when
the bits are equal and −1 when they differ. So for `N` samples of which `d`
differ at lag `k`,

    r[k] = N - 2d

and the entire autocorrelation is a popcount of a shifted XOR. 128 lags over
37632 samples per channel costs a few hundred thousand word operations, since 64
samples move per instruction — or 256 with the AVX2 kernel, which replaces the
hardware popcount with the usual nibble-table `vpshufb`.

The lagged reads run back into the 128 bits of `0xAA` history preceding the
frame, which is exactly the history the decoder will have.

### The initial filter

Levinson-Durbin on `r` gives a 128-tap predictor. The recursion stops early if a
reflection coefficient leaves (−1, 1) or the residual hits zero; on DSD it
essentially always runs to 128. There is deliberately **no white noise
correction**: inflating `r[0]` measurably costs prediction accuracy here. (The
reference encoder applies one, but only as a repair when a first solve produces
coefficients larger than 10 — a badly conditioned frame — not by default.)

The solution is then **smoothed** with a `[1 2 1]/4` pass. Least squares fits
every wrinkle of the measured correlation, including the part that is estimation
noise rather than signal; smoothing takes that off, usually predicts slightly
better, and codes cheaper because neighbouring coefficients differ by less.
Usually, not always — so both the smoothed and the unsmoothed designs are kept,
and refinement follows each.

**Quantisation** scales the filter so its largest tap is 255, then rounds to
integers clamped to the 9-bit field. Scaling to the peak is the finest
quantisation the format allows, and since only the sign of the prediction
matters, a common scale factor changes nothing else. Measured cost of the
quantisation itself: nothing.

### Analysis

An analysis pass runs the decoder's own prediction over every channel and
records, per sample, one byte:

    code[i] = bin | (correct << 7)          bin = min(|predict| >> 3, 63)

from which everything downstream is computed. Accumulating `bin_count[]` and
`bin_errors[]` over that gives the per-bin error rates. The first 128 samples are
left out of the statistics — that is the full filter length, and equally the
history length, so those are the samples whose prediction still draws on the
fixed `0xAA` pattern rather than on signal.

There are two implementations, chosen at compile time, and they produce
identical bytes:

- **Scalar** — the decoder's sixteen lookups per sample, over a 128-bit shift
  register.
- **AVX2** — the same sum, 32 samples at a time. Two observations make it
  possible. Byte `j` of the shift register at sample `i` is the byte it held at
  sample `i - 8j`, so one sliding array of "the eight bits before this sample"
  serves every tap; and unlike the decoder, which cannot look ahead because each
  bit depends on the one just decoded, the encoder knows every bit in advance, so
  samples are independent and vectorise. Four taps then index a 16-entry table,
  which is a byte shuffle — except that four taps of a 9-bit coefficient do not
  fit in a byte, so each coefficient is split into high and low nibbles,
  accumulated apart, and recombined as `16·high + low`.

### The cost model

Refinement needs a number to minimise, and it has to include the filter, or the
search happily buys prediction accuracy with coefficients that cost more to send
than they save:

```
cost = code_table(coeff, trimmed_length)                       // bits, exactly
     + Σ over bins of  errors·-log2(p/256)
                     + (count - errors)·-log2(1 - p/256)
```

where `p = quantise_prob(count, errors)` is the probability the encoder would
send for that bin. The filter term is exact — it runs the same table coder that
will write the frame — while the payload term assumes each bin keeps its own
probability, the table-length search being deferred to the end.

`trimmed_length` drops trailing zero coefficients, which contribute nothing to
the prediction and so are free to omit.

**The probability quantiser** is

    p = clamp(round(256.5 · errors / count), 1, 128)

The 256.5 rather than 256 is the interval-rounding correction described under
[the arithmetic decoder](#the-arithmetic-decoder). It is worth about 0.01%,
and it replaces a swept constant with a justified one: the reference encoder
uses exactly 256.5, which is the same correction arrived at independently.

### Refinement

This is the biggest single difference from the reference encoder, and it is
where the last few percent come from. The reasoning is short: **the decoder only
takes the sign of the prediction**, so the filter is a binary classifier, not a
magnitude estimator. Levinson-Durbin minimises the squared error of a quantity
whose magnitude is never used. Maximising the likelihood of the bits actually
observed is the right objective, and it is a different filter.

For a logistic model, the gradient of the log-likelihood with respect to the
filter is

    Σ over samples  P(model gets this one wrong) · target · history

and the probability bin a sample landed in already measures how often the model
is wrong there. So the weight comes free:

- a sample the filter got **wrong** is weighted by `1 - rate` — it was confident
  and wrong, so it matters,
- a sample it got **right** is weighted by `rate` — how likely a sample in that
  bin was to have gone the other way,

with both pushing the filter towards the bit that actually occurred. Weights are
quantised to four bitplanes and, for each tap, the weighted correlation is again
a masked popcount:

    Σ over planes p of  2^p · popcount(plane_p & (bits ^ bits >> lag))

Written as a step on the unquantised weights, with `total` the sum of all
weights and `disagree` the weighted count of samples differing from the bit
`t+1` places earlier:

    weight[t] += step · (total - 2·disagree) / total

Twelve steps run per starting point, with `step` beginning at 0.35 and decaying
by 0.7 each pass. The filter is requantised and reanalysed after every step, the
cost is evaluated each time, and the cheapest iterate is what survives —
stopping early on the first non-improving step is not safe, because the cost
dips and recovers.

The whole refinement then runs **twice**, once from the smoothed design and once
from the unsmoothed one, keeping whichever result codes cheaper. The search is
local and the two starts land in different basins; following both is worth about
0.12% and costs half the encoder's running time.

Measured contributions, against the reference encoder's own files:

| | effect |
|---|---|
| Levinson-Durbin alone | ≈ 4% behind |
| perceptron-style sign-error refinement | ≈ 0.2% behind |
| maximum-likelihood refinement, one start | ≈ 0.07% ahead |
| the same from two starts | ≈ 0.19% ahead |

### The probability table

With the final filter fixed, the table length is chosen by cost. For each
candidate length the estimate is a rough table cost of `7·length + 7` bits plus
the coded size of the payload under that table, where every bin from
`length - 1` up is merged into the final entry — because the decoder clamps the
bin index to the last entry, the tail is genuinely one bin.

Empty bins take the value of the bin below, which costs nothing and keeps the
table smooth for the differential coding that follows. (The reference encoder
fills from above and trims structurally rather than by cost; neither is obviously
right, and its tables come out shorter — around 11 entries against 14–21 here.)

**Half probability** is then decided per channel: the first `filter_length`
samples are priced both ways — through the table, or at one bit each — and the
cheaper wins. The table was built without those samples, so this is a genuine
comparison.

### Assembling the frame

The header is written as the fixed pattern the decoder demands: coded, same
segmentation, same for all channels, end of segmentation, same mapping, and a
map putting every channel on one shared element — six 1 bits — then the
per-channel half-probability flags.

Both tables are then written by a coder that simply tries everything the format
allows and keeps the smallest: verbatim, or each of the three prediction orders
against each of the eight Golomb parameters, costed exactly. One guard applies —
a residual whose unary prefix would exceed 4095 bits is rejected, since it would
not decode back — and the search abandons a candidate as soon as it exceeds the
best so far. The same routine, called with no bit writer, is what the cost model
uses.

### The arithmetic encoder

The encoder is the exact inverse of `ac_get`. It keeps the same interval width
`a` and a 12-bit `low`, and writes the bits that leave the window:

```
k   = (a >> 8) | ((a >> 7) & 1)
q   = k · p
a_q = a - q

if e:  a = a_q                                    // [0, a_q) codes e = 1
else:  low += a_q;  if low ≥ 4096: low -= 4096, carry();  a = q

while a < 2048:
    put_bit((low >> 11) & 1)
    low = (low << 1) & 4095
    a <<= 1
```

Two consequences of the 12-bit window are worth noting. First, adding `a_q` to
`low` can **carry into bits already written**; the whole frame is buffered, so
the carry propagates backwards through the buffer, incrementing bytes until one
does not overflow. Second, the flush just writes the 12 bits of `low`, which
leaves the decoder's offset at zero — inside the final interval — and everything
the decoder reads past the frame is zero padding anyway.

The payload loop mirrors the decoder exactly, sample by sample and channel by
channel, feeding it the recorded `correct` flag and the probability that sample's
bin resolves to. The discarded opening symbol is coded as the cheaper of its two
values.

### Fallbacks and trimming

Trailing zero bytes are stripped from the finished frame: the decoder reads
zeroes past the end, so they carry nothing. Typically a byte or two per frame,
which over 19119 frames is not nothing.

If the coded frame would be no smaller than the raw DSD — or the bit writer
overflowed — the frame is emitted raw instead: a zero header byte followed by the
interleaved DSD, exactly what the decoder's uncompressed path expects. On real
material this never fires; on white noise it fires on every frame, which is how
the path is tested here. Such a file round trips bit-exactly, padding included.

> **FFmpeg cannot read those frames.** In `libavcodec/dstdec.c` the uncompressed
> path `memcpy`s the frame's bytes packed (line 274), but the DSD-to-PCM
> conversion both paths share reads them with a stride of `channels * 4`, which
> is what the coded path writes (line 367). So FFmpeg takes every fourth byte and
> then runs into the uninitialised remainder of the buffer: decoding the same
> white-noise `.dff` three times gives three different md5s, while a DST coded
> file is stable. Nothing to do with the file — the same frames decode correctly
> here, and byte for byte back to the input — but it means "FFmpeg agrees" is a
> claim about coded frames only. Checked against FFmpeg master `74705b3`.

### Threads

Frames are independent, so `--threads N` encodes N of them at once: frames move
through a ring of slots two longer than the worker count, the calling thread
fills every free slot and writes finished frames in order, and workers take the
next uncoded frame in turn. Each worker holds its own encoder, since every buffer
in one is rewritten per frame anyway.

Nothing about the result depends on the thread count — the same frame always
produces the same bytes — which is checked on every run of `tests/verify.sh`.

## Constants

| symbol | value | what |
|---|---|---|
| frame rate | 75 Hz | one frame is 1/75 s |
| `frame_bits` | 588 × rate/44100 | 37632 at DSD64 |
| filter taps | up to 128 | 9-bit signed coefficients |
| probability entries | up to 64 | 7-bit values, 1..128, an error rate out of 256 |
| bin width | 8 | `bin = abs(predict) >> 3`, saturating at the table length |
| coder width | 12 bits | `a` starts at 4095, renormalises below 2048 |
| history | 128 bits of `0xAA` | per channel, reset every frame |
| refinement | 12 steps × 2 starts | step 0.35, decay 0.7 |
| gradient planes | 4 | weight quantisation for the popcount kernel |
| probability scale | 256.5 | rather than 256, for the coder's interval rounding |

## Where this lives in the source

| file | contents |
|---|---|
| `src/dst.hpp` | the decoder: tables, filter LUT, arithmetic decoder, sample loop |
| `src/bits.h` | MSB-first bit reader, JPEG-LS Golomb |
| `src/dstenc.hpp` | the encoder: autocorrelation, filter design, refinement, coding |
| `src/bitwrite.h` | bit writer with backwards carry propagation |
| `src/encpool.hpp` | encoding frames on several threads |

`README.md` covers the containers and the measured results;
`docs/philips-encoder.md` compares this encoder against the reference encoder's
own algorithm, read out of a decompilation of its DLL.
