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

## What the header is worth

enwik8, 1526 blocks x 16 lanes = 24416 substreams.  Order-0 entropy of each
header field, as it stands:

```
                              bits    x24416 = bytes
  length low byte             7.929            24198
  length high byte            2.959             9032
  head byte                   6.450            19687
                                       ------------
  header today (48832 + 24416)                 73248   0.117% of the file
  order-0 coded                                52917
                                             = 20331 saved,  0.033%
```

Two things to note before building a header coder.  The head byte is *not*
where most of that is -- it gives 4.7 KB, and the length high byte gives 15.6
KB, because substream lengths cluster tightly (min 57, max 4163, mean 2558,
so the high byte is nearly always 0x09 or 0x0A).  And coding the length as one
16-bit symbol is *worse* than as two bytes (10.535 vs 10.888 bits), as is
delta-coding against the previous lane (10.914) -- the lanes' lengths are close
to each other but their low bytes are not correlated, so the delta only
destroys the high byte's structure.
