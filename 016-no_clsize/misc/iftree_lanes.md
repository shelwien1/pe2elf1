# The if-tree on the real lane arrangement

`misc/iftree_gen.py` / `iftree_bench.cpp` measured a generated 255-node if-tree
against the decode loop and found it 1.63-1.88x faster, and `dec_vectorize.md`
section 9 recorded the result as unusable: that bench had **one lane decode a
whole byte**, and this codebase puts a byte's eight bits in eight different
lanes.

That reading was wrong about the consequence.  The arrangement is not an
obstacle to the if-tree, it is what makes the if-tree fit: a byte's eight bits
come from eight *independent* coder instances whose `code` and `range` are all
known when the byte starts, so the only serial edge across a byte is the model
context -- which is precisely what the tree is for.

`iftree_lanes_gen.py` / `iftree_lanes.cpp` are the same experiment on this
codebase's geometry (`rc_Process(m*8+j, ...)`, so byte m's bit j is lane
m*8+j).  Three decoders over the same stream:

```
  A   the loop, one byte at a time        model1.inc's tail form
  W   the loop, NB bytes interleaved      model1.inc's wavefront -- what runs
  B   the generated if-tree               one byte at a time
```

16 MB of enwik8, best of 5, cycles per output byte:

```
                                enwik8            /dev/urandom
  A  loop, byte at a time       143.18              205.87
  W  loop, wavefront            133.80              186.44
  B  if-tree                    105.55              197.85
                              1.27x vs W          0.94x vs W
```

On text the tree is 27% faster than the wavefront.  On incompressible data --
where every branch is a coin flip and the predictor has nothing to learn -- the
advantage does not shrink, it **inverts**: the tree becomes 6% slower than the
loop it replaces, which is what 255 static branch sites cost when none of them
predict.  That is the control, and it says the mechanism is branch prediction
and speculation and not, say, the constant `cty[c]` addresses.

## It is stream-compatible, which the earlier note did not expect

The tree walks one byte at a time; the decoder today interleaves NB.  Those
code the same stream, and the reason is the same one that made the wavefront
safe in the first place: a counter at depth k is touched only at bit k, so
interleaving bytes at different depths never reorders any individual counter's
updates.  Byte-at-a-time preserves that order too, trivially.  The encoder's
`model_pass` is already byte-at-a-time.

So all three of A, W and B decode each other's output.  No format change --
which is what `dec_vectorize.md` section 9 thought this would need.

## What the probe is not

It uses a shift-updated counter rather than the FSM, `while(range<sTOP)`
rather than the counted 0/1/2-byte renorm, and flat per-lane buffers rather
than the substream rows the real decoder indexes through `tmpbase`/`tmpptr`.
All of that is work the real decoder does on both sides of the comparison, so
it dilutes the ratio: 1.27x here is an upper bound on what integration can
give, not a prediction.

## Integrated, it loses -- and the tree is not why

It was built into `model1.inc` behind a depth knob, measured, and taken back
out; the numbers are what is left of it.  The shape was: a generator emitting
the tree at build time like `mk_kernel.sh` does the coder, a walk that does
nothing but the compares -- each node's multiply has to sit inside the branch
structure, because the frequency is what the context selects -- leaving `rpre`
in a small array; the range/rpre update batched over the lane arrays
afterwards; and the renorm one pass at the end over all RCNUM lanes.  The knob
was the tree's depth, with the remaining bits below it running as a loop.
Every depth coded the same stream and round-tripped the default encoder, so
the arrangement is sound -- it is just slower.

```
  depth   binary    dec MB/s      (base: 51224 bytes, 46.2 MB/s)
      1    51704       29.40
      2    51848       30.25
      4    51864       33.71   <- best
      6    53080       27.36
      8    59928       23.13
```

0.73x at best.  The interesting row is **depth 1**: one branch site, which is
the plain loop with the batched update and nothing else, and it is already
0.64x.  So the restructuring costs 36% and the tree then wins 15% of it back
(29.40 -> 33.71) without ever reaching the loop it replaced.

The tree does what the probe said it does.  What the probe did not model is
the cost of taking the update out of `rc_Process`.  There, a lane's `code`,
`range` and `rpre` live in registers across a tight window and the counter
update is two loads and a store; here `pre[]` is written by the walk and read
back by the batch, `range[]` and `rpre[]` are written by the batch and read
again by the renorm, and all of it goes through stack arrays that the loop
version never materialises.  That is the 36%.

One implementation note worth keeping: renormalising at the END of a group
needs normalised state on ENTRY, and the block-length header above the group
loop runs through `rc_Process`, which renormalises at the start and leaves
`range` wherever the last bit put it.  One renorm before the loop squares that
up; without it the first group decodes garbage.

The batch does not vectorize, with or without `RC_UNROLL` on it -- clang
reports the same four vectorized loops in the model1 translation unit either
way, and the measurement does not move (33.30 against 33.71).  A read-modify-
write over a runtime-based slice with a select per element is not what the loop
vectorizer wants, and forcing the unroll only removes the loop it would have
taken.

What would have to change for this to pay is the renorm: it is a 99%
not-taken branch per lane with a masked 16-bit load (see `rc_Renorm` in the
generated kernel), so a common vectorized renorm has to do unconditionally,
for sixteen lanes, work that today happens about 1.2 times a group.  That is
the wrong direction, and it is why the batch form starts 36% down.
