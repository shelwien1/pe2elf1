# The second refactoring plan: two names, and code written out

`REFACTOR.md` is the record of the first plan and of the rounds that followed
it. That work was about *structure the decompiler invented* — frames, blobs,
offsets, casts, unions — and it is finished: `shape.py` reports one frame left
and it is declined for a reason about the tool rather than the code, and every
counting tool in `tools/` is at zero.

This plan is about something else, and it is the part the decompiler did **not**
invent. What is left is the shape of a 1997 MSVC build seen through Hex-Rays:
one value under two names because a register was spilled, a loop written out
five times because the scheduler interleaved it, and a block written out four
times because it was inlined. None of that is a defect — it all compiles to the
right bytes — and none of it says anything a reader wants to know.

**The rule for this round: one name per value, one copy per idea, and let the
compiler do the unrolling.** Everything below is measured; the counts are what
the tree answers today, and each phase says what would show the measurement was
wrong.

---

## The gate does not change

`./build.sh && ./test.sh` — 110 checks over 17 images, byte-identical
compressed streams against references taken from BMF 2.01 itself. Then
`tools/x32.sh` (23 of 23 at the other pointer width), `tools/asan.sh` (44 runs),
`tools/hdrscan.sh` (8,704 malformed headers), `tools/fuzz.sh` (400 mutants),
`BMF_WARN=1 ./build.sh` (0), and `tools/sweep.sh` (every tool at zero).

Two things the last rounds learned that this one must not relearn:

* **the streams are a narrow claim.** `search_filter`'s frame lift was
  byte-identical on all seventeen images *and* a stack-buffer-overflow; ASan is
  what said so. Any phase below that changes the extent of an object gets ASan,
  not just `test.sh`.
* **a gate that cannot see the change is not a gate.** `choose_plane_coding`
  reads an array index off a stack address, and all nineteen images produce
  identical streams whether it is 0 or 4. When a transformation touches
  something no image distinguishes, the plan says so rather than claiming the
  green run as evidence.

---

## What is already at zero, so this plan does not re-propose it

`tools/sweep.sh` runs 86 tools against the spliced unit and every counting one
answers zero. In particular `uncopy.py` (a local that is only a copy of
another), `unaliasvar.py`, `unsave.py` (a save across a region that cannot
change the value), `unrec.py` (unrolled record copies), `undup.py` (identical
`if`/`else` arms), `untail.py` (shared tails) and `unhoist.py` are all at zero.

**Those zeros are honest and they are narrow.** Each tool implements one precise
rule; what follows is three classes that fall outside all of them, and the first
thing each phase needs is a tool that can report before its zero means anything.
That has been the recurring lesson of this project and it applies here first:
`uncopy.py` reads zero on 534 sites of aliasing because its rule is *a local
assigned exactly once, from another local, at the same brace depth*.

---

## Phase 1 — one name per value (534 sites measured)

The largest class, and the one to do first because it makes the other two
readable.

A throwaway pass over the spliced unit counts every `a = b;` where both names
are still read afterwards:

| shape | sites |
| --- | --- |
| a local that is a copy of a **parameter** | 47 |
| a local that is a copy of a **field** that never changes under it | 82 |
| a local that is a copy of **another live local** | 405 |

By body: `choose_plane_coding` 47, `search_filter` 43, `code_pixel` 41,
`write_bmp` 39, `decode_pixel` 38, `reduce_alphabet` 31, `unmodel_plane_slow`
28, `alt_p2_context` 23.

### 1a. The spill-back before a jump

`write_bmp`'s RLE encoder is the clearest specimen. Four values are each under
two names, and the second name exists only to survive a `goto` out of the loop
nest:

```c
out_at = buf_2;
row_i = row_i2;
rows3 = img->height;
stride3 = img->stride;
goto write_rows;
```

That block appears at every exit of the nest. `buf_2` is the register, `out_at`
the stack slot; `row_i2` and `row_i` likewise. And `rows3`/`stride3` are not
even a pair — they are *reloads of two fields that nothing writes*, so the whole
line is a no-op restated four times.

Collapsing each pair to one name removes four locals and about sixteen
assignments from that one function.

**The tool.** `unspillpair.py`: for each `x = y;` where both are plain locals,
check that between the assignment and every subsequent read of `x` nothing
writes `y`, and vice versa. Where that holds, they are one variable. This is
`uncopy.py`'s rule with the single-assignment requirement dropped and a
liveness walk put in its place, which is why it is a new tool and not a flag on
the old one.

**The hazard, and it is real.** A pair that looks like a spill can be two
variables that happen to agree on the path you read. The last round hit exactly
this: renaming `x0[2]`, `x0[3]` and `x1[0]` to one name each in
`choose_plane_coding` gave *one name to two roles*, every gate passed because
the storage was the same, and `uncopy.py` caught it afterwards by reporting
`quad_r is wt8_up_end_cur`. So: the tool proposes, the gate decides, one pair at
a time, and the sweep runs after each.

### 1b. The field re-read under a local name

82 sites. `bpp = img_at->depth`, `want2 = hdr.data_size`, `row_bytes =
p_i->stride`, `rows3 = img->height`. Where the field cannot change between the
copy and the reads, the local is a second name for it.

Where it *can* change, the local is load-bearing and the copy is the point —
`search_filter` calls `transpose_image`, which swaps `width` and `height` under
it. The tool must decline any body that calls anything between the copy and the
read, and say so; a decline here is a finding, not a gap.

### 1c. The expression written twice

84 sites assign an expression to a name and then repeat the expression on the
very next line:

```c
bin = &grp[1];
p2_freq_add(&grp[1], is_dec, 4);
```

`alt_p2_model` has this shape a dozen times over. The second write is what the
register allocator emitted; the name is what a reader has. Pass the name.

And 69 member expressions are written five or more times inside one body:
`src2->buf[0]` 14×, `blk->row_cur[5]` 11× in `model_plane` and 10× in
`unmodel_plane_slow`, `blk->width` 12×, `hdr.data_size` 9×. Hoisting these to a named local
is the opposite of what `unhoist.py` does, and that tension is deliberate:
`unhoist` puts back a load the compiler hoisted *into the middle of an
expression*; this is about a chain repeated across twenty lines. The rule that
separates them is whether the repetition spans statements.

**Expected outcome:** 400–500 of the 534, with the rest declined and the reason
recorded. If it comes out under 200, the shapes above are not the common case
and this phase was mis-scoped — say so in the table and move on.

---

## Phase 2 — let the compiler unroll (89 runs measured)

A pass looking for three or more consecutive lines with the same shape and
different integers finds 89 runs. Not all are loops: table initialisers and
declaration lists have the same silhouette and must be left alone. The ones that
are loops:

### 2a. The interleaved initialiser, 15 lines

`AltP2Block`'s constructor ends with fifteen consecutive assignments:

```c
ctx_w[4].w[0] = 0;    ctx_w[0].w[1] = 64;
ctx_w[3].w[0] = 0;    ctx_w[0].w[2] = 128;
ctx_w[2].w[0] = 0;    ctx_w[1].w[1] = 192;
…
```

Three independent sequences, interleaved by the scheduler. Sorted, they are
`w[0] = 0`, `w[1] = 64·3ᵏ` and `w[2] = 2·w[1]` for k = 0..4 — three lines of
loop with the formula visible, in place of fifteen in which it is not.
`AltP1Block` has the same thing at **27 assignments**, k = 0..8, base 32, with
an unrelated declaration interleaved into the middle of it.

**Order does not matter here** — the stores are to distinct locations — which is
exactly what makes it safe, and exactly what a tool must check before it rolls
anything. Of the 89 runs, 78 are already in index order and 11 are in scheduler
order; these two are the largest of the 11, and being out of order is a decent
first filter for "this was a loop".

### 2b. The row shift, where order does matter

```c
sym_cache[5] = sym_cache[4];
sym_cache[4] = sym_cache[3];
sym_cache[3] = sym_cache[2];
sym_cache[2] = sym_cache[1];
sym_cache[1] = sym_cache[0];
```

Five lines that are one loop — but a loop that must run in this direction,
because the ranges overlap. Same in `here[0] = here[-1]` (×5, in three files),
`rec4[-1] = rec4[0]` (×8) and `here[-2] = up[1]` (×7).

**The tool.** `reroll.py`: take a maximal run of same-shape lines, extract the
integer that varies, refuse the run unless it is an arithmetic progression, and
then check whether any destination of one line is a source of a later one. If
none is, emit a `for` in either direction; if some are, emit a `for` in the
direction the run was written and say in a comment that the direction is
load-bearing. Refuse anything else.

**What this is not.** It is not a performance change and must not be argued as
one. GCC unrolls small counted loops at `-O2` if it wants to; whether it does is
the compiler's business. The claim here is only that five lines saying "shift
the array down" read better as one line saying that. If a re-rolled loop moves a
stream, the transformation was wrong about the order and the gate has done its
job.

### 2c. The out-of-order table

```c
row[1] = 24*alt_freq_limit;   row[2] = 205;   row[6] = 48;
row[3] = 124;                 row[7] = 16;    row[4] = 147;
row[5] = 83;                  row[8] = 8;     row[9] = 4;
*row = 635;
```

Ten stores to ten distinct slots in scheduler order, with `row[0]` last and
spelled `*row`. Not a loop — the values are unrelated — but an initialiser list
in index order says the same thing and can be read against the table it seeds.
This is the third of the eleven scheduler-order runs, and the only one of the
three that is a table rather than a loop; `init_counter_node` next door writes
the same kind of table *in* order and reads fine as it stands, which is the
comparison that makes the point.

**Expected outcome:** perhaps 20 of the 89 runs are genuine loops — the row
shifts, the two `ctx_w` blocks, the `sym_cache` shift and their siblings — and
the rest are tables, declaration runs and `CtxIdx` bit lists that are already
in the clearest order they have. If a first pass proposes more than 40, the
rule is matching declarations and needs narrowing before anything is applied.

---

## Phase 3 — one copy per idea (16 duplicated blocks measured)

A pass for blocks of six or more identical non-comment lines appearing more than
once finds sixteen, and two are large:

| lines | copies | where |
| --- | --- | --- |
| 60 | 2 | `alt_model_p2_encode` / `alt_model_p2_decode` |
| 21 | 2 | `interleave_plane` / `colour_transform` |
| 14 | 2 | `alt_model_p2_encode` / `alt_model_p2_decode` |
| 11 | 4 | `alt_p2_model`, four times in one body |
| 8 | 3 | `alt_p2.inc` / `alt_p2_encode.inc` / `alt_p2_decode.inc` |
| 7 | 4 | `alt_p2_model`, the same four places |
| 7 | 2 | `model.inc` / `model_plane.inc` (twice) |

### 3a. The sixty lines

`alt_model_p2_encode` and `alt_model_p2_decode` contain sixty identical lines
that roll one plane's row buffers forward: swap `row0` and `row1`, step both,
copy the seam, clear the p2 row, zero four biases. That is a method on
`AltP2Block` and both callers want it. It is the same shape as the two methods
`decode_symbol_list` turned out to be carrying — `promote` and `rescale` — and
those came out cleanly.

### 3b. The fourfold group

`alt_p2_model` contains two blocks, of 7 and 11 lines, each written four times
at 36-line intervals — the four selector groups, unrolled. Whether this rolls
into a loop or factors into a helper depends on what varies between the four,
which is a reading job and the first thing this phase does.

### 3c. The encode/decode pairs

Three of the sixteen are an encoder and its decoder. This tree already merged
nine such pairs into one `template<int32_t f_DEC>` and declined five, with the
measurement for each decline recorded in a table at the top of `bmf.cpp` and
`tools/pairshare.py` re-measuring it every sweep. **Any pair this phase merges
or declines belongs in that table**, and a decline needs a number, not an
opinion.

---

## Phase 4 — the declaration lists (16 lines, two of them enormous)

`decode_pixel` declares 73 locals on one line. `code_pixel` declares 70.
`search_filter` 37, `unmodel_plane_slow` 27, `alt_p2_model` 23.

`firstuse.py` already moves a local to where it is first assigned and is at
zero — but only for locals whose single assignment it can find. The rest are
assigned in several places and the tool correctly declines them. What is left
after Phase 1 removes the aliases is the real question: a 73-name declaration
whose names all mean something distinct is a different problem from one where
half of them are second names.

**So this phase is ordered last on purpose, and it is the one most likely to be
mostly done by then.** Re-measure before proposing anything.

---

## Phase 5 — the last frame

`ReduceAlphabetFrame` is the only frame left. It is at file scope, so
`liftframe.py` declines it for a reason about the tool rather than about the
frame; the gate failure it carries — the lifted build aborts on DLRAW while
compressing — is recorded in the file's own header comment, `PROVEN` having
been emptied.

That comment names the reason too: `memset(buf, 0, 0x10000)` clears
`kids[127][4]` and 64KB past it, which is why `_pad0[64504]` is there. It is the
same class of finding as the five frames that dissolved this round — a `memset`
or an `fread` whose *length* says what the object is — and the resolution is
likely the same: name the 64KB thing the clear is actually clearing, and the pad
goes with it.

It also carries a twelve-slot union of numbered spill slots, `slot0`..`slot11`,
of which `slot7` is read fifteen times and `slot9` twelve. Those are Phase 1's
class, one file over.

**This phase is speculative and says so.** The other four are measured; this one
is a hypothesis with a recorded gate failure standing against it. Do it last, do
it with ASan, and if it fails again put the failure back into `PROVEN` with
today's measurement. That table is empty now, which is a fact about the tree and
not a target: the right number of entries is however many frames have been tried
and reverted, and an empty table stops being honest the moment something is
tried and not written down.

---

## What this plan declines

* **Renaming for taste.** `p3`, `p6`, `p10` are gone from the bodies this round
  touched because the variables were. Where a cryptic name still stands for a
  distinct value, renaming it is a separate judgement and not this plan's.
* **Splitting the four 500-line bodies.** `decode_pixel`, `code_pixel`,
  `choose_plane_coding` and `alt_p2_context` are long because the algorithm is,
  and `methodise.py` has declined the extractions it can see with reasons in its
  table. Phases 1–3 will shorten all four; whether what is left should be split
  is a question to ask *after* the measurement, not before.
* **Anything justified by speed.** This tree's reference streams are the only
  thing that matters and timing is not a gate here. Where a phase's motivation
  sounds like performance — "let the compiler unroll it" — the actual claim is
  about what the source says, and the document should not pretend otherwise.
* **`-O3`, `-ffast-math`, `-march=native`.** All three move streams and
  `build.sh`'s header records which and why. Not on the table.

---

## How to know it worked

Each phase adds a tool to `tools/`, so the sweep picks it up and the claim
becomes standing rather than a one-off. Before any tool's zero is trusted, it
gets the treatment the last four rounds established: **put a defect back and
watch it report.** `flagnames.py` was half a rule when it was written — its
name pattern needed a character in front of the word it was looking for, so it
found one of the two sites put back to prove it — and that was caught only
because the proof was run.

The numbers to re-take at the end, and to write into this document beside the
ones above:

```
aliasing sites          534   →  467   (see below -- not the same scan)
unrolled runs            89   →    0   (3 declined as tables, with reasons)
duplicated blocks        16   →    0
declaration lines 7+     16   →   11
frames                    1   →    0
bodies over 200 lines    16   →   11
lines, spliced       11,321   → 10,858
```

A plan kept without its outcome is a plan that gets re-proposed. This one has a
column for the outcome and it is expected that several of these numbers will
move less than the phase predicted — that is the useful part.

---

## What happened

All five phases ran. Four of the seven numbers went to zero; one of the other
three is not the measurement it looks like.

**The aliasing number is the weak one and the reason is worth stating.** 534
came from a one-off scan written while this plan was, and re-taking it meant
writing that scan again — the buckets do not agree (47/82/405 then, 23/96/348
now), so 467 is not 534 measured twice. What can be trusted is the part that
became a tool: `passname.py`, `unspillpair.py`, `unaliasvar.py` and `uncopy.py`
each report zero on every sweep, and each was proved able to report before its
zero was believed. **A number that only a tool in `tools/` can re-take is the
only kind this document should have quoted.**

**Phase 2** found 39 unrolled loops rather than the ~20 predicted, and three
runs stayed as tables — `init_counter_node`, the p2 frequency seed, and
`update_model`'s neighbour list — because their values are unrelated numbers
and a `for` would have to carry them in an array anyway. `reroll.py` was wrong
three times before it was right, and all three were caught by a dry run into a
scratch tree rather than by the gate: a counter named `i` shadowing a row index
also named `i`, a dependence check that looked only for read-after-write when
the row shifts are write-after-read, and a descending run being filed as
"scheduler order" when it is index order backwards.

**Phase 3** went to zero, and the encode/decode pairs moved with it: the p2
pair fell from 123 shared lines of 586 to 39 of 385 and the p1 pair from 8 of
60 to 3 of 50. Every block that came out was a named thing that had no name —
`start_row`, `plane_transform`, `transform_cost`, `save_descriptors`. The
decline of those pairs is stronger than it was, which is the outcome the
phase's own text predicted and the reason it insisted the table be re-taken.

**Phase 4 was the surprise.** It was ordered last on the theory that it was
mostly a consequence of Phase 1, and it was not: it moved 478 declarations, and
not one line of it was a new rule. `firstuse.py` already had the rule and was
reporting zero because three of its guards were wider than what they guarded
against. Two of the three were caught by g++ and not by the gate —
`-fpermissive` turns "jump into scope past an initialiser" into a warning, so
the build stays green and the variable is uninitialised on that path. **A tool
reporting zero is a claim about the tool as much as about the tree**, and this
round that claim was wrong three times in one file.

**Phase 5 was speculative and the speculation held.** The hypothesis was that
the 64KB `_pad0` and the `memset(buf, 0, 0x10000)` beside it were saying what
the object is, and they were: `buf[4]` and `kids[127][4]` interleave — `kids`
begins four bytes after `buf` and both stride by eight — so they are one array
of 8192 eight-byte records, which is exactly the 65536 bytes the `memset`
clears. With the array named, the twelve numbered spill slots around it were
ordinary locals and the frame dissolved. `PROVEN` stays empty and the failure
that stood against this phase is gone, because the reason for it was.

Gated at every step: 110 checks over 17 images, ASan clean over 44 runs, x32
23 of 23, 400 fuzz mutants with nothing reported, the header scan, `BMF_WARN`
at zero, and the sweep at zero.

---

## The round after: what asking the same question differently found

The plan above ended with every tool in `tools/` at zero, which is where a
round is supposed to end. The question that reopened it was not a new class —
it was **phase 3's question with the identifiers normalised away**:

```
duplicated blocks, identical text        0
duplicated runs, modulo renaming       108   →    85
lines that are a copy                 1031   →   480
lines, spliced                      11,321   → 10,357
```

A decompiler names a temporary after the register it landed in, so two copies
of one idea are almost never spelled the same way twice, and phase 3's scan —
six or more *identical* lines — could not see any of it. `tools/dupblock.py`
asks with the names taken out and holds the answer as a ratchet, because this
class cannot reach zero: an encoder and its decoder share scaffolding whatever
you do to them.

Fifteen things came out of it, and each was a named idea that had no name:
`AltP2Block::start_row`, `seed_history`, `seed_row0`, `encode_sample`,
`decode_sample`, `nlms_step`; `ModelBlock::start_row`, `load_neighbours`,
`init_symbol_lists`, `load_selectors`, `match_seed`; `FreqRec::resum`,
`blend_from`, `find_level`, `bump`; `AltP1Block::seed_activity`,
`record_sample`; `weight_pair_cost`, `residual_bin`, `over_thresholds`,
`plane_transform`, `transform_cost`, `save_descriptors`. `code_pixel` and
`decode_pixel` lost 40% of their length between them; `alt_p2_model` went from
326 lines to 184; `alt_model_p2_encode` from 217 to 91 and its decoder from 166
to 62.

**Where it stops.** The largest groups left are the p2 coders' prologue and
epilogue — allocate the planes, read the reference flags, begin the coder; end
it, restore the coefficients, free the planes. That is the pair `pairshare.py`
owns, measured and declined at every size it has been: 123 shared lines of 586
when its table was written, 34 of 153 now. Taking those out means merging the
pair, which is a decision with a number attached and not a duplication to sweep
up. The share went *up* while both halves shrank, which is the number behaving
correctly rather than the decline weakening.

**Three of the gates were counting the wrong thing, and all three had been
green for rounds.** That is the more useful finding:

* `sweep.sh` tested for a zero *anywhere* on a tool's line, so `pairshare.py`
  answering "1 of 5 declined pairs have drifted, 0 named but not found" passed
  on the `0` eleven words later.
* `BMF_WARN=1 ./build.sh` printed the count of lines matching `': warning: '`,
  and an error is not a warning — so a build that did not compile answered `0`.
* `proven.sh`, the meta-gate whose whole job is asking whether these zeros can
  move at all, replayed `subs1.hpp`. The tree stopped being one file a long
  time ago; every run has ended on `no history for subs1.hpp` and exit 2.

All three are fixed and each was proved able to fail on a probe copy. The
lesson is the one the tools directory was already built on and had stopped
applying to itself: **a zero is a claim about the instrument as much as about
the tree.**

---

## The round after that: the sweep was asking 138 of 219 bodies

The previous section ended on "a zero is a claim about the instrument". This
round is what happens when that is taken as a work item rather than a moral.

**What surfaced it.** Four locals in the tree were `this` under another name —
`this_1` and `blk` in `unmodel_plane_slow`, `blk1` and `blk4` in
`reduce_alphabet` — with fifty-nine dereferences between them. Two tools exist
for exactly that: `uncopy.py` ("a local that is only ever a copy of another")
and `unaliasvar.py`. Both were green on all four, and had been for every round
since the methods were written.

**Three defects, each proved on a probe before it was fixed.**

* `structs.bodies` opened a body only at brace depth 0, which was the whole
  file when the whole file was free functions. Measured on the spliced unit its
  answer was **131 bodies where `defs` finds 219** — every one of the 81
  in-class methods invisible — and **43 of the 103 tools read the program
  through it**. Their zeros were zeros about 138 of 219 bodies, and nothing in
  the sweep could say so, because a tool that is never shown a body reports the
  same number as a tool that looks and finds nothing.
* `uncopy.py` required the copy's source to be a name the *body* declares. A
  parameter is declared in the signature and `this` is declared nowhere, so
  `blk1 = blk` and `this_1 = this` failed `src not in ty` and were skipped
  without a word.
* `uncopy.py` also required the copy to be its own statement. Hex-Rays declares
  at the top and assigns lower down; a hand-written `ModelBlock *blk1 = blk;`
  is a declaration, and declarations are filtered out of the use list, so its
  own line was never `uses[0]`.

**Fixing the second and third broke the first probe**, which is why probes are
kept rather than run once: the new declaration pattern let `blkA = blk;` parse
as a declaration of `A` with type `blk`, and the plain-assignment form that had
worked for rounds went to zero. The separator between the two identifiers has
to be there. All four probes report now.

**What the fixed instrument then found in a tree that had just gone green:**

| tool | answer |
| --- | --- |
| `compact_locals.py` | 2 declaration lines mergeable, in a method |
| `uncopy.py` | 6 locals that are only a copy of another, 40 reads |
| `methodise.py` | 2 functions whose first parameter is only ever a receiver |

The six copies were `dst_keep`/`dst`, `owner`/`syms`, `sym_high`/`cum_lo`,
`p2_rec`/`frec_step`, and `arc`/`arc_in` twice — 25 reads in `expand_image`
alone. Folding them is what turned `decode_symbol_list` into
`SymList::decode_symbol_list`, which in turn showed three locals shadowing the
members they copy and one total, `cum + tot`, living in the body under two
names. Each fix made the next one nameable; none of them was visible from the
line the round started on.

`compress_image` was the second `methodise` candidate and is declined, with the
measurement recorded in the tool: its inverse `expand_image` uses `arc_in` for
eight things that are not member accesses, and a pair this project keeps
matched does not become a method on one side.

`unspillpair.py` had the same wrapped-declaration defect `unreload.types`
records fixing and names three tools for, so this was a fourth — and it did not
fail silently, which is worse: it declined every pair among `code_banks`'
twenty-eight `int32_t` as "not a local of this body", a sentence that reads
like a judgement and was a parse. Joined, the two real pairs there decline for
a reason instead (`a read of res_s precedes every copy into it`). Four more
declines were `x = nullptr;`, which is not one name copied into another at all.
54 remain and every one of them is a member store, a global, a constant or a
reference parameter — the class is at its floor, and the floor is now made of
judgements.

One more, found by running the fixed tool's *apply* path on the probes rather
than only its report: `uncopy.py --all` folded a chain — `blkA = blk;
blkB = blkA;` — by applying the two folds independently, which deleted both
declarations and left `blkA` in the text with nothing declaring it. Each `dst`
is assigned exactly once, so following `dst -> src` to its root terminates;
that is what it does now. The report had been right about the chain all along,
which is why only running the transform caught it.

---

## The gotos, asked with a working denominator

`shape.py` counts 42 jumps and `degoto.py` reported **"0 candidates of 0
gotos"**. The denominator was the giveaway: it looked for `goto LABEL_n;` in
five places, the spelling Hex-Rays emits and the spelling every jump in this
tree stopped using rounds ago. `shape.py` records fixing exactly this in its own
jump rows — "a row that can only ever count something the tree no longer has is
a row that reports the rename rather than the jumps" — and `degoto.py` never
got the same fix. Its zero was a zero about zero jumps.

Pointed at the labels the tree has, it declines 26 with reasons: 17 where the
`goto` is not the whole of an `if`, 6 where more than one `goto` reaches the
label, 3 backward. One of the 26 was `keep_flag8`, declined as "no label of
that name" while the label sat eight lines below the jump — the pattern
required the colon to end the line and that one carries a statement. A decline
whose reason is that the tool could not find something the file has is not a
decline.

**A third shape, which neither of the other two can see.** Both existing rules
need the label to have the jump as its only predecessor, because both *move*
the region the jump skips. A `goto` whose target is the next statement moves
nothing, so how many other jumps reach the label cannot matter:

```
if ( c ) { S; goto L; }        if ( c ) { S; }
L:                        ->   L:
```

`next_list` in `unmodel_plane_slow` had five predecessors and one of its five
jumps was this. It is gone.

**Then the loop it was in.** Four arms wrote a decoded row out at four widths,
reached by `goto list_ready` out of the row loop and four `goto next_list` back
into it. They are one `write_row` now, and *both* places that write a row are
the same call — the second reached only by the `break` a zero-width image
takes, where every arm's `row_w > 0` guard is false and the arm `nbytes`
selects writes nothing either way. **42 jumps to 37, 27 labels to 26**, and the
ratchet from 373 to 369.

**And the 37 that are left are not `break`s or `continue`s.** `degoto.py`
declines them per *label*, which is the right unit for a rewrite that moves a
region but the wrong one for asking whether a single jump has a structured
name — `next_opcode`'s five were declined together by an earlier round, on the
measurement that substituting `continue` for all five "compiles, passes the
corpus, and sends two of the five to the wrong loop", and that says nothing
about the other three. Asked per jump — is this label the first statement of
the immediately enclosing loop (`continue`), or the first thing after its
closing brace (`break`)? — the answer over all 37 is zero. The measurement was
run against a two-function probe containing one of each first, and named both;
the zero on the tree is a zero the instrument could have broken.

---

## Nesting, and a detector that was wrong before it was right

Nothing measures how deep a body nests. Measured: 125 of the 219 bodies reach
depth 1, and three reach 9 — `choose_plane_coding`, `reduce_alphabet`,
`init_tables`.

The cheap part of that is `if (a) { if (b) { … } }` where the inner is the
whole of the outer, which is `if (a && b)` and one level less. **The first
detector for it reported five and was wrong about four of them**: it found the
inner `if`'s closing brace by running the depth back to zero, which for
`if (b) { X } else { Y }` lands on the *final* brace and reads as no `else` at
all. An inner `else` blocks the merge outright — `if (a) { if (b) X else Y }`
does nothing when `a` is false, and `if (a && b) X else Y` runs `Y`. Two of the
five survive the corrected question and both are done:
`plane_predictor != pred_p2 && plane_predictor == pred_p1` is the second test
alone, and the weight search's two descent bounds are one `&&`.
`choose_plane_coding` is no longer one of the three at depth 9.

The pattern of the whole session, one more time: the finding was not in the
tree, it was in the question — and the first version of the question was wrong
in a way only reading its answers could show.

---

## The two things the round had left, and the third under them

The round ended by naming what it had not done: two bodies at nesting depth 9,
and three encode/decode pairs declined with numbers. Both were work, not a
floor.

**`init_tables`, depth 9.** A move-to-front cache over eight entries, written
as an eight-deep `if`/`else` nest with the slide unrolled into every arm — a
linear search with its loop written out and its counter spelled as the depth of
the nesting. Seven arms named the entry they had just read (`c0`…`c6`), and
`cache` was a second name for `sym_cache` used interchangeably between the two
halves of the same arm. Only slots 1..6 are searched, which is the original's
reach: a miss therefore lands on 7, and the deepest arm's `cache[7] = c6` is
exactly the store the slide makes for `k == 7`, so not-found is
found-at-the-end and needs no arm of its own. 34 lines out.

**`reduce_alphabet`, depth 9.** Two `while( 1 )`s, a `goto` out of both and a
`break` landing in the same place — and the shape of it was an unrolling by
*two*: an inner pair loop over `n_kids >> 1`, then one extra component for an
odd count. Which loop carried the turn counter depended on the parity, so the
two increments are one increment and the two loops are one loop:
`out[c + 2]` takes `rp[c]`, `n_kids` bytes a turn, `plane_size` turns. **No
body in the tree is at depth 9 now**, and 36 jumps remain of the 42 this
document started with.

**The pairs, and what was actually under them.** The declined table's own
argument is that the shared part is scaffolding, and that naming it leaves less
behind rather than more. Read that way, the p2 pair's 34 shared lines are a
prologue and an epilogue that are *already* nothing but named calls —
`p2_coef.fold()`, `alt_p2_planes_alloc`, three `ref_transformed`,
`rc_begin_encode`/`decode` — and the one thing left to name would be the call
sequence itself, which would hide the single line that differs. That is the
decline standing up under a closer look, not a new one.

What the look found instead: `AltP2Block *blk2 = (AltP2Block*)(plane[2])`,
eight times across the two coders, where `plane` is already `AltP2Block*[4]`.
Five cast tools reported zero on them, and behind that were **two more defects
in shared machinery**:

* `structs.decl_types` began its scan on the body's *opening-brace* line, and a
  signature reads as a declaration — `int32_t f(BmfImage* p, uint8_t* q) {`
  starts with a type and a name. So it opened there, ran to the next `;`, and
  swallowed the first real declaration into a join that parses as nothing.
  **57 names in 43 bodies had no type at all**, every one of them the first
  local its function declares, and about twenty tools read the program through
  this table. `samecast.py` skips a name it cannot type, which is why `plane`
  was invisible.
* `samecast.py`'s pattern then required a bare name after the cast, and all
  eight of these parenthesise their operand.

With both fixed it reports 15, and applying its own answer exposed one more
alias underneath — `BmfImage *img = p_i` in `compress_image`, which the cast on
the line above had been hiding from `uncopy.py`.

Three rounds in a row now the finding has been one layer under a green gate,
and each time the thing that surfaced it was a transformation that made the
next defect nameable.

### The type table's hole, one layer down

Fixing `decl_types` did not stop at `samecast.py`. `uncastwidth.py` — which the
sweep exempts from its zero test, because it classifies rather than counts —
went from 19 casts of the form `*(T*)&…` to 7:

* **6 "same"**: `*(int32_t*)&blk1->width` read back through `(uint32_t)`, a
  round trip through a same-width type, six times in `reduce_narrow_alphabet`.
  Its `--apply` could not do these itself and did not pretend to: it types a
  member from the struct declaration, which a lone `.inc` does not contain, so
  against `sym_reduce.inc` it honestly reports them as `untyped` and deletes
  nothing. The unit types them; the file is what gets written. Done by hand.
* **1 "untyped"**: `*(uint32_t*)&blk->height` is `(uint32_t)blk->height`.
* **4 "wide"**: `_pad8[2]`, `depth` and `flags` are four consecutive bytes at
  offset 8 of `BmfImage`, and both copies of the header block moved them as one
  `uint32_t` — then the second recovered `flags` from the top byte of the saved
  word twenty lines later. Four member copies and a read of `p_i->flags` say it
  without the layout.

**And one that stays, because the probe said so.** `hist_scratch` is a
`uint8_t*` indexed by four over 1024 `uint32_t` counters — exactly the shape an
earlier round turned into `int32_t hists[8*1024]` in `plane_choose.inc`. It is
not the same answer here. An assert on the base's alignment in `packer_reset`
**fires**, on the `len1` malformed-decode case in the corpus; the same assert
moved to the three sites that actually index the counters survives 110 checks,
400 fuzz mutants and 8,704 header variations. So the counters are only ever
reached through an aligned base, but a `uint32_t*` would be a misaligned
pointer the moment `packer_reset` formed it, dereferenced or not. The storage
stays as the binary has it and the access gets the name — `hist_count(k)` — with
the measurement written where the decision is.

The remaining 7 are the BMP palette writes in `bmp_write.inc`, which are
serialisation into a byte buffer at computed offsets, and that one scratch
access.

---

## Declining is not finishing

Everything the last two rounds left as "declined with a measurement" was work.
Taken as work, all of it moved.

**The punning went to zero rather than to a reason.** The six BMP palette
writes were `*(uint32_t*)&out_buf[4*slot+54]` — a BMP's palette starts at byte
54, so no entry in it is four-aligned and none of those could ever be a
`uint32_t*` store. `put_u32` is a `memcpy`: the same store with the alignment
told truthfully. `hist_scratch`'s counter is the same, a `memcpy` pair instead
of a `uint32_t&`, which needs no claim about the base at all — so the careful
alignment argument recorded above is now a note on why the storage is a byte
array rather than a licence for a cast. **`uncastwidth.py` reports 0 of 19.**

**`alt_model_p2_encode`/`decode` merged.** It was the largest declined pair at
34 shared of 153 and the decline rested on "what they share is scaffolding".
Read line by line that was not true: outside the four coding calls they are the
same body. Eight of the differences were spellings — a bias loop counting up in
one and down in the other over four slots it sets to zero, a `for` against a
`do`/`while`, two register spills of the row index saved across a loop that
does not touch it — and one is a deliberate near-miss the merge keeps, the
decoder's `out[3]` where the encoder uses `plane_desc[3].src_plane`. That is
the only `f_DEC` in the body that is not about direction. **153 lines to 6.**

The instrumentation for it is worth recording because the first reading was
wrong: `./test.sh 2>&1 | grep -c` said *zero* entries for both instantiations,
which would have meant the 110 checks gated nothing. `test.sh` captures each
run's stderr. Asked per image, the encoder runs over six of the seventeen and
the decoder over two — so both halves are entered, which is what bmf.cpp's note
requires and what the grep would have hidden.

**`model_plane`/`unmodel_plane` fell from 14 of 170 to 4 of 146** the same way:
what the two shared was the sixteen-line dispatch over the descriptor's model
and depth, and that is `alt_model_plane<f_DEC>`. What is left is the main model
— 100 lines written out in the encoder against a call to `unmodel_plane_slow`
— which is not a shape a template makes one thing.

**And a declaration nothing defines.** Reading those signatures turned up
`decls.inc` declaring `alt_model_p1_encode(uint16_t* hdr, uint8_t* src)` where
the definition takes `BmfImage*` — two different functions, with the call sites
resolving to the real one and the declared overload never defined at all. A
sweep of every declaration in the file against every definition in the unit
finds exactly that one.

### The pair whose number was measuring the wrong thing

`alt_model_p1_d8_encode`/`decode` was the *smallest* line of the declined
table, 3 shared of 34, and that number was not about the two coders. The encode
half was a method — `AltP1Block::d8_encode_body` — and the decode half was
written out inside the wrapper in `alt_p1.inc`. So what `pairshare.py` compared
was a four-line wrapper against a twenty-six-line one, and the twenty-two lines
they really share were on the wrong side of a call. A small share can mean the
bodies differ or it can mean one of them is not where you are looking, and the
tool cannot tell those apart.

Naming the decoder's body the way the encoder's was already named made them
comparable, and then they are the same skeleton with three lines in the middle
differing: `AltP1Block::d8_body<f_DEC>`, the mirror of
`AltP2Block::alt_p2_d8_body` that was merged rounds ago. The decoder walked its
rows with a signed comparison where the encoder cast both sides to `uint32_t`;
`height` is `int32_t` and the guard above has already established it is
positive, so the two readings agree.

Instrumented over the corpus: 70 encode entries and 5 decode. **Eleven merged
pairs now, three declined** — `code_pixel`/`decode_pixel` at 94 of 653,
`predict_med`/`unpredict_med` at 10 of 122, and `model_plane`/`unmodel_plane`
at 4 of 146.
