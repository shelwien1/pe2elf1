# The decoder's renorm as a vector pass

The ask: make the decoder's `tmpptr` reads branchless and let the compiler
vectorise them -- a dummy zero dword next to the lane rows, kept hot, and
each lane's refill gathering from either its cursor or that dword, so no lane
is left out and nothing branches.

The short answer: **the pass vectorises, the refill must not.** A branchless
renorm pass over all sixteen lanes, auto-vectorised from a plain loop, is
worth about +7% decode on this box *when it has no load in it* -- and every
way of putting the load in the pass loses, because on this box a
`vpgatherdd` costs 25 cycles and the sixteen lanes' cursors are sixteen
lines.  What works is shape 9's cached window from `dec_renorm.md`, lifted
into the pass: the bytes a lane shifts in come from a per-lane dword the pass
holds in lane state, and only the ~1.2 lanes a group that shifted reload
theirs, in scalar code, into an array nothing on the scalar path reads.  The
zero slot turned out to be unnecessary at every stage: clang produced the
masked gather on its own, and the mask is what the pass computes anyway.

Where it landed is in §4; the mechanisms, all of which were surprises, are
§2 and §3.  `RC_DEC_VRENORM` in `rc_config.inc` is the knob, with
`RC_DEC_VRENORM_GATHER` and `RC_DEC_ZSLOT` for the shape originally asked
for, and `RC_DEC_VRENORM_PROBE` for taking it apart again.

## 1. The shape

The group form of the decode loop (`model1.inc`) steps the two bytes of a
group together, one bit position at a time.  Lane `m*8+j` is bit `j` of
byte `m`, and each lane is touched exactly once a group -- which is what makes
a pass possible: a lane's renorm can move from the head of its `rc_Process`
to *after* its bit, and a run of lanes can be renormalised together once
their bits are in.

- **The kernel** (`rc.inc`, under `RC_VECOUT && RC_DEC_VRENORM`):
  `rc_Process`'s decode arm stops calling `rc_Renorm` and applies `rpre` to
  `code` where it is produced; `rpre` is dead after that and the array is
  never read on the decode side.  `rc_Renorm`'s decode arm keeps the
  one-branch shape minus the fold, for the block-length bits and the tail,
  which `model1.inc` drives through `rc_ProcessR` (renorm, then process).
- **Slots.** The lane arrays are laid out bit-major under the knob --
  `RC_SLOT(m*8+j) = j*NB + m` -- so the lanes finished by step `j` are a
  contiguous run.  A pass of `W` lanes fires every `W/NB` steps over the run
  just finished: `W=16` is one pass at the group's end, `W=8` two passes,
  after steps 3 and 7.
- **The pass** is a loop the compiler vectorises (`#pragma clang loop
  vectorize_width(8) unroll(disable)` -- without the second half the full
  unroller scalarises it before the vectoriser ever sees it):

  ```c
  r   = range[k];
  sh  = ((r<sTOP) + (r<gTOP)) * 8;               // 0, 8, 16
  tmpptr[k] -= sh>>3;
  code[k]    = (code[k]<<sh) | ((vwin[k]>>16)>>(16-sh));
  range[k]   = r<<sh;
  vsh[k]     = sh;
  ```

  `vwin[k]` is the dword ending at the lane's cursor,
  `vwin == (uint&)tmpbase[tmpptr-3]` on entry to every pass.  The cursor's
  own byte is its top byte, so the top `sh` bits are what an `sh`-bit shift
  wants, and `(win>>16)>>(16-sh)` is 0 for a lane that shifted nothing.
  Twelve vector instructions a group for sixteen lanes, no memory but the
  lane arrays, and clang emits exactly that: two compares to `k`-masks,
  `vpmovm2d`, the subtracts, `vpsllvd`, `vpsrlvd`, `vpor`, six 32-byte
  stores.
- **The refill** is the lanes with `vsh != 0`, as a bit mask (one
  `vpcmpneqd` + `kmov` per eight lanes on AVX-512, `vpcmpeqd` + `vmovmskps`
  on AVX2), refilled two at a time unconditionally -- the lowest set bit,
  `tzcnt`, or the run's first slot when there is none, which then reloads a
  window that did not move -- and a loop for the third and beyond:

  ```c
  m = lanemask( vsh[k0..k0+W) != 0 );
  i = k0 + (tzcnt(m) & (W-1)); m &= m-1;  vwin[i] = *(uint*)(tmpbase + tmpptr[i] - 3);
  i = k0 + (tzcnt(m) & (W-1)); m &= m-1;  vwin[i] = ...;
  if( m ) do { ... } while( m );
  ```

  Six instructions a slot.  P(three or more lanes shift in a group) is about
  13%, so that loop's branch is the one branch left in the renorm, against
  sixteen 8%-taken ones before.

## 2. Why the load cannot be in the pass here

The first build did the obvious thing -- every lane loads the dword at its
new cursor, `(1<<sh)-1` keeps the bytes that came in -- and clang made it a
`vpgatherdd`.  Two things about that build:

- With the index as `uint`, clang produced 4-lane `vpgatherqd` (the offset
  is zero-extended to 64 bits and `vpgatherdd` sign-extends) *and* it
  produced the masked gather on its own: `vpxor` the destination, gather
  under the `range<sTOP` mask.  The backend folds `store(select(k, new,
  old))` into a masked store and the gather's dead lanes into its mask.
  **The zero slot was never needed**: the compiler had already arranged that
  an idle lane loads nothing.  `RC_DEC_ZSLOT=1` measures identical to 0
  (33.6 against 33.1 MB/s, single runs).  With the index as `int` the gather
  is the 8-lane `vpgatherdd` and, oddly, unmasked again.
- Either way it costs 59 cycles a group, on a 139-cycle group.  The probe
  that replaces the load with the cursor (`RC_DEC_VRENORM_PROBE=1`, garbage
  out, same work) runs at **+7% over the baseline**; the same pass with its
  gather runs at −27%.  A gather is the whole loss.

`misc/gather_bench.cpp` is the microbenchmark: one 8-lane `vpgatherdd` from
L1-hot data costs **25 ticks of throughput** here, 37 dependent, against 19
for eight dependent scalar loads.  That is the Downfall (GDS) microcode
mitigation, which serialises gathers on every Skylake-generation core; an
unmitigated Skylake-X does one every ~5 cycles.  The guest kernel reports
`gather_data_sampling: Not affected` and times exactly like a core that is.
`-mno-gather`, clang's flag for this situation, scalarises the sixteen loads
into `vpextrd`/load/`vpinsrd` -- 23 cycles of port 5 a group, and −12%.

So on this box the refill has to be scalar and it has to be for the lanes
that need it, and `speed_plan_next.md`'s "gathers stay out on both sides"
was right for a reason it did not know.  On a core where a gather is one
instruction, `RC_DEC_VRENORM_GATHER=1` is the shape to measure -- run the
microbenchmark first.

## 3. Why the refill cannot write `code[]`

The second build kept the pass gather-free (`code <<= sh`, the mask to an
array) and refilled the shifted lanes scalar, `code[i] |= load & mask[i]`.
It measured **exactly like the gather build** -- 2.92 s against 2.87 -- and
so did its 8-lane twin, whose first pass has four steps of slack before any
of its lanes is used again.  Latency exposure was not the story, and a probe
that dropped the load but kept the store was just as slow.

The store is the story.  `code[i]`'s address is known only after the mask,
which is after the whole pass, some forty cycles after the group's last
decision -- and the next steps' loads of `code[k]` are issued long before
that.  A load issued behind an older store whose address is unknown goes
through the memory disambiguation predictor; the ones that have aliased
(the run's first slot aliases in ~34% of groups: 26% no lane shifted, 8% it
was that lane) get predicted to alias and wait for *every* older store
address to resolve, which puts the refill's whole chain on the critical
path -- including the second pass's, in the 8-lane build, for loads that
never aliased it.  Fifty cycles a group either way.

Hence the window: the refill's store goes to `vwin[i]`, which nothing on the
scalar path reads.  The next pass reads it a group later, when the address
is old.  That build is the one in §4.

## 4. What it measures

Alternating rounds, best of 3 decode passes each, 100 MB of enwik8, clang
18, `-march=native` (Cascade Lake, AVX-512), with a byte-identical copy of
the baseline in the rotation as the noise control.

| build | median MB/s | vs base | rounds |
|---|---|---|---|
| base, `RC_DEC_VRENORM=0` | **45.58** | -- | 45.56 46.24 45.60 45.37 46.38 44.72 |
| base again (the noise control) | 45.22 | −0.8% | 44.93 42.89 45.44 44.99 46.20 46.34 |
| `PROBE=1`, gather shape: the pass, no load | 48.48 | **+6.4%** | 49.10 48.74 48.92 40.79 47.83 48.22 |
| `PROBE=3`: the window pass, no refill | 48.24 | **+5.8%** | 46.84 48.22 48.30 48.26 48.74 46.43 |
| `PROBE=4`: ... and the lane mask | 46.22 | +1.4% | 47.26 48.14 45.18 44.17 44.96 47.47 |
| `PROBE=6`: ... and two slots, no loop | 46.34 | +1.7% | 46.38 46.30 45.61 46.14 46.54 46.49 |
| `RC_DEC_VRENORM=16`, two slots and the loop | 43.14 | −5.4% | 42.98 42.97 43.23 43.05 43.52 44.46 |
| `RC_DEC_VRENORM=8`, two passes a group | 40.33 | −11.5% | 40.73 40.98 41.11 39.55 39.38 39.92 |
| `GATHER=1`, the shape asked for | 33.26 | −27.0% | 33.10 33.44 33.39 33.40 33.03 33.13 |

So: the pass is worth +6%, the mask and two unconditional slots give back
4% of it, and the loop for the third lane -- 13% taken, six instructions a
trip -- gives back 7% more, which is far more than a 13% mispredict rate
accounts for and is probably the layout sensitivity `speed_ideas.md` §0
warns about as much as the branch.  The second table is the slot count,
`RC_DEC_VRENORM_SLOTS`, which is the only lever left in the refill:

| build, all `RC_DEC_VRENORM=16` | median MB/s | vs base | rounds |
|---|---|---|---|
| base | **45.75** | -- | 45.70 44.79 45.80 45.25 45.88 46.29 |
| base again | 45.41 | −0.8% | 45.35 45.46 43.80 45.46 43.06 45.81 |
| `PROBE=6`: mask + two slots, no loop | 46.00 | +0.6% | 44.52 45.03 46.35 46.37 46.56 45.66 |
| `SLOTS=2`, loop for the rest (13% taken) | 42.95 | −6.1% | 43.03 43.17 43.28 42.53 42.86 42.68 |
| `SLOTS=3`, loop (3.3% taken) | 44.23 | −3.3% | 44.00 43.89 44.47 45.16 43.15 44.74 |
| `SLOTS=4`, loop (0.6% taken) | 43.61 | −4.7% | 44.52 42.10 44.90 42.07 42.70 44.79 |
| `SLOTS=2`, rare path reloads all 16, straight-line (13% taken) | 40.52 | −11.4% | 39.55 40.74 40.80 36.56 40.30 41.39 |

Two things fall out.  The mask and two slots -- 17 instructions, none of
them predicted -- cost the whole +6% of the pass by themselves: +0.6% is
the noise floor.  And the loop for the rare lanes costs 3-6% *whether or
not it runs*: at four slots it is taken in 0.6% of groups, which is a tenth
of a cycle of mispredicts, and the build is still 4.7% behind base and 5.3%
behind the same code with the loop deleted -- in that run.  The next run
put the same binary at −0.5%, which is the box's layout sensitivity doing
what `speed_ideas.md` §0 says it does.  A straight-line rare path that
reloads every window is worse at 13% taken (48 instructions a trip), and
`RC_DEC_VRENORM_TAIL=1` measures it at the higher slot counts below.

| build, all `RC_DEC_VRENORM=16` | median MB/s | vs base | rounds |
|---|---|---|---|
| base | **44.70** | -- | 45.17 42.38 45.43 45.54 43.55 44.23 |
| base again | 45.17 | +1.1% | 44.23 44.10 45.53 44.81 46.38 46.24 |
| `PROBE=6`: mask + two slots, no tail at all | 46.03 | +3.0% | 45.77 46.35 46.29 46.84 45.19 45.38 |
| `SLOTS=4`, loop tail | 44.47 | −0.5% | 43.28 43.61 44.38 44.64 44.83 44.55 |
| `SLOTS=4`, straight-line tail (`TAIL=1`) | 44.15 | −1.2% | 44.17 42.13 44.28 44.13 43.02 44.76 |
| `SLOTS=3`, straight-line tail | 43.29 | −3.2% | 44.53 41.56 43.30 43.28 43.14 43.64 |
| `SLOTS=2`, straight-line tail | 39.98 | −10.5% | 40.47 35.74 40.46 39.51 31.42 40.55 |

A noisier run (the baseline's own rounds span 7%), and it moves the
four-slot loop build from −4.7% to −0.5%: at four slots the tail's shape
does not matter and the build sits **within about a percent of the
baseline**, which is the noise floor.  Three slots is 3% behind either
way, two slots is 6-10% behind, and the straight-line tail never beats the
loop.  `RC_DEC_VRENORM_SLOTS=4` is the default under the knob; the knob's
own default stays 0, because on this box the whole thing is a wash: the
pass buys 6%, the mask and slots that keep the stream honest cost 6%.

## 4a. What would make it win

- **A core whose gathers are one instruction.**  Then the refill is back
  inside the pass (`RC_DEC_VRENORM_GATHER=1`), the mask, slots and tail
  vanish, and the +6% of the bare pass is what is left to lose a gather's
  throughput from.  `misc/gather_bench.cpp` says in a minute whether a box
  is that box; this one is not, and a patched Skylake-X is not either.
- **Fewer reloads.**  The refill is ~1.3 lanes a group because a 4-byte
  window is spent after one 2-byte shift.  An 8-byte window in lane state
  -- two dword arrays, or qword lanes -- with a consumed-bytes count would
  reload a lane every ~7 bytes instead of every shift: 0.18 reloads a
  group, P(more than one) 1.4%, one unconditional slot and a rare tail
  that is rare.  The pass grows by the 64-bit shifts; the refill shrinks
  by a slot and the mask stays.  Unmeasured.
- **The other box.**  Every branch/branchless call in `dec_renorm.md`
  inverted between this machine and the AVX2 one; the pass's shape on AVX2
  is `vpcmpgtd`/`vpblendvb` for the mask work here done in `k` registers.
  Try `-DRC_DEC_VRENORM=16` there before believing any of the above.

Single runs of the probe ladder, the same box, for the shape of the cost:

| build | MB/s | what it is |
|---|---|---|
| base | 45.5 | the one-branch refill, `RC_DEC_VRENORM=0` |
| `PROBE=1`, gather shape | 48.4 | the pass with no load at all |
| `PROBE=3` | 46.8 | the window pass, no refill |
| `PROBE=4` | 47.3 | ... and the lane mask |
| `PROBE=5` | 43.3 | ... and one slot |
| `PROBE=6` | 45.5 | ... and two slots |
| `RC_DEC_VRENORM=16` | 42.5 | the whole thing |
| `RC_DEC_VRENORM=8` | 37.9 | two passes a group |
| `GATHER=1`, index `uint` | 33.1 | the shape asked for |
| `GATHER=1`, `-mno-gather` | 40.9 | loads and `vpinsrd` |
| `VF=1` | 27.5 | the same pass, scalar, unrolled |
| `RC_DEC_VRENORM=4` | 18.0 | four passes a group, gather shape |

## 5. The matrix

Every row in `t_matrix.sh` for the knob reproduces the reference stream on
both targets (`-march=skylake`, AVX2, and `-march=native`, AVX-512): pass
widths 2, 4, 8 and 16, the scalar and 16-wide loop shapes, the gather
variant with and without the zero slot, and widths 16 and 4 at RCNUM=32.
gcc builds and roundtrips `RC_DEC_VRENORM=16` too; its pragma is clang's,
so it vectorises or not as it pleases.

## 6. Things learned about the tools

- `#pragma clang loop vectorize_width(N)` needs `unroll(disable)` beside it
  or the loop is fully unrolled first and never vectorised; SLP does not form
  gathers from the result.
- A gather's index must be a signed 32-bit value to become `vpgatherdd`; a
  `uint` offset into a `byte*` gives two 4-lane `vpgatherqd`.
- clang folds `store(select(k, new, old))` into an AVX-512 masked store and
  masks the gather feeding it.  Whether it does so depends on details
  (the index type changed it); the shape of the pass is worth reading in
  the assembly every time.
- The store-to-load forwarding pairs in a mixed scalar/vector loop are
  asymmetric: a scalar load from a 32-byte store forwards, a 32-byte load
  from eight scalar stores waits for them to commit.  Neither showed up as a
  cost here; the disambiguation stall in §3 is the one that did.
- There is no `perf` in this VM, and the CPU reports itself unaffected by a
  mitigation it plainly has.  A microbenchmark of the instruction in
  question is cheaper than a theory.
