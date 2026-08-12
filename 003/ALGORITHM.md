# BMF 2.01 — how the compressor works

A reading of `subs1.hpp` and `bmf.cpp` in this tree: the container format, the
two coded streams, the range coder and its state, the modelling front end, and
the two entropy back ends.

Everything below was derived by reading the decompiled bodies. Function names
started as the donor's addresses inside `BMF.exe` and have been replaced, as
each role was established, by names that say what the body does;
`tools/addrmap.txt` maps every one of them back to the address it came from.
Global names are IDA's, and each one is a reference into `blob.inc` — BMF.exe's
data segment — at the address given in the tables.

**Confidence.** Sections 1–6 and the range coder in §5 are read off the code
directly; where a detail is inferred rather than read, it says so. §7 is where
that stops being true: the range coder the `-S` path drives is settled, but the
context model above it (`model_plane` and below) is only mapped in outline, and
the non-`-S` back end barely at all. §9 lists exactly what is not settled, so
the rest can be relied on.

---

## 1. Shape of the program

```
main
└── __main                                 argv: "c in.bmp out" / "d in out.bmp"
    ├── __bmf_compress
    │   ├── read_bmp                     read a BMP into the in-memory image
    │   ├── bmf_open_archive                     open the output archive
    │   └── compress_image                     compress one image      ← §2
    └── __bmf_decompress
        ├── bmf_open_archive                     open the input archive
        ├── expand_image                     expand one image        ← §2
        └── write_bmp                     write a BMP
```

`compress_image` (compress) and `expand_image` (expand) are the two halves that
matter. Everything else is container plumbing or file format I/O.

## 2. Container format

An archive is a sequence of members. `compress_image` writes one member per image
and `expand_image` reads one back; `bmf c` produces exactly one.

A member is:

| bytes | what |
| --- | --- |
| 4 | signature `81 8A 32 30` (`"\x81\x8A20"`) |
| 16 | image descriptor (§3) |
| — | a stored sub-member, when descriptor byte +11 bit `0x80` is set; `bmf c` never writes one |
| n | coded data — the buffer described in §4 |
| p | palette, if the descriptor says there is one |

The signature is written by

```c
fwrite("\x81\x8A""20\x81\x90""20a+b", 4u, 1u, out);
```

Only the first four bytes are written. What looks like one long literal is
three string constants that the original linker tail-merged into one another —
the signature, the second accepted signature `81 90 32 30`, and `"a+b"`, the
mode `bmf_open_archive` opens the archive with. The reader that used to sniff input
formats accepted either signature.

When the data does not compress (`coded size >= raw size`) the member is
rewritten with the descriptor's "compressed" flag clear and the raw pixels in
place of the coded data.

> **What `read_bmp` checks.** The `BM` signature, a 40-byte DIB header and a
> plane count of 1 — and, since the fixes in `REFACTORING9.md` §46, the fields
> the rest of it indexes with: the height's sign (a top-down BMP is legal and
> this reader fills rows from the bottom up, so it is refused rather than sent
> to `alloc_image` as a negative size), the width and height against the
> sixteen bits `BmfImage` stores them in, the row width against the sixteen
> `alloc_image` computes it in, `biClrUsed` against the palette space reserved
> for it, a run opcode against the depth it implies, and the depth against what
> can be written back — 2, 15 and 16 bits per pixel read and compress but have
> no writer, and `bmf_decompress` exits 5 saying so.
>
> **And what `expand_image` checks**, since §47 and §48: the two lengths in a
> member header that become allocations, against what is left of the file;
> `data_len` on the uncompressed path against the pixel buffer `alloc_image`
> sized from the *other* three header fields, which nothing had tied it to; a
> plane count of 1 to 4, which is both ends of the depth field — 63 asks for
> eight planes into a five-record table, and 0 divides by zero in
> `expand_alphabet`; a nonzero width and height, since `alloc_image` multiplies
> them and `bmf_new` turns a request for nothing into one byte and succeeds; and
> the deinterleave flag against the shape it assumes, one byte per plane per
> pixel, which a packed image is not.

## 3. The image descriptor

16 bytes, and also the header of the in-memory image object that `alloc_image`
allocates. Pixels follow it at offset 16; the palette, if any, follows the
pixels.

| offset | type | meaning |
| --- | --- | --- |
| +0 | `uint16` | width |
| +2 | `uint16` | height |
| +4 | `uint32` | bytes per scanline (stride); the bodies read the low half as `p_i[2]` |
| +8..9 | | *(unused by the paths here)* |
| +10 | `uint8` | bits 0–5: bits per pixel; bit 6 (`0x40`): sub-byte packing; bit 7 (`0x80`): palette present |
| +11 | `uint8` | flags, below |
| +12 | `uint32` | pixel data size in bytes (`stride * height`) |

Byte +11:

| bit | set by | meaning |
| --- | --- | --- |
| `0x02` | `compress_image` | planes were transposed (rows ↔ columns) before coding |
| `0x04` | `-S` | the slow, context-modelling back end was used |
| `0x08` | `compress_image` | per-plane filters are present |
| `0x10` | `search_filter` | the filter search ran |
| `0x20` | `compress_image` | the member is compressed at all |
| `0x80` | | an appended, stored member follows |

The palette is `3 << bpp` bytes — three bytes per entry, `2^bpp` entries — and
exists only for `bpp <= 8`.

## 4. One buffer, two streams

`compress_image` allocates a single output buffer

```c
coded_size = datasize + 0x20000;
coded_buf = malloc(coded_size);
```

and everything the coder emits goes into it. Two different coders write into
that one buffer, alternating, and they share the write cursor:

* a **bit packer**, for headers and filter parameters — §4.1
* a **range coder**, for pixel data — §5

| global | address | type | role |
| --- | --- | --- | --- |
| `coded_buf` | `0x0044337C` | `void *` | base of the coded buffer |
| `out_cursor` | `0x00443378` | `uint8 *` | write/read cursor, **shared by both coders** |
| `packer_word` | `0x00443374` | `uint32 *` | the 32-bit word the bit packer is filling |
| `packer_acc` | `0x0044336C` | `uint32` | bit packer accumulator |
| `packer_free_bits` | `0x00443368` | `int32` | bits still free in that accumulator |
| `coded_size` | `0x00443370` | `uint32` | buffer size |
| `hist_scratch` | `0x00443380` | `uint32 *` | scratch histogram / workspace, parked at `coded_buf + size - 4096` |

### 4.1 The bit packer

Bits are packed LSB-first into a 32-bit accumulator and flushed as
little-endian words. `packer_free_bits` is the number of bits still **free** in the
accumulator, so the next value goes in at bit position `32 - packer_free_bits`. Writing a
`K`-bit value `v`:

```c
if (packer_free_bits >= K) {                       /* fits in the word being filled */
    packer_acc |= v << (32 - packer_free_bits);
    packer_free_bits   -= K;
} else {                               /* close this word, open the next  */
    *(uint32_t *)packer_word = packer_acc | (v << (32 - packer_free_bits));   /* low packer_free_bits bits of v */
    packer_word  = out_cursor;          /* the closed word's successor      */
    out_cursor += 4;
    packer_acc = v >> packer_free_bits;                /* the rest of v, at bit 0          */
    packer_free_bits   = packer_free_bits + (32 - K);          /* — note: packer_acc uses the OLD packer_free_bits */
}
```

Two details are easy to get wrong reading this:

* **The shift in the flush is not `v << (32 - packer_free_bits)` as written above.** The
  donor computes it as `2 * (v << ((31 - packer_free_bits) & 31))`, and that is not just a
  way of keeping the shift count below 32: at `packer_free_bits == 0` the donor's form is
  `2 * (v << 31)`, which is **0**, whereas `v << 32` would be undefined and on
  x86 evaluates to `v`. `packer_free_bits == 0` is a state the packer really reaches, so
  the `2 *` form is load-bearing, not cosmetic.
* **`packer_acc = v >> packer_free_bits` uses the value of `packer_free_bits` from before the update.**
  Those are the `K - packer_free_bits` bits of `v` that did not fit in the word just
  closed, and they become the bottom of the new accumulator.

`packer_free_bits` starts at 0, which means "no word is open yet" rather than "the word is
full". The first write therefore takes the flush branch and stores a throwaway
word at `coded_buf`; because `packer_word` is only then advanced to where
`out_cursor` already was — the same address — the next flush overwrites it.
From there on `packer_word` trails `out_cursor` by one word: `packer_word` is the
word being filled, `out_cursor` is where the next one will go.

The packer is not called through a function — the sequence above is inlined at
every use site in `compress_image`, which is why the compressor's body is full of
near-identical blocks.

## 5. The range coder

This is the heart of the entropy stage: a **carry-counting range coder over a
31-bit `low`**, byte-at-a-time renormalisation, in the Subbotin lineage (the
same family as the one in Shkarin's PPMd).

### 5.1 State

The coder is a `RangeCoder` struct with one instance, `rc`. Its state is six
private members, which in the donor were five words of `blob.inc` — two of the
six shared a word. The `was` column is here because those names are what a
disassembly of BMF.exe shows, and they describe nothing: IDA named each word
after whatever constant it saw the word compared against, so `range` is
`__n0x800000` after the renormalisation floor it is tested against, and `low`
is `__n0x7F800000` after the pending-byte threshold.

| member | was | encoder | decoder |
| --- | --- | --- | --- |
| `range` | `__n0x800000` @ `0x004456E0` | **`range`** | **`range`** |
| `low` | `__n0x7F800000` @ `0x004456E4` | **`low`** (31 bits + carry in bit 31) | **`code`** (31 bits) |
| `cache` | `__byte_4456F0` @ `0x004456F0` | the byte awaiting a possible carry | the previous input byte |
| `pending` | `__dword_4456E8` @ `0x004456E8` | deferred `0xFF` bytes | — |
| `rdiv` | *(the same word)* | — | `r`, the last `range / total` |
| `bytes` | `__dword_4456EC` @ `0x004456EC` | emitted so far; the flush writes it out | — |

`low` is one member with two jobs, not two variables: it holds `low` while
encoding and `code` while decoding, and nothing switches between them because
a run of the program only ever does one of the two. `pending` and `rdiv` were
likewise one word in the donor, for the same reason; they are two members here
since nothing is gained by overlapping them.

The output cursor is not the coder's: `out_cursor` (`0x00443378`) is shared
with the bit packer (§4), which is why the class lives in subs1.hpp.

A coding step's arguments — `cumFreq`, `cumFreq + freq`, `totFreq` — are
parameters. In the donor they were three more globals (`__n0x2000_1`,
`__n0x2000_0`, `__n0x2000` at `0x004456F4`…`FC`), because its entries took no
parameters at all, plus `__n0x88` at `0x00445704` for `f0 + f1` of a binary
step. See §5.4.

Invariant: `0x800000 < range <= 0x80000000`, i.e. `range` is always more than
2²³ and at most 2³¹, so `low` and `code` are 31-bit quantities.

### 5.2 Encoder

**Init — `rc.enc_init()`**, called by `rc_begin_encode` once it has built the model
tables

```c
range = 0x80000000;   /* 2^31                  */
low   = 0;
pending = 0;          /* no deferred 0xFF bytes */
bytes   = 0;
cache = 0x97;         /* the marker             */
```

The cache is seeded with `0x97` (151). Because the cache is what gets written
on the first renormalisation, **the first byte of every range-coded section is
`0x97`** — the coder's own machinery emits the section marker, and the decoder
checks for it.

**Renormalise + encode — `rc.enc_normalise()` and `rc.encode()`** (the donor
had this loop copied into a dozen bodies; §5.4)

```c
while (range <= 0x800000) {
    ++bytes_emitted;
    if (low < 0x7F800000) {              /* top byte settled, no carry     */
        emit(0);
    } else if (low & 0x80000000) {       /* carry out of bit 30            */
        emit(1);
    } else {                             /* low in [0x7F800000,0x80000000) */
        ++pending;                       /* top byte is 0xFF — defer it    */
    }
    range <<= 8;
    low = (low << 8) & 0x7FFFFFFF;
}

/* the coding step proper */
r     = range / totFreq;
low  += r * cumFreq;
range = (cumFreq + freq < totFreq) ? r * freq : range - r * cumFreq;
```

where `emit(c)` is

```c
*out_cursor++ = cache + c;                     /* the carry lands here       */
for (; pending; --pending)
    *out_cursor++ = c - 1;                     /* 0xFF if c==0, 0x00 if c==1 */
cache = low >> 23;                             /* next top 8 bits of low     */
```

`c - 1` as a byte is `0xFF` for `c == 0` and `0x00` for `c == 1`, which is
exactly what a carry rippling through a run of `0xFF`s produces.

**Flush — `rc.flush()`**, which `rc_end_encode` calls before freeing the model
tables, runs the renormalisation loop once more and then writes the tail, in
this order:

```
[ cache + carry ]  [ pending run ]  [ rounding byte ]
[ 3-byte big-endian length ]
[ zero padding until (offset - coded_buf) % 4 == 3 ]  [ 0x97 ]
```

The first two are the ordinary `emit` of §5.2 — the held cache byte and any
`0xFF`s behind it — so the tail is not special-cased, it just runs the same
machinery one last time. The rounding byte is

```c
rounded = ((low & 0x7FFFFF) >= ((length & 0xFFFFFF) >> 1)) + (low >> 23);
```

i.e. the next eight bits of `low`, rounded up when what remains below them is
large enough to need it; the carry out of that rounding is what `cache + carry`
above absorbed.

`length` is `bytes_emitted + 5`, written big-endian in three bytes — so a
section carries its own size, which is how a decoder can skip one. After the
marker, `packer_free_bits`, `packer_acc` and `packer_word` are reset, and the padding rule
guarantees the bit packer resumes on a 4-byte boundary.

### 5.3 Decoder

**Init — `rc.dec_init()`**, called by `rc_begin_decode`

```c
if (*out_cursor++ != 0x97) exit_402E40(4);   /* "Read error!" */
cache = *out_cursor++;                        /* the previous byte */
range = 128;                                  /* 2^7               */
low   = cache >> 1;                           /* code, 7 bits      */
```

Setting `range = 2^7` forces the renormalisation loop to run three times
immediately (2⁷→2¹⁵→2²³→2³¹), which reads three more bytes and leaves `code`
holding exactly 31 bits and `range = 2³¹` — the encoder's initial state.

**Renormalise + decode**

```c
while (range <= 0x800000) {
    code  = (code << 8) | ((prev & 1) << 7) | (cur >> 1);   /* cur = *out_cursor++ */
    prev  = cur;
    range <<= 8;
}
```

The one-bit shuffle looks like the stream is stored at a bit offset. It is
not. `code` was seeded with only seven bits of the first byte (`b >> 1`), so it
permanently sits one bit short of a byte boundary; each step puts back the bit
that `>> 1` discarded — which is why the *previous* byte has to be kept — and
takes seven fresh ones. Eight new bits per step, in order, from a plainly
byte-aligned stream. What `code` holds is the same 31-bit window of it that the
encoder's `low` does.

The two decoding steps are split, matching the classic interface:

```c
/* rc.get_freq(tot) */
r     = range / totFreq;              /* kept in `rdiv` for decode() */
count = code / r;
return (count >= totFreq) ? totFreq - 1 : count;

/* rc.decode(cum, cum + freq, tot), after the caller has turned `count`
   into the slot it landed in                                           */
code  -= cumFreq * r;
range  = (cumFreq + freq < totFreq) ? freq * r : range - cumFreq * r;
```

`rc.decode_bit(f0, f1)` is the binary shortcut — decode one bit whose two
frequencies are `f0` and `f1`, without a `get_freq`/`decode` pair:

```c
rt = f0 * (range / (f0 + f1));
if (code >= rt) { range -= rt; code -= rt; return 1; }
else            { range  = rt;             return 0; }
```

`rc.encode_bit(f0, f1, bit)` is its encoder twin. It returns the width of the
interval it chose — the new `range` — which is what `encode_symbol_tree` passes on as
its own result.

**Finish — `rc.finish()`**, called by `rc_end_decode`, renormalises, then scans
forward for the `0x97`
terminator and resets the bit packer's cursor to just past it.

### 5.4 The RangeCoder class

All of the above is a `RangeCoder` struct with one instance, `rc`, declared in
subs1.hpp just after the globals block — it goes there rather than in bmf.cpp
because it shares its output cursor with the bit packer, and `out_cursor` is a
blob global.

| method | direction | what |
| --- | --- | --- |
| `rc.enc_init()` | encode | `range = 2^31`, `low = 0`, cache = the `0x97` marker |
| `rc.dec_init()` | decode | check the marker, prime `code` with 31 bits |
| `rc.enc_normalise()` | encode | the carry-counting byte loop |
| `rc.dec_normalise()` | decode | eight more bits into the window |
| `rc.encode(cum, high, tot)` | encode | one symbol; returns `freq * r` |
| `rc.encode_bit(f0, f1, bit)` | encode | one bit; returns the width of the interval chosen |
| `rc.get_freq(tot)` | decode | which slot of `tot` the code falls in |
| `rc.decode(cum, high, tot)` | decode | finish the step `get_freq` started |
| `rc.decode_bit(f0, f1)` | decode | one bit; returns it |
| `rc.flush()` | encode | tail, length, padding, terminator |
| `rc.finish()` | decode | skip to the terminator |
| `rc.dec_get(q)` | decode | one input byte, bounded by the end of the coded buffer |

`dec_get` is the only place the decoder touches its input, and the only place
that knows where that input stops. It exists because none of the three read
paths above had an end: `dec_normalise` primed `code` from whatever followed the
buffer, and `finish` scanned for the `0x97` terminator with `while (*q++ !=
kMarker) { }` and nothing on the other side of it. A well-formed stream never
reaches the bound — `flush` pads the tail to a four-byte boundary before the
marker, which is the read-ahead `get_freq` and `decode_bit` need — so it fires
only on a stream that lied about its own length. `REFACTORING9.md` §47.

Every argument is an argument. The donor had no choice about that — its
entries took no parameters and three globals *were* the argument list, so a
model function would assign `__n0x2000_1`, `__n0x2000_0` and `__n0x2000` over
the course of a symbol search and then call. Those sites now carry their own
locals and pass them, which matters because the assignments are not adjacent
to the call: `code_pixel` computes its `high` from a counter that it then
increments *before* coding, so moving the expression to the call site would
have coded a different number.

The state is `private`, so "does anything outside still touch the coder?" is a
question the compiler answers.

Two model functions still hold a coding step that is more than one call:
`SymList::code_symbol` encodes a symbol against a sorted frequency list (§7.3) and
`P2Freq::decode_symbol` decodes one from a 3-way counter node. Both now do their search
and then call `rc`.

## 6. The modelling front end

Before any entropy coding, the image is taken apart. All of this is reversed
exactly by the decoder, so it is part of the format.

### 6.1 Plane split

```c
plane_count = ((bpp & 0x3F) + 7) >> 3;      /* 0x00443394 — number of planes */
```

Interleaved pixels are de-interleaved into planes with a strided copy: plane
`k` is `data[k], data[k+n], data[k+2n], …`. 24-bit RGB becomes three planes,
32-bit RGBA four, 8-bit and below one.

### 6.2 Per-plane descriptor

A 16-byte record per plane at `0x0044339C`, indexed `[16 * k]`:

It is `PlaneDesc plane_desc[5]` here; record 0 is a header the four plane
records sit above, which is why every subscript in the code is `[k + 1]`.

| offset | member | meaning |
| --- | --- | --- |
| +0 | `nrefs` | number of reference planes in the colour transform, 0–3 |
| +1 | `src_plane` | this plane's byte offset within an interleaved pixel |
| +2 | `flags` | bits 0–1 = spatial predictor mode (→ `plane_predictor`), bit 2 = alternate model (→ `plane_alt_model`), bit 3 = colour transform present |
| +3 | `b3` | constant bias |
| +4 | `w4` | coefficient 0 |
| +8 | `w8` | coefficient 1 |
| +12 | `w12` | coefficient 2 |

`nrefs` was called `predictor` in the struct until this table was checked
against it. The two disagreed about the same byte, and the header emitter
settles it: it writes `(flags << 2) | nrefs`, then one coefficient if `nrefs`
is `> 1` and a second if it is `> 2`, so the byte counts the coefficients that
follow. The *predictor* is `flags & 3`, which is what `plane_predictor` is
assigned from four lines further on.

`compress_image` writes these into the bit stream ahead of the coded data: 6 bits of
`(flags << 2) | nrefs`, then — if the transform is present — 8 bits of bias, and
8 bits per coefficient, each stored biased by `+64`.

### 6.3 Colour transform — `colour_transform`

Coefficients are 7-bit fixed point, held biased by `+64` in the bit stream, so
they run over roughly `[-0.5, +1.5]` once divided by 128. The references are
other planes of the *same pixel*, reached by the byte offsets in the descriptors'
`+1` fields: `R₁` and `R₂` are planes 0 and 1, and the three-reference form uses
the three bytes preceding the current plane in the pixel. Per pixel, for a plane
with `n` references:

| `n` | transform |
| --- | --- |
| 0 | `dst = X` (plain de-interleave) |
| 1 | `dst = X - bias - R₁` |
| 2 | `dst = (X - bias) - ((c₀·R₁ + c₁·R₂ + 40) >> 7)` |
| 3 | `dst = (X - bias) - ((c₀·R₁ + c₁·R₂ + c₂·R₃ + 63) >> 7)` |

Two oddities that are in the code rather than in this description: the rounding
constants differ (40 for two references, 63 for three, neither of them the 64
that would round to nearest), and the two shifts are not the same kind — the
two-reference sum is shifted as `int32_t`, the three-reference sum as `uint32_t`.
Since the coefficients can be negative, that is a real difference in behaviour,
not a decompiler artefact.

`colour_transform` also short-circuits the two-reference form: when `c₀ + c₁ == 128`
and one of them is zero, it drops to the `n == 1` path against whichever plane
still has weight, skipping the multiply.

Everything is byte arithmetic, so each subtraction wraps mod 256 and is exactly
invertible. The `n == 1` case is the familiar green-as-predictor RGB
decorrelation; `n == 2` and `n == 3` let the filter search fit a linear
combination instead of assuming one.

### 6.4 Spatial prediction — `predict_med`

In predictor mode 1 — the mode the filter search picks for most planes — each
plane is predicted in place by `predict_med`, walking **backwards** from the last
pixel so that `N`, `W` and `NW` are all still originals when they are read. The
predictor is **MED**, the median edge detector from LOCO-I / JPEG-LS:

```c
if (NW >= max(N, W))      pred = min(N, W);
else if (NW <= min(N, W)) pred = max(N, W);
else                      pred = N + W - NW;
```

which the decompiled body writes as a nest of comparisons on `W < N`. The
residual `X - pred` (mod 256) is then folded from signed to unsigned through a
256-entry table built at the top of the function: `0 → 0`, `+d → 2d`,
`-d → 2d - 1`. The folded byte replaces the pixel, and a histogram is
accumulated in `hist_scratch` for the filter search.

The table is 256 entries and its last one matters: `fold[128] = 255`, a residual
of −128, which happens whenever the prediction is 128 above the pixel. The
inverse table in `unpredict_med` was declared 255 long and filled to 254, so
that code decoded to whatever the stack held past the array. `REFACTORING9.md`
§49 — it is the missing entry that is the defect, not the index that found it.

`__byte_44339E & 3` (`plane_predictor`, `0x00443360`) chooses the mode, and it interacts
with `E`. The dispatch, read off `transform_planes`, is:

| `plane_predictor` | `E == 0` | `E != 0` |
| --- | --- | --- |
| 0 | no spatial prediction | *(near-lossless)* |
| 1 | `predict_med` — MED, above | *(near-lossless)* |
| 2 | skipped entirely | skipped entirely |
| 3 | no spatial prediction | *(near-lossless)* |

So MED is mode 1, mode 2 turns the stage off, and what distinguishes 0 from 3 I
did not establish (§9).

> The `E != 0` column is BMF's, not this source's. `-E` is a constant 0 here and
> the bodies behind that column are deleted — see REFACTORING.md §2.1, and §6.5
> below.

### 6.5 Near-lossless

`-E<N>` (`__n7_1`, `0x004410A0`) is copied into what was the global
`near_lossless_max` (`0x00443398`) and becomes the maximal allowed error `E`.
REFACTORING4 §4.1 folded that global into the plane descriptors, so it is
`plane_desc[n].w12` in the source now. When it is non-zero the residual is
quantised with step `2E + 1`:

```c
n128_1 = 2 * near_lossless_max[0] + 1;
```

`sub_4111B0` (encode) and `expand_predictor_mode0` (decode) built the
quantisation and
reconstruction tables with SSE, then ran the same MED loop against the
*reconstructed* neighbours so encoder and decoder stayed in step. This is the
one mode where the round trip is not bit-exact, by design.

> **Not in this source.** `-E` is a constant 0 (REFACTORING.md §2.1), so those
> bodies are deleted and a stream whose 4-bit `E` field is non-zero is refused
> at the point the field is read rather than expanded wrongly. The description
> above is of BMF, kept because it is what the format's `E` field means.

### 6.6 Filter search — `search_filter`, cost by `estimate_cost`

`-F` (`__dword_44108C`) turns the search on; `-Q<N>` (`__n7_0`, `0x0044109C`)
sets how hard it looks. It appears as a table-size multiplier
(`(__n7_0 + 5) / 3`, `(desc_slow_mode + __n7_0 + 1) << 6`) and as several
`__n7_0 > k` gates that enable extra candidate transforms, so raising it both
widens the search and enlarges the structures it searches over.

Candidates are scored with `estimate_cost`, which is a plain **order-0 entropy in
bits** of a histogram of `n` counts summing to `N`:

```
H = (N·log N  −  Σᵢ nᵢ·log nᵢ) · log₂(e)
```

computed in `double`, two histogram entries at a time in SSE.  BMF had a
two-lane `log` helper, `log_two_lane`, which took a mask: where a lane's count
was zero it substituted `0.5` (`__bmf_half_half`, `0x0043B480`) so that `log`
was never handed a zero, and the caller then masks those lanes back out of the running sum
with `_mm_andnot_ps`. The substituted value is never used — it exists to keep a
`-inf` out of the vector. This
and the cost functions `choose_plane_coding` (`choose_plane_coding`) and `cost_candidate`
(`cost_candidate`) are the floating-point part of the compressor, and
the reason the SSE denormal mode (`bmf_set_denormal_mode`, §5 of the build) is
set at startup: flush-to-zero and denormals-are-zero keep these sums fast and
identical run to run. The search picks the transform and predictor whose folded
residuals have the lowest estimated entropy, and records them in the per-plane
descriptor.

`-T<N>` (`__n2_4`) stores or reuses the chosen filter set as a template, so a
run of similar images can skip the search.

## 7. Entropy coding

Two back ends. `desc_slow_mode` (`0x00443384`), copied from `-S`
(`__dword_441090`), chooses between them per image, and the choice is recorded
in descriptor byte +11 bit `0x04` so the decoder follows.

### 7.1 Fast path (default in BMF) — **not in this source**

Without `-S`, BMF sent the folded residuals through `sub_408510` (encode) and
`sub_40CF80` (decode). These wrote through the **bit packer**, not the range
coder — they never called `rc`. They built histograms and called `estimate_cost`
to measure them, which says a code was being *chosen* from the statistics rather
than adapted; what kind of code, I did not establish.

`-S` is a constant 1 here, so both bodies and the eight more they reached are
deleted (REFACTORING.md §2.1), and a stream with descriptor bit 2 clear is
refused rather than expanded. Two things are worth recording about them, since
nobody will read them here again:

* they crashed. `bmf` without `-S` faulted in the imported sources, in
  `sub_40F450`, writing through an index read from one slot *before* a rank
  table — which is the adjacent pointer field, so the store went to a heap
  address. Whether the original binary had the same defect is not established;
  the decompilation certainly did.
* nothing was lost by deleting them, because nothing could reach them: the
  command line has pinned `-S` since the day it was written.

### 7.2 Slow path (`-S`) — `model_plane` and below

This is the one `bmf c` uses when `plane_alt_model` is clear, which on the ten
images in `testfiles/` is **7 of the 15 planes decoded** — the other 8 go to the
alternate families below, and this section does not describe them. Measured, not
assumed: `plane_alt_model` is a bit in each plane's descriptor, and the counts
are in `REFACTORING.md` §2.3.

That is worth saying plainly, because the shape of this document implies
otherwise. The part that is read in detail is the minority path.

It is a binary context model driving the range coder of §5.

`model_plane` runs the plane through this pipeline, in this order:

1. `malloc(0x7BA230)` — an ~8 MB workspace — laid out by `layout_workspace`.
2. `rc_begin_encode` — start the range coder (§5.2).
3. `reduce_alphabet` — **alphabet reduction**, and the first thing written, with
   `expand_alphabet` reading it back: the same helpers in the same order, the
   flat slot encoded with `rc.encode` and decoded with `rc_decode_flat`, and
   both recursive. It scans
   the plane for the symbols actually used, encodes `distinct - 1` with
   `rc.encode` as a flat one-wide slot out of the full alphabet size, encodes each used
   symbol with `SymList::code_symbol`, and then re-indexes the plane onto the dense
   alphabet that leaves. A plane using 40 of 256 values is coded over 40
   symbols from here on.
4. Model initialisation — the tables described below.
5. `code_pixel` — the per-pixel coder, once per pixel.
6. `rc_end_encode` — flush the range coder (§5.2).

When `plane_alt_model` (descriptor `+2` bit 2) is set, `model_plane` instead
dispatches to one of four separate model families, picked by predictor mode
and by whether the depth is exactly 8. `unmodel_plane` has the same dispatch
on the way back, which is what pairs them:

| predictor | depth | encode | decode |
| --- | --- | --- | --- |
| 1 | 8 | `alt_model_p1_d8_encode` | `alt_model_p1_d8_decode` |
| 1 | otherwise | `alt_model_p1_encode` | `alt_model_p1_decode` |
| 2 | 8 | `alt_model_p2_d8_encode` | `alt_model_p2_d8_decode` |
| 2 | otherwise | `alt_model_p2_encode` | `alt_model_p2_decode` |

The names say the entry condition because that is all that is established;
what the families *do* is §9.

What the model initialisation builds:

* **15 context groups.** The loop runs `v58` from 0 to 14, indexed by a 15-entry
  table of flag bytes at `ctx_group_flags` (`0x00439860`). Each flag bit selects
  whether one of six inputs is folded into that group's context.
* **A 64K counter table per group.** Each group gets `0x10000` entries of two
  `uint16` counters, both initialised to `0x2000`:

  ```c
  LOWORD (v24[n + 531818]) = 0x2000;
  HIWORD (v24[n + 531818]) = 0x2000;
  ```

  Two 8192s is p = ½ — a fresh binary counter. The index is a 16-bit value
  `code_pixel` derives from the neighbourhood, of the form
  `table[N] - <one of N, W, NE, NW>`; exactly which table and which neighbour is
  in §9's unread list.
* **A 5×5 sub-state grid per group**, with mixing weights `1 << (5 - depth)` and
  a running total. `model_plane` computes, for each cell, how many of the six
  inputs are active and derives the weight from that count — a static mixing
  table rather than a learned one.

  The record is sixteen bytes at `ModelBlock + 96 + 16 * bucket`: five counts,
  their total, the scaled weight, then a byte for the depth and a byte for
  `1 << (5 - depth)`. 15 groups × 5 × 5 is 375 cells, but the counter only
  advances for cells that get one, and it reaches exactly **189** — measured
  with a `__builtin_trap()` on `>= 188`, which fires, against one on `>= 189`,
  which does not.

  Those 189 records are the front of one array. The frequency records
  `code_pixel` and `decode_pixel` walk are the same sixteen bytes on the same
  grid, starting at record 188 (+3104) and running to +1051663 — so the last
  sub-state record and the first frequency record are one and the same, and the
  initialisation loop's final iteration seeds it.

`code_pixel` is the per-symbol coder on this path. It forms a context by
comparing the current pixel's neighbours (`N`, `W`, `NE`, `NW` — the locals
Hex-Rays named `n15_3`, `n15_7`, `n15_4`, `n15_2`) against each other and
against the two values cached in the counter entry, producing a state index in
steps of 15 and 75:

```c
if      (v == N)  n15_5 = 15;
else if (v == W)  n15_5 = 30;
else if (v == NE) n15_5 = 45;
else if (v == NW) n15_5 = 60;
else              n15_5 = 0;
/* … then the same again against the second cached value, in steps of 75 */
```

so the state is `(match₁, match₂)` over a 5 × 5 grid — the same 5 × 5 the
workspace allocated. That index selects the sub-state, which selects the counter
pair, which is fed to `rc.encode_bit` / `rc.decode_bit` as `(f0, f1)`.

### 7.3 Counters

Two counter shapes appear, both 16-bit, both with the halve-on-overflow rule
that keeps them adaptive:

**Binary pair — `update_binary_pair`.** Walks a bit tree; at each node

```c
if (counter > 0x2000) {            /* rescale before it saturates */
    c0 -= c0 >> 1;
    c1 -= c1 >> 1;
}
counter += (__n8_0 * ((plane_predictor == 2) + 5)) >> 3;   /* adaptive increment */
```

**Three-way node — `P2Freq::decode_symbol`,** the decoder for a node holding three
frequencies (`node[1]`, `node[2]`, `node[3]`) plus its own increment in
`node[0]`. After decoding it does

```c
if (freq > 0x4000) {                        /* rescale */
    node[1] -= node[1] >> 1;
    node[2] -= node[2] >> 1;
    node[3] -= node[3] >> 1;
    if      (node[0] >  256) node[0] >>= 1;   /* and slow the increment down */
    else if (node[0] >   32) node[0] -= 32;
    else if (node[0] >   16) node[0] -=  2;
    /* at or below 16 it stops shrinking */
}
node[chosen] += node[0];
```

The increment starts large and ratchets down each time the node rescales — fast
adaptation early, stability later — with 16 as the floor, so the node never
stops adapting entirely.

**Sorted list — `SymList::code_symbol` and `decode_symbol_list`.** For alphabets
rather than bits: a list of
`(symbol, count)` triples kept in descending count order. Encoding sums counts
until the symbol is found (that sum is `cumFreq`), calls the range coder, then
adds 4 to the symbol's count and bubbles it toward the front by swapping with
its neighbour. Symbols masked out by `exclusion_mask` are skipped, and running off the
end of the list codes an escape — a PPM-style exclusion mechanism.

## 8. Global map

Every address is an offset into `blob.inc`, BMF.exe's data segment. For
functions rather than globals, `tools/addrmap.txt` is the corresponding map:
every named body to the address it was decompiled from, recovered by
`tools/addrmap.py` from the commits that made each rename rather than
reconstructed from the code.

**Range coder.** None: its state is `RangeCoder`'s private members, and the
nine words it used to occupy (`0x004456E0`…`0x004456FC` and `0x00445704`) are
no longer declared. §5.1 maps the members onto the addresses a disassembly of
BMF.exe shows.

**Streams and buffers**

| global | address | meaning |
| --- | --- | --- |
| `coded_buf` | `0x0044337C` | coded buffer base |
| `out_cursor` | `0x00443378` | shared byte cursor |
| `packer_word` | `0x00443374` | bit packer's current word |
| `packer_acc` | `0x0044336C` | bit packer accumulator |
| `packer_free_bits` | `0x00443368` | free bits in it |
| `coded_size` | `0x00443370` | buffer size |
| `hist_scratch` | `0x00443380` | scratch histogram at `coded_buf + size - 4096` |
| `exclusion_mask` | `0x00443440` | symbol exclusion mask |
| `model_tables` | `0x00445708` | model table allocated by `rc_begin_encode` / `rc_begin_decode` |

**Coding parameters.** Not globals any more: `constexpr` constants at the top of
`subs1.hpp`, which is what lets the branches testing them fold and the other
modes' code be deleted (REFACTORING.md §2.1). The addresses are BMF.exe's, for
comparing against a disassembly.

| constant | address | switch | value here | meaning |
| --- | --- | --- | --- | --- |
| `opt_use_filters` | `0x0044108C` | `-F` | 1 | use filters |
| `opt_slow` | `0x00441090` | `-S` | 1 | slow but efficient |
| `opt_filter_template` | `0x00441094` | `-T` | 0 | filter template |
| `opt_pack_output` | `0x00441098` | `-N` | 1 | pack the output |
| `opt_search_quality` | `0x0044109C` | `-Q` | 9 | filter search quality |
| `opt_max_error` | `0x004410A0` | `-E` | 0 | max error, near-lossless |
| *(none)* | `0x004410A4` | | — | the `-T` template store; deleted with it |

**Per-image working state**

| global | address | meaning |
| --- | --- | --- |
| `plane_count` | `0x00443394` | number of colour planes |
| `plane_predictor` | `0x00443360` | current plane's predictor mode |
| `plane_alt_model` | `0x00443364` | current plane's alternate-model flag |
| `desc_slow_mode` | `0x00443384` | slow mode for this image — always 1; a stream saying otherwise is refused |
| `plane_desc[0].w12` | `0x00443398` | near-lossless max error `E` — always 0, likewise; was the global `near_lossless_max`, and it is record 0's fourth word |
| `plane_desc[1…4]` | `0x0044339C` | per-plane descriptors, 16 bytes each (§6.2) |
| `ctx_group_flags` | `0x00439860` | 15 context-group flag bytes |
| `model_geometry` | `0x00445660` | model geometry tables built by `rc_begin_encode` |

## 9. What is not settled

Stated plainly, so the rest can be trusted:

* ~~**The fast (non-`-S`) back end.**~~ Moot for this source: it is deleted
  (§7.1). It remains unestablished about *BMF* — `sub_408510` was 1576 lines and
  all that was established is that it wrote through the bit packer and used
  `estimate_cost`. Whether it was Huffman, Golomb–Rice or something else would
  now have to be read out of BMF.exe rather than out of this tree.
* **The `-S` context construction in detail.** §7.2 gives the shape — 15 groups,
  64K binary counters each, a 5 × 5 sub-state grid, neighbour-match state
  indices — but not the exact derivation of the 16-bit context index, nor how
  the 15 groups' predictions are combined into the one probability handed to the
  coder. Part of it is settled now: the neighbour-match state is stored, not
  recomputed. Each row buffer holds one eight-byte record per pixel — the symbol
  and six flags comparing it against the pixel above, the pixel to the left, and
  four diagonals — and four counters slide eight-record and four-record windows
  over two of those flags. `algorithm_v2.md` §8.2.1 has the table. `code_pixel` is 781 lines and would need a full reading.
* **The alternate model families.** When descriptor `+2` bit 2 is set,
  `model_plane` hands the plane to one of the four alternate families instead
  (the table is in §7.2). I established only which one is picked, not what they
  do; `alt_p2_model` (1402 lines here) sits under two of them.

  **The gap has a boundary, at least.** 22 bodies are reachable *only* from
  this dispatch and from nothing else — `alt_p2_model`, `alt_p2_context`,
  `AltP1Block::update_model`, `alt_p1_alloc`, `alt_p2_alloc` and the rest, plus
  `update_binary_pair` and `P2Freq::decode_symbol`, which the slow path does not
  use. That is the subsystem, and it is a closed one: anything outside it is
  described elsewhere in this document.

  One thing has already come out of knowing that. `alt_init_tables` had a
  predictor-mode-0 branch, and because the dispatch reaches this subsystem only
  under predictor 1 or 2, no path could enter it; 111 lines went (REFACTORING.md
  §2.3). It is the only such guard in the 22.

  **Three things about the p2 family's memory are settled, from its own code
  rather than from reading the model.** They are not the model, but anyone who
  reads it will need them:

  * **The record is 18 bytes**, and thirty-two places copy one. MSVC unrolled
    each into four dwords and a word, which is what made them recognisable:
    `*(uint32_t *)(d - 18) = *(uint32_t *)s` through `*(uint16_t *)(d - 2) =
    *(uint16_t *)(s + 16)`. Round three folded each into a `bmf_copy(d, s, 18)`
    and round six gave the record a type, so every one of them is a record
    assignment now -- `((P2Ctx *)p)[i] = ((P2Ctx *)p)[j]` -- and the helper has
    no callers left.
  * **Rows are 144 bytes apart.** `alt_model_p2_decode` rotates five row
    pointers and advances three of them by exactly 144: `f278744 = v84 + 144`,
    `f278748 = v83 + 144`, `f278752 = v81 + 144`. Eight rows of 18 bytes.
  * **The copies never overlap**, which is why they can be `memcpy`. This was
    checked at run time while they were `memcpy`s -- `BMF_COPY_CHECK=1
    ./build.sh` aborted if any pair of regions touched, and the gate passed
    against it with all thirty-two sites executing. Round six made every one of
    them a `P2Ctx` assignment, so the types say it instead: two distinct
    records are `sizeof(P2Ctx)` apart at the least.

  **A fourth thing is settled now, and it is about the model rather than its
  memory: how a context becomes a table index.** Both families keep a fixed
  number of weight groups — nine for p1, five for p2 — of a selector and three
  weights. The context routine writes each selector from a difference of
  causal neighbours and reads it back as its sign, 0, 1 or 2; the index is the
  chosen weights added together. The weights `alt_p1_alloc` writes are 32·3^g
  and 64·3^g for g = 0..8, so the sum is a **base-3 number with nine digits**,
  scaled by the 32 counters a context owns — and 3^9 · 32 is 629856, exactly
  the number of `CounterNode`s the allocator asks for. The p2 side is the same
  with five digits and a unit of 64: 3^5 · 64 = 15552. `algorithm_v2.md` §9.1
  has the table and the two asymmetries.

  This does not say what the *neighbours* mean, and it is not a reading of the
  model. It says the context space is 3^9 (or 3^5) sign patterns, which is a
  bound on what the rest can be.

  **And what an 18-byte record holds is settled too, which it was not when the
  paragraph above was written.** Both pixel bodies end a record with the same
  block: `lane[0]` is the pixel scaled by 16, `lane[2]` is its horizontal
  gradient, `lane[3..7]` are five copies of that gradient's magnitude which
  then diverge, and the last two bytes are a three-way sign and a magnitude the
  neighbourhood sums. `algorithm_v2.md` §9.2 has the block; §9.3 has the
  counters, where the fourth word of a frequency record turns out to be the
  size of the update rather than a count.

  **The sign byte closes a loop**: `alt_p2_context` assigns it straight to the
  fifth weight group's selector, so the sign of one record's gradient is a
  base-3 digit of the next context. The model writes a digit into the plane and
  reads it back as a digit.

  And the five digits themselves are readable: group 0 buckets a predicted
  level against two fixed thresholds, groups 1–3 each classify a different
  difference against one adaptive band, and group 4 is the stored sign. The band is ±(16q + 7..8) and the counter
  dead zone is ±(4q + 1) for the same `q` — one tolerance at two scales.
  `algorithm_v2.md` §9.1 has the table.

  The five magnitudes start equal and diverge because **each is the input to a
  different fixed-tap linear predictor** over the same causal neighbourhood —
  three of them with distinct integer taps and their own adaptive bias out of
  `ctx_bias`, one an unweighted sum, one read singly. None feeds a context
  digit directly: the digits take `lane[0]` twice, `sign` twice, and a
  predicted level. `algorithm_v2.md` §9.1 has both tables.

  What is left is quantitative rather than structural — what the taps are
  tuned for, and what the four `ctx_bias` accumulators converge to. The five fields are four `uint32_t` and a `uint16_t`, and
  `alt_p2_context` computes the values that go into them — that is where a
  reading would start.

  **This is the largest gap in the document, and larger than it used to look.**
  A coverage run over `testfiles/` puts 8 of 15 decoded planes through these
  families and 7 through the path §7.2 describes. Three of the four run; only
  `alt_model_p1_decode` — predictor 1 at a depth other than 8 — is never
  reached, so nothing in the corpus is finally coded that way.

  It also explains a result that looks impossible at first: `unpredict_med` is
  never executed, on a corpus that round-trips, while `predict_med` is. Every
  plane whose predictor is MED also has `plane_alt_model` set, and the
  alternate family inverts the prediction itself. `expand_image`'s
  `if (plane_predictor == 1)` is reached 7 times out of 11 and the call inside
  it never.

  An earlier draft of §6.6 listed `alt_p2_context`, `alt_p2_filter` and `alt_p2_model`
  as cost functions alongside the two above. They are not: none of them calls
  `estimate_cost`, and `alt_p2_model` calls `update_binary_pair`, which makes it
  a model rather than a measurement. They are floating-point and they are in
  the alternate model families, which is as much as is established.
  (`sub_40A8A0` was in that list too and no longer exists — it went with the
  fast path, REFACTORING.md §2.1.)
* **What predictor modes 0 and 3 mean.** The dispatch table in §6.4 is read off
  the code, but 0 and 3 land in the same places and I did not work out what
  distinguishes them, nor what mode 2 does instead of predicting. This is why
  `expand_predictor_mode0` is still in the source even though the
  linker discards it: see REFACTORING.md §2.2.
* **The `search_filter` search order.** Which candidates are tried is visible as
  loop bounds but I did not enumerate the candidate set. How `-Q` pruned them is
  no longer a question here: `-Q` is a constant 9, the branch that capped the
  search for lower values is folded away, and the search always sees the whole
  image.
* **The first-byte carry.** The decoder demands exactly `0x97`, so a carry into
  the coder's initial cache would break it. Encoding is correct on every test
  image, so the design evidently precludes it, but I did not prove why.

## 10. Where to look

| topic | function | address |
| --- | --- | --- |
| compress one image | `compress_image` | `0x00402FE0` |
| expand one image | `expand_image` | `0x00403820` |
| range coder init / flush (encode) | `rc_begin_encode` / `rc_end_encode` | `0x00414F60` / `0x00414CE0` |
| range coder init / finish (decode) | `rc_begin_decode` / `rc_end_decode` | `0x004149C0` / `0x00414920` |
| the coder itself | `struct RangeCoder`, instance `rc` | — |
| colour transform | `colour_transform` | `0x00407EF0` |
| MED prediction + folding | `predict_med` | `0x004108C0` |
| ~~near-lossless encode / decode~~ | deleted; see §6.5 | `0x004111B0` / `0x00410650` |
| filter search | `search_filter` | `0x004043E0` |
| entropy estimate | `estimate_cost` | `0x00411700` |
| `-S` model driver | `model_plane` | `0x00415380` |
| alphabet reduction | `reduce_alphabet` | `0x00417200` |
| `-S` per-pixel coder | `code_pixel` | `0x004159E0` |
| adaptive symbol list | `SymList::code_symbol` | `0x00412B10` |
| BMP read / write | `read_bmp` / `write_bmp` | `0x0042AB20` / `0x0042B0C0` |
