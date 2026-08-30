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

## 5. Pipelining the passes by halves

The barriers are the problem, so cover them. The two halves of a group are
completely independent -- lanes 0..RCNUM/2-1 are bytes 0..NB/2-1 and carry
their own context chains -- so one half's barrier can be filled with the other
half's work. `RC_DEC_SPLIT=2` issues the passes staggered:

```
renorm A | div A | renorm B | model A | div B | model B | upd A | upd B
```

renorm B sits over div A's latency, and div B sits over model A's serial walk,
which is the longest stall in the group.

It works, and only where the group is small:

| | RCNUM=16 | RCNUM=32 |
|---|---|---|
| baseline, shape 0 | **2.055 s** | **1.934** |
| split, shape 9 | 3.674 | 2.979 |
| split by halves, shape 9 | 3.414 (**+7.1%**) | 3.313 (−11%) |

At RCNUM=16 staggering buys 7.1%. At 32 it *costs* 11%, because a 32-lane group
already has enough independent work per pass to cover its own joins, and
halving it just doubles the number of joins. Same effect from two directions:
what the barrier needs is neighbouring independent work, and either more lanes
or a stagger will supply it, but not both.

The best split remains 2.979 s at RCNUM=32 against a 1.934 baseline.

### Deferring the window refill does not help either

Shape 14 is shape 9 with the reload lifted out of the lane body into its own
pass placed after the divide, where it has the model walk to hide under -- the
loads are for the *next* group, so they have a whole group of slack.

| | split | split by halves |
|---|---|---|
| shape 9, reload inline | **3.674 s** | **3.414** |
| shape 14, reload deferred | 3.992 (−8.7%) | 3.537 (−3.5%) |

It loses both ways, and for the reason shapes 11 and 12 already established:
lifting the reload out of the renorm turns sixteen register-resident window
values into an array round-trip, and that costs more than any placement gains.
The reload wants to stay where it is.

## 6. A 24-bit range, so the divide can be float32

Shelwien's: `CODBYTES` already exists, `range` can be initialised to
`(1<<(8*CODBYTES))-1`, and at `CODBYTES=3` code fits float32's 24-bit mantissa
-- so the division needs no correction at all.

Every part of that is right. `sTOP`/`gTOP` were hardcoded `1<<24`/`1<<16`; they
now derive as the top two byte boundaries of a CODBYTES-wide range, the clz
shapes (3 and 13) take a `CLZBIAS` so they still count from the top of the
range rather than the top of the word, and `RC_RANGE64` is refused with
`CODBYTES!=4`. `CODBYTES=3` decodes byte-identically across the shapes.

And the divide gets what it promised:

| | cycles / 16 lanes | exactness |
|---|---|---|
| double + one correction (32-bit code) | 59.1 | 0 miss in 6.4M |
| **bare float32 (24-bit code)** | **43.8** | **0 miss in 6.4M** |
| bare float32 on a 32-bit code | -- | 2021 miss in 3.2M |

One `vdivps` for sixteen lanes instead of two `vdivpd` for eight each, and the
`mullo`/compare/masked-sub correction gone. Confirmed in the binary: the split
build at `CODBYTES=3` contains exactly one divide instruction in the loop.

**It still does not pay, twice over.**

The range is narrower, so the coder is coarser -- the unit `range>>SCALElog`
falls from `[2^9, 2^17)` to `[2, 2^9)`:

| enwik8 | size | bpc |
|---|---|---|
| CODBYTES=4 | 62,513,092 | 5.00105 |
| CODBYTES=3 | 63,437,360 | 5.07499 |

**+1.479%**, 924 KB. That is more than any decode result in this file.

And the speed does not come:

| | CODBYTES=4 | CODBYTES=3 |
|---|---|---|
| plain loop | 2.247 s | 2.272 |
| split, shape 9 | 3.830 | 4.005 |

The plain decoder is unmoved -- it never divides. The split gets *slower*,
about 4.6%, of which ~1.5% is just the larger stream it has to read.

That last row is the useful one. Cutting the division's cost by a quarter
changed the split's time by nothing good, which is independent confirmation of
section 4: **the split is bound by its pass barriers, not by the divide.**
Making the expensive pass cheaper does not help when the problem is that the
passes cannot overlap.

The knob stays because a CODBYTES-wide range is a real generalisation and the
constants were hardcoded for no reason. It should not be turned on.

## 7. Speculating the context tree

The other direction: we do not know which `freq` the comparison needs, so
evaluate *all* of them. Fill a table with every node's `rpre*freq`, the matching
`code` for each node, compare, get 255 bits, and let the actual decode be a
walk through precomputed bits.

The dependency this attacks is real, and it is the right one. Today the per-bit
chain is

```
load cty[ctx] (5) -> p -> mul (3) -> cmp (1) -> bit -> ctx*2+bit (1) -> next ADDRESS
```

about 10 cycles, and the counter load is inside it because the next address
depends on this bit. Speculated, every address is known at the root — level *k*
of a byte's tree is the contiguous run `cty[2^k .. 2^(k+1)-1]` — so no load is
on the chain and what remains is `mul -> cmp -> select`, about 5.

It also works: the path to node *c* is exactly *c*'s binary representation, so
each node's `(code, range)` is a deterministic function of *c*. Nothing is
ambiguous, and reading the whole tree before updating any of it is safe,
because an update writes the node just left, never a descendant.

### The state is nearly redundant, but only nearly

Before costing it: is the per-node state even needed? If the leaf intervals
could come from products of the `p`'s down each root-to-leaf path, prefix-summed
to 256 thresholds, there would be no per-node state and no per-node renorm at
all — one cumulative-frequency lookup for the whole byte. `misc/tree_cumul.cpp`
compares that against the real walk over 20,000 random bytes:

| | |
|---|---|
| symbol mismatches | **0.40%** |
| bit mismatches | 0.09% |
| first divergence at bit 0-4 | **0.0%** |
| at bit 5 / 6 / 7 | 0.1% / 0.1% / 0.2% |

So the state is 99.6% redundant, and the residue is the per-level
`(range>>SCALElog)` truncation, whose error only accumulates enough to flip a
bit at the deep levels. Exact for a nibble in this sample, never exact for a
byte. A decoder needs all of it, so the shortcut is unusable — but it does mean
the per-node cost is smaller than a first reading suggests: child 0's range
**is** `rpre`, already computed for the comparison, and child 1 is two
subtracts. The only real cost per node is the two child renorms, which cannot
be deferred because `range` underflows within a nibble when `p` is extreme.

**It loses at every depth.** `misc/tree_spec.cpp` speculates D bits at a time —
D=1 is what the decoder does now, D=4 the nibble, D=8 the whole symbol:

| D | nodes/byte | loads on the chain | cycles/byte | /bit |
|---|---|---|---|---|
| **1** | 8 | 8 | **117.8** | 14.7 |
| 2 | 12 | 4 | 130.3 | 16.3 |
| 4 | 30 | 2 | 445.7 | 55.7 |
| 8 | 255 | 1 | 3137.4 | 392.2 |

A hand-vectorised D=8 (AVX-512, level by level, contiguous counter loads) gets
that last row from 3137 to 684 cycles — vectorising is worth 4.6× — and it is
still **6.6× slower** than the serial walk's 104. And that comparison is an
upper bound in speculation's favour: neither side pays the stream refill, which
the serial walk owes once per bit and speculation would owe 255 ways, each from
a different substream position.

The curve never turns. Even D=2, at only 1.5× the work, is already 10% down.

The reason is simple once measured: **the loads being speculated away are L1
hits.** The whole tree is 2 KB and permanently resident, so a counter load is
about 5 cycles — and one extra speculated node costs a multiply, a compare and
two child renorms, comfortably more than 5. Removing an L1 hit from the chain
is not worth the arithmetic it takes to remove it. Speculation pays when the
load you avoid is a miss; here there are none.

### Correcting the premise: this decoder is not chain-bound

Everything above is argued from "the counter load sits in a ~10 cycle per-bit
chain, so getting it out is worth having". **That premise is false here, and
`rc_config.inc` had already measured it.** From `RC_EAGER_CTY`'s own note:

> at 8.77 cyc/bit over ~25 instructions it is running ~2.9 IPC out of a
> 1756-byte loop body, i.e. it is now FRONT-END bound, not chain-bound. The
> lever moved.

`RC_EAGER_CTY` *is* depth-1 speculation of exactly this kind, and the cheapest
possible form of it — it speculates only the **load**, never the state, since
`cty[2c]` and `cty[2c+1]` are adjacent and arrive in one 8-byte load whose
address is known a bit early. Two counters for one load, no extra arithmetic.
And it is **off by default, because it loses.**

`misc/tree_spec.cpp`'s companion reproduces that ordering — speculating only
the loads, state kept serial:

| | cycles/byte | /bit |
|---|---|---|
| **load per bit, on the chain** | **60.6** | **7.57** |
| both children, one load (EAGER_CTY) | 82.8 | 10.35 |
| four grandchildren, two levels early | 93.0 | 11.62 |

Deeper is monotonically worse, and even depth 1 loses.

That is the unifying result for this whole file. A front-end-bound loop has no
latency to buy, and every scheme in these seven sections pays instructions to
buy latency: the gather, the group branch, the four-pass split, the staggered
halves, the cheaper divide, and now speculation — which is the most extreme
version, spending 32× the instruction count on a loop whose binding constraint
is instruction bandwidth. It is not a coincidence that it loses hardest.

The right question for this decoder is not "what is on the chain" but "what is
in the loop body", which is what `RC_DEC_UNROLL`'s note says is worth
re-asking, and what `RC_DEC_RENORM=8` — the smallest body of the ten shapes,
431 instructions — is quietly the best answer to.

## 8. Guessing the bit from p

`RC_GUESS_STATS=1` counts how often the MAP guess `bit = (p<hSCALE)` would have
been right. enwik8, 800M bits:

| | | | |
|---|---|---|---|
| bit 0 of byte | **99.606%** | bit 4 | 67.658% |
| bit 1 | 76.975% | bit 5 | 61.917% |
| bit 2 | 90.106% | bit 6 | 80.134% |
| bit 3 | 68.706% | bit 7 | 73.455% |
| | | **OVERALL** | **77.319%** |

Bit 0 is the byte's top bit and this is ASCII, so it is nearly free; the other
seven average about 74%. Only **32.5%** of bits have p outside [2768, 30000] --
two thirds are not confidently predictable in either direction.

**Speculating on it cannot pay, for three reasons that compound.**

**Verification costs exactly what the work costs.** To find out whether the
guess was right you must compute `rpre = (range>>SCALElog)*p` and
`code >= rpre` -- which is the entire per-bit job. Speculation cannot skip it.
All it can do is let the *next* step start earlier, so this is purely a latency
play.

**There is no latency here to buy.** Section 7: the loop runs 2.9 IPC out of a
1756-byte body and is front-end bound. And `RC_EAGER_CTY` is the *100% correct*
version of this same speculation -- it covers both branches with one 8-byte
load and needs no fixup path at all -- and it loses. A 77%-accurate guess that
also needs a verify and a rollback is strictly worse than a both-ways load that
is never wrong: more instructions, in a loop limited by instruction bandwidth.

**And 22.7% is a dreadful branch.** Written as a real branch that is ~15-20
cycles per miss, or 3.4-4.5 cycles per bit against a current 8.77 -- a 40-50%
penalty on its own. This is exactly why the coder is branchless
(`range = _b ? range : rpre`) to begin with; a guess reintroduces the branch
the shape was built to avoid, at a worse rate than the renorm branch it already
declines to take.

The 32.5% confidence figure closes the selective version too: restricting
speculation to extreme p covers a third of bits at best, and those are the
cheap, well-predicted ones anyway.

Bit 0's 99.6% is a genuinely different regime, but it is one bit in eight and
verification still costs full price, so using it would take a byte-level fast
path -- "is this ASCII" -- not RC speculation.

## 9. The generated if-tree — the one that works

Shelwien's: generate a binary tree of real `if` branches, 255 nodes and 256
leaves, and let the CPU's own predictor track each node's bias. On compressible
data those branches are not 50/50, so the hardware speculates past them and
runs the next node before this one's `code >= rpre` resolves.

This is not the software guess of section 8, and the differences are the whole
point:

- **Hardware speculation is free when correct.** No verify, no rollback, no
  extra instructions on the taken path — the thing that sank section 8.
- **Every node gets its own predictor entry.** A single shared branch site
  would see the average bias; 255 sites each learn their own.
- **`cty[c]` becomes a compile-time constant address.** The dependent load
  address — the thing that put the load on the chain in section 7 — is gone by
  construction.

`misc/iftree_gen.py` emits the tree; `misc/iftree_bench.cpp` is a complete
adaptive binary rangecoder (encode, then decode both ways, output checked
against the source) so the branch outcomes and their per-site biases are the
genuine ones from real data. Medians of 9 alternating rounds:

| data | lanes | A indexed walk | B if-tree | |
|---|---|---|---|---|
| enwik8 | 1 | 117.6 cyc/byte | **64.7** | **1.82× faster** |
| enwik8 | 16 | 121.9 | **74.8** | **1.63× faster** |
| random | 1 | 150.1 | 130.0 | 1.15× |
| random | 16 | 156.5 | 135.1 | 1.16× |

**The incompressible row is the control.** With branches at 50/50 the predictor
is useless and the win collapses from 1.63× to 1.16×. That gap is the
prediction; the 1.16 residual is the constant addresses. The mechanism is
exactly the one proposed.

**It survives lane interleaving**, which was the obvious way for it to die: 16
lanes share the 255 branch sites, so each site sees 16 interleaved outcome
streams. It costs only 1.82× → 1.63×, because the lanes *share the model* — every
lane at node *c* has the same P(bit|c), so the interleaving does not blur the
bias the predictor is learning.

And it is the first idea in this file that **reduces** the instruction count on
the executed path instead of adding to it: 8 of the 255 nodes run per byte, each
a load / multiply / compare / predicted branch, against the branchless walk's
select-and-index sequence. Section 7 established this decoder is front-end
bound, and this is the only proposal so far that pushes on that constraint in
the right direction — which is presumably why it is also the only one that wins.

### What this does not yet establish

- The model here is a plain adaptive counter, not `counter.inc`/FSM0 with
  `RC_FUSE_PP`. The absolute cycle counts will not transfer.
- **The lane arrangement is different, and this is the real obstacle.** Here
  lane *L* codes whole bytes, so `code` and `range` stay in registers across all
  eight levels of the walk. In the codebase, lane `m*8+j` codes bit *j* of byte
  *m*, so a byte's eight bits live in eight different coder lanes and each level
  of the tree would load a different lane's state. Adopting this needs the
  lane-to-byte assignment changed, which is a stream-format change.
- That reassignment may be worth having anyway: it would give RCNUM independent
  model chains instead of section 1's `NB = RCNUM/8` — the exact limit that
  capped everything else in this file.
- 53 KB of generated code per tree, against a decode loop that is currently
  1756 bytes. It wins here despite that, but the real decoder's front end is
  already the binding constraint and a second tree (encoder side) would double
  it.

The next step is not another probe: it is the lane reassignment, measured on
the real model.

## 10. Where this leaves it

Nothing here beats the interleaved loop, and the reason is now specific rather
than vague: the decoder's cross-lane overlap is worth more than its vector
width, so any restructuring that adds a barrier to gain width starts behind.

The one result worth acting on is unrelated to all of it: **RCNUM=32 decodes at
1.897 s against 16's 2.085**, about 9%, on the ordinary interleaved loop. That
wants a twin-controlled re-measure and a look at what it costs the encoder,
but it is free if it holds.
