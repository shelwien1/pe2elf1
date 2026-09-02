# Speed: what is left, and where it is

A review of the coder as it stands at the end of the header-model work, with
the ideas that survive everything already measured.  The older plans
(`speed_plan.md`, `speed_plan_next.md`, `dec_renorm.md`, `dec_vectorize.md`,
`avx2_profile.md`) each carry a "do not retry" list; nothing below repeats an
entry from those.  Decode first, because it is still the slow side.

## 0. The numbers to argue from

Default knobs, clang 18, `-march=native` (cascadelake here), enwik8:

```
                 MB/s      cyc/byte    cyc/bit
  encode         ~80        ~40
  decode         ~46        ~70          8.7      -> decode is 1.7x encode
```

Three facts from the earlier rounds fix what kind of idea can win:

- **Decode is front-end bound, not chain-bound.**  ~25 instructions a bit at
  8.7 cycles is 2.9 IPC on a 4-wide machine.  Every scheme that spent
  instructions to buy latency lost -- eager counter loads, group branches, the
  four-pass split, staggered halves, the float divide, tree speculation.  The
  blend fitted in `speed_plan_next.md` §0 prices a marginal instruction at
  ~0.24 cycles and a cycle of chain at ~0.28: **instructions are the
  currency**, and the per-bit path has already been squeezed to the shape with
  the fewest of them (renorm shape 8, 431 in the 16-lane body).
- **The decoder's cross-lane overlap is worth more than any vector width.**
  The wavefront runs two bytes' chains at once; every restructuring that put a
  barrier between lanes to gain width started behind (`dec_vectorize.md` §10),
  and the if-tree integration this round is one more data point -- see §4.
- **Encode is rename-bound on both its loops**, `model_pass` the majority, and
  every mechanical lever there has been pulled.  What is left is "change what
  the counter update does".

And one from this round that changes how anything here has to be measured:

- **This build is layout-sensitive to about ±5% on encode.**  Adding code that
  never executes moved encode 4%; `RC_HDR_CTXBITS`, which changes a table from
  4 entries to 2 MB with identical code, moved it 6%; the ordering did not
  survive a target change; `-falign-loops` did not tame it.  `flush_bits.md`
  has the four probes.  A few percent read off one build is not a result.

## 1. The one structural lever: stop coding the MSB with the rangecoder

`msb_lanes.md` found where the zero runs in the output come from, and the
finding is a speed item before it is a ratio item.

At RCNUM=16, lanes 0 and 8 both have `i%8==0`: between them they code the
**MSB of every byte** -- lane 0 the even bytes, lane 8 the odd.  On text that
bit is 0 in 99.33% of bytes.  So two of the sixteen lane steps in every
group, one eighth of every `rc_Process`, every counter load and update, every
renorm test, are spent on a bit that carries 0.6% of the output:

```
  lanes 0+8, enwik8:   386,116 bytes of 62,487,196   (0.62%)
  their share of decode work:                        12.5%
```

And the model is wrong for them anyway: coded in the root context with no
history, the MSB costs 0.031 bits, where one bit of history would make it
0.018 -- 158 KB.

**The idea:** take the MSB out of the rangecoder entirely.  Code it as a side
stream per block -- run lengths of low bytes between high ones, through the
header coder that now exists for exactly this kind of thing -- and have the
main coder code seven bits a byte.

What it is worth:

- *Decode:* one eighth of the per-group work gone.  The wavefront's body is
  16 lane steps; it becomes 14.  Instructions being the currency, that is on
  the order of **10% decode**, and it is the only item on this list that
  removes work from the per-bit path rather than rearranging it.
- *Encode:* the same eighth off `model_pass` and the sweep, so of the same
  order there.
- *Ratio:* enwik8 has 674,412 high bytes, 442 a block.  Run lengths modelled
  against the previous run (the geometry of UTF-8: a high byte is followed by
  another with high probability, and long ASCII runs are long) should land
  near the 0.018-bit conditional entropy: about 150 bytes a block against the
  253 the two lanes cost today.  On text it is a ratio *win* of ~100 KB on
  top of the speed.

What it costs, and the two things to decide before building it:

- **Seven bits a byte is 14 lane steps a group, and 14 is not a multiple of
  8.**  The whole group structure -- `RCNUM%8==0`, `NB=RCNUM/8`, the
  pair-stores, `RC_FF_LANES=8` dividing RCNUM -- assumes eight bits a byte.
  The cleanest shape keeps sixteen lanes and two bytes a group but leaves two
  lanes idle... which is no saving.  The real shape is 14 lanes, 2 bytes, and
  a group of 14; `rc_soa.pl` and the kernel are generic in RCNUM, the model
  loops are not.  Budget a day for the geometry, not an hour.
- **Binary input.**  On data whose MSB is not constant the side stream costs
  more than the lane did.  It needs a per-block choice -- "MSB in the side
  stream" against "MSB in the coder" -- signalled in the header, which is one
  more bit through `hdr_Write`.  The decision itself is cheap: `model_pass`
  already walks the block before anything is coded, so it can count high bytes
  as it goes.

Falsifier before the geometry work: build the 14-lane decoder with the MSB
simply *dropped* (decodes to garbage, but the loop is right) and measure.  If
that is not ~10% faster, the rest is not worth doing.

## 2. RCNUM=32

`dec_vectorize.md` §10 saw RCNUM=32 decode 9% faster on the interleaved loop
and flagged it as wanting a twin-controlled measurement and a look at the
encode cost.  The group becomes four bytes, so the wavefront runs four chains
instead of two, which is the mechanism if it holds.  Measured this round, four
paired rounds, best of 4 encode / 5 decode passes:

```
              enc MB/s                  dec MB/s                  enwik8
  RCNUM=16    66.25 67.50 68.02 65.87   37.69 36.58 36.41 36.70   62,487,196
  RCNUM=32    65.60 57.54 66.08 65.13   34.03 32.41 33.72 33.64   62,512,348
  median      66.9 -> 65.4              36.6 -> 33.7              +25,152
```

It does not hold.  Decode is **9-10% slower** at RCNUM=32 in every paired
round, encode is down too, and the stream is 25 KB bigger -- the 32-entry
length row and a header model with half the samples per lane.  (Absolute
numbers are lower than §0's because a second job shared the box; the pairing
is what the table is for.)  The 9% in `dec_vectorize.md` was one unpaired run
on a box that swings that much on its own.  Settled: RCNUM stays 16.

For the record, the costs it would have had to beat: `rc_config.inc` has
`RC_SCATTER` losing 3.2% at RCNUM=32 on the encoder (its default already
flips it off there), and the header row and `hdrctr`/`hdrlen` double.

## 3. Instruction-count items still open on the decode path

Small, and each needs the twin discipline to see at all.  Listed because the
front-end-bound conclusion says they are the *only* kind of item left inside
the loop.

- **The tail loop uses the member `ctx`.**  The group loop moved to local
  `lctx[]` (speed_plan_next §1.2 landed for it); the tail after `nb1` and the
  `RCNUM%8!=0` path still go through `this->ctx`, whose store may-alias
  `cty[ctx]` and keeps the context out of a register.  It runs for the last
  <16 bits of a block, so it is not measurable -- but it is the same fix and
  it is free.
- **`rc_Init` runs `RC_STAGE_INIT` before the `f_DEC` test** and the kernel
  prelude declares the encoder's eleven lane arrays for the decoder to carry.
  `speed_plan_next.md` §3 wanted an `f_DEC` guard in the generator's emission.
  Since then `RC_KALIGN` is empty on the decoder side and the alignment was
  measured (dropping it: 1.4% slower, `model1.inc`), so the frame-pointer
  argument is moot -- but the dead declarations still cost stack and the
  prologue still zeroes nothing it needs to.  Probably nil; cheap to make the
  generator honest.
- **`Counter2::Update` is a load and a store per bit**, 16 stores a group into
  `stats[]`.  The one experiment that took the updates off the per-bit path
  (the if-tree's replay, §4) lost for other reasons, so it is not known what
  the stores themselves cost.  A probe: leave `Update` in place but make the
  store conditional on the state actually changing (`if(s!=n) sp=n`) -- it
  changes on most bits, so probably nothing, but it is one build.

## 4. Settled this round: the if-tree, and what it taught about batching

`iftree_lanes.md` has the whole thing.  The short form for anyone tempted to
batch the decoder's update:

- On this lane arrangement -- eight independent lanes a byte -- the generated
  255-node if-tree is **1.27x** faster than the wavefront in a standalone
  probe, and the incompressible-data control (0.94x) says the mechanism is the
  branch predictor.  The old note that it "needs a format change" was wrong;
  it codes the same stream.
- Integrated into `model1.inc` the same shape is **0.73x at best**, and the
  reason is not the tree: a *one-node* tree -- the plain loop with the batched
  update and nothing else -- is already 0.64x.  Taking the update out of
  `rc_Process` pushes `pre[]`, `range[]` and `rpre[]` through stack arrays the
  loop version keeps in registers, and that is 36% on its own.
- A common renorm at the end of the group is the wrong direction: the renorm
  is a 99%-not-taken branch with a masked 16-bit load, firing ~1.2 times a
  group, and "do it for all sixteen lanes" adds work.  It does not vectorize
  either way.

So: anything that batches lane state across the group has to keep it out of
memory, and there is no evidence yet that anything does.  §2.1 of
`speed_plan_next.md` (batching the bookkeeping at the group edges *to remove
instructions*) is still the only framing under which it could pay.

## 4a. Settled after this: the renorm as a vector pass

`vrenorm.md` has it.  The decoder's renorm CAN be a branchless pass over all
sixteen lanes, auto-vectorised from a plain loop, and the pass on its own is
worth **+6% decode** -- measured with paired rounds, the first item on this
list to move the decoder in the right direction since the wavefront.  What
it cannot be is a gather: a `vpgatherdd` costs 25 ticks on this box
(`gather_bench.cpp`, the GDS microcode mitigation), and a refill of the
~1.2 lanes a group that shifted has to be scalar, off a lane mask, into a
window array nothing on the scalar path reads.  That refill is what the
knob is fighting: the mask and the unconditional slots cost back the 6%,
and `RC_DEC_VRENORM=16` with four slots lands within a percent of the
baseline -- a wash here, and the knob's default stays 0.  `vrenorm.md` §4a
has the three things that would change that: a core whose gathers are one
instruction, an 8-byte window that reloads a lane every ~7 bytes instead of
every shift, and the AVX2 box.

Two mechanisms from it belong on the "know before building" list: a scalar
store whose address is known late (a `tzcnt` off a mask) stalls every later
load of the same array in the disambiguation logic, whatever the slack; and
clang folds `store(select)` into masked stores and gathers on its own, so
the "zero slot to gather from" idea is something the compiler already does.

## 5. Outside the loop

Things that do not touch the hot path and are not the coder's problem in the
narrow sense, listed because they are where the large multipliers are.

- **Block-parallel decode is not available as the format stands.**  Blocks
  are independent in their coders -- every lane re-inits per block -- but the
  model is not: `stats[]` and the header counters carry across blocks, so
  block *n* cannot be decoded without block *n-1*'s counter updates.  Making
  it available means resetting the model per block, which costs ratio (a 64
  KB block warms a 512-context order-0 model quickly; the header model, with
  1526 samples a lane over the whole file, less so).  Measure the ratio cost
  of a per-block reset first; if it is small, N threads is the only Nx on
  this list.  (`RC_THREADS` was removed from the *encoder* as "not a
  rangecoder change"; this is the same call, made once more.)
- **`rc_Read` copies every block's payload into the lane rows.**  ~100 MB of
  memcpy over a 2.1 s decode -- under 1%.  Decoding in place from the input
  buffer would need the 0xFF pad below each substream to exist in the input,
  which it does not.  Not worth it.
- **Per-host tuning.**  `TUNE=` in `build.sh`; znver tuning costs 6-8% encode
  on the Intel box here and is reported to gain 10% on a Skylake-X.  It is a
  per-host measurement, never a default.
- **`-fno-pie`**: 0.85 cyc/grp on encode per `avx2_profile.md` §10, one flag.

## 6. Where the model is the speed problem

Two of the items above (§1, and the reason RCNUM=32 can pay) are about how
bits are distributed over lanes, not about the coder.  That is the general
shape of what is left: the rangecoder's per-bit path is at its instruction
floor for the model it is given, and the model is an order-0 bit tree that
hands the coder one bit per byte that is almost always the same and seven
that are not.  The next speed win is a model that gives the coder fewer bits
to code, and the MSB is the first eighth of that.

## 7. How to measure anything here

- Twin builds, alternating in one loop, medians over four or more rounds,
  best-of-N passes inside each.  Single runs here swing 5-13%.
- Before believing a few percent: build the change with its new code
  **present but gated on a volatile zero**, so it never executes.  If the gap
  stays, it is layout.  (`flush_bits.md`, "The encode measurement".)
- Byte-identical stream for any change that claims to be speed-only: `md5sum`
  the output, and run `t_matrix.sh`.
- Sizes and speeds in the same table, every time.  Half the ideas above trade
  one for the other.
