# BMF v2.01 — Compression Algorithms

A detailed description of the compression algorithms in `bmf.cpp` — a reconstructed
source of Dmitry Shkarin's **BMF** lossless image codec, v2.01 (1998–1999, 2009).
The identifiers in the source were produced during reverse engineering, so names are
descriptive rather than original.  The source is `bmf.cpp` plus the `.inc` files it
includes; references below are `file:line` as of this commit.

Every tunable number the algorithms below use — thresholds, counter bumps,
adaptation rates, NLMS seeds, context-index layouts, quantiser ladders — is
declared in `IDX/*.idx` rather than written into the sources, and reaches them
through the generated headers in `MOD/`. Build with `./mk.sh release` to ship
or `./mk.sh` to tune; see `IDX-NOTES.md` for what moved where and
`IDX-FORMAT.md` for the declaration format.

Range-coder internals (`struct RangeCoder`, `rangecoder.inc:7` — interval arithmetic,
renormalization, carry handling) are **out of scope** here; the coder is treated as an
ideal entropy-coding backend that consumes `(cumFreq, freq, totFreq)` triples and
adaptive bit probabilities. Everything that *produces* those statistics is in scope.
`RangeCoder` owns the `CodedStream` it writes into and is the base class of
`BMFState` (`bmf_state.inc:19`), so every model that holds a `BMFState*` calls
`encode`/`decode`/`encode_bit`/`decode_bit` on it directly.

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
| **slow / ModelBlock** (`model.inc:77`) | default (no `desc_alt_model`) | paletted / indexed / arbitrary symbol planes; also any interleaved multi-plane data |
| **alt-P1** (`AltP1Block`, `alt_p1.inc:4`) | `pred_p1 \| desc_alt_model` | continuous-tone planes; MED prediction + huge direct product-context residual model |
| **alt-P2** (`AltP2Block`, `alt_p2.inc:253`) | `pred_p2 \| desc_alt_model` | continuous-tone planes; adaptive linear (NLMS) prediction + SSE-style correction cascade |

There is additionally a standalone **MED pre-filter** path: `pred_p1` *without*
`desc_alt_model` runs LOCO-I/MED prediction as an explicit in-place transform
(`predict_med`, `codec.inc:874`) and feeds the folded residuals to the slow model.

Both encoder and decoder are generated from the same templates (`f_DEC` template
parameter throughout), and every model statistic is updated identically on both sides
from reconstructed data only — the classic CM symmetry that makes side information
unnecessary.

The build corresponds to `bmf -S -Q9` (slow mode, max search quality): the `opt_*`
constants (`records.inc:92`) are baked in and, notably, are never read anywhere else
in the file — the reconstruction has the option handling constant-folded away.

### 1.1 Recurring design principles

A few ideas recur across all three models and explain most of the design:

* **Progressive fallback.** Easy pixels exit through cheap paths; hard ones fall
  through to progressively more general (and more expensive) representations. The
  slow model is the clearest case: run copy → neighbour-rank hit → candidate flag
  hit → learned symbol list → dense escape list, with exclusion making each failed
  stage free for the next.
* **Predict, then fold.** Wherever the data is numeric, a predictor concentrates
  the entropy near zero and a zig-zag fold turns the signed residual into a
  small-code-biased unsigned alphabet.
* **Statistics sharing instead of probability mixing.** There is no PAQ-style
  mixer anywhere. Instead, one observation trains many *related* contexts at
  update time: parent/child inheritance (`BitCtr` seeding, `FreqRec::blend_from`),
  sign-mirrored contexts trained with the negated residual, Hamming-distance-1 and
  quantization-neighbour contexts trained at reduced gain (alt-P1's
  `update_model`, alt-P2's `code_banks` and lattice smearing).
* **Annealed adaptation.** Nearly every counter starts fast and slows as it
  matures — growing rescale limits (`BitCtr`), increment schedules
  (`FreqRec`, tree strips, `P2Freq.step`), rate countdowns (`P2Count`) — and the
  same maturity measures gate when the generalization updates above are switched
  off.
* **Measured rate over heuristics.** Closed-form fits and entropy histograms only
  seed the search; every mode decision that matters is made by running the real
  coder and counting bits.

---

## 2. Container and stream layout

(Summary only — this is framing, not compression, but the interleaving of raw bits and
range-coded segments matters for understanding the models' entry points.)

* A BMF file is a sequence of members: 4-byte tag (`bmf_tag`, `bmp.inc:194`; image
  signature `0x8A81`, auxiliary `0x9081`, version chars `'2','0'`), then a 16-byte
  `BmfImage` header (width/height/stride u16, 4 unused pad bytes, depth u8, flags u8,
  data_size u32),
  then an optional opaque `CodedTail` block, then the payload, then (for paletted
  images) the raw palette bytes. Auxiliary members are skipped by the reader
  (`expand_image`, `codec.inc:702`).
* `depth` packs bit-depth (mask 0x3F) with `depth_grey=0x40` and `depth_palette=0x80`
  (`codec.inc:702`). `flags` (`bmp.inc:11`) include `flags_transposed`, `flags_slow`,
  `flags_planar` (planes coded separately), `flags_descriptors`, `flags_coded`,
  `flags_tail`.
* If the coded body is not smaller than the raw pixels, the member is stored **raw**
  (`compress_image`, `codec.inc:525`) — the format never expands beyond raw + header.
  Images with `data_size < 16` are always stored raw. A raw member writes the
  *image's own* header, not the coder's — so its flags byte never carried
  `flags_coded`/`flags_slow`/`flags_descriptors` at all (verified: a random
  24-bit image produces flags `0x00` and a file of exactly 20 + w·h·3 bytes).
  Since BMF stores rows unpadded with a 20-byte header and 3-byte palette
  entries, a raw member is always *smaller* than its source BMP (54-byte header,
  4-byte palette entries, rows padded to 4).
* Inside a coded payload, a **bit packer** (`Packer`, `records.inc:11`; `pack_bits` /
  `unpack_bits`, `codec.inc:821` — LSB-first accumulation into 32-bit words) and
  the range coder share one buffer — `RangeCoder::stream`. Raw-bit fields (the 4-bit near-lossless quantizer,
  the plane descriptors) are packed first; each plane's range-coded segment is then
  bracketed by `rc_begin`/`rc_end` (`bmf_state.inc:282`). The range coder's flush
  writes a 3-byte length, zero-pads so the next byte falls at offset 3 mod 4, then
  writes a `0x97` sentinel as the final byte of that word — leaving the stream 4-byte
  aligned so the packer can resume word-aligned; `packer_rewind` (`records.inc:31`)
  reclaims the packer's partially-used trailing word before the coder starts. Planar
  mode thus produces `plane_count` concatenated range-coded segments after the
  descriptor bits; interleaved ("together") mode produces one.
* Per-plane descriptors are coded as raw bit fields (`code_plane_descs`,
  `codec.inc:501`): 6 bits `(flags<<2)|nrefs` per plane, plus 8-bit `dc` when
  `desc_has_refs`, plus 8-bit weights (bias +64) when `nrefs≥2` (`weight0`,
  `weight1`) and `nrefs>2` (`weight2`).
* Images are never tiled: one member covers the whole image.

---

## 3. Input handling and internal representation

* `read_bmp` (`bmp.inc:495`) accepts BITMAPINFOHEADER-only bottom-up BMPs with bpp ∈
  {1,4,8,24,32}, RGB or RLE4/RLE8 compression (full RLE decoders at `bmp.inc:384`/
  `bmp.inc:416`, including delta and absolute ops with nibble-alignment handling). Rows are
  stored **top-down**, tightly strided, packed-pixel interleaved (B,G,R[,A] byte
  order); sub-byte depths stay bit-packed. Palettes are stored as 3-byte B,G,R
  triplets immediately after the pixel data (`BmfImage::palette()`, `bmp.inc:42`).
* **Grey-ramp detection** (`bmf_compress`, `bmf.cpp:67`): a palettized image whose
  palette exactly equals the canonical grey ramp (entry *i* = *i*·(256>>bits) in all
  three channels) is converted to `depth_grey` and its palette dropped — the decoder's
  `write_bmp_palette` (`bmp.inc:559`) regenerates the identical ramp. Otherwise the
  palette is stored raw (never entropy-coded).
* On output, `write_bmp` (`bmp.inc:694`) re-encodes 4/8-bit images to RLE4/RLE8
  (`bmp_rle_encode`, `bmp.inc:597`) and keeps the RLE form only if strictly smaller
  than the flat rows.
* `plane_count = ceil(bits/8)` ∈ 1..4. A "plane" is one byte lane of the interleaved
  pixels, extracted/re-inserted by strided copies (`deinterleave_plane` /
  `interleave_flat`, `codec.inc:667`).
* **Transposition**: the encoder may transpose the whole image (rows↔columns,
  interleaving preserved; `transpose_image`, `bmp.inc:77`) when a trial encode of
  the transposed image is cheaper (§5). The decoder transposes back as its last step.

---

## 4. Inter-plane decorrelation (colour transform)

There is no fixed RGB→YCbCr-style transform. Instead the encoder chooses, per image,
a **plane coding order** and per-plane linear predictions from already-coded planes,
described by `PlaneDesc` (`records.inc:70`):

```
struct PlaneDesc { uint8 nrefs;      // # reference planes (0..3); doubles as coding-order slot
                   uint8 src_plane;  // physical plane coded at this order position
                   uint8 flags;      // desc_predictor(0x03: 0/P1/P2) | desc_alt_model(0x04) | desc_has_refs(0x08)
                   uint8 dc;         // additive DC recentring, mod 256
                   int32 weight0,1,2; } // blend weights, fixed-point /128, range −64..191
```

With coding order O0, O1, O2(, O3=alpha), the forward transform
(`code_colour_plane<0>`, `codec.inc:417`) produces, all mod 256:

* **O0**: raw.
* **O1** (`nrefs==1`): `O1 − dc − O0` — plain plane difference.
* **O2** (`nrefs==2`): `O2 − dc − ((w0·O0 + w1·O1 + 40) >> 7)` — a weighted blend of
  the two earlier planes of the *same pixel* (rounding constant 40 ≈ 0.31·128; a
  2-weight blend whose weights sum to 128 with one weight zero — i.e. an exact copy
  of one reference — is collapsed to the single-reference form, `plane_transform`,
  `codec.inc:854`).
* **O3** (`nrefs==3`, the alpha plane): `A − dc − ((w1·ch1 + w0·ch0 + w2·ch2 + 63) >> 7)`
  — a 3-weight mix of the pixel's three colour bytes.

The decoder applies the exact mirror (`code_colour_plane<1>`); coding order guarantees
references are reconstructed first. `dc` recentres the residual histogram so the bulk
of the distribution avoids mod-256 wraps (chosen by `widest_window`, §5).

The alt models replicate this transform internally instead of using the external pass:
alt-P1 codes in the transformed domain via `fold_from`/`unfold_to` with
`plane_mix2/plane_mix3` (`planes.inc:29` — same formulas, +64 rounding in the
3-ref case); alt-P2 does not subtract at all but seeds its per-pixel `dval` state with
the inter-plane prediction scaled to its internal 16× fixed point (§8.2).

### How the transform is chosen (encoder heuristic, `choose_plane_coding`, `codec.inc:272`)

Every estimate in this subsection is printed by `bmf c -v` (§5.1).

All heuristic decisions use **order-0 empirical entropy** of residual histograms:
`estimate_cost` (`planes.inc:5`) computes `Σ nᵢ·log2(N/nᵢ)` bits exactly (no log
table; the even/odd accumulator split is only an ILP unrolling).

For ≥3 planes:

1. Three candidates — which physical channel is the 2-reference "chroma" plane — are
   costed by `cost_candidate` (`codec.inc:575`). It works in the **gradient domain**:
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
   (`WeightSearch`, `codec.inc:12`; step window 4, both directions) on the full
   histogram cost `weight_pair_cost` (`codec.inc:1222`).
3. A slack of `min(cost>>7, kSlackMax=0x4000)` (~0.78 %) lets degenerate predictors
   (copy-A / copy-B / 64:64 average) replace the general blend when nearly as good —
   cheaper to model and to store.
4. Per-plane `dc` = position of the heaviest 256-bin window slid over a value-domain
   residual histogram (`widest_window`, `planes.inc:136`), +1: the blended (and alpha)
   plane over a 1024-bin histogram of the chosen predictor's residuals, the O1
   difference plane over a 512-bin histogram of the O1−O0 differences; the flat plane
   O0 gets no dc.
5. For 32-bit images, the alpha plane gets its own **3-variable closed-form LS fit**
   over 2×2-quad second differences (`fit_alpha_weights`, `planes.inc:156`) with the
   same one-hot simplification slack (`choose_alpha_plane`, `codec.inc:1312`).

---

## 5. Encoder mode search (`search_filter`, `codec.inc:932`)

Everything past the heuristic layer is decided by **real trial encoding**: run the
actual model + range coder over the image, count `8·(stream.cur−stream.buf)` bits,
reset, compare. (At `-Q9` the "tile" is the entire image; the vestigial centering
math suggests smaller quality settings originally used a central sub-window.) Trials
run with `alphabet_reduced=1`, which suppresses the expensive neighbour/mirror
counter updates in the alt-P2 bank cascade (§10.4; alt-P1 and the slow model are
unaffected) — search costs are therefore measured on a slightly cheaper model
variant than the final encode.

1. **Per-plane search** (`search_planes`, `codec.inc:1241`), in coding order. Trial
   flag sets (`try_*`, `planes.inc:185`; as descriptor flag values: `mode0`=0,
   `p1`=5, `p2`=6, `refs`=8, `refs_p1`=13, `refs_p2`=14): `mode0` (raw plane into
   the slow model), `p1 = pred_p1|alt`, `p2 = pred_p2|alt`, and for non-first
   planes the same three with `desc_has_refs`. Pruning: `try_p2` runs only if P1 came within 1/32 of the
   best so far (or another plane already chose P2); `try_mode0` is skipped entirely
   once any plane chose P2. Cheapest flags win per plane.
2. **Transpose trial**: each plane re-encoded on the transposed image, with early
   abort when a plane exceeds its normal cost by >1/256; adopted only when the total
   wins by >1/4096 — then the real image is transposed and `flags_transposed` set.
3. **Mode unification trials**, each a full re-encode via `transform_cost`
   (`codec.inc:1110`): for >2 planes, force P1 on all planes and force P2 on all
   planes (optionally re-adding refs via `allow_refs_where_present`); separately,
   for >1 plane when no plane chose P2: drop `desc_alt_model` everywhere (explicit
   MED + slow model) and clear all flags (plain interleaved slow model). Any winner
   selects **interleaved** ("together") coding.
4. Return value: 1 → **planar** mode (each plane its own transform + model + coded
   segment), 0 → **interleaved** mode (`transform_planes`, `codec.inc:1117`: apply all
   transforms in place reading from a pristine copy, then one model pass over the
   interleaved bytes — either one slow-model pass, or the joint multi-plane alt-P1 /
   alt-P2 coders). The trade-off being searched: interleaved coding lets the model
   see cross-byte pixel-word structure directly (and lets the joint alt models use
   same-pixel cross-plane state), while planar coding wins when each channel's own
   spatial structure dominates the inter-channel correlation.

Images of depth ≤ 4 bpp skip the search entirely and go straight to a single
slow-model pass over the packed data (`code_image_body`, `codec.inc:456`).

### 5.1 Watching the search: `-v`

`bmf c -v in.bmp out` prints every cost the encoder measures or estimates — the
colour-transform candidates of §4, then each per-plane coding trial, then each
whole-image trial — with `<<` on the option that was taken; `bmf d -v in out.bmp`
prints the descriptors it reads back off the wire. All costs are reported in
bytes with one decimal: the encoder works in bits internally, and the fraction is
what is left after dividing by 8. Reporting is print-only — it never touches the
coded stream, and without `-v` not a byte of output changes.

```
$ bmf c -v testfiles/t24.bmp /tmp/t24.bmf
File testfiles/t24.bmp, image 320x240x24, size - 230400:
[choose_plane_coding] 3 planes; order-0 entropy of gradient-domain residuals
  which plane is predicted from the two others:
    plane 0          81848.8 bytes  <<
    plane 1          81848.8 bytes
    plane 2          82028.4 bytes
  plane 0 predicted from plane 2 (coded flat) and plane 1 (coded as a difference):
    least-squares fit       65464.0 bytes
    copy first ref          12405.2 bytes  <<
    copy second ref         12532.5 bytes
    average 64:64           15649.8 bytes
    weights (0,0) fitted -> (-5,-5) after coordinate descent, 65863.1 -> 65464.0 bytes; a degenerate form wins if it comes within 511.4 bytes of that
[search_filter] 320x240, 3 planes, 24 bits/pixel raw
  per-plane trials (whole-image tile, each plane coded on its own):
    slot 0 = plane 2:
      slow           58756.0 bytes
      p1             53052.0 bytes  <<
      p2             53764.0 bytes
    slot 1 = plane 1:
      slow           55232.0 bytes
      p1             52796.0 bytes
      p2             53148.0 bytes
      refs+slow        476.0 bytes
      refs+p1          120.0 bytes  <<
      refs+p2         1328.0 bytes
    slot 2 = plane 0:
      slow           52640.0 bytes
      p1             55804.0 bytes
      refs+slow        724.0 bytes  <<
      refs+p1          896.0 bytes
  whole-image trials (against the per-plane total):
    planar                      53896.0 bytes  (p1=2 p2=0 refs=2)
    transposed                 106536.1 bytes  (abandoned after 1 of 3 planes)
    joint alt-P1                54624.0 bytes
    joint, no alt model         57964.0 bytes
    joint, flags cleared       164000.0 bytes
  choice: planar -- each plane coded separately, 53896.0 bytes (5.614 bpp) by the trials' own measure
[chosen descriptors]
  plane 0: slot 2  refs+slow dc=42  w=(128,0)
  plane 1: slot 1  refs+p1   dc=128
  plane 2: slot 0  p1
  coded body 53904 bytes vs 230400 raw: shipping the coded member
  actual coded size:  5.615 bpp
```

Two different measures are on show. The `[choose_plane_coding]` block is
**estimated**: order-0 entropy of gradient-domain residual histograms, never a
real encode, which is why its numbers bear no relation to the coded size. The
`[search_filter]` block is **measured**: every line is a real model + range-coder
pass, counted as `8·(stream.cur−stream.buf)`.

The listing shows the pruning of step 1 directly: plane 0 (coding slot 2) never
gets a `p2` trial, because `p1` did not come within 1/32 of the best so far and no
earlier plane had chosen P2. It shows how cheap a referenced plane is once its
reference is coded — 120 bytes against 52 796 for the same plane standalone — and
that the search's own 53 896.0 bytes is within a thousandth of the 5.615 bpp
actually shipped, since at `-Q9` the "tile" is the whole image and the final
encode repeats the winning trial almost exactly. The residual difference is the
descriptor bits plus the alt-P2 counter updates that trials suppress.

It also makes §4's slack rule concrete. Here the least-squares fit lands on
`(0,0)` — it predicts nothing, so its 65 464 bytes is five times what simply
copying a reference costs, and `copy first ref` wins outright; that is where the
`w=(128,0)` in the descriptor dump comes from.

`x_ep` exercises everything `t24` does not — four planes, so the alpha block
appears; a least-squares fit that beats every degenerate form; alt-P2 winning
every plane; and two whole-image trials adopted in turn:

```
$ bmf c -v testfiles/x_ep.bmp /tmp/x_ep.bmf
File testfiles/x_ep.bmp, image 705x800x32, size - 2256000:
[choose_plane_coding] 4 planes; order-0 entropy of gradient-domain residuals
  which plane is predicted from the two others:
    plane 0         550702.6 bytes  <<
    plane 1         552624.1 bytes
    plane 2         561658.2 bytes
  plane 0 predicted from plane 2 (coded flat) and plane 1 (coded as a difference):
    least-squares fit      140058.1 bytes  <<
    copy first ref         171751.5 bytes
    copy second ref        220114.9 bytes
    average 64:64          175670.2 bytes
    weights (21,46) fitted -> (19,49) after coordinate descent, 140893.0 -> 140058.1 bytes; a degenerate form wins if it comes within 1094.1 bytes of that
  alpha plane predicted from the pixel's three colour bytes:
    least-squares fit       91630.8 bytes  <<
    copy channel 0         203675.9 bytes
    copy channel 1         268818.5 bytes
    copy channel 2         201224.6 bytes
    weights (-10,-1,-15) fitted, (-10,-1,-15) used
[search_filter] 705x800, 4 planes, 32 bits/pixel raw
  per-plane trials (whole-image tile, each plane coded on its own):
    slot 0 = plane 2:
      slow          125500.0 bytes
      p1            115536.0 bytes
      p2            110740.0 bytes  <<
    slot 1 = plane 1:
      p1            185792.0 bytes
      p2            174632.0 bytes
      refs+slow     170432.0 bytes
      refs+p1       160944.0 bytes
      refs+p2       149080.0 bytes  <<
    slot 2 = plane 0:
      p1            119592.0 bytes
      p2            112872.0 bytes
      refs+slow      87068.0 bytes
      refs+p1        84424.0 bytes
      refs+p2        81420.0 bytes  <<
    slot 3 = plane 3:
      p1             24156.0 bytes
      p2             21652.0 bytes  <<
      refs+slow      59276.0 bytes
      refs+p1        61676.0 bytes
      refs+p2        69124.0 bytes
  whole-image trials (against the per-plane total):
    planar                     362892.0 bytes  (p1=0 p2=4 refs=2)
    transposed                 362424.0 bytes  <<
    joint alt-P2               331852.0 bytes  <<
    joint alt-P2 + refs        331936.0 bytes
  choice: together -- one joint transform, 331852.0 bytes (4.707 bpp) by the trials' own measure
[chosen descriptors]
  plane 0: slot 2  refs+p2   dc=237  w=(19,49)
  plane 1: slot 1  refs+p2   dc=244
  plane 2: slot 0  p2
  plane 3: slot 3  p2
  coded body 330640 bytes vs 2256000 raw: shipping the coded member
  actual coded size:  4.690 bpp
```

Two `<<` marks appear in the whole-image block because the marker means *adopted
at this point*, not *final winner*: the transpose beat the planar total, and the
joint alt-P2 transform then beat the transpose. The `choice:` line names the mode
that survives, and `flags 0x36` in §12 records both decisions.

Absences are informative again. Coding slot 0 has no `refs` trials — nothing is
coded before it — and slots 1–3 have no `slow` trial, because `try_mode0` is
dropped once any plane picks P2. `joint alt-P1` never appears because no plane
chose P1, and the two no-alt-model trials never appear because some plane did
choose P2.

The alpha plane is where the two measures visibly disagree. `choose_plane_coding`
estimates its three-weight mix at 91 630.8 bytes against 201 224.6 for the best
single-channel copy, so it fits weights `(-10,-1,-15)` and sets up a reference;
the real trial encodes then price that reference at 69 124 bytes against 21 652
for coding the plane standalone, and the standalone form wins. That is why plane 3
ends up with no `refs` flag despite the transform having fitted weights for it —
the estimate proposes, the trial encode disposes.

In general: `joint alt-P2` appears only when some plane chose P2 (or joint alt-P1
won), `joint, no alt model` and `joint, flags cleared` only when *no* plane chose
P2, an image with fewer than three planes prints `no inter-plane blend to search`
instead of the transform block, and a ≤ 4 bpp image prints only a line saying the
short path was taken. A member whose trials lose to storing the pixels raw says
so:

```
  coded body 9424 bytes vs 9216 raw: SHIPPING RAW, the coding lost
```

---

## 6. The MED predictor and residual folding

`pred_p1` without the alt model is the classic **LOCO-I / JPEG-LS median edge
detector** applied as a separate pass:

* `med_predict(W, N, NW)` (`planes.inc:36`): returns `min(W,N)` if `NW ≥ max(W,N)`,
  `max(W,N)` if `NW ≤ min(W,N)`, else `W+N−NW`.
* Residuals are **zig-zag folded** to unsigned codes 0,−1,+1,−2,+2,… → 0,1,2,3,4,…
  (`med_fold_table`/`med_unfold`planes.inc:45`.inc:369`).
* `predict_med` (`codec.inc:874`) runs in place, **back to front**, so it reads
  original neighbour values while overwriting; row 0 uses left-DPCM, column 0 uses
  up-DPCM. `unpredict_med` (`planes.inc:77`) is the forward-order mirror over
  reconstructed values.

The folded residual plane is then handed to the slow model as an ordinary symbol
plane. (This path wins on images where MED decorrelates well but the residuals still
have palette-like structure the slow model exploits.)

The alt models use the same zig-zag through a generalized table set
(`alt_init_tables`, `bmf_state.inc:167`): `unfold[code]` → signed residual (odd codes
negative, even positive), `fold[resid]` → code with **near-lossless bucketing** of
width `2q+1` (with `q = near_lossless_q = 0` in this build, `fold` degenerates to the
exact zig-zag), and `fold_hi[resid]` → always-exact code (physically written as
`fold[resid+256]`, deliberately overflowing into the adjacent array member).
`fold_or_refuse` (`counters.inc:167`) is the encoder-side guard: if the quantized
reconstruction would drift from the source by more than ±16 (including mod-256 wrap
pathologies), the exact `fold_hi` code is sent instead. The decoder is oblivious —
it always computes `pred + unfold[code]`.

---

## 7. Shared adaptive-statistics primitives

All models are built from a small set of counter structures. Common idioms: counts
are halved with **ceiling rounding** (`halve_up(x) = x−(x>>1)`, `bmf_util.inc:109` — a
nonzero count never dies), and rescale thresholds grow with use, so every context
anneals from fast adaptation toward long memory.

### 7.1 `BitCtr` — binary counter with lazy parent seeding (`counters.inc:73`)

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

### 7.2 `FreqRec` — 5-level frequency record (`sym_list.inc:4`)

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

### 7.3 `SymList` — sorted symbol list with escape mass and exclusion (`sym_list.inc:72`)

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

### 7.4 `CounterNode` — 7-slot counter with tail buckets (`counters.inc:8`)

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

### 7.5 Symbol-tree strips — log-bucketed magnitude coder (`bmf_state.inc:52`)

A shared magnitude coder used by both alt models (and by `CounterNode`'s tail). Each
context owns a 254-word strip in `model_table_store` (1024 strips): `freq[0]` total,
`freq[1]` an **adaptive per-strip increment**, `freq[2..9]` eight magnitude-level
frequencies, then 122 `FreqPair{f[2]}` binary nodes forming per-level bit trees.
A value is split Elias-gamma-style: level 0 = 0, level 1 = 1, level 2 = 2–3, level
*L*≥3 covers 2^(L−1) values; the level is coded from the 8 frequencies, the offset by
walking the level's binary tree MSB-first (each node a plain dual-count bit model
seeded `{60,36}` — a mild bias toward bit 0 — with increment `alt_freq_init`, halved
above 0x4000). Level seeds: `{205,124,147,83,48,
16,8,4}`; increment seeded at `24·alt_freq_limit` and **annealed** at each rescale
(−16 while large, then −4 down to `alt_freq_limit`) — per-context adaptation that
starts very fast and settles. Model parameters differ by predictor: P1 uses
`alt_freq_init=64, alt_freq_limit=16`; P2 uses 8/8 (`begin_plane_stream`,
`bmf_state.inc:236`). `update_binary_pair` (`bmf_state.inc:133`) is a training-only twin used
for context-space generalization (reduced increments, no coding).

---

## 8. The "slow" model — `ModelBlock` (`model.inc:77`)

The universal plane coder: a context-mixing **symbol** model with no notion of
numeric pixel value — everything is equality structure. It is BMF's palette-image
engine, and also the fallback for any plane/interleaved data the alt models don't
win on. One `ModelBlock` (~13 MB of tables, pooled) codes one plane
(`code_plane_slow`, `model.inc:987` area).

### 8.1 Alphabet reduction

The model operates on a dense alphabet 0..A−1 of the values that actually occur
(`reduce_alphabet` encoder side, `model.inc:315`; `expand_alphabet` decoder side):

* **depth ≤ 8** (`reduce_narrow_alphabet`, `model.inc:214`): presence bitmap over 256
  values; the distinct count is coded flat (`code_alphabet_size`, `rangecoder.inc:166`:
  interval `[n−1, n)` of total 2^depth); the value set is transmitted as **gaps
  between consecutive present values in ascending order**, coded with one adaptive
  dense `SymList` of `2^depth − A + 2` symbols. Ids are value-ordered.
* **depth > 8**: distinct pixel words are collected into a fixed 8192-node BST
  (`tree_place`, `model.inc:291`); ids are assigned in **first-occurrence order**.
  The values are transmitted in id order, byte by byte, each byte coded by one of
  `4·nbytes` dense 256-ary `SymList`s selected by a carry context from the previous
  byte — 2 bits (top bits of the previous byte) within a value, 1 bit across value
  boundaries (`code_symbol_bytes`, `sym_list.inc:162`).
* **Overflow** (> 8192 distinct words): the plane is de-interleaved into byte planes,
  `height *= nbytes`, and the whole model recurses on the stacked byte image
  (re-interleaved on output by `interleave_depth_bytes`).

### 8.2 Per-pixel state

`PixRec` (`model.inc:4`, 8 bytes) per pixel: `sym` (dense symbol) + six **match
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
  **truncated binary number, MSB first** (`code_run_length`, `model.inc:1038`): bit
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
  neighbours are looked up in a perfect table (`ctx_group_flags`, `bmf_tables.inc:4`)
  since transitivity permits exactly B(4)=15 patterns. Each group folds the
  unreachable ranks' prior weight into the surviving ranks (`GroupFolds`), and the
  (last,prev) ranks come from a per-(group, pair-key) memory of the last two symbols
  coded in that context. Seeding gives fewer-level groups a coarser increment
  (`1<<(5−levels)`).
* The **learned** record: `grid[188 + id]` where `id` is an **exactly interned**
  (collision-free, sequentially minted — `intern_ctx`, `model.inc:616` area) context
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
22 spatial neighbours in roughly increasing distance. Writing `r5` for the
current row at the current column and `r6..r9` for one to four rows up, the
exact probe order (`load_neighbours`, `model.inc:734`) is:

```
10 r5[-2]  11 r6[+2]  12 r7[+1]  13 r7[ 0]  14 r6[-2]  15 r7[-1]
16 r5[-3]  17 r6[+3]  18 r6[+4]  19 r5[-4]  20 r6[-3]  21 r7[+2]
22 r8[ 0]  23 r7[-2]  24 r5[-5]  25 r8[+1]  26 r6[+5]  27 r9[ 0]
28 r5[-7]  29 r8[-1]  30 r6[+7]  31 r7[+3]
```

Slots 11–15 are the "near band" and 16–31 the "far band" of the support
context below. For each surviving candidate one adaptive bit "pixel == this
candidate?" is coded. The binary context (`pixel_context`, `model.inc:411`) combines
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

## 9. The alt-P1 model — `AltP1Block` (`alt_p1.inc:4`)

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

Two context indices are formed per pixel (`ctx_of`, `alt_p1.inc:91` area):

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

In the **multi-plane** driver (`alt_model_p1`, `codec.inc:56` — planes coded
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

## 10. The alt-P2 model — `AltP2Block` (`alt_p2.inc:253`)

The strongest continuous-tone model: an adaptive **normalized-LMS linear predictor**
with context-selected weight sets, followed by a five-stage integer bias-correction
cascade (chained SSE), a context-quantized ternary-plus-tree residual coder, and
pervasive update-time generalization. All values are carried in **16× fixed point**
("sample16" = 16·pixel).

### 10.1 Feature vector

`fill_row_inputs` (`alt_p2.inc:378`) gathers **28 float features** (7 rows × 4 lanes)
from the causal neighbourhood: rows 0–3 are gradient-adjusted combinations
(`grad(a,b,c) = a+b−c`, GAP/CALIC-style) of `dval` — the **inter-plane-decorrelated**
values — including vertical/horizontal extrapolations and long-range taps; rows 4–6
are plane-specific mixes of raw `val` neighbours with reference-plane rows,
including cross-plane gradient corrections and taps that carry the reference plane's
own prediction *error* (`v0(−2)+ref->err`). Separate feature sets exist for plane 0,
plane 1, planes ≥2, and the no-reference (greyscale) path.

### 10.2 Per-pixel state and cross-plane wiring

`P2Ctx` (`alt_p2.inc:143`) per pixel: `val` (16×), `dval` (value minus the inter-plane
prediction), signed `err`/absolute `aerr` against the NLMS prediction, four
directional absolute gradients, a ternary residual `sign` with an
activity-dependent deadzone, and the residual magnitude `mag`. Five-row ring with
mirrored margins; row 0/1 have dedicated seeders whose constants (`val=256`,
`aerr=512`, `dup=1024`, `mag=3`, `err=−16`, …) describe a never-seen
neighbourhood as *moderately active with a small error* rather than flat — a
deliberate prior, since the top of an image is where flatness is least safe to
assume.

In the multi-plane driver (`alt_model_p2`, `codec.inc:147`) planes are coded
**interleaved per pixel** (0→1→2→3), so a later plane sees the earlier planes'
*current* pixel. Before a plane's sample is coded, its `dval` is seeded with the
inter-plane prediction (16·ref for the 1-ref plane; `(w1·p1+w0·p0)>>3` — the /128
weights rescaled to 16× — for the 2-ref plane; a 3-term mix for alpha). The model
then stores `dval = val − seed`, i.e. the inter-plane residual, which is what feature
rows 0–3 read. Additionally a shared, per-pixel-decayed accumulator `ctx_bias[4]`
couples the four directional activity sums **across planes** — a busy pixel in one
channel raises the others' activity estimates.

### 10.3 The NLMS predictor (`NbRow`, `alt_p2.inc:165`; `predict`, `alt_p2.inc:169`)

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
  (`bmf_set_denormal_mode`, `records.inc:104`) so the float path stays fast.

### 10.4 The five-bank bias cascade

The float prediction `filt` is refined by **five chained context-modeled integer
corrections** (`step_bank`; run0 = filt + bias₀(ctx₀), run1 = run0 + bias₁(ctx₁), …
run4 = final prediction):

* Each bank has 2¹⁵ contexts of `P2Count{int8 rate; uint8 b1; int16 weighted}`
  (5·32768 total). The predictor is an integrator read-out:
  `bias = (weighted + 2^(rate−1)) >> rate`, i.e. LMS with step 2⁻ʳᵃᵗᵉ.
* A bank's 15-bit context = 4 bits of coarse quantization (`ctx_quant`: 2 bits of
  running-prediction/brightness thresholds + 2 bits of activity thresholds, per-bank
  magic constants) + **11 single-bit features**: each is bit *k* of a signed
  differential test (does the running prediction over/undershoot specific
  neighbour extrapolations, Laplacians, inter-plane consistency terms, pooled
  neighbour-error signs). For a difference bounded below 2^k the extracted bit is
  its sign, but several of the tested expressions carry the unbounded running
  prediction — upstream instrumentation over a 19-image corpus measured one term
  reaching 36157 against bit 15 — so strictly these are arbitrary correlated bits
  the counters learn against, not a guaranteed sign pattern. Each bank is still
  best understood as an SSE/APM stage keyed on the over/undershoot pattern.
* Updates (`code_banks`, `alt_p2.inc:541`): the exact context integrates the full
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

* **Ternary split** (`P2Freq{step; f[3]}`, `alt_p2.inc:10`): zero / negative (odd
  codes) / positive (even codes), from `f[]` with adaptive increment `step` that
  starts at 4096 and anneals toward 16 through the rescale schedule (halve counts at
  a 16384 slot cap; step >>=1 above 256, −32 down to 32, −2 through the 17..32
  band — a band that is exercised in practice: perturbing the −2 changes the
  coded sizes of three of the ten test images). The `{2048, 2816, 2816}` seed
  starts each context biased *against* the zero residual, 2048 vs 5632 for the
  two sign classes together. `step` doubles as a **maturity meter** gating all
  generalization (below).
* **Magnitude** `(code−1)>>1` via the shared symbol-tree strips (§7.5), on a strip
  context = plane | 15-level brightness bucket of the prediction (`p2_ctx_edges`,
  dense near black/white) | activity class; the odd/even halves use two strip
  contexts offset by ~4 pixel levels.

The ternary context (`seat_symbol_context`, `alt_p2.inc:477`) is a 15 552-point
lattice: 16 activity classes (a ~40-tap weighted sum of neighbour residual
magnitudes quantized by `p2_len_edges`) × 2 flatness bits × **five ternary
selectors** composed positionally in mixed radix (64·3⁵): prediction-brightness
class, prediction−N and prediction−W band tests, and the N/W ternary residual signs
(replaced, for chroma planes with refs, by reference-plane gradient band tests).

After coding, the frequency lattice is **smeared** (`alt_p2_model`,
`alt_p2.inc:812`): the ±1 activity-class neighbours get fractional-step updates
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

## 12. Measured behaviour on the test corpus

Which model actually runs is a per-plane, per-image decision; the table below is
read out of the streams this build produces for `testfiles/` (and can be
regenerated a line at a time with `bmf c -v`, §5.1) (`depth`/`flags` are
bytes 14/15 of the member header; models parsed from the descriptor bits, listed
in coding order). Verified locally against this build; the same values appear in
the upstream reconstruction's reference streams.

| image | BMP | depth | flags | path | model(s), coding order |
|---|---|---|---|---|---|
| `t1` | 1 bpp palette | `0x81` | `0x64` | ≤4 bpp short path | slow |
| `f05_200` | 1 bpp palette | `0x81` | `0x64` | short path | slow |
| `DLRAW` | 4 bpp palette | `0x84` | `0x64` | short path | slow |
| `x_ai` | 8 bpp grey ramp | `0x48` | `0x3e` | planar, transposed | slow |
| `x_ci` | 8 bpp grey ramp | `0x48` | `0x3c` | planar | slow |
| `t8g` | 8 bpp grey ramp | `0x48` | `0x3c` | planar | alt-P2 |
| `t8p` | 8 bpp palette | `0x88` | `0x3c` | planar | alt-P2 |
| `t24` | 24 bpp | `0x18` | `0x3c` | planar | alt-P1, alt-P1, slow |
| `t32` | 32 bpp | `0x20` | `0x3c` | planar | alt-P1, alt-P1, slow, alt-P1 |
| `x_ep` | 32 bpp | `0x20` | `0x36` | interleaved, transposed | alt-P2 ×4 |

Descriptor examples (wire values; per *physical* plane, `nrefs` = coding slot):

```
t24   p0: flags=8  (slow+refs)  nrefs=2  dc=42  w=(128,0)   — coded third
      p1: flags=13 (p1+refs)    nrefs=1  dc=128             — coded second
      p2: flags=5  (p1)         nrefs=0                     — coded first
x_ep  p0: flags=14 (p2+refs)    nrefs=2  dc=237 w=(19,49)
      p1: flags=14 (p2+refs)    nrefs=1  dc=244
      p2: flags=6  (p2)         nrefs=0
      p3: flags=6  (p2)         nrefs=3   (no dc/weights on the wire — has_refs clear)
```

Concrete takeaways: the model choice is genuinely per plane (`t24` mixes alt-P1
and the slow model in one stream); the file subformat does not determine the
model above 4 bpp (`x_ci` and `t8g` are both 8-bit grey and land on different
models; `t8g`/`t8p` are the same pixels under different palettes and land on the
same one); coding order is a searched permutation (`t24` codes planes 2, 1, 0,
and its slot-0 blend weight `(128,0)` is the degenerate copy-one-reference
form); and a noise image reaches alt-P2, loses to raw, and ships as a raw
member with flags `0x00` (verified with a random 24-bit image: exactly
20 + 3·w·h bytes, 24.000 bpp).

The upstream corpus adds cases ours lacks (reported there, not verifiable here):
an image whose whole-image trials win as interleaved MED + slow model — the only
way `pred_p1` without `desc_alt_model` (descriptor flags 1/9) reaches a stream —
and interleaved joint alt-P1.

---

## 13. Review notes — oddities and reconstruction artifacts

Observations from reviewing the source; none affect correctness of this build
(the codec round-trips bit-exactly), but they are worth knowing when reading or
modifying the code:

* `opt_*` option constants (`records.inc:92`) are defined but never referenced —
  the `-S -Q9` configuration is constant-folded into the code.
* `predict_med` histograms every folded residual into `hist_scratch`
  (tail of the output buffer), but no reader of that histogram survives in this
  build — likely a vestige of the original's cost estimation.
* `write_bmp_palette` (`bmp.inc:559`) had a `memset` placed *after* the `return`
  of its no-palette branch, so palette bytes for palette-less ≤8-bit images were
  left uninitialised. Harmless for the depths this build emits (≤8-bit output
  always carries `depth_grey` or `depth_palette`), but the two statements are now
  in the order the code clearly intended.
* `alt_init_tables` deliberately writes `fold[i+256]` to fill the adjacent
  `fold_hi[256]` member, and reads `fold[-1+…]` via a `neg` pointer — layout-
  dependent by design.
* The alt-P2 lazy filter pool: `nb_id[1920]` slots map into `nb_weights[1088]`
  rows, and the allocation counter carried no bound — the 1088th distinct slot in
  one plane would have seated past the end of the pool. Upstream measurements put
  the invariant on an empirical footing: a 19-image corpus reaches a high-water
  mark of 593 distinct slots per plane, an adversarial 2500-tile montage reaches
  980, and the union of slots ever seen is 1034; adversarial images built here
  reach 940 — under the 1088 bound, but by a margin narrow enough that 1088 reads
  as a measured figure rather than a guessed one. `seat_nb_row` (`alt_p2.inc:294`)
  now caps the counter and shares the last seated row once the pool is full.
  Encoder and decoder reach that branch identically, so a stream that hit it would
  still decode; only prediction quality would degrade. No known input reaches it,
  so every stream in the corpus is unaffected.
* `P2Coef::fold` (`alt_p2.inc:204`) brackets only the **multi-plane** alt-P2 driver
  (`alt_model_p2`, `codec.inc:147`): it element-wise adds the *static seed*
  coefficients of feature rows 4–6 (`bmf_p2_coef_init` rows 4–6, installed at
  `BMFState::reset`) into rows 0–2 — i.e. into the static weights of *different*
  features — zeroes rows 4–6 and resets their NLMS rates to 0.0024 for the
  duration of the image, restoring everything afterwards. The single-plane `_d8_`
  path never calls it and runs with the declared 7-row table intact, so the
  planar and interleaved alt-P2 paths use *different* static predictors out of
  the same table. Whether the cross-feature addition is intentional is unclear.
* `widest_window`'s DC scans start at histogram entry 4 rather than 0. The
  upstream reconstruction traces this to the original computing the start from
  the low four bits of an `alignas(16)` stack address (always 4), and reports
  byte-identical streams with 0 and 4 over its corpus — an accidental constant,
  kept for fidelity.
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

### 13.1 Defects found and fixed

None of these changed a single coded byte: the 81-image corpus (12 upstream test
files plus 69 generated edge cases) produces streams byte-identical to the
pre-fix baseline, and every image still round-trips losslessly when checked with
an independent BMP decoder rather than BMF's own reader.

* **RLE4 rows never reset the nibble phase at end-of-line** (`read_rle4`,
  `bmp.inc:416`). `hi_nibble` tracks whether the next pixel lands in the high or
  low nibble of the current byte. It is updated by runs and by delta ops, but the
  end-of-line op only moved `row` to the next scanline — so a row containing an
  odd number of pixels left the reader half a byte out of phase, and every
  following row was decoded shifted by one nibble. Rows are byte-aligned in BMP,
  so the phase must reset with the row. This corrupted any conforming RLE4 file
  with an odd-length row; a 300-image generated RLE corpus went from 77
  mis-decodes to zero.
* **RLE4 row-capacity check was over-strict by one byte** (`read_rle4`,
  `bmp.inc:416` and `write_nibbles`'s call site). Both sites asked
  `holds(row, n/2+1)`, which is exact when the run starts on a low nibble but
  demands one byte too many when it starts on a high nibble and covers an even
  number of pixels — so a run that exactly filled the last byte of the image was
  rejected as an overrun. The span is `(n + 1 + !hi_nibble)/2` bytes. This is
  what made BMF refuse to re-read its own RLE4 output for `s4_bands`.
* **Misaligned 4-byte store for 1–3 bytes of row padding** (`write_bmp`,
  `bmp.inc:754`). The padding was zeroed with `((uint32_t*)out_at)[0] = 0`, an
  unaligned 4-byte write that also ran up to 3 bytes past the row. The excess was
  overwritten by the next row's `memcpy` and excluded from the coded length, so
  the file was correct, but the last row wrote past its data. `memset(out_at, 0,
  pad)` writes exactly the padding; UBSan's only complaint on the corpus is gone.
* **Dead `memset` after `return`** in `write_bmp_palette`'s no-palette branch
  (see above).
* **Indeterminate values read** — `choose_plane_coding` and `predict_med` each
  returned a scratch local that is never assigned on a degenerate path
  (`n_planes < 2`, and `height == 1 && width == 1` respectively). Every caller
  discarded the value, so both are now `void`. `alt_model_p1`'s `cur0/cur1/cur3/
  want2` were passed into `code_sample` uninitialised in the `f_DEC == 1`
  instantiation, where the parameter is overwritten by `decode_sample()` before
  use; they are initialised to 0. Clang's `-Wsometimes-uninitialized` /
  `-Wconditional-uninitialized` are now clean.
* **Unbounded NbRow pool index** in `seat_nb_row` (see above).
* **`read_bmp` leaked the `FILE*` and the image buffer** when RLE decoding failed,
  the one early return that did not clean up. The process calls `bmf_fatal`
  immediately afterwards, so nothing observable changed; ASan now reports no leaks
  across ~1000 malformed and well-formed fuzz inputs.

### 13.2 Cache prefetching

`bmf_prefetch<hint>` (`bmf_util.inc:67`) wraps `_mm_prefetch`; the hint names are
`pf_all`/`pf_l2`/`pf_l3`/`pf_once` = `PREFETCHT0`/`T1`/`T2`/`NTA`
(`bmf_util.inc:62`). There is no write-prefetch wrapper: `PREFETCHW` needs the
PRFCHW feature, which `-march=haswell` does not include, so `_m_prefetchw` would
compile down to `PREFETCHT0` anyway.

The only prefetch sites are in `code_banks` (`alt_p2.inc:568`, `alt_p2.inc:585`),
covering the mirror counter and the 11 direct/mirror/rotated triples of the
alt-P2 bias cascade — 34 lines of a 640 KB `p2_ctr` whose addresses are all
derived from `ctxw` before any of them is touched. They are worth having, and
the hint matters (interleaved A/B, min of 4 rounds, compress+decompress):

| variant | `x_ep` (alt-P2 ×4) | `t8g` (alt-P2) |
|---|---|---|
| no prefetch | 13 766 ms | 505 ms |
| `pf_l3` (`PREFETCHT2`) | 12 549 ms | 483 ms |
| `pf_all` (`PREFETCHT0`) | **12 219 ms** | **463 ms** |

`PREFETCHT0` over nothing is −6.7 %/−8.3 %; over `PREFETCHT2` it is −2.6 %/−3.3 %.
`PREFETCHNTA` measured level with `T0`, `T1` between `T0` and `T2`. That ordering
is what the access pattern predicts: these counters are read *and written* within
a few hundred instructions and are re-touched on later pixels, so they want to
land in L1 (`T0`) rather than L2/L3 (`T1`/`T2`); `NTA` also lands in L1 on Intel,
but its streaming eviction policy is wrong for data this hot. The original build
used `__builtin_prefetch(p, 0, 1)`, which gcc/clang map to `PREFETCHT2` — the
weakest of the four here.

**Sites that were tried and dropped.** Cachegrind (`--D1=32K --LL=1M`, so "LL
miss" ≈ misses past L2) says where the deep misses are, and it is not where the
D1 misses are:

| function | D1 read misses | past-L2 read misses |
|---|---|---|
| `walk_bank_bits` (`t8g`) | 6 348 269 (39 %) | 28 097 (3.8 %) |
| `AltP1Block::update_selector` (`t8g`) | 1 328 118 (8.2 %) | 520 913 (70 %) |
| `ModelBlock::open_pixel` (`x_ci`) | 18 947 678 (27 %) | 6 756 736 (54 %) |

Four candidate sites were implemented and A/B'd against this build: prefetching
alt-P1's nine `update_selector` counter triples (10 MB table), alt-P2's five
`bump_bank` frequency bins, `open_pixel`'s `sym_ctr` line (1 MB table, read only
after the `context_ids` chain), and software-pipelining the 34-line burst above
so it stops overrunning the core's line-fill buffers. Every one of them landed
inside the ±1.5 % run-to-run noise floor, in both directions, so none were kept.
Two reasons, both visible in the table: the bulk of the D1 misses are L2 hits
that out-of-order execution already covers, and the misses that do go deep sit on
serial dependency chains (context → table → next context) where the address is
not known any earlier. `code_banks` is the one place in the codec with a batch of
*independent* scattered addresses known ahead of use — which is presumably why it
is the one place the original author prefetched.

---

## 14. Key constants

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
