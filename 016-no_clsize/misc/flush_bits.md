# rc_Quit: filling the flush at bit granularity

What `rc_Quit` has to do is pick some value in `[low, low+range)` and write
enough of its top bytes that the decoder can tell which interval it was.  The
decoder reads `0xFF` past the end of a substream (`rc_Read` pads it -- see
`RC_FF_PADSIZE` in `rc_config.inc`), so any *trailing all-ones byte* of that
value can be left out.  The flush is therefore a search for the value in the
interval with the longest run of 1 bits at the bottom.

`rc.inc` used to walk that search a byte at a time:

```c
for( i=0; i<LOWBYTES; i++ ) {
  const qword mask = ((1ULL<<((i+1)*4))<<((i+1)*4))-1;   // 2^(8(i+1))-1
  if( hic || ((llow|mask)<high) ) llow|=mask, --n;
}
```

It now fills at bit granularity, and does the whole search with one bit scan.

## The closed form

For a mask `m = 2^k - 1`:

```
low | (2^k - 1) < high   <=>   (low >> k) < (high >> k)
```

(both sides say the top `64-k` bits of `low` and `high` differ: `low|(2^k-1)`
is the largest value sharing `low`'s top `64-k` bits, so it is below `high`
exactly when `high` has larger top bits).  `low < high`, so `low>>k <= high>>k`
always, and the two stop differing once `k` passes the top set bit of
`low^high`.  The longest fillable run is therefore

```c
k = 63 - __builtin_clzll( low ^ high );
```

-- no loop.  `range >= sTOP` holds at every flush, so that bit is at position
24 or above and `k >= 24`: a flush always drops at least 3 bytes.

The same identity says the fill is *optimal*: a value whose bottom `8m` bits
are all 1 and which is `>= low` is at least `low|(2^(8m)-1)`, so if that one
misses the interval no other candidate hits it.  Both the old loop and the new
scan find the same maximum, and the old loop's nested masks meant it was
already finding it -- which is why this is size-neutral.

## Size-neutral, by construction and in measurement

Only the byte-aligned part of the run shortens the flush, and that is
`floor(k/8)` either way.  Filling the leftover `k%8` bits cannot drop another
byte.  enwik8, default configuration:

```
                        n=3     n=4      n=5
  byte fill (old)         8    4174    20234
  bit  fill (new)         8    4174    20234
```

and the encoded file is 62,513,092 bytes both ways.

## What it does change: the first byte of every substream

`rc_Write` copies each lane's payload with an ascending `memcpy`, and the
coder writes its substream backwards (`*ptr--`, see `rc_io.inc`), so the byte
the flush writes *last* is the byte that lands *first* in the file.  That is
the byte the fill rewrites.

Because the search stops at `k`, bit `k` of `low` is necessarily 0 (if it were
1 the `k+1` fill would have succeeded too), so the last written byte has
exactly `k%8` trailing 1 bits -- no more, no less.  24416 flushes over enwik8:

```
  trailing-1 run of the first substream byte, %

        0      1      2      3      4      5      6      7      8
  old  53.85  23.41  11.30   5.95   2.88   1.49   0.74   0.38   0.00
  new  12.66  12.38  12.26  12.44  12.43  12.66  12.79  12.37   0.00
```

`k` is essentially uniform mod 8 (`k` itself spans 24..44, mean 28.9, and its
low bits are the range's), so the run length is uniform over 0..7.  It can
never reach 8: a byte of all ones would have been dropped instead of written.

That turns into a strong bias on the byte value:

```
  P(bit = 1) by position of the first substream byte, %

        bit0   bit1   bit2   bit3   bit4   bit5   bit6   bit7
  old  46.15  46.31  46.74  46.09  45.83  44.76  42.87  40.96
  new  87.34  80.94  74.53  67.59  61.48  54.70  47.32  40.96
```

which is exactly `P(bit j = 1) = P(k%8 > j) + P(k%8 < j)*P(bit j = 1 | free)`
= `(7-j)/8 + (j/8)*0.46`.  Bit 7 is untouched: it is only ever forced by a
`k%8` of 8, which cannot happen.  Top values: `7F` 12.4%, `3F` 6.7%, `BF`
6.1%, `1F` 3.8% -- the old distribution's mode was `00` at 6.5%, everything
else under 0.6%.

Order-0 entropy of that byte drops from **7.786 to 6.450 bits**.

## Is the skew worth anything?

Not here.  There are 24416 flushes on enwik8 (1526 blocks x 16 lanes), so
1.336 bits x 24416 = **4077 bytes**, 0.0065% of the output -- and only if
something entropy-coded that byte, which nothing does.  For scale, the flushes
cost 117,890 bytes of payload (0.19%) and the substream length table another
48,832.

So the change is worth having for its own sake -- the search became one bit
scan instead of a loop, and the flush now writes the largest value the
interval permits rather than an arbitrary one -- but the skew is not a size
win the payload can collect.  Moving the *trim* below byte granularity would
mean giving up byte-aligned substreams.  What can collect it is the header.

## The head row

The block header now carries those bytes.  `rc_Write` lifts each substream's
first byte out of the payload and writes them all together, right after the
length row:

```
  per block:  RCNUM x 2-byte little-endian substream length
              one head byte per non-empty substream, in lane order
              RCNUM x substream payload minus its head byte, in lane order
```

Nothing codes them yet -- this is a pure reordering, and enwik8 is still
62,513,092 bytes -- but it puts the block's most predictable bytes in one
contiguous run where a header coder can reach them without walking payload.
A lane that coded nothing has no head byte and contributes none; the lengths
come first, so both sides agree on which lanes are in the row.

Round-robin twin builds over 6 rounds, best of 6 passes each: enc 67.4 -> 67.0
MB/s, dec 38.4 -> 38.4 MB/s, inside a within-build spread of 4%.  It is one
extra 16-byte pass per 41 KB block.

## The header stream

The header is now coded, not written raw: a scalar carry-propagating
rangecoder of its own (`Rangecoder_HDR`, rc.inc instantiated a second time
over `RC_HDR_IO`) takes the RCNUM 16-bit lengths and the 8-bit head bytes,
every bit at p=1/2 for now.  Bypass coding is the same size as the raw bytes;
the point is that a header model now has somewhere to go.

That coder's cursor differs from the payload's on two counts, and both come
back to this document's subject.

**Forwards, not backwards.**  A payload substream is written and read
backwards, which is only possible because its length is in the header: the
reader knows where its last byte is before it starts.  The header stream's
length is what it is on its way to describing, so its reader has to start at
the first byte and walk up.

**An aligned flush, not the 0xFF one.**  A reader consumes CODBYTES at
`rc_Init` plus one byte per renorm; a writer emits one byte per renorm plus the
flush, less the RC_SKIP zero prefix.  The renorm counts are the same sequence
on both sides, so

```
  bytes read = bytes written + LOWBYTES - (flush bytes)
```

The flush length settles the over-read, and the reader has to derive it -- it
cannot be told, because that is the length again.  Both of those come out of
`rc_Quit`, and neither works with the flush every payload substream uses.

### Why the 0xFF trim cannot come along

`rc_Quit` may drop a trailing byte **because the reader reads 0xFF past the end
of a substream** -- `rc_Read` pads below every payload row with them, so a
reader running off the end reads back exactly what was dropped.  The header
stream has no pad under it.  What follows is the payload, so a reader running
off the end folds *payload bytes* into `code` where the writer assumed 0xFF,
and whenever they are smaller the reconstructed value drops below `low` and the
last bits decoded flip.

It is a low-probability event, which is the dangerous kind.  enwik8 at RCNUM=16
round-tripped clean; the matrix caught it at RCNUM=32, and it was one bit: the
head byte of the last lane of block 9, 0x00 for 0x01, from a three-byte
over-read landing on payload.  The header decoder's own accounting was exact --
97 bytes written, 100 read, every block -- so the byte positions were right and
only the value was wrong.

### The aligned flush

The bytes this stream drops have to be bytes that *any* value may occupy.  So
instead of the largest value in the interval ending in 1 bits, flush an
**aligned block**: round `low` up to a multiple of 2^(8j), write the top
LOWBYTES-j bytes of that, and whatever the reader picks up for the bottom 8j
bits it lands in [A, A+2^(8j)) -- which is wholly inside the interval.

A 2^k-aligned block fits in [low, low+range) exactly when

```
  (low + kmask) | kmask  <  high        kmask = 2^k - 1
```

which is the fill test with `low` rounded **up** instead of down -- the whole
difference between the two flushes.  Equivalently `ceil(low/2^k) <
floor(high/2^k)`, against the fill's `floor(low/2^k) < floor(high/2^k)`.  The
similar-looking `low <= ((high-1) & ~kmask) && high > (low|kmask)` is not the
same test and over-accepts: it takes the top block's *base* being above `low`
and the bottom block's *top* being below `high`, which two different blocks can
satisfy while neither fits.  2625 false accepts in 900,000 random intervals.

### j comes from `range`, not from the interval

The maximal k for a given interval is not usable, because the reader cannot
compute it.  At the flush the writer holds `low` and the reader holds `code`;
the only value they share is `range`.  So j has to be a function of `range`
alone.

Any interval of length >= 2^(k+1) contains a 2^k-aligned block whatever `low`
is, and that bound is tight -- over random intervals the smallest achievable
kmax is exactly bsr(range)-1 for every bsr.  So the best such function is

```c
  j = (bsr(range) - 1) / 8;
```

one bit scan, and the reader mirrors `rc_Quit`'s opening renorm (`FinishDecode`)
and asks its own `range` the same question.  At CODBYTES=4, `range` after the
renorm is in [2^24, 2^32), so j is 3 whenever range >= 2^25 and 2 below it.
Measured over enwik8:

```
  j=3   1480 blocks   96.99%
  j=2     46 blocks    3.01%
  mean header stream   49.00 bytes   (48 raw)
```

The one thing to watch is that rounding `low` up can carry out of the
accumulator.  That is a carry out of low like any other and folds into
`Carry`, and it cannot collide with one already pending: a pending carry means
low has just wrapped, so it is under 2^32 and nowhere near the top.  It does
mean the aligned flush needs the carry-propagating `ShiftLow`, which a
`static_assert` on RC_CARRYLESS enforces.  The vector coder is untouched
either way -- the switch is in the scalar body, and `n -= k/8` is what the
generated kernel still carries.

### What it costs

```
  head row, raw (the previous format)         62,513,092
  header coded, 0xFF flush (wrong)            62,514,572   +1,480
  header coded, full LOWBYTES flush           62,519,150   +6,058
  header coded, aligned flush                 62,514,618   +1,526
```

+1 byte a block, which is the coder itself: 49 bytes of stream for 48 bytes of
fields.  The aligned flush gives back 4,532 of the full flush's 6,058, and the
46 bytes it still trails the (incorrect) 0xFF flush by are exactly the 46
blocks that landed on j=2.  Encode and decode speed are unchanged -- round-robin
twin builds, best of 6 passes, 3 rounds, medians: enc 78.5 -> 80.6 MB/s, dec
46.6 -> 46.9, inside a 13% within-build spread.

## The header model

The header stream codes each bit against an FSM counter, one per bit position
per lane: `RCio::hdrctr[RCNUM][24]`, 16 positions for the length and 8 for the
head byte, carried across blocks and reset once per file.

Per **lane** is what makes it work, and it is not a small effect.  The lanes
are not interchangeable -- they are fixed slices of the bit stream with fixed
jobs, and their lengths sit in tight bands a long way apart:

```
  lane   mean length   sd     bits/length, position-only model
     0        127      72          7.997
     1       3276     107          9.020
     2       1648     252         10.491
     5       3523      76          8.989
    ...
```

One shared set of 24 counters sees the union of those bands, so its top
position bits look near-random and it learns almost nothing: 11.530 bits a
length.  Per lane it is 8.907.  On the head byte the two are nearly the same
(6.908 vs 6.865) -- that byte's skew is `k%8`, which has no lane in it.

```
  static, from the enwik8 header data      header bytes    saved
    raw                                          73248         -
    24 counters, shared                          56273     16975
    24 x RCNUM, per lane                         48135     25113
    (byte-level order-0, for reference)          52917     20331
```

The per-lane bit model beats byte-level order-0, which is the point: the lane
is the context that matters, and a byte-level coder without it does not have
it.

### The head byte is not coded whole

Everything above about the flush says what the head byte's bottom bits are:
its trailing run of 1s is exactly `k%8`, and the bit above the run is the zero
that stopped it.  So the run goes first, in a 3-bit field of its own with three
more counters per lane, and only bits 7..r+1 of the byte are left to code.

Splitting a value into a prefix and a remainder is its own entropy either way
-- `H(byte) = H(r) + H(rest | r)`, and r is a function of the byte.  What it
buys is that a **position-only model cannot see structure inside a byte**: bit
3 of the head byte is 1 whenever r>3, and no counter indexed by position alone
learns that.  Told r first, it does not have to.

```
  head byte, per-lane counters              bits/value
    8 position counters                       6.8648
    3 for k%8, then bits 7..r+1               6.5162
    pooled order-0 byte entropy (the floor)   6.4504
```

r is capped at 7, not 8.  A payload lane's flush cannot leave an all-ones head
byte -- that byte would have been dropped -- but the carry twin's degenerate
n==0 flush can end on one, so at r==7 bit 7 is coded like any other and 0xFF is
representable.

Decoded r over enwik8 comes back as 3092/3023/2993/3038/3035/3091/3124/3020,
which is the `k%8` histogram at the top of this document, byte for byte.

### Measured

```
  header stream   49654 bytes over 1526 blocks   32.54 a block   (48 raw)
  enwik8          62,514,618 -> 62,489,498       -25,120
  vs the raw head row it started from            -23,594        4.9992 bpc
```

Of that, 23,769 is the per-lane position model and 1,351 the k%8 split (against
a 1,064-byte estimate -- the counters do slightly better than the static bound
because r is not quite uniform within a lane).  49654 against a static optimum
near 47,000 plus about 1,660 bytes of flush: the FSM gives up a couple of
percent to adaptation, which is what an FSM counter costs.

Encode speed is unchanged; decode is about 1% slower (round-robin twins, 7
rounds, best of 6 passes, medians: enc 80.48 -> 80.79 MB/s, dec 46.95 ->
46.43).  Most of a first, larger regression was `hdr_Init`'s reset loop being
inlined into `do_process` -- 384 counters once per file, costing registers in
the loop that runs 800 million times, the same effect that makes `model_pass`
NOINLINE.  It is NOINLINE now.

### What is left in it

No context beyond the bit position: a bit does not see the bits above it in
its own value, so the model cannot learn that a length starting `0x09` is
likely to continue one way.  A binary-tree context within the value -- the
`ctx`/`cty` shape the payload model already uses -- is the obvious next step,
and the gap between the position-only bound (8.907 bits a length) and the
per-lane byte entropies says there is something in it.  The other thing the
data suggests and this model does not use: a length is close to the same
lane's length in the *previous* block (sd 72-250 against means in the
thousands).

## What the header is worth

The numbers this model was built against.  enwik8, 1526 blocks x 16 lanes =
24416 substreams; order-0 entropy of each header field in the raw format:

```
                              bits    x24416 = bytes
  length low byte             7.929            24198
  length high byte            2.959             9032
  head byte                   6.450            19687
                                       ------------
  header raw (48832 + 24416)                   73248   0.117% of the file
  order-0 coded                                52917
                                             = 20331 saved,  0.033%
```

The head byte is *not* where most of that is -- it gives 4.7 KB, and the length
high byte gives 15.6 KB, because substream lengths cluster tightly (min 57, max
4163, mean 2558, so the high byte is nearly always 0x09 or 0x0A).  And coding
the length as one 16-bit symbol is *worse* than as two bytes (10.535 vs 10.888
bits), as is delta-coding against the previous lane (10.914) -- the lanes'
lengths are close to each other but their low bytes are not correlated, so the
delta only destroys the high byte's structure.  Delta against the same lane in
the previous block is a different question, and untried.
