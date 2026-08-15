# BMF 2.01 — the three models, and which image reaches which

This is a second pass over the source, written after the round that split it
into 37 files and renamed them into subsystems. It is organised the other way
round from `ALGORITHM.md`: that one goes container → decision → model, this one
starts from the BMP you hand the program and follows the branches until a model
runs. Where the two disagree, the disagreement is marked and the measurement
that settles it is given.

Range coder internals are out of scope. Everything below treats `rc.encode` and
`rc.decode` as "spend this many bits on this interval" and says nothing about
how the interval is carried.

**The one-sentence answer.** BMF has three models, and which one runs is decided
*per plane* by trial encoding, not by the file's format. A 32-bit BMP can end up
with three different models in one stream.

---

## 1. What the reader accepts

`bmp_read.inc` is strict. A file gets past it only if:

* the header is a 40-byte `BITMAPINFOHEADER` — no `BITMAPCOREHEADER`, no v4, no
  v5 — with `biPlanes == 1`;
* the depth is 1, 4, 8, 24 or 32 — **16-bit BMPs are refused**;
* `biCompression` is 0, 1 (RLE8, and only at 8 bpp) or 2 (RLE4, and only at
  4 bpp);
* width and height are both 1..65535, and for 8 bpp and above,
  `width * bytes_per_pixel` also fits in 16 bits;
* `biClrUsed` is not negative and, at 8 bpp or below, not more than `1 << bpp`.

Two things happen on the way in that matter downstream.

**Run-length is decoded, not carried.** Both RLE forms are expanded to flat
pixels while reading. Nothing later knows the file was run-length coded, and
nothing writes it back — `rle8.bmp` and a flat 8-bit copy of the same image
produce byte-identical streams, which is why `testfiles/rle8.bmp` and
`testfiles/out_rle8.bmp` have the same reference size.

**A greyscale ramp loses its palette.** `bmf_compress` walks the palette
before compressing and checks whether entry *i* is exactly `(i·step, i·step,
i·step)` for `step = 256 >> bpp`. If it is, the palette is dropped and bit 6 of
`depth` is set instead. `t8g.bmp` goes this way; `t8p.bmp` has the same pixels
under a non-ramp palette and does not.

---

## 2. The two branch points

Everything about which model runs is decided in `image_compress.inc`, at two
`if`s.

### Branch 1 — depth ≤ 4 takes the short path

```c
if( (p_i->depth&0x3Fu)<=4 ) {
  plane_predictor = 0;
  plane_alt_model = 0;
  model_plane(p_i, p_i->pixels, p_i->pixels);
  goto LABEL_57;
}
```

No filter search, no colour transform, no plane descriptors, no per-plane
anything. One pass of the **main model** over the whole image at its native
depth. 1-bit and 4-bit images never reach the other two models, and the header
they get has no descriptor block for the decoder to read.

This is worth stating plainly because it is easy to miss: `search_filter` at 473
lines and `choose_plane_coding` at 656 lines -- most of what the encoder spends
its time on -- **never run for a 1-bit or 4-bit image**.

### Branch 2 — depth ≥ 8, planar or interleaved

For everything else, `search_filter` runs (§4) and returns one number,
`filtered`:

```c
if( filtered ) {
  for each plane k:  model_planes(img, buf, plane_desc[k+1].src_plane, 0);
} else {
  transform_planes(p_i, 0);          // transform in place, then …
}                                      // … one model_plane over everything
```

* **`filtered != 0` — planar.** One model pass per plane, over that plane's
  bytes, in `src_plane` order. Each plane carries its own descriptor and so its
  own model.
* **`filtered == 0` — interleaved.** The colour transform is applied in place to
  each plane that asks for one, and then a *single* model pass runs over the
  whole image at full depth. A 32-bit image is then one pass over 4-byte
  symbols, and one model handles all four planes.

The interleaved path is not a fallback. It wins whenever the planes are
correlated enough that coding them together beats coding them apart —
`med32`, `altp1`, `x_ep` and `xform2` all take it.

---

## 3. The plane descriptor, and how flags name a model

When depth ≥ 8, the header carries one descriptor per plane, written by
`pack_bits` and read by `unpack_bits` (`rc_io.inc`):

| field | bits | meaning |
| --- | --- | --- |
| `flags` | 4 | see below |
| `nrefs` | 2 | how many other planes the colour transform reads |
| `dc` | 8 | DC offset subtracted before coding — present only if `flags & 8` |
| `weight0` | 8 | first transform weight, biased by 64 — only if `nrefs > 1` |
| `weight1` | 8 | second — only if `nrefs > 1` |
| `weight2` | 8 | third — only if `nrefs > 2` |

`flags` is the whole decision:

| bits 0–1 | predictor | bit 2 | model |
| --- | --- | --- | --- |
| 0 | none | 0 | **main model** |
| 1 | MED | 0 | MED, then the **main model** on the residual |
| 1 | — | 1 | **p1 model** |
| 2 | — | 1 | **p2 model** |

Bit 3 is "this plane has a colour transform", which selects whether `dc` and the
weights are in the stream.

`plane_predictor` and `plane_alt_model` are set from these two fields and read
by `model_plane` and `unmodel_plane`, which is the only place the three
models are chosen between:

```c
if( plane_alt_model ) {
  if( plane_predictor==1 )  p1 model   (…_d8_… at depth 8, generic otherwise)
  if( plane_predictor==2 )  p2 model   (…_d8_… at depth 8, generic otherwise)
} else {
  main model
}
```

> **A naming correction, made while writing this.** Nine file headers called p1
> "the alternate model for one-bit planes" and p2 "the model for deep planes".
> Both were wrong, and the measurement in §8 is what says so: `t24`'s **8-bit**
> planes go through p1, and `t8g` — a single 8-bit plane — goes through p2. The
> names are `plane_predictor == 1` and `== 2` and have nothing to do with bit
> depth. `ALGORITHM.md` had it right ("Model B — the alternate model for
> predictor 1") and the source comments did not; they say "predictor 1" and
> "predictor 2" now.

### the two bytes that carry the decision

`BmfImage` is sixteen bytes and two of them decide everything the decoder does.

**`depth`** — bits 0–5 are the bit depth, and the top two are what the reader
worked out about the palette:

| bit | set means |
| --- | --- |
| `0x80` | a palette follows the pixel data, three bytes an entry |
| `0x40` | greyscale — the palette was an exact ramp and was dropped |

**`flags`** — the decoder reads five bits, and a sixth is written but never read:

| bit | set means |
| --- | --- |
| `0x02` | the image was **transposed** — rows and columns swapped by the search |
| `0x04` | "slow" mode; this build prints `written in fast mode` and exits 3 without it |
| `0x08` | planar — each plane coded separately |
| `0x10` | a descriptor block is present; without it the whole image goes to the main model |
| `0x20` | coded; **clear means the rest of the member is raw pixels** |
| `0x40` | rows are sub-byte packed. Set by `alloc_image` for depths below 8, carried into the stream, and never tested on the way back |

`0x20` is the raw fallback. `compress_image` codes the image, compares the
result against `data_size`, and if coding did not shrink it, throws the coded
buffer away and writes the pixels. `noise24.bmp` is the corpus's example: the p2
model runs over it interleaved, produces nothing smaller than 24 bpp, and the
stream that reaches disk is raw.

---

## 4. The decision layer

Two bodies decide, and both work by *estimating cost from a histogram* rather
than by classifying the image. `estimate_cost` (`sym_code.inc`) is the common
currency, and it is exactly the histogram's entropy in bits:

```
(N·ln N − Σ nᵢ·ln nᵢ) · log₂e
```

summed in two accumulators over even and odd bins, which is the original's
pairing and changes nothing but the rounding.

### 4.1 `choose_plane_coding` — transform, weights, DC

Runs first, over the whole image. It picks:

**Which plane predicts which.** Three candidate rotations of the plane order are
costed by `cost_candidate`, and the cheapest wins. For each rotation that body
accumulates the five covariances of the two neighbour-gradient differences,
solves the 2×2 least-squares system

```
w1 = (Syy·Sxz − Sxy·Syz) · inv
w2 = (Sxx·Syz − Sxy·Sxz) · inv        inv = 128 / (Sxx·Syy − Sxy² + 0.1)
```

clamps both to [−64, 191], builds a residual histogram under those weights, and
returns its estimated cost. It also compares five simpler forms — the plain
gradient, `y−x`, `z−x`, `z−y` and `z−(x+y)/2` — and reports the best.

**The transform weights.** A hill-climb: from the candidate's weights, step each
of the two weights ±1 up to four times, rebuild the 512-bin residual histogram
each time, keep whatever is cheapest. The search is bounded to [−64, 191] and to
a step of 4 in each direction.

**The DC offset.** A 256-wide window is slid across the 1024-bin residual
histogram and the position with the most mass wins; `dc` is that position. This
is a mode, not a mean — it puts the *most common* residual at zero.

**The fourth plane, if there is one.** For a 32-bit image the alpha plane gets
its own three-variable least-squares solve against the other three, and its own
choice between four predictors (linear combination, or plain difference from
each of the three planes).

### 4.2 `search_filter` — trial encoding

Where the actual model choice is made, and what `-Q9` spends its time on.

It crops a tile from the centre of the image (the whole image if it is small),
allocates a scratch coded buffer, and then **encodes that tile repeatedly**,
once per candidate flag value per plane, keeping whichever produced the fewest
bits. The candidates are the flag values 0, 5, 6, 8, 13 and 14 — that is, the
main model with and without a transform, p1 with and without, p2 with and
without. It also encodes the transposed image and keeps the transpose if it
wins, and finally compares "all planes planar" against "everything interleaved",
which is the `filtered` return value.

Costs are measured as `8 * (out_cursor - coded_buf)` — actual coded bytes, not
an estimate. Between trials the packer state is reset and the plane descriptors
are saved and restored.

> **One property worth knowing before trusting the comparison.**
> `choose_plane_coding` sets `alphabet_reduced = 1`, and `compress_image`
> sets it back to `0` before the real encode. That flag gates the p2 model's
> whole neighbour-counter cascade (§7.3). So the search measures a *cheaper*
> p2 model than the one that finally runs. The trial costs are consistent with
> each other, which is all the search needs, but they are not the final sizes.

---

## 5. Model A — the main model

`model.inc`, 1,882 lines, the two largest bodies in the program. It is a
*match* model rather than a *prediction* model: it never does arithmetic on
symbol values, only asks whether this pixel equals some other pixel. That is
what makes it right for palette images, where symbol 200 and symbol 201 have no
numeric relationship, and it is why it is the only model available below 8 bits.

Each pixel goes through up to three stages, and stops at the first that hits.

### 5.1 alphabet reduction

`sym_reduce.inc` runs once per plane before any pixel is coded. It marks which
of the `2^depth` symbol values actually occur, then:

1. codes the **count** of distinct symbols;
2. codes the **set**, as sorted gaps between used values, each gap through a
   dense `SymList`;
3. rewrites the plane with each pixel replaced by its **index in the reduced
   alphabet**.

Everything downstream is sized by what the image uses rather than by its depth.
A 256-colour palette image using 40 entries pays for 40 gaps and then works in a
40-symbol alphabet.

### 5.2 stage one — the five-way selector

The four named neighbours are north (`sym[8]`), west (`sym[10]`), north-east
(`sym[6]`) and north-west (`sym[5]`). Stage one codes one of five outcomes:
*equals north*, *equals west*, *equals north-east*, *equals north-west*, or
*none of these — escape*.

The frequencies come from a `FreqRec` — seven 16-bit weights and two bytes — and
which record is a large context:

* six match bits from the four neighbours (`nb`);
* four more from the row above that (`nb2`);
* four "is this gradient flat" bits and two "did four rows above all match" bits
  (`sig1`);
* the pair `(last symbol here, previous symbol here)` classified against the
  four neighbours, giving 15 states × 5 × 5;
* and, chaining the above, three levels of *learned* context id — `ctx_id1`,
  `ctx_id2`, `ctx_id3` — each a table that hands out a fresh dense id the first
  time a raw context is seen. This is how the model gets a large context space
  without a table sized for it.

The record's weights are halved when their total passes a threshold, and the
escape weight has a floor of 256 so an escape never becomes uncodable.

### 5.3 the run shortcut

Before stage one, if the west and north-west pixels both matched and a long
strip of the row above is all matches, the model measures the run of pixels
equal to north, codes its length against a bucketed context, and then *copies*
that many records forward — cursor advances, match flags set to
`0x01010101` — without coding a symbol each. This is what makes flat regions
nearly free.

### 5.4 stage two — the 32-candidate list

On escape, the model builds a list of 32 candidate symbols:

* slots 0–1: the two most recent symbols seen at this exact context — the
  `SymPair` the stage-one selector also read;
* slots 2–9: an eight-entry move-to-front **cache**, `sym_ctr[8 * key]`, keyed
  by the same neighbourhood hash;
* slots 10–31: **spatial neighbours**, widening outward — west−2, north+2, the
  row two above and its neighbours, west−3, north+3, north−3 … out to west−7,
  north+7 and the row four above.

`sym_pos` walks the list. For each candidate, `pixel_context` decides whether it
is worth a bit at all: a candidate already excluded is skipped, and one with no
supporting evidence past position 6 is skipped. Candidates that survive are
coded as one binary decision each, against a counter selected by how many of the
near band (slots 11–15) and far band (slots 16–31) agree with it, and by whether
the candidate is near the top of the west and north symbol lists.

Every candidate that is coded and rejected is added to the exclusion mask, so
stage three never spends probability on it again.

### 5.5 stage three — the symbol lists

If all 32 candidates miss, the model falls through to `SymList` (`sym_list.inc`)
— an adaptive frequency list, kept sorted by count, with move-to-front on each
hit and a halving rescale. Lists are tried in order:

1. the order-1 list conditioned on **west**;
2. the order-1 list conditioned on **north**;
3. successively broader fallbacks, ending in a dense list of the whole reduced
   alphabet.

Coding within a list is under the exclusion mask built by stage two, so the
counts of already-rejected symbols are subtracted from the total. A list that
cannot code the symbol emits an escape covering all its live symbols, marks them
all excluded, and the search moves to the next list.

---

## 6. Model B — the p1 model (predictor 1)

`alt_p1_block.inc` and `alt_p1_code.inc`. Selected when `flags & 4` and
`flags & 3 == 1`. Unlike the main model this one *predicts a value* and codes
the residual, so it wants planes where neighbouring values are numerically
close — continuous-tone data, not palette indices.

### 6.1 the predictor

`AltP1Block::ctx_of` starts from the LOCO-I / MED predictor: with north `N`,
west `W` and north-west `NW`,

```
if NW >= max(N,W)  →  min(N,W)
if NW <= min(N,W)  →  max(N,W)
otherwise          →  N + W − NW
```

written in the source as the two nested comparisons the original compiled to.

### 6.2 the context

The residual is coded against a context assembled from nine two-or-three-valued
selectors plus an activity level:

* **activity** — a weighted sum of ten neighbouring residual magnitudes at
  weights 1, 2, 3, 4 and 6, plus one to five more from the reference planes when
  there are any, quantised through `level_of[]` into a level and through
  `group_of[]` into a group;
* **nine selectors**, each contributing `ctx_w[i].w[sel]` — the sign of the
  prediction error at various neighbours, whether the gradient exceeds the
  level's step, and where the predicted value sits against 22 and 216;
* **a quiet bit**, set when the surrounding magnitudes are all zero;
* and, when other planes are available, three of the selectors are replaced by
  cross-plane ones — the same residual in the reference planes.

The nine weight tables are what makes this a mixing model rather than a
concatenation: each selector adds an offset, and the offsets are per-context
learned values, not fixed strides.

### 6.3 coding the residual

The residual is folded to unsigned by `fold[]`, built once by
`alt_init_tables` — `unfold[]` runs 0, −1, +1, −2, +2, … so a small residual
of either sign gets a small code, and `fold[]` is its inverse. (The table is
built in buckets of `2*near_lossless_q + 1`; at `E = 0` that is one, and the
mapping is the plain zigzag.) The folded code is coded by `CounterNode::code_symbol` against a seven-slot frequency table:
slots 0–4 are the five smallest folded residuals coded directly, and slots 5 and
6 are escapes by parity into a binary symbol tree (`code_symbol_tree`) that
codes `(sym − 5) >> 1`. The table rescales when its total passes 0x2000, with
the divisor depending on whether any slot has fallen to 1.

### 6.4 near-lossless

The encoder reconstructs each sample after folding and checks whether the
reconstruction has drifted more than 16 from the source; if it has, it re-sends
through `fold_hi[]` and stores the original byte. The decoder is simply told
which code was used. This is the machinery for `-E` lossy mode; at `E = 0` the
drift never exceeds the threshold, so the second path is present but not taken.

---

## 7. Model C — the p2 model (predictor 2)

`alt_p2_block.inc`, `alt_p2.inc`, `alt_p2_context.inc`, `alt_p2_model.inc` —
2,030 lines, the most elaborate of the three. Selected when `flags & 4`
and `flags & 3 == 2`.

### 7.1 an NLMS linear predictor

Where p1 uses a fixed MED rule, p2 runs an **adaptive linear filter**. The
context row `p2_row[7][4]` holds 28 neighbour features — differences of `dval`
at various offsets, and for a plane with references, the same differences in the
reference planes. The prediction is a dot product against a weight row, and the
weights are updated by normalised least mean squares:

```c
err = (target − prediction) * 2.1;
for each coefficient:
    ms = ms + (x*x − ms) * ms_rate;                 // running mean square
    w += rate[j][k] * err * x / (ms + floor);       // normalised step
```

`NbRow::predict` adds a second layer: a fixed mixture of six weight sets forms
a *reference* prediction, the block's own adapted weights form another, and the
output is the reference plus a scaled difference between them, with the scale
itself decaying from 47/169.2.

### 7.2 which weight set — the context index

Weight rows are not per pixel; they are per *neighbourhood class*. Four
activity sums are formed over the four difference directions — `sum4` (left),
`sum_u` (up), `sum_ul` and `sum_ur` — each a weighted sum of eight neighbours'
difference fields, with a per-direction bias from `ctx_bias`. From those:

* `band` — five comparisons of `8*sum4` against 2, 5, 9, 17 and 43 times
  `sum_u`, so 0..5;
* `gA` — four comparisons of the total against `bmf_p2_thresholds[band][9..12]`,
  so 0..4;
* `gB`, `gC`, `gD` — three comparisons each of three other ratios, so 0..3.

These combine into a slot index:

```c
nb_slot = CtxIdx{}.bits<4, 2>(gB).bits<2, 2>(gC).bits<0, 2>(gD)
                  .digit<64, 5>(gA).digit<320, 6>(band);
```

`gA` and `band` are **digits, not bit fields** — five values at stride 64 and
six at stride 320 — which is why `nb_id[]` is sized 1920 and not 2048. Writing
`gA` as a two-bit field folds `gA == 4` onto `gA == 0`, which happens once in
the corpus and moves three streams. `CONTEXT-INDEX.md` records that experiment.

The first time a slot is seen it is handed a fresh weight-row id; after that the
row is reused and adapted.

### 7.3 the counter cascade

Prediction gives an expected value; the residual against it is coded through a
cascade of counters, and this is where the model's size comes from. `p2_ctr` is
163,840 `P2Count` records — a rate, a countdown and a 16-bit weighted value.

The cascade has **eleven levels**, one per bit of the context word from `0x4000`
down to `0x0010`. At each level it takes three records — the context with that
bit flipped (`d`), a mirrored variant (`m`), and a rotation of the low bits
(`r`) — and updates **four** counters: `d`, its predecessor `d[-1]`, `r` and
`m`. A fifth, `d[+1]`, gets the lighter update. So one sample touches 44
counters through `p2_update` and 11 through `p2_nudge`, at eleven different
context resolutions, and the prediction that comes out is their mixture.

Each counter is updated identically:

```c
p2_update(d0800, res_c, 2);      // w = p->weighted;
                                 // p->weighted = p2_bump(w, res − p2_pred(w, p->rate), 2);
```

115 of the file's 117 update sites are exactly that — the fold that
`CLEANER.md`'s first phase performed. 21 more sites fold the same arithmetic
into a `+=` on the next counter in the bank, at the same rounding but without
the dead-zone kick; those are `p2_nudge`.

The whole cascade is gated on `!alphabet_reduced`, which is the flag the filter
search leaves set — §4.2.

### 7.4 coding the residual

The folded residual goes to `P2Freq::code_three_way`: a three-way frequency record
where slot 0 is "the symbol" and slots 1 and 2 escape by parity into the binary
symbol tree, with the escape's context chosen by `idx & 1`. The records rescale
three-way when their total passes 29,696, halving the first two counts and
taking half of the third, and stepping the record's own increment down.

Around the record sits a second cascade: each of five weight selectors updates
its own neighbouring frequency records at 3/16, 5/16, 6/16 and 7/16 of their
step, so a symbol coded in one context also nudges the records on either side of
it.

---

## 8. Measured — what the corpus actually does

Probed by printing the decision at the point `compress_image` commits to it,
so this is the final stream and not what the search tried. Nineteen images.

`depth` and `flags` are read out of the reference streams themselves, at
offsets 14 and 15 of the member header, so they are the bytes on disk and not a
reconstruction.

| image | BMP | depth | flags | path | model(s), in coding order |
| --- | --- | --- | --- | --- | --- |
| `t1` | 1 bpp palette | `0x81` | `0x64` | short | main |
| `f05_200` | 1 bpp palette | `0x81` | `0x64` | short | main |
| `DLRAW` | 4 bpp palette | `0x84` | `0x64` | short | main |
| `rle4`, `out_rle4` | 4 bpp RLE4 | `0x84` | `0x64` | short | main |
| `x_ai` | 8 bpp RLE8, grey ramp | `0x48` | `0x3e` | planar, transposed | main |
| `x_ci` | 8 bpp RLE8, grey ramp | `0x48` | `0x3c` | planar | main |
| `t8g` | 8 bpp grey ramp | `0x48` | `0x3c` | planar | **p2** |
| `t8p` | 8 bpp palette | `0x88` | `0x3c` | planar | **p2** |
| `rle8`, `out_rle8` | 8 bpp RLE8 palette | `0x88` | `0x3c` | planar | **p2** |
| `t24` | 24 bpp | `0x18` | `0x3c` | planar | **p1**, **p1**, main |
| `noise24` | 24 bpp | `0x18` | **`0x00`** | interleaved | **p2** — then discarded, stream is raw |
| `t32` | 32 bpp | `0x20` | `0x3c` | planar | **p1**, **p1**, main, **p1** |
| `xform1` | 32 bpp | `0x20` | `0x3c` | planar | **p2** ×4 |
| `med32` | 32 bpp | `0x20` | `0x36` | interleaved, transposed | MED + main, one pass at depth 32 |
| `altp1` | 32 bpp | `0x20` | `0x36` | interleaved, transposed | **p1**, one pass at depth 32 |
| `x_ep` | 32 bpp | `0x20` | `0x36` | interleaved, transposed | **p2**, one pass at depth 32 |
| `xform2` | 32 bpp | `0x20` | `0x36` | interleaved, transposed | **p2**, one pass at depth 32 |

`noise24`'s flag byte is **zero**, not merely missing `0x20`: the raw fallback
writes the image's own header rather than the one the coder built, and that
header never had the coding bits set. So a raw member is not "a coded member
with one bit cleared" — it is a different header, and the decoder's first test
is the one that matters.

And the descriptors those images carry, which is the same decision from the
other side — flags, and the transform each plane got:

```
t24     P0 flags=8  (main, transform, nrefs=2, dc=42)   src=2
        P1 flags=13 (p1,   transform, nrefs=1, dc=128)  src=1
        P2 flags=5  (p1,   no transform)                src=0
x_ep    P0 flags=14 (p2,   transform, nrefs=2, dc=237, w=19,49)
        P1 flags=14 (p2,   transform, nrefs=1, dc=244)
        P2 flags=6  (p2,   no transform)
        P3 flags=6  (p2,   no transform, nrefs=3, dc=5, w=-10,-1,-15)
xform1  P0..P3 all flags=6 (p2), plane order 2,0,1,3, three of them with a DC
```

**Four things this makes concrete.**

*The choice is per plane.* `t24` codes one plane with the main model and two
with p1 in the same stream; `t32` codes three with p1 and one with the main
model. Nothing about the file format says so — the search measured it.

*The subformat does not determine the model above 4 bits.* `t8g` and `x_ci` are
both single-plane 8-bit images and land on different models. `t8p` and `t8g`
are the same pixels under different palettes and land on the same one.

*Coding order is not plane order.* `src_plane` is a permutation chosen by
`choose_plane_coding`, so `t24` codes planes 2, 1, 0 and `xform1` codes
2, 0, 1, 3. The transform for a plane reads planes that have already been coded.

*The p2 model can run and be thrown away.* `noise24` reaches p2, interleaved,
and produces nothing smaller than the raw pixels, so `compress_image` writes
the pixels and clears flag `0x20`. It is the corpus's only image where the
coded stream loses.

For the record, what each one costs:

```
f05_200  0.042   med32  0.022   altp1  0.149   out_rle8 0.083   rle8  0.083
out_rle4 0.208   rle4   0.208   t1     0.270   x_ai     0.264   x_ci  1.123
DLRAW    1.844   t8g    4.466   t8p    4.466   x_ep     4.690   t24   5.615
t32      5.622   xform1 9.656   xform2 11.969  noise24 24.000  (bits per pixel)
```

---

## 9. The decoder

`image_expand.inc` mirrors the branches exactly, reading the same decisions
rather than making them:

1. read and check the magic and the 16-byte `BmfImage`;
2. if `flags & 0x20` is clear, the rest of the member is raw pixels — read and
   return;
3. if `flags & 0x04` is clear, refuse: this build decodes `-S` streams only;
4. if depth ≤ 4 **or** `flags & 0x10` is clear, run the main model over the whole
   image and stop — this is branch 1's mirror;
5. otherwise read the near-lossless nibble, then one descriptor per plane with
   `unpack_bits`, exactly the fields §3 lists;
6. if `flags & 0x08`, decode each plane with the model its descriptor names and
   un-transform it; otherwise decode the whole image in one pass and
   un-transform the planes afterwards;
7. if `flags & 0x02`, transpose back.

The near-lossless nibble is read and refused: a non-zero value prints
`near-lossless stream (E=%d); this build only decodes E=0` and exits 3. The
encoder in this tree never writes one.

---

## 10. What is present and cannot happen

Three things exist in the source and no input reaches them.

**The fast back end.** BMF.exe had a second, cheaper coder selected by the
absence of `-S`. It was not decompiled; the decoder refuses streams without flag
`0x04` and the encoder always sets it.

**Near-lossless coding.** The `fold_hi` path in both alternate models, the drift
check that selects it, and the 4-bit nibble in the header are all present and
wired. With `E = 0` the drift never exceeds 16, so the branch is never taken.

**Predictor mode 0 on the expand side.** `expand_predictor_mode0` is an empty
body — the original's counterpart to a predictor this build never selects.
