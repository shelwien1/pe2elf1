# BMF v2.01 — Compression Algorithms

A detailed description of the compression algorithms in `bmf.cpp` — a reconstructed
source of Dmitry Shkarin's **BMF** lossless image codec, v2.01 (1998–1999, 2009).
The identifiers in the source were produced during reverse engineering, so names are
descriptive rather than original. Line references are into `bmf.cpp` as of this commit.

Range-coder internals (`struct RangeCoder`, `bmf.cpp:334`–498 — interval arithmetic,
renormalization, carry handling) are **out of scope** here; the coder is treated as an
ideal entropy-coding backend that consumes `(cumFreq, freq, totFreq)` triples and
adaptive bit probabilities. Everything that *produces* those statistics is in scope.

---

## 1. Big picture

BMF is a lossless (optionally near-lossless, vestigial in this build) codec for BMP
images of 1/4/8/24/32 bpp. Its architecture:

```
BMP in ──► normalize (grey-ramp detect, RLE-decode) ──► BmfImage (interleaved bytes)
                                                            │
                              ┌─────────────────────────────┘
                              ▼
                encoder mode search (search_filter):
                  · inter-plane decorrelation candidates (LS-fit weights + entropy heuristic)
                  · per-plane trial encodes: {mode0, P1, P2} × {refs, no refs}
                  · transpose trial, planar-vs-interleaved trial, all real coded bits
                              │
                              ▼  per-plane PlaneDesc descriptors
        ┌─────────────────────┼──────────────────────────┐
        ▼                     ▼                          ▼
  "slow" model          alt-P1 model               alt-P2 model
  (ModelBlock)          (AltP1Block)               (AltP2Block)
  universal symbol/     MED predictor +            NLMS linear predictor +
  palette CM coder      630K-context residual      5-stage context-bias cascade +
                        coder                      ternary/tree residual coder
        └─────────────────────┴──────────────────────────┘
                              │
                              ▼
                 binary range coder (out of scope)
```

Three pixel models coexist; the encoder picks per plane (or per image) by **measuring
actual coded bits** on trial encodes:

| model | selected by | intended content |
|---|---|---|
| **slow / ModelBlock** (`bmf.cpp:2054`) | default (no `desc_alt_model`) | paletted / indexed / arbitrary symbol planes; also any interleaved multi-plane data |
| **alt-P1** (`AltP1Block`, `bmf.cpp:1127`) | `pred_p1 \| desc_alt_model` | continuous-tone planes; MED prediction + huge direct product-context residual model |
| **alt-P2** (`AltP2Block`, `bmf.cpp:1635`) | `pred_p2 \| desc_alt_model` | continuous-tone planes; adaptive linear (NLMS) prediction + SSE-style correction cascade |

There is additionally a standalone **MED pre-filter** path: `pred_p1` *without*
`desc_alt_model` runs LOCO-I/MED prediction as an explicit in-place transform
(`predict_med`, `bmf.cpp:4962`) and feeds the folded residuals to the slow model.

Both encoder and decoder are generated from the same templates (`f_DEC` template
parameter throughout), and every model statistic is updated identically on both sides
from reconstructed data only — the classic CM symmetry that makes side information
unnecessary.

The build corresponds to `bmf -S -Q9` (slow mode, max search quality): the `opt_*`
constants (`bmf.cpp:283`–289) are baked in and, notably, are never read anywhere else
in the file — the reconstruction has the option handling constant-folded away.

---

## 2. Container and stream layout

(Summary only — this is framing, not compression, but the interleaving of raw bits and
range-coded segments matters for understanding the models' entry points.)

* A BMF file is a sequence of members: 4-byte tag (`bmf_tag`, `bmf.cpp:3271`; image
  signature `0x8A81`, auxiliary `0x9081`, version chars `'2','0'`), then a 16-byte
  `BmfImage` header (width/height/stride u16, 4 unused pad bytes, depth u8, flags u8,
  data_size u32),
  then an optional opaque `CodedTail` block, then the payload, then (for paletted
  images) the raw palette bytes. Auxiliary members are skipped by the reader
  (`expand_image`, `bmf.cpp:6434`).
* `depth` packs bit-depth (mask 0x3F) with `depth_grey=0x40` and `depth_palette=0x80`
  (`bmf.cpp:3081`). `flags` (`bmf.cpp:3088`) include `flags_transposed`, `flags_slow`,
  `flags_planar` (planes coded separately), `flags_descriptors`, `flags_coded`,
  `flags_tail`.
* If the coded body is not smaller than the raw pixels, the member is stored **raw**
  (`compress_image`, `bmf.cpp:6582`) — the format never expands beyond raw + header.
  Images with `data_size < 16` are always stored raw.
* Inside a coded payload, a **bit packer** (`Packer`, `bmf.cpp:236`; `pack_bits` /
  `unpack_bits`, `bmf.cpp:3538`/3551 — LSB-first accumulation into 32-bit words) and
  the range coder share one buffer. Raw-bit fields (the 4-bit near-lossless quantizer,
  the plane descriptors) are packed first; each plane's range-coded segment is then
  bracketed by `rc_begin`/`rc_end` (`bmf.cpp:3494`/3423). The range coder's flush
  writes a 3-byte length, zero-pads so the next byte falls at offset 3 mod 4, then
  writes a `0x97` sentinel as the final byte of that word — leaving the stream 4-byte
  aligned so the packer can resume word-aligned; `packer_rewind` (`bmf.cpp:3439`)
  reclaims the packer's partially-used trailing word before the coder starts. Planar
  mode thus produces `plane_count` concatenated range-coded segments after the
  descriptor bits; interleaved ("together") mode produces one.
* Per-plane descriptors are coded as raw bit fields (`code_plane_descs`,
  `bmf.cpp:6312`): 6 bits `(flags<<2)|nrefs` per plane, plus 8-bit `dc` when
  `desc_has_refs`, plus 8-bit weights (bias +64) when `nrefs≥2` (`weight0`,
  `weight1`) and `nrefs>2` (`weight2`).
* Images are never tiled: one member covers the whole image.

---

## 3. Input handling and internal representation

* `read_bmp` (`bmf.cpp:6040`) accepts BITMAPINFOHEADER-only bottom-up BMPs with bpp ∈
  {1,4,8,24,32}, RGB or RLE4/RLE8 compression (full RLE decoders at `bmf.cpp:5930`/
  5962, including delta and absolute ops with nibble-alignment handling). Rows are
  stored **top-down**, tightly strided, packed-pixel interleaved (B,G,R[,A] byte
  order); sub-byte depths stay bit-packed. Palettes are stored as 3-byte B,G,R
  triplets immediately after the pixel data (`BmfImage::palette()`, `bmf.cpp:3119`).
* **Grey-ramp detection** (`bmf_compress`, `bmf.cpp:6631`): a palettized image whose
  palette exactly equals the canonical grey ramp (entry *i* = *i*·(256>>bits) in all
  three channels) is converted to `depth_grey` and its palette dropped — the decoder's
  `write_bmp_palette` (`bmf.cpp:6101`) regenerates the identical ramp. Otherwise the
  palette is stored raw (never entropy-coded).
* On output, `write_bmp` (`bmf.cpp:6236`) re-encodes 4/8-bit images to RLE4/RLE8
  (`bmp_rle_encode`, `bmf.cpp:6139`) and keeps the RLE form only if strictly smaller
  than the flat rows.
* `plane_count = ceil(bits/8)` ∈ 1..4. A "plane" is one byte lane of the interleaved
  pixels, extracted/re-inserted by strided copies (`deinterleave_plane` /
  `interleave_flat`, `bmf.cpp:5013`/5017).
* **Transposition**: the encoder may transpose the whole image (rows↔columns,
  interleaving preserved; `transpose_image`, `bmf.cpp:3154`) when a trial encode of
  the transposed image is cheaper (§5). The decoder transposes back as its last step.

---

## 4. Inter-plane decorrelation (colour transform)

There is no fixed RGB→YCbCr-style transform. Instead the encoder chooses, per image,
a **plane coding order** and per-plane linear predictions from already-coded planes,
described by `PlaneDesc` (`bmf.cpp:261`):

```
struct PlaneDesc { uint8 nrefs;      // # reference planes (0..3); doubles as coding-order slot
                   uint8 src_plane;  // physical plane coded at this order position
                   uint8 flags;      // desc_predictor(0x03: 0/P1/P2) | desc_alt_model(0x04) | desc_has_refs(0x08)
                   uint8 dc;         // additive DC recentring, mod 256
                   int32 weight0,1,2; } // blend weights, fixed-point /128, range −64..191
```

With coding order O0, O1, O2(, O3=alpha), the forward transform
(`code_colour_plane<0>`, `bmf.cpp:5052`) produces, all mod 256:

* **O0**: raw.
* **O1** (`nrefs==1`): `O1 − dc − O0` — plain plane difference.
* **O2** (`nrefs==2`): `O2 − dc − ((w0·O0 + w1·O1 + 40) >> 7)` — a weighted blend of
  the two earlier planes of the *same pixel* (rounding constant 40 ≈ 0.31·128; a
  2-weight blend whose weights sum to 128 with one weight zero — i.e. an exact copy
  of one reference — is collapsed to the single-reference form, `plane_transform`,
  `bmf.cpp:5031`).
* **O3** (`nrefs==3`, the alpha plane): `A − dc − ((w1·ch1 + w0·ch0 + w2·ch2 + 63) >> 7)`
  — a 3-weight mix of the pixel's three colour bytes.

The decoder applies the exact mirror (`code_colour_plane<1>`); coding order guarantees
references are reconstructed first. `dc` recentres the residual histogram so the bulk
of the distribution avoids mod-256 wraps (chosen by `widest_window`, §5).

The alt models replicate this transform internally instead of using the external pass:
alt-P1 codes in the transformed domain via `fold_from`/`unfold_to` with
`plane_mix2/plane_mix3` (`bmf.cpp:4173`/4177 — same formulas, +64 rounding in the
3-ref case); alt-P2 does not subtract at all but seeds its per-pixel `dval` state with
the inter-plane prediction scaled to its internal 16× fixed point (§8.2).

### How the transform is chosen (encoder heuristic, `choose_plane_coding`, `bmf.cpp:5531`)

All heuristic decisions use **order-0 empirical entropy** of residual histograms:
`estimate_cost` (`bmf.cpp:3744`) computes `Σ nᵢ·log2(N/nᵢ)` bits exactly (no log
table; the even/odd accumulator split is only an ILP unrolling).

For ≥3 planes:

1. Three candidates — which physical channel is the 2-reference "chroma" plane — are
   costed by `cost_candidate` (`bmf.cpp:5285`). It works in the **gradient domain**:
   per pixel and per plane, `d = NW + cur − N − W` (a second difference, invariant to
   per-plane DC and to whatever spatial predictor is used later). One pass
   accumulates six 1024-bin histograms (each ref plane flat, ref difference,
   candidate−refA, candidate−refB, candidate−avg) *and* the normal equations for a
   closed-form **2-variable least-squares fit** `dz ≈ (w1·dx + w2·dy)/128` (with +0.1
   determinant regularization; weights clamped to [−64,191]). It also decides which
   reference plane is coded flat and which as a difference, by comparing entropy sums
   of the two orderings. Cost of a candidate = flat-plane costs + min(fitted blend,
   copy-refA, copy-refB, average) entropies.
2. The winning candidate's weights are refined by a greedy 2-axis coordinate descent
   (`WeightSearch`, `bmf.cpp:5392`; step window 4, both directions) on the full
   histogram cost `weight_pair_cost` (`bmf.cpp:5265`).
3. A slack of `min(cost>>7, kSlackMax=0x4000)` (~0.78 %) lets degenerate predictors
   (copy-A / copy-B / 64:64 average) replace the general blend when nearly as good —
   cheaper to model and to store.
4. Per-plane `dc` = position of the heaviest 256-bin window slid over a value-domain
   residual histogram (`widest_window`, `bmf.cpp:5378`), +1: the blended (and alpha)
   plane over a 1024-bin histogram of the chosen predictor's residuals, the O1
   difference plane over a 512-bin histogram of the O1−O0 differences; the flat plane
   O0 gets no dc.
5. For 32-bit images, the alpha plane gets its own **3-variable closed-form LS fit**
   over 2×2-quad second differences (`fit_alpha_weights`, `bmf.cpp:5427`) with the
   same one-hot simplification slack (`choose_alpha_plane`, `bmf.cpp:5456`).

---

## 5. Encoder mode search (`search_filter`, `bmf.cpp:5768`)

Everything past the heuristic layer is decided by **real trial encoding**: run the
actual model + range coder over the image, count `8·(stream.cur−stream.buf)` bits,
reset, compare. (At `-Q9` the "tile" is the entire image; the vestigial centering
math suggests smaller quality settings originally used a central sub-window.) Trials
run with `alphabet_reduced=1`, which suppresses the expensive neighbour/mirror
counter updates in the alt-P2 bank cascade (§10.4; alt-P1 and the slow model are
unaffected) — search costs are therefore measured on a slightly cheaper model
variant than the final encode.

1. **Per-plane search** (`search_planes`, `bmf.cpp:5708`), in coding order. Trial
   flag sets (`try_*`, `bmf.cpp:5647`): `mode0` (raw plane into the slow model),
   `p1 = pred_p1|alt`, `p2 = pred_p2|alt`, and for non-first planes the same three
   with `desc_has_refs`. Pruning: `try_p2` runs only if P1 came within 1/32 of the
   best so far (or another plane already chose P2); `try_mode0` is skipped entirely
   once any plane chose P2. Cheapest flags win per plane.
2. **Transpose trial**: each plane re-encoded on the transposed image, with early
   abort when a plane exceeds its normal cost by >1/256; adopted only when the total
   wins by >1/4096 — then the real image is transposed and `flags_transposed` set.
3. **Mode unification trials**, each a full re-encode via `transform_cost`
   (`bmf.cpp:5693`): for >2 planes, force P1 on all planes and force P2 on all
   planes (optionally re-adding refs via `allow_refs_where_present`); separately,
   for >1 plane when no plane chose P2: drop `desc_alt_model` everywhere (explicit
   MED + slow model) and clear all flags (plain interleaved slow model). Any winner
   selects **interleaved** ("together") coding.
4. Return value: 1 → **planar** mode (each plane its own transform + model + coded
   segment), 0 → **interleaved** mode (`transform_planes`, `bmf.cpp:5149`: apply all
   transforms in place reading from a pristine copy, then one model pass over the
   interleaved bytes — either one slow-model pass, or the joint multi-plane alt-P1 /
   alt-P2 coders).

Images of depth ≤ 4 bpp skip the search entirely and go straight to a single
slow-model pass over the packed data (`code_image_body`, `bmf.cpp:6533`).

---

## 6. The MED predictor and residual folding

`pred_p1` without the alt model is the classic **LOCO-I / JPEG-LS median edge
detector** applied as a separate pass:

* `med_predict(W, N, NW)` (`bmf.cpp:4937`): returns `min(W,N)` if `NW ≥ max(W,N)`,
  `max(W,N)` if `NW ≤ min(W,N)`, else `W+N−NW`.
* Residuals are **zig-zag folded** to unsigned codes 0,−1,+1,−2,+2,… → 0,1,2,3,4,…
  (`med_fold_table`/`med_unfold_table`, `bmf.cpp:4946`/4953).
* `predict_med` (`bmf.cpp:4962`) runs in place, **back to front**, so it reads
  original neighbour values while overwriting; row 0 uses left-DPCM, column 0 uses
  up-DPCM. `unpredict_med` (`bmf.cpp:5094`) is the forward-order mirror over
  reconstructed values.

The folded residual plane is then handed to the slow model as an ordinary symbol
plane. (This path wins on images where MED decorrelates well but the residuals still
have palette-like structure the slow model exploits.)

The alt models use the same zig-zag through a generalized table set
(`alt_init_tables`, `bmf.cpp:4088`): `unfold[code]` → signed residual (odd codes
negative, even positive), `fold[resid]` → code with **near-lossless bucketing** of
width `2q+1` (with `q = near_lossless_q = 0` in this build, `fold` degenerates to the
exact zig-zag), and `fold_hi[resid]` → always-exact code (physically written as
`fold[resid+256]`, deliberately overflowing into the adjacent array member).
`fold_or_refuse` (`bmf.cpp:1085`) is the encoder-side guard: if the quantized
reconstruction would drift from the source by more than ±16 (including mod-256 wrap
pathologies), the exact `fold_hi` code is sent instead. The decoder is oblivious —
it always computes `pred + unfold[code]`.

---

## 7. Shared adaptive-statistics primitives

All models are built from a small set of counter structures. Common idioms: counts
are halved with **ceiling rounding** (`halve_up(x) = x−(x>>1)`, `bmf.cpp:88` — a
nonzero count never dies), and rescale thresholds grow with use, so every context
anneals from fast adaptation toward long memory.

### 7.1 `BitCtr` — binary counter with lazy parent seeding (`bmf.cpp:991`)

`{n[0], n[1], limit}`; p(bit) = `n[bit]/(n0+n1)` fed directly to the bit coder.
Increment +8 to the coded bit; when the total exceeds `limit`, both counts are
ceiling-halved and `limit += 64` (capped 0x4000) — adaptation slows as a context
matures. The distinctive feature is the **two-level lazy hierarchy**
(`code_context_bit`, used by the slow model's candidate flags and run coder):

1. a virgin child codes through its *parent* counter and only records the first
   observed bit;
2. on second use it is seeded from the parent distribution scaled to total 64, +4 on
   the first-seen bit, and the parent *loses* 3 counts of that bit (mass moves from
   the generic to the specific context);
3. mature children code independently, but while their total is < 0x88 the parent
   still receives +1 per bit — keeping it a good prior for other virgin children.

### 7.2 `FreqRec` — 5-level frequency record (`bmf.cpp:592`)

The slow model's rank-stage statistics: `w[0..4]` level frequencies (0 = escape),
`w[5]` total, `w[6]` adaptive rescale floor, `b14` = seeding credit/countdown,
`b15` = per-hit increment ("kick"). Coding is cumulative in level order. Updates:
`w[lvl] += kick`, with rescale (ceiling-halve all levels) only when the total is past
the floor **and** the just-coded level is not dominant (`w[lvl]+kick+8 < total`) or
the hard cap 0x4000 is hit — near-deterministic contexts keep sharp distributions.
After the seeding phase the floor rises to 256 and the kick to 15 (mature = adapt
faster per event but rescale rarely). `blend_from(src)` clones a parent bucket into a
young specific context (parent distribution renormalized to ~21 total plus own marks)
and grants a `b14·8` countdown during which every hit **dual-updates** parent and
child.

### 7.3 `SymList` — sorted symbol list with escape mass and exclusion (`bmf.cpp:660`)

An adaptive multi-symbol distribution: entries `{sym, cnt(u8)}` kept sorted by
descending count; `tot` is the **escape weight** (escape p = `tot/(tot+Σcnt)`).
Coding walks the list with **exclusion** — symbols marked in the shared
generation-stamped `exclusion_mask` contribute zero probability. On a hit: `cnt += 4`
and bubble up. On escape: the top interval is coded and *all listed symbols become
excluded* (so the next list in a fallback chain never pays for them); if everything
was already excluded, the escape is free (no bits). Rescale (count > 251 or
accumulated increments past `rescale_at`): halve counts, drop zero-count tail entries
— each dropped symbol adds +1 to `tot` (its mass is recycled into the escape).
`add_weight` trains a list without coding: new symbols enter with cnt 2, evicting the
weakest entry (whose count also flows into `tot`). Dense initialization (all symbols
cnt 1, `tot=0`) yields escape-free terminal lists; sparse initialization (`live=0,
tot=2`) yields learned lists.

### 7.4 `CounterNode` — 7-slot counter with tail classes (`bmf.cpp:963`)

Alt-P1's residual-code distribution: slots 0–4 = codes 0..4 (residuals 0, −1, +1,
−2, +2), slot 5 = all remaining odd codes (negative tail), slot 6 = even tail. Init
`{8,2,2,2,2,3,3}`. The slot is coded from cumulative counts; increment +32; rescale
at total > 0x2000 divides counts by 3 (rounding up) while the smallest slot count is
still ≥2, and ceiling-halves once any slot has decayed to 1 — since all slots start
≥2, the first rescale always divides by 3, and rarely-hit slots then decay to 1 over
successive rescales — and sets a "has rescaled" flag in bit 15 of the total (which
permanently stops the alt-P1 model's neighbour-training for that context, §9.3).
Tail codes are completed by the shared
symbol-tree strips, on a strip context that includes the residual sign and a
tail-dominance flag `2·c[slot] > c[0] + total + 96`.

### 7.5 Symbol-tree strips — log-bucketed magnitude coder (`bmf.cpp:3572`)

A shared magnitude coder used by both alt models (and by `CounterNode`'s tail). Each
context owns a 254-word strip in `model_table_store` (1024 strips): `freq[0]` total,
`freq[1]` an **adaptive per-strip increment**, `freq[2..9]` eight magnitude-level
frequencies, then 122 `FreqPair{f[2]}` binary nodes forming per-level bit trees.
A value is split Elias-gamma-style: level 0 = 0, level 1 = 1, level 2 = 2–3, level
*L*≥3 covers 2^(L−1) values; the level is coded from the 8 frequencies, the offset by
walking the level's binary tree MSB-first (each node a plain dual-count bit model,
increment `alt_freq_init`, halved above 0x4000). Level seeds: `{205,124,147,83,48,
16,8,4}`; increment seeded at `24·alt_freq_limit` and **annealed** at each rescale
(−16 while large, then −4 down to `alt_freq_limit`) — per-context adaptation that
starts very fast and settles. Model parameters differ by predictor: P1 uses
`alt_freq_init=64, alt_freq_limit=16`; P2 uses 8/8 (`begin_plane_stream`,
`bmf.cpp:3452`). `update_binary_pair` (`bmf.cpp:3709`) is a training-only twin used
for context-space generalization (reduced increments, no coding).

---

## 8. The "slow" model — `ModelBlock` (`bmf.cpp:2054`)

The universal plane coder: a context-mixing **symbol** model with no notion of
numeric pixel value — everything is equality structure. It is BMF's palette-image
engine, and also the fallback for any plane/interleaved data the alt models don't
win on. One `ModelBlock` (~13 MB of tables, pooled) codes one plane
(`code_plane_slow`, `bmf.cpp:2690` area).

### 8.1 Alphabet reduction

The model operates on a dense alphabet 0..A−1 of the values that actually occur
(`reduce_alphabet` encoder side, `bmf.cpp:3991`; `expand_alphabet` decoder side):

* **depth ≤ 8** (`reduce_narrow_alphabet`, `bmf.cpp:3862`): presence bitmap over 256
  values; the distinct count is coded flat (`code_alphabet_size`, `bmf.cpp:3965`:
  interval `[n−1, n)` of total 2^depth); the value set is transmitted as **gaps
  between consecutive present values in ascending order**, coded with one adaptive
  dense `SymList` of `2^depth − A + 2` symbols. Ids are value-ordered.
* **depth > 8**: distinct pixel words are collected into a fixed 8192-node BST
  (`tree_place`, `bmf.cpp:3939`); ids are assigned in **first-occurrence order**.
  The values are transmitted in id order, byte by byte, each byte coded by one of
  `4·nbytes` dense 256-ary `SymList`s selected by a carry context from the previous
  byte — 2 bits (top bits of the previous byte) within a value, 1 bit across value
  boundaries (`code_symbol_bytes`, `bmf.cpp:3971`).
* **Overflow** (> 8192 distinct words): the plane is de-interleaved into byte planes,
  `height *= nbytes`, and the whole model recurses on the stacked byte image
  (re-interleaved on output by `interleave_depth_bytes`).

### 8.2 Per-pixel state

`PixRec` (`bmf.cpp:1995`, 8 bytes) per pixel: `sym` (dense symbol) + six **match
flags**: equal-to-N, W, NE, NW, NEE, NEEE. Five rows are kept in a ring
(`row_store[5][kMaxWidth+16]`), with margins pre-set to "sym 0, all matches true" so
borders read as flat. Four `grad[]` accumulators maintain sliding windows of match
flags (vertical matches in the two rows above, horizontal matches in the current row)
whose ==0 tests mean "window fully matched" — cheap local flatness features.

### 8.3 The coding cascade

Per pixel, up to four stages; `hit` records which stage fired and steers the updates.

**Stage 0 — run mode** (entered only when the neighbourhood is verified flat: W
equals its own W and NW, and rows −1/−2 are constant across a 4–5 column window):

* `run_scan` measures how far the row above continues as a constant, vertically
  repeated run (`run_len`, capped at the row end).
* One adaptive bit codes "the current row copies that entire run" — context:
  log2-bucket of `run_len` (`run_bucket`), whether the flat patch extends 3 rows
  deep, whether the up-row run continues ≥2 past its end, and `alpha_map[N]` — a
  per-symbol memory of whether the **last** run headed by this symbol completed
  (257 lazily-seeded `BitCtr`s).
* On a miss with `run_len>1`, the actual copied prefix length is coded as a
  **truncated binary number, MSB first** (`code_run_length`, `bmf.cpp:2741`): bit
  positions that cannot fit under the cap are skipped; each bit uses one of 48
  `BitCtr`s indexed by (bit position, phase ∈ {top bit, still-leading, after first
  1}).
* Consumed pixels are stamped in bulk (all-match flags, `fill_run`), cursors jump,
  and the gradient windows are re-seeded. A full hit charges the whole run a single
  model update; a miss falls through **directly to stage 2** for the breaking pixel
  (the run flag already implied "not N").

**Stage 1 — neighbour-rank coding**: the 5-ary event
`rank ∈ {0:none, 1:=N, 2:=W, 3:=NE, 4:=NW}` (duplicates take the smaller rank) is
coded from a pair of `FreqRec`s:

* The **bucket** record: one of 188 seeded prototypes indexed by (context group,
  rank-of-last, rank-of-prev). The *context group* is one of the **15 set
  partitions of {N,W,NE,NW}** — the 6 pairwise equality bits among the four
  neighbours are looked up in a perfect table (`ctx_group_flags`, `bmf.cpp:184`)
  since transitivity permits exactly B(4)=15 patterns. Each group folds the
  unreachable ranks' prior weight into the surviving ranks (`GroupFolds`), and the
  (last,prev) ranks come from a per-(group, pair-key) memory of the last two symbols
  coded in that context. Seeding gives fewer-level groups a coarser increment
  (`1<<(5−levels)`).
* The **learned** record: `grid[188 + id]` where `id` is an **exactly interned**
  (collision-free, sequentially minted — `intern_ctx`, `bmf.cpp:2320` area) context
  id built in up to three tiers: (bucket id, far-match flags of W/WW, four
  window-flatness bits, two 4-deep column-match bits) → id1; (id1, three more match
  bits) → id2; and for alphabets < 32, (id2, the actual left symbol) → id3, with a
  minting cap (`kCtxId3Limit=53248`) redirecting the overflow to a shared slot.
  Encoder and decoder mint identical ids because they see identical context
  sequences.
* A virgin learned record only accumulates marks while coding goes through the
  bucket; on its second visit it is initialized from the bucket (`blend_from`) and
  dual-updates the bucket for a while (§7.2). This is a two-level context hierarchy
  with explicit statistics inheritance — SSE-era context cloning rather than mixing.

Rank ≠ 0 ends the pixel. Rank 0 is the escape into stage 2, with all four neighbour
symbols added to the exclusion set.

**Stage 2 — candidate offering** (`offer_candidates`): a ranked array of up to 32
concrete candidate symbols is assembled: the context pair's last two symbols; an
8-entry **move-to-front cache** keyed by a 16-bit pair key (`bit-reverse(N)·8 −
first-differing-neighbour` — the bit reversal spreads N across the key space); then
22 spatial neighbours in roughly increasing distance (WW, NNE, NN, … out to 4 rows
up / 7 columns left). For each surviving candidate one adaptive bit "pixel == this
candidate?" is coded. The binary context (`pixel_context`, `bmf.cpp:2115`) combines
spatial support (is the candidate WW / in the near band / in the far band) with
**co-occurrence support** from the stage-3 lists (is the candidate in the top-k of
`sel0[N]`, `sel0[W]`, `sel0[NE]`; is W in the top-10 of `sel0[candidate]`), giving
4096 lazily-seeded `BitCtr` leaves over 16 parents. Candidates without support are
skipped outright (deeper positions need progressively stronger support). Every
rejection feeds the exclusion set, so duplicates and later stages never pay for
rejected symbols. The MTF cache is only updated when the pixel was *not* predicted
by N/W — it deliberately learns the locally-recurring symbols that are not the
obvious neighbours.

**Stage 3 — selector lists**: three chained `SymList`s with escape:
`sel0[W]` (≤33 entries; "symbols that follow W") → `sel1[N]` (≤99 entries; "symbols
that appear under N") → a dense, escape-free full-alphabet list that guarantees
decodability. Escaping a list excludes all its symbols from the next. After a
literal, the lists are cross-trained (`init_tables`): the coded symbol is inserted
into every list that was escaped past, and the sel0/sel1 lists of N, W and of the
symbol itself receive graded `add_weight` updates — the co-occurrence statistics that
stage 2 reads as context.

### 8.4 Row I/O

The encoder pre-loads each row's true symbols into the ring and "predicts" against
them; the decoder writes each finished row out through `sym_code[]` (dense id →
value), packing sub-byte depths MSB-first (`write_row`), and re-interleaves stacked
byte planes at the end if the alphabet had overflowed.

---

## 9. The alt-P1 model — `AltP1Block` (`bmf.cpp:1127`)

A residual coder for continuous-tone 8-bit planes: MED prediction plus an extremely
large **direct product context** over quantized local features, with heavy
update-time context generalization instead of mixing.

### 9.1 Prediction and residual

The predictor is MED (same clamped-gradient rule as §6, computed inline in `ctx_of`).
The residual is zig-zag folded (with the near-lossless `fold`/`fold_hi` tables and
the ±16 drift guard of §6; at q=0 everything is exact). State per pixel is
`P1Ctx{sym, mag}` — reconstructed value and |error| magnitude — in a 5-row ring with
mirrored margins; rows are seeded with `sym=72, mag=0`.

### 9.2 Context

Two context indices are formed per pixel (`ctx_of`, `bmf.cpp:1205` area):

* **Activity**: a weighted sum of neighbour error magnitudes
  (`act = NW + W3 + 3(NE+NN) + 6W + 4(N+WW) + 2(NNE₂+NE₂+W4)` plus variant-specific
  extra taps), quantized twice: by `p1_level_edges` into 8 **levels** (fine, for the
  counter context) and by `p1_group_edges` into 8 **groups** (coarse, for the tail
  strips). An activity-dependent deadzone `p1_level_step = {1,1,2,2,2,4,4,4}` widens
  the "≈0" band of two gradient selectors in noisy areas.
* **Counter context** `ctx[0]`: a 5-bit base — activity level (3 bits), "current
  parity lane perfectly predicted recently" (1 bit; two interleaved sliding
  error-sum lanes, one per column parity), "column quiet" (1 bit) — times **nine
  ternary selectors** combined positionally as Σ selₖ·32·3ᵏ. The selectors are signs
  (with deadzones) of local gradients and curvatures: sign(NW−N), sign(NW−W),
  sign(NE−pred), horizontal/vertical curvature (2W−WW−pred, 2N−NN−pred), a
  third-order horizontal extrapolation error, plus a brightness tri-bucket of the
  prediction. Total 32·3⁹ = **629 856 `CounterNode`s** per plane — direct-mapped, no
  hashing.
* **Strip context** `ctx[1]` = plane·256 + brightness-slot(pred)·8 + activity-group:
  indexes the 1024 shared magnitude strips for tail coding (§7.5), with sign and
  tail-dominance flags injected at bits 6–7.

In the **multi-plane** driver (`alt_model_p1`, `bmf.cpp:4181` — planes coded
interleaved per pixel, in transformed domain per §4), up to five of the nine
selectors are replaced by **cross-plane features**: the sign of the reference
plane's own just-produced residual at the same pixel (`nb_resid`), and
slope-agreement tests `cross_grad` = (own W−pred) + (ref horizontal gradient); the
activity sum also absorbs co-located reference error magnitudes, and the "quiet" bit
becomes "reference error small".

### 9.3 Coding and adaptation

The folded code is coded by the context's `CounterNode` (slots 0–4 direct, tails via
the strips; §7.4/7.5). Because 630K contexts would starve, each coded sample also
performs **context-space diffusion** (`update_model`, active until the context's
first rescale, with a second budget cut at total ≥ `kCounterNudgeLimit=0xCCC`):

* the **mirror context** — all sign selectors reflected 0↔2 (`mixer_rev`) — is
  trained with the **negated** residual's slot (weight 17): a patch with all
  gradients reversed should see the opposite error sign;
* the same context at **adjacent activity levels** (±1) gets the forward slot
  (weights 11/13), and on tail escapes the strips at the adjacent activity group and
  adjacent brightness slots are trained via `update_binary_pair`;
* for each of the nine ternary dimensions, the contexts with that one selector
  changed (extreme↔neutral↔opposite-extreme) receive graded updates (weights 3–7),
  the mirrored one-off contexts receiving the negated slot.

This substitutes explicit neighbour training for probability mixing: one observation
updates ~30 nearby contexts with weights decaying in Hamming distance, giving the
huge table SSE-like sample efficiency while keeping per-symbol coding a single table
lookup.

---

## 10. The alt-P2 model — `AltP2Block` (`bmf.cpp:1635`)

The strongest continuous-tone model: an adaptive **normalized-LMS linear predictor**
with context-selected weight sets, followed by a five-stage integer bias-correction
cascade (chained SSE), a context-quantized ternary-plus-tree residual coder, and
pervasive update-time generalization. All values are carried in **16× fixed point**
("sample16" = 16·pixel).

### 10.1 Feature vector

`fill_row_inputs` (`bmf.cpp:4435`) gathers **28 float features** (7 rows × 4 lanes)
from the causal neighbourhood: rows 0–3 are gradient-adjusted combinations
(`grad(a,b,c) = a+b−c`, GAP/CALIC-style) of `dval` — the **inter-plane-decorrelated**
values — including vertical/horizontal extrapolations and long-range taps; rows 4–6
are plane-specific mixes of raw `val` neighbours with reference-plane rows,
including cross-plane gradient corrections and taps that carry the reference plane's
own prediction *error* (`v0(−2)+ref->err`). Separate feature sets exist for plane 0,
plane 1, planes ≥2, and the no-reference (greyscale) path.

### 10.2 Per-pixel state and cross-plane wiring

`P2Ctx` (`bmf.cpp:1593`) per pixel: `val` (16×), `dval` (value minus the inter-plane
prediction), signed `err`/absolute `aerr` against the NLMS prediction, four
directional absolute gradients, a ternary residual `sign` with an
activity-dependent deadzone, and the residual magnitude `mag`. Five-row ring with
mirrored margins; row 0/1 have dedicated neutral-prior seeders.

In the multi-plane driver (`alt_model_p2`, `bmf.cpp:4865`) planes are coded
**interleaved per pixel** (0→1→2→3), so a later plane sees the earlier planes'
*current* pixel. Before a plane's sample is coded, its `dval` is seeded with the
inter-plane prediction (16·ref for the 1-ref plane; `(w1·p1+w0·p0)>>3` — the /128
weights rescaled to 16× — for the 2-ref plane; a 3-term mix for alpha). The model
then stores `dval = val − seed`, i.e. the inter-plane residual, which is what feature
rows 0–3 read. Additionally a shared, per-pixel-decayed accumulator `ctx_bias[4]`
couples the four directional activity sums **across planes** — a busy pixel in one
channel raises the others' activity estimates.

### 10.3 The NLMS predictor (`NbRow`, `bmf.cpp:1606`; `predict`, `bmf.cpp:4275`)

* A fixed global 28-tap dot product (`p2_coef`, seeded from `bmf_p2_coef_init`)
  produces `centre`; all features are then **centred** by it.
* Weight sets live in a pool of 1088 `NbRow`s per plane, allocated lazily by a
  **1920-slot activity context**: 6 bands of horizontal/vertical activity ratio ×
  4-level quantizations of overall activity, local brightness, diagonal ratio and
  area brightness (`bmf_p2_thresholds[6][13]`, per-band threshold rows with
  sentinel saturations).
* Prediction mixes **weight vectors, not predictions**: the `NbRow`s used at the six
  neighbouring pixel positions (W, N, NE, WW, NW, and the same column two rows up —
  remembered in two per-column pointer rows swapped each row) are blended by one of
  four fixed 6-weight profiles chosen by the **smoothest direction** (argmin of
  scaled directional activity sums); `prediction = dot(mixed_weights, features) +
  centre`. If the current context's own row is mature, the output is the
  covariance-optimal blend `prediction + (own−prediction)·cov/var` with per-row
  cov/var trackers.
* Adaptation is **normalized LMS**: per-tap mean-square trackers normalize the step
  (`w += rate·err·x/(ms+floor)`), with a per-tap rate table (`bmf_p2_rate_init`,
  0.0108→0.0009 — earlier features adapt faster), floor constants that are squares
  of per-stage noise sigmas (88², 164², 71², 23²) scaled by a slowly-annealing
  per-row regularizer, and error boosts (×2.1 / ×2.6) on the training targets. Each
  pixel trains: the current row (fast, on the mixed-prediction error), the left
  pixel's row (slow, on the own-prediction error scaled by a confidence derived from
  cov/var), plus a catch-up step when a row is re-seated after serving other slots;
  fresh rows are seeded from their neighbours' mixed weights (×0.78) and
  mean-squares (×0.19). SSE denormals are flushed to zero globally
  (`bmf_set_denormal_mode`, `bmf.cpp:295`) so the float path stays fast.

### 10.4 The five-bank bias cascade

The float prediction `filt` is refined by **five chained context-modeled integer
corrections** (`step_bank`; run0 = filt + bias₀(ctx₀), run1 = run0 + bias₁(ctx₁), …
run4 = final prediction):

* Each bank has 2¹⁵ contexts of `P2Count{int8 rate; uint8 b1; int16 weighted}`
  (5·32768 total). The predictor is an integrator read-out:
  `bias = (weighted + 2^(rate−1)) >> rate`, i.e. LMS with step 2⁻ʳᵃᵗᵉ.
* A bank's 15-bit context = 4 bits of coarse quantization (`ctx_quant`: 2 bits of
  running-prediction/brightness thresholds + 2 bits of activity thresholds, per-bank
  magic constants) + **11 sign bits** of differential tests: does the running
  prediction over/undershoot specific neighbour extrapolations, Laplacians,
  inter-plane consistency terms, pooled neighbour-error signs. Each bank is an
  SSE/APM stage keyed on the *pattern* of over/undershoot.
* Updates (`code_banks`, `bmf.cpp:4688`): the exact context integrates the full
  residual, with a small deadzone kick; the learning rate **anneals** 2⁻⁵→2⁻⁸ via a
  countdown (`b1`, reloads `{7,46,197}`) decremented by small residuals
  (`|res|<38`; large residuals leave it untouched), doubling `weighted` on each
  rate step to keep the read-out invariant; a fully-annealed context freezes its
  secondary/generalization updates entirely (the primary integrator keeps running).
* **Context-bit diffusion**: every pixel also trains, per bank, the ±1 quantization
  neighbours, all 11 Hamming-distance-1 contexts (one sign bit flipped), their
  brightness-rotated variants (`p2_ctx_rotate`), and — with **negated** residual —
  the full sign-mirror context (`ctx XOR 0x7FF0`, all 11 sign bits flipped) and the
  mirrors of the one-off contexts, all at reduced gains (shifts 1–3). This is the
  same philosophy as alt-P1's diffusion: neighbour training in context space instead
  of mixing. (These generalization updates are suspended while `alphabet_reduced`
  is set, i.e. during the encoder's trial-encoding search.)

### 10.5 Residual coding

`pred = clamp((run4+7)>>4, 0, 255)`; the zig-zag/near-lossless folded code is coded
in two stages:

* **Ternary split** (`P2Freq{step; f[3]}`, `bmf.cpp:1460`): zero / negative (odd
  codes) / positive (even codes), from `f[]` with adaptive increment `step` that
  starts at 4096 and anneals toward 16 through the rescale schedule (halve counts at
  a 16384 slot cap; step >>=1 / −32 / −2 by range). `step` doubles as a **maturity
  meter** gating all generalization (below).
* **Magnitude** `(code−1)>>1` via the shared symbol-tree strips (§7.5), on a strip
  context = plane | 15-level brightness bucket of the prediction (`p2_ctx_edges`,
  dense near black/white) | activity class; the odd/even halves use two strip
  contexts offset by ~4 pixel levels.

The ternary context (`seat_symbol_context`, `bmf.cpp:4535`) is a 15 552-point
lattice: 16 activity classes (a ~40-tap weighted sum of neighbour residual
magnitudes quantized by `p2_len_edges`) × 2 flatness bits × **five ternary
selectors** composed positionally in mixed radix (64·3⁵): prediction-brightness
class, prediction−N and prediction−W band tests, and the N/W ternary residual signs
(replaced, for chroma planes with refs, by reference-plane gradient band tests).

After coding, the frequency lattice is **smeared** (`alt_p2_model`,
`bmf.cpp:4761`): the ±1 activity-class neighbours get fractional-step updates
(10/16, 13/16), the sign-mirrored lattice point (all five selectors reflected,
`mixer_rev`) gets the **negated** residual's ternary slot, the ±1 brightness strips
are trained via `update_binary_pair`, and each of the five selector digits is
flipped through its alternatives with graded gains — all gated by the context's
`step` maturity (fully-annealed contexts stop smearing entirely, and chroma planes
stop the digit-flip spread earlier).

Row 0 is special-cased: left-DPCM prediction with a small dedicated context (left
value ordering/signs + 3-level brightness), then normal rows use the full machinery.

### 10.6 Encoder/decoder symmetry

`encode_sample`/`decode_sample` share `alt_p2_context` and the entire
`alt_p2_model` update pass; the encoder writes the (possibly quantized)
reconstruction back into the pixel buffer before later planes/pixels read it, so
both sides model identical data. The only encoder-exclusive decision is the
near-lossless accept/refuse choice (§6), communicated implicitly through the code.

---

## 11. Near-lossless mode (vestigial)

The stream carries a 4-bit quantizer field E for grey and true-colour images, and the
entire fold/`fold_hi`/deadzone machinery is parameterized by `near_lossless_q`
(bucket width 2q+1, alt-P2 deadzone 4q+1, context bands ±(16q+7/8)). In this build
the encoder always writes E=0 and the decoder rejects E>0, so behaviour is strictly
lossless; the ±16 drift guard still exists but never fires at q=0.

---

## 12. Review notes — oddities and reconstruction artifacts

Observations from reviewing the source; none affect correctness of this build
(the codec round-trips bit-exactly), but they are worth knowing when reading or
modifying the code:

* `opt_*` option constants (`bmf.cpp:283`–289) are defined but never referenced —
  the `-S -Q9` configuration is constant-folded into the code.
* `predict_med` histograms every folded residual into `hist_scratch`
  (tail of the output buffer), but no reader of that histogram survives in this
  build — likely a vestige of the original's cost estimation.
* `write_bmp_palette` has a dead `memset` after a `return` in its no-palette branch
  (`bmf.cpp:6101` area): palette bytes for palette-less ≤8-bit images are written
  uninitialised (harmless for the depths this build emits, which always have
  `depth_grey` or `depth_palette` set on ≤8-bit output).
* `alt_init_tables` deliberately writes `fold[i+256]` to fill the adjacent
  `fold_hi[256]` member, and reads `fold[-1+…]` via a `neg` pointer — layout-
  dependent by design.
* The alt-P2 lazy filter pool: `nb_id[1920]` slots map into `nb_weights[1088]`
  rows with no bound check on the allocation counter; the context geometry
  (saturating band-5 rows) appears to keep the live slot count under 1088, but the
  invariant is not enforced in code.
* `P2Coef::fold` (`bmf.cpp:1613`) brackets each alt-P2 image: it element-wise adds
  the *static seed* coefficients of feature rows 4–6 (`bmf_p2_coef_init` rows 4–6,
  installed at `BMFState::reset`) into rows 0–2 — i.e. into the static weights of
  *different* features — zeroes rows 4–6 and resets their NLMS rates to 0.0024 for
  the duration of the image, restoring everything afterwards. Whether the
  cross-feature addition is intentional is unclear.
* In `search_filter`, `ps.n_p2 = bits_a;` overwrites the P2 plane *count* with a
  bit count after the all-P2 unification wins — type-wise a quirk, but it is
  load-bearing: the (nonzero) value makes the later `!ps.n_p2` gate skip the
  drop-alt-model / clear-flags trials.
* `ModelBlock::layout_workspace` zeroes `0x100000` bytes = 65 536 `FreqRec`s from
  index 188, one record past `kFreqGridCount = 65723` − 188 = 65 535 — absorbed by
  an explicit padding member.
* `tree_place` stores its last comparison direction into `mode_symbol[1]`; no
  consumer exists during alphabet reduction — likely a decompilation artifact of a
  shared register.
* `unmodel_planes_together` dispatches the joint decode (alt vs slow model) on
  coding-order-slot-0's descriptor flags alone; the per-plane inverse transforms
  afterwards do honor each plane's own predictor. The encoder's unification trials
  set predictor/alt flags uniformly before choosing the "together" mode, so the
  slot-0 dispatch is always representative.

---

## 13. Key constants

| constant | value | role |
|---|---|---|
| `kWeightMin/kWeightMax` | −64 / 191 | inter-plane blend weight range (fixed-point /128) |
| `kSlackMax` | 0x4000 | cap on the heuristic's predictor-simplification slack |
| `no_symbol` | 8192 | alphabet cap + "impossible symbol" sentinel of the slow model |
| `kFreqTableOffset` | 188 | number of seeded rank buckets = start of learned `FreqRec`s |
| `kCtxId3Limit` | 53248 | minting cap of the slow model's tier-3 context interning |
| `kCounterNudgeLimit` | 0xCCC | alt-P1 counter total above which cheap diffusion updates stop |
| `kBankMirrorMask` | 0x7FF0 | alt-P2 bank-context sign-mirror (flips the 11 sign bits) |
| `kP2FreqRescaleTotal` | 29696 | alt-P2 ternary-cell total cap for smeared updates |
| `kModelTableBytes` | 0x7F000 | shared symbol-tree strip storage (1024 strips × 254 words) |
| `p1_level_edges` | {1,2,4,8,14,35,103} | alt-P1 activity quantizer (in act/16 units) |
| `p2_ctx_edges` | {17,20,27,…,236,237} | alt-P2 prediction-brightness quantizer |
| `p2_len_edges` | {4,6,…,87,120} | alt-P2 residual-magnitude-sum quantizer |
| `p2_b1_reload` | {7,46,197} | alt-P2 bias-bank rate-annealing countdowns |
| `bmf_p2_coef_init` / `bmf_p2_rate_init` | 7×4 floats | NLMS seed coefficients / per-tap learning rates |
