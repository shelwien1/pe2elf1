# bmg — what the program does to an image

`bmg` is a lossless Grey/RGB/RGBA BMP compressor built on bcdr5's range coder,
adaptive counter and IDX parameter framework, with the model taken from
`ALGORITHM_v2.md` — the second reading of BMF 2.01.

This document is the counterpart of that one: what was taken, what was changed,
and what was tried and thrown away. Section numbers in the form §9.2 refer to
`ALGORITHM_v2.md` throughout.

---

## 1. From the outside

```
bmg c input.bmp output.bmg      compress
bmg d input.bmg output.bmp      expand
```

Exactly two forms, four arguments each, and the mode letter must be a single
character — `bmg cc` is not `c`. Anything else prints usage and exits 1.

`bmg c` reports the geometry, the coded size and the bits per pixel; `bmg d`
reports the time. Both write to stderr, so the size line does not contaminate a
pipe.

---

## 2. The container, and why it is byte-exact

BMF stores *pixels*: §3.2 says so plainly — a decoded RLE file is pixel-identical
to its input and byte-different from it, because `write_bmp` re-encodes the runs
with its own splitting rules. §4.2's raw fallback likewise writes the pixels and
not the file.

`bmg` does not do that. **A `.bmg` round-trips to the byte**, for every input,
including inputs the image model cannot touch. The whole file is the unit:

```
offset 0   4 bytes   'B','M','G','1'
offset 4   4 bytes   the original file's length, little-endian
offset 8   1 byte    mode
offset 9   ...       mode 2: the file verbatim; otherwise the coded stream
```

Three modes:

| mode | |
| --- | --- |
| 0 | generic — every byte through an order-1/order-2 mixed byte model |
| 1 | image — the model of §4 below |
| 2 | stored — the file verbatim (§4.2's raw fallback, one level up) |

The encoder picks mode 1 when `BmpInfo::parse` accepts the file and, for a
run-length input, when the RLE stream also decodes cleanly; otherwise mode 0.
Either way, if the coded stream did not come out shorter than the input, mode 2
is written instead — so a `.bmg` is never more than nine bytes larger than what
went into it.

Inside mode 1 the stream is, in order:

1. **the prologue** — every byte before the pixel data, which is the 14-byte file
   header, the DIB header at whatever size it declares, the palette if there is
   one, and any gap. Coded as a delta against the byte four back, contexted on
   the position modulo four and the previous delta at the same offset. That is
   what a palette *is*, and a grey ramp collapses to four perfectly predictable
   contexts — 1024 bytes of ramp cost about thirty.
2. **the transform descriptors** — which plane leads and what each of the others
   subtracts of it (§5.2 below).
3. **the planes**, interleaved (§4).
4. **the palette-expansion inverse**, when that path was taken (§3).
5. **the pixel bytes back into the file** — for an uncompressed BMP that is the
   row padding, which is usually zeros and costs nothing; for an RLE8 BMP it is
   the run structure (§6).
6. **the trailing bytes**, if the file has any past the pixel data.

The decoder learns the geometry by decoding the prologue and then *parsing* it,
exactly as the encoder did. Nothing about the image is on the wire twice.

The header's own `bfSize` field is deliberately **not** trusted: files whose
declared size disagrees with their actual size are common, and refusing them
would send them to the generic model for no reason. Every bound is checked
against the real length.

---

## 3. What the image model accepts

`BmpInfo::parse` takes a 40-byte `BITMAPINFOHEADER` or larger, with

- width in 1…65535 and height in 1…65535 (a negative height is read as a
  top-down bitmap rather than refused);
- `biPlanes == 1`;
- depth exactly 8, 24 or 32;
- `biCompression` 0, or 1 (RLE8) with a depth of 8 and a positive height;
- the palette, if the depth has one, fitting between the DIB header and the
  pixel data;
- the pixel data fitting inside the file.

Anything else goes to the generic model, which is why this is allowed to be
strict.

### 3.1 the palette, and the two ways an 8-bit image can be read

§1.1 detects a linear grey ramp and throws the palette away, saving 768 bytes.
`bmg` does not need that special case — the prologue model codes a ramp for
almost nothing — but it does need the *other* half of the question: **what do
the indices mean?**

- A palette that is grey (`r == g == b` everywhere) and non-decreasing is an
  ordinal scale. The indices are samples and the plane is coded directly.
- A colour palette is not. Index 200 and index 201 have nothing to do with each
  other, and MED prediction on them is arithmetic on labels.

For a colour palette the encoder costs both readings with §6.3's tool —
`estimate_cost` over the MED residuals — and puts one bit on the wire:

- **as indices**, one plane; or
- **expanded**, the palette applied to give three planes of B, G, R, coded as a
  colour image, with the inverse mapping coded afterwards.

The expansion inverse handles a palette with two entries of the same colour,
which is legal and has to round-trip: colours are hashed into a table of index
lists, and where a list holds more than one index the choice is coded as a rank
in `[0,k)` with the constraint respected bit by bit, so a count that is not a
power of two costs no more than it should.

Measured on the corpus: `t8p.bmp`, whose palette is an 8×8×4 colour cube,
**refuses** the expansion — 46 KB as indices against 60 KB expanded. The index
bits of a cube palette *are* the colour components, and coding them directly
beats reconstituting them. The estimate gets that right, which is the whole
reason it is an estimate and not a rule.

---

## 4. The plane model

### 4.1 planes, margins, and the order they are coded in

A 24-bit image is three byte planes, a 32-bit image four, held separately with a
margin of eight samples on the left and right and eight rows above. §10.5's point
is that filling the margins is what lets the predictor have no branches in it:
the left margin of a row is the leftmost sample of the row just finished, the
right margin is that row's last sample, and everything above row 0 is a neutral
128. Every pixel, including the first one of the first row, reads a full
neighbourhood.

Planes are coded **interleaved** — plane 0, 1, 2, 3 of a pixel before moving on —
which is what lets a plane's context see what the other planes just did at the
same pixel. §9 makes the same distinction between its per-plane and interleaved
drivers, and notes that only the interleaved one can do this.

### 4.2 the colour transform (§7.1)

Before a plane is modelled, what the planes it references predict of it is
subtracted, in the value domain and modulo 256:

```
D = ( P  −  (w0·R0 + w1·R1 + 64)/128  −  dc )  &  255
```

`w0`, `w1` and `dc` are on the wire, one set per plane. This is §7.1's
`PlaneTransform::blend` with a rounding constant of 64 rather than BMF's 40 and
63/64 — bmg is a new format and has no byte-identical output to preserve, so it
rounds to nearest and does so the same way everywhere.

**This is the single largest thing in the compressor for colour images, and the
form of it matters more than it looks.** Subtracting the reference plane's
*residual* — which is what a cross-plane term in a predictive coder normally
means — was measured first: it takes `t24`'s three planes from 17.7 to 10.4 bits
per pixel. Subtracting the reference plane's *value* before MED runs takes them
to 7.3. The residuals are not the correlated part; the values are.

### 4.3 which plane leads (§6.3)

Three candidate orderings, one per choice of hub plane, each costed by
`estimate_cost` over the MED residuals of the transformed planes, cheapest wins.
Within a candidate the weights are fitted by coordinate descent from a coarse
ladder, halving the step, exactly as §6.3 step 3 describes; `dc` centres the
transformed plane on 128 on a subsample, so the modulo-256 wrap of the transform
lands where the data is not.

The hub goes first because nothing can predict it; the second plane references
the hub; the third references the hub and the second; a fourth plane — alpha —
references the hub and the second and normally comes back with weights of zero,
which is the search noticing that alpha is not a colour.

### 4.4 prediction

MED / LOCO-I (§9.1), and nothing else:

```
NW ≥ max(W,N)  →  pred = min(W,N)      a vertical edge left of the pixel
NW ≤ min(W,N)  →  pred = max(W,N)      a horizontal edge above it
otherwise      →  pred = W + N − NW    a plane through the three
```

Which of the three branches answered is kept and used as context. It is a
statement about the local edge and it costs nothing to carry.

**What is not here.** §10's learned predictor — an NLMS filter over the
neighbourhood — was built, in integer arithmetic so it would be reproducible,
and then removed. Once the colour transform of §4.2 is doing the cross-plane
work in the value domain the filter has nothing left to find, and it cost 0.8%
on the corpus at every step size tried, monotonically improving as the step went
to zero. A JPEG-LS bias corrector went the same way: it is worth 2.7% on `t8g`
and costs 4.3% on `t24`, and neither a magnitude gate nor a self-scoring on/off
switch nor feeding it as context instead of as a shift separated the two cases.
Both are absent from the source rather than switched off in it.

Two things about that are worth writing down, because they cost a day between
them:

* **An arithmetic right shift is not a divide.** Every gradient step in a mixer
  or an LMS is a signed product scaled down by a shift, and `>>` rounds toward
  minus infinity: a step of −1 for every negative product too small to register,
  and none for the positive ones. In the NLMS filter that walked every weight to
  its clamp and left the correction larger than the error it was correcting —
  `t24` went from 118 KB to 72 KB when the filter's power estimate stopped
  drifting. In the mixer the same expression costs 0.01%, because there the
  error term shrinks as the prediction improves and the weights stop moving
  before the drift accumulates. Everything goes through `rsh()` anyway: rounding
  is the cheap way not to have to know which case one is in. The bug was
  invisible because the code looked exactly like the textbook.
* **The mean is the wrong statistic for a residual.** A bias corrector minimises
  the mean error; an entropy coder wants the mode. On a plane that is ninety
  percent zeros with lopsided rare spikes those are different numbers, and
  moving the prediction to the mean turns every one of those zeros into a ±1.

### 4.5 context

An **activity measure** over the magnitudes of the neighbours' errors, weighted
`6·W + 4·N + 3·NW + 3·NE + 2·NN + 2·WW + NNE + NEE`, plus the reference planes'
magnitudes at this pixel, quantised `(act+7)>>4` into 0…511 and looked up in two
step functions built from two edge lists (§9.2):

```
activity levels   1, 2, 4, 8, 14, 35, 103
escape groups     1, 3, 6, 10, 16, 27, 52
```

Deliberately not the same ladder — that is §9.2's point and it is kept, with the
level ladder addressing the counters and the group ladder addressing the escape
strips, so the two quantisers disagree and each index carries something the
other does not. A third table turns the level into a deadband width, so the
ternary features get coarser exactly where the neighbourhood is busier.

**Nine ternary features**, each 0, 1 or 2, combined mixed-radix base 3 (§9.2):
the prediction against the range ends, NW against N, NW against W, NN and NE
against the prediction, three second-order gradients, and the reference plane's
residual.

**The fifteen partitions** (§8.3). The six pairwise equalities among N, W, NW,
NE are an equivalence relation, so only the patterns closed under transitivity
occur — the partitions of a four-set, Bell(4) = 15 of them, and computing the
signatures directly gives BMF's `ctx_group_flags` exactly, in that order. This is
model A's reading of a neighbourhood as *equalities* rather than as quantities,
which is what suits a palette or a screenshot, and it is carried here as one
context dimension rather than as a separate model.

Ten context models feed the mixer:

| | context |
| --- | --- |
| M1 | seven ternary features × activity level × plane class |
| M2 | all nine features, both activity ladders, both neighbour errors — hashed |
| M3 | quantised W and N errors × predicted value × activity × plane class |
| M4 | activity × plane class × zero-run length × W error — fast rate |
| M5 | the two reference planes' residuals × activity × plane class |
| M6 | partition × MED branch × activity × plane class |
| M7 | match state × match length × plane class |
| M8 | the low nibbles of W and N × activity × plane class |
| M9 | the exact four-neighbour values, hashed |
| MA | the exact eight-neighbour values, hashed |

M8 is §8.5's `ctx_id3` idea — that context refines itself with the low nibble of
W, and it is there for the same reason: on data that came through a colour
quantiser or a palette, the low bits of a neighbour say which cell of the
quantiser grid this pixel sits in, and no gradient or activity measure can see
that. It is worth 3.0% on the corpus and 6% on `x_ai`.

M9 and MA are worth another 3.9% between them, which is the answer to a question
§8 raises without settling: model A's four-way ladder and its escape ladder are
asking "is this pixel one of its neighbours, and if not, which of the symbols
nearby is it" — and a residual coder can ask the same question by carrying the
exact neighbourhood as a context and letting the mixer weigh it.

### 4.6 the match model

A hash of eight neighbour values indexes a table of positions. What followed
that neighbourhood last time is a prediction; how many consecutive times it has
been right is a match length. Neither is used to change the prediction — the
match state and the match length are context, and the mixer decides how much a
match of that length is worth.

This is §8.5's run mode and its four-way ladder in the form a residual coder can
use. Added together with the partition context of §4.5, the two are worth 5.3%
on the corpus, and the match hit rate ranges from 0.4% on `t8g`, which is index
data being read as grey, to 96% on `t24`.

### 4.7 coding the residual

The residual is folded modulo 256 into [−128,127] and zigzagged — 0 → 0,
−1 → 1, +1 → 2 — so small residuals become small codes (§9.3). Then:

* a **literal ladder**: is the code 0? is it 1? … up to `NLIT`, twelve by
  default. Each rung is one binary decision, mixed from all ten models plus a
  bias input, refined by two SSE stages, and coded.
* an **escape** into a level and a path (§5.4): the level, which is
  `floor(log2(v))` of what is left, coded as a unary chain of its own mixed
  decisions against three models; then the level's payload bits down a binary
  tree, mixed from two.

A unary ladder is near-optimal where the distribution is monotone, which a
zigzagged residual distribution is, so the ladder is exact where it reaches and
the escape is logarithmic beyond it. `NLIT` is a compile-time constant because
it is a table dimension; twelve is the corpus optimum and the curve is flat
between eight and sixteen.

The escape strip is indexed by the *group* ladder rather than the *level*
ladder, for the reason §9.2 gives.

### 4.8 counters, mixing, and rates

The counter is bcdr5's: one 16-bit probability, an EMA toward the coded bit,
bounded into `[mw, SCALE−mw]`. Its `P` is `P(bit==0)`, which is the same
convention `rc_BProcess` uses, so a counter's probability goes to the coder with
no complement anywhere in the model.

Two things are added around it.

**A use count per context row.** A fixed-rate EMA throws away most of what the
first few observations of a context are worth: the n-th of them should move the
estimate by about 1/n, not by the steady-state rate. Every model carries one
byte per row, the count picks the rate out of a `1/(n+1.5)` schedule floored at
the tuned rate, and — the half that matters more — a row that has never been
seen feeds the mixer a **neutral** input instead of its seed value, so a cold
model does not vote. Worth 1.4%.

**A gated linear mixer**, in the stretched domain, one weight set per
`(rung, activity, plane class, run length, match state, MED branch)`, followed
by two SSE stages on different questions — one on the gradient state, one on
what the match model is doing — averaged with the mixer's own answer.

§5.5 describes BMF training the strips *next to* the one that coded, because
nothing blends predictions at read time: "context mixing done by writing rather
than by averaging". Here something does blend them, so those neighbour writes
become a refinement rather than the only mechanism — the two activity levels
either side of the coding context are still updated, at their own slower rate,
which is §9.4's idea kept for what it is worth on its own.

---

## 5. Below the model

### 5.1 the coder

bcdr5's carryless range coder, unchanged. Two interfaces: `rc_Process` for a
frequency-table symbol and `rc_BProcess` for a binary decision against a 15-bit
probability. The stream lives in memory rather than in a `FILE*`, because the
encoder needs its own length before it can decide whether coding beat storing.

One detail is load-bearing and easy to lose in that change: **the decoder must
read `0xFF` past the end of the stream, not 0.** `rc_Quit` drops a tail byte
whenever setting the low bits of `low` to ones still lands inside `[low,high)`,
so the omitted bytes are ones. bcdr5's `FILE*` reader got that for free from
`byte(EOF)`; a memory reader has to say it. The symptom is one wrong byte, the
last one coded, and nothing else.

### 5.2 the generic and prologue models

Both are plain binary trees over bytes with mixing. The prologue model codes a
delta against the byte four back (§2 above). The generic model is order-1 and
order-2 mixed, and it carries the trailing bytes, the row padding, and any file
the image model refused.

---

## 6. RLE8, preserved exactly

§3.2 decodes both run-length forms and does not preserve them. `bmg` preserves
RLE8 byte for byte, and it costs almost nothing, because **the pixels are coded
first and the ops are coded against them.**

By the time the run structure is coded, both halves have the whole image. So for
each op the model already knows:

- the run length available at the cursor, and the value of the pixel there;
- how far the next run of three or more is — which is what a greedy encoder's
  literal run stops at;
- every byte an absolute run will contain.

What is left on the wire is the *encoder's choices*. They are coded twice over:
first as **one bit** saying whether this op is, whole, what a greedy encoder
would have emitted here — the op, its length and its data — and only if that
bit says no, as an op code, a length, and a "matches the image" flag in front of
the data with a raw fallback behind it.

So a stream from a greedy encoder costs about a third of a bit an op and a
stream from anything else still round-trips. On `x_ci`, whose 3.2 MB of RLE
carries a million ops, the whole run structure costs **334 bytes**.

The context that bit is read in decides whether it is worth having, and by a
lot. Read against a coarse "is there a run of three or more here" it hits 89% and
*costs* more than coding the op longhand. Read against the run length and the
literal length at full resolution below four — because a run of exactly two is
the case where an encoder may reasonably emit either a pair or a literal, and
lumping it in with "three or more" is what makes the bit expensive — the same
89% hit rate is worth 7.4% of `x_ci`. Predicting the absolute-run length from
the greedy rule was worth another 12% before that, taking those lengths from
missing every single time to missing never.

RLE4 is not accepted: it needs a 4-bit depth, which is outside Grey/RGB/RGBA.
Those files go to the generic model.

---

## 7. Parameters

Fifty-one knobs are declared in `IDX/bmg-P0.idx` and generated into
`MOD/bmg-P0_h.inc` by `IDX/idx2inc.pl`: every counter rate and probability
floor, the mixer learning rate and initial weight, both SSE rates and their
shares, both activity ladders, the activity weights, the per-level deadbands.

```
./mk.sh                     # tuning build:   Debug 1, Const 0, knobs are objects
./mk.sh release             # shipping build: Const 1, knobs are literals
./mk.sh check               # prove the two code identically
cp bmg bmg_opt
perl IDX/opt.pl opt.lst     # hill-climb the binary; writes export.!!!
cd IDX && for f in bmg-*.idx; do perl import.pl $f ../export.!!! > t && mv t $f; done
```

`opt.pl` does not parse or rebuild anything: `pdesc` embeds each pattern in the
executable as `!MAP!name!base\0pattern`, the optimizer flips bits in the binary,
re-runs the corpus and keeps what shrinks the total. `mk.sh check` is the test
IDX-FORMAT.md §12 asks for, and it passes on every corpus file.

`opt.lst` names five images cropped small enough that one measurement is about a
second: two of them 8-bit, two 32-bit, one 24-bit, so no model is left untouched
while the others move.

Every knob is clamped at the point of use rather than in the `.idx` — the
pattern is the search space and the clamp is the contract (IDX-FORMAT.md §5).
The ladders are sorted and bounded, the SSE rate is bounded because it is a
shift, the mixer rate is bounded because its product must not overflow.

### 7.1 one fix to the shared tooling

`idx2inc.pl` did not strip trailing whitespace left behind by a stripped
comment, and `Number`'s pattern is matched with a greedy `(.*)` where `Rate`,
`Rate1` and the mapping forms all anchor with `\s*$`. A comment written after a
`Number` line therefore left spaces in the pattern, every one of them read as a
zero bit, silently multiplying the seeded value by a power of two — and
identically in both builds, so `mk.sh check` could not see it either. One line,
noted here because the file is shared with other projects.

`sh_mapping.inc` gained the three frozen macros (`mdesc`, `mmask`, `mmask2`)
that IDX-FORMAT.md §10 describes and that no copy of the file had ever carried.

---

## 8. Results

Corpus is `testfiles/`, the seven images from the BMF distribution that are
8, 24 or 32 bits deep. Round-trip is byte-exact on all of them.

| file | | input | bmg | bits/pixel |
| --- | --- | ---: | ---: | ---: |
| `t8g.bmp` | 320×240 grey | 77 878 | 45 859 | 4.78 |
| `t8p.bmp` | 320×240 palette | 77 878 | 45 934 | 4.79 |
| `t24.bmp` | 320×240 RGB | 230 454 | 56 361 | 5.87 |
| `t32.bmp` | 320×240 RGBA | 307 254 | 56 429 | 5.88 |
| `x_ai.bmp` | 2820×1600 grey RLE8 | 887 278 | 152 200 | 0.27 |
| `x_ci.bmp` | 2820×1600 grey RLE8 | 3 278 170 | 574 487 | 1.02 |
| `x_ep.bmp` | 705×800 RGBA | 2 256 054 | 353 074 | 5.01 |
| **total** | | **7 114 966** | **1 284 344** | |

For scale, the order-0 entropy of the MED residuals — which is what §6.3's
`estimate_cost` measures and what a good non-adaptive coder would reach — is
4.80 bpp on `t8g` and 17.69 bpp on `t24`. The colour transform accounts for most
of the distance on `t24`; context mixing accounts for the rest and for all of it
on the screenshots.

---

## 9. What is in the source and cannot happen

- **Near-lossless.** There is none. §9.5's drift check and `fold_hi` table have
  no counterpart here; `bmg` is lossless and the fold is always exact.
- **RLE4, 1-bit and 4-bit depths.** Refused by `parse`, handled by the generic
  model. §6.1's short path for depths of 4 and below has no counterpart because
  those depths are outside what this compressor claims.
- **A quality setting.** There is one model and it always runs. §6.4's trial
  encoding — the whole cost of BMF's `-Q9` — is replaced by estimates
  everywhere: the plane order, the transform weights and the palette reading are
  all settled by `estimate_cost`, and nothing is coded twice.
