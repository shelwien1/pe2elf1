# Where the decoder stops being vector, and what it would take

Working notes on making the decode loop vectorise, rather than half-vectorise.
Everything here is measured on this box: clang 18, `-march=native` (AVX-512),
100 MB of enwik8, RCNUM=16 unless said otherwise.

## 1. The boundary is the multiply, and the reason is the model

`vpextrd`/`vpinsrd` in the group loop means clang gave up somewhere. It is not
the renorm and it is not `rc_Process`'s arithmetic -- that is a shift, a
multiply, a compare and two selects, all of it SIMD-able, with no division in
the decode path. Disassembling shape 9's loop around the first transfer shows
exactly where the wall is:

```
vpsllvd      ymm4,ymm12,ymm9      ; range <<= dsh, EIGHT LANES AT ONCE
vmovd        ecx,xmm4             ; ... and now take lane 0 back out
shr          ecx,0xf              ;     range >> SCALElog
imul         ecx,eax              ;     * freq
vpextrd      eax,xmm4,0x1         ; lane 1
shr          eax,0xf
vpextrd      r9d,xmm4,0x2         ; lane 2
...
```

The renorm **is** vectorised. Then every lane is extracted to do
`rpre = (range>>SCALElog)*freq` one at a time, because `freq` arrives one at a
time. That is the whole boundary, and it is the model's fault, not the coder's.

Why the model cannot hand over sixteen freqs: in the wave loop the group's
sixteen coder lanes are `NB = RCNUM/8` bytes of eight bits, and the eight bits
of a byte are one root-to-leaf walk of the context tree -- `ctx = ctx*2 + bit`,
where `bit` is what the decoder just produced. Bit *j+1* cannot be predicted
until bit *j* is decoded. So the independent model work per group is NB chains,
not sixteen: **NB=2 at RCNUM=16**, against a vector that wants 8 or 16.

## 2. Raising RCNUM does not fix it

If NB is the limit, more lanes should give clang more independent model work.
It does the opposite -- it gives up entirely:

| | insns/lane | transfers | transfers/lane | mask ops |
|---|---|---|---|---|
| RCNUM=16, shape 13 | 58.8 | 118 | 7.38 | 124 |
| RCNUM=32, shape 13 | 55.5 | 17 | 0.53 | 2 |
| RCNUM=64, shape 13 | 51.9 | 17 | 0.27 | 2 |

At 32 and above the transfers and the mask-register traffic simply vanish,
because clang stops vectorising at all and emits the straight scalar loop. It
is not vectorising *more*; the body got too big for it.

Decode, each RCNUM against its own stream (the format differs, so streams are
not interchangeable -- cross-decoding segfaults):

| | shape 0 | shape 13 |
|---|---|---|
| RCNUM=16 | 2.074 s | 2.411 |
| **RCNUM=32** | **1.903** | 2.366 |
| RCNUM=64 | 2.235 | 2.843 |

RCNUM=32 is the fastest decode measured in this session, ~8% ahead of 16, and
it is fully scalar. Worth a twin-controlled re-measure before believing the
size of it, but the ordering is clear enough.

## 3. The three-pass shape, and what the division costs

Shelwien's proposal, which removes the boundary rather than working around it:

> decoder version with `code/(range>>SCALElog)` division... we'd have to
> prepare a freq array in scalars, because there's a serial dependency. But
> once freqs are known, rc update and renorm can be done in vectors.

The point of the division is that **the quotient does not depend on freq**.
Today the decision is `code >= (range>>SCALElog)*freq`, which needs the model
before it can touch the coder state. Written as `q = code/(range>>SCALElog)`
and `bit = (q >= freq)`, the expensive part depends only on `code` and `range`
-- both of which are fixed at group entry, since each lane decodes exactly one
bit per group. So:

1. **vector**: `q[j] = code[j] / (range[j]>>SCALElog)` for all sixteen lanes
2. **scalar, serial**: walk the NB context chains -- load freq, `bit = q>=freq`,
   `ctx = ctx*2+bit` -- writing `freq[]` and `bit[]`. No coder state, no
   multiply on the chain.
3. **vector**: `rpre = (range>>SCALElog)*freq`, the range/code update, and the
   renorm, all sixteen lanes at once

Pass 2 is the irreducible serial part and it gets *shorter*: the multiply comes
off the chain, leaving load -> compare -> shift-add.

### Is the division exact?

Yes, in doubles. At `rc_Process` entry `rc_Renorm` has already run, so
`range >= 1<<24` and `d = range>>15` is in `[1<<9, 1<<17)`, `code < range`, and
`q < 1<<15`. Both operands are exact in a double, the quotient is within one
ulp, so one downward correction settles it:

```c
q = (uint)_mm512_cvttpd_epu32( _mm512_div_pd( (double)code, (double)d ) );
q -= (q*d > code);            // q*d <= code always fits in 32 bits
```

**0 mismatches in 6.4M random cases**, plus exact multiples and one-below
boundaries.

float32 is tempting -- sixteen lanes in one register, and `code`'s rounding
error of at most 128 over a divisor of at least 512 puts the quotient error
under 0.25 -- but it needs a two-sided correction, and the upward one
(`(q+1)*d <= code`) **overflows uint32** when q is near its maximum: 13
mismatches with code near 2^32. It would need a 64-bit compare, and it is not
faster anyway.

### What it costs

| | cycles / 16 lanes |
|---|---|
| `vdivpd` path, exact | **62.0** |
| `vdivps` path (unsafe as written) | 65.5 |
| current scalar `d*freq` + compare, in isolation | 4.6 |

62 cycles is the number to beat, against a whole group that runs about 176
cycles today at shape 0. float32 buys nothing -- the conversions and
corrections dominate, not the divide.

So it is not obviously a win, and it is not obviously a loss either. The
divides are *independent throughput* work while the model chain is *latency*:
if the eight-deep chain of dependent counter loads really is ~56 cycles per
byte, 62 cycles of divide can hide underneath it rather than adding to it. That
is the question a prototype answers and arithmetic does not.

## 4. Built: RC_DEC_SPLIT, and why it loses anyway

The four-pass loop is implemented and decodes byte-identically. `rc_DecPre`
renormalises, `RC_DIVALL` produces the quotients, the model walks, `rc_DecUpd`
updates the coder; the lane-to-byte mapping and the stream are `RC_DEC_WAVE=2`'s
exactly, so this changes the order the work is issued in and nothing else.

**It does what it was supposed to do.** In `Model<1>::do_process`:

| | insns | vector ops | zmm | ymm | vdivpd | transfers |
|---|---|---|---|---|---|---|
| baseline, shape 0 | 1502 | 303 | 0 | 144 | 0 | 58 |
| split, shape 9 | **979** | **398** | 40 | 203 | 2 | 48 |

A third smaller, a third more vector work, two `vdivpd` covering all sixteen
lanes, and fewer transfers. The vectorisation goal is met.

**And it is 33-77% slower.**

| RCNUM | baseline | split | gap |
|---|---|---|---|
| 16 | 2.085 s | 3.683 | +77% |
| **32** | **1.897** | 2.930 | +54% |
| 64 | 2.241 | 2.986 | +33% |
| 128 | 2.817 | 3.986 | +41% |

The reason is structural and I had it wrong in section 3. I argued the divides
were independent throughput work that could hide under the model's latency
chain. They cannot: **the model consumes the quotients.** The four passes are a
hard dependency chain — renorm, then divide, then the eight-deep context walk,
then the update — and a barrier between each.

The interleaved loop has the same per-lane serial chains, but nothing stops the
scheduler overlapping lane 5's renorm with lane 3's counter load, and it has
sixteen lane chains plus NB model chains to draw from. Splitting into passes
throws that away to gain vector width. On this box the overlap was worth more
than the width.

The one thing that does amortise is the barrier itself — the gap narrows from
+77% to +33% as RCNUM goes 16 to 64, then reverses at 128 when both variants
start losing to cache pressure. It never crosses.

### What the split does confirm

Inside it, **branchless renorm beats branchy**, which is the opposite of every
measurement in `dec_renorm.md`:

| split, RCNUM=16 | decode |
|---|---|
| shape 9, cached window, branchless | **3.683 s** |
| shape 13, clz | 3.873 |
| shape 0, two branches | 4.174 |
| shape 8, one branch | 4.311 |

+12% for shape 9 over shape 0. So the sub-hypothesis holds — once the renorm
is a clean whole-group pass with no model interleaved, the branch stops paying
and branchless wins. It just does not rescue the structure around it.

### Hints from sserangecoding

Two things in richgel999/sserangecoding are worth recording, and one of them
does not transfer:

- It divides in **float32** (`_mm_div_ps` on `_mm_cvtepi32_ps`) with no
  correction step. That is exact for it because its value is seeded by
  `read_be24` and stays inside float32's 24-bit mantissa. Ours is
  `RC_CODBYTES=4`, a full 32 bits, so float32 needs a two-sided correction and
  the upward one overflows 32 bits — measured, 13 mismatches near 2^32. Doubles
  are not optional here.
- Its refill is a **shuffle**, not a gather: `_mm_loadl_epi64` of eight bytes
  then `_mm_shuffle_epi8` with tables indexed by the lanes' renorm mask. That
  works because its four lanes share ONE stream and consume from it in order.
  Ours are RCNUM independent substreams in separate rows, so the addresses are
  unrelated and no shuffle can assemble them. Sharing a stream across lanes
  would be a format change, not a decoder change — but it is the thing that
  makes their refill free, and it is the direction to look if the format is
  ever on the table.

## 5. Where this leaves it

Nothing here beats the interleaved loop, and the reason is now specific rather
than vague: the decoder's cross-lane overlap is worth more than its vector
width, so any restructuring that adds a barrier to gain width starts behind.

The one result worth acting on is unrelated to all of it: **RCNUM=32 decodes at
1.897 s against 16's 2.085**, about 9%, on the ordinary interleaved loop. That
wants a twin-controlled re-measure and a look at what it costs the encoder,
but it is free if it holds.
