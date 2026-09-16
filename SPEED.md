# Making the walk faster

[ALGORITHM.md](ALGORITHM.md) describes what this repository does: it turns a
model that predicts one **bit** at a time into a probability for every one of
the 256 possible next **bytes**, by running the model's compiled step
speculatively down the prefix tree and rolling its memory writes back through a
journal. This document is about what that costs, where the cost actually is, and
what can be done about it.

The short version: **about four fifths of the time is the journal, not the
model.** Four changes came out of that - 40 % faster by default on `tangelo_w`,
43 % with the opt-in fifth - and two plausible-looking ideas turned out to be
worth nothing at all.

Contents

1. [How to measure it](#1-how-to-measure-it)
2. [Where the time goes](#2-where-the-time-goes)
3. [What worked](#3-what-worked)
4. [What did not](#4-what-did-not)
5. [What is left](#5-what-is-left)
6. [All the numbers](#6-all-the-numbers)

---

## 1. How to measure it

Everything below is `tangelo_w` on the first 64 KB of `book1`, built with GCC 13
`-O3 -march=native` and run on a 2.8 GHz Xeon, best of four runs. The machine is
noisy to about ±10 % on a single run, which is enough to hide any of the smaller
effects, so nothing here rests on one timing: where two builds are compared they
are run alternately in one session and the best of each is taken.

Three tools made the difference between guessing and knowing.

**Counting, not estimating.** `NEST()` counts speculative steps and `UNDO()`
counts journal cells by width, in a build that does nothing else differently.
Those two numbers - steps per input byte, cells per step - explain most of what
the profile shows, and they are stable where timings are not.

**`track.pl --repeat=N`.** This is the useful one. It emits each journaling call
N times instead of once, which is harmless: both copies save the same pre-store
value, `UNDO()` restores both, and the program produces *exactly the same output*.
So the journal's cost can be measured by making it bigger, on a program that
still works, with everything else held constant:

| journal passes | time |
| ---: | ---: |
| 1 | 6.48 s |
| 2 | 11.75 s |
| 3 | 17.00 s |

That is a straight line: 5.26 s per pass, and 1.22 s left when extrapolated to
none. **The journal is 81 % of the run; the model and everything else is 19 %.**
An ablation that simply removed the journaling would not have measured this, and
would have been worthless anyway: with nothing rolled back the model corrupts,
the code lengths change, the pruning changes, and the program stops doing the
same amount of work.

**Counting instructions, when the clock will not hold still.** Several of the
changes here are worth a few per cent, and on this machine a few per cent is not
something the clock can see. Not because of noise that averaging removes: two
builds whose hot code is *the same instructions* can differ by 8 % because the
linker put `UNDO()` at a different offset, and that difference is perfectly
stable across runs. (That is not hypothetical - it is how the first version of
§3.5 was nearly accepted, and then nearly rejected, on the same code.)
`valgrind --tool=cachegrind` counts instructions, data reads, data writes and
cache misses exactly, per function, unaffected by alignment or scheduling and
reproducible to the instruction. The rule used from §3.5 on: **cachegrind says
whether a change is real, the clock says whether it matters**, and where they
disagree both are reported. Cachegrind cannot run AVX-512, so those runs are
built `-march=x86-64-v3`, and they use a 4 KB input because they are ~50x slower.

**Verifying every step.** Each change here alters hand-written assembly or the
shape of the walk. `-DTRACK_VERIFY=48` (§5.5 of ALGORITHM.md) compares all
360.8 MB of model state before and after each walk, checks that every journal
cell points inside the state the model declared, and checks the walk's predicted
code length against the model's real one for every byte. Every optimisation below
was kept only after that passed and the compressed output came out byte for byte
identical.

## 2. Where the time goes

Per input byte, on English text, with the default pruning:

| | |
| --- | ---: |
| speculative model steps | 73.5 |
| journal cells | 26 323 |
| journal cells per model step | 358.1 |
| journal traffic | 206 KB |

Two things stand out.

**The tree is already mostly pruned.** 73.5 steps out of a possible 254 - the
code-length threshold and the alphabet map together throw away 71 % of the tree
before it is walked. Pruning harder is available (`PRUNE_LOG`) and costs
compression; §4.3 of ALGORITHM.md has the trade curve.

**Each model step writes ~1.3 KB of state.** That is Tangelo, not the framework:
22 context slots each advancing a bit-history byte and a `StateMap` entry, five
mixer contexts each rewriting 80 weights, three APM stages, the match model. 328
journal cells per step is the direct consequence, and 376 KB of journal traffic
per input byte is the direct consequence of that.

So the cost is not the model's arithmetic. It is the bookkeeping around the
model's *writes*, and every worthwhile optimisation below either reduces the
number of writes journaled or reduces what journaling one write costs.

(Those figures are after §3.5. Before it a cell was 16 bytes: 24 091 cells and
376 KB per input byte. The cell count went *up* and the traffic nearly halved,
which is the whole of §3.5 in one line.)

## 3. What worked

### 3.1 Test before the step, not after (15 %)

The walk used to enter a node and then decide whether to go further:

```
visit(ctx, bit):
    NEST(); p = encode_sim(bit)          # the model is now inside the child
    if bit == 0: fill both children's code lengths from p
    if clen[child] > threshold: visit(child, 0); visit(child, 1)
    UNDO()
```

The model step is paid for *every child of every visited node*, including the
children it then declines to explore. But nothing in that step is needed to
decide: both children's code lengths come from the prediction at the **parent**,
and `P()` gives that with no side effect at all. So the test can move in front of
the step:

```
go(ctx, depth):
    p = P()                              # no update, no journal, no rollback
    fill both children's code lengths from p
    for bit in 0, 1:
        if live[child] and clen[child] > threshold:
            NEST(); encode_sim(bit); go(child, depth+1); UNDO()
```

A pruned child now costs nothing: no step, no journaling, no rollback. The code
lengths are unchanged - `tangelo_w` produced byte-identical output, 20 739 bytes
- and it is 15 % faster. On a fully unpruned walk the two shapes do the same
254 steps; the whole gain is the pruned frontier, which the old shape paid for
and threw away.

This also removed the `if (bit==0)` inside the hot template and the separate
leaf specialisation, which is a branch and a template instantiation per node.

### 3.2 An alphabet map (19 %, and 0.2 % smaller)

`coder0.cpp` codes 256 flags at the head of the stream saying which byte values
occur in the file at all. It is worth more here than it is there, for a reason
specific to this framework: a byte value that cannot occur makes a whole
*subtree* unreachable, and an unreachable subtree can be skipped **exactly**,
where the code-length threshold can only skip an improbable one approximately.

`book1` uses 82 of the 256 values. The map costs 256 bits and pays for itself
twice:

* **Size.** Each dead symbol was holding one of the ~65 536 frequency units, and
  every coded byte paid for all 174 of them. Removing them is worth about
  0.0038 bits per byte - 405 bytes on `book1` for `tangelo_w`, 407 for
  `fpaq0mw`, against a 32-byte header.
* **Speed.** The walk skips the dead subtrees, and the frequency loop shrinks
  from 256 scattered lookups into a 4 MB table to 82.

Together, 19 % on top of §3.1. The encoder decides whether the map is worth
sending - it is the side that knows how many values are missing - and spends one
bit saying so, because on a short file 32 bytes is more than the map can save.
The break-even is around 64 KB.

### 3.3 One journal record for a wide store (8 %)

The journal cell is `{ptr, mask, value}`, 16 bytes holding at most 4 bytes of
saved data. A 16-byte SSE store from the mixer's weight update therefore cost
four cells, 64 bytes of journal, to save 16 bytes; a 32-byte store cost eight
cells and 128 bytes. Those wide stores were **46.7 % of all journal cells**.

They are now one record: the overwritten bytes copied verbatim, 16 to a cell
(8 since §3.5), followed by a footer naming the address and the width. A 32-byte
store is three cells instead of eight. `UNDO()` walks backwards, so the footer is written last
and is met first; its mask field holds the width, which is what distinguishes a
record from a plain cell (16, 32 and 64 are none of the narrow tags 0xFF,
0xFFFF, 0xFFFFFFFF).

| | before | after |
| --- | ---: | ---: |
| journal cells per byte | 28 554 | 24 091 |
| journal traffic per byte | 457 KB | 376 KB |
| wide stores' share of cells | 46.7 % | 18.6 % |

8 % faster, identical output. The payload is copied through `rdx` in 8-byte
pieces rather than a vector register, because the stub has to leave every
register it does not save exactly as it found it, and saving an XMM register
would cost more than the copy.

### 3.4 Inlining the journal code (5 %, opt-in)

What is left after §3.3 is per-*call* cost rather than per-cell cost: the counts
fell 16 % but the measured journal time fell only 4 %, because the number of
calls did not change. `track.pl --inline` writes the cell at the store site
instead of calling a stub, for the 1-, 2- and 4-byte widths - which is 81 % of
the calls.

It needs one more push/pop than the stub does. The stub gets the target address
for free in its argument register, which the caller has already saved; inline
code has three live values (the cell address, the target address, the saved
word) and two scratch registers, so the target goes on the stack. Even so it
wins the call and the return.

5 % faster, and 48 % more instrumented assembly - and still 5 % after §3.5 made
the stub it replaces cheaper, 6.20 s to 5.87 s. It is off by default for the
assembly: the trade was measured on one model, and a bigger model's step is more
likely to care about instruction cache than this one is.

### 3.5 A journal cell in 8 bytes (6 %, and 10 % for `fpaq0mw`)

The cell was `{void* ptr; uint msk; uint val;}` - 16 bytes, of which eight were a
pointer and four a width tag, to save at most four bytes of data. But every
address the journal ever sees is inside one window of memory: the model object
and the few globals beside it. So the pointer is now a 30-bit offset from the
base of that window, and the width is the two bits that frees:

```
off = (address - trk_base) | (tag<<30)     tag 0, 1, 2 = 1, 2, 4 bytes
val = the overwritten bytes                tag 3       = wide record footer
```

Three things had to work out, and the second one is the whole story.

**The stub has to form the offset without touching EFLAGS.** `sub` sets flags and
`lea` cannot subtract a register, so the base is kept negated in memory and added
instead: `mov rdx,[rip+trk_negbase]` then `lea edx,[ARG+rdx+tag<<30]`, where the
tag is a displacement and therefore free. That load is the one instruction the
new format adds to a stub. It takes back three: `trkptr` is now a pointer rather
than an index, so the cell address needs no scaling, and the cell is two stores
where it was three. `track4` went from 15 instructions to 13, and from 962 M
instructions to 834 M on the reference run.

**`UNDO()` must not pay the offset back.** The obvious `p = base + (off &
0x3FFFFFFF)` costs a mask and an add per cell, and the first version of this
change did exactly that. Measured: the stubs got 15 % cheaper and `UNDO()` got
15 % dearer, for a total of **+0.3 % instructions** - a change that nearly halves
the journal's memory traffic and comes out slightly *worse*. It is avoided by keeping four base
pointers in `UNDO()`, one per tag, each pre-biased by that tag's bit pattern; the
cell's offset then indexes the right one directly and the compiler folds the bias
into the store's displacement (`mov [r10+rdi-0x80000000],edx`). The tag is read by
comparing the whole offset against the tag boundaries rather than by shifting it
out. The inner loop went from 12 instructions per 4-byte cell to 9, and `UNDO()`
from 2.44 G instructions to 1.79 G.

**The wide-record branch must stay out of the common path.** Inlined, its copy
loop wants four callee-saved registers, so `UNDO()` built a stack frame and saved
them on *every* call - four stack writes and four reads per speculative step,
paid in full by
`fpaq0mw`, whose model has no store wider than 4 bytes and never reaches the
branch at all. Records of one or two payload cells - every 8- and 16-byte store,
which is nearly all of them - are now copied straight line, and only the tail is
a call. An 8-byte store became a record too: the same two cells as before, but
one iteration of `UNDO()`'s loop instead of two.

| `tangelo_w` | before | after |
| --- | ---: | ---: |
| instructions (4 KB of `book1`) | 7.33 G | 6.34 G |
| data writes | 1.54 G | 1.38 G |
| L1 data misses | 14.5 M | 9.7 M |
| journal cells per input byte | 24 091 | 26 323 |
| journal traffic per input byte | 376 KB | 206 KB |
| time (64 KB of `book1`) | 6.52 s | 6.15 s |

`fpaq0mw` gains more, because its model step is four stores and almost all of its
time is journal: 1.99 G instructions to 1.85 G, and `book1` from 1.985 s to
1.777 s. Both outputs are byte for byte identical, and both pass
`-DTRACK_VERIFY=64 -DTRACK_VERIFY_EVERY=5000` over all of `book1`.

What it costs is an assumption: a store outside the declared window would be
journaled against an address that aliases something else inside it. A program
declares its state in `TRACK_STATE_RANGES`, `Track_Base()` refuses a window wider
than 2^30 bytes at startup, and `-DTRACK_VERIFY` checks every cell's offset
against that window. The trade is deliberate - it is the same shape as the
annotations in §5, an optional promise from the model that the self-check
verifies - but it is the first place in this design where the model has to tell
the framework anything at all.

## 4. What did not

**A paired log table.** Every node takes `LOG2(SCALE-p)` and `LOG2(p)` - two
lookups at opposite ends of a 128 KB table, two cache lines. Packing both into
one qword indexed by `p` makes it one 8-byte load from a 32 KB table, which
looks like an obvious win: half the lookups, a quarter of the footprint.

Measured: 7.02 s to 6.97 s. Nothing, inside the noise. The table is hot, the
loads are independent, and there are only 73.5 nodes per byte against 26 323
journal cells - the walk's own arithmetic is not where the time is. Rejected,
and worth stating because the same reasoning kills several similar ideas: the
frequency loop's 256 lookups, the cumulative-frequency scans (~128 adds per
byte), the `clen` array initialisation (512 stores per byte). All of them are
noise next to the journal.

**A branchless `UNDO()`.** `UNDO()` dispatches on the cell's width, which is a
poorly-predicted three-way branch over tens of thousands of cells per byte. The
obvious fix is to restore every cell with one unconditional 32-bit
read-modify-write - which is what the code used to do. It was **9 % slower** and
could fault, because it reads and writes up to 3 bytes past a 1-byte cell.

Re-measured against the 8-byte cell it is worse still, because the mask is no
longer in the cell and has to be looked up from the tag: **8 % more instructions
on `tangelo_w`, 9 % on `fpaq0mw`, and 12 % more data reads on both**, for
identical output. The read is the problem, not the branch. Restoring at the
cell's own width, with a plain store that never reads its destination, is the
faster and the safer version.

## 5. What is left

Ranked by what the profile says they are worth, not by how interesting they are.

**Merge adjacent stores in `track.pl` (est. 10-20 %).** The remaining journal
cost is per call, and the model makes many pairs of stores to adjacent addresses
from the same base register - `p1[cxt]` and `p2[cxt]`, `runp[i][0]` and
`runp[i][1]`, consecutive `tx[nx++]`. Where `track.pl` can prove statically that
two stores are adjacent and nothing between them changes the base, it can emit
one wider journaling call covering both. That is a peephole pass over the
assembly it is already rewriting, and it attacks exactly the quantity that is
left: the number of calls.

**Let the model declare bulk state (est. 10-15 %, invasive).** Half of each
step's writes are the mixer rewriting five rows of 80 weights. Journaling that as
five block copies of 160 bytes, once per `NEST()`, would cost 800 bytes where the
200 cells that cover them cost 1 600. It needs the model to say "this region is bulk state", which
breaks the black-box premise the whole design rests on - but as an *optional*
annotation, checked by `-DTRACK_VERIFY`, it is defensible.

**Not journaling state that is dead across a step (unsound to infer).** `tx[]`,
the mixer's input vector, is overwritten from index 0 every step before it is
read, so its old contents never matter and journaling it is pure waste - perhaps
15 % of the cells. Nothing in the assembly says so, and getting it wrong is the
silent kind of wrong this codebase is built to avoid. It would have to be an
annotation, and it would have to be checked.

**Fewer steps.** 73.5 per byte is already down from 254. `PRUNE_LOG` buys more
at a measured cost in compression (§4.3 of ALGORITHM.md). The exact version of
the same idea - the alphabet map - was free, and there may be more exact
constraints available: an order-1 alphabet map (which bytes can follow the
previous byte) would prune far harder, at 8 KB of header rather than 32 bytes,
and is worth trying on a long file.

**What cannot be removed.** The model step itself, 1.4 s of the 6.15 s, is the
floor for this approach: the framework's whole premise is that the model is a
black box whose real step must run. The eighth bit is already free - the walk
reads the pending prediction rather than advancing - and that is worth half the
tree.

## 6. All the numbers

`tangelo_w`, first 64 KB of `book1`, GCC 13 `-O3 -march=native`, best of four.
Every row produced a bit-identical round trip and passed `-DTRACK_VERIFY`.

| | size | time |
| --- | ---: | ---: |
| where this started | 20 739 | 10.20 s |
| test before the step (§3.1) | 20 739 | 8.71 s |
| + alphabet map (§3.2) | 20 687 | 7.02 s |
| + wide journal records (§3.3) | 20 687 | 6.48 s |
| + 8-byte journal cells (§3.5) | 20 687 | 6.15 s |
| + `--inline` (§3.4, opt-in) | 20 687 | 5.87 s |
| paired log table (§4, rejected) | 20 687 | 6.97 s |

Times in that column come from different sessions, and this machine drifts by a
few per cent between them; each step was also measured against the one before it,
interleaved in a single session, which is the number to trust. For §3.5 that was
6.52 s to 6.15 s.

Instruction counts for §3.5, which timing could not resolve. `tangelo_w`, first
4 KB of `book1`, GCC 13 `-O3 -march=x86-64-v3`, `valgrind --tool=cachegrind`:

| | before §3.5 | after §3.5 |
| --- | ---: | ---: |
| instructions | 7 327 250 450 | 6 337 775 432 |
| — `UNDO()` | 2 439 883 133 | 1 788 298 266 |
| — `track4`, the commonest stub | 962 093 310 | 833 814 202 |
| — the model step and its call sites | 2 376 596 776 | 2 376 596 776 |
| data reads | 1 782 413 887 | 1 753 192 075 |
| data writes | 1 544 891 550 | 1 376 178 507 |
| L1 data misses | 14 477 251 | 9 678 329 |

Journal share, by `--repeat`:

| | 1 pass | 2 passes | 3 passes | per pass | non-journal |
| --- | ---: | ---: | ---: | ---: | ---: |
| before §3.3 | 7.02 s | 12.59 s | 17.94 s | 5.46 s | 1.56 s (22 %) |
| after §3.3 | 6.48 s | 11.75 s | 17.00 s | 5.26 s | 1.22 s (19 %) |
| after §3.5 | 6.37 s | 11.18 s | 16.26 s | 4.95 s | 1.42 s (22 %) |

Journal composition, per input byte:

| | before §3.3 | after §3.3 | after §3.5 |
| --- | ---: | ---: | ---: |
| speculative steps | 73.5 | 73.5 | 73.5 |
| cells | 28 554 | 24 091 | 26 323 |
| cells per step | 388.5 | 327.7 | 358.1 |
| traffic | 457 KB | 376 KB | 206 KB |
| 1-byte cells | 5.3 % | 6.2 % | 5.7 % |
| 2-byte cells | 16.6 % | 19.6 % | 18.0 % |
| 4-byte cells | 31.5 % | 55.6 % | 34.2 % |
| wide stores | 46.7 % | 18.6 % | 42.1 % |

The last two columns are not measuring quite the same thing: since §3.5 an
8-byte store is a record rather than two 4-byte cells, so it moved from the
4-byte row into the wide row. That is also why the cell count went up while the
traffic nearly halved.

`fpaq0mw` for comparison, on all of `book1`: 95.5 speculative steps per input
byte, 4.0 journal cells per step, 3 030 bytes of journal traffic per input byte,
and a peak of 24 live cells. Its model step is four stores, so it is almost pure
journal, and it gains most from §3.5: 1.985 s to 1.777 s, and 1.99 G
instructions to 1.85 G.

Reproducing any of it:

```sh
CXXFLAGS="-O3 -march=native" ./build.sh tangelo_w      # the default build
TRACKFLAGS="--repeat=2"      ./build.sh tangelo_w      # journal cost, still correct
TRACKFLAGS="--inline"        ./build.sh tangelo_w      # §3.4
CXXFLAGS="-O3 -march=native -DPRUNE_LOG=0x90000" ./build.sh tangelo_w
```
