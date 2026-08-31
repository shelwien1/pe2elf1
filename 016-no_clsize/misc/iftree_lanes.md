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
