# BMF 2.01 — the compression algorithms

What this program does to an image, read out of the decompilation in this
directory. The range coder is deliberately out of scope: everything below says
"coded" where a value is handed to `rc` (`rc.inc`), and the only thing you need
to know about it is that it takes either a `(cum, cum+w, total)` triple against
a frequency table or a `(f0, f1, bit)` triple against a binary counter pair.

Names in `code font` are the names in this tree, so every claim here can be
checked against the file it comes from.

There is not one algorithm. There are **three models**, a **decision layer**
that chooses between them per plane, and a **raw fallback**. Which one runs is
decided partly by the BMP subformat and partly by trial encoding.

---

## 1. The container

`bmf c in.bmp out` **appends**. `arc.inc` opens the output `"a+b"`, and
`compress_image.inc` walks any members already there before writing, so a second
run against an existing file produces a two-member archive rather than replacing
it. `bmf d` decodes members until one fails to parse, writing each to the same
output path — so the file left behind is the last member.

Each member is:

| bytes | what |
| --- | --- |
| 4 | magic `81 8A 32 30` — `\x81\x8A` then `"20"`, the version. `expand_image.inc` computes `((major<<8) - 12288) \| (minor - 48)` and requires 512, i.e. exactly "2.0" |
| 16 | the header, which is the in-memory `BmfImage`: `width` u16, `height` u16, `stride` u32, 2 pad, `depth` u8, `flags` u8, `data_size` u32 |
| n | the coded stream, `data_size` bytes |
| p | the palette, `3 << depth` bytes, only when `depth & 0x80` |

A second magic, `81 90 32 30`, marks an auxiliary block: an 8-byte header whose
second word is a length, then that many bytes. The decoder skips it (or hands it
back through `coded_block` so a re-compress can copy it forward); this build
never writes one.

### the header's two flag bytes

`depth` carries the pixel depth in bits 0–5 and two markers:

- `0x80` — a palette follows the coded data.
- `0x40` — greyscale. Set by `__bmf_compress` when the palette turns out to be
  an exact linear grey ramp: it walks the entries checking
  `pal[3i] == pal[3i+1] == pal[3i+2] == i * (256 >> depth)`, and if they all
  are, sets `0x40`, clears `0x80` and **the palette is not stored at all**. The
  decoder rebuilds it. For an 8-bit greyscale image that is 768 bytes saved and
  the reason `t8g.bmp` and `t8p.bmp` — same pixels, different palettes — do not
  code to the same size.

`flags` is the mode word:

| bit | meaning |
| --- | --- |
| `0x02` | the image is stored **transposed** — rows and columns swapped |
| `0x04` | written in "slow" mode (`-S`). This build **refuses** a stream without it: `expand_image.inc` prints `written in fast mode` and exits 3 |
| `0x08` | **planar**: each plane was coded on its own. Without it the planes are coded interleaved, in one pass |
| `0x10` | per-plane descriptors are present in the stream (set for depth > 4) |
| `0x20` | the member is **coded**. Without it the payload is the raw pixels |

### the raw fallback

After coding, `compress_image.inc` compares the coded length against
`data_size` — the size of the **pixel data**, not of the input file. If coding
did not beat that, the member is rewritten with `0x20` clear and the pixels
stored verbatim.

A raw member is therefore 20 bytes of overhead (magic and header) plus the
pixel data plus, if there is one, a palette at 3 bytes an entry. The BMP it
came from is 54 bytes plus 4 bytes an entry plus rows padded to a multiple of
four, and BMF stores its rows unpadded — so a raw member is always **smaller**
than the file it came from, not merely bounded by it. `noise24.bmp` is 49206
bytes of random data and `ref_noise24.bmf` is 49172, thirty-four fewer.

If the image was transposed for coding it is transposed back first, since a raw
member has no model to undo it.

---

## 2. What comes in: the BMP subformats

`read_bmp.inc` accepts a 40-byte `BITMAPINFOHEADER` only, with `biPlanes == 1`,
positive dimensions up to 65535, and

- **1, 4, 8, 24 or 32 bits per pixel** — nothing else, and no bitfield formats,
- **`biCompression` 0, 1 (RLE8, 8bpp only) or 2 (RLE4, 4bpp only)**.

Both run-length forms are decoded on the way in: the absolute runs, the encoded
runs, the end-of-line marker, and the delta escape (`00 02 dx dy`) that skips
forward in both axes. RLE4 additionally tracks a nibble phase across deltas.
**The run structure is not preserved.** BMF holds pixels, and `write_bmp.inc`
re-encodes runs with its own splitting when it writes an RLE file back out. That
is why the corpus carries `out_rle4.bmp` and `out_rle8.bmp`: the decoded image
is pixel-identical to the input and byte-different from it, and those two files
are what the decoder is expected to write.

Only bottom-up BMPs are accepted — a negative `biHeight` is refused rather
than read as top-down — and the reader **un-flips them**: it fills the buffer
from its last row backwards while reading the file forwards, so what BMF holds
is top row first, in ordinary raster order, and `write_bmp.inc` flips it back.

Whatever the depth, the image is held as **`plane_count = (depth + 7) / 8`
byte planes, interleaved per pixel**, in `__alloc_image`'s single allocation. So 24-bit is 3 planes (B, G, R in file
order), 32-bit is 4, and everything at 8 bits or below is 1 plane — sub-byte
depths are held unpacked, one byte per pixel, which is what lets the same model
handle 1-, 4- and 8-bit images.

---

## 3. The decision layer

Only the encoder runs this; the decoder is told the answers.

### 3.1 depth ≤ 4 takes the short path

`compress_image.inc` checks `(depth & 0x3F) <= 4` and, if so, sets
`plane_predictor = 0`, `plane_alt_model = 0` and calls `__model_plane`
directly. **No search, no transform, no predictor** — a 1- or 4-bit image
always goes through the main model, raw. There is one plane and it is small
alphabet; the main model's match and run machinery is what suits it.

Everything below applies to 8, 24 and 32 bits.

### 3.2 `__choose_plane_coding` — plane order, colour transform, DC

The descriptors are the stream's whole record of these decisions, and they are
indexed two ways, which is easy to get backwards: **`plane_desc[k+1]` describes
source plane `k`**, while **`plane_desc[k+1].src_plane` gives the plane coded
`k`-th**. So a plane that references the others is described in its own slot and
coded last. `t24` codes its planes in the order 2, 1, 0, and it is plane 0 —
coded last, with both others available — that carries the colour transform.

First the plane order: planes are paired up (`0,1`, `2,3`, …) so a plane can
reference the one before it. Then, for three or more planes, `__cost_candidate`
is run over two candidate orderings and the cheaper is kept — this is what
decides whether the green plane or the blue plane is the one everything else is
predicted from.

Then the transform itself. For each plane it accumulates **histograms of
differences** against the reference planes over the whole image:

- `c0 - c1` and `c0 - c2` — the plane minus each of two references,
- `c0 - ((c1 + c2) * 64 + 40) / 128` — the plane minus their average.

Each histogram is scored by sliding a 256-wide window over it and taking the
densest position; the density is the cost (a proxy for entropy: the tighter the
residual distribution, the cheaper the plane), and the window's position becomes
the **DC offset** stored in `plane_desc[].dc`. The four candidates — subtract
reference A, subtract reference B, subtract their average, or subtract nothing —
are compared with a `cost + cost/32` slack, and the winner sets
`weight0`/`weight1` to `(128,0)`, `(0,128)`, `(64,64)` or the flat case.

For a fourth plane (32-bit) the same is done with three references and
`weight2`, using the three plane values preceding this one within the pixel.

`__colour_transform` then applies it, in `plane_desc[].nrefs` modes:

```
nrefs == 1   d = x - dc - ref
nrefs == 2   d = x - dc - (w0*refA + w1*refB + 40) / 128
nrefs == 3   d = x - dc - (w0*p[-3] + w1*p[-2] + w2*p[-1] + 63) / 128
```

all in 8-bit wraparound arithmetic, which is what makes it exactly invertible.

### 3.3 `__search_filter` — trial encoding

This is what `-Q9` spends its time on. It cuts a **tile from the centre of the
image** — the full width and height if they fit, otherwise centred — and
actually encodes it, repeatedly, throwing the output away and keeping only the
bit count.

For each plane it tries this set of `flags` values, where bits 0–1 are the
predictor, bit 2 selects the alternate model and bit 3 the colour transform:

| flags | predictor | model | transform |
| --- | --- | --- | --- |
| 0 | none | main | no |
| 5 | 1 | alternate p1 | no |
| 6 | 2 | alternate p2 | no |
| 8 | none | main | yes |
| 13 | 1 | alternate p1 | yes |
| 14 | 2 | alternate p2 | yes |

Not all six are always tried, and the condition that prunes them is the same
one throughout: **has any earlier plane already chosen predictor 2**. The
counter is called `n_hard` and it is incremented exactly when a plane settles
on predictor 2, on the reasoning that an image needing the deep model for one
plane will need it for the rest and the cheap candidates are not worth timing.

So: `0` is skipped once that counter is non-zero; `6` is tried when `5` came
within `cost/32` of the best so far **or** the counter is non-zero; `8` and
`13` only from the second plane onward, since the first has nothing to
reference; `14` only when the winner so far is predictor 2, or `13` came within
`cost/32`.

Three more decisions follow, each another trial encode of the whole tile:

- **transposed or not.** The tile is rotated — `width`/`height` swapped,
  `flags ^= 2` — re-encoded, and the transpose is kept if it is smaller. An
  image with strong vertical structure codes better with the rows running the
  other way.
- **planar or interleaved, alternate models kept.** With three or more planes
  and at least one plane having chosen predictor 1, it re-encodes with every
  plane forced to `(flags & 8) | 5` through `__transform_planes` — the
  transform decision kept, everything driven by the p1 model — and takes it if
  it is no worse.
- **planar or interleaved, alternate models dropped.** Then, with more than one
  plane, it re-encodes with every plane's flags masked by **`& 0xFB`**, which
  clears the alternate-model bit and *keeps the predictor and the transform*.
  So `13 → 9`, `5 → 1`, `14 → 10`, `6 → 2`: the same transform and the same
  predictor, but the **main model** coding the interleaved image in one pass at
  full depth. This is the only way a stream ends up with predictor 1 and the
  main model, and it is what `testfiles/med32.bmp` exercises.

The return value becomes header bit `0x08`: set means planar, clear means the
whole interleaved image goes through one model pass.

### 3.4 what the corpus actually chooses

Measured, by counting which model entry points the *decoder* enters for each
reference stream — so this is what the final stream uses, not what the search
tried:

| image | BMP subformat | header flags | what codes it |
| --- | --- | --- | --- |
| `t1`, `f05_200` | 1 bpp | — | main model, whole image |
| `DLRAW`, `rle4` | 4 bpp, one RLE4 | — | main model, whole image |
| `x_ai`, `x_ci` | 8 bpp RLE8 | — | main model, whole image |
| `t8g`, `t8p`, `rle8` | 8 bpp (grey, palette, RLE8) | `0x3c` planar | **p2**, single plane |
| `t24` | 24 bpp | `0x3c` planar | plane 0 → main model + transform; plane 1 → **p1** + transform; plane 2 → **p1**. Coded in the order 2, 1, 0 |
| `t32` | 32 bpp | `0x3c` planar | as `t24`, plus plane 3 → **p1**. Three planes p1, one main model |
| `med32` | 32 bpp | `0x36` interleaved, transposed | **MED + main model**, one interleaved pass |
| `altp1` | 32 bpp | `0x36` interleaved, transposed | **p1 interleaved**, all four planes per pixel |
| `x_ep` | 32 bpp | `0x36` interleaved, transposed | **p2 interleaved** |
| `noise24` | 24 bpp | `0x20` clear | raw — nothing compressed it |

Two things this makes concrete. **The choice is per plane, not per image**:
`t24` codes one plane with the main model and two with p1, in the same stream.
And **the subformat does not determine the model** above 4 bits — `t8g` and
`x_ci` are both 8 bpp and land on different models, because the search measures
them rather than classifying them.

One property worth knowing: `__choose_plane_coding` sets `alphabet_reduced = 1`
and `__compress_image` sets it back to `0` before the real encode. That flag
gates the p2 model's neighbour-context updates (§6.3), so **the search measures
a cheaper model than the one that finally runs.** The costs it compares are
consistent with each other, not with the final stream.

---

## 4. Model A — the main model

`model.inc`, driven by `model_plane.inc`. It runs for every image at 4 bits or
below, for every plane the search leaves at flags 0 or 8, and for the whole
interleaved image when the alt-off trial of §3.3 wins — in that last case at
the image's full depth, so a 32-bit image is one pass over 4-byte symbols
rather than four passes over bytes. It is a context-mixing coder
built around *matching neighbours* rather than predicting values, which is why
it suits palette images: symbol 200 and symbol 201 have nothing to do with each
other, so arithmetic on them is meaningless, but "the same as the pixel above"
is meaningful at any depth.

### 4.1 alphabet reduction

`reduce_alphabet.inc` runs first. It scans the plane, marks which of the 2^depth
symbol values actually occur, and:

1. codes the **count** of distinct symbols with a flat code over the full range;
2. codes the **set**, as sorted gaps between used values, each gap coded through
   a dense `SymList` (§4.5) — so a palette image using 40 of 256 entries pays
   for 40 gaps, not 256 flags;
3. rewrites the plane into `sym_word[]` with each pixel replaced by its **index
   in the reduced alphabet**.

Everything after this works in the reduced alphabet, so the model's tables are
sized by what the image actually uses. For depths above 8 the same is done over
multi-byte values through a hash of distinct words.

### 4.2 the neighbourhood

`row_cur[0..4]` are five row buffers, rotated one step per row, and
`row_cur[5..9]` are the working cursors into them — the current row and then
one, two, three and four rows up. Each cursor sits seven records into its
buffer, so the model can read several pixels either side of the current column
without falling off an end. Four neighbours are named throughout, and are
kept in `mode_symbol[1..4]`:

```
        NW  N  NE
         W  ?
```

`mode_symbol[1] = N`, `[2] = W`, `[3] = NE`, `[4] = NW`.

Alongside each symbol, every `PixRec` carries six `match[]` flags recording
whether that pixel equalled particular neighbours — these are the model's
"texture" bits, and they are what the context signatures are built from rather
than the symbol values.

### 4.3 the per-pixel ladder

`ModelBlock::code_pixel` is one pass down a ladder, stopping at the first rung
that succeeds.

**Rung 1 — run mode.** If the current pixel's west neighbour matched on both
its flags *and* ten `match` flags across the two rows above are all set, the
model is in a flat region. It measures how far the flat region extends
(`run` = the run of pixels above whose `match[0] & match[1]` hold), counts the
actual run of pixels equal to N, and codes a **binary "did the run reach the
end"** decision through `esc_ctr[]`, contexted on the alphabet-map entry for N,
a run-length bucket, and two match bits. If the run was short, its length is
coded bit by bit, most significant first, through `run_ctr[48]` — three groups
of sixteen, the group chosen by whether any higher bit was set yet and whether
the bucket is the maximum, the entry within it by the bit position.
A hit returns the whole run at once; that is the return value `code_pixel`
gives back to the driver, which advances `x` by it.

**Rung 2 — the four-way match.** Otherwise the pixel is coded against a
five-symbol alphabet: *escape, N, W, NE, NW*, with weights `w[0..4]` and total
`w[5]` in a `FreqRec` picked by a context bucket. That bucket is
`ctx_bucket[ctx_state + cap]`. `ctx_state` is a 6-bit signature of the
neighbourhood's match flags folded through `ctx_state[]` into one of fifteen
groups, and `cap` says which of the four neighbours the **last** and the
**previous** symbol seen in this exact context were — five possibilities each,
counting "none". Fifteen groups times five times five is the 375 entries
`ctx_bucket[]` has. The frequencies are rescaled by halving when
the total passes `w[6]` or 0x4000, with a floor that keeps the escape reachable.

There are two `FreqRec`s in play: a per-bucket one and a per-`ctx_id` one, and
when the latter is fresh it is seeded from the former by a 21/`w[5]` blend —
a cheap way to give a new context a prior instead of a uniform start.

The `ctx_id` chain is three levels of **lazily allocated context identifiers**:
`ctx_id1` is indexed by a 10-bit signature of gradient flags and match bits and
the bucket; `ctx_id2` refines it with four more bits; `ctx_id3` refines that
with the low nibble of W, but only when the alphabet is under 32 symbols. Each
table hands out a fresh id the first time a signature is seen, so the model
only pays for contexts the image actually visits.

A hit on any of the four neighbours ends the pixel.

The binary counters all this runs on — `bit_node[]` against `bit_root[]`,
`esc_ctr[]`, `run_ctr[]` — are one three-state object, `BitCtr`. A counter is
**unseen** while it is all zeroes, in which case its *parent* codes the bit and
the counter keeps it as `bit + 1`; **seeded** on the second visit, taking the
parent's shape scaled to 64 by `(64*(parent - 1) + total) / total` per side;
and **live** after that, coding on its own counts, halving them when the total
passes a limit that itself climbs by 64 up to 0x4000, and passing a partial
update back up to the parent while the total is still under 0x88.

That is not a reading of the decompilation alone: `bcdr.cpp`'s `BIN_CODER` is
the same counter written by hand, with the same three states, the same seeding
formula at 32 instead of 64, the same `scaleRare`, and the same "kinda partial
update" of the parent. The two agree closely enough that `bitctr.inc` now uses
that file's names — `scale_rare`, `seed_from`, `init_parent` — and records
where they came from. The one structural difference is which slot holds the
"seen once" mark: `BIN_CODER` uses `MaxFreq == 0` and remembers the bit in
`s[1]`, BMF uses `n[1] == 0` and remembers it in `n[0]`, so `n[0]` is a count
in one state and a remembered bit in another.

**Rung 3 — the escape ladder.** On escape, 32 candidate symbols are assembled:

- 2 from `group_ctr[ctx_state][key]` — the last and previous symbol seen in this
  exact context, where `key` is derived from N, W, NE, NW through `sym_rev[]`;
- 8 from `sym_ctr[8*key]`, a per-key symbol cache;
- 22 spatial neighbours, spiralling outward — `W-2`, `NE+1`, two rows up, `W-3`,
  `NE+2`, `W-4`, three rows up, and so on.

Each candidate goes through `pixel_context`, which returns −1 to skip it (it is
already excluded, or the candidate has no support: it does not appear among
the 11..15 or 16..31 bands of the neighbourhood and is past position 6). For a
candidate worth asking about, a **binary decision** is coded through
`bit_node[]` against `bit_root[]`, contexted on whether the candidate matched
the near band, the far band, whether it appears in the top of the W and N
symbol lists, and the candidate's position in the ladder. A hit ends the pixel;
a miss adds the candidate to the **exclusion mask**, so it costs nothing in
every later step.

**Rung 4 — the symbol lists.** If all 32 miss, three `SymList`s are searched in
order: the list of symbols seen after this W, the list seen after this N, and
finally the dense escape list holding the whole alphabet. Each is a
move-to-front frequency list (§4.5) and each codes an escape if the symbol is
not in it. The last one always has it.

### 4.4 what the driver does between pixels

`model_plane.inc` rotates the five row buffers per row, recomputes the four
`grad[]` accumulators from the match flags of the rows above, and calls
`init_tables()` after every pixel — that is the per-pixel bookkeeping that
folds the coded symbol back into the counters and caches.

### 4.5 `SymList`

An array of `(symbol, count)` pairs kept in descending count order. Coding a
symbol walks the list accumulating the counts of entries not excluded, codes
`(cum, cum+count, total+cum)` and then bumps the entry by 4 and bubbles it
toward the front. Not finding it codes the accumulated total as an escape and
excludes everything in the list on the way out.

Rescale runs when `since_rescale` passes `rescale_at`, or the moment any one
count reaches 252. It halves every count — with a rounding bias of one when the
threshold is below `20 * n` — and re-sorts as it goes. Counts that reach **zero
are dropped**: the tail of the list is walked back, `live` shrinks by however
many died, and the escape weight `tot` gains one for each. So the list forgets
symbols rather than protecting them, and a forgotten symbol costs an escape to
say again. `tot` and `since_rescale` are then halved themselves.

Lists come in two flavours: **dense** (`__init_symbol_list(..., 1)`) starts with
every symbol present at count 1, and **sparse** starts empty and grows.

---

## 5. Model B — the alternate model for predictor 1

`altp1.inc` holds the block and its context. Selected by flags 5 or 13. This is
a *continuous-tone* model: it predicts a value, codes the residual, and
everything is arithmetic on pixel magnitudes.

It has **two drivers**, and which one runs is the planar/interleaved decision of
§3.3:

- `p1.inc`'s `__alt_model_p1_d8_*` — **one plane**, one `AltP1Block`, called
  once per plane in planar mode. This is what `t24` and `t32` use.
- `alt_model_p1.inc`'s `__alt_model_p1` — **all planes at once**, one
  `AltP1Block` each, walking the interleaved buffer and coding plane 0, 1, 2, 3
  of every pixel before moving on. Each plane's block is given the two planes
  before it as `nb0`/`nb1`, so its context can see what the other planes just
  did — which the planar driver cannot. It also applies the colour transform
  itself, per pixel, rather than having it done up front. `altp1` is the corpus
  image that reaches it.

### 5.1 prediction

`AltP1Block::ctx_of` computes the **MED / LOCO-I gradient predictor**, the
same one JPEG-LS uses:

```
NW >= max(W, N)   ->  pred = min(W, N)      a vertical edge left of the pixel
NW <= min(W, N)   ->  pred = max(W, N)      a horizontal edge above it
otherwise         ->  pred = W + N - NW     a plane through the three
```

The decompilation writes it as a nest of four comparisons that fall through to
`N` when none fires, which is the same function — the fall-through cases are
exactly the two where `NW` is outside the interval `[min(W,N), max(W,N)]` and
`N` is the answer the table above gives.

### 5.2 the context

An **activity measure** is accumulated from the `mag` fields — the magnitudes of
recent residuals — of thirteen neighbours with fixed weights (`6*W`, `4*(N+W2)`,
`3*(NE+NN)`, `2*(NNE+NE2+W4)`, …), plus contributions from the neighbouring
planes when there are any. That sum is quantised through `level_of[]` and
`group_of[]`, tables built from the edge lists `p1_level_edges` and
`p1_group_edges` — an eight-level logarithmic ladder.

On top of that, **nine ternary features** are computed, each a comparison
against zero or against a step that scales with the activity level:

| feature | what it compares |
| --- | --- |
| `ctx_w[0]` | the predicted value against 22 and 216 — is this near the range ends |
| `ctx_w[1]` | NW against N |
| `ctx_w[2]` | NW against W |
| `ctx_w[3]` | the reference plane's own residual, or NN against the prediction |
| `ctx_w[4]` | NE against the prediction, with the activity step as deadband |
| `ctx_w[5..8]` | four second-order gradients — `2W − W2 − pred`, `2N − NN − pred`, `NE + W − N − pred`, and either a third-order term or the reference plane's local slope |

Each is 0, 1 or 2, and the nine are combined as a **mixed-radix base-3 index**:
`ctx[0] = 32 * Σ sel_k · 3^k`, which is why `counters[]` has 629856 = 32 · 3^9
entries. The low five bits are left for the activity level, so contexts that
differ only in activity are adjacent — which is what lets the update step
(§5.4) also nudge the neighbouring levels.

### 5.3 coding a residual

The residual `sample − pred` is **zigzag folded** by `__alt_init_tables`:
0 → 0, −1 → 1, +1 → 2, −2 → 3, +2 → 4, … so small residuals become small
codes. The fold and unfold tables are exact inverses.

The folded code is coded in two stages:

1. **`__alt_p1_code_symbol`** codes the code's *slot* against a seven-way
   frequency array — slots 0–4 are the codes 0–4 directly, and slots 5 and 6
   are "an odd code ≥ 5" and "an even code ≥ 5".

   The total lives in `freq[0]`, and only its low fifteen bits: **bit 15 is a
   marker that the array has been rescaled at least once**, which is why every
   read of the total is `& 0x7FFF`. Rescale triggers above 0x2000 and picks its
   divisor from the *smallest* count in the array — by **halves** if that has
   fallen to 1 or 0, and by **thirds** otherwise. Rescaling by thirds when
   something is already at 1 would round it away.
2. Slots 5 and 6 **escape into a binary tree**: `__code_symbol_tree` walks
   `level_geom[]` — a table of per-level spans laid out by `__rc_begin` — coding
   one bit per level through `FreqPair` counters, with the level itself chosen by
   `model_geometry[code]`. The tree is what makes large residuals cost
   logarithmically rather than linearly.

### 5.4 the update

`AltP1Block::update_model` is where the model earns its keep. It updates not
just the context that coded the symbol but its **neighbours in activity space**:
the context one level up gets `+11`, one level down `+13`, and an *alternate*
context — the same nine features read in the opposite direction,
`ctx_w[k].w[2 - sel_k]` — gets `+17` for the *reversed* residual. That last one
is the model learning the mirrored statistic for free, on the assumption that
an image with a bias one way has the opposite bias somewhere else.

When a context's total is still under 0xCCC it also updates the alternate's
neighbours, so a cold context borrows from a warm one.

### 5.5 near-lossless machinery, inert here

Every encode path in this model carries a drift check:

```
recon = unfold[code] + pred
out   = recon + *dst - want
if (|*dst - out| > 16) code = fold_hi[resid];   // re-send through the coarse table
else *dst = out;
```

With `-S` — the only mode this build writes — `near_lossless_q` is 0, so
`__alt_init_tables` builds `fold` as the exact inverse of `unfold`, `recon`
always equals `want`, the drift is always 0 and the `fold_hi` branch cannot
fire. It is the `-E` near-lossless path, kept because it is what the donor does.

---

## 6. Model C — the alternate model for predictor 2

`altp2.inc`, `alt_p2_context.inc`, `alt_p2_model.inc`, `p2.inc`. Selected by
flags 6 or 14, and the most expensive of the three — `alt_p2_model.inc` is the
largest body in the program. Where model B predicts with a fixed rule, this one
**learns its predictor**.

It has the same two drivers as model B: `p2.inc`'s `__alt_p2_d8_body` for one
plane at a time (what `t8g`, `t8p` and `rle8` use — a single 8-bit plane is
this model's home ground), and `alt_model_p2_decode.inc` /
`alt_model_p2_encode.inc` for all planes at once (`x_ep`).

### 6.1 an NLMS linear predictor

Each pixel carries a `P2Ctx` record of eight 16-bit fields: the value, and seven
differences — `err` against W, `aerr` its magnitude, `dleft`, `dup`, `dupleft`,
`dupright`. Those become a **7×4 feature matrix** `p2_row[7][4]` built from the
neighbourhood.

The prediction is `bias + Σ w[j][k] · p2_row[j][k]` over all 28 terms, and after
each sample the weights are updated by **normalised least mean squares**:

```
err = sample - pred
ms[j][k] += (x*x - ms[j][k]) * 0.023          // running mean square of the input
w[j][k]  += rate[j][k] * err * x / (ms[j][k] + floor)
```

with per-position rates from `bmf_p2_rate[7][4]` (0.0108 down to 0.0009 — the
nearest neighbours adapt fastest) and a floor that keeps the division stable.
Weights live in `nb_weights[1088][16][4]`; rows 0–6 are the weights, 7–13 the
mean squares, 14 the scale and confidence, 15 a use count.

### 6.2 which weight set

Not one predictor but **1088 of them**, selected by a quantised description of
the local texture. `__alt_p2_context` accumulates four directional activity sums
— up-left, up-right, left, up — each a fixed-weight combination of eight
neighbours' difference fields, and from them derives:

- `band` — five thresholds on `sum_left` against `sum_up`, so 0–5 says how
  horizontal or vertical the neighbourhood is;
- `gA` — total activity against four thresholds;
- `gB` — a local value estimate against three;
- `gC` — the up-right/up-left ratio against three;
- `gD` — a four-neighbour average against three.

All the thresholds come from `bmf_p2_thresholds[band][13]`, i.e. **the
quantiser itself depends on the band**. The slot is
`320*band + 64*gA + 16*gB + 4*gC + gD`, and `nb_id[]` maps slots to weight sets
lazily, allocating on first use.

A second linear stage, `__alt_p2_filter`, blends **six** weight sets — the six
pointers in `CtxWeights::f0`, each a 7×4 matrix like the one above — using the
six coefficients of `bmf_p2_mix[mode]`, one row of a 4×6 table. `bmf_p2_coef`
is separate and earlier: it makes the centre value the features are measured
against. The result is a mixed prediction.

When the weight set has been used before, its own prediction is blended with
that mixed one by a confidence ratio, `w[14][0] / w[14][1]`. The pair starts at
47/169.2 and both halves are then tracked with a rate of 0.001 — `[14][1]`
following the variance of the difference between the two predictions and
`[14][0]` following its covariance with the sample — so the ratio is a running
least-squares weight, and a set that keeps agreeing with the mix is trusted
less on its own account rather than more.

### 6.3 coding the residual, and the counter cascade

The residual is folded (§5.3) and coded through `P2Freq`, a **three-way counter**
— "the symbol", "escape odd", "escape even" — whose escapes go into the same
binary level tree model B uses. Its three counts halve on rescale,
and the increment they are bumped by decays as the context settles: halving
while it is above 256, then stepping down by 32 at a time until it reaches the
floor.

Underneath sits `p2_ctr[163840]` = **five banks of 32768 counters**, each bank
addressed by a different context derived from the same neighbourhood. Each bank
holds a `weighted` accumulator and an adaptive `rate`, and after each sample:

- the bank's own counter takes the residual, with a deadzone (`deadzone_hi`,
  `deadzone_lo`) so small residuals do not move it;
- its rate accelerates — `b1` counts down, and each time it reaches zero the
  rate index steps up and the accumulator doubles — so a context that keeps
  seeing the same thing sharpens;
- and, **unless `alphabet_reduced` is set**, the counters at
  Hamming-distance-one contexts are updated too: `ctx ^ 0x4000`, `ctx ^ 0x2000`,
  … down to `ctx ^ 0x0010`, each at a reduced weight, plus a mirrored context
  `ctx ^ 0x7FF0` and a rotated one through `p2_ctx_rotate[]`.

That neighbour smoothing is most of the cost of this model, and it is exactly
what `__choose_plane_coding` disables while the search is running (§3.3).

---

## 7. Predictors and transforms outside the models

### MED, standalone

`__predict_med` / `__unpredict_med` in `predict.inc` apply the same gradient
predictor as §5.1 **in place, over a whole plane**, folding each residual
zigzag. This is what runs for predictor 1 when the *main* model is coding —
flags 1 or 9, which only the alt-off interleaved trial in §3.3 produces — and
it walks the plane backwards from the last pixel so each prediction still sees
unmodified neighbours. The first row and column fall back to "the pixel
before", and the last row and column are done separately for the same reason.

So the same gradient rule appears twice in the program, in two roles: inside
model B as a live predictor whose residual feeds the context, and here as a
whole-plane pre-transform handed to the main model. `testfiles/med32.bmp` is
the corpus image that reaches the second one.

### interleave and de-interleave

`__interleave_plane` and the loop at the top of `__colour_transform` move a
plane between the interleaved image and a flat per-plane buffer. In planar mode
(`flags & 0x08`) every plane makes that trip; in interleaved mode the models
walk the interleaved buffer with a stride of `plane_count`.

---

## 8. The decoder

`expand_image.inc` mirrors the above with no decisions of its own: read the
magic and header, refuse anything that is not version 2.0 slow-mode, allocate
from the header's dimensions, and

1. if `0x20` is clear, read `data_size` raw bytes and stop;
2. read the coded stream into memory;
3. if depth ≤ 4 or `0x10` is clear, call `__unmodel_plane` once on the whole
   image with predictor 0 and the main model;
4. otherwise unpack the per-plane descriptors — 6 bits of flags and refs, then
   8 bits of DC and up to three 8-bit weights per plane, all through the same
   bit packer the encoder used;
5. for each plane in `src_plane` order, run the model its flags name, undo MED
   if predictor 1 without the alternate model, and interleave it back;
6. if `flags & 0x02`, transpose;
7. rebuild the greyscale palette if `depth & 0x40`, or read the stored one.

The decoder's per-plane work is the same code as the encoder's — nine
encode/decode pairs in this tree are one `template<int f_DEC>` each, so the two
directions of the counter updates, the context computation and the tree walk are
literally the same lines.

---

## 9. What is in the source and cannot happen

Three things are visible in the decompilation but unreachable in this build,
because the command line is pinned to `-S -Q9`:

- **fast mode.** `desc_slow_mode` is always 1 and the header's `0x04` always
  set. The decoder refuses streams without it rather than implementing it.
- **near-lossless (`-E`).** `near_lossless_q` is pinned to 0 (§5.5), so
  `fold_hi`, the drift checks and the `bucket_size` in `__alt_init_tables` are
  all inert.
- **quality below 9.** `opt_search_quality` is a `constexpr` 9.

That third one needs a caveat the other two do not, and it applies to all six
`opt_*` constants in `globals.inc`: **nothing reads any of them.** Not
`opt_use_filters`, `opt_slow`, `opt_filter_template`, `opt_pack_output`,
`opt_search_quality` or `opt_max_error` — the mode-folding passes replaced every
read with the folded behaviour, and what is left is a record of the settings
this build is pinned to, not a set of switches. Changing one changes nothing.

`tools/deadcheck.py` is what keeps track of code that became unreachable as
those modes were folded away, and `tools/unused.py` of declarations left behind
by it.
