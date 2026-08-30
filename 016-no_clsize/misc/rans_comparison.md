# Four vectorised entropy coders, compared

`rz/rans.h` + `rz/rans-inl.h`, `lolz/rans.h`, `oodle/lzna.inc`, and this
project. Read from source only — see §8 for why there are no speed numbers
across implementations.

## 1. The first thing to say: only three of them are rANS

| | coder | alphabet | streams |
|---|---|---|---|
| rz | rANS | nibble-ish, N = 16…32 | 2, swapped |
| lolz | rANS | 4 / 8 / 16 / 32, plus binary | 2, interleaved |
| LZNA | rANS | nibble (16) and 3-bit (8), plus binary | 2, swapped |
| **ours** | **binary range coder** | **binary only** | **16, independent** |

This is not pedantry — it decides everything below. rANS carries the whole
coder in one integer `x` and recovers the symbol by *searching* a cumulative
frequency table with `x mod M`. A range coder carries an interval `(low,
range)` and recovers the symbol by *comparing* against a scaled split point.
The first wants a wide alphabet and a table; the second is at its best binary,
where the "search" is one compare.

So the four split two-and-two on what SIMD is even for:

- **rz, lolz, LZNA vectorise INSIDE one symbol.** One state, one model, and the
  16 or 32 cumulative frequencies are searched and updated in parallel. The
  coder itself is scalar.
- **We vectorise ACROSS symbols.** The coder state is sixteen independent
  lanes, one substream each, and the sweep is 16-wide. The model is scalar.

Both are ways to fill a vector register; they are not the same width axis, and
neither subsumes the other.

## 2. Coder parameters

| | state | L (renorm floor) | renorm unit | direction | probability scale |
|---|---|---|---|---|---|
| rz | 32-bit ×2 | 2^16 | 16 bits | **backward** (`*--stream_end_`) | 14-bit (`0x3fff`) |
| lolz | typedef'd¹ | `RANS_L`¹ | `RansOut_t`¹ | forward decode, backward encode | `scale_bits`, a template parameter |
| LZNA | **64-bit** ×2 | 2^31 | **32 bits** | forward (`*tab->src++`) | 15-bit (`0x7FFF`) |
| ours | 32-bit ×16 | 2^24 (`sTOP`) | **8 bits** | backward substreams | 15-bit (`SCALElog`) |

¹ `RansState_t`, `RansOut_t` and `RANS_L` are typedef'd outside the file we
were given.

LZNA's 64-bit state with a 32-bit renorm is the outlier and it is deliberate:
one refill covers ~32 bits of output, so the refill branch is taken rarely and
the whole `if` predicts almost perfectly. rz's 32/16 split refills twice as
often. Ours refills 8 bits at a time and takes the branch on about 8% of bits —
which is why the decoder's shape is a *branch* rather than a branchless shift
(`misc/dec_renorm.md`).

## 3. The shared nibble-model idiom

All three rANS coders do the identical five-step trick for "find which of N
cumulative frequencies contains `x mod M`". LZNA:

```c
t  = broadcast(x & 0x7FFF);            // the slot to locate
c0 = _mm_cmpgt_epi16(t0, t);           // prob[i] > slot ?
c1 = _mm_cmpgt_epi16(t1, t);
_BitScanForward(&bitindex,
    _mm_movemask_epi8(_mm_packs_epi16(c0, c1)) | 0x10000);
start = model->prob[bitindex - 1];
end   = model->prob[bitindex];
```

rz is the same with `_mm_sub_epi16` and `BSR` instead of compare and BSF; lolz
is the same again with `_mm_cmplt_epi16` and `_BitScanForward`, generalised to
4/8/16/32 by template parameter. Sixteen-way search in ~6 instructions, no
branch, no table walk. It is the single best idea in all three files and they
arrived at it independently enough to differ only in bit-twiddling.

The decode step that follows is textbook rANS, and identical in all three:

```
x = freq * (x >> scale_bits) + (x & mask) - start
```

**No division on the decode path** — for any of them. That is rANS's real
advantage over a multi-symbol range coder, which needs `x / range_unit` to find
the symbol.

## 4. Where they actually differ: the mixin

All three adapt with the same shape — pull every cumulative frequency a
fraction of the way toward a per-symbol target, at rate 1/128:

```c
t0 = _mm_add_epi16(_mm_srai_epi16(_mm_sub_epi16(c0, t0), 7), t0);
```

but they get the target `c0` differently, and this is the sharpest difference
in the set:

- **rz and lolz load it from a table.** rz: `NibbleModelUpdate<N>::v[symbol][]`,
  an N×N array of `uint16` — 512 bytes for N=16, ~2 KB for N=32, one per
  alphabet size. lolz: the same thing as a template parameter,
  `CDF_upd_tabl[symbol * sym_size]`.
- **LZNA synthesises it from the compare mask it already has:**

```c
c0 = _mm_and_si128(_mm_set1_epi16(0x7FD9), c0);        // 0x7FD9 above the symbol
c0 = _mm_add_epi16(c0, _mm_set_epi16(56,48,40,32,24,16,8,0));   // + a ramp
```

The target is a step function — 0 below the decoded symbol, `0x7FD9` above it —
plus a small `8*i` ramp that keeps the CDF strictly increasing so no symbol's
frequency can collapse to zero. It costs two constants and two instructions and
**touches no memory at all**, where rz and lolz spend a 32-byte table load per
symbol and carry a table per alphabet size.

The table version buys generality: an arbitrary target CDF per symbol, which
lets you shape adaptation (faster toward common symbols, say). LZNA gives that
up for a fixed step-plus-ramp. Given that the update runs on every symbol, and
that the mask is already in a register, LZNA looks like the better trade unless
you actually want a non-uniform mixin.

## 5. The encoder asymmetry, which is the real cost of rANS

rANS decodes forward but **must encode backward** — the last symbol is encoded
first. Every rANS encoder therefore needs the whole symbol sequence buffered
before it can emit a byte. lolz is completely explicit about this:

```c
RansPreEnc_AddSym(preenc, start, freq);        // pass 1: record (start,freq)
...
for (intptr_t i = num_records - 1; i >= 0; i -= 2) {   // pass 2: replay backwards
  RansEnc_Put(&enc_rans_state2, &ptr, buf[i].start,   buf[i].freq,   ...);
  RansEnc_Put(&enc_rans_state1, &ptr, buf[i-1].start, buf[i-1].freq, ...);
}
```

`rz`'s `RansWriter` has the same shape (an `array_` of `uint16` filled forward,
flushed in reverse). And the encode step itself needs a division:

```c
*r = ((x / freq) << scale_bits) + (x % freq) + start;
```

So rANS pays, on the encoder: a full buffering pass over the block, a
`RansPreEncInfo` record per symbol, and a hardware divide per symbol.

**Our range coder encodes forward and divides never.** The binary step is
`rpre = (range>>SCALElog)*p`, a shift and a multiply, and the output is written
as it is produced. That is what lets our encoder be a single vectorised sweep
over 16 lanes with no second pass — and it is the reason our encoder is roughly
twice our decoder's speed, where an rANS codec is usually the other way round.

This is the trade in one line: **rANS moves work from the decoder to the
encoder; a binary range coder splits it evenly and needs no division on either
side.**

## 6. Interleaving: two states versus sixteen lanes

rz and LZNA both hold two states and swap them on every operation:

```c
// LZNA
tab->bits_a = tab->bits_b;
tab->bits_b = x;
// rz
std::swap(state1_, state2_);
```

This is a two-deep software pipeline in disguise. Symbol *n* uses state A while
symbol *n−1*'s update to state B is still in flight, so the serial
multiply-and-renorm chain overlaps with itself. It costs one extra register and
no code, and it is the cheapest ILP available to a single-stream coder.

Ours goes much further and pays for it in a different currency: sixteen
independent coders, sixteen substreams in the output, and a lane index threaded
through everything. That buys 16-way ILP and makes the *coder* vectorisable,
but it means the format carries 16 separate byte streams with their own length
headers, and the model has to be walked 16 times to feed it.

Neither is strictly better. Two-state swapping is nearly free and gets you 2×;
sixteen lanes gets you 16× on the coder and costs you the file format, a
per-lane cursor, and — as this project measured at length — a decoder whose
model side then becomes the bottleneck rather than the coder.

## 7. Model sophistication

Worth noting because it changes what the coder is being asked to do:

- **LZNA** is a complete LZ model: literal/match/distance/length submodels,
  `next_state_lit[12]` state machine, nibble models for distances split
  hi/lo. The rANS coder is a component.
- **lolz** goes furthest on modelling: it has **CM mixing** —
  `RansCM_mix` blends two CDFs with a learned weight using `_mm_mulhi_epu16`,
  and `RansCM_wt_Update` adapts the weight — plus a price table
  (`kRansPriceBits = 13`) for optimal parsing, and a cascade coder for variable
  alphabet sizes. This is the only one of the four that mixes models.
- **rz** sits in between: adaptive nibble models plus a `RansVarint` layer
  (model-coded exponent, raw mantissa bits) — the classic way to code lengths
  and distances cheaply.
- **ours** is the simplest by far: an order-0 bit-tree per byte over an FSM
  counter, 255 contexts. The project's interest is the coder, not the model.

lolz's `RansCM_mix` is the one idea here with no counterpart on our side, and
it is not coder work at all — it is a mixing model that happens to be
vectorised the same way the CDFs are.

## 8. Why there are no cross-implementation speed numbers

There is no honest way to produce them from what we have:

- The three rANS files are **fragments**. `lolz/rans.h` references
  `lolz_struct_t`, `RANS_L`, `RansState_t` and its CDF tables, none of which
  are present. `oodle/lzna.inc` is decoder-only. `rz/rans.h` is a header whose
  `.cpp` is missing.
- They are **not the same workload**. LZNA and lolz are LZ codecs; the coder
  runs on a token stream from a match finder. Ours runs on an order-0 bit
  stream. "MB/s" means a different thing in each.
- They are **not the same alphabet**. Comparing a nibble decode against a bit
  decode per unit time is meaningless without normalising by symbols, and
  normalising by symbols hides that a nibble carries four bits.

What can be said from source: the rANS decoders do ~6 instructions of search
plus a multiply per *nibble*; ours does a compare, a multiply and a predicted
branch per *bit*, times sixteen lanes per group. At four bits per nibble those
are closer than they look, and which wins is a property of the model and the
machine, not of the coder.

## 9. What each could take from the others

**For us, from LZNA:** the wider renorm. Ours refills 8 bits and takes a branch
on ~8% of bits; LZNA refills 32 and takes one on ~3%. Our 8-bit unit is forced
by the carryless-with-fallback design (`RC_LOWBYTES`, the FF run), not by the
coder, so this is not free — but the branch-frequency argument in
`dec_renorm.md` says the outer test is worth 17–36%, and a wider renorm makes
that test rarer still.

**For us, from all three:** nothing on the model side transfers, because ours is
binary. The SIMD cumfreq search has no binary analogue — with two symbols the
"search" is the compare we already do.

**For them, from us:** lane interleaving beyond two states. Both rz and LZNA
stop at two swapped states; nothing prevents four or eight, and this project
measured that more independent chains is exactly what a front-end-bound decode
loop wants. The cost is the output format, which is the same cost we paid.

**For lolz, from LZNA:** the synthesised mixin (§4) would remove a table load
per symbol from an inner loop that already has plenty to do.

## 10. Summary

| | rz | lolz | LZNA | ours |
|---|---|---|---|---|
| coder | rANS | rANS | rANS | binary range coder |
| state | 32×2 | ? ×2 | 64×2 | 32×16 |
| renorm | 16 bits | ? | 32 bits | 8 bits |
| decode division | no | no | no | no |
| **encode division** | **yes** | **yes** | (n/a) | **no** |
| **encode buffering** | **whole block** | **whole block** | (n/a) | **none** |
| SIMD used for | CDF search + update | CDF search + update + CM mix | CDF search + update | the coder itself, 16 lanes |
| mixin target | table | table | synthesised | (binary: FSM) |
| model | nibble + varint | nibble + CM mixing | full LZ | order-0 bit tree |

The single sentence: **the three rANS coders vectorise the model of one stream;
we vectorise the coder across sixteen streams** — and that choice, not any
detail of the arithmetic, is what makes their encoders need a buffering pass
and a divider while ours does not, and what makes their decoders fast on wide
alphabets while ours needs sixteen lanes to keep up on bits.
