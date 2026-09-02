# Using the instruction cache as a lookup table -- and why it cannot be one

## The idea

L1 data cache is small.  The instruction cache is a second L1 of the same size
sitting right next to it, and it caches whatever the front end fetches.  So:
generate a run of instructions that does nothing -- short jumps hopping across
cache lines, two bytes of code per two lines -- interleaved with useful data,
execute it once so the front end pulls those lines into L1i, and then read the
data from the main loop.  The data rides in a cache the loop was not using.
For the FSM table, renumber the states to step around the bytes the jumps
occupy, and the lookups come out of a cache that has nothing else in it.

It is a good question because the two halves of it are each true in
isolation: L1i *is* a cache the data path is not using, and a line in L1i *is*
faster to reach than main memory.  What breaks is the step between them.

## Why a load cannot read from L1i

On x86 (and every ARM since the Harvard split arrived at L1), the load and
store units talk to **L1d only**.  A `mov` from memory probes L1d, and on a
miss goes to **L2**, which is the first level shared by code and data.  It
never looks in L1i.  There is no datapath from the instruction cache to the
load ports, in the same way there is none from the register file to the
front end.

So the sequence the idea needs -- line fetched as code, line served as data --
gives:

```
  line in L1i, not in L1d:   load misses L1d  ->  L2 hit   ~14 cycles (Skylake)
  line in L1d:                                    L1d hit   ~5 cycles
```

Executing the jump run does one useful thing for a later data read: it brings
the line into L2 (Intel's L2 is inclusive of L1i on the parts this runs on).
That is a weaker version of `prefetcht1`, bought with front-end bandwidth and
L1i pollution, on a decoder whose binding constraint is the front end.

The uop cache (the DSB) is the same story one level up: it holds decoded
uops, keyed by fetch address, and is not addressable by loads at all.

Writes are worse than useless.  If a store hits a line that is present in
L1i, the core treats it as self-modifying code and takes a **machine clear**
-- a full pipeline flush, a few hundred cycles -- on every such store.  The
decoder stores to `stats[]` once per bit.  The writable half of the model
could not be placed this way even if reads worked.

## The variant that does use L1i: execute the data

There is one way to get bytes out of the instruction cache: run them.  Encode
the table as code -- a run of `mov eax, imm32; ret` stubs, or a jump table
into them -- and *call* entry `i` instead of loading it.  The value arrives in
a register from an immediate, fetched by the front end, and L1i really is the
cache that serves it.

What it costs is the branch.  The target is `base + i*stride` with `i` the
lookup index, and an indirect branch whose target the predictor cannot guess
costs 15-20 cycles -- measured below at 25-29 ticks a lookup, stubs in L1i.  For an FSM lookup the index is the adaptive state, which
is unpredictable *by construction* -- if the predictor could guess it the
model would have nothing to model.  This round measured exactly that
mechanism from the other side: the generated if-tree in `iftree_lanes.md`
wins 1.27x on text, where the branches predict, and on incompressible data
it goes 0.94x, slower than the loop.  An unpredictable branch per lookup
against a 5-cycle L1d hit is not a trade.

## Measured: `misc/icache_call_probe.cpp` -- the table as code

That variant was built and timed too: entry *i* is `B8 imm32 C3`, a lookup is
an indirect call to it, and every lookup is chained (the value fetched is the
next index) so the number is latency.  Three orders -- a fixed random cycle,
which a predictor can learn; sequential; and *scrambled*, where the fetched
value is mixed with an LCG before it becomes the next index so the sequence
never repeats, which is the FSM's situation -- at two strides, 8 bytes
(N=2048 is 16 KB, inside L1i) and 64 (one entry a line, 128 KB, outside it).
The data columns are the same chains through a plain array, so they are the
L1d number at 8 B and the L2 number at 64 B / N=2048.  Everything warm;
ticks per lookup, TSC 2.80 GHz, min of five:

```
                  --- table as CODE: call mov eax,imm; ret ---   ---- table as DATA: load ----
     N  size    cycle/8B seq/8B cycle/64B seq/64B scr/8B scr/64B   cycle/8B cycle/64B scr/8B scr/64B
    16   ..       5.3     5.2      4.9     4.9    25.3    22.1       6.6      6.6      8.2    8.2
    64   ..       5.4     5.2      5.0     5.0    25.6    23.0       6.6      6.6      8.2    8.6
   256  2/16K     5.6    23.9     13.0    21.6    26.1    23.4       6.6      6.6      8.3    8.2
  1024  8/64K    24.9    27.0     31.9    33.6    27.4    30.4       6.6     14.0      8.2   10.4
  2048 16/128K   26.2    27.5     35.0    36.2    29.2    35.2       6.6     14.0      8.4   13.2
```

Read it in three parts.

**Yes, it can be faster than a load -- when the target is predictable.**  A
small fixed cycle (top-left) runs at 5.0-5.6 ticks a lookup against the data
chain's 6.6: a predicted `call`+`ret` delivering an immediate beats a load
whose address arithmetic sits on the chain, by about 20%.  That is the whole
of what "reading from the instruction cache" can do, and it is real.

**No, when it is not.**  The scrambled columns are the FSM's case, and at
every N the code lookup costs **25-29 ticks with the stubs in L1i** against
**8.2 for L1d and 13.2 for L2**: three times a data load, twice an L2 miss.
That is the indirect-branch misprediction, and it does not get cheaper with a
smaller table -- N=16 mispredicts as hard as N=2048.  The fixed cycle looks
good at N<=256 only because the predictor memorises it; the FSM's next state
is decided by the data, and nothing memorises that.

**Being in L1i helps the miss, not the hit.**  At N=2048 scrambled, 8 B (16
KB, resident) is 29.2 and 64 B (128 KB, not) is 35.2: about 6-9 ticks, which
is the refetch after the misprediction coming from L1i instead of L2.  It
moves the wrong number.

(One artefact worth a line so nobody chases it: at N=256 the *sequential*
order mispredicts -- 23.9 -- while the *random* cycle over the same 256
targets predicts, 5.6.  Targets 8 bytes apart in address order alias in the
indirect predictor's indexing; a permutation of the same addresses spreads
them.  Sequential is not the easy case here.)

For the FSM lookup that started this: 234 states, index decided by the data,
today an L1d hit on a fused load.  As code it would be a 26-tick
misprediction per bit, on a decoder that spends 8.7 ticks a bit in total.

## Checking the premise: how big is the FSM, really?

`N_STATES` is 32768 and `FSM2` is declared at 256 KB, which is what made the
question look live.  But `FSM0.txt` is 4608 bytes.  Measured:

```
  states defined in FSM0.txt              256
  reachable from state 0                  245   (max index 253)
  visited, 800,000,000 updates, enwik8    234
  FSM2 cache lines touched                 32   (2 KB; 31 if perfectly packed)
```

The other 32,512 entries are parsed past end-of-file into state 0 and never
reached.  The table the decoder actually uses is **2 KB, contiguous, states
0..253**, and it is already as packed as a renumbering could make it -- 32
lines used against a floor of 31.  Beside it sits `stats[]`, 512 counters at
4 bytes, 2 KB.  The whole model is 4 KB in a 32-48 KB L1d, and the input rows
stream through underneath it once.  It is resident, all of it, all the time.

The concentration is real, for what it is worth:

```
  top   8 states     39.1% of updates
  top  16            53.1%
  top  32            68.9%
  top  64            84.7%
  top 128            95.9%
```

but concentration into fewer lines of a table that is already entirely in L1d
buys nothing.  The earlier rounds already established the FSM lookup is not a
capacity problem from the other direction: `RC_DEC_PREFETCH` measured nothing
(`dec_renorm.md` §6), and the decoder profiles as front-end bound with no miss
signature.  What the lookup costs is its **latency on the per-bit chain** --
five cycles, once -- and `RC_FUSE_PP` already halved that by folding `pp[]`
into the same load.  There is no cache to move it to that is closer than L1d.

## The only storage closer than L1d: the register file

If the aim is "a lookup that does not go through L1d", the honest answer on
this hardware is a register-resident table.  AVX-512 makes about 2 KB of zmm
registers addressable as a LUT: `vpermb` indexes 64 bytes, `vpermi2b` 128,
`vpermi2d` 32 dwords from a register pair, at 3 cycles latency and one a
cycle.  The 64 hottest FSM states, at 8 bytes each (both `(p,next)` entries),
are 512 bytes -- eight zmm registers, a two-level `vpermi2d` select.  It is
the one form in which "the hot part of the FSM lives in a cache the loop is
not using" is physically true.

And it does not pay here, for a reason this tree has already paid to learn.
The per-bit path is scalar: `imul`, `cmp`, `cmov`, and the index it would
hand the permute is a scalar in a GPR.  Getting it into a vector register and
the result back is two `movd` round trips, three cycles each, which is the
L1d latency it was trying to save -- plus a fallback path for the 15% of
updates outside the hot 64, plus the extra instructions on a loop that is
front-end bound and prices each one at ~0.24 cycles.  `dec_vectorize.md` §4
already measured "put the model lookup in the vector unit" through the split
shapes and it lost every time; this is the same move with a smaller table.

## Where the idea has a legitimate home

Two places, neither of them a cache:

- **The compiler already keeps small constant tables in the instruction
  stream** when it can -- immediates, `lea`-computed values, switch tables
  reached by predictable branches.  The generated if-tree is the extreme case:
  255 constant addresses baked into code, the tree walked by the predictor.
  That works when the *index* is predictable, which is what "code as data"
  actually requires: the front end is an associative memory keyed by control
  flow, not by an integer.
- **Prefetch.**  The only real effect executing the jump run has is to pull
  lines into L2.  If a table were too big for L1d and read with a pattern the
  hardware prefetcher missed, a `prefetcht1` sweep in the block prologue does
  the same thing without the front-end cost.  Nothing in this coder is that
  table; the decoder's per-block data is the model (4 KB, resident) and the
  input rows (streamed once, prefetched by the stride detector).

## Measured: `misc/icache_probe.cpp`

The claim above is testable, so it was tested.  A 4K-aligned 128 KB block is
filled with a data pattern and threaded with an instruction sequence that
touches every line -- a 2-byte short jump straddling each line boundary
(`EB 3E` at bytes 63..64 of every line, jumping to the next boundary), ending
in `ret`.  Reads are timed as a dependent-load pointer chase over a random
single cycle of lines, so the tick count is the latency of wherever the line
lives and not a prefetcher's bandwidth; and they are timed over the first and
last 16 KB separately, because 128 KB is four L1i's and only the *last* lines
executed can still be in L1i.  Cold means a 256 MB read plus `clflush`.
Intel Cascade Lake, TSC 2.80 GHz, minimum of five:

```
  ticks per dependent load          whole 128KB   first 16KB   last 16KB
  cold (256MB read + clflush)         106.0        127.0        102.7
  after EXECUTING the block            12.4         12.5         12.5
  after reading it as data             12.4          5.0          5.0
  read twice (L1d reference)           12.4          5.0          5.0
  cold again (control)                136.2         93.2         98.9
```

Three numbers calibrate it: 5.0 is an L1d hit, 12.4 is an L2 hit (the whole
128 KB does not fit L1d, so even the data-warm chase of the full block runs at
L2 speed while the 16 KB regions run at L1d), and 100-136 is DRAM.

The row that answers the question is **after executing the block, last
16 KB: 12.5**.  Those are the lines the front end fetched most recently; they
are in L1i.  A load reads them at exactly the L2 latency -- the same 12.5 as
the first 16 KB, which left L1i long ago -- and not at the 5.0 that a line in
L1d gets.  Executing the code did one thing for the data path: it filled L2.
A 1 GB flush gives the same table.

Windows builds with `cl /O2 icache_probe.cpp`; that branch is written to the
documented API and not yet run.

## Verdict

The mechanism the idea rests on -- a data load served from L1i -- does not
exist, and the probe shows it: a line that is in L1i and not in L1d reads at
L2 latency, 12.5 ticks against L1d's 5.0.  L1i and L1d meet at L2, so the
best case is an L2 hit where an L1d hit was already happening, and a store to
such a line is a machine clear.  The
variant that does read L1i, executing the data, replaces a 5-cycle load with an
unpredictable branch.  And the table it was aimed at is 2 KB, already packed,
already resident, and already had its one latency win taken by `RC_FUSE_PP`.
The FSM lookup's remaining cost is the five cycles on the chain, and the only
thing closer than L1d is a register, which the scalar per-bit path cannot use
without paying more in moves than the load costs.
