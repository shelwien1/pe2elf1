# PSRC in oggcomp

`oc_psrc.inc` replaces the thing oggcomp used to do with every bit its model
produced -- hand it and its probability to the range coder -- with PSRC: the
probability picks a class, the bit goes into that class's pile, and a block
at a time the piles are run-length coded and *those* bytes reach the range
coder.

The reference is `../PSRC-006`, checked in beside this tree.  This is a port
of its scheme, not of its code; section 9 lists where the two differ and why.

Build the old coder back with `CXXFLAGS='-O2 -DOC_PSRC=0' ./mk.sh release`.
The two write different formats and say so in the stream header: `OC_VER` is
6 with PSRC and 5 without (`oc_coro.inc`), and `oggcomp d` refuses a stream
the build cannot read.

## 1. The shape of it

Encoding, `tc_bit()` no longer codes anything:

    p, bit  ->  class = ps_qs[p]  ->  pool[qn++] = class ^ (bit << 7)

and when the pool reaches `PS_BLOCK` bits, or the stream ends:

    pool -> sort by class -> per-class bit chunks -> run lengths -> range coder

Decoding runs it backwards.  A whole block is read and expanded before the
model asks for its first bit of that block, and then a bit is a byte read
from the cursor of its class:

    p  ->  class = ps_qr[p]  ->  bit = *ptr[class]++

So the model's inner loop does a table lookup and a byte access where it
used to do a range coder step, and the range coder runs a block at a time on
a different kind of data.

## 2. Classes

A class is a quantized probability with the likely outcome folded onto one
side: a magnitude, which is how far from even the model was, and a mirror
bit, which is which side of even it was on.

    mirror = p > 1/2
    magnitude = quantized |stretch(p)|,  PS_MAGN steps
    class = 2 * magnitude + mirror

The pool byte is the class with bit 7 holding `mirror ^ bit`, so bit 7 is 1
whenever the model was right.  A chunk is then a run of 1s broken by the
rare 0, which is the whole point: that is what run-length codes well.

The decoder never has to undo the fold.  Chunk `k` expands to runs of
`(k & 1) ^ 1` broken by `(k & 1)`, and since `k`'s low bit *is* the mirror,
what comes out is the bit the model coded:

    stored = bit ^ mirror,  expanded = stored ^ mirror = bit

## 3. Why the two sides agree

The model is adaptive, but causal: the state it predicts bit *i* with is a
function of bits 0..*i*-1 only.  So by induction the decoder's state before
bit *i* equals the encoder's, hence its probability, hence its class -- and
the block a bit falls in is `i / PS_BLOCK`, a function of *i* alone.  Both
sides walk the same bits in the same order into the same blocks and the same
chunks; nothing else has to be agreed.

This holds through oggcomp's adaptive-length values (`tc_fam::code` codes a
unary bit-length and then that many mantissa bits), because the length is
itself decided by a coded bit: both sides decide it identically and so emit
the same *number* of further bits.

## 4. The block

    length     n, the model bits in this block: a "full" flag, else 23 bits
    counts     the run bytes of each of PS_NCLS classes, freq()
    runs       the run bytes themselves, in ascending class order

`n` has to be coded because oggcomp, unlike PSRC, does not know a stream's
length in model bits before coding it.  Everything else follows from `n`:
the decoder serves `n` bits and loads the next block when they run out, so
there is no end-of-block marker and no end-of-stream marker -- the model
simply stops asking.

`freq()` codes the counts as a recursive split: the total, then how it
divides between the halves, down to single classes.  A subtree whose total
is zero costs one flag and says nothing more, which is what keeps it cheap
on a block where most classes are empty.

A run byte is the number of 1s before the next 0; 255 means 255 of them and
the run goes on.  The run a chunk *ends* on is the 32-bit alignment padding
and is dropped down to `255 * PS_TRAILFF` bits, which the decoder puts back
without being told (section 7).

## 5. The quantizer

`ps_tables()` builds `ps_qs`/`ps_qr` once, uniform in the stretch (logit)
domain over [1/4096, 1/2] -- read off `cm_st`, so the quantizer and the
model agree on what stretch means.

Uniform in stretch is what bounds the *relative* loss of bucketing.  A
bucket coded at its own average rate costs, per bit, about
`p(1-p) w^2 / (8 ln 2)` more than the bit's own probability would, for a
bucket of logit width `w`; dividing by the bit's own cost gives something
that stays flat in *p* rather than blowing up at either end.  At
`PS_MAGN = 64` the width is 8.32/64 = 0.13 and the bound is under 0.001
bits on a 1-bit decision.

Measured against this model's actual probabilities, over the whole corpus,
against the ideal code length of the unquantized probabilities:

    magnitudes   classes used   excess
    32           60             +0.05%
    48           86             +0.00%
    64           110            -0.01%
    120          190            -0.03%

Negative because bucketing is a secondary estimate: a class codes at the
rate its bits *turned out* to have, which can beat the rate the model named.
64 is the default; it is also the most the pool byte has room for, since the
class has to fit in 7 bits.

PSRC-006's own `qmaq` table measures within 0.03% of this on the same data,
but it is hill-climbed on enwik8 and is not monotone in *p* -- it gives
neighbouring probabilities non-neighbouring classes -- so it is not used.

## 6. The run model

Per magnitude (not per class: the two classes of a magnitude are the same
statistic written two ways, so they share counters):

    "the run goes on"     one flag, byte == 255, magnitudes under PS_FFMAG
    "the run ends here"   unary, PS_UNARY counters, one per length
    the rest              a binary tree over the byte, PS_TREECTX bits of
                          magnitude as context

A run in a class of rare-bit probability *q* is geometric in *q*, so its
length is one parameter and little else, and a chain of adaptive "is it over
yet" decisions is exactly the adaptive Golomb code for it.  That matters:
the first thing tried here was a plain 8-bit tree per class, 255 counters
each, which measured **2% over** what the classes say is achievable -- the
price of learning 32640 counters from half a million bytes.  The unary chain
has twelve counters per magnitude and cost 0.78%.

Two further things paid, and both are about counters that are cold or stuck:

**The flag is not free where the answer is never in doubt** (`PS_FFMAG`).
Asking "does this run pass 255" on every byte of a class whose runs are
about one bit long cost 0.3% of the whole file.  Not because the question is
expensive -- because `cm_cnt`'s update is integer and stalls before it
reaches the edge: at limit *t* it stops about *t* short of certain, which at
`PS_LIM_FF = 4095` pins a never-happens flag at 0.09 bits instead of the
0.0004 a 12-bit probability could say.  Lowering the limits and asking the
question only where runs can be long took that back.

**A counter that has never seen anything still knows the class it is in**
(`PS_PRIOR`).  Magnitude *m*'s rare-bit probability is not a mystery: it is
what defines the magnitude.  `prime()` seeds every run counter from the
geometric that follows from it -- the unary chain from *q*, the "run goes
on" flag from (1-*q*)^255, the escape tree from the same geometric truncated
to the range the escape covers -- with `PS_PRIOR` observations' worth of
weight.  Worth 0.05% on music and 0.6% on the small bundled testfiles, where
nothing ever warms up.

## 7. Buffers and what cannot overrun them

Encode: `PS_BLOCK` pool bytes, then the run bytes in place over them, then
the bitsort's word buffer at `PS_TMPOFF` -- past where the run bytes can
reach, not merely past the pool, so the in-place rewrite can never clobber a
word it has not read.  Every run byte covers at least one bit of its chunk
and a chunk is its bits rounded up to a word, so the run bytes of one block
are at most `PS_BLOCK + 31 * PS_NCLS` = `PS_RLEMAX`.  That bound is
`static_assert`ed against `PS_TMPOFF` and `FATAL`-checked in `flush()`,
because `freq()` is given it as a ceiling and would silently clamp a total
that broke it -- which would desync the decoder with nothing to show.

Decode: the expansion is bounded the same way and every write is checked
against it, so a damaged stream whose counts say a chunk expands to a
hundred times its block cannot get past the end.  The one unchecked access
is the model's own `*ptr[k]++`, and it is safe by arithmetic: a block serves
at most `PS_BLOCK` bits, each advancing one cursor by one, so no cursor can
get more than a block past where it started -- and `PS_DECCAP` leaves
exactly that much slack after the expansion.  The slack is never written, so
it costs no pages.

The trailing-run trim is the other bound worth writing down.  The encoder
stops emitting the run a chunk ends on once it is down to
`255 * PS_TRAILFF` = 510 bits; the decoder appends `255 * (PS_TRAILFF + 1)`
= 765.  Those bits are all 1s and so is the word padding they are attached
to, so the first `32 * words` expanded bits are exactly right and the model
reads only the first `n_k` of them.  255 bits of margin.

120 truncations and 400 turned bytes of a `.oc`, across three inputs: every
one a clean refusal, no crash, no hang.

## 8. What it measures

Seventeen Vorbis files, 12.7 MB, `-O2`, the same model both ways:

                        bytes        vs plain RC
    plain rangecoder    11305849
    PSRC                11318883     +0.115%

    the bundled testfiles (34 files, mostly under 10 kB)
    plain rangecoder      509631
    PSRC                  511041     +0.277%

                    encode            decode
    5.7 MB file     9.76 -> 10.41s    9.52 -> 10.32s    +7% / +8%
    2.9 MB file     4.49 ->  4.92s    4.45 ->  4.96s   +10% / +11%
    0.9 MB file     1.77 ->  1.87s    1.71 ->  1.89s    +6% / +10%

So on this model PSRC costs about a tenth of a percent of ratio and about a
tenth of the running time.  It is worth being clear about why, because
neither number is a bug.

**Ratio.** `-DOC_PSRC_STATS` accounts for every bit the coder spends.  On
the 2.9 MB file: 2533188 bytes written against 2530116 that the classes say
is the floor, so the whole PSRC overhead is +0.12% and it is spread thin --
0.03% the per-block headers, 0.03% the run-goes-on flags, 0.06% the run
lengths themselves.  The classes are not the problem; the quantizer is at
parity or slightly better than the model's own probabilities (section 5).
What is left is the run model being an adaptive model of its own where the
plain coder had none.

**Speed.** PSRC's promise is that the range coder stops being called once
per model bit.  It is kept: that file is 31483290 model bits and 25848341
coder decisions, 18% fewer.  But oggcomp's plain coder was never the
expensive part -- one `rc_Process` against a mixer, two APMs, four hashed
counter tables and 366 MB of them -- and each PSRC decision carries a
counter update the plain path did not have, plus a pass over the pool to
sort it.  18% fewer decisions of a more expensive kind, plus the sort, is a
net loss, and the model is large enough that the whole thing shows up as
about 10%.

PSRC pays where the model is cheap and its output is lopsided, which is what
PSRC-006 is: a small order-1 model over text, where the coder really is the
bottleneck and runs really are long.  Here the model's output averages 0.64
bits per decision -- the model is often genuinely unsure -- so the runs are
short and there is not much coder work to remove.

What the change does buy, and what the reference implementation is built
around, is that the coder is no longer in the model's inner loop: it reads a
block of quantized classes and touches nothing the model owns.  That is the
precondition for running it on another thread, or splitting the classes
across several coders.  Neither is done here, and Amdahl caps what either
could return at the coder's ~10% share.

## 9. Where this differs from PSRC-006

- the quantizer is derived, not the tuned `qmaq` table (section 5);
- the run model is contexted on the magnitude, so the serpentine chunk order
  PSRC uses to carry one counter set from chunk to chunk buys nothing and is
  not done;
- the run counters are `cm_cnt`, this tree's, which clamps to [1, 4095] and
  starts at a state-count-driven rate;
- they are seeded from the class (section 6), which PSRC has no need of
  because it never codes a short stream;
- the block length is coded, because it is not known in advance (section 4);
- the count coder drops PSRC's degenerate second flag when a subtree's
  ceiling is 1, and takes the recursion depth as context;
- every decode-side write is bounded, because a `.oc` may be damaged.

## 10. Knobs

All `#ifndef`, so `CXXFLAGS='-O2 -DPS_MAGN=48' ./mk.sh release` works.
Every one of them changes the format: a stream is only readable by a build
with the same values.

    OC_PSRC       1     0 puts the plain rangecoder back
    PS_MAGN       64    magnitude levels; 2*this classes, at most 128
    PS_BLOCKLOG   23    model bits per block, log2
    PS_UNARY      12    run lengths coded one decision at a time up to here
    PS_TREECTX    4     magnitude bits the escape tree is contexted on
    PS_FFMAG      32    magnitudes that get the "run goes on" flag
    PS_PRIOR      128   observations' weight given to the seeded priors
    PS_LIM_*      ...   counter rate limits

Swept over both corpora; the plateau is broad.  Within the corpus measured,
`PS_MAGN` trades the two against each other -- 32 is 0.05% better on the
small bundled files and 0.04% worse on music -- and `PS_BLOCKLOG=24` is
worth another 0.015% for twice the buffer.

`-DOC_PSRC_STATS` adds an exit-time report of where the coder's bits went,
by part, against what the classes say the floor is, with the bit count and
rare-bit rate of every class.  Measuring only; it does not change a stream.
`oggcomp c -v` carries a shorter version of the same thing: a `coder` line
under the per-stage breakdown, saying what was actually written against what
the model's probabilities say it should have cost.

## 11. What was tried and did not pay

- **A binary tree per class over the run byte.** +2% over the class floor,
  all of it learning 255 counters per class (section 6).
- **The previous run byte of the same magnitude as extra context.** Four
  times the counters for the length chain; 0.06% *worse*.  Runs are not
  bursty enough to pay for the dilution.
- **The count coder contexted on the ceiling rather than the recursion
  depth.** 0.01% worse, and the same information later in the recursion.
- **PSRC-006's `qmaq`.** Within 0.03%, not monotone (section 5).
