# The rANS flush: what is in it, and what can be taken out

rANS ends every substream by writing its whole state -- four bytes per lane
per block, 97,664 bytes of enwik8 at RCNUM=16 and BLKSIZE=64K, which is most
of the 75,436-byte ratio gap against the range coder this coder replaced. The
range coder's `rc_Quit` does much better than that: it moves `low` inside
`[low, low+range)` to a value ending in 0xFF bytes and then drops them,
because the decoder reads 0xFF past the end of a substream anyway. This file
is the measurement of whether rANS can do the same, and the short answer is
that a quarter of a byte of it is recoverable and the rest is information.

## 1. The distribution

Every flushed state, 24,416 of them, dumped from the encoder on enwik8 at
`RC_RANS_KLOG=8`, where the invariant is `x` in `[2^23, 2^31)`:

| bit length | count | share | bytes needed |
|---|---|---|---|
| 24 | 2975 | 12.185% | **3** |
| 25 | 3078 | 12.606% | 4 |
| 26 | 3122 | 12.787% | 4 |
| 27 | 3035 | 12.430% | 4 |
| 28 | 3087 | 12.643% | 4 |
| 29 | 3076 | 12.598% | 4 |
| 30 | 3046 | 12.475% | 4 |
| 31 | 2997 | 12.275% | 4 |

Eight octaves, each within a fifth of a percent of 12.5%. That is not a
property of enwik8, it is what rANS is: the state's stationary density is
proportional to `1/x`, so `log2(x/L)` is uniform on `[0,8)` and every octave
gets an eighth. **Leading zeros: mean 4.5019 bits, median 5**, so the state
carries 27.50 bits of the 32 written.

There is nothing else in it. Conditioned on the octave, the remaining bytes
measure 7.93 to 7.95 bits of 8 -- the shortfall is the sample-size bias of
estimating an 8-bit entropy from ~3000 points, not structure. The one real
deviation is octave 0's second byte at 6.946 bits, and that is arithmetic: in
`[2^23,2^24)` the top bit of that byte is always set, so it has 7 bits, not 8.

So the whole of the recoverable redundancy is the leading zeros, and the
question is only how finely they can be cut.

| | per flush | enwik8 | |
|---|---|---|---|
| whole leading BYTES | 0.1218 B | 2,975 B | 0.00475% |
| all leading BITS | 0.5627 B | 13,739 B | 0.02195% |

The bit-granular figure is not achievable as stated, because the decoder has
to be told how many bits to read. Self-delimiting costs 3 bits of octave
against 4.5 saved, so a bit-packed flush is worth ~2.5 bits per lane
(7,630 B, 0.0122%) and needs a bit-packer in the block format. The
byte-granular one is free, and that is what is implemented.

## 2. What is implemented: RC_RANS_FLUSH

Write three bytes when the state fits in 24 bits, four otherwise. The decoder
needs no flag, because the state's own invariant separates the two cases:

- a 3-byte flush read back is `x` in `[2^23, 2^24)`, which is `>= RANSL`;
- the top three bytes of a 4-byte flush are `x>>8` in `[2^16, 2^23)`, which
  is not.

So the decoder reads three, and reads a fourth only if three did not reach
`RANSL`. One dword load covers both -- the substream runs backwards, so the
top three bytes of the dword at `tmpptr-3` are the short flush and the whole
dword is the long one. On the encoder side the dword is staged whole either
way and only the cursor decides how much of it is payload.

Measured: 62,588,528 -> **62,585,553** bytes, exactly the 2,975 predicted, and
speed-neutral (one compare per lane per block). Verified against the scalar
reference coder and by roundtrip.

### It moves the best RC_RANS_KLOG

The state's range and the flush length are the same parameter seen twice, so
making the flush variable changes where `RC_RANS_KLOG` should sit:

| KLOG | fixed 4-byte flush | variable flush |
|---|---|---|
| 1 | 62,694,293 | 62,669,877 |
| 2 | 62,622,501 | 62,601,175 |
| 4 | 62,593,252 | **62,578,025** |
| 6 | 62,589,286 | 62,580,209 |
| 8 | **62,588,528** | 62,585,553 |

The optimum moves from 8 to 4, and 4 is another 7,528 bytes below 8 -- 10,503
bytes under the shipped default, 0.0168%. It is not taken, because it measures
about 5% slower to encode (76.4 against 80.9 MB/s, round-robin on 20 MB) and
0.017% of ratio is not worth 5% of encode here. The knob is there.

## 3. Why the range coder's trick has no analogue

`rc_Quit` works because **the range coder's decoder maps an interval to one
output**: any point in `[low, low+range)` decodes identically, so the encoder
may pick the point in it with the most trailing 0xFFs and drop them. That is
`log2(range)` bits of freedom, spent at the flush.

**rANS's decoder is injective.** It starts at `x_final` and every distinct
state decodes to a distinct symbol sequence, so there is no interval to round
within and nothing at the flush to choose. The leading zeros above are not
slack the encoder created; they are what `x_final` happened to be.

The freedom does exist, but at the other end: **the encoder's initial state is
unconstrained.** The decoder stops after `n` symbols and never checks where it
lands, so any `x0` in `[L, 256L)` codes the same block -- 8 bits of freedom,
exactly the 8 bits the flush spends. Confirmed directly: a build seeded at
`x0 = L+7` instead of `L` roundtrips unchanged.

It cannot be steered. `x0` sits at the *start* of the encoder's backward
sweep, and every renormalisation shifts the state right by 8, so a
perturbation smaller than 256 is gone at the first one and the renormalisation
fires once per output byte. Measured on 20 MB: moving `x0` from `L` to `L+200`
changes **7 of 4,896 flushed states, 0.14%**, leaves the compressed size
byte-for-byte identical, and alters only the never-read byte at the bottom of
each substream. To land `x_final` under `2^24` deliberately you would have to
search `x0` over values far enough apart to change the renormalisation
schedule, and each trial is a full re-encode of the block: about 8 tries for
one byte per lane, 24,416 bytes (0.039%), at several times the encode cost.

The honest summary: the range coder trims the flush because its decoder
tolerates a range of final values. rANS does not have that tolerance at the
flush at all, and the tolerance it does have -- at the initial state -- is
destroyed by its own renormalisation within about a dozen symbols.
