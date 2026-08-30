# The decoder's renorm, and what forty builds of it say

An analysis of Shelwien's `log.txt` -- forty measurements of `rc_Renorm`'s
`f_DEC==1` arm -- plus the two shapes it ended at, what they measure here, and
the two coding-style rules he drew out of it.

Headline: **the big result reproduces on both machines and every small one
does not.** The outer branch is worth a fifth to a third of decode on both
boxes. Everything inside ±5% has now inverted between them at least once,
including the `RC_DEC_RENORM=1` default I set last round, which was wrong --
and wrong on this box too, once it was measured on a box that wasn't sharing
its cores with a runaway process. The default is back to 0.

## 1. The seven shipped shapes, on two machines

His box, clang 23, AVX2, 100 MB of enwik8. Mine, clang 18, `-march=native`
(AVX-512), same input, same `%defs%`, medians of best-of-2 over 5 round-robin
rounds with byte-identical twins of shapes 0 and 9 in the rotation -- they came
back 2.2% and 1.9% apart, which is the noise floor here.

| | shape | his (MB/s) | vs 0 | mine (s) | vs 0 |
|---|---|---|---|---|---|
| 8 | one branch, no locals | -- | -- | **1.925** | **+2.5%** |
| 0 | two nested branches | **48.21** | -- | 1.975 | -- |
| 6 | counted inner, addressed | 46.54 | −3.5% | 1.987 | −0.6% |
| 1 | counted inner, shifted | 45.86 | −4.9% | 2.033 | −2.9% |
| 5 | branchless 1 byte + guard | 43.04 | −10.7% | 2.088 | −5.4% |
| 9 | branchless, window cached | -- | -- | 2.485 | −20.5% |
| 4 | branchless, addressed | 37.74 | −21.7% | 2.861 | −31.0% |
| 3 | branchless, clz | 36.87 | −23.5% | 2.936 | −33.4% |
| 2 | branchless, two compares | 37.20 | −22.8% | 2.971 | −34.4% |
| 7 | branchless, no locals | -- | -- | 3.097 | −36.2% |

Two things to take from it.

**The outer branch is real everywhere.** Removing it costs 17-36% on both
machines. That is not a prediction effect -- it mispredicts on ~8% of bits and
that is worth a couple of percent at most. It is that `code <<= sh` and
`range <<= sh` land on the two loop-carried chains on *every* bit instead of
one bit in thirteen.

**Nothing small transfers.** Shapes 1 and 6 lose on his box by 3.5-4.9% and
lose here too. Shape 8 wins here by 2.5% and loses there by 1.9%. And inside
the branchless family the ranking is exactly reversed: 7 is his best branchless
shape and my worst, 2 is my second-worst and his second-best. Same source, same
input, two compilers and two microarchitectures.

That is the third time this project has produced a knob whose sign depends on
the box. It is why the default is 0: not because 0 is fastest anywhere, but
because it is the only shape that is never worse than second.

## 2. Reading the exploration

Lines 12-41 of his log walk from shape 4 (37.74) to 47.28 in about thirty
builds. His noise floor looks like ~0.5% -- five re-measures of the same state
land on 37.94 / 37.76 / 37.79 / 37.77 / 37.79 -- so anything above ~1% is real.

### The rewrite phase: 37.2 to 37.9, no net movement

| change | | Δ |
|---|---|---|
| rewrite shape 4 one-operation-per-line | 37.94 | -- |
| `_q` local removed | 37.18 | −2.0% |
| `const n, sh, _c` | 37.88 | +1.9% |
| `+_c` → `\|_c` | 37.89 | 0 |
| `msh` as a ternary / as `(1u<<sh)-1` / inlined | 37.35-37.87 | 0 to −1.1% |
| `msh = (1u<<sh); range *= msh` | 36.72 | −2.8% |

Only two of these mean anything. `range *= (1<<sh)` instead of `range <<= sh`
puts a 3-cycle multiply on the range chain where a 1-cycle shift was: −2.8% is
about right for one extra cycle on a chain that runs once per bit. And the
`_q`/`const` pair is the first sighting of the style rule -- see §4.

### The two real wins: 37.9 to 40.1

| change | | Δ |
|---|---|---|
| mask selected on the BYTE count (`stn==0/1`) | 39.13 | **+3.1%** |
| `__builtin_clz` for the shift count | 39.67 | **+1.4%** |
| `_lzcnt_u32` instead of the builtin | 39.59 | 0 |
| `stn = (range<sTOP)?(range<gTOP)?16:8:0` | **40.13** | **+1.2%** |

The nested ternary beating `__builtin_clz` is the interesting one. `clz` is
`lzcnt` (3-cycle) plus an `and`, both on the range chain. The ternary is two
`cmp` and two `cmov`: more instructions, but the compares are one cycle and
independent of each other, so the chain through it is shorter. `_lzcnt_u32`
against `__builtin_clz` measures identical, which is what you would expect --
with `-march=native` the builtin already lowers to `lzcnt`.

### The traps: three ways to lose 4-31%

| change | | Δ |
|---|---|---|
| `tmpptr -= (range<sTOP) + (range<gTOP)` | 38.85 | −2.1% |
| `code \|= win & ((1u<<stn)-1)` | 38.54 | −4% |
| `tmpptr -= ((stn==0)?0:(stn==8)?1:2)` | 37.24 | −7% |
| `tmpptr -= (range<sTOP)?(range<gTOP)?2:1:0` | **28.38** | **−29%** |
| "use rpre for mask before zeroing" | **27.38** | **−31%** |

The first four are one mistake at four sizes: **deriving the cursor decrement
from `range` again instead of from the shift count.** `stn` holds 0/8/16
because that is what both shifts want; the byte count is `stn>>3`, one
instruction, already on the chain. Every alternative starts a *second*
selection chain off `range` -- two more compares, or a second nested ternary --
and the second chain is as long as the first. Doing it with the full nested
ternary (−29%) is the same mistake as doing it with two compares (−2.1%), just
with three times the latency.

The last one, "use rpre for mask before zeroing", is a different failure:
routing the mask through the `rpre` slot before it is set to 0 makes `code`
depend on `rpre`, and forces `rpre`'s store to be ordered before the mask's
use. Two chains that were independent become one. −31% is what serialising
them looks like.

### The branch comes back: 40.0 to 47.3

| change | | Δ |
|---|---|---|
| `if_e0( range<sTOP ) { ... }` | **47.28** | **+18%** |
| `if_e1` instead of `if_e0` | 47.25 | 0 |
| hoist `code -= rpre; rpre = 0` above the branch | 46.46 | **−1.7%** |

+18% for one test, which is the same result as §1 from the other direction.

Two details worth keeping. The hint direction does not matter -- `if_e0` and
`if_e1` measure identical -- because the block is short enough that layout is
not the point; the branch is buying skipped *work*, not skipped instructions.
And **duplicating the rpre fold into both arms beats hoisting it**, which is
the opposite of what you would write by hand. Hoisted, `code` and `rpre` are
both live across the test in a body that is already short of registers.
Duplicated, each arm's copy is scheduled inside its own arm and neither value
crosses the branch.

## 3. What got integrated

Three new shapes, all producing the byte-identical reference stream, all in
`t_matrix.sh`:

- **`RC_DEC_RENORM=7`** -- his branchless shape verbatim (his `rc_renorm.txt`).
- **`RC_DEC_RENORM=8`** -- his one-branch shape (`rc_renorm_1br.txt`), rpre
  fold duplicated into both arms as he wrote it. **The best shape measured on
  this box**: 431 instructions in the 16-lane body against 0's 460, and 103
  stack references against 113.
- **`RC_DEC_RENORM=9`** -- the answer to "could we cache that value".

He borrowed the encoder's `stn[]` staging slot for the shift count; the
integrated version has its own `dsh` lane field, declared next to the staging
trio and for the same reason (the generated kernel must depend on the `-D` set
and nothing else). Shapes 4, 6, 7 and 8 all read above the cursor, so
`rc_io.inc` now asserts `RC_SKIP >= 4` for them.

### Caching the refill window

The complaint was that branchless is slower because of the memory read, and
that is exactly right, but not because the load is *expensive* -- because it is
on the chain:

```
range -> dsh -> tmpptr -> address -> load (5 cycles) -> code
```

every bit. Shape 9 keeps the four bytes ending at the cursor in lane state,
with the invariant `win == (uint&)tmpbase[tmpptr-3]` at every entry, seeded by
`rc_Init`. `code` takes its bytes out of the window the *previous* bit loaded,
and this bit's reload only has to land before the next bit reads it:

```c
dsh     = (range<sTOP) ? (range<gTOP) ? 16 : 8 : 0;
code   -= rpre;
rpre    = 0;
code  <<= dsh;
range <<= dsh;
code   |= (win>>16) >> (16-dsh);   // the cached word, not memory
tmpptr -= dsh>>3;
win     = (uint&)tmpbase[tmpptr-3];  // for the NEXT bit
```

Same number of loads, a whole iteration of slack on each. It is the trick
`RC_FOLD_RPRE` plays on the encoder, pointed the other way.

It works, and by a lot: **+24.7% over shape 7 here**, 3.097 → 2.485 s, and the
loop goes from 735 instructions with 204 stack references to 478 with **80 --
the fewest of any of the ten shapes**, branchy ones included. A branchless
decoder that spills less than the branchy one is not what I expected.

It still does not beat the branch here: 2.485 against 1.975. But look at where
the two boxes start from. His shape 7 runs at 83% of his shape 0; mine runs at
64% of mine. **The same +25% on his box lands at roughly 50 MB/s against shape
0's 48.21** -- branchless would win outright, which is the thing actually being
asked for. That is the one measurement worth making next:

```
set defs=%defs% -DRC_DEC_RENORM=9
```

If it lands where the arithmetic says, the branch can go.

## 4. The two style rules

> one operation per line with `[rcidx]` and no locals -- plain local vars may
> be left out as scalars depending on clang's mood. Also, const locals are
> better than plain locals.

Both are about the 16x unrolled decode loop specifically, and the mechanism is
register allocation.

A **local** is an SSA value with a live range. Sixteen unrolled copies of a
body with three locals is forty-eight simultaneous live ranges competing for
fifteen general-purpose registers, and clang resolves that by spilling -- which
is exactly what the `rsp` column in §1 counts. A **lane-array slot** is memory
with a known address that nothing aliases, so clang can re-read it instead of
keeping it live, and can rematerialise rather than spill. Writing the body as
one operation per line on lane state hands the allocator a body where every
intermediate has a home, rather than one where it has to invent sixteen.

The evidence in the log is not one-sided -- removing the `_q` local *hurt* by
2.0%, and adding `const` to the remaining three won 1.9% back. So the rule is
not "locals are bad": it is that the allocation is fragile at this unroll
factor and small source changes move it. `const` helps for the same reason the
array slots do -- a value that provably never changes can be rematerialised
instead of kept live.

The counts bear it out. His shape 8, written entirely on lane state, is the
smallest body of the ten (431 instructions, 103 stack references) and the only
one that beats the two-branch shape here. Shape 9, which adds one more lane
field, has the fewest stack references of anything measured.

**The rule does not carry to the encoder.** `RC_ENC_NSEL` puts the same two
changes on `rc_Renorm`'s `f_DEC==0` arm -- shift count from a nested ternary,
in a `const` local (1) or in lane state (2):

| | encode, mine |
|---|---|
| 0 sum of compares, byte count in a local | 93.7 MB/s |
| 1 nested ternary, shift count in a const local | 94.3 |
| 2 the same, in lane state | 94.0 |
| *twin of 0* | *92.4* |

The whole spread is inside the twin's. That is what should happen: the
encoder's sweep is vector code over the lane arrays, so a scalar local there
becomes one vector register, not sixteen scalar ones, and there is no pressure
for the rule to relieve. His own log says the same from the other end -- the
encode column sits between 80.0 and 81.8 MB/s across all forty rows, with no
structure in it. The knob is kept because it costs nothing and his box is not
this one.

## 5. One bug the exercise found

`RC_ENC_NSEL=1` was the first call in this codebase to pass an *expression* to
a generated macro, and it encoded a stream that never finished decoding.
`rc_soa.pl` turns `void ShiftLow(uint n)` into `#define ShiftLow(rcidx,n)` and
the body contains `n*8`, so `ShiftLow( sh>>3 )` expanded to `sh>>3*8` --
`sh>>24`, always 0. `rc_macro.pl` had the machinery for a safe parameter bind
(`$par`/`$paq`) and never wired it up; it now parenthesises every use of a
parameter in the body, and `mk_kernel.sh` fails the build if it finds a kernel
generated by a version that does not.

Second-order damage worth recording: the runaway decode wrote a 22 GB `t.dec`
and filled the container's disk twice, the second time while a background
benchmark was running -- which is why an earlier round of these measurements
read shape 1 as +2.2% here. `t.sh` now caps the write with `ulimit -f` at four
times the input, so a broken build fails the test instead of the machine.
