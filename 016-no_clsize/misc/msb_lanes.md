# The zero runs in the output are lanes 0 and 8

The compressed file has visible runs of 0x00 -- 3371 runs of eight or more,
46,878 bytes on enwik8.  They are not the header, and they are not a coder bug.
They are the two lanes that code the MSB of a text file, and they are the
visible signature of a model that has no context for it.

## They are not the header

The header stream is easy to isolate: gate out `rc_Write`'s payload copy and
encode again, and the output is nothing but block headers.

```
  header-only stream        47,352 bytes over 1526 blocks   31.03 a block
  order-0 entropy            7.9961 bits/byte
  zero bytes                    190  (0.4%; uniform is 0.39%)
  runs of >= 8 zeros              0
```

Not one run.  The header coder's output is as flat as it can be.

## They are lanes 0 and 8

Mapping every run back through the per-block lane lengths puts essentially all
of them in two lanes:

```
  lane   runs    bytes   lane payload   in runs
     0   1681    23439        191,628    12.23%
     8   1687    23407        191,436    12.23%
   1-7,9-15   3      32     58,000,000     0.00%
```

Which two lanes those are is not a coincidence.  A bit's lane is `i % RCNUM`
and its position in its byte is `i % 8`; at RCNUM=16 both lanes 0 and 8 have
`i % 8 == 0`, so both code the **MSB** -- lane 0 of the even-numbered bytes,
lane 8 of the odd.  enwik8 is text, so that bit is 0 in 99.33% of bytes, and
those two lanes are where the whole file's MSB stream goes.

Every other lane is exactly what a rangecoder should emit:

```
  lane    written    model ideal   over    H0 (bits/byte)   zeros    xz
     0    193,154        190,922  +1.2%        7.050        20.96%   84.9%
     1  4,999,816      4,999,508  +0.006%      8.000         0.39%  100.0%
     2  2,515,117      2,514,747  +0.007%      8.000         0.39%      -
```

Lane 1 is incompressible, to three decimal places and to xz.  Lane 0 is 7.05
bits a byte and xz takes 15% off it.

## Why: the MSB is coded with no history at all

The coder is doing its job -- lane 0 is within 1.2% of what the model asks for,
and that 1.2% is the flush.  The model is the problem, and the zeros are what
model mismatch looks like on the wire.

`model_pass` resets the bit context at every byte (`lctx = 1`), so the MSB is
always coded in the root counter `cty[1]`: one counter, no history.  But high
bytes do not arrive independently -- they arrive in UTF-8 runs -- so the true
probability swings between "essentially never" inside a long ASCII stretch and
"very likely" in the middle of a multi-byte sequence.  The counter tracks the
average of those, so through an ASCII stretch it keeps charging for a bit that
is not going to happen; `range` keeps shrinking, renorms keep firing, and since
a 0 bit adds nothing to `low` (`rpre &= -_b`) every one of those renorms shifts
out a 0x00.

What one bit of history is worth, over enwik8's 100,000,000 MSBs:

```
  H0(msb)                     0.05834 bits    729,206 bytes
  H(msb | prev msb)           0.01825 bits    228,118 bytes
  H(msb | prev 2)             0.01718 bits    214,743 bytes
  H(msb | prev 8)             0.01555 bits    194,359 bytes

  lanes 0+8 actually cost     0.03089 bits    386,116 bytes
  (the model's own ideal      0.03055 bits    381,860 bytes)
```

The adaptive counter already beats a static order-0 by a wide margin -- 386 KB
against 729 KB, because its state machine has some memory -- but **one bit of
real context is worth another 158 KB**, 0.25% of the output.  That is three
times what xz finds in the coder's output, because xz is working on the coded
stream and this is the source.

## What would fix it

A context for the MSB.  The cheapest form is to stop resetting the bit context
to a constant and seed it with something about the previous byte -- its MSB, or
its top bits -- which costs one more level of `stats[]` and touches every bit
position, not just the MSB.  That is a model change with its own size and speed
consequences on the other fifteen lanes, so it is written down here rather than
made.

Worth noting for whoever does it: the lane geometry means the MSB stream is
split across two lanes that never see each other, so a context built from "the
previous byte" is also a context that crosses a lane boundary.  Lane 0 codes
the even bytes and lane 8 the odd ones, and each would need the other's last
bit -- which the model pass has (it runs over the whole block before any
coding) but the lanes themselves do not.
