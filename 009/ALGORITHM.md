# BMF 2.01 — the compression algorithms

What this program does to an image, read out of the decompilation in this
directory. The range coder is deliberately out of scope: everything below says
"coded" where a value is handed to `rc` (`rc.inc`), and the only thing you need
to know about it is that it takes either a `(cum, cum+w, total)` triple against
a frequency table or a `(f0, f1, bit)` triple against a binary counter pair.

Names in `code font` are the names in this tree, so every claim here can be
checked against the file it comes from. `tools/unstale.py` checks that much
automatically — it fails if this document names something the program does not
have — and it caught a mistake in this very revision: the file is
`image_compress.inc` and the function in it is `compress_image`, and the first
draft put the file's name in backticks as if it were the function's.

What it cannot check is the *numbers*, and a review of this document against the
program found eight claims that had gone quietly wrong as the tree moved under
them: a descriptor index that still carried a `+1` the array had lost, a
signature described as ten bits that is ten bits plus a bucket, a refinement
said to add four bits that adds three, a counter total described at its old
address, a weight record described at its old shape, "nine encode/decode pairs"
that are now fourteen, a flag byte the corpus table simply had wrong for three
of its rows, and a whole flag — `flags_packed` — that the table did not list at
all though every 1- and 4-bit stream in the corpus carries it. Each is corrected
below and none of them changed the program's behaviour, which is the point: **a
document about an algorithm goes stale in a direction nothing tests.**

Four names were stale in their *spelling* rather than their existence:
`alt_p2_context` and three like it, still written here with the decompiler's two
leading underscores long after the round that took them off every body.
`unstale.py` was passing them on purpose, by a clause written back when the
prefix was on the bodies and not in the documents; with the tree the other way
round, that clause had become the reason nobody noticed. It is gone, and the
check is proven to report when the old spelling is planted back.

There is not one algorithm. There are **three models**, a **decision layer**
that chooses between them per plane, and a **raw fallback**. Which one runs is
decided partly by the BMP subformat and partly by trial encoding.

---

## 1. The container

A `.bmf` holds **one image**. `bmf c in.bmp out` writes it and `bmf d in
out.bmp` reads it back; `file.inc` opens `"wb"` and `"rb"`.

The donor's was an archive — several images in one file, because its command
line took a list of them and appended each to the archive named by `-o`, and
the reader walked the members until one failed to parse. This program takes one
input and one output, so the append, the walk that found the place to append
to, and the member count are gone. What is left of the format is one member,
unchanged: the same magic, the same header, the same coded stream, so a `.bmf`
this writes is a one-member archive as far as the donor is concerned.

The member is:

| bytes | what |
| --- | --- |
| 4 | magic `81 8A 32 30` — `\x81\x8A` then `"20"`, the version. `image_expand.inc` computes `((major<<8) - 12288) \| (minor - 48)` and requires 512, i.e. exactly "2.0" |
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
- `0x40` — greyscale. Set by `bmf_compress` when the palette turns out to be
  an exact linear grey ramp: it walks the entries checking
  `pal[3i] == pal[3i+1] == pal[3i+2] == i * (256 >> depth)`, and if they all
  are, sets `0x40`, clears `0x80` and **the palette is not stored at all**. The
  decoder rebuilds it. For an 8-bit greyscale image that is 768 bytes saved and
  the reason `t8g.bmp` and `t8p.bmp` — same pixels, different palettes — do not
  code to the same size.

`flags` is the mode word:

| bit | name in `bmp.inc` | meaning |
| --- | --- | --- |
| `0x02` | `flags_transposed` | the image is stored **transposed** — rows and columns swapped |
| `0x04` | `flags_slow` | written in "slow" mode (`-S`). This build **refuses** a stream without it: `image_expand.inc` prints `written in fast mode` and exits 3 |
| `0x08` | `flags_planar` | **planar**: each plane was coded on its own. Without it the planes are coded interleaved, in one pass |
| `0x10` | `flags_descriptors` | per-plane descriptors are present in the stream (set for depth > 4) |
| `0x20` | `flags_coded` | the member is **coded**. Without it the payload is the raw pixels |
| `0x40` | `flags_packed` | rows are packed sub-byte. Written by `alloc_image`, **read by nothing** — not by this program and not by the file format; it is on the wire for every 1- and 4-bit stream in the corpus and nothing consults it |
| `0x80` | `flags_tail` | a `CodedTail` sits between the header and the data — the auxiliary block above |

Bit 0 is unused. Two of the seven describe the image (`flags_transposed`,
`flags_packed`), four describe the stream that follows it, and one says whether
anything sits between the two.

### the raw fallback

After coding, `image_compress.inc` compares the coded length against
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

`bmp_read.inc` accepts a 40-byte `BITMAPINFOHEADER` only, with `biPlanes == 1`,
positive dimensions up to 65535, and

- **1, 4, 8, 24 or 32 bits per pixel** — nothing else, and no bitfield formats,
- **`biCompression` 0, 1 (RLE8, 8bpp only) or 2 (RLE4, 4bpp only)**.

Both run-length forms are decoded on the way in: the absolute runs, the encoded
runs, the end-of-line marker, and the delta escape (`00 02 dx dy`) that skips
forward in both axes. RLE4 additionally tracks a nibble phase across deltas.
**The run structure is not preserved.** BMF holds pixels, and `bmp_write.inc`
re-encodes runs with its own splitting when it writes an RLE file back out. That
is why the corpus carries `out_rle4.bmp` and `out_rle8.bmp`: the decoded image
is pixel-identical to the input and byte-different from it, and those two files
are what the decoder is expected to write.

Only bottom-up BMPs are accepted — a negative `biHeight` is refused rather
than read as top-down — and the reader **un-flips them**: it fills the buffer
from its last row backwards while reading the file forwards, so what BMF holds
is top row first, in ordinary raster order, and `bmp_write.inc` flips it back.

Whatever the depth, the image is held as **`plane_count = (depth + 7) / 8`
byte planes, interleaved per pixel**, in `alloc_image`'s single allocation. So 24-bit is 3 planes (B, G, R in file
order), 32-bit is 4, and everything at 8 bits or below is 1 plane — sub-byte
depths are held unpacked, one byte per pixel, which is what lets the same model
handle 1-, 4- and 8-bit images.

---

## 3. The decision layer

Only the encoder runs this; the decoder is told the answers.

### 3.1 depth ≤ 4 takes the short path

`image_compress.inc` checks `(depth & 0x3F) <= 4` and, if so, sets
`plane_predictor = 0`, `plane_alt_model = 0` and calls `model_plane`
directly. **No search, no transform, no predictor** — a 1- or 4-bit image
always goes through the main model, raw. There is one plane and it is small
alphabet; the main model's match and run machinery is what suits it.

Everything below applies to 8, 24 and 32 bits.

### 3.2 `choose_plane_coding` — plane order, colour transform, DC

The descriptors are the stream's whole record of these decisions, and they are
indexed two ways, which is easy to get backwards: **`plane_desc[k]` describes
source plane `k`**, while **`plane_desc[k].src_plane` gives the plane coded
`k`-th**. So a plane that references the others is described in its own slot and
coded last. `t24` codes its planes in the order 2, 1, 0, and it is plane 0 —
coded last, with both others available — that carries the colour transform.

(The array was `PlaneDesc[5]` for several rounds and every access carried a
`+1`, because slot 0 was a header slot rather than a plane: the original parked
`plane_count`, `near_lossless_q` and the archive version in the three weight
words it left free. All three are their own storage now and the slot is gone.)

One field does two jobs, and the format depends on it: **`nrefs` is both the
number of reference planes and the plane's position in the coding order.** They
are the same number — a plane coded `k`-th has exactly the `k` planes before it
available to reference — which is why `read_plane_descs` can write

```c
plane_desc[pl].nrefs = nrefs;       // how many references this plane has
plane_desc[nrefs].src_plane = pl;   // …and therefore where it sits in the order
```

from one six-bit field. Nothing on the wire says the coding order separately.

First the plane order: planes are paired up (`0,1`, `2,3`, …) so a plane can
reference the one before it. Then, for three or more planes, `cost_candidate`
is run over **three** candidate orderings — 0, 1 and 2 — and the cheapest is
kept. This is what decides whether the green plane or the blue plane is the one
everything else is predicted from.

Each candidate writes two things: a row of four costs, and a 16-entry window
describing the ordering it costed. The three rows and the three windows are each
one contiguous table — costs at `acc0[4*cand + …]`, windows at
`tbl16[16*cand + …]` — and the winner is read back by indexing them with the
candidate number. The 64 bytes of the winning window are then copied straight
into `plane_desc`, which is how a candidate becomes the plane order. Every image
in `testfiles/` except `xform1` and `xform2` picks candidate 0; those two exist
because the difference between "the table is contiguous" and "the first row
happens to be where the pointer already points" is invisible until something
picks 1 or 2. See the header of `plane_choose.inc`.

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

`colour_transform` then applies it, in `plane_desc[].nrefs` modes:

```
nrefs == 1   d = x - dc - ref
nrefs == 2   d = x - dc - (w0*refA + w1*refB + 40) / 128
nrefs == 3   d = x - dc - (w0*p[-3] + w1*p[-2] + w2*p[-1] + 63) / 128
```

all in 8-bit wraparound arithmetic, which is what makes it exactly invertible.

### 3.3 `search_filter` — trial encoding

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
counter is `PlaneSearch::n_p2` and it is incremented exactly when a plane
settles on predictor 2, on the reasoning that an image needing the deep model
for one plane will need it for the rest and the cheap candidates are not worth
timing.

So: `0` is skipped once that counter is non-zero; `6` is tried when `5` came
within `cost/32` of the best so far **or** the counter is non-zero; `8` and
`13` only from the second plane onward, since the first has nothing to
reference; `14` only when the winner so far is predictor 2, or `13` came within
`cost/32`.

Then come the **whole-tile trials**, each of which edits every descriptor at
once, re-encodes the tile, and keeps the edit only if it is no worse. There are
five of them and a transpose, guarded as follows.

- **transposed or not.** The tile is rotated — `width`/`height` swapped,
  `flags ^= flags_transposed` — re-encoded, and the transpose is kept if it is
  smaller. An image with strong vertical structure codes better with the rows
  running the other way. The walk stops at the first plane that costs more
  transposed than the per-plane figures said it cost upright, so where it
  stopped is the answer.
- **everything on p1** (`plane_count > 2`, and at least one plane chose
  predictor 1). `offer_to_all(try_p1)` puts `(flags & desc_has_refs) | 5` in
  every descriptor — the reference decision kept, everything else driven by the
  p1 model.
- **everything on p2** (some plane chose predictor 2, *or* the p1 trial above
  won). `offer_to_all(try_p2)` likewise with 6. Two shortcuts skip the
  measurement: if every plane already chose p2 and all but one already
  reference, it is taken unmeasured; and if it wins with all but one plane
  referencing, it is taken as it stands. Otherwise, having won, it gets one
  more trial with `allow_refs_where_present()` — `desc_has_refs` added to every
  plane whose `nrefs` is non-zero — kept only if that is cheaper still.
- **alternate models dropped** (no plane chose predictor 2, more than one
  plane). `drop_alt_model_from_all()` clears `desc_alt_model` and *keeps the
  predictor and the reference bit*. So `13 → 9`, `5 → 1`: the same references
  and the same predictor, but the **main model** coding the interleaved image
  in one pass at full depth. This is the only way a stream ends up with
  predictor 1 and the main model, and it is what `testfiles/med32.bmp`
  exercises.
- **everything cleared** (same guard, and some plane either references or chose
  p1). The tile's pixels are restored from a copy taken before the previous
  trial, `clear_flags_on_all()` writes 0 into every descriptor, and the result
  is main model, no predictor, no references — the plainest coding there is,
  measured last because the trials above it are destructive.

One subtlety that changes which trials run: on the path where the p2 trial wins,
the counter that records "some plane chose predictor 2" is **overwritten with a
bit count** — `n_hard = bits_a`. Since a bit count is never zero in practice,
the two `!n_hard` trials below are then skipped. Reading the counter as a
counter after that point is a mistake the name invites.

The return value becomes header bit `flags_planar`: set means planar, clear
means the whole interleaved image goes through one model pass.

### 3.4 what the corpus actually chooses

Measured, by counting which model entry points the *decoder* enters for each
reference stream — so this is what the final stream uses, not what the search
tried:

The flag byte is read straight out of `testfiles/ref_<name>.bmf` at offset 15.

| image | BMP subformat | header flags | what codes it |
| --- | --- | --- | --- |
| `t1`, `f05_200` | 1 bpp | `0x64` packed | main model, whole image |
| `DLRAW`, `rle4` | 4 bpp, one RLE4 | `0x64` packed | main model, whole image |
| `x_ci` | 8 bpp RLE8 | `0x3c` planar | main model, whole image |
| `x_ai` | 8 bpp RLE8 | `0x3e` planar, transposed | main model, whole image |
| `t8g`, `t8p`, `rle8` | 8 bpp (grey, palette, RLE8) | `0x3c` planar | **p2**, single plane |
| `t24` | 24 bpp | `0x3c` planar | plane 0 → main model + transform; plane 1 → **p1** + transform; plane 2 → **p1**. Coded in the order 2, 1, 0 |
| `t32` | 32 bpp | `0x3c` planar | as `t24`, plus plane 3 → **p1**. Three planes p1, one main model |
| `med32` | 32 bpp | `0x36` interleaved, transposed | **MED + main model**, one interleaved pass |
| `altp1` | 32 bpp | `0x36` interleaved, transposed | **p1 interleaved**, all four planes per pixel |
| `x_ep` | 32 bpp | `0x36` interleaved, transposed | **p2 interleaved** |
| `xform1` | 32 bpp | `0x3c` planar | picks plane-order candidate 1 or 2, which nothing else does |
| `xform2` | 32 bpp | `0x36` interleaved, transposed | likewise |
| `noise24` | 24 bpp | `0x00` | raw — nothing compressed it, and the whole flag byte is zero because a raw member writes the *image's* header rather than the one the coder filled in |

Three things this makes concrete. **The choice is per plane, not per image**:
`t24` codes one plane with the main model and two with p1, in the same stream.
**The subformat does not determine the model** above 4 bits — `t8g` and `x_ci`
are both 8 bpp and land on different models, because the search measures them
rather than classifying them. And, read off the files rather than remembered:
**exactly the four sub-byte images carry `flags_packed`** and no other stream
does, which is `alloc_image` recording `sub_byte_rows` on a bit that nothing
afterwards consults.

One property worth knowing: `choose_plane_coding` sets `alphabet_reduced = 1`
and `compress_image` sets it back to `0` before the real encode. That flag
gates the p2 model's neighbour-context updates (§6.3), so **the search measures
a cheaper model than the one that finally runs.** The costs it compares are
consistent with each other, not with the final stream.

---

## 4. Model A — the main model

`model.inc`, driven by `plane.inc`. It runs for every image at 4 bits or
below, for every plane the search leaves at flags 0 or 8, and for the whole
interleaved image when the alt-off trial of §3.3 wins — in that last case at
the image's full depth, so a 32-bit image is one pass over 4-byte symbols
rather than four passes over bytes. It is a context-mixing coder
built around *matching neighbours* rather than predicting values, which is why
it suits palette images: symbol 200 and symbol 201 have nothing to do with each
other, so arithmetic on them is meaningless, but "the same as the pixel above"
is meaningful at any depth.

### 4.1 alphabet reduction

`sym_reduce.inc` runs first. It scans the plane, marks which of the 2^depth
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

`mode_symbol[1] = N`, `[2] = W`, `[3] = NE`, `[4] = NW`, and the same four come
back from `rank_neighbours()` as a `Neighbours{ up, left, up_next, upleft }`
whose `rank()` numbers them 1..4 with 0 for "none of them".

Alongside each symbol, every `PixRec` carries six `match[]` flags. **These are
the model's texture bits, and every context signature in §4.3 is built from them
rather than from symbol values** — which is the whole reason the model works on
a palette, where symbol 200 and symbol 201 mean nothing to each other. They are
written once per pixel, by `init_tables` (§4.4), and each says whether this
pixel equalled one particular neighbour:

| lane | this pixel equalled |
| --- | --- |
| `match[0]` | N — the pixel above |
| `match[1]` | W — the pixel before |
| `match[2]` | NE |
| `match[3]` | NW |
| `match[4]` | two right and one up |
| `match[5]` | three right and one up |

Lanes 4 and 5 have no short name because they are not neighbours of *this*
pixel: they are neighbours the row above will have when the cursor gets there,
recorded now so the run scan and the gradients can read a whole span of them
without recomputing.

### 4.3 the per-pixel ladder

`ModelBlock::code_pixel` is one pass down a ladder, stopping at the first rung
that succeeds.

**Rung 1 — run mode.** If the current pixel's west neighbour matched on both
its flags *and* ten `match` flags across the two rows above are all set, the
model is in a flat region — that conjunction is nine records across three rows
and both coders spell it out identically.

`run_scan` then measures how far the flat region reaches: how far the north
row's match flags run on from here, capped by what is left of the row, and the
AND of the row above that one's `match[0]` over the same span. It answers the
length and leaves `run` one behind it — the last index whose flags were tested,
which is what the escape context indexes with.

A **binary "did the run reach the end"** decision is coded through `esc_ctr[]`
at

```
8·run_bucket[len] + 4·(two rows up, the two records past the run both matched)
                  + 2·(the north-north AND) + alpha_map[N] + 1
```

If it did, the whole scan length is taken at once. If it did not, the run is
shorter than the scan — and when the scan only reached 1 the answer is 0 and
costs no further bits. Otherwise the length is coded by
`ModelBlock::code_run_length<f_DEC>`: **one bit per bucket level
from the top down**, and a bit is coded only at a level that could still belong
to a run shorter than the cap. The counter is
`run_ctr[16·((first bit at this level) + (this is the first turn)) + bucket]` —
`run_ctr[48]` is three groups of sixteen, and the group says whether any higher
bit has been set yet and whether the bucket is still the one the scan started
in. Both coders wrote this walk out; it is one `template<int32_t f_DEC>` now,
differing only in where the bit comes from.

`fill_run` then writes the symbol, head word and flag word of the record before
into every record the run covers, `seed_after_run` primes the one past it, and
the length is what `code_pixel` returns to the driver, which advances `x` by
it.

**Rung 2 — the four-way match.** Otherwise the pixel is coded against a
five-symbol alphabet: *escape, N, W, NE, NW*, with weights `w[0..4]` and total
`w[5]` in a `FreqRec` picked by a context bucket.

Everything both coders work out before they part is `open_pixel`, and it is
worth following exactly, because the widths matter:

1. `rank_neighbours()` fills a `Neighbours{ up, left, up_next, upleft }`, and
   `Neighbours::rank(sym)` answers where a symbol sits among them: 1..4 for the
   four, **0 for the escape**.
2. `match_context` builds a **six-bit** signature out of match flags alone — no
   symbol values:

   | bit | flag |
   | --- | --- |
   | 5 | `W-1` matched NE |
   | 4 | `W-1` matched `up[2]` |
   | 3 | `N+1` matched W |
   | 2 | NE equals NW |
   | 1 | `W-1` matched N |
   | 0 | N matched its own N |

   `ctx_state[64]` folds those 64 signatures into **fifteen** groups.
3. `pair_key` is the key into the symbol-pair tables: the north neighbour's
   `sym_rev[]` entry **less whichever of W, NE, NW first disagrees with it** —
   and less the second neighbour west when all four agree.
4. The bucket is `ctx_bucket[state + 15·rank(pair->last) + 75·rank(pair->prev)]`
   — where `pair` is `group_ctr[state][key]`, the last and previous symbols seen
   at this exact state and key. Fifteen states times five times five is the 375
   entries `ctx_bucket[]` has.

The frequencies are rescaled by halving when the total passes `w[6]` or 0x4000,
with a floor that keeps the escape reachable. There are two `FreqRec`s in play:
a per-bucket one and a per-context one, and when the latter is fresh
`blend_from` seeds it from the former — the incumbent weights scaled by the
donor's increment, the donor's own weights put in their place, and 21 parts of
the donor's total added back rounded up. A new context gets a prior instead of a
uniform start.

Under the bucket sits a chain of **lazily allocated context identifiers**,
`intern_ctx` minting a dense id per distinct signature in first-seen order with
`0xFFFF` for "not yet":

- **`ctx_id1`** is indexed by `sig1`, which is *not* ten bits — it is ten bits
  **plus the bucket above them**: four "is this gradient flat" bits at 4..7, two
  "did four rows above all match" bits at 8..9, four more match bits at 0..3,
  and `bucket << 10`.
- **`ctx_id2`** refines `id1` with **three** more match bits: `sig2 = 8·id1 +
  3 bits`.
- **`ctx_id3`** refines that with the **low nibble of W** — `sig3 = 16·id2 +
  (W & 15)` — but only when the alphabet is under 32 symbols. Past 53248 ids the
  nibble is forced to 15, which merges those sixteen contexts into one rather
  than allocating past the table.

Each level pays only for contexts the image actually visits, which is what makes
a 629k-entry chain affordable on a small image.

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

**Rung 3 — the escape ladder.** On escape, `seed_candidates` assembles 32
candidate symbols in slot order:

| slots | from |
| --- | --- |
| 0, 1 | `group_ctr[state][key]` — the last and previous symbol seen at this exact state and key |
| 2 … 9 | `sym_ctr[8·key]`, the eight-entry per-key symbol cache |
| 10 … 31 | twenty-two spatial neighbours, `load_neighbours` |

The neighbours spiral outward from the pixel, and the order is the order
`pixel_context` reads them in, not raster order. Writing `r5` for the current
row's cursor and `r6`…`r9` for one to four rows up:

```
10 r5[-2]  11 r6[+2]  12 r7[+1]  13 r7[ 0]  14 r6[-2]  15 r7[-1]
16 r5[-3]  17 r6[+3]  18 r6[+4]  19 r5[-4]  20 r6[-3]  21 r7[+2]
22 r8[ 0]  23 r7[-2]  24 r5[-5]  25 r8[+1]  26 r6[+5]  27 r9[ 0]
28 r5[-7]  29 r8[-1]  30 r6[+7]  31 r7[+3]
```

Slots 11–15 are the **near band** and 16–31 the **far band**, and those two
bands are what the candidate is scored against.

Each candidate goes through `pixel_context`, which returns −1 to skip it in
three cases:

- it is already in the **exclusion mask** for this pixel;
- it is in neither band and the ladder is past position 6 — no support, not
  worth a bit;
- it is past position 14 and the three-bit list evidence has neither of its two
  strongest bits set.

For a candidate worth asking about, a **binary decision** is coded through
`bit_node[]` against `bit_root[]`, at a context built in three layers: bit 6 is
"this is slot 10, the immediate neighbour", bits 5 and 4 are the near and far
band hits, bit 3 is "the candidate is in the top ten of `sel0_list[N]`", and
bits 2..0 ask the same of `sel0_list[W]`'s top four, of the candidate's own list
holding W in its top ten, and of `sel0_list[NE]`'s top six. The ladder position
goes in at bit 7 and above.

A hit ends the pixel; a miss adds the candidate to the exclusion mask, so it
costs nothing in every later step and in rung 4.

**Rung 4 — the symbol lists.** If all 32 miss, three `SymList`s are searched in
order: the list of symbols seen after this W, the list seen after this N, and
finally the dense escape list holding the whole alphabet. Each is a
move-to-front frequency list (§4.5) and each codes an escape if the symbol is
not in it. The last one always has it.

### 4.4 what the driver does between pixels

`ModelBlock::model_plane_slow` walks rows and columns; `start_row` rotates the
five row
buffers one step (`ring_advance`) and seeds the fresh row's match flags against
symbol 0; and `init_tables()` runs after **every** pixel. That last one is where
the model learns, and it does seven things in order:

1. **Feed the selector lists.** The two per-symbol `SymList` tables learn the
   symbol just coded under the neighbours that would have to find it next time:
   `sel0_list[N].add_weight(sym, 3)`, `sel1_list[N].add_weight(sym, 4)`,
   `sel0_list[sym].add_weight(W, 2)` and so on. Which set of weights depends on
   how the pixel ended. On an escape with the selector cursor still at the
   start, four of them; on an escape with it moved, four different ones **plus**
   an insert into every list the search walked past — the symbol at count 2,
   swapped one place forward, so the next pixel in that context finds it early.
   On a *hit* below the top two ranks, with NE and NW differing, just
   `sel0_list[W].add_weight(sym, 1)`.
2. **Bump the exclusion generation**, on the escape path only. The mask is a
   generation counter rather than a cleared array; when the counter would wrap
   at −1 it is reset to 1 and the whole mask zeroed instead.
3. **Move-to-front over the eight-entry symbol cache** — unless the symbol was
   already in the top two, which is what `promoted` records. Only slots 1..6 are
   searched, and a miss lands on slot 7, so "not found" and "found at the end"
   are the same store.
4. **Roll the context pair**: `pix_cur->prev = pix_cur->last; pix_cur->last =
   sym`. That is what the next pixel at this state and key reads as its two most
   recent symbols.
5. **Set the six `match[]` flags** on the record just coded — `sym == N`,
   `== W`, `== NE`, `== NW`, `== up[2]`, `== up[3]`. Everything the context
   machinery above reads is built from these, and this is the only place they
   are written.
6. **Advance all five row cursors** by one.
7. **Slide the four `grad[]` accumulators.** Each is a running "how many of the
   last *n* records matched on this lane, less *n*" — so **0 means all of them
   matched**, which is exactly what `sig1`'s four flat bits test. Sliding one
   step is one add and one subtract per gradient, over `match[0]` of the two
   rows above and `match[1]`/`match[0]` of the current row.

Steps 5 to 7 are why the model is affordable: the context signatures are read
from bytes that were computed once, not recomputed per lookup.

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

Lists come in two flavours: **dense** (`SymList::init(n, 1)`) starts with
every symbol present at count 1, and **sparse** starts empty and grows.

---

## 5. Model B — the alternate model for predictor 1

`alt_p1_block.inc` holds the block and its context. Selected by flags 5 or 13. This is
a *continuous-tone* model: it predicts a value, codes the residual, and
everything is arithmetic on pixel magnitudes.

It has **two drivers**, and which one runs is the planar/interleaved decision of
§3.3:

- `alt_model_p1_d8_encode` and `alt_model_p1_d8_decode` — **one plane**, one
  `AltP1Block`, called once per plane in planar mode, and one
  `AltP1Block::d8_body<f_DEC>` between them. This is what `t24` and `t32` use.
- `alt_p1_code.inc`'s `alt_model_p1` — **all planes at once**, one
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

Each is 0, 1 or 2, and the nine are combined as a **mixed-radix base-3 index**.
`CtxWeight` holds `w[3]` per feature and the index is `Σ ctx_w[k].w[sel_k]`,
with the three values of feature *k* pre-scaled to `32 · 3^k` — which is why
`counters[]` has 629856 = 32 · 3^9 entries. The eight-way activity level goes in
the low bits, so contexts differing only in activity are **adjacent**, which is
what lets the update step (§5.4) nudge the neighbouring levels.

Two more binary features share those low five bits with it, and they are easy to
miss because they are added rather than folded through a `CtxWeight`:

```c
ctx[0] = Σ ctx_w[k].w[sel_k]  +  16·quiet  +  8·(ctx[3 + ctx[2]] == 0)  +  act_lvl;
```

`quiet` is "the neighbours this plane can see are all still" — the magnitudes of
the reference planes' west samples under 16 (two references), under 8 (one), or
every one of five own-plane magnitudes exactly zero (none). The `8·` term is a
lookup into the block's own small state array. So the low five bits are
`act_lvl` (0–7) plus two flags, not the activity level alone.

`ctx_of` also returns a **second** index, `ctx[1] = group_of[act_q] +
slot_of[pred]`, which does not select a counter at all — it selects the binary
tree the escape walks. See §5.3.

### 5.3 coding a residual

The residual `sample − pred` is **zigzag folded** by `alt_init_tables`:
0 → 0, −1 → 1, +1 → 2, −2 → 3, +2 → 4, … so small residuals become small
codes. The fold and unfold tables are exact inverses.

The folded code is coded in two stages:

1. **`CounterNode::code_symbol`** codes the code's *slot* against a seven-way
   frequency array — `c[0..4]` are the codes 0–4 directly, and `c[5]`/`c[6]` are
   "an odd code ≥ 5" and "an even code ≥ 5": `slot = 6 - (sym & 1)`, so an odd
   `sym` lands on 5 and an even one on 6.

   The node's `total` is read as `total & 0x7FFF`: **bit 15 is a marker that the
   array has been rescaled at least once**, and rescale sets the new total to
   `0x8000` plus the counts so the marker survives. Rescale triggers above
   0x2000 and picks its divisor from the *smallest* count in the array — by
   **halves** if that has fallen to 1 or 0, and by **thirds** otherwise.
   Rescaling by thirds when something is already at 1 would round it away. The
   coded slot is then bumped by 32, and so is the total.

   (In the decompilation this was a bare `uint16_t*` where index 0 was the total
   and 1..7 the counts, which is why the total looked like `freq[0]`. It is a
   `total` member beside a `c[7]` array now, and the `& 0x7FFF` is the only
   thing left of the punning.)
2. Slots 5 and 6 **escape into a binary tree**: `code_symbol_tree` walks
   `level_geom[]` — a table of per-level spans laid out by `rc_begin` — coding
   one bit per level through `FreqPair` counters, with the level itself chosen by
   `model_geometry[code]`. The tree is what makes large residuals cost
   logarithmically rather than linearly.

   **Which tree**, though, is the second context, and it is the one thing §5.2
   builds that the account above leaves out. `ctx_of` returns *two* values:
   `ctx[0]`, the counter index, and `ctx[1] = group_of[act_q] + slot_of[pred]`,
   which is handed to `code_symbol` and reaches `model_strip` as
   `128·(slot & 1) + <a term folded off the counts> + ctx`. So the parity picks
   one of two strips, the activity group and the predicted value pick the place
   in it, and only then does the tree code the magnitude — `(sym - 5) >> 1`
   going out, `slot + 2·tree` coming back.

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
`alt_init_tables` builds `fold` as the exact inverse of `unfold`, `recon`
always equals `want`, the drift is always 0 and the `fold_hi` branch cannot
fire. It is the `-E` near-lossless path, kept because it is what the donor does.

---

## 6. Model C — the alternate model for predictor 2

`alt_p2_block.inc`, `alt_p2_context.inc`, `alt_p2_model.inc`, `alt_p2.inc`. Selected by
flags 6 or 14, and the most expensive of the three — `alt_p2_model.inc` is the
largest body in the program. Where model B predicts with a fixed rule, this one
**learns its predictor**.

It has the same two drivers as model B: `AltP2Block::alt_p2_d8_body` for one
plane at a time (what `t8g`, `t8p` and `rle8` use — a single 8-bit plane is
this model's home ground), and `alt_p2_code.inc` for all planes at once
(`x_ep`).

### 6.1 an NLMS linear predictor

Each pixel carries a `P2Ctx` record: eight 16-bit fields — `val`, `dval`, `err`
against the prediction, `aerr` its magnitude, and the four differences `dleft`,
`dup`, `dupleft`, `dupright` — plus two bytes: `sign`, a ternary
`(resid <= dead) + (resid < -dead)` saying which side of the deadzone the
residual fell, and `mag`, its absolute value — those two are what the *context*
quantiser of §6.2 sums over, not the 16-bit fields. Those become a
**7×4 feature matrix** `p2_row[7][4]` built from the neighbourhood.

The prediction is `bias + Σ w[j][k] · p2_row[j][k]` over all 28 terms, and after
each sample the weights are updated by **normalised least mean squares**:

```
err = sample - pred
ms[j][k] += (x*x - ms[j][k]) * 0.023          // running mean square of the input
w[j][k]  += rate[j][k] * err * x / (ms[j][k] + floor)
```

with per-position rates from `bmf_p2_rate[7][4]` (0.0108 down to 0.0009 — the
nearest neighbours adapt fastest) and a floor that keeps the division stable.

Weights live in `nb_weights[1088]`, each an `NbRow` of `float w[15][4]` and a
use count: rows 0–6 are the weights, 7–13 the running mean squares, 14 the scale
and confidence, and the use count sits where row 15's first slot would be. (The
record was a bare `float[16][4]` in the decompilation and a `static_assert`
keeps it that size, because `NbRow::predict` is handed `p2_row` and writes one
row *past* the end of it — see below.)

**The rate and coefficient tables are not constants.** `P2Coef::fold` runs at
the top of an image and `restore` at the bottom, and between them the model has
edited `p2_coef` and `p2_rate` in place: rows 4–6 of the coefficients —
the cross-plane terms — are folded into rows 0–2 and zeroed, and three rate rows
are set to `bmf_p2_rate_reset`. So a single-plane image and a multi-plane one
are running different filters out of the same table, and reading the table's
declared values tells you what the *interleaved* path uses, not the `d8` one.

Those two are members of `BMFState`, seeded in `reset` from `bmf_p2_coef_init`
and `bmf_p2_rate_init`, which are the genuine constants. They were file-scope
arrays until the codec became a class, and that is what made the borrow above a
*shared* session: `tools/parallel.cpp` under ThreadSanitizer caught `fold`
writing what another codec's `nb_dot` was reading.

One more shape worth knowing because the code depends on it: `NbRow::predict`
takes `p2_row` as its `nb` argument and writes `nb[7][0..2]` — one row past the
end of a `float[7][4]`, which lands in `bias[]` declared immediately after it.
That is how the filter returns four things (an `int32_t` and three floats)
through a signature that names one. The adjacency was guaranteed by a union in
the decompilation and is asserted with `offsetof` now.

### 6.2 which weight set

Not one predictor but **1088 of them**, selected by a quantised description of
the local texture. `alt_p2_context` accumulates four directional activity sums
— up-left, up-right, left, up — each a fixed-weight combination of eight
neighbours' difference fields, and from them derives:

- `band` — five thresholds on `sum4` (the left sum) against `sum_u` (the up
  sum), so 0–5 says how horizontal or vertical the neighbourhood is;
- `gA` — total activity against four thresholds;
- `gB` — a local value estimate against three;
- `gC` — the up-right/up-left ratio against three;
- `gD` — a four-neighbour average against three.

All the thresholds come from `bmf_p2_thresholds[band][13]`, i.e. **the
quantiser itself depends on the band**. The slot is
`320*band + 64*gA + 16*gB + 4*gC + gD`, and `nb_id[]` maps slots to weight sets
lazily, allocating on first use.

A second linear stage, `NbRow::predict`, blends **six** weight sets — the six
pointers in `CtxWeights::row`, each a 7×4 matrix like the one above — using the
six coefficients of `bmf_p2_mix[mode]`, one row of a 4×6 table. `p2_coef`
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

The residual is folded (§5.3) and coded through `P2Freq`, a **three-way
counter** — "the symbol", "escape odd", "escape even" — whose escapes go into
the same
binary level tree model B uses. Each of the three is bumped by *sixteenths* of
the record's `step`, and the file uses exactly eight numerators: 2, 3, 4, 5, 6,
7, 10 and 13.

`step` is the thing that decays as a context settles, and its ladder is worth
writing out because the middle rung is not monotonic:

| `step` | what `rescale_three_way` does to it |
| --- | --- |
| > 256 | halve |
| 33 … 256 | subtract 32 |
| 17 … 32 | **add 2** — `((16 - step) >> 30) & ~1` is −2 here, and the subtraction of it is an addition |
| ≤ 16 | leave it |

So a settling context walks down in steps of 32, bounces in a narrow band around
32 for a few rescales, and comes to rest at 16 or below, which is a fixed point.
The three counts halve on every rescale regardless, and `rescale` fires when
they total more than 29696.

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
what `choose_plane_coding` disables while the search is running (§3.3).

---

## 7. Predictors and transforms outside the models

### MED, standalone

`predict_med` / `unpredict_med` in `plane_predict.inc` apply the same gradient
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

`interleave_plane` and the loop at the top of `colour_transform` move a
plane between the interleaved image and a flat per-plane buffer. In planar mode
(`flags & 0x08`) every plane makes that trip; in interleaved mode the models
walk the interleaved buffer with a stride of `plane_count`.

---

## 8. The decoder

`image_expand.inc` mirrors the above with no decisions of its own: read the
magic and header, refuse anything that is not version 2.0 slow-mode, allocate
from the header's dimensions, and

1. if `flags_coded` is clear, read `data_size` raw bytes and stop;
2. `expand_coded` reads the whole coded stream into memory and answers false if
   the file is short or the stream does not end exactly where its length said;
3. if depth ≤ 4 or `flags_descriptors` is clear, call `unmodel_plane` once on
   the whole image with predictor 0 and the main model;
4. otherwise `unmodel_described` reads the near-lossless nibble, then
   `read_plane_descs` unpacks one descriptor per plane — 6 bits of flags and
   refs, then 8 bits of DC and up to three 8-bit weights, through the same bit
   packer the encoder used (§8.1);
5. `unmodel_planes_apart` (planar) walks `plane_desc[k].src_plane` in order,
   decoding each plane through a header that says one eight-bit greyscale
   plane and interleaving it back; `unmodel_planes_together` (interleaved)
   decodes the whole image in one pass and then, unless the alternate model
   coded it, takes each plane out, undoes its predictor and puts it back;
6. if `flags_transposed`, transpose;
7. rebuild the greyscale palette if `depth & 0x40`, or read the stored one.

The decoder's per-plane work is the same code as the encoder's — **fourteen**
encode/decode pairs in this tree are one `template<int32_t f_DEC>` each, so the
two directions of the counter updates, the context computation and the tree walk
are literally the same lines. `bmf.cpp`'s header lists them, with the two that
stay separate and why.

### 8.1 the bit packer, which is not the range coder

The descriptors are not range coded. They go through `pack_bits`/`unpack_bits`
in `rc_io.inc`, a plain LSB-first bit packer over 32-bit words, into the **same
buffer** the coded stream will use — so a member's payload is: the
near-lossless nibble, the descriptors, and then the range coder's output.

The two meet at `packer_rewind`. The packer writes whole 32-bit words and the
coder starts at a byte, so before the first symbol is coded the output cursor is
backed off over whatever whole bytes the last word did not need, one at a time,
leaving `stream.pk.free_bits` holding what is still owed. It goes transiently
negative on the way, which is the thing to know about it. On the way back in,
`packer_word` refills the accumulator a word at a time from the same cursor.

That is why `compress_image` can measure a coded stream by `stream.cur -
stream.buf` and get a byte count that includes the descriptors: there is one
cursor and both writers advance it.

---

## 9. What is in the source and cannot happen

Three things are visible in the decompilation but unreachable in this build,
because the command line is pinned to `-S -Q9`:

- **fast mode.** `desc_slow_mode` is always 1 and the header's `0x04` always
  set. The decoder refuses streams without it rather than implementing it.
- **near-lossless (`-E`).** `near_lossless_q` is pinned to 0 (§5.5), so
  `fold_hi`, the drift checks and the `bucket_size` in `alt_init_tables` are
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
