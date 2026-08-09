# BMF 2.01 — the compression algorithm

Read out of `subs1.hpp` as it stands, not out of the original binary and not out
of `ALGORITHM.md`. Where the two disagree, this one was checked against the
code; where this document is unsure it says so, and §11 lists what it could not
establish at all.

The range coder is deliberately out of scope. It appears here only as four
operations — `encode(cum, high, tot)`, `encode_bit(c0, c1, bit)`, and their
decode halves — and everything below is about what gets handed to them.

One caveat that shapes the whole document: **this source is BMF with its six
mode switches folded to constants** (`REFACTORING.md` §2.1). `-S` is on, `-F`
is on, `-Q` is 9, `-E` is 0, and the code for the other settings is deleted. So
this describes the algorithm *this program runs*, which is BMF's slow, filtered,
lossless mode. Where a decision is made by a constant rather than by data, it is
marked.

---

## 1. The shape of it

BMF is a lossless still-image compressor. The pipeline, for one image:

```
  BMP file
    │  read_bmp
    ▼
  image descriptor + interleaved pixels          §2
    │  compress_image
    ├─ choose per-plane coding                   §5   (cost = §4)
    ├─ search for a filter                       §6
    ▼
  model_planes: for each plane p
    ├─ colour_transform  — de-interleave, and subtract a linear
    │                      combination of one or two other planes  §3
    ├─ predict_med       — MED spatial prediction, residuals folded
    │                      into an unsigned alphabet               §7
    └─ model_plane       — entropy-code the plane                  §8, §9
    ▼
  range-coded payload
```

Two things are chosen per plane and written into the stream: **which other
planes it is predicted from** (§3) and **which model codes it** (§5). Everything
expensive in the encoder is the search for those choices.

---

## 2. Container and descriptor

### 2.1 The archive

A `.bmf` file is a sequence of members with no index. `bmf c` opens the output
`"a+b"` and appends; `bmf d` walks members until EOF. Compressing twice to one
path makes a two-member archive, and the decoder writes each member in turn —
this is not a quirk to be tidied away, and `test.sh` pins it.

Each member is

| bytes | |
| --- | --- |
| 4 | magic `81 8A 32 30` |
| 16 | the image descriptor, §2.2, with `data_size` replaced by the payload length |
| n | the coded payload |

Verified against the corpus: `t8g.bmf` is 42 896 bytes with `7C A7 00 00` at
+12 — 42 876 + 4 + 16.

### 2.2 The image descriptor — `BmfImage`

Sixteen bytes, then the pixels. `alloc_image` writes all four of its words in
one place, which is what fixes the layout:

| offset | | |
| --- | --- | --- |
| +0 | `uint16 width` | |
| +2 | `uint16 height` | |
| +4 | `uint32 stride` | bytes per row, rounded up per depth |
| +8 | | never read |
| +10 | `uint8 depth` | bits 0–5 the depth; 0x80 = a palette follows the payload; 0x40 = the palette was a grey ramp and was dropped (§2.4) |
| +11 | `uint8 flags` | mode bits, §2.3 |
| +12 | `uint32 data_size` | `stride * height`; in the stream, the payload length |
| +16 | | pixels, and the palette after them at `+16 + data_size` |

`read_bmp` accepts 1, 4, 8, 16, 24 and 32 bits per pixel, with a palette below
9; the corpus covers all but 16. The *writer* refuses depths 2, 15 and 16,
because BMF sent those to a TGA writer this build does not have — so a 16-bit
BMP compresses to a stream nothing here can expand.

### 2.3 The flags byte

The byte that goes into the stream is assembled in `compress_image`'s frame
rather than in the descriptor — the four slots it fills *are* the sixteen bytes
it writes — so what follows is what that assembly sets, and no more:

* **bit 2** — slow mode, and **bit 5**. Both set together by `|= 0x24`, and
  always set here, since `-S` is a constant.
* **bit 4** — set by `|= 0x10` after the filter search runs (§6).
* **bit 1** — **the plane data is stored transposed.** `search_filter` toggles
  it when coding the image with rows and columns exchanged costs less, and both
  `expand_image` and the raw-store path toggle it back while undoing the
  transpose.
* **bit 7** — set from `compress_image`'s fifth argument, which is 1 whenever
  the caller passed a coded buffer.

Observed in the corpus: `3C` for a photographic 8-bit or 24-bit image, `64` for
a palette image that fell back to raw. **Bit 3 is set in both and this document
did not find where** — it is not in the `|= 0x24` / `|= 0x10` assembly and not
in any write to the descriptor's `+11`, so it arrives in the frame slot from
somewhere this reading missed. §11.

### 2.4 Grey palettes are dropped

Before the encoder runs, `bmf_compress` looks at the palette of any image that
has one. It walks the entries and asks whether they are exactly the grey ramp

```c
step = 256 >> depth;
palette[i] == (r, g, b) == (i·step, i·step, i·step)   for every i
```

If they are, it sets 0x40 in the depth byte and clears 0x80: the image is coded
as greyscale and **the palette is not stored at all**. If they are not, 0x80
stays and the palette is written after the payload, three bytes per entry.

This is why `t8g.bmp` — an 8-bit BMP with a grey palette — has `48` at +10 and
no palette in its stream, while `t1.bmp` and `rle4.bmp` have `81` and `84` and
carry theirs. It is a real saving on exactly the class of image where a palette
is pure overhead, and it costs one pass over at most 256 entries.

(There is a third case in the same branch: an image that arrives with both bits
already set has 0x80 cleared and nothing else done.)

### 2.5 Planes

```
plane_count = ((depth & 0x3F) + 7) >> 3
```

So 8 bits and below are one plane, 24-bit is three, 32-bit is four. The pixels
are interleaved by plane with stride `plane_count`, and `colour_transform` is
what de-interleaves them.

---

## 3. Inter-plane decorrelation

`colour_transform(image, dst, p, ·)` produces plane `p` as a separate byte
array, and while doing so subtracts a prediction made from other planes. Three
cases, selected by the per-plane header (§5.1):

**No reference** — bit 3 of the mode nibble clear. A plain de-interleave:

```c
for (i = 0; i < w*h; i++) dst[i] = pixels[i * plane_count];
```

**One reference** — reference count 1:

```c
dst[i] = plane[i] - bias - ref[i];
```

**Two references** — reference count 2, with two 7-bit weights:

```c
dst[i] = plane[i] - bias - ((w0 * ref0[i] + w1 * ref1[i] + 40) >> 7);
```

where `w0 + w1 == 128` (the code special-cases exactly that sum), the `+ 40`
is the rounding term, and both weights are stored biased by 64. `bias` is the
8-bit constant from the header.

This is a **general linear inter-plane predictor**, not a fixed colour
transform. There is no RGB→YCoCg matrix anywhere in the file; what there is, is
a per-plane choice of up to two reference planes and a pair of weights, searched
for by §5. A green-from-blue-and-red prediction is one point in that space and
the search will find it on a photograph, but it is not built in.

---

## 4. The cost metric

Every choice in §5 and §6 is made by the same measure. `estimate_cost(hist, n)`
computes, over the non-zero bins,

```
S = Σ h[i]
T = Σ h[i] · ln h[i]
cost = (S·ln S − T) · log₂e          bits
```

which is `Σ h[i]·log₂(S / h[i])` — the **zeroth-order empirical entropy of the
histogram, in bits**. Nothing about the actual model enters it. It is a proxy,
and a cheap one: the histogram it measures is the one `predict_med` fills in as
a side effect of predicting (§7), so a candidate costs one prediction pass.

`cost_candidate` wraps it: build the residual histogram for one candidate
configuration, call `estimate_cost`, return the bits.

---

## 5. Choosing how to code each plane

`choose_plane_coding` (867 lines) is the search. For every plane it decides:

* the spatial predictor (2 bits),
* whether the alternate model handles it (1 bit),
* whether it is predicted from other planes, and from which, and with what
  weights (§3).

It costs candidates with §4 and keeps the cheapest. The candidate set is built
from plane pairs — the initialisation walks `i < plane_count/2` writing plane
`2i` and `2i+1` into the reference tables — so the search is over pairings of
planes rather than over all subsets.

### 5.1 The per-plane header

Written by `compress_image` through the bit packer, read back by `expand_image`.
Per plane, in order:

| bits | |
| --- | --- |
| 6 | `(mode << 2) | refcount` |
| 8 | bias — only if mode bit 3 is set |
| 8 + 8 | two weights, each biased by 64 — only if `refcount > 1` |
| 8 | a third weight — only if `refcount > 2` |

and the mode nibble is

| bit | |
| --- | --- |
| 0–1 | spatial predictor: 1 and 2 are the two live values |
| 2 | use the alternate model family (§9) |
| 3 | this plane has an inter-plane reference |

The decoder sets `plane_predictor = mode & 3` and
`plane_alt_model = (mode >> 2) & 1` from these, and those two globals are what
`model_plane` dispatches on.

---

## 6. The filter search

`search_filter` (837 lines) runs before the per-plane coding and sets flag bit 4
when it applies something. `-Q` is 9, which in BMF selected the most thorough
search; with the other quality levels folded away, what remains is the full one.
It costs candidates with §4 and calls `transform_planes` to apply the winner.

What the filter *is* — the family of transforms searched over — is not
established here. `search_filter` and `choose_plane_coding` share their cost
machinery and their candidate loop shape, and the filter is applied to all
planes at once by `transform_planes`, which then calls `model_plane` per plane.

---

## 7. Spatial prediction

`predict_med` is exactly the **MED predictor of JPEG-LS / LOCO-I**, on the three
causal neighbours `W` (left), `N` (above) and `NW` (above-left):

```
if NW ≥ max(W, N):   pred = min(W, N)
if NW ≤ min(W, N):   pred = max(W, N)
otherwise:           pred = W + N − NW
```

The decompiled form is the two-branch version of that — compare `W` against `N`,
then place `NW` — and it checks out against the definition case by case.

The residual `x − pred` is then **folded into an unsigned byte alphabet** through
a 256-entry table built at the top of the function: positive residuals map to
even codes, negative to odd, so that small magnitudes of either sign are small
symbols. The first row is only prediction from `N` (there is no `W`), and the
first column only from `W`.

Every folded residual increments `hist_scratch[4·s]`, which is the histogram
§4 measures. Prediction and costing are the same pass.

`unpredict_med` is the inverse and runs in the decoder under
`plane_predictor == 1`.

---

## 8. The default model — `code_pixel`

This is where the bits are actually produced. 793 lines, one call per pixel,
and its structure is two stages: a **ranked-candidate cascade**, then a **PPM
escape ladder**.

### 8.1 The workspace

`model_plane` allocates 8 102 448 bytes and hands it to `layout_workspace`,
which carves it into the model's tables for the given width, height and depth.
`free_workspace` gives it back. The coded output gets its own buffer of
`data_size + 0x20000`, with the last 4 KiB used as `hist_scratch`.

Before coding, `reduce_alphabet` restricts the alphabet to the symbols that
actually occur, and `init_symbol_list` / `init_model_tables` build the initial
counts.

### 8.2 The candidate cascade

The model keeps a list of recent and neighbouring symbols — the array
`pixel_context` reads as `p_n15[·]`. For up to **32 iterations**:

1. `pixel_context` proposes the next candidate symbol and, at the same time,
   computes a **match state** for it: a small integer formed by testing that
   symbol against the neighbourhood in groups —

   ```
   +64  if it equals the nearest neighbour
   +32  if it equals any of the next five
   +16  if it equals any of the next sixteen
   + 8  if it equals an entry in the second table
   ```

   A candidate already excluded, or one whose match state is zero after the
   sixth position, is skipped (`pixel_context` returns −1).

2. One **binary decision** is coded: is this candidate the actual symbol?
   The counter node is selected by the match state, so the probability that
   "the symbol equals the k-th ranked neighbour" is learned separately for each
   pattern of neighbourhood agreement.

3. If yes, the pixel is done and the model returns the rank. If no, the
   candidate is added to `exclusion_mask` and the loop continues.

This is the part of BMF that does the work on natural images: most pixels equal
one of their close neighbours, and the cascade spends a fraction of a bit
saying which.

### 8.3 The binary counter model

`encode_context_bit(node, parent, bit)` codes each of those decisions. A node is
three `uint16`: `{c0, c1, limit}`.

* **Lazy initialisation from the parent.** A node that has seen a zero and no
  one yet is seeded from its parent's ratio, scaled to 64:

  ```c
  P  = p0 + p1;
  c0 = (P + 64·p0 − 64) / P;
  c1 = (64·p1 + P − 64) / P;
  limit = 512;
  ```

  and the parent is nudged in the same step. So a fresh context starts from the
  statistics of its more general context rather than from uniform — this is the
  secondary-estimation structure of the model, and it is why the tables can be
  as fine-grained as they are.

* **Coding**: `rc.encode_bit(c0, c1, bit)`.

* **Update**: `node[bit] += 8`, and `parent[bit] += 1` **only while the node's
  total is under 0x88** — the parent keeps learning until the child has enough
  of its own evidence, then stops.

* **Rescale**: when `c0 + c1` passes `limit`, both are halved and `limit` grows
  by 64, up to 0x4000. The limit rising with age is a deliberate slow-down of
  adaptation.

`rescale_counter_pair` is the same halving without the coding.

### 8.4 The escape ladder

If all 32 candidates are wrong, the model falls through to a chain of symbol
lists. A list is

```
list[0]  alphabet size
list[1]  number of live entries
list[2]  escape weight
list[3]  running total
list[4]  a rescale limit
list[5]  → entries, three bytes each: { uint16 symbol; uint8 count; }
```

`encode_symbol_list(list, sym)` walks the entries, skipping anything in
`exclusion_mask`, accumulating counts. If it finds the symbol it codes
`(cum, cum + count, total)`. If it runs off the end it codes an **escape** —
`(total, total + escape_weight, total + escape_weight)` — marks every symbol it
walked as excluded, and returns failure, and the caller moves to the next list
in the chain. This is exclusion coding: a symbol ruled out by one context is not
paid for again in the next.

`init_symbol_list` builds a list either **full** — every symbol present with
count 1, escape weight 0 — or **empty** — no entries, escape weight 2 — so the
chain mixes a learned model with a fallback that can always code anything.

`symbol_list_update` is the update: add to the symbol's count and to the running
total, bubble the entry forward while it outweighs its predecessor (so the list
stays sorted by count, and the walk above finds likely symbols first), and
rescale — halve every count and re-sort — when a count passes 251 or the total
passes the limit.

---

## 9. The alternate model families

When mode bit 2 is set for a plane, `model_plane` hands it to one of four
bodies instead of `code_pixel`:

| | depth 8 | other depths |
| --- | --- | --- |
| `plane_predictor == 1` | `alt_model_p1_d8_encode` | `alt_model_p1_encode` |
| `plane_predictor == 2` | `alt_model_p2_d8_encode` | `alt_model_p2_encode` |

with matching decoders. That dispatch is the whole of what selects them: the
predictor field picks the family, the depth picks the specialisation.

**What these do is not established.** Twenty-two bodies are reachable only from
this dispatch and from each other — `alt_p1_context`, `alt_p2_context`,
`alt_p2_model`, `alt_p1_model`, the two allocators and the rest — and reading
them is open work. What *is* established, from their own code:

* `alt_p1_context` and `alt_p2_context` compute context and code nothing: no
  `rc.` call appears in either, and both encoder and decoder call them. They
  read the causal neighbourhood, form gradients (`2W − WW − N` and relatives)
  and a weighted neighbour sum, and store them with the table pointers those
  select.
* `alt_p2_filter` is a fixed-coefficient weighted sum over float vectors, called
  only by `alt_p2_context`.
* The p2 family's working memory is a table of **18-byte records** with rows
  **144 bytes apart** — eight records to a row. Fifty-six places copy one record;
  the compiler unrolled every one into four dwords and a word, which is what
  made them findable. The five fields are four `uint32_t` and a `uint16_t`.
* `alt_model_p1_decode` keeps a **five-deep ring of row pointers**, rotated one
  place per pass, and derives five working cursors from it with three of them
  offset by eight.
* `alt_p1_encode_symbol` / `alt_p1_decode_symbol` and their p2 counterparts code
  a symbol from a cumulative-frequency table with a halving rescale at 0x4000,
  through `encode_symbol_tree` / `decode_symbol_tree`. The p2 pair starts with a
  three-way choice over three counters before descending.

So: a context model with its own neighbourhood statistics and its own
frequency tables, on a sliding window of rows. What the eighteen bytes of a
record hold is the question, and `alt_p2_context` — which computes the values
that go into them — is where a reading would start.

---

## 10. Deciding to keep the coded stream, and the raw fallback

After the last plane is modelled, `compress_image` flushes the bit packer and
compares:

```c
coded = out_cursor − coded_buf;
if (coded < data_size)   keep it
else                     store the pixels
```

**If the coded stream wins**, three writes go out: the 16-byte descriptor with
`data_size` replaced by `coded` — the four frame slots the encoder has been
filling are literally the descriptor, and the fourth holds the coded length —
then the coded buffer, then the palette from `pixels + data_size` if the depth
byte's 0x80 is set.

**If it does not**, the coded buffer is freed and the pixels are written
instead. Two cases:

* the image was transposed for modelling (flag bit 1) — it is transposed back
  into a fresh buffer, the width and height are exchanged in the descriptor, the
  flag is cleared, and *that* is written;
* otherwise the pixels go out where they lie.

Either way the payload is `data_size + palette` bytes and the descriptor
precedes it. `noise24.bmp` exists in the corpus to exercise this: 49 206 bytes
in, 49 172 out — which is 4 magic + 16 descriptor + 49 152 pixels, no palette,
and not one bit of model.

There is also a floor before any of this: an image whose `data_size` is under 16
bytes skips the model entirely and goes straight to the raw store.

---

## 11. What this document does not establish

* **What the filter search searches over** (§6). The mechanism is clear and the
  family of transforms is not.
* **The alternate model families** (§9), beyond their memory layout and their
  dispatch.
* **The exact derivation of the match state** in `pixel_context` (§8.2). The
  four groups and their weights are read off the code; which neighbours are in
  which group, in image coordinates, is not worked out here.
* **Which symbol lists are in the escape chain, and in what order** (§8.4). The
  walk is over a pointer chain built by `init_model_tables`, and the chain's
  membership is not established.
* **Where flag bit 3 comes from** (§2.3). It is set in every stream the corpus
  produces and it is not in the assembly that builds the byte.
* **The fast (non-`-S`) back end**, which is deleted from this source. It
  remains unestablished about BMF itself, and would have to be read out of
  `BMF.exe`.

## 12. Where to look

| | |
| --- | --- |
| container, descriptor, grey palettes | `bmf_compress`, `bmf_open_archive`, `alloc_image`, `read_bmp`, `write_bmp` |
| per-plane header | `compress_image` (writes), `expand_image` (reads) |
| inter-plane prediction | `colour_transform` |
| cost | `estimate_cost`, `cost_candidate` |
| the searches | `choose_plane_coding`, `search_filter`, `transform_planes` |
| spatial prediction | `predict_med`, `unpredict_med` |
| the default model | `model_plane`, `code_pixel`, `pixel_context`, `encode_context_bit`, `encode_symbol_list`, `symbol_list_update`, `layout_workspace`, `reduce_alphabet` |
| the alternate families | `alt_model_p{1,2}[_d8]_{en,de}code` and everything under them |

`tools/addrmap.txt` maps every name here to the address of the body in
`BMF.exe` it was decompiled from.
