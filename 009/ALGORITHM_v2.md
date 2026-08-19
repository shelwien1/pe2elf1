# BMF 2.01 — what the program does to an image

A second reading of the compression algorithms, taken from the source in this
directory and nothing else. Every claim below was checked against the file it
names; where a claim is a measurement rather than a reading, it says so and says
how it was taken.

**The range coder is out of scope.** `rc.inc` is not discussed. Everywhere below
"coded" means a value was handed to the global `rc`, and the only thing you need
to know about it is its two interfaces: a frequency-table symbol arrives as
`(cum, cum + weight, total)` and a binary decision as `(f0, f1, bit)`. What
happens to those numbers afterwards is a separate subject.

The **bit packer** *is* in scope, because it is not the range coder: the plane
descriptors are packed with plain shifts into the same buffer, and where the two
writers meet is part of the format. So are the **adaptive counters** — `rc` is
handed a distribution and this is what computes one, which makes it the model
and not the coder. Both are §5.

There is no single algorithm here. There are **three models**, a **decision
layer** that picks between them one plane at a time by trial encoding, and a
**raw fallback** for images that beat all three.

---

## 1. From the outside

`bmf.cpp`'s `main` accepts exactly two forms, both with four arguments:

```
bmf c input.bmp output      compress
bmf d input output.bmp      expand
```

Anything else prints usage and returns 1. The mode letter is read as
`argc == 4 && !args[1][1] ? toupper(args[1][0]) : 0` — a single-character
argument, so `bmf cc` is not `c`.

`bmf_compress` reads the BMP, prints a line about it, opens the output, does one
thing to the header described in §1.1, calls `compress_image`, and reports bits
per pixel. `bmf_decompress` calls `expand_image`, refuses a depth of 2, 15 or 16
with exit code 5 (*"%d bits per pixel is not a BMP depth"*), and writes the BMP.

`bmf_set_denormal_mode()` runs first. The p2 model (§11) is built on 32-bit
floats and its output has to be reproducible, so the flush-to-zero and
denormals-are-zero bits are set once at startup rather than left to whatever the
environment had.

### 1.1 the greyscale palette, detected and thrown away

Before compressing, `bmf_compress` looks at the palette of any palette image:

```c
int32_t Colours = 1 << (Depth & depth_bits);
int32_t Step    = 0x100u >> (Depth & depth_bits);
for( i = 0; i < Colours; ++i ) {
  if( Palette[3*i] != Grey || Palette[3*i+1] != Grey || Palette[3*i+2] != Grey )
    break;
  Grey += Step;
}
if( i >= Colours )
  p_i->depth = (Depth | depth_grey) ^ depth_palette;
```

If the palette is exactly the linear grey ramp — entry *i* is
`(i·256/Colours)` in all three channels — the image is marked `depth_grey`,
`depth_palette` is **cleared**, and the palette is therefore never written to
the file. The decoder rebuilds it from the depth alone.

For an 8-bit image that is 768 bytes saved, and it is the entire difference
between `testfiles/t8g.bmp` and `t8p.bmp`, which hold the same pixels behind
different palettes.

(An image that arrives already marked grey *and* palette has the palette bit
cleared without the walk.)

---

## 2. What an image is in memory

`bmp.inc`'s `BmfImage` is sixteen bytes followed by the pixels and then, if
there is one, the palette — **one allocation**, `alloc_image`:

| offset | field | |
| --- | --- | --- |
| 0 | `width` | `uint16_t` |
| 2 | `height` | `uint16_t` |
| 4 | `stride` | `uint32_t`, bytes per row |
| 8 | `_pad8[2]` | |
| 10 | `depth` | `uint8_t` — bits 0–5 the depth, `0x40` grey, `0x80` palette |
| 11 | `flags` | `uint8_t` — the mode word of §4 |
| 12 | `data_size` | `uint32_t`, bytes of pixels |
| 16 | `pixels[]` | |
| 16 + `data_size` | palette, 3 bytes an entry, BGR | |

`palette()` is `pixels + data_size`, and `row(y)` is
`pixels + data_size - (y+1)·stride` — rows are held **top first** and indexed
from the bottom, which is where a BMP's rows start.

### 2.1 sub-byte depths stay packed

This is the fact most likely to be got wrong, and it is visible in the reference
streams. `alloc_image(w, h, bpp, palette, packed)` is called with `packed = 1`
from both `read_bmp` and `expand_image`, and for a depth below 8 that takes the
sub-byte branch: a 1-bit row is `(w+7)>>3` bytes, a 4-bit row is `(w+1)>>1`.
Reading the header out of the reference streams:

| image | width | depth | `stride` |
| --- | --- | --- | --- |
| `t1` | 320 | 1 | 40 |
| `DLRAW` | 816 | 4 | 408 |
| `t8g` | 320 | 8 | 320 |
| `t24` | 320 | 24 | 960 |

So the buffer holds 8 pixels per byte at 1 bit and 2 at 4 bits. `flags_packed`
(§4) is set for exactly those, by `alloc_image`, and read by nothing.

The model does **not** see packed bytes: `reduce_alphabet` (§9.1) unpacks the
row bit by bit into a `uint16_t` per pixel before anything is coded, and the
decoder re-packs on the way out. The packing is a storage decision, not a
modelling one.

At 8 bits and above the layout is `plane_count = ((depth & 0x3F) + 7) >> 3`
byte planes **interleaved per pixel**: 24-bit is B, G, R per pixel in file
order, 32-bit is four. There is no planar buffer; planar *coding* (§7) copies a
plane out and back.

The one call with `packed = 0` is `search_filter`'s trial tile, which only ever
runs at depth 8 and above where the two agree.

---

## 3. The BMP front end

### 3.1 what `read_bmp` accepts

A 40-byte `BITMAPINFOHEADER` only. The acceptance test is one condition per
line and it is the only statement of the accepted format anywhere:

- width in 1…65535 and height in 1…65535 — a **negative height is refused**,
  not read as a top-down bitmap;
- at 8 bits and above, one row must fit in 16 bits:
  `w · ((bpp+7)>>3) <= 0xFFFF`;
- depth exactly 1, 4, 8, 24 or 32 — no 16-bit forms, no bitfields;
- `biClrUsed` not negative, and at 8 bits or below not more than `1 << bpp`;
- `biCompression` 0 (none), 1 (RLE8) or 2 (RLE4), with RLE8 requiring 8 bpp and
  RLE4 requiring 4;
- `biPlanes == 1`.

The palette is read as 4-byte BGRA entries and stored as 3-byte BGR.

Rows are read forwards from the file into the buffer **backwards**, from the
last row up, which is what turns a bottom-up BMP into top-first storage.

### 3.2 both run-length forms are decoded, and not preserved

`read_rle8` and `read_rle4` implement the five BMP opcodes — encoded run,
end of line, end of bitmap, delta, absolute run — as a five-way branch on two
bytes:

| `n` | `v` | |
| --- | --- | --- |
| ≠ 0 | any | repeat `v` for `n` pixels |
| 0 | 0 | end of line: step up a row |
| 0 | 1 | end of bitmap |
| 0 | 2 | delta: two more bytes, `dx` and `dy` |
| 0 | ≥ 3 | absolute run of `v` pixels, padded to an even byte count |

`read_rle_op` is the two-byte read both share, and the one place the truncation
check lives: `ferror` first, then two `fgetc`s, false if either came back
negative. The int return of `fgetc` is what makes that possible — end of file has
to be distinguishable from a legitimate `0xFF`, and a byte-wide result could not.
A truncated RLE file is refused here rather than run off the end of.

RLE4 is the same machine with a nibble for a pixel: half the horizontal step in
a delta, a run counted in nibbles, and a cursor carrying which half of the
current byte the next nibble goes in. The parity is tracked across deltas — an
odd `dx` flips it and, when the cursor was on the low nibble, costs an extra
byte of advance. `write_nibbles` is the absolute-run inner loop, and the thing to
know about it is that **the parity does not change inside a run**: each turn
writes one whole byte, so the half-byte cursor is read once at the top and set
once at the end.

Every opcode is bounds-checked against the pixel buffer through
`BmfImage::holds(at, n)` before it writes, and both walks clear the buffer first
because the opcodes are not obliged to cover it.

The uncompressed case is `read_rows`: rows bottom-up, each read at `stride` bytes
and then `fseek`ed past its padding to the next multiple of four. Reading
bottom-up is not a transform — it is what a BMP is — so the buffer BMF models is
already top-down.

**The run structure is not preserved.** BMF stores pixels. `write_bmp`
re-encodes
runs with its own splitting rules, so a decoded RLE file is pixel-identical to
its input and byte-different from it; `testfiles/out_rle4.bmp` and
`out_rle8.bmp`
are what the decoder is expected to produce.

### 3.3 what `write_bmp` emits

A 54-byte header, the palette if the depth has one, and the rows bottom-up with
each padded to a multiple of four. Three palettes are possible and the depth
byte picks: a grey ramp rebuilt from the depth, the stored palette, or none.

If the image is 4- or 8-bit and run-length encoding is asked for, the rows go
through an encoder that alternates runs and literals: `emit_runs` emits repeat
pairs while the next run is worth more than the literal it would otherwise join,
and a literal is flushed either as an escape-plus-count-plus-bytes block or,
when
it is too short to be worth the two-byte header, as one or two encoded pairs. If
the result is not smaller than the raw rows, the whole attempt is dropped and
the
file is written uncompressed.

---

## 4. The container

One image per file. `file.inc` opens `"wb"` for compress and `"rb"` for expand;
there is no archive walk, because there is one member.

| bytes | |
| --- | --- |
| 4 | magic — `bmf_tag(bmf_sig_image, '2', '0')`, i.e. `81 8A 32 30` |
| 16 | the header, which is the in-memory `BmfImage` written verbatim |
| *n* | the coded stream, `data_size` bytes |
| *p* | the palette, `3 << depth` bytes, only when `depth & 0x80` |

`data_size` in a **coded** member is the coded byte count, not the pixel count.
The two are compared to decide the raw fallback and only one of them survives
into the header.

A second magic, `81 90 32 30`, introduces an auxiliary block: an eight-byte
`CodedTail` whose second word is a length, then that many bytes. `expand_image`
either skips it or hands it back through `p_coded_buf`; this build never writes
one.

### 4.1 the flag byte

```
0x01  unused
0x02  flags_transposed    width and height are swapped
0x04  flags_slow          coded in -S mode; this build decodes no other
0x08  flags_planar        each plane coded alone and interleaved afterwards
0x10  flags_descriptors   a plane descriptor per plane heads the stream
0x20  flags_coded         the payload is a coded stream, not raw pixels
0x40  flags_packed        rows are packed sub-byte
0x80  flags_tail          a CodedTail sits between the header and the data
```

Two of the seven describe the image, four describe the stream, one says whether
anything sits between them. `flags_packed` is written by `alloc_image` and read
by nothing — in this program or in the format — and it is on the wire for every
sub-byte stream regardless.

Read out of `testfiles/ref_*.bmf` at offset 15, the corpus uses four values:

| flags | images |
| --- | --- |
| `0x64` = packed, coded, slow | `t1`, `f05_200`, `DLRAW`, `rle4` |
| `0x3c` = planar, descriptors, coded, slow | `t8g`, `t8p`, `rle8`, `t24`, `t32`, `x_ci`, `xform1` |
| `0x3e` = the same plus transposed | `x_ai` |
| `0x36` = transposed, descriptors, coded, slow (**not** planar) | `med32`, `altp1`, `x_ep`, `xform2` |
| `0x00` | `noise24` |

`noise24`'s byte is zero rather than "coded clear", and the reason is worth
knowing: a raw member writes **the image's own header**, not the one the coder
filled in, so none of the stream bits ever reach it.

### 4.2 the raw fallback

After coding, `compress_image` compares the coded length against `data_size` —
the size of the *pixels*, not of the input file — and if coding did not beat it,
writes the pixels verbatim with `flags_coded` clear. If the image had been
transposed for coding it is transposed back first, because a raw member has no
model to undo it.

A raw member costs 20 bytes of overhead plus the pixels plus a 3-byte-an-entry
palette. The BMP it came from costs 54 bytes plus 4 bytes an entry plus rows
padded to four, and BMF stores rows unpadded, so a raw member is always
*smaller*
than its input rather than merely bounded by it.

---

## 5. Below the models: the packer, the counters, and the bit meter

Three things sit between a model and `rc`, and all three are in scope because
none of them is the range coder. The packer is a second writer into the same
buffer. The counters are where the probabilities actually come from — `rc` is
handed numbers, and this is what computes them. And `estimate_cost` is the one
number every decision in §6 compares.

### 5.1 the bit packer

`rc_io.inc`'s `pack_bits` and `unpack_bits` are a plain LSB-first packer over
32-bit words. The near-lossless nibble and the plane descriptors go through it
into the **same buffer** the range coder will then write, so a coded member's
payload is:

```
[ 4-bit near-lossless nibble ][ 6..30 bits per plane of descriptor ][ coded stream ]
```

The two writers meet at `packer_rewind`. The packer writes whole words and the
coder starts at a byte, so before the first symbol the output cursor is backed
off over whatever whole bytes the last word did not need, one at a time, leaving
`stream.pk.free_bits` holding what is still owed — it goes transiently negative
on the way. Coming back in, `packer_word` refills the accumulator a word at a
time from the same cursor.

One cursor, two writers: that is why `compress_image` can measure a coded stream
as `stream.cur - stream.buf` and get a byte count that includes the descriptors.

### 5.2 `BitCtr` — a binary counter that inherits

Every binary decision in model A goes through `BitCtr::code_context_bit`
(`bitctr.inc`): two 16-bit counts and a rescale threshold. What makes it more
than a pair of counts is that a fresh one does not code its own bit — it borrows
a parent's distribution until it has evidence of its own. The counter has
**three states**, and they are distinguished by the two counts rather than by
any flag:

| state | test | what happens on a bit |
| --- | --- | --- |
| unseen | `n[0] == 0` | the **parent** codes the bit and takes the +8; this stores `bit + 1` into `n[0]` and nothing else |
| seen once | `n[0] != 0`, `n[1] == 0` | `seed_from(parent)` runs first, then it codes normally |
| live | both non-zero | it codes its own bit |

An unseen counter is one that is all zeroes, which is why the arrays it lives in
are simply `memset` and never initialised entry by entry.

`seed_from` is the interesting one. The child takes the parent's *shape* scaled
to a total of 64 —

```
n[0] = (par_tot + 64*par[0] - 64) / par_tot
n[1] = (64*par[1] + par_tot - 64) / par_tot
```

— then bumps the side the remembered first bit went by 4, sets its threshold to
512, and **takes the parent down by 3 on that same side** (when the parent has
more than 3 there to give). The last of those is
easy to miss and is the point of the arrangement: a parent that has just handed
its shape to a child is that much less sure of itself, so the next child gets a
slightly different inheritance. Prediction spreads from parents to children and
is depleted by the spreading.

A coded bit adds `kStep = 8` to its side. When the total passes `limit`,
`scale_rare` halves both sides **and raises the limit by 64** (to a ceiling of
0x4000): a counter that keeps being right is allowed to get more confident
before it is cut back again. And while the child is still cheap to be wrong
about — total under 0x88 — the parent hears about the bit too, gaining 1 on the
side that was coded. That is a partial update, not a full one, and it stops as
soon as the child has 136 units of its own evidence.

Two seeded shapes exist. `init_parent` is even (4/4) under a low threshold of
72, so the first handful of bits move it a long way. `init_root` is **not**
even: 40 against 16, five to two toward "no", which is the prior for an escape.

### 5.3 `CounterNode` — seven slots and an escape

Model B codes a residual through a seven-slot frequency node, not a 256-way
table. `init_counter_node` seeds it `{8, 2, 2, 2, 2, 3, 3}` with a total of 22,
and the mapping from symbol to slot is:

```
sym < 5          slot = sym               five slots of their own
sym >= 5, odd    slot = 5      \  the two escape slots, split by parity
sym >= 5, even   slot = 6      /
```

So the alphabet is five literal symbols and two escapes. A coded symbol adds 32
to its slot and to the total.

The rescale is unusual and worth stating exactly. When the total passes 0x2000
the node finds its **smallest** count and then divides every count by two if
that minimum is 0 or 1, but by **three** — `(c + 2) / 3` — if it is 2 or more.
A node with a rare symbol still in it is cut gently so the rare symbol is not
rounded away; a node where everything has been seen is cut hard. The new total
starts from `0x8000` rather than 0, because the top bit of `total` is a flag
and the coder masks it off with `& 0x7FFF` at every read.

When the slot is 5 or 6 the node has escaped, and the remaining magnitude is
coded by the binary tree of §5.4 against a frequency strip chosen by three
things: the escape's parity (`128 * (slot & 1)`), the caller's context, and one
bit from `escape_bias`.

`escape_bias` is 0 or **+64**, and it is +64 when the escaped slot holds more
than half the node's mass. It computes that with an unsigned wrap rather than a
comparison: `(total & 0x7FFF) + c[0] - 2*c[slot]` in `uint32_t` arithmetic goes
huge when the slot is over half, `>> 25` turns huge into 127, and `& ~63` turns
127 into 64. The source comment on it claimed `-64` and claimed the opposite
condition; both are wrong, and the call sites settle it without needing the
shift read at all — the value is *added* to a strip index, and `0xFFFFFFC0`
would leave a 1024-entry table on the first escape. Corrected in `bitctr.inc`
in the same commit as this section.

### 5.4 the symbol tree

`code_symbol_tree` (`sym_code.inc`) codes a value in two stages: a **level**
from a ten-entry frequency header, then a **path** down a binary tree within
that level. `begin_plane_stream` lays the geometry out once per plane:

```
level        2   3   4   5   6   7
first        2   4   8  16  32  64      first symbol of the level
symbols      2   4   8  16  32  64      2 * level_geom[lvl].half
path bits    1   2   3   4   5   6      log2 of that
tbl_base     0   1   4  11  26  57      first - lvl, except level 2
```

The six trees are laid end to end after the ten-entry header, and `model_geometry`
covers symbols 0..127 — a 128-symbol alphabet in six levels.

The seeding fills the whole strip: 10 header entries plus 122 pairs is 254
`uint16_t`, which is the strip width exactly. The *walk* does not reach that
far. Level 7 is the deepest, its table starts at index 122, and the last pair it
can touch is at 248..249 — so four `uint16_t` at the top of every strip are
seeded and never read. (Checked by running the recurrence from `rc_io.inc` and
the walk from `sym_code.inc` against each other, rather than by reading them.)

`model_geometry[i]` is the level that owns symbol `i` — 0 at 0, 1 at 1, and 2 or
more from there up — so a symbol below 2 *is* its own level and the tree is
skipped entirely. That is why the encoder's `sym >= 2` and the decoder's
`lvl >= 2` are the same test written from the two ends.

The level is coded from `freq[2..9]` with `freq[0]` as the running total, and
the two directions meet at the slot: the encoder sums the frequencies below its
level, the decoder walks until it passes the point `rc` gave it. From there —
rescale, escape counter, tree walk — the code is shared.

The level rescale (at a total of 0x4000) halves all eight level counts, and then
does something to `freq[1]`, the increment that every coded level adds:

```
freq[1] <= 4*alt_freq_limit   ->  freq[1] -= 4 * (freq[1] > alt_freq_limit)
otherwise                     ->  freq[1] -= 16
```

The increment **decays toward `alt_freq_limit`** — fast at first, then by 4, then
not at all. A plane starts adaptive and settles; `freq[1]` seeds at
`24 * alt_freq_limit` and walks down. The two limits themselves are set by which
model is running: 8 and 8 for predictor 2, 16 and 64 for everything else.

Each pair in the tree is halved at 0x4000 — by `halve_pair`, which halves both
sides and hands back the one the walk took — and the side taken is then set to
`alt_freq_init + f`, so a tree node's counts never run away and never reach zero.
All 1024 strips are seeded identically at `begin_plane_stream` — a ten-entry
header `{635, 24*limit, 205, 124, 147, 83, 48, 16, 8, 4}` and 122 pairs at
60/36. The header is a decaying geometric prior over levels; the pairs are a
mild bias toward the low branch.

### 5.5 `update_binary_pair` — the same tree, moved without coding

`update_binary_pair` walks the identical structure and codes nothing. It is
called from seven sites — four in model B, three in model C — always on strips
**neighbouring** the one that coded: the context one step up, one step down, one
bias apart. A symbol trains the strips near it as well as the strip that
carried it. This is
context mixing done by writing rather than by averaging: nothing blends the
neighbours' predictions at read time, they are simply kept warm.

Two constants differ from the coding path, both keyed on the running model:

```
level increment   (freq[1] >> 2) & ~31        predictor 1
                  15 * (freq[1] >> 5)         predictor 2
pair increment    (alt_freq_init * 5) >> 3    predictor 1
                  (alt_freq_init * 6) >> 3    predictor 2
```

and the pairs are halved at 0x2000 rather than 0x4000 — a strip that is only
being trained is rescaled twice as often as one that is being read.

The level counts are not rescaled here at all. There is no equivalent of
`code_symbol_tree`'s 0x4000 branch; instead the **whole function** does nothing
once `freq[0]` passes 0x8000. So a strip that is only ever trained and never
coded saturates and then stops moving, where a strip the coder reaches is
halved and carries on. Training decays to nothing on its own; it does not have
to be switched off.

### 5.6 `estimate_cost` — the one number every decision compares

Every choice in §6 — which transform, which predictor, which references, which
of the four candidate filters — is settled by handing a histogram to
`estimate_cost` and comparing integers. It returns

```
(N·ln N  −  Σ nᵢ·ln nᵢ) · log₂(e)
```

which is `N · H(p)`: the **total** cost in bits of coding N symbols with those
frequencies, not the per-symbol cost. Nothing in the decision layer models the
actual coder; it models an order-0 entropy of the residuals and picks the
smallest. That is why §6.5 matters — the thing being measured is not the thing
that will run.

The histograms it reads are built by `hist_bump`, one increment at
`hist_scratch[4k]` — a `uint8_t*` addressed as 32-bit counters, so the increment
goes through a pair of `memcpy`s rather than a cast.

And `hist_scratch` is not its own allocation. `packer_reset` places it at
`stream.buf + stream.size − 4096`: **the last 4096 bytes of the output buffer**,
1024 counters, living at the far end of the same block the coded stream is
growing into from the front. The decision layer's scratch space and the output
share one allocation and grow toward each other. `transform_planes` clears all
4096 bytes at the top of a candidate; `model_planes` clears the first 1024 —
256 counters — per plane.

`stream_open` sizes the block `data_size + 0x20000`, so the histogram sits in the
last 4KB of 128KB of headroom past the raw pixel count, and a coded stream would
have to exceed the raw size by 124KB to reach it — which the raw fallback of §4.2
makes moot. `packer_reset` re-places the pointer on every rewind, and since
`search_filter` rewinds between every trial, that placement is part of the reset
rather than a one-time setup.

The two accumulators over even and odd bins are the original's pairing for a
two-lane SSE add. They are kept: floating-point addition is not associative, so
merging them changes the sum, and the sum feeds a comparison that decides a
filter.

---

## 6. The decision layer

Only the encoder runs any of this. The decoder is told the answers, in the
descriptors.

### 6.1 depth 4 and below takes the short path

`code_image_body` tests `(depth & depth_bits) <= 4` and, if so, opens the stream,
sets `plane_predictor = 0`, `plane_alt_model = 0`, `alphabet_reduced = 0`, and
calls `model_plane` on the whole image. **No search, no transform, no
predictor, no descriptors** — `flags_descriptors` is never set, which is why the
corpus's four sub-byte streams carry `0x64` and not `0x3c`.

That is the whole of the narrow arm; the wide one runs `search_filter`, sets
`flags_descriptors`, re-reads the header if the search transposed the image, and
then `write_plane_descs` and the planes. `write_member_head` puts the sixteen-byte
header out, followed by the coded tail when there is one — which is what makes a
`.bmf` a container of members rather than one stream, and §4.2's raw fallback the
same function with no tail.

`search_filter` has its own refusal, separate from that: an image narrower than 4
or shorter than 3 gets `reset_descriptors` — every descriptor cleared to predictor
0, no references, no alternate model — and a returned cost of 0. Too small to
measure anything on, so nothing is claimed.

There is one plane and its alphabet is at most sixteen symbols. The main model's
match-and-run machinery is what suits that; a predictor on a 4-bit palette index
would be arithmetic on numbers that do not mean quantities.

Everything from here on applies to 8, 24 and 32 bits.

### 6.2 the descriptors, and the field that does two jobs

Four `PlaneDesc`, one per plane, each six bits on the wire plus what its flags
call for:

```c
struct PlaneDesc {
  uint8_t nrefs, src_plane, flags, dc;   // packed into one int32_t
  int32_t weight0, weight1, weight2;
};
```

They are indexed two ways and it is easy to get backwards. **`plane_desc[k]`
describes source plane `k`**; **`plane_desc[k].src_plane` names the plane coded
`k`-th**. A plane that references others is described in its own slot and coded
last.

The wire format carries `(flags << 2) | nrefs` in six bits, then — only when
`desc_has_refs` is set — a byte of `dc`, and then, only when `nrefs > 1`, one
byte each of `weight0` and `weight1` biased by 64, and `weight2` too when
`nrefs > 2`. `write_plane_descs` and `read_plane_descs` are the two ends of
exactly that, through the packer of §5.1; the bias of 64 is there because a
weight is signed and the field is not.

Nothing on the wire states the coding order separately, because **`nrefs` is
both the reference count and the position**. They are the same number: a plane
coded `k`-th has exactly the `k` planes before it to reference.
`read_plane_descs`
uses the one field twice:

```c
plane_desc[pl].nrefs = nrefs;        // how many references
plane_desc[nrefs].src_plane = pl;    // …and therefore where in the order
```

The flag field is four bits: `desc_predictor` (0x03, the two-bit predictor
number), `desc_alt_model` (0x04), `desc_has_refs` (0x08).

### 6.3 `choose_plane_coding` — order, transform, DC

This runs first and it costs everything by **entropy**, not by trial coding:
`estimate_cost(hist, n)` is

```
(N·ln N − Σ nᵢ·ln nᵢ) / ln 2
```

in bits, over a histogram of residuals. Cheap enough to run over the whole image
several times.

1. **Pair the planes.** `plane_desc[2k]` and `plane_desc[2k+1]` are seeded with
   `src_plane = nrefs = 2k` and `2k+1`, so a plane can reference the one before
   it.

2. **Three candidate orderings** (3+ planes). `cost_candidate(img, cand, …)`
   fills a row of four costs and a row of four descriptors for each of
   `cand ∈ {0,1,2}`; the cheapest wins, ties to the lower index, and its
   sixty-four bytes of descriptors are copied straight into `plane_desc`. This
   is what decides whether green or blue is the plane the others are predicted
   from. `testfiles/xform1.bmp` and `xform2.bmp` are in the corpus because
   they are the only images that pick anything other than candidate 0.

3. **Fit the two weights** by coordinate descent. `WeightSearch::descend(4, −1)`
   then `descend(4, +1)`: each axis walks by one, a trial replaces one weight
   and
   keeps it if `weight_pair_cost` — a full pass over the image — comes back
   cheaper, and a win pushes that axis's bound four further out, so an axis
   keeps going as long as it keeps paying.

4. **Choose among four transforms.** One pass accumulates four histograms of the
   plane against, respectively, the fitted weighted pair, reference A alone,
   reference B alone, and the two averaged. The incumbent is the fitted pair and
   the other three have to beat `best + min(best >> 7, 0x4000)` — a slack of one
   part in 128, capped. The winner writes `(weight0, weight1)`:

   | winner | weights |
   | --- | --- |
   | the fitted pair | whatever the descent found |
   | reference A | (128, 0) |
   | reference B | (0, 128) |
   | their average | (64, 64) |

5. **The DC offset** is where the residual distribution sits. `widest_window`
   slides a 256-wide window over the winning histogram and answers where the
   densest position starts; that plus one, truncated to a byte, is
   `plane_desc[].dc`.

   The scan starts at entry 4 rather than 0, and the reason is that the original
   computed the start as the low four bits of a stack address — a frame that was
   `alignas(16)` with the histogram four bytes into it, so the answer was always
   4. All nineteen corpus images produce byte-identical streams at 0 and at 4;
   the measured value is what the code keeps, with the 4 written down.

6. **A fourth plane gets a least-squares fit.** For a 32-bit image the alpha
   plane's three weights are not chosen from a menu. One pass accumulates the
   nine covariances `S_ab = Σ d_a·d_b` of the four planes' local gradients, and
   the 3×3 system `S·w = S_w` is solved by cofactors:

   ```c
   d1  = Sxx·Szz − Sxz²
   d2  = −Sxx·Syz + Sxz·Sxy
   d3  = Sxz·Syz − Sxy·Szz
   inv = 128 / (Syy·d1 + Syz·d2 + Sxy·d3 + 0.1)
   ```

   The 128 is because the weights are applied as 128ths; the `+ 0.1` is what
   keeps a flat image — every gradient zero, the system singular — from dividing
   by zero. Each weight is clamped to [−64, 191], which is exactly the range the
   descriptor's biased byte can carry. Then four more candidates are compared
   the
   same way: the fitted triple, or any one of the three references alone at 128.

### 6.4 `search_filter` — trial encoding

Where `choose_plane_coding` estimates, this one **actually encodes and throws
the output away**, keeping the bit count. It is the whole cost of `-Q9`.

The measurement is `transform_cost`: run `transform_planes` over a scratch
image, take `8 * (stream.cur - stream.buf)`, then `packer_flush` and
`packer_reset` to put the cursor back where it started. Every figure below is
that function, and every one of them is real coded bits from the real models —
not an entropy estimate. That is the difference between §6.3 and §6.4, and it
is why one runs per plane and the other runs a few dozen times per image.

It cuts a tile from the centre of the image — the full width and height if they
fit, otherwise centred — and refuses outright if the image is narrower than 4 or
shorter than 3, resetting every descriptor and returning 0.

For each plane, in `src_plane` order, it probes a subset of six flag values:

| value | name | predictor | model | references |
| --- | --- | --- | --- | --- |
| 0 | `try_mode0` | none | main | no |
| 5 | `try_p1` | 1 | alternate p1 | no |
| 6 | `try_p2` | 2 | alternate p2 | no |
| 8 | `try_refs` | none | main | yes |
| 13 | `try_refs_p1` | 1 | alternate p1 | yes |
| 14 | `try_refs_p2` | 2 | alternate p2 | yes |

and the pruning is one idea throughout: **has any earlier plane already settled
on predictor 2.** That counter is `n_hard`, and the reasoning is that an image
needing the deep model for one plane will need it for the rest, so the cheap
candidates stop being worth their time.

- `try_mode0` is skipped once `n_hard` is non-zero;
- `try_p2` is tried when `try_p1` came within `cost/32` of the incumbent, **or**
  `n_hard` is non-zero;
- `try_refs` and `try_refs_p1` only from the second plane on, since the first
  has
  nothing to reference;
- `try_refs_p2` when the winner so far is predictor 2, or `try_refs_p1` came
  within `cost/32`.

Then the whole-tile trials, each of which edits every descriptor at once,
re-encodes, and keeps the edit only if it did not cost more:

1. **transposed or not.** The tile is rotated by `transpose_image` — which
   copies the pixels out, writes them back column-major, and then calls
   `transpose_image_in_place` to swap `width` with `height` and recompute the
   stride — `flags ^= flags_transposed`, and
   re-encoded plane by plane against the per-plane figures just taken. The walk
   stops at the first plane that costs more transposed than it did upright — so
   where it stopped *is* the answer, and no flag is needed to record it.
2. **everything on p1** (3+ planes, at least one plane chose predictor 1):
   `offer_to_all(try_p1)` keeps each descriptor's `desc_has_refs` and puts p1
   under it.
3. **everything on p2** (some plane chose p2, or the p1 trial won). Two
   shortcuts skip the measurement — if every plane already chose p2 and all but
   one already reference, it is taken unmeasured — and a win gets one further
   trial with `allow_refs_where_present()`, which adds `desc_has_refs` to every
   plane whose `nrefs` is non-zero.
4. **alternate models dropped** (no plane chose p2, more than one plane).
   `drop_alt_model_from_all()` clears `desc_alt_model` and keeps the predictor
   and the reference bit, so `13 → 9` and `5 → 1`. This is the only way a stream
   ends up with predictor 1 under the **main** model, and `testfiles/med32.bmp`
   is the image that reaches it.
5. **everything cleared** (same guard, and some plane either references or chose
   p1). The pixels are restored from a copy taken before the previous trial and
   `clear_flags_on_all()` writes 0 everywhere: main model, no predictor, no
   references.

One thing changes which of those run, and the name hides it. On the path where
the p2 trial wins, the counter is overwritten with a bit count
(`n_hard = bits_a`) — and a bit count is never zero in practice, so the two
`!n_hard` trials below are skipped from then on.

The return value becomes `flags_planar`.

### 6.5 the search measures a cheaper model than the one that runs

`choose_plane_coding` sets `alphabet_reduced = 1` on entry and
`compress_image` sets it back to 0 before the real encode. That flag gates the
p2 model's neighbour-context updates (§11.4) — the most expensive part of the
most expensive model. So every figure the decision layer compares was taken
against a model that is not quite the one that will run. The costs are
consistent with each other, which is what a comparison needs; they are not
consistent with the final stream.

---

## 7. Transforms and predictors, outside the models

Two things happen to a plane's samples before a model ever sees them, and both
are exactly invertible in 8-bit wraparound arithmetic.

### 7.1 the colour transform

`code_colour_plane<f_DEC>` moves one plane between the interleaved image and a
flat per-plane buffer, subtracting what the planes it references predict of it.
`plane_transform(plane)` decodes the descriptor into four modes:

| `nrefs`, `desc_has_refs` | what happens |
| --- | --- |
| no `desc_has_refs` | a plain strided copy, no arithmetic |
| one reference | `x − dc − ref` |
| two references | `x − dc − (w0·a + w1·b + 40) / 128` |
| three references | `x − dc − (w0·p[−3] + w1·p[−2] + w2·p[−1] + 63) / 128` |

The three-reference case reads the three bytes *before* the sample, which are
the other components of the same pixel, rather than two named planes.

There is a fifth case that is not a mode: a two-reference plane whose weights
are
128 and 0 reads only one plane, and *which* one depends on which weight is zero.
`PlaneTransform::by_weights` takes that path explicitly, so a descriptor saying
`nrefs == 2` can behave as one reference.

The rounding constants differ — 40 for two references, 63 for three — and the
whole blend is done unsigned, which is what keeps the forward direction from
signed overflow. The inverse writes `blend + dc + x` over the same operands in
the same order, which is why both directions are one
`template<int32_t f_DEC>`.

**The same blend exists twice in the program, and the two do not agree.**
`PlaneTransform::blend` above is the planar path. Model B does not use it: it
codes the interleaved image directly, one plane at a time within a pixel, and
subtracts the reference blend itself through `plane_mix2` and `plane_mix3` in
`alt_p1_code.inc`. Term for term the expressions are identical — same weights,
same operands, same shift — except for the rounding constant on the
three-reference form:

| | two references | three references |
| --- | --- | --- |
| `PlaneTransform::blend` (planar) | `+40` | `+63` |
| `plane_mix2` / `plane_mix3` (interleaved) | `+40` | **`+64`** |

So a three-reference plane rounds one way when the image is coded planar and
the other way when it is coded interleaved, a difference of one in the last
place of the prediction. It is not dead code on either side: counting calls over
the corpus, `plane_mix3` runs on `altp1`, `med32` and `t32` — the four-plane
images — and `plane_mix2` on those three plus `t24`. Every other image reaches
neither.

Whether this was deliberate in 1997 is not answerable from the source. What is
answerable is that both constants are in `BMF.exe`, since the gate here is
byte-identical output and neither may be changed. Anyone reimplementing from
this document needs both.

### 7.2 MED, standalone

`predict_med` / `unpredict_med` apply the gradient rule — `med_predict`, which
is LOCO-I's: `min(N,W)` at a rising edge, `max(N,W)` at a falling one, and
`N + W − NW` between — **in place over a whole plane**, folding each residual
zigzag through a 256-entry table built by `med_fold_table` (and inverted by
`med_unfold_table`). This is what runs for predictor 1 when the *main* model is
coding — the flags 1 or 9 that only `search_filter`'s alt-off trial produces.

The forward pass walks the plane **backwards from the last pixel**, so each
prediction still sees unmodified neighbours; the inverse walks forwards, so each
sees reconstructed ones. That difference is structural, not a spelling, and it
is why the two stay separate functions where thirteen other encode/decode pairs
in this tree became one template each.

The first row is a plain left-to-right difference (nothing above it) and the
first column is a plain vertical one (nothing to the left). A one-pixel-wide
image never calls the gradient rule at all — a pixel's only neighbour is the one
above.

So the same gradient rule appears twice in the program in two different roles:
inside model B as a live predictor whose residual feeds the context, and here as
a whole-plane pre-transform handed to the main model.

### 7.3 planar and interleaved

`transform_planes` is the encoder's loop over the planes and `plane_in_order(k)`
its one-line indexer: it returns `plane_desc[k].src_plane` — the plane coded
`k`-th — and sets `plane_predictor` from *that plane's own* descriptor on the way
past. Two indices in one line, which is the §6.2 confusion in its most
compressed form.

In planar mode (`flags_planar`) `model_planes` runs per plane: it sets
`plane_predictor` and `plane_alt_model` from the descriptor, calls
`colour_transform` to gather the plane into a flat buffer, clears the
1024-entry `hist_scratch`, applies `predict_med` when the descriptor says
predictor 1 without the alternate model, and hands the model **a header claiming
one eight-bit greyscale plane** regardless of the image's real depth.

In interleaved mode the model is handed the image's own header and walks the
interleaved buffer at a stride of `plane_count`, so a 32-bit image is one pass
over four-byte symbols rather than four passes over bytes.

### 7.4 which model a plane actually gets

`code_plane<f_DEC>` is where the descriptor becomes a model. It asks
`alt_model_plane` first, and falls through to the main model only if that
answers false:

```
plane_alt_model == 0            ->  false: the main model codes the plane
predictor is neither 1 nor 2    ->  true, and nothing is coded here at all
predictor 1 or 2                ->  true, after one of six entry points
```

The middle line is the one to notice. A descriptor with `desc_alt_model` set and
a predictor of 0 or 3 returns **true without coding anything** — it does not fall
through to the main model. That is what the original's nesting says, and §12's
"what cannot happen" depends on the search never producing such a descriptor.

The six entry points are three pairs, and which pair is chosen by the plane's
depth and the direction, not by the predictor:

| | encode | decode |
| --- | --- | --- |
| 8-bit, p1 / p2 | `alt_model_p1_d8_encode` / `alt_model_p2_d8_encode` | `…_d8_decode` |
| other depths, p1 | `alt_model_p1_encode` | `alt_model_p1_decode` |
| other depths, p2 | `alt_model_p2_encode` | `alt_model_p2_decode` |

The eight-bit variants take `(pixels, width, height)` and the others take the
image header, which is the whole reason the split exists: an 8-bit plane is a
flat byte array and needs no header to walk.

When it does fall through, `new_model_block` allocates the main model's
workspace for this plane's dimensions and depth, `model_plane_slow` or
`unmodel_plane_slow` runs the row walk, and `free_workspace` gives it back. One
allocation per plane, not one per image.

---

## 8. Model A — the main model

`model.inc`, dispatched by `plane.inc`. It runs for every image at 4 bits or
below, for every plane the search leaves at flags 0 or 8, and for the whole
interleaved image when `search_filter`'s alt-off trial wins.

It is not a predictive coder. It never computes "what value should this pixel
be"; it asks **"is this pixel the same as one of its neighbours, and if not,
which of the symbols nearby is it"**. That is what suits a palette, where symbol
200 and symbol 201 have nothing to do with each other and arithmetic on them is
meaningless, while "the same as the pixel above" is meaningful at any depth.

### 8.1 alphabet reduction

`reduce_alphabet` runs before anything is modelled, and it has two arms.

**Eight bits or fewer** (`reduce_narrow_alphabet`). A 256-entry flag table
records which symbol values occur. Then:

1. the **count** of distinct symbols is coded flat over the full range —
   `rc.encode(n−1, n, mask+1)`;
2. the **set** is coded as sorted **gaps** between the values that occur, each
   gap through a dense `SymList` of capacity `mask − n + 2` with
   `rescale_at = 19·n`, so an image using 40 of 256 palette entries pays for 40
   gaps rather than 256 flags;
3. the image is rewritten into `sym_word[]` — one `uint16_t` per pixel — with
   each pixel replaced by its **index in the reduced alphabet**. The same 1024
   bytes hold the occurrence flag during the count and the new symbol number
   after it.

For a depth below 8 this pass is also where the **packed rows are unpacked**:
the source is read bit-field by bit-field and each pixel lands in its own
`sym_word` entry. Everything downstream works one entry per pixel.

Coming back the other way, `write_row` is what undoes it: the decoded symbol
numbers in `row_cur[0]` are mapped back through `sym_code[]` and written out at
whatever width the depth calls for — `write_row_at<uint32_t>` for four bytes, a
three-byte `SymEntry` store for 24-bit, and the narrow cases repacked. The
alphabet reduction is a permutation, so the inverse is a table lookup and not a
search.

**More than eight bits.** A symbol is now `(depth+7)/8` bytes and no flag table
will do, so the distinct values go into a **binary search tree** of
`ReduceNode { uint32_t val; uint16_t kid[2]; }` — eight bytes a node, 8192 of
them, which is the cap. `tree_place` walks and hangs a new leaf off the side the
walk fell out of.

Then one of two things:

- **the alphabet fits** (≤ 8192 distinct values). The count is coded flat over
  0x2001 and the tree's values are coded **byte by byte**, through `4·n_kids`
  symbol lists: the list for byte *k* is chosen by a two-bit carry taken from
  the
  *previous* byte's top bits (`carry = byte >> 6`, and `low_byte >> 7` across a
  symbol boundary). So the alphabet itself is coded with a small context.
- **the alphabet overflows**. The image is **de-interleaved into `n_kids` byte
  planes stacked vertically** — `height` becomes `n_kids · height`, `depth`
  becomes 8 — and `reduce_alphabet` **recurses** on that. A 24- or 32-bit image
  with more than 8192 distinct colours degrades into a tall 8-bit image and the
  model codes bytes.

### 8.2 the workspace

`layout_workspace` builds, per plane:

- **five row buffers** of `PixRec[width+16]`, with `row_cur[0..4]` the buffers
  and `row_cur[5..9]` cursors sitting **eight records in**, so the model can
  read
  several pixels either side of the current column without leaving the
  allocation. `row_cur[5]` is the current row and 6 to 9 are one, two, three and
  four rows up. Every record starts `sym = 0` with **all six match flags set**,
  which is what makes the borders behave as "everything matched".
- `run_bucket[width+1]`, a **log-scale bucket of a run length**: the bucket
  steps whenever the index reaches the next power of two.
- `sym_rev[8192]`, each entry the **13-bit reversal** of its index times 8.
- three context-id tables filled with `0xFFFF`, `sym_ctr[524288]` filled with
  `no_symbol` (8192), the `FreqRec` grid cleared from entry 188 on, and the
  binary counters seeded.

`seed_alphabet` then builds the symbol-list side of it: an `alpha_map` of one
byte per symbol set to 1, the dense escape list, and the two per-symbol selector
list arrays from `new_sym_lists` — one `SymList` per symbol, each starting with
no entries. `init_symbol_lists` gives every list its capacity. On the decoding
side the same work is preceded by `expand_alphabet`, which is where the alphabet
arrives off the stream rather than being computed: the count flat-coded, then
the values byte by byte through the carry-context lists of §8.1, run in reverse.

A `PixRec` is eight bytes: a `uint16_t sym` and six `match[]` bytes. **Those six
bytes are the model's texture, and every context signature below is built from
them rather than from symbol values.** `init_tables` writes them once per pixel:

| lane | this pixel equalled |
| --- | --- |
| `match[0]` | N, the pixel above |
| `match[1]` | W, the pixel before |
| `match[2]` | NE |
| `match[3]` | NW |
| `match[4]` | two right and one up |
| `match[5]` | three right and one up |

Lanes 4 and 5 are not neighbours of *this* pixel. They are neighbours the row
above will have when the cursor reaches them, recorded now so the run scan and
the gradients can read a whole span without recomputing.

### 8.3 the fifteen context groups are the fifteen partitions of four neighbours

This is the piece of the model most worth deriving rather than reading.

`match_context` builds a **six-bit** signature, and every bit is one of the six
pairwise equalities among the four neighbours:

| bit | flag in the source | what it means |
| --- | --- | --- |
| 0 | `up->match[1]` | N = NW |
| 1 | `west[-1].match[0]` | W = NW |
| 2 | `n.up_next == n.upleft` | NE = NW |
| 3 | `up[1].match[1]` | NE = N |
| 4 | `west[-1].match[4]` | W = NE |
| 5 | `west[-1].match[2]` | W = N |

A set of pairwise equalities on four objects is an **equivalence relation**, so
only the patterns closed under transitivity can occur — the partitions of a
4-set, and there are Bell(4) = **15** of them. Computing the fifteen signatures
directly gives

```
0, 1, 2, 4, 8, 10, 13, 16, 17, 22, 32, 35, 36, 56, 63
```

which is `tables.inc`'s `ctx_group_flags` **exactly, in that order**.
`ctx_state`
is a 64-entry table with only those fifteen written; the other 49 are never
read, not by corpus luck but by construction. (Checked both ways: derived above,
and confirmed by instrumenting `match_context` and compressing all nineteen
corpus images, which produced no signature outside the fifteen.)

That is also what `GroupFolds` is for. The stage-one alphabet has five slots —
escape, N, W, NE, NW at `w[0..4]` — and two neighbours in the same block of the
partition are *the same symbol*, so they must not get two slots. Each equality
bit folds one weight into another, and the correspondence is one-to-one:

| bit | equality | fold |
| --- | --- | --- |
| 0 | N = NW | `ctx_w4_to_w1` — NW's weight into N's |
| 1 | W = NW | `ctx_w4_to_w2` — into W's |
| 2 | NE = NW | `ctx_w3_double` — NE's doubles, NW's goes to zero |
| 3 | NE = N | `ctx_w3_to_w1` |
| 4 | W = NE | `ctx_w3_to_w2` |
| 5 | W = N | `ctx_w2_to_w1` |

`b14` ends up holding the number of live slots. If that exceeds the alphabet
size the escape is impossible, so `w[0]` is set to zero and `b14` drops by one:
a two-symbol image cannot escape from a four-neighbour alphabet.

### 8.4 seeding, and how 375 contexts become a handful of records

`model_plane_slow` seeds before its first pixel, and the shape is three nested
loops: fifteen groups, five "rank of the last symbol", five "rank of the
previous symbol".

`Neighbours::rank(sym)` is 1…4 for the four neighbours and **0 for anything
else**, so the two ranks are each one of five values, and the bucket index is

```
ctx_bucket[ state + 15·rank(pair->last) + 75·rank(pair->prev) ]
```

— 15 × 5 × 5 = the 375 entries `ctx_bucket` has. `pair` is
`group_ctr[state][key]`, the last and previous symbols seen at this exact state
and key.

A bucket is only *allocated* when it can occur. `seed_pair(lo, hi, alphabet)`
answers false — and the running bucket number does not advance — when either
slot has no weight or `b14` exceeds the alphabet, so an impossible triple simply
shares the next live bucket. When it does allocate, the increment is
`b15 = 1 << (5 − b14)`: **fewer live slots, bigger steps**, and the escape floor
`w[6]` is 64 times that.

`group_ctr[15][65536]` is initialised to `no_symbol` in both slots.

### 8.5 the per-pixel ladder

`code_pixel` is one pass down four rungs, stopping at the first that succeeds,
and returns how many pixels it consumed.

**Rung 1 — run mode.** The gate is a conjunction of nine records across three
rows: the west neighbour matched on both its lanes, and ten match flags on the
two rows above are all set. `run_scan` then measures how far the flat region
reaches — how far the north row's flags run on, capped by the rest of the row —
and the AND of the row above that one's `match[0]` over the same span.

A binary "did the run reach the end" decision is coded through `esc_ctr[]` at

```
8·run_bucket[len] + 4·(two rows up, the two records past the run both matched)
                  + 2·(the north-north AND) + alpha_map[N] + 1
```

`alpha_map` is one byte per symbol, all 1 at the start, holding whatever that
symbol's last run escape decided — a one-bit-per-symbol memory of how runs at
this symbol have been going.

If the run reached the end, the whole scan length is taken at once. If it did
not, and the scan reached more than 1, the length is coded by
`code_run_length<f_DEC>`: **one bit per bucket level from the top down**, and a
bit is coded only at a level that could still belong to a run shorter than the
cap. The counter is

```
run_ctr[ 16·((no higher bit set yet) + (still the bucket the scan started in)) + bucket ]
```

— `run_ctr[48]` is three groups of sixteen. Afterwards `fill_run` copies the
symbol, head word and flag word of the record before into every record the run
covers and `seed_after_run` primes the one past it.

**Rung 2 — the four-way match.** The pixel is coded against the five-slot
alphabet of §8.3 in the `FreqRec` the bucket names. `find_level` walks the five
weights to find which holds the target, `cum_below` sums the weights beneath a
level to give the coder its `cum`, `bump` adds `b15` to that level and the
total, and a rescale halves all five when the total passes `w[6]` or 0x4000 —
with the escape floor keeping the escape reachable.

`cum_below` is a four-case `switch` rather than a loop, and the fourth case is
worth a look. `w[5]` is the record's running total — `resum` is the one line
that recomputes it as `w[0..4]` summed — so level 4's cumulative
weight — the sum of `w[0..3]` — is taken as `w[5] - w[4]`: the total less the one
weight above it, rather than four additions that reproduce a number already
stored.

Two `FreqRec`s are in play, a per-bucket one and a per-context one, and when the
latter is fresh `blend_from` seeds it from the former: the incumbent weights
scaled by the donor's increment, the donor's own weights put in their place, and
21 parts of the donor's total added back rounded up. A new context starts with a
prior instead of a uniform.

Under the bucket runs a chain of **lazily interned context identifiers** —
`intern_ctx` mints a dense id per distinct signature in first-seen order:

- `ctx_id1` is indexed by ten bits **plus the bucket above them**: four "is this
  gradient flat" bits at 4…7, two "did four rows above all match" bits at 8…9,
  four more match bits at 0…3, and `bucket << 10`;
- `ctx_id2` refines that id with three more match bits, `sig2 = 8·id1 + 3 bits`;
- `ctx_id3` refines *that* with the **low nibble of W** — but only when the
  alphabet is under 32 symbols, and past 53248 ids the nibble is forced to 15,
  merging those sixteen contexts rather than allocating past the table.

`context_ids` is the function that builds all three, and the four gradients the
first of them reads are running counts from `match_seed`: "how many of the last
*n* records matched on this lane, less *n*", so **zero means all of them
matched**, and each slides by one add and one subtract per pixel.
`match_seed_split` is the same count over five records that are *not*
consecutive — −3, −2, +2, +3, +4, stepping over the three around the pixel being
coded, which are the ones not yet known.

**Rung 3 — the escape ladder.** `exclude_stage_one` runs first, marking the four
ranked neighbours and `no_symbol` excluded — rung 2 has just failed to be any of
them, so offering them again would cost bits to say no twice. Then
`seed_candidates` assembles 32 candidates and `offer_candidates` walks them:

| slots | from |
| --- | --- |
| 0, 1 | the last and previous symbol at this state and key |
| 2 … 9 | the eight-entry per-key cache `sym_ctr[8·key]` |
| 10 … 31 | twenty-two spatial neighbours |

`pair_key` is what indexes those first ten: **the north neighbour's `sym_rev`
entry less whichever of W, NE, NW first disagrees with it**, and less the second
neighbour west when all four agree. `sym_rev` being a bit reversal times eight,
that is a hash mixing N's bits against a neighbour's value.

The twenty-two spiral outward, and the order is the order they are asked in, not
raster order — writing `r5` for the current row and `r6`…`r9` for one to four
up:

```
10 r5[-2]  11 r6[+2]  12 r7[+1]  13 r7[ 0]  14 r6[-2]  15 r7[-1]
16 r5[-3]  17 r6[+3]  18 r6[+4]  19 r5[-4]  20 r6[-3]  21 r7[+2]
22 r8[ 0]  23 r7[-2]  24 r5[-5]  25 r8[+1]  26 r6[+5]  27 r9[ 0]
28 r5[-7]  29 r8[-1]  30 r6[+7]  31 r7[+3]
```

Slots 11–15 are the **near band** and 16–31 the **far band**. `pixel_context`
declines a candidate outright in three cases: it is already excluded; it is in
neither band and the ladder is past position 6; or it is past position 14 and
the list evidence has neither of its two strongest bits. For anything else a
binary decision is coded through `bit_node[]` against `bit_root[]` at a context
made of: is this slot 10 (the immediate neighbour), near-band hit, far-band hit,
is the candidate in the top ten of `sel0_list[N]`, in the top four of
`sel0_list[W]`, does the candidate's own list hold W in its top ten, is it in
the
top six of `sel0_list[NE]` — and the ladder position above all of it.

Those five "in the top *n* of" questions are all `sym_in_top`, a linear scan of a
list's first *n* entries. Since a `SymList` is kept in descending count order,
"in the top ten" is "among the ten most frequent symbols seen in that
neighbour's list", so the context is asking how well the candidate agrees with
what the neighbourhood has been doing — using the lists as evidence without
coding anything from them.

A hit ends the pixel. A miss adds the candidate to the **exclusion mask**, which
is a generation counter rather than a cleared array, so it costs nothing in
every
later step *and* in rung 4.

**Rung 4 — the symbol lists.** If all 32 miss, `load_selectors` points at
`sel0_list[W]` and `sel1_list[N]` and the walk resumes from `sel_cur` — a cursor
that **persists across pixels**, so a pixel whose predecessors kept escaping
does
not pay to re-walk the lists that failed. Past the escape list is
`bmf_fatal(bmf_read_error)`; the escape list is dense over the whole alphabet
and
always has the symbol.

### 8.6 `SymList`

An array of `(symbol, count)` pairs in descending count order.

Coding a symbol walks the live entries accumulating the counts of those **not
excluded**; a hit codes `(cum − c, cum, tot + total)` and then `promote`s the
entry — plus 4, bubbled toward the front while it outranks its neighbour — and
rescales if that is due. A miss codes the whole accumulated weight as the escape
and marks every entry in the list excluded on the way out, so the next list does
not offer them again. A list whose live entries are all excluded codes nothing
at
all.

`decode_symbol_list` is the reading half, and its shape is the one thing about
`SymList` that is not symmetric: the encoder knows its symbol and sums the
weights below it, while the decoder has to build **a pointer per symbol still in
play** — the live entries that are not excluded, null-terminated — walk them
against the point the coder gave it, and then fold the answer back. The bound on
that array is `no_symbol + 1`, and an instrumented build measures the high-water
mark over the corpus at 257: a 256-symbol list with every entry live, plus the
terminator.

Rescale runs when `since_rescale` passes `rescale_at` **or** the moment any one
count reaches 252. It halves every count — with a rounding bias of one when the
threshold is below `20·n` — and re-sorts as it goes. **Counts that reach zero
are
dropped**: the tail is walked back, `live` shrinks by however many died, and the
escape weight `tot` gains one for each. The list forgets, and a forgotten symbol
costs an escape to say again. `tot` and `since_rescale` are then halved
themselves.

Lists come in two flavours: **dense** (`init(n, 1)`) starts with every symbol
present at count 1, **sparse** starts empty and grows. The per-symbol selector
lists are sparse with capacities 99 and 33; the escape list is dense.

### 8.7 what happens between pixels

`init_tables()` runs after **every** pixel and does seven things:

1. **feed the selector lists** — on an escape, four `add_weight` calls pairing
   the coded symbol with N and W (weights 3, 4, 2, 1 depending on where the
   search stopped), plus an insert at count 2 into every list the walk passed;
   on a hit below the top two ranks with NE ≠ NW, a single `add_weight`;
2. **bump the exclusion generation**, on the escape path only, zeroing the whole
   mask when the counter would wrap;
3. **move-to-front the eight-entry symbol cache**, unless the symbol was already
   in the top two. Only slots 1…6 are searched, so a miss lands on slot 7 and
   "not found" and "found at the end" are the same store;
4. **roll the context pair**: `prev = last`, `last = sym`;
5. **write the six `match[]` flags** of the record just coded — the only place
   they are written;
6. **advance all five row cursors**;
7. **slide the four gradients**, one add and one subtract each.

The row loop adds `start_row`, which rotates the five buffers by one and seeds
the new row's flags against symbol 0, and — on the encoder only — a pass that
copies the row's reduced-alphabet symbols out of `sym_word` into the records
before any of them is coded. The decoder cannot do that, which is one of the two
places the two directions genuinely differ.

---

## 9. Model B — the alternate model for predictor 1

`alt_p1_block.inc` holds the block, `alt_p1.inc` and `alt_p1_code.inc` the two
drivers. Selected by descriptor flags 5 or 13.

Where model A asks "which neighbour is this", this one **predicts a value and
codes the residual**, and everything in it is arithmetic on magnitudes. It has
two drivers, and which runs is §6.4's planar/interleaved answer:

- `alt_model_p1_d8_encode` / `alt_model_p1_d8_decode`, one plane at a time
  through `AltP1Block::d8_body<f_DEC>` — what `t24` and `t32` use;
- `alt_p1_code.inc`'s `alt_model_p1<f_DEC>`, **all planes at once**, one
  `AltP1Block` each, coding plane 0, 1, 2, 3 of a pixel before moving on. Each
  block is handed the two planes before it as `nb0`/`nb1`, so its context can
  see what the other planes just did — which the per-plane driver cannot — and
  it applies the colour transform itself, per pixel, rather than up front.
  `testfiles/altp1.bmp` is the image that reaches it.

Each block keeps five rows of `P1Ctx { uint8_t sym, mag; }` — the sample and the
magnitude of its residual — with cursors four records in and the borders seeded
to `sym = 72, mag = 0`.

### 9.1 the prediction

`ctx_of` computes the **MED / LOCO-I gradient predictor**, the same one JPEG-LS
uses:

```
NW ≥ max(W, N)  ->  pred = min(W, N)      a vertical edge left of the pixel
NW ≤ min(W, N)  ->  pred = max(W, N)      a horizontal edge above it
otherwise       ->  pred = W + N − NW     a plane through the three
```

The source writes it as a nest of comparisons that fall through to `N`, which is
the same function: the fall-through cases are exactly the two where NW is
outside
`[min(W,N), max(W,N)]` and N is the answer.

### 9.2 the context

An **activity measure** is accumulated from the `mag` fields of thirteen
neighbours at fixed weights — `6·W`, `4·(N + W2)`, `3·(NE + NN)`,
`2·(NNE + NE2 + W4)`, and the singles — plus contributions from the neighbouring
planes when the interleaved driver supplies them. That sum is quantised
`(act + 7) >> 4` into 0…511 and looked up in `level_of[]` and `group_of[]`, two
step functions built at set-up time from the edge lists

```
p1_level_edges = 1, 2, 4, 8, 14, 35, 103        eight activity levels
p1_group_edges = 1, 3, 6, 10, 16, 27, 52        eight groups, spaced differently
```

— roughly logarithmic ladders, and deliberately not the same ladder. A third
table, `p1_level_step = 1, 1, 2, 2, 2, 4, 4, 4`, turns the level into the
deadband width two of the features below compare against, so the quantiser gets
coarser exactly where the neighbourhood is busier.

`alt_p1_alloc` builds all three step functions, per plane, by one pass over 256
activity values carrying a running level and group and stepping each when the
value reaches its next edge — so the tables in `tables.inc` are edge lists and
the step functions are derived, not stored. `group_of` carries the plane index in
its high byte (`plane << 8`), which is how one counter array serves four planes
without their statistics mixing. The same pass builds `slot_of` over 256 values
of a separate `p1_slot_edges` ladder, times 8.

That function also seeds the counters: `init_counter_node` over all **629,856**
of them, each starting `{8, 2, 2, 2, 2, 3, 3}` with a total of 22 — a prior that
favours the first slot eight to two and the two escape slots three, rather than a
uniform. Model B's whole context space is initialised before its first sample.

On top of it, **nine ternary features**, each 0, 1 or 2:

| feature | compares |
| --- | --- |
| `ctx_w[0]` | the prediction against 22 and 216 — is this near the range ends |
| `ctx_w[1]` | NW against N |
| `ctx_w[2]` | NW against W |
| `ctx_w[3]` | a reference plane's own residual, or NN against the prediction |
| `ctx_w[4]` | NE against the prediction, with the activity step as a deadband |
| `ctx_w[5..8]` | second-order gradients — `2W − W2 − pred`, `2N − NN − pred`, `NE + W − N − pred` — or, with reference planes present, cross-plane gradients and their residuals |

The five that depend on how many other planes are visible are genuinely three
different formulas for two references, one, and none.

The nine combine as a **mixed-radix base-3 index**. `CtxWeight` holds `w[3]` per
feature with the three values pre-scaled to `32 · 3^k`, and the counter index is

```c
ctx[0] = Σ ctx_w[k].w[sel_k]  +  16·quiet  +  8·(ctx[3 + ctx[2]] == 0)  +  act_lvl;
```

`ctx[3]` and `ctx[4]` are a **second** activity measure, separate from the one
above and kept per column parity. `seed_activity` builds both at the start of a
row, each from ten `mag` fields — four from two rows up, four from four rows up,
and two from the current row, all at that parity's stride — and `record_sample`
then slides each along by one add and three subtracts per sample, flipping
`ctx[2]` so the two parities alternate. The whole pair is read in exactly one
place, the `== 0` above: all that survives of twenty fields is one bit, "has this
column parity been completely flat".

That is worth knowing because it settles a question the source cannot. Two of
the three coders read those `mag` bytes as `int8_t` and one as `uint8_t`, so a
magnitude of 128 or more gives sums 256 apart — and it does not matter, because a
sum is only ever tested against zero and both readings are zero together.
Measured over the corpus: the generic coder runs `seed_activity` **18,224** times
and **1,212** of those calls read at least one `mag` at or above 128, so the
difference is exercised heavily and still cannot reach the output. The casts stay
as the binary has them.

(`alt_p1_block.inc` gave those as 3,216 and 70, which were stale by more than
five- and seventeen-fold. Re-measured for this document by counting calls and
their operands, and corrected in the source in the same commit. The conclusion
never depended on the size of either number — a sum compared against zero is
settled by any count above zero — which is exactly why nothing noticed them
drifting.)

so `counters[]` has 32 · 3⁹ = **629856** entries. The low five bits are *not*
just the activity level: bits 0–2 are `act_lvl` (0…7), bit 3 is a small state
flag, and bit 4 is `quiet` — "the neighbours this plane can see are all still",
which is the reference planes' west magnitudes under 16 or 8, or all five of
this
plane's own magnitudes exactly zero. Adjacency in activity is what the update
step exploits.

`ctx_of` returns a **second** index too, and it does not select a counter:

```c
ctx[1] = group_of[act_q] + slot_of[pred];
```

`slot_of[256]` buckets the *predicted value* — a residual around 8 behaves
differently from one around 200 — in strides of eight, and `ctx[1]` selects the
binary tree the escape walks (§9.3).

### 9.3 coding a residual

The residual is **zigzag folded** by `alt_init_tables` — 0 → 0, −1 → 1, +1 → 2,
−2 → 3, +2 → 4 — so small residuals become small codes, and `fold` and `unfold`
are exact inverses. Then two stages:

1. **`CounterNode::code_symbol`** codes the code's *slot* against seven counts.
   `c[0..4]` are the codes 0–4 directly; `slot = 6 − (sym & 1)` sends an odd
   code
   ≥ 5 to `c[5]` and an even one to `c[6]`.

   The node's total is read as `total & 0x7FFF`, because **bit 15 marks that the
   array has been rescaled at least once** — rescale sets the new total to
   `0x8000` plus the counts so the mark survives. Rescale fires above 0x2000 and
   picks its divisor from the *smallest* count: **halves** if that has fallen to
   1 or 0, **thirds** otherwise, because thirding something already at 1 would
   round it away. The coded slot then gains 32, and so does the total.

2. Slots 5 and 6 **escape into a binary tree**. `code_symbol_tree` walks
   `level_geom[]` coding one bit per level through `FreqPair` counters, with the
   level chosen by `model_geometry[code]`, which is what makes a large residual
   cost logarithmically. The strip is
   `model_strip(128·(slot & 1) + escape_bias + ctx[1])`, so the parity picks one
   of two strips and the activity group and predicted value pick the place in
   it.
   `escape_bias` is a sign test folded to 0 or −64 on
   `(total & 0x7FFF) + c[0] − 2·c[slot]`.

### 9.4 the update, which is where the model earns its keep

`update_model` does not update only the context that coded the symbol.

It folds the residual **both ways**: `code_f` from `sample − pred` and `code_r`
from `pred − sample`. Then

- the **alternate context** — every feature read in the opposite direction,
  `ctx_w[k].w[2 − sel_k]`, with feature 0 forced to its middle value and the low
  five bits kept — is bumped by **17** for the reversed code. That is the model
  learning the mirrored statistic for free, on the assumption that an image with
  a bias one way has the opposite bias somewhere else.
- the context **one activity level up** is bumped by 11 and the one **down** by
  13, both for the forward code, each guarded by whether that level exists
  (`(ctx[0] & 7) != 7` and `!= 0`). If the slot escaped, that neighbour's tree
  strip is updated too.
- and when the coding context's own total is still under 0xCCC — a context that
  has not settled — the **alternate context's neighbours** are bumped as well,
  at
  7 and 5. A cold context borrows from a warm one, in the mirror.

Then `update_selector`, nine times — once per ternary feature. Each call walks
three counters: the feature read the *opposite* way, the middle value, and an
alternate context built from two of the three weights. Which two is the only
thing across the nine that is not uniform: **slot 0 pairs `w[0]` with `w[1]`
where the other eight pair `w[1]` with `w[2 − sel]`.** The same one-weight shift
appears in `ctx_alt`'s slot-0 term, so it is a parameter of the family rather
than a special case invented to close a loop.

A feature already at its middle value (`sel == 1`) is the symmetric case: the two
outer values sit either side of it, so **both** are bumped, and equally:

```
sel == 1     lo, hi          6      forward code
             their level +1  4      when that level exists
             their level -1  3
sel != 1     w[2 - sel]      7      forward
             w[1]            6      forward,   +1 -> 4,  -1 -> 3
             ctx_alt         4      REVERSED,  +1 -> 2,  -1 -> 2
```

The third line of the off-centre case is the one that matters: the alternate
context is bumped with the **reversed** code, not the forward one, at half the
weight the middle gets. So every coded sample teaches nine mirrored features as
well as its own — and the increments across the whole update, 17, 13, 11, 7, 6,
5, 4, 3, 2, are the model's own ranking of how much each of those neighbours'
evidence is worth. None of them is adaptive; they are nine constants in the
binary.

### 9.5 near-lossless machinery, inert in this build

Every encode path carries a drift check:

```c
recon = unfold[code] + pred;
out   = recon + *dst - want;
if( |*dst - out| > 16 ) code = fold_hi[resid];   // re-send through the coarse table
else                    *dst = out;
```

With `-S` — the only mode this build writes — `near_lossless_q` is 0, so
`alt_init_tables` builds `fold` as the exact inverse of `unfold`, `recon` always
equals `want`, the drift is always zero and the `fold_hi` branch cannot fire. It
is the `-E` near-lossless path, kept because it is what the donor does.

It appears in two forms, and they are **not** the same decision. `fold_or_refuse`
compares the reconstruction against a byte in a *different* buffer — the source
and the destination are separate there — while `encode_sample` reads one byte
twice, before and after the context step, so what it compares against is the
byte it is about to overwrite. Same five lines, different operands, and a merge
that assumed otherwise would be wrong in the near-lossless mode this build
cannot reach. `encode_sample` also carries the coding call itself, so it is the
whole per-sample body of model B: fold, drift-check, `alt_p1_encode_symbol`,
return the reconstruction.

---

## 10. Model C — the alternate model for predictor 2

`alt_p2_block.inc`, `alt_p2_context.inc`, `alt_p2_model.inc`, and the two
drivers in `alt_p2.inc` and `alt_p2_encode.inc`/`alt_p2_decode.inc`. Selected by
flags 6 or 14, and the most expensive of the three — `alt_p2_model.inc` holds
the largest body in the program.

Where model B predicts with a fixed rule, this one **learns its predictor**, and
then learns which learned predictor to trust.

### 10.1 an NLMS linear filter

Each pixel carries a `P2Ctx`: eight 16-bit fields — `val`, `dval`, `err` against
the running prediction, `aerr` its magnitude, and the four differences `dleft`,
`dup`, `dupleft`, `dupright` — plus two bytes, `sign` (a ternary
`(resid ≤ dead) + (resid < −dead)`, which side of the deadzone the residual
fell)
and `mag` (its absolute value). The two bytes are what the context quantiser of
§10.2 sums over; the 16-bit fields are what the filter reads.

Those become a **7×4 feature matrix** `p2_row[7][4]`. The prediction is
`centre + Σ w[j][k]·p2_row[j][k]` over all 28 taps — that sum is `nb_dot` — and
one tap's update is a **normalised least-mean-squares** step, `nlms_step`:

```c
ms          = w[7+j][k] + (x·x − w[7+j][k])·ms_rate;   // running power of the tap
w[7+j][k]   = ms;
w[j][k]    += bmf_p2_rate[j][k] · err · x / (ms + floor);
```

`w[j]` and `w[7+j]` are one tap: the weight and its own running power. Dividing
by that power is the whole of what "normalised" means. The per-tap rates in
`bmf_p2_rate[7][4]` run from 0.0108 down to 0.0009 — twenty-eight of them, the
nearest neighbours adapting fastest.

The step happens in **three places with three different rates**, which is what
makes this a mixer rather than one filter:

| caller | row | `ms_rate` | `floor_` |
| --- | --- | --- | --- |
| `nlms_predict_and_correct` | the set that predicted | `bmf_p2_rate[j][k]` (implicit) | `w[14][2] · 529` |
| `nlms_track_two_rows` | the **fast** row, against this sample's error | 0.05 | `w[14][2] · 26896` |
| `nlms_track_two_rows` | the **slow** row, against the confidence-scaled error | `0.013 · conf` | `w[14][2] · 5041` |

The floor is not a constant either: `w[14][2]` decays toward 10 at a rate of
0.0002, so a set that has been used a great deal ends up dividing by a smaller
floor and moving further per sample.

An `NbRow` is `float w[15][4]` plus a use count: rows 0–6 the weights, 7–13 the
mean squares, 14 the scale and confidence, and the count where row 15's first
slot would be. A `static_assert` holds it to the size of a `float[16][4]`,
because `NbRow::predict` is handed `p2_row` and writes `nb[7][0..2]` — one row
*past* the end of a `float[7][4]`, landing in the `bias[4]` declared immediately
after it. That is how the filter returns four things through a signature that
names one, and the adjacency is asserted with `offsetof`.

**The rate and coefficient tables are not constants.** `P2Coef::fold` runs at
the
top of an image and `restore` at the bottom, and between them `bmf_p2_coef` and
`bmf_p2_rate` have been edited in place: coefficient rows 4–6, the cross-plane
terms, are folded into rows 0–2 and zeroed, and three rate rows are set to
`bmf_p2_rate_reset`. A single-plane image and a multi-plane one are running
different filters out of the same table, so the declared values describe the
interleaved path and not the `d8` one.

### 10.2 which filter — 1088 of them

Not one predictor but **1088**, chosen by a quantised description of the local
texture. `alt_p2_context` accumulates four directional activity sums — up-left,
up-right, left, up — each a fixed-weight combination of eight neighbours'
difference fields, and derives five small numbers. Four of them are
`over_thresholds`: how many of a run of thresholds a value clears, optionally
against a multiple of each. (`ctx_quant` is the same counting for the run and
sum contexts of §10.4, packed as two two-bit fields.)

| | from | range |
| --- | --- | --- |
| `band` | the left sum against the up sum, over `p2_band_edges[5]` | 0…5 |
| `gA` | total activity, four thresholds | 0…4 |
| `gB` | a local value estimate, three | 0…3 |
| `gC` | the up-right against up-left ratio, three | 0…3 |
| `gD` | a four-neighbour average, three | 0…3 |

Every threshold but the band's comes from `bmf_p2_thresholds[band]`, so **the
quantiser itself depends on the band**. The slot is

```
nb_slot = 320·band + 64·gA + 16·gB + 4·gC + gD
```

with a maximum of 1919, which is `nb_id[1920]`; `nb_id` maps slots to weight
sets lazily, minting one on first use.

A second linear stage, `NbRow::predict`, blends **six** weight sets — the six
pointers in `CtxWeights::row`, each a 7×4 matrix — using the six coefficients of
`bmf_p2_mix[mode]`, one row of a 4×6 table. `bmf_p2_coef` is separate and
earlier: it establishes the centre value the features are measured against.

`predict` hands three numbers back through `bias[0..2]`: the centre the features
are measured against, the **mixed** prediction, and the set's **own**. The two
predictions are then reconciled by a confidence that is a running least-squares
weight, tracked in row 14 at a rate of 0.001:

```c
ms_a = w[14][0] + ((sample - own)·(mix - own) - w[14][0])·0.001;   // covariance
ms_b = w[14][1] + ((mix - own)²          - w[14][1])·0.001;        // variance
w[14][1] = ms_b;
w[14][0] = clamp(ms_a, 0.1·ms_b, ms_b);
conf     = (1 - w[14][0] / (ms_b + 576)) · 2;
```

`w[14][1]` follows the variance of the disagreement between the two predictions
and `w[14][0]` its covariance with the sample's own error, clamped into
`[0.1·ms_b, ms_b]` so the ratio cannot run away in either direction. `conf`
lands
in [0, 2] and scales both the correction fed to `nlms_track_two_rows` and the
step it takes, so a set whose disagreement with the mix carries no information
about the error is corrected less, not more.

### 10.3 coding the residual

Folded as in §9.3 and coded through `P2Freq`, a **three-way counter** — the
symbol, escape odd, escape even — whose escapes go into the same binary level
tree model B uses. Each count is bumped by *sixteenths* of the record's `step`,
and the file uses exactly eight numerators: 2, 3, 4, 5, 6, 7, 10 and 13.

`step` decays as a context settles, and its ladder is not monotonic:

| `step` | `rescale_three_way` |
| --- | --- |
| > 256 | halve |
| 33 … 256 | subtract 32 |
| 17 … 32 | **add 2** — `((16 − step) >> 30) & ~1` is −2 here, and subtracting it adds |
| ≤ 16 | leave it |

So a settling context walks down in 32s, bounces in a narrow band around 32 for
a
few rescales, and comes to rest at 16 or below, which is a fixed point. The
three
counts halve on every rescale regardless, and rescale fires when they total more
than 29696.

### 10.4 the counter cascade, and what the search turns off

Underneath sits `p2_ctr[163840]` — **five banks of 32768 counters**, each bank
addressed by a different context derived from the same neighbourhood. Each holds
a `weighted` accumulator and an adaptive `rate`. `alt_p2_alloc` seeds every one
of the 163,840 at `weighted = 0, rate = 5, b1 = 2`, and all 15,552 `P2Freq`
records at `f = {2048, 2816, 2816}` with `step = 4096` — so the three-way counter
starts biased **against** the literal symbol, 2048 against 5632 for the two
escapes together, and the step starts at its maximum and only decays. It also
sets the deadzone from `near_lossless_q` (which is 0, so `±1`) and the band to
`[−7, 8]`.

This runs **per plane**: 163,840 counters and 15,552 records re-seeded four times
for a 32-bit image, which is a large part of why this is the expensive model.

**The five banks are a cascade, not five opinions.** Each one predicts a
*correction* to the running prediction, and the next one's context is built from
the corrected value:

```
run_s = filt                    the NLMS prediction of §10.1
ctx0 -> bank0 -> pred0          run0 = pred0 + run_s
ctx1 -> bank1 -> pred1          run1 = pred1 + run0
ctx2 -> bank2 -> pred2          run2 = pred2 + run1
bank3         -> pred3          run3 = pred3 + run2
bank4         -> pred4          run4 = pred4 + run3
```

Every term of `ctx1` carries `run0`, every term of `ctx2` carries `run1`, and so
on down — which is why none of these can be reasoned about from the types alone,
and it is the single most important fact about how this model is put together.
The ten intermediate values are kept in `nb_sum[0..9]` as five (running,
correction) pairs, and `run4` is what the context selectors of §10.2 finally
read.

Each bank context has the same shape: **eleven single-bit features at bits
15…25, and `ctx_quant`'s two two-bit fields at 11…14**, then `>>11` to index the
bank. That leaves fifteen bits, and 2¹⁵ is 32768 — exactly one bank. The eleven
features are differences between the running prediction and various neighbours;
each contributes whichever bit of that difference sits at its position.

Those are **not** sign tests, and the source says so in its spelling. A term the
types prove negative-or-not is written `bit<k>(expr < 0)`; a term that merely
contributes bit *k* of something unbounded is written `bit_of<k>(expr)`. Across
the six context words there are 66 such terms and **exactly one** is provable —
because every other one carries a `run`, which is an accumulated prediction with
no type bound. Instrumented over the corpus, four of them demonstrably exceed
their bit: `d_run0` reaches 36157 against bit 15. So a bank context is not
"eleven signs of eleven differences"; it is eleven arbitrary bits, and the model
works because the counters learn whatever those bits happen to correlate with.

After each sample:

- the bank's own counter takes the residual through a **deadzone**, so small
  residuals do not move it;
- its rate accelerates: `b1` counts down, and each time it reaches zero the rate
  index steps up and the accumulator doubles — a context that keeps seeing the
  same thing sharpens;
- and, **unless `alphabet_reduced` is set**, the counters at
  Hamming-distance-one
  contexts are updated too. `bump_bank` walks eleven context bits, and for each
  bit updates three counters: `direct` at the context with that bit flipped,
  `mirror` at the same one with the top bits flipped as well
  (`ctx ^ bit ^ 0x7FF0`), and `rot` at the context with its low nibble rotated
  through `p2_ctx_rotate`.

`code_banks` is the loop over the five, `bump_bank` one bank's worth, and
`walk_bank_bits` the eleven-bit pass inside it. That last one is where the
asymmetry lives: it skips `direct[0]` and `rot[0]` at the start and `mirror[10]`
at the end, because those three are bumped by the caller — before the branch
that chooses between the two shapes, and after the two rejoin. So the eleven
bits are not treated eleven identical ways; the first and last are handled
outside the loop, and the middle nine inside.

One flag distinguishes the three arms of `code_banks`, and only one: whether the
record *after* the direct one is nudged as well. Everything else in the five
lines is the same.

Four primitives do the arithmetic, and they exist separately for reasons the
call sites forced:

| | what it does |
| --- | --- |
| `p2_pred` | the bank's own prediction, two shifts of the accumulator by the rate |
| `p2_bump` | accumulator plus error, shifted — the whole update in one line |
| `p2_update` | the two together, for a caller that does not need the residual back |
| `p2_update_into` | the same, but it subtracts the prediction **into** the caller's residual by reference, because two sites read that residual again below |

`p2_nudge` and `p2_rescale` are the two halves that could not be folded in:
two of the fifty sites in `alt_p2_model` compute the increment and then add it
*conditionally*, so the add cannot move into a helper even though the guard can.

`add_bias` is the other accumulator running alongside all of this — 32 times each
of a neighbour's four difference fields, summed into `ctx_bias` and read only by
`alt_p2_context`. The encoder writes it back one plane late, carried in four
locals; the decoder adds after every plane. That is MSVC's scheduling and not a
difference in the model, because the next sample reloads `ctx_bias` from the
total before anything could observe it.

That neighbour smoothing is most of the cost of this model, and §6.5 is where it
gets turned off: the search measures the model without it.

### 10.5 rows and borders, in both alternate models

Neither alternate model special-cases an edge in its prediction. Both keep a
**ring of history rows with margins**, and handle the borders by *filling the
margins with mirrored records* before the row walk starts — so every pixel,
including the first one of the first row, reads a full neighbourhood and the
predictor has no branches in it.

Model B does this in `advance_row`, once per row: copy the six records before the
cursor forward over the six after it (mirroring the right margin of the row just
finished), rotate the four-row ring with `ring_advance`, carry one record across
the seam, and mirror three more back over the left margin.

Model C splits it three ways, driven by `alt_p2_start_row`:

| row | what runs |
| --- | --- |
| 0 | `seed_row0` — every record in the top row set to a **neutral invented value**, then `copy_row0_down` duplicates it into the three rows behind |
| 1 | `seed_history` — mirror the coded first row into the margins, then `copy_row0_down` again |
| any | `start_row` — the per-row cursor reset |

`seed_row0`'s constants are the interesting part, because they are a prior and
not a zero:

```
val 256   dval 256   err −16   sign 1   mag 3
aerr 512  dup 1024   dleft 256  dupleft 512  dupright 512
```

A neighbourhood that has never been seen is described as *moderately active with
a small negative error*, not as flat. Seeding it flat would tell the context
quantiser that the top of every image is a smooth region, and the first row is
exactly where that is least likely to be true.

`code_sample` is the per-sample switch — `encode_sample` or `decode_sample` —
and is what `alt_model_p2_d8`, `alt_model_p2` and their model B counterpart
`alt_model_p1_d8` drive in their row loops. Those three templates are the bodies
behind the six entry points of §7.4: each is one `template<int32_t f_DEC>` with
its encode and decode instantiations named separately.

---

## 11. The decoder

`image_expand.inc` makes no decisions. It reads the magic and the sixteen-byte
header, refuses anything that is not version 2.0 with `flags_slow` set,
allocates
from the header's own dimensions, and then:

1. if `flags_coded` is clear, read `data_size` raw bytes and stop;
2. otherwise `expand_coded` reads the whole coded stream into memory. It answers
   false when the file is shorter than the header promised, and again when the
   stream does not end **exactly** where its length said — the caller turns
   either into `fail()`;
3. if the depth is 4 or below, or `flags_descriptors` is clear, `unmodel_plane`
   runs once over the whole image with predictor 0 and the main model;
4. otherwise `unmodel_described` reads the near-lossless nibble and
   `read_plane_descs` unpacks one descriptor per plane through the bit packer of
   §5;
5. the planes come back one of two ways. `unmodel_planes_apart` (planar) walks
   `plane_desc[k].src_plane` in coding order, decodes each through a header
   claiming one eight-bit greyscale plane, undoes its predictor, and interleaves
   it back. `unmodel_planes_together` (interleaved) decodes the whole image in
   one pass and then — unless the alternate model coded it, in which case there
   is nothing left to undo — takes each plane out, undoes its predictor, and
   puts
   it back;
6. if `flags_transposed`, transpose;
7. rebuild the grey ramp if `depth & 0x40`, or read the stored palette.

There is a bounds check before the transpose that is worth noticing: the
transposed walk covers `width · height · plane_count` bytes, which is only the
buffer's size when the image is one byte per plane per pixel. For a packed image
it is up to eight times it, so the decoder refuses rather than walking off the
end.

**The decoder's work is the encoder's code.** Fourteen encode/decode pairs in
this tree are one `template<int32_t f_DEC>` each — the counter updates, the
context computation, the tree walk, the colour transform, the run-length walk
are
literally the same lines with the direction as a template argument. `bmf.cpp`'s
header lists all fourteen with the line counts each half had.

Two pairs stay two functions, and each has a reason that is structural rather
than cosmetic:

- **`code_pixel` / `decode_pixel`.** The encoder knows the symbol and asks where
  it ranks; the decoder knows a point in the range and asks which rank holds it.
  Everything before that divergence has been named and lifted out —
  `open_pixel`, `run_scan`, `code_run_length<f_DEC>`, `decode_stage_two`. What
  is left of the two bodies shares thirteen lines out of two hundred and
  eleven.
- **`predict_med` / `unpredict_med`.** The forward pass walks backwards so each
  prediction sees unmodified neighbours; the inverse walks forwards so each sees
  reconstructed ones. A template over a direction flag would have to reverse
  three loop headers and two edge cases, which is writing both bodies out again
  with an `if` around each line.

---

## 12. What is in the source and cannot happen

The command line is pinned to `-S -Q9`, and three things are visible in the
program that this build cannot reach:

- **fast mode.** `desc_slow_mode` is always 1 and `flags_slow` always set. The
  decoder refuses a stream without it rather than implementing it.
- **near-lossless (`-E`).** `near_lossless_q` is pinned to 0, so the `fold_hi`
  table, the drift checks of §9.5 and the deadzone widths that read it are all
  inert.
- **quality below 9.** `opt_search_quality` is a `constexpr` 9.

That last one needs a caveat the other two do not, and it applies to all six
`opt_*` constants in `globals.inc`: **nothing reads any of them.** Not
`opt_use_filters`, `opt_slow`, `opt_filter_template`, `opt_pack_output`,
`opt_search_quality` or `opt_max_error`. The mode-folding passes replaced every
read with the folded behaviour, so what is left is a record of the settings this
build is pinned to and not a set of switches. Changing one changes nothing.

---

## 13. How this document was checked

Everything above was read out of the source. Eight things were checked by running
something rather than by reading it, and they are the eight a careless reading
would get wrong:

- **the packed-row claim of §2.1** — read out of the `stride` field of
  `testfiles/ref_*.bmf`, which is the encoder's own answer;
- **the flag values of §4.1** — read out of byte 15 of the same files;
- **the sign of `escape_bias` in §5.3** — the expression compiled and evaluated
  on both sides of the half-mass boundary. It returns 0 and 64; the source
  comment said 0 and −64, and gave the two cases the wrong way round. Reading it
  is what produced the wrong answer twice, because `>>25` looks like a sign test
  and is not one here. Corrected in `bitctr.inc`;
- **the level geometry table of §5.4** — the recurrence in `begin_plane_stream`
  and the tree walk in `code_symbol_tree` were run against each other to get
  `tbl_base` and the highest index a walk touches. That is where the four unread
  `uint16_t` at the top of each strip came from; a first draft of the sentence
  claimed the trees tiled the strip exactly, which the numbers do not say;
- **the two rounding constants of §7.1** — found by reading `plane_mix3` beside
  `PlaneTransform::blend`, then checked for reachability by counting calls to
  both over all nineteen corpus images. `plane_mix3` is entered by three of
  them, so the `+64` is live and not a path the program never takes;
- **the fifteen partitions of §8.3** — derived from the six bit definitions, and
  then confirmed by instrumenting `match_context` to record every signature it
  produced over all nineteen corpus images. No signature outside the fifteen
  occurred, which is what makes "the other 49 entries are never read" a
  statement about the program rather than about the corpus;
- **the candidate bound of §8.6** — `decode_symbol_list` instrumented to record
  the widest candidate array it ever fills, over every reference stream. 257,
  against a declared capacity of 8193;
- **the two counts in §9.2** — `seed_activity` instrumented over all nineteen
  images: 18,224 calls, 1,212 of them reading a magnitude at or above 128. The
  source comment said 3,216 and 70. Both were stale, by more than five- and
  seventeen-fold, and neither had ever been checked.

Two of the eight came out different from what was written down, and they differ
in an instructive way. The last one drifted: it was true when measured and the
corpus grew underneath it, and nothing failed, because the conclusion it supports
— a sum compared against zero — does not depend on the size of the number. A
figure that does not carry its own argument can rot silently.

The other three were never true. Each is a place where the *arithmetic* is
unsigned or exact and the *prose* was written from what the expression looks
like: `>>25` reads as a sign test, `2 * half` reads as a pair count, a strip
seeded 254 wide reads as a strip used 254 wide. None of those survives being
run — which is the argument for running them.

Two tools check this document, one in each direction.

`tools/unstale.py` checks that every name it writes in backticks is a name the
program still has, and that a line count quoted beside a function's name matches
that function.

`tools/covered.py` asks the reverse, and it exists because of what it found. The
first draft of this document was gated, committed and pushed **missing the entire
counter layer** — `bitctr.inc` and `sym_code.inc`, which is to say every
adaptive probability in the program. `unstale.py` reported zero throughout,
correctly: it can only check names that are present, and silence is invisible to
it. So `covered.py` walks the other way — for every function in the spliced unit,
does any document name it? — and reports a ratchet rather than a zero, because
the honest answer is that seven functions are still undescribed and they are
frees and byte-movers.

That pass took the undescribed count 64 → 50 → 7, and the two things it turned up
that were not just missing identifiers are worth naming: the two rounding
constants of §7.1, and the fact that `alt_p1_alloc` and `alt_p2_alloc` — which
look like allocators and are named like allocators — seed 629,856 counter nodes,
163,840 bank counters and 15,552 three-way records with the priors §9.2 and §10.4
now give.

Both of those caught something here while it was being written: an ambiguous
sentence in §11 that read as if `decode_stage_two` were thirteen lines long, and
a name in backticks that belongs to the tool rather than to the program. The
list itself was the third: it named a file that has never existed in this
repository, and because a document it cannot open was silently skipped, the
entry looked like coverage and provided none. A document that cannot be opened
is now reported.
