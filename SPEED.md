# Making the walk faster

[ALGORITHM.md](ALGORITHM.md) describes what this repository does: it turns a
model that predicts one **bit** at a time into a probability for every one of
the 256 possible next **bytes**, by running the model's compiled step
speculatively down the prefix tree and rolling its memory writes back through a
journal. This document is about what that costs, where the cost actually is, and
what can be done about it.

The short version: **about four fifths of the time is the journal, not the
model.** Four changes came out of that, worth 40 % together on `tangelo_w`, and
one plausible-looking idea turned out to be worth nothing at all.

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
effects, so nothing here rests on one timing.

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

**Verifying every step.** Each change here alters hand-written assembly or the
shape of the walk. `-DTRACK_VERIFY=48` (§5.5 of ALGORITHM.md) compares all
360.8 MB of model state before and after each walk and checks the walk's
predicted code length against the model's real one for every byte. Every
optimisation below was kept only after that passed and the compressed output
came out byte for byte identical.

## 2. Where the time goes

Per input byte, on English text, with the default pruning:

| | |
| --- | ---: |
| speculative model steps | 73.5 |
| journal cells | 24 091 |
| journal cells per model step | 327.7 |
| journal traffic | 376 KB |

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

They are now one record: the overwritten bytes copied verbatim, 16 to a cell,
followed by a footer naming the address and the width. A 32-byte store is three
cells instead of eight. `UNDO()` walks backwards, so the footer is written last
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

5 % faster, and 48 % more instrumented assembly. It is off by default for that
reason: the trade was measured on one model, and a bigger model's step is more
likely to care about instruction cache than this one is.

## 4. What did not

**A paired log table.** Every node takes `LOG2(SCALE-p)` and `LOG2(p)` - two
lookups at opposite ends of a 128 KB table, two cache lines. Packing both into
one qword indexed by `p` makes it one 8-byte load from a 32 KB table, which
looks like an obvious win: half the lookups, a quarter of the footprint.

Measured: 7.02 s to 6.97 s. Nothing, inside the noise. The table is hot, the
loads are independent, and there are only 73.5 nodes per byte against 24 091
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
Restoring at the cell's own width, branch and all, is the faster and safer
version.

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

**Compact cells (est. 10 %, more work).** A cell is 16 bytes: an 8-byte pointer,
a 4-byte width tag, 4 bytes of data. Everything journaled lives inside one
object, so the pointer could be a 32-bit offset from a base, with the width in
its spare bits - 8 bytes a cell, half the traffic. The obstacles are real but
small: the stub needs a flag-neutral subtract (`lea` cannot subtract a register,
and `sub` sets flags), and the model's globals would have to live inside the same
arena as the model object, which for `tangelo_w` means moving `y`, `bpos` and
`rnd` into the `Coder`.

**Let the model declare bulk state (est. 10-15 %, invasive).** Half of each
step's writes are the mixer rewriting five rows of 80 weights. Journaling that as
five block copies of 160 bytes, once per `NEST()`, would cost 800 bytes where 200
cells cost 3 200. It needs the model to say "this region is bulk state", which
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

**What cannot be removed.** The model step itself, 1.2 s of the 6.2 s, is the
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
| + `--inline` (§3.4, opt-in) | 20 687 | 6.17 s |
| paired log table (§4, rejected) | 20 687 | 6.97 s |

Journal share, by `--repeat`:

| | 1 pass | 2 passes | 3 passes | per pass | non-journal |
| --- | ---: | ---: | ---: | ---: | ---: |
| before §3.3 | 7.02 s | 12.59 s | 17.94 s | 5.46 s | 1.56 s (22 %) |
| after §3.3 | 6.48 s | 11.75 s | 17.00 s | 5.26 s | 1.22 s (19 %) |

Journal composition, per input byte:

| | before §3.3 | after §3.3 |
| --- | ---: | ---: |
| speculative steps | 73.5 | 73.5 |
| cells | 28 554 | 24 091 |
| cells per step | 388.5 | 327.7 |
| traffic | 457 KB | 376 KB |
| 1-byte cells | 5.3 % | 6.2 % |
| 2-byte cells | 16.6 % | 19.6 % |
| 4-byte cells | 31.5 % | 55.6 % |
| wide stores | 46.7 % | 18.6 % |

Reproducing any of it:

```sh
CXXFLAGS="-O3 -march=native" ./build.sh tangelo_w      # the default build
TRACKFLAGS="--repeat=2"      ./build.sh tangelo_w      # journal cost, still correct
TRACKFLAGS="--inline"        ./build.sh tangelo_w      # §3.4
CXXFLAGS="-O3 -march=native -DPRUNE_LOG=0x90000" ./build.sh tangelo_w
```
