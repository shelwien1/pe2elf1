# Four vectorised rANS coders, compared

`rz/rans.h` + `rz/rans-inl.h`, `lolz/rans.h`, `oodle/lzna.inc`, and this
project's `rans.inc`. Read from source only -- see §10 for why there are no
speed numbers across implementations.

An earlier draft of this document compared three rANS coders against our
*range* coder, and most of it was about that difference. `rans.inc` removed
it. All four are now rANS, so the comparison is between four answers to the
same question rather than between two kinds of coder, and the interesting
line falls somewhere else entirely.

## 1. Where the line actually falls: which axis gets vectorised

| | alphabet | states | SIMD is used for |
|---|---|---|---|
| rz | nibble-ish, N = 16…32 | 2, swapped | searching + updating one CDF |
| lolz | 4 / 8 / 16 / 32, **plus binary** | 2, interleaved | searching + updating one CDF, and CM mixing |
| LZNA | nibble (16), 3-bit (8), **plus binary** | 2, swapped | searching + updating one CDF |
| **ours** | **binary only** | **16, independent** | **the coder itself, across 16 lanes** |

A vector register has to be filled with *something*. The first three fill it
with the 16 or 32 cumulative frequencies of one symbol: the coder stays scalar
and SIMD does the alphabet. We fill it with 16 independent coder states: the
model stays scalar and SIMD does the coder.

These are different axes, and neither subsumes the other. But they are not
symmetric in what they cost, and that asymmetry is most of this document:

- **Vectorising the alphabet is local.** It changes one function. The stream
  format, the encoder's structure and the state layout are untouched, which is
  why all three arrived at nearly the same code independently.
- **Vectorising across lanes is global.** It puts 16 substreams and a length
  table in the file format, it makes the state a `[16]` array that every
  operation indexes, and -- the part that is easy to miss -- it takes the
  encoder's division away from you, because x86 has no vector integer divide.

§6 is that last point, and it is the only place where our design was forced
into something none of the others needed.

## 2. Coder parameters

| | state | L | renorm unit | direction | scale |
|---|---|---|---|---|---|
| rz | 32-bit ×2 | 2^16 | 16 bits | backward (`*--stream_end_`) | 14-bit (`0x3fff`) |
| lolz | `RansState_t` ×2¹ | `RANS_L`¹ | `RansOut_t`¹ | forward decode, backward encode | `scale_bits`, a template parameter |
| LZNA | **64-bit** ×2 | 2^31 | **32 bits** | forward (`*tab->src++`) | 15-bit, per-site for binary² |
| ours | 32-bit ×16 | 2^23 (`SCALE<<RC_RANS_KLOG`) | **8 bits** | backward | 15-bit (`SCALElog`) |

¹ typedef'd outside the file we were given.
² `LznaRead1Bit(tab, model, nbits, shift)` takes the scale per call site.

Our `L` is the one that is not a free choice. The state has to be 32 bits so
that sixteen of them fit a 512-bit register; `L` must be a multiple of
`M = 2^15`; and `L·b` must fit the word. With `b = 256` that leaves
`L/M ∈ [1, 256]`, and `RC_RANS_KLOG` picks it. §7 has what the low end costs.

## 3. The shared idiom: SIMD cumulative-frequency search

All three multi-symbol coders do the identical five-step trick for "which of N
cumulative frequencies contains `x mod M`". LZNA:

```c
t  = broadcast(x & 0x7FFF);              // the slot to locate
c0 = _mm_cmpgt_epi16(t0, t);             // prob[i] > slot ?
c1 = _mm_cmpgt_epi16(t1, t);
_BitScanForward(&bitindex,
    _mm_movemask_epi8(_mm_packs_epi16(c0, c1)) | 0x10000);
start = model->prob[bitindex - 1];
end   = model->prob[bitindex];
```

rz is the same with `_mm_sub_epi16` and `BSR` instead of compare and `BSF`;
lolz is the same again with `_mm_cmplt_epi16` and `_BitScanForward`,
generalised to 4/8/16/32 by template parameter. Sixteen-way search in ~6
instructions, no branch, no table walk. It is the best idea in all three files
and they differ only in bit-twiddling.

**We have no counterpart, and cannot have one.** With two symbols the search
*is* one compare -- `(x & mask) >= p` -- so there is nothing left to
parallelise inside a symbol. That is not a gap in our implementation; it is
what "binary" means. It is also why the lane axis was the only one available
to us.

## 4. The binary step: three implementations of the same algebra

This is the comparison the earlier draft could not make, and it is the
sharpest one in the set. Three of the four code single bits, and all three
reach the identical pair of expressions.

lolz (`RansDec_Update_0` / `_1`):

```c
x = prob * (x >> scale_bits) + cumm_freq;        // bit 0
x = x - prob * (x >> scale_bits) - prob;         // bit 1
```

LZNA (`LznaRead1Bit`):

```c
q = *model * (x >> nbits);
x = (x & (magn-1)) + q;                          // bit 0
x -= q + *model;                                 // bit 1
```

ours (`rans.inc`, `f_DEC==1`):

```c
const uint _s = rx & mSCALE;
const uint _a = freq * (rx>>SCALElog);
_b = (_s>=freq);
rx = _b ? (rx - freq - _a) : (_a + _s);
```

All three are `f*(x>>15) + s - start` specialised to the two arms, using the
same identity to avoid computing `M-p` on the `bit==1` side:

```
(M-p)*(x>>15) + s - p  ==  x - p*(x>>15) - p        because  M*(x>>15) + s == x
```

The difference is not the arithmetic, it is the control flow. **lolz and LZNA
branch on the bit and evaluate one arm; we evaluate both and select.** They
branch because they can: one state, and the two arms have different model
updates anyway. We cannot, because sixteen lanes take sixteen different
branches -- and it costs us almost nothing, because both arms are built from
the same `_a = p*(x>>15)`: a couple of extra adds and a blend, over a multiply
the branchy version pays too.

That is a small, pleasant result: the constraint that forced the branchless
form is the same structure that made the branchless form cheap. It does not
generalise -- it works because a binary alphabet has exactly two arms and they
share their only multiply.

Second difference, and this one is ours alone: **we renormalise before the
step, not after it.** lolz and LZNA both refill at the end of a decode, which
is the textbook placement. Ours sits at the head of the next step. It is the
same point in the sequence, reached one step earlier, and it is worth having
because it moves the refill off the path between the state update and the
compare that produces the next bit. It costs one small thing, documented in
`rc_io.inc`: the encoder's very first renormalisation has no step after it to
undo it, so 0-2 bytes at the bottom of each substream are written and never
read back.

## 5. The mixin: table versus synthesised

The three multi-symbol coders adapt with the same shape -- pull every
cumulative frequency a fraction of the way toward a per-symbol target at rate
1/128 -- but they get the target differently:

```c
t0 = _mm_add_epi16(_mm_srai_epi16(_mm_sub_epi16(c0, t0), 7), t0);
```

- **rz and lolz load it from a table.** rz: `NibbleModelUpdate<N>::v[symbol][]`,
  an N×N `uint16` array -- 512 bytes for N=16, ~2 KB for N=32, one per
  alphabet size. lolz: the same as a template parameter,
  `CDF_upd_tabl[symbol * sym_size]`.
- **LZNA synthesises it from the compare mask it already has:**

```c
c0 = _mm_and_si128(_mm_set1_epi16(0x7FD9), c0);                  // step at the symbol
c0 = _mm_add_epi16(c0, _mm_set_epi16(56,48,40,32,24,16,8,0));    // + a ramp
```

The target is a step function -- 0 below the decoded symbol, `0x7FD9` above --
plus an `8*i` ramp that keeps the CDF strictly increasing so no frequency can
collapse to zero. Two constants, two instructions, no memory, and the mask is
already in a register. The table version buys an arbitrary per-symbol target
(you could adapt faster toward common symbols); LZNA gives that up. Given that
this runs on every symbol, LZNA looks like the better trade unless you
actually want a non-uniform mixin.

**Ours is a third thing again**: a binary model has one probability, so
there is no CDF to mix into. `FSM0.txt` is a state machine -- the counter's
next state and its probability come from one table lookup
(`RC_FUSE_PP_ENC`/`_DEC` fold them into a single entry), and the adaptation
rate is baked into the machine rather than applied arithmetically. Cheaper
than all three, and it does strictly less.

## 6. The encoder, where our design was forced

Every rANS encoder must run backwards over the symbols, so every one of them
buffers. That is not a distinguishing feature -- lolz records
`RansPreEncInfo{start, freq, scale_bits}` per symbol and replays in reverse,
rz fills a `uint16 array_` forward and flushes it backward, and we run the
model pass into `pbit[]` and sweep it in reverse. Ours is the cheapest of the
three almost by accident: `pbit[]` is the model pass's output, which the
*range coder* build writes too, so the buffer was already there for
vectorisation reasons and rANS did not add it. What rANS did cost us is
`RC_CHUNK` -- interleaving the model pass and the coding pass a chunk at a
time, worth 3% -- which a reverse sweep cannot do, because the last bit of the
block has to be coded first.

The division is the real difference. lolz:

```c
*r = ((x / freq) << scale_bits) + (x % freq) + start;
```

a hardware integer divide and remainder per symbol. That is the normal cost of
rANS encoding and every scalar implementation here pays it.

**We could not.** There is no integer divide in any x86 SIMD instruction set,
and the divisor here is data (`f` comes out of the model), so the
magic-multiplier trick would need its magic computed per symbol. Sixteen lanes
of `x/f` had to become something a vector unit can do:

```c
RANS_DIV(a,b) == (a) * (RANS_BIAS/(b))     // model.inc, RANS_BIAS = 1-2^-19
q = (uint)(int)RANS_DIV( (float)(int)rx, (float)(int)_f );
r = rx - q*_f;
u = -(r >= _f);                            // the quotient may be one low
rx = (q<<SCALElog) + r + _c + ((SCALE-_f) & u);
```

float32 has 24 mantissa bits and the post-renormalisation quotient is below
`2^16`, so a reciprocal multiply carries it -- but only if the rounding is
*one-sided*. Without the `1-2^-19` bias the quotient rounds up about once in
800 symbols, `r` wraps negative, and the correction makes it worse rather than
better. `misc/rans_div.cpp` settles that exhaustively: the whole question
reduces to where `f*rcp` falls in a window that depends only on `f`, and there
are 32767 frequencies, so all of them are checked rather than sampled.

Two things worth taking from this beyond our own build. First, writing it as a
reciprocal rather than a divide is worth 4% *even scalar*, because the
reciprocal depends only on `f` and stays off the loop-carried chain -- lolz's
`x / freq` does not. Second, the biased-reciprocal form is a general answer to
"rANS encoding needs a divide": it is not specific to SIMD, and a scalar
encoder with a hot divider could use it. The bias is the load-bearing part,
not the float.

## 7. Renormalisation: an exact law, and what a wider unit would cost us

The refill branch's rate is not a tuning question. A rANS coder that
renormalises in units of `u` bits refills **exactly once per `u` bits of
output**, whatever the alphabet. So the branch fires, per coded symbol,
`(bits of output per symbol) / u`.

Measured on our decoder over 20 MB of enwik8 at 5.01 bpc: 12,520,077 firings
in 160,000,322 coded bits -- **7.83%**, and 1.000 bytes per firing. The
two-byte case (`x` below `L>>8`) happened 19 times in 12.5 million.

So per byte of output, all four fire the same number of times per unit `u`:
ours every 8 bits, rz every 16, LZNA every 32. LZNA's 64-bit state exists to
buy that: one refill covers 32 bits of output, the `if` predicts almost
perfectly, and it needs no loop -- a single `if`, not a `while`, because the
64/32 split guarantees one refill suffices. rz gets the same guarantee from
32/16. lolz keeps a `while`.

**We cannot follow LZNA here, and it is worth being precise about why.** A
64-bit state means eight lanes per 512-bit register instead of sixteen, which
halves the axis the whole design is built on. Staying at 32 bits and widening
the renorm unit to 16 instead forces `L·2^16 < 2^32` with `L ≥ M = 2^15`, so
`L/M ≤ 2`. That is measurable, and it is not cheap -- enwik8, encoded at each
`RC_RANS_KLOG`:

| `L/M` | 2 | 4 | 16 | 64 | 256 |
|---|---|---|---|---|---|
| bytes | 62,694,293 | 62,622,501 | 62,593,252 | 62,589,286 | 62,588,528 |
| over `L/M`=256 | +105,765 | +33,973 | +4,724 | +758 | — |

A 16-bit renorm would cost 105,765 bytes, which is **more than the entire
75,436-byte gap between our rANS and our range coder**. The 8-bit unit is not
a limitation we inherited from the range coder's design; at a 32-bit state it
is the right choice, and the branch it costs is the cheaper half of the trade.

The same table says why `RC_RANS_KLOG` is otherwise uninteresting: past
`L/M = 64` the rounding loss is under 800 bytes in 62 MB, and the substream
flush -- four bytes per lane per block, which does not move with `L` -- is the
larger term by two orders of magnitude.

## 8. Interleaving: two swapped states versus sixteen lanes

rz and LZNA both hold two states and swap on every operation:

```c
tab->bits_a = tab->bits_b; tab->bits_b = x;    // LZNA
std::swap(state1_, state2_);                    // rz
```

A two-deep software pipeline in disguise: symbol *n* uses state A while symbol
*n−1*'s update to state B is still in flight, so the serial
multiply-and-renorm chain overlaps with itself. One extra register, no code,
and it is the cheapest ILP available to a single-stream coder. lolz
interleaves two states in the encoder explicitly (`i -= 2`).

Ours goes to sixteen and pays in a different currency: sixteen substreams in
the format, a length table, a lane index threaded through every macro, and a
model that has to be walked sixteen times to feed it. What it buys is not just
16× ILP but *vectorisability* -- two states can only overlap, sixteen can
share an instruction.

The honest reading is that this project measured the limit of that trade and
found it is already behind us. `RCNUM=32` -- twice the independent chains --
does not help; on 20 MB of enwik8 it is 63.46 against 64.05 MB/s encoding and
**39.88 against 42.86 decoding**, a 7% loss, plus 0.2% of size for the extra
sixteen substreams. Sixteen lanes is not a step on the way to more; it is at
or past the point where the coder stopped being the bottleneck and the model's
working set started being one.

Which puts the interesting question on their side of the table, not ours. rz
and LZNA stop at 2, and the first few steps up are the cheap ones -- 2 to 4
buys what 16 to 32 does not.

## 9. Raw bits stuffed into the state

rz and LZNA both read unmodelled bits straight out of the rANS state:

```c
uint32 rv = tab->bits_a & ((1 << bits) - 1);  tab->bits_a >>= bits;   // LZNA
uint32 v = state2_;  state2_ = v >> nb;  return v & ((1<<nb)-1);      // rz
```

No separate bit reader, no second stream, no alignment -- the state is already
a bit reservoir and the renormaliser already refills it. For an LZ codec with
mantissa bits and literal tails this is close to free, and both take it.

We have no such path: every bit our coder sees is a modelled bit. If this
project ever grows a component that needs raw bits, this is the idiom to copy
and it costs nothing to support -- our state is a reservoir too.

## 10. Why there are no cross-implementation speed numbers

There is no honest way to produce them from what we have:

- The three files are **fragments**. `lolz/rans.h` references `lolz_struct_t`,
  `RANS_L`, `RansState_t` and its CDF tables, none of which are present.
  `oodle/lzna.inc` is decoder-only. `rz/rans.h` is a header whose `.cpp` is
  missing.
- They are **not the same workload**. LZNA and lolz are LZ codecs; the coder
  runs on a token stream from a match finder. Ours runs on an order-0 bit
  stream. "MB/s" means a different thing in each.
- They are **not the same alphabet**. A nibble decode against a bit decode per
  unit time is meaningless without normalising by symbols, and normalising by
  symbols hides that a nibble carries four bits.

The one comparison that *is* controlled is ours against our own range coder,
same model, same file, same build -- and it is in the README. Summarised:
encode −6.2%, decode +10.0%, size +0.121% on AVX-512. The direction is the
part that transfers: rANS's decode step is shorter than a range coder's
because the bit falls out of a compare rather than out of a multiply, and its
encode step is longer because of the divide. Every coder here inherits that
shape.

## 11. What each could take from the others

**For us, from LZNA:** not the wider renorm -- §7 measured that and it costs
more than it saves. The synthesised-mixin *idea* has no binary analogue
either. What does transfer is the raw-bit path in §9, if we ever need it.

**For us, from lolz:** `RansCM_mix` is the one thing in these files with no
counterpart on our side and a real use for us. It mixes two models' CDFs with
a learned weight; the binary case is one `mulhi` and a shift-update, and it
would drop into our model without touching the coder at all. It is also the
only idea here that would change our *ratio* rather than our speed.

**For them, from us:** lane interleaving past two states, and the branchless
binary step in §4 if they ever want to vectorise one. Also the biased
reciprocal in §6 -- lolz's encoder does an integer `x / freq` per symbol on
the critical path, and the reciprocal form is 4% faster scalar for the same
reason it is necessary vectorised.

**For lolz, from LZNA:** the synthesised mixin (§5) removes a table load per
symbol from an inner loop that already has plenty to do.

## 12. Summary

| | rz | lolz | LZNA | ours |
|---|---|---|---|---|
| coder | rANS | rANS | rANS | rANS |
| alphabet | 16…32 | 4…32 + binary | 16, 8 + binary | binary |
| state | 32 ×2 | ? ×2 | 64 ×2 | 32 ×**16** |
| renorm unit | 16 bits | ? | 32 bits | 8 bits |
| refill shape | one `if` | `while` | one `if` | one `if`, 2-byte select |
| renorm placement | before | after | after | **before** |
| decode division | no | no | no | no |
| encode division | (not in fragment) | **integer `/` and `%`** | (n/a) | **float32 reciprocal, biased** |
| encode buffering | whole block | whole block | (n/a) | whole block (already there) |
| binary step | — | branch per arm | branch per arm | **branchless select** |
| SIMD for | CDF search + update | CDF search + update + CM mix | CDF search + update | the coder, 16 lanes |
| mixin target | table | table | synthesised | (binary: FSM) |
| raw bits | in state | — | in state | — |
| model | nibble + varint | nibble + CM mixing | full LZ | order-0 bit tree |

The single sentence: **the other three vectorise the alphabet of one stream
and leave the coder scalar; we vectorise the coder across sixteen streams and
leave the alphabet at two** -- and the only thing that choice really forced was
the encoder's divide, because the search it removed from the decoder was
already one compare and the division it removed from the encoder was not
optional.
