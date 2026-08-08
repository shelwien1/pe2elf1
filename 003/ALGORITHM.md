# BMF 2.01 — how the compressor works

A reading of `subs1.hpp` and `bmf.cpp` in this tree: the container format, the
two coded streams, the range coder and its state, the modelling front end, and
the two entropy back ends.

Everything below was derived by reading the decompiled bodies. Function names
are the donor's addresses inside `BMF.exe` (`sub_402FE0` is the body at
`0x00402FE0`); global names are IDA's, and each one is a reference into
`blob.inc` — BMF.exe's data segment — at the address given in the tables.

**Confidence.** Sections 1–6 and the range coder in §5 are read off the code
directly; where a detail is inferred rather than read, it says so. §7 is where
that stops being true: the range coder the `-S` path drives is settled, but the
context model above it (`sub_415380` and below) is only mapped in outline, and
the non-`-S` back end barely at all. §9 lists exactly what is not settled, so
the rest can be relied on.

---

## 1. Shape of the program

```
main
└── __main                                 argv: "c in.bmp out" / "d in out.bmp"
    ├── __bmf_compress
    │   ├── sub_42AB20                     read a BMP into the in-memory image
    │   ├── sub_402EF0                     open the output archive
    │   └── sub_402FE0                     compress one image      ← §2
    └── __bmf_decompress
        ├── sub_402EF0                     open the input archive
        ├── sub_403820                     expand one image        ← §2
        └── sub_42B0C0                     write a BMP
```

`sub_402FE0` (compress) and `sub_403820` (expand) are the two halves that
matter. Everything else is container plumbing or file format I/O.

## 2. Container format

An archive is a sequence of members. `sub_402FE0` writes one member per image
and `sub_403820` reads one back; `bmf c` produces exactly one.

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
mode `sub_402EF0` opens the archive with. The reader that used to sniff input
formats accepted either signature.

When the data does not compress (`coded size >= raw size`) the member is
rewritten with the descriptor's "compressed" flag clear and the raw pixels in
place of the coded data.

## 3. The image descriptor

16 bytes, and also the header of the in-memory image object that `sub_42B830`
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
| `0x02` | `sub_402FE0` | planes were transposed (rows ↔ columns) before coding |
| `0x04` | `-S` | the slow, context-modelling back end was used |
| `0x08` | `sub_402FE0` | per-plane filters are present |
| `0x10` | `sub_4043E0` | the filter search ran |
| `0x20` | `sub_402FE0` | the member is compressed at all |
| `0x80` | | an appended, stored member follows |

The palette is `3 << bpp` bytes — three bytes per entry, `2^bpp` entries — and
exists only for `bpp <= 8`.

## 4. One buffer, two streams

`sub_402FE0` allocates a single output buffer

```c
__ElementCount_0 = datasize + 0x20000;
__Buffer = malloc(__ElementCount_0);
```

and everything the coder emits goes into it. Two different coders write into
that one buffer, alternating, and they share the write cursor:

* a **bit packer**, for headers and filter parameters — §4.1
* a **range coder**, for pixel data — §5

| global | address | type | role |
| --- | --- | --- | --- |
| `__Buffer` | `0x0044337C` | `void *` | base of the coded buffer |
| `__Buffer_0` | `0x00443378` | `uint8 *` | write/read cursor, **shared by both coders** |
| `__Buffer_1` | `0x00443374` | `uint32 *` | the 32-bit word the bit packer is filling |
| `__n256` | `0x0044336C` | `uint32` | bit packer accumulator |
| `__n8` | `0x00443368` | `int32` | bits still free in that accumulator |
| `__ElementCount_0` | `0x00443370` | `uint32` | buffer size |
| `__buf` | `0x00443380` | `uint32 *` | scratch histogram / workspace, parked at `__Buffer + size - 4096` |

### 4.1 The bit packer

Bits are packed LSB-first into a 32-bit accumulator and flushed as
little-endian words. `__n8` is the number of bits still **free** in the
accumulator, so the next value goes in at bit position `32 - __n8`. Writing a
`K`-bit value `v`:

```c
if (__n8 >= K) {                       /* fits in the word being filled */
    __n256 |= v << (32 - __n8);
    __n8   -= K;
} else {                               /* close this word, open the next  */
    *(uint32_t *)__Buffer_1 = __n256 | (v << (32 - __n8));   /* low __n8 bits of v */
    __Buffer_1  = __Buffer_0;          /* the closed word's successor      */
    __Buffer_0 += 4;
    __n256 = v >> __n8;                /* the rest of v, at bit 0          */
    __n8   = __n8 + (32 - K);          /* — note: __n256 uses the OLD __n8 */
}
```

Two details are easy to get wrong reading this:

* **The shift in the flush is not `v << (32 - __n8)` as written above.** The
  donor computes it as `2 * (v << ((31 - __n8) & 31))`, and that is not just a
  way of keeping the shift count below 32: at `__n8 == 0` the donor's form is
  `2 * (v << 31)`, which is **0**, whereas `v << 32` would be undefined and on
  x86 evaluates to `v`. `__n8 == 0` is a state the packer really reaches, so
  the `2 *` form is load-bearing, not cosmetic.
* **`__n256 = v >> __n8` uses the value of `__n8` from before the update.**
  Those are the `K - __n8` bits of `v` that did not fit in the word just
  closed, and they become the bottom of the new accumulator.

`__n8` starts at 0, which means "no word is open yet" rather than "the word is
full". The first write therefore takes the flush branch and stores a throwaway
word at `__Buffer`; because `__Buffer_1` is only then advanced to where
`__Buffer_0` already was — the same address — the next flush overwrites it.
From there on `__Buffer_1` trails `__Buffer_0` by one word: `__Buffer_1` is the
word being filled, `__Buffer_0` is where the next one will go.

The packer is not called through a function — the sequence above is inlined at
every use site in `sub_402FE0`, which is why the compressor's body is full of
near-identical blocks.

## 5. The range coder

This is the heart of the entropy stage: a **carry-counting range coder over a
31-bit `low`**, byte-at-a-time renormalisation, in the Subbotin lineage (the
same family as the one in Shkarin's PPMd).

### 5.1 State

| global | address | type | encoder | decoder |
| --- | --- | --- | --- | --- |
| `__n0x800000` | `0x004456E0` | `uint32` | **`range`** | **`range`** |
| `__n0x7F800000` | `0x004456E4` | `uint32` | **`low`** (31 bits + carry in bit 31) | **`code`** (31 bits) |
| `__dword_4456E8` | `0x004456E8` | `uint32` | **pending count** — deferred `0xFF` bytes | **`r`** — the last `range / total` |
| `__dword_4456EC` | `0x004456EC` | `uint32` | **bytes emitted**, written into the tail | — |
| `__byte_4456F0` | `0x004456F0` | `uint8` | **cache** — the byte awaiting a possible carry | **previous input byte** |
| `__Buffer_0` | `0x00443378` | `uint8 *` | output cursor | input cursor |

The names are IDA's, and they describe nothing: IDA named each global after a
constant it saw compared against nearby. `__n0x800000` is `range`, named for
`0x800000`, the renormalisation floor it is tested against; `__n0x7F800000` is
`low`/`code`, named for `0x7F800000`, the pending-byte threshold.

Each row of the table is **one** global with two jobs, not two variables: the
word at `0x004456E4` holds `low` while encoding and `code` while decoding.
Nothing switches between them, because a run of the program only ever does one
of the two.

Three more globals are the argument-passing convention for a coding step. The
donor's `Encode`/`Decode` take no parameters; the caller fills these in first:

| global | address | meaning |
| --- | --- | --- |
| `__n0x2000_1` | `0x004456F4` | `cumFreq` — cumulative frequency below the symbol |
| `__n0x2000_0` | `0x004456F8` | `cumFreq + freq` — the top of the symbol's slot |
| `__n0x2000` | `0x004456FC` | `totFreq` — the total |
| `__n0x88` | `0x00445704` | `a + b`, set by the two-frequency binary entries |

Invariant: `0x800000 < range <= 0x80000000`, i.e. `range` is always more than
2²³ and at most 2³¹, so `low` and `code` are 31-bit quantities.

### 5.2 Encoder

**Init — `sub_414F60`**

```c
__n0x800000    = 0x80000000;   /* range = 2^31           */
__n0x7F800000  = 0;            /* low   = 0              */
__dword_4456E8 = 0;            /* no pending 0xFF bytes  */
__dword_4456EC = 0;            /* no bytes emitted yet   */
__byte_4456F0  = 0x97;         /* cache = the marker     */
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
*__Buffer_0++ = cache + c;                  /* the carry lands here      */
while (pending--) *__Buffer_0++ = c - 1;    /* 0xFF if c==0, 0x00 if c==1 */
cache = low >> 23;                          /* next top 8 bits of low     */
```

`c - 1` as a byte is `0xFF` for `c == 0` and `0x00` for `c == 1`, which is
exactly what a carry rippling through a run of `0xFF`s produces.

**Flush — `sub_414CE0`** runs the renormalisation loop once more, then writes
the tail, in this order:

```
[ cache + carry ]  [ pending run ]  [ rounding byte ]
[ 3-byte big-endian length ]
[ zero padding until (offset - __Buffer) % 4 == 3 ]  [ 0x97 ]
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
marker, `__n8`, `__n256` and `__Buffer_1` are reset, and the padding rule
guarantees the bit packer resumes on a 4-byte boundary.

### 5.3 Decoder

**Init — `sub_4149C0`**

```c
if (*__Buffer_0++ != 0x97) exit_402E40(4);   /* "Read error!" */
b = *__Buffer_0++;
__n0x800000   = 128;                          /* range = 2^7  */
__byte_4456F0 = b;                            /* previous byte */
__n0x7F800000 = b >> 1;                       /* code, 7 bits */
```

Setting `range = 2^7` forces the renormalisation loop to run three times
immediately (2⁷→2¹⁵→2²³→2³¹), which reads three more bytes and leaves `code`
holding exactly 31 bits and `range = 2³¹` — the encoder's initial state.

**Renormalise + decode**

```c
while (range <= 0x800000) {
    code  = (code << 8) | ((prev & 1) << 7) | (cur >> 1);   /* cur = *__Buffer_0++ */
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
r     = range / totFreq;              /* cached in __dword_4456E8 */
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
rt = a * (range / (a + b));
if (code >= rt) { range -= rt; code -= rt; return 1; }
else            { range  = rt;             return 0; }
```

`rc.encode_bit(f0, f1, bit)` is its encoder twin.

**Finish — `sub_414920`** renormalises, then scans forward for the `0x97`
terminator and resets the bit packer's cursor to just past it.

### 5.4 The RangeCoder class

All of the above is a `RangeCoder` struct with one instance, `rc`, declared in
subs1.hpp just after the globals block — it goes there rather than in bmf.cpp
because it shares its output cursor with the bit packer, and `__Buffer_0` is a
blob global.

The state that §5.1 lists as nine words of `blob.inc` is nine members. The one
that was a single word doing two jobs is two members, `pending` and `rdiv`,
since encoding and decoding never run together.

| method | direction | what |
| --- | --- | --- |
| `rc.enc_init()` | encode | `range = 2^31`, `low = 0`, cache = the `0x97` marker |
| `rc.dec_init()` | decode | check the marker, prime `code` with 31 bits |
| `rc.enc_normalise()` | encode | the carry-counting byte loop |
| `rc.dec_normalise()` | decode | eight more bits into the window |
| `rc.encode(cum, high, tot)` | encode | one symbol; returns `freq * r` |
| `rc.encode_bit(f0, f1, bit)` | encode | one bit against two frequencies |
| `rc.get_freq(tot)` | decode | which slot of `tot` the code falls in |
| `rc.decode(cum, high, tot)` | decode | finish the step `get_freq` started |
| `rc.decode_bit(f0, f1)` | decode | one bit against two frequencies |
| `rc.flush()` | encode | tail, length, padding, terminator |
| `rc.finish()` | decode | skip to the terminator |

Every argument is an argument. The donor had no choice about that — its
entries took no parameters and three globals *were* the argument list, so a
model function would assign `__n0x2000_1`, `__n0x2000_0` and `__n0x2000` over
the course of a symbol search and then call. Those sites now carry their own
locals and pass them, which matters because the assignments are not adjacent
to the call: `sub_4159E0` computes its `high` from a counter that it then
increments *before* coding, so moving the expression to the call site would
have coded a different number.

The state is `private`, so "does anything outside still touch the coder?" is a
question the compiler answers.

Two model functions still hold a coding step that is more than one call:
`sub_412B10` encodes a symbol against a sorted frequency list (§7.3) and
`sub_414620` decodes one from a 3-way counter node. Both now do their search
and then call `rc`.

## 6. The modelling front end

Before any entropy coding, the image is taken apart. All of this is reversed
exactly by the decoder, so it is part of the format.

### 6.1 Plane split

```c
__n4_5 = ((bpp & 0x3F) + 7) >> 3;      /* 0x00443394 — number of planes */
```

Interleaved pixels are de-interleaved into planes with a strided copy: plane
`k` is `data[k], data[k+n], data[k+2n], …`. 24-bit RGB becomes three planes,
32-bit RGBA four, 8-bit and below one.

### 6.2 Per-plane descriptor

A 16-byte record per plane at `0x0044339C`, indexed `[16 * k]`:

| offset | global | meaning |
| --- | --- | --- |
| +0 | `__byte_44339C` | number of reference planes in the colour transform, 0–3 |
| +1 | `__byte_44339D` | this plane's byte offset within an interleaved pixel |
| +2 | `__byte_44339E` | flags: bits 0–1 = spatial predictor mode (→ `__n2`), bit 2 = alternate model (→ `__dword_443364`), bit 3 = colour transform present |
| +3 | `__byte_44339F` | constant bias |
| +4 | `__dword_4433A0` | coefficient 0 |
| +8 | `__dword_4433A4` | coefficient 1 |
| +12 | `__dword_4433A8` | coefficient 2 |

`sub_402FE0` writes these into the bit stream ahead of the coded data: 6 bits of
`(flags << 2) | nrefs`, then — if the transform is present — 8 bits of bias, and
8 bits per coefficient, each stored biased by `+64`.

### 6.3 Colour transform — `sub_407EF0`

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

`sub_407EF0` also short-circuits the two-reference form: when `c₀ + c₁ == 128`
and one of them is zero, it drops to the `n == 1` path against whichever plane
still has weight, skipping the multiply.

Everything is byte arithmetic, so each subtraction wraps mod 256 and is exactly
invertible. The `n == 1` case is the familiar green-as-predictor RGB
decorrelation; `n == 2` and `n == 3` let the filter search fit a linear
combination instead of assuming one.

### 6.4 Spatial prediction — `sub_4108C0`

In predictor mode 1 — the mode the filter search picks for most planes — each
plane is predicted in place by `sub_4108C0`, walking **backwards** from the last
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
accumulated in `__buf` for the filter search.

`__byte_44339E & 3` (`__n2`, `0x00443360`) chooses the mode, and it interacts
with `E`. The dispatch, read off `sub_407B30`, is:

| `__n2` | `E == 0` | `E != 0` |
| --- | --- | --- |
| 0 | no spatial prediction | `sub_4111B0` |
| 1 | `sub_4108C0` — MED, above | `sub_410AC0` |
| 2 | skipped entirely | skipped entirely |
| 3 | no spatial prediction | `sub_410AC0` |

So MED is mode 1, mode 2 turns the stage off, and what distinguishes 0 from 3 I
did not establish (§9).

### 6.5 Near-lossless

`-E<N>` (`__n7_1`, `0x004410A0`) is copied into `__n256_0` (`0x00443398`) and
becomes the maximal allowed error `E`. When it is non-zero the residual is
quantised with step `2E + 1`:

```c
n128_1 = 2 * __n256_0[0] + 1;
```

`sub_4111B0` (encode) and `sub_410650` (decode) build the quantisation and
reconstruction tables with SSE, then run the same MED loop against the
*reconstructed* neighbours so encoder and decoder stay in step. This is the one
mode where the round trip is not bit-exact, by design.

### 6.6 Filter search — `sub_4043E0`, cost by `sub_411700`

`-F` (`__dword_44108C`) turns the search on; `-Q<N>` (`__n7_0`, `0x0044109C`)
sets how hard it looks. It appears as a table-size multiplier
(`(__n7_0 + 5) / 3`, `(__dword_443384 + __n7_0 + 1) << 6`) and as several
`__n7_0 > k` gates that enable extra candidate transforms, so raising it both
widens the search and enlarges the structures it searches over.

Candidates are scored with `sub_411700`, which is a plain **order-0 entropy in
bits** of a histogram of `n` counts summing to `N`:

```
H = (N·log N  −  Σᵢ nᵢ·log nᵢ) · log₂(e)
```

computed in `double`, two histogram entries at a time in SSE. `sub_436E10`
supplies the two-lane `log`, and takes a mask: where a lane's count is zero it
substitutes `0.5` (`__bmf_half_half`, `0x0043B480`) so that `log` is never
handed a zero, and the caller then masks those lanes back out of the running sum
with `_mm_andnot_ps`. The substituted value is never used — it exists to keep a
`-inf` out of the vector. This
and the cost functions in `sub_405CF0`, `sub_407460`, `sub_40A8A0`, `sub_41A130`,
`sub_41C4B0` and `sub_41CAB0` are the floating-point part of the compressor, and
the reason the SSE denormal mode (`bmf_set_denormal_mode`, §5 of the build) is
set at startup: flush-to-zero and denormals-are-zero keep these sums fast and
identical run to run. The search picks the transform and predictor whose folded
residuals have the lowest estimated entropy, and records them in the per-plane
descriptor.

`-T<N>` (`__n2_4`) stores or reuses the chosen filter set as a template, so a
run of similar images can skip the search.

## 7. Entropy coding

Two back ends. `__dword_443384` (`0x00443384`), copied from `-S`
(`__dword_441090`), chooses between them per image, and the choice is recorded
in descriptor byte +11 bit `0x04` so the decoder follows.

### 7.1 Fast path (default) — `sub_408510` / `sub_40CF80`

Without `-S`, the folded residuals go through `sub_408510` (encode) and
`sub_40CF80` (decode). These write through the **bit packer**, not the range
coder — they never touch `__n0x800000`. They build histograms and call
`sub_411700` to measure them, which says a code is being *chosen* from the
statistics rather than adapted; what kind of code, I did not establish. See §9.

*(Note: this path crashes in the tree as it stands — `bmf` without `-S` faulted
in the imported sources too, which is why the CLI pins `-S`. The observation is
about the decompilation, not necessarily about the original binary.)*

### 7.2 Slow path (`-S`) — `sub_415380` and below

This is the one `bmf c` uses. It is a binary context model driving the range
coder of §5.

`sub_415380` runs the plane through this pipeline, in this order:

1. `malloc(0x7BA230)` — an ~8 MB workspace — laid out by `sub_417980`.
2. `sub_414F60` — start the range coder (§5.2).
3. `sub_417200` — **alphabet reduction**, and the first thing written. It scans
   the plane for the symbols actually used, encodes `distinct - 1` with
   `rc.encode` as a flat one-wide slot out of the full alphabet size, encodes each used
   symbol with `sub_412B10`, and then re-indexes the plane onto the dense
   alphabet that leaves. A plane using 40 of 256 values is coded over 40
   symbols from here on.
4. Model initialisation — the tables described below.
5. `sub_4159E0` — the per-pixel coder, once per pixel.
6. `sub_414CE0` — flush the range coder (§5.2).

When `__dword_443364` (descriptor `+2` bit 2) is set, `sub_415380` instead
dispatches to one of `sub_424500` / `sub_424D90` / `sub_4197A0` / `sub_421930`,
picked by predictor mode and by whether the depth is exactly 8. Those are
separate model families and are not described here.

What the model initialisation builds:

* **15 context groups.** The loop runs `v58` from 0 to 14, indexed by a 15-entry
  table of flag bytes at `__byte_439860` (`0x00439860`). Each flag bit selects
  whether one of six inputs is folded into that group's context.
* **A 64K counter table per group.** Each group gets `0x10000` entries of two
  `uint16` counters, both initialised to `0x2000`:

  ```c
  LOWORD (v24[n + 531818]) = 0x2000;
  HIWORD (v24[n + 531818]) = 0x2000;
  ```

  Two 8192s is p = ½ — a fresh binary counter. The index is a 16-bit value
  `sub_4159E0` derives from the neighbourhood, of the form
  `table[N] - <one of N, W, NE, NW>`; exactly which table and which neighbour is
  in §9's unread list.
* **A 5×5 sub-state grid per group**, with mixing weights `1 << (5 - depth)` and
  a running total in `*((uint16 *)v12 + 53)`. `sub_415380` computes, for each
  cell, how many of the six inputs are active and derives the weight from that
  count — a static mixing table rather than a learned one.

`sub_4159E0` is the per-symbol coder on this path. It forms a context by
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

**Binary pair — `sub_413430`.** Walks a bit tree; at each node

```c
if (counter > 0x2000) {            /* rescale before it saturates */
    c0 -= c0 >> 1;
    c1 -= c1 >> 1;
}
counter += (__n8_0 * ((__n2 == 2) + 5)) >> 3;   /* adaptive increment */
```

**Three-way node — `sub_414620`,** the decoder for a node holding three
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

**Sorted list — `sub_412B10`.** For alphabets rather than bits: a list of
`(symbol, count)` triples kept in descending count order. Encoding sums counts
until the symbol is found (that sum is `cumFreq`), calls the range coder, then
adds 4 to the symbol's count and bubbles it toward the front by swapping with
its neighbour. Symbols masked out by `__buf_0` are skipped, and running off the
end of the list codes an escape — a PPM-style exclusion mechanism.

## 8. Global map

Every address is an offset into `blob.inc`, BMF.exe's data segment.

**Range coder** — §5.1 has the full table.

| global | address | meaning |
| --- | --- | --- |
| `__n0x800000` | `0x004456E0` | `range` |
| `__n0x7F800000` | `0x004456E4` | `low` (encode) / `code` (decode) |
| `__dword_4456E8` | `0x004456E8` | pending count (encode) / `r` (decode) |
| `__dword_4456EC` | `0x004456EC` | bytes emitted |
| `__byte_4456F0` | `0x004456F0` | cache byte (encode) / previous byte (decode) |
| `__n0x2000_1` | `0x004456F4` | `cumFreq` |
| `__n0x2000_0` | `0x004456F8` | `cumFreq + freq` |
| `__n0x2000` | `0x004456FC` | `totFreq` |
| `__n0x88` | `0x00445704` | `freq0 + freq1` for the binary entries |

**Streams and buffers**

| global | address | meaning |
| --- | --- | --- |
| `__Buffer` | `0x0044337C` | coded buffer base |
| `__Buffer_0` | `0x00443378` | shared byte cursor |
| `__Buffer_1` | `0x00443374` | bit packer's current word |
| `__n256` | `0x0044336C` | bit packer accumulator |
| `__n8` | `0x00443368` | free bits in it |
| `__ElementCount_0` | `0x00443370` | buffer size |
| `__buf` | `0x00443380` | scratch histogram at `__Buffer + size - 4096` |
| `__buf_0` | `0x00443440` | symbol exclusion mask |
| `__n256_1` | `0x00445708` | model table allocated by `sub_414F60` / `sub_4149C0` |

**Coding parameters**

| global | address | set from | meaning |
| --- | --- | --- | --- |
| `__dword_44108C` | `0x0044108C` | `-F` | use filters (default 1) |
| `__dword_441090` | `0x00441090` | `-S` | slow mode — **pinned to 1** |
| `__n2_4` | `0x00441094` | `-T` | template usage |
| `__dword_441098` | `0x00441098` | `-N` | pack output (default 1) |
| `__n7_0` | `0x0044109C` | `-Q` | filter search quality — **pinned to 9** |
| `__n7_1` | `0x004410A0` | `-E` | max error, near-lossless (default 0) |
| `__dword_4410A4` | `0x004410A4` | | filter template |

**Per-image working state**

| global | address | meaning |
| --- | --- | --- |
| `__n4_5` | `0x00443394` | number of colour planes |
| `__n2` | `0x00443360` | current plane's predictor mode |
| `__dword_443364` | `0x00443364` | current plane's alternate-model flag |
| `__dword_443384` | `0x00443384` | slow mode active for this image |
| `__n256_0` | `0x00443398` | near-lossless max error `E` |
| `__byte_44339C…__dword_4433A8` | `0x0044339C` | per-plane descriptors, 16 bytes each |
| `__byte_439860` | `0x00439860` | 15 context-group flag bytes |
| `__dword_445660` | `0x00445660` | model geometry tables built by `sub_414F60` |

## 9. What is not settled

Stated plainly, so the rest can be trusted:

* **The fast (non-`-S`) back end.** `sub_408510` is 1847 lines and I only
  established that it writes through the bit packer and uses `sub_411700` for
  cost estimates. Whether it is Huffman, Golomb–Rice, or something else is not
  established here.
* **The `-S` context construction in detail.** §7.2 gives the shape — 15 groups,
  64K binary counters each, a 5 × 5 sub-state grid, neighbour-match state
  indices — but not the exact derivation of the 16-bit context index, nor how
  the 15 groups' predictions are combined into the one probability handed to the
  coder. `sub_4159E0` is 901 lines and would need a full reading.
* **The alternate model families.** When descriptor `+2` bit 2 is set,
  `sub_415380` hands the plane to `sub_424500` / `sub_424D90` / `sub_4197A0` /
  `sub_421930` instead. I established only which one is picked, not what they
  do; `sub_41CAB0` (2320 lines) sits under two of them.
* **What predictor modes 0 and 3 mean.** The dispatch table in §6.4 is read off
  the code, but 0 and 3 land in the same places and I did not work out what
  distinguishes them, nor what mode 2 does instead of predicting.
* **The `sub_4043E0` search order.** Which candidates are tried, and how `-Q`
  prunes them, is visible as loop bounds but I did not enumerate the candidate
  set.
* **The first-byte carry.** The decoder demands exactly `0x97`, so a carry into
  the coder's initial cache would break it. Encoding is correct on every test
  image, so the design evidently precludes it, but I did not prove why.

## 10. Where to look

| topic | function | address |
| --- | --- | --- |
| compress one image | `sub_402FE0` | `0x00402FE0` |
| expand one image | `sub_403820` | `0x00403820` |
| range coder init / flush (encode) | `sub_414F60` / `sub_414CE0` | `0x00414F60` / `0x00414CE0` |
| range coder init / finish (decode) | `sub_4149C0` / `sub_414920` | `0x004149C0` / `0x00414920` |
| the coder itself | `struct RangeCoder`, instance `rc` | — |
| colour transform | `sub_407EF0` | `0x00407EF0` |
| MED prediction + folding | `sub_4108C0` | `0x004108C0` |
| near-lossless encode / decode | `sub_4111B0` / `sub_410650` | `0x004111B0` / `0x00410650` |
| filter search | `sub_4043E0` | `0x004043E0` |
| entropy estimate | `sub_411700` | `0x00411700` |
| `-S` model driver | `sub_415380` | `0x00415380` |
| alphabet reduction | `sub_417200` | `0x00417200` |
| `-S` per-pixel coder | `sub_4159E0` | `0x004159E0` |
| adaptive symbol list | `sub_412B10` | `0x00412B10` |
| BMP read / write | `sub_42AB20` / `sub_42B0C0` | `0x0042AB20` / `0x0042B0C0` |
