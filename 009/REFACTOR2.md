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

### The same mistake twice, and the tool it broke

`model_plane`/`unmodel_plane` was the last easy line of the declined table and
it came off in two steps. The dispatch over the descriptor's model and depth
went first — sixteen lines apiece, `alt_model_plane<f_DEC>` — taking the pair
from 14 shared of 170 to 4 of 146. What was left read as "the main model, which
the two do differently", and that reading was wrong in exactly the way
`AltP1Block::d8_body` had been wrong an hour earlier: the *decoder's* main model
was a call to `unmodel_plane_slow` and the *encoder's* was 130 lines of `blk->`
written out in `model_plane`. The two bodies really are different and stay two
methods; naming the encoder's the way the decoder's was already named left two
five-line wrappers with one line between them, and `code_plane<f_DEC>` is those.

**Twelve merged pairs now, two declined.** So: a share this table reads as "they
differ" can also mean one of the two is not where the tool is looking, and that
is not a rare accident — it happened twice in one session on the two smallest
rows.

Three things the round broke and had to fix, all of them mine rather than the
binary's:

* the extraction dropped `rc_end_encode()` — the slice ended one line early, and
  22 of 110 checks failed. The stream simply never terminated.
* the encoder's `SymPair* group_ctr = blk->group_ctr[g]` became
  `group_ctr = group_ctr[g]` once `blk->` went away: a local that could carry
  the member's name as a free function cannot as a method. It is `gctr` now,
  which is what the decoder already called it.
* `alt_model_plane`'s p1 and p2 arms were the same ten lines with a `1` against
  a `2`, which `dupblock.py` caught as a *rise*. The four entry points come in
  pairs with one signature each, so the predictor picks which of a pair and the
  depth and direction pick which pair — `(p1 ? f : g)(args)`.

That last one broke `deadcheck.py`, which reads the call graph textually and
had a rule for a function named before `,` or `)` but not before `:`. It
reported all four p1 entry points dead while their p2 twins *on the same lines*
were fine — the giveaway. Taking a function's address is a use; it counts one
now, and a planted never-called body proves the tool can still report.

### The last two, and why a reason beats a share

`predict_med`/`unpredict_med` and `code_pixel`/`decode_pixel` are the two rows
left, and after three pairs came off this table on the strength of "the share
was measuring the wrong thing", each of these needed its reason written out
rather than left as a percentage.

**`predict_med` walks backwards and `unpredict_med` forwards, and that is
structural.** The forward transform must read each pixel's *original* north and
west, so it starts at the bottom-right corner and steps back; the inverse must
read the *reconstructed* ones, so it starts at the top-left and steps forward.
The first row and column follow the walk — taken last by one and first by the
other. A template over a direction flag would have to reverse three loop
headers and two edge cases, which is writing both bodies out again with an `if`
around each line.

What they did share and did not say: the zigzag that turns a signed residual
into a small symbol — 0, -1, 1, -2, 2 … into 0, 1, 2, 3, 4 … — built inline in
each, five lines apiece, one counting up in twos from each end of the table and
the other filling odd and even slots in one pass. `med_fold_table` and
`med_unfold_table` now, and neither is the alt models' `alt_init_tables` pair,
which buckets by `near_lossless_q` and is a different table with the same job.

**`code_pixel` and `decode_pixel` diverge at the top and meet at the bottom.**
The encoder knows the symbol and asks where it ranks; the decoder asks the coder
for a target and finds which rank holds it. That is `neighbour_rank` against
`FreqRec::find_level`, and it decides the shape of everything above — the
encoder can test candidates in rank order and stop, the decoder cannot. What
they genuinely share is the context, and that has been coming out by name for
several rounds.

**Where the pairs ended: twelve merged, two declined**, from nine and five when
the round began.

---

## The pair that stays two functions, taken apart anyway

`code_pixel`/`decode_pixel` is one of the two declines left, and the decline
holds — the encoder knows the symbol and asks where it ranks, the decoder asks
for a target and finds which rank holds it, and that decides the shape of
everything above. But a pair that stays two functions is not a pair with
nothing to do. Six things came out of them this round, and each one made the
next visible:

* **`seed_candidates`** — the 32-candidate table. Both filled it and both
  reached the same two members through a local of their own: `pixr` and `wq`
  for `pix_cur`, `cache` for `sym_cache`, and the encoder then spelled the
  first two slots into `w6d` and `wq1` before storing them. **Six names for
  three members.**
* **`offer_candidates<f_DEC>`** — the walk that offers each candidate for one
  context bit. The decoder stores the candidate into the record before reading
  the bit and the encoder reads the record to make the bit; that is the whole
  of the difference, which is why it answers the symbol rather than storing it.
* **`exclude_stage_one`** — five stores of one value, under eight names.
* **`write_row_at<T>`** — three of `write_row`'s four arms are one walk with a
  different cursor type, and only the narrowest spelled the cast the other two
  make implicitly.
* **`intern_ctx`** — a dense id per context signature. Four sites, and every
  one read the table back after storing rather than using the value it had just
  stored.
* **`context_ids`** — and this is the one the others uncovered. Once the
  borrowed candidate slot had a name, the two 25-line blocks that build `sig1`,
  `sig2` and the two ids were the same computation under different names
  throughout: `r8`/`r1` and `r7`/`r2` for `row_cur[8]` and `row_cur[7]`,
  `row`/`cur5` for the row cursor, `up_m0`/`up_match0` for the slot, a
  `match1` one of them named and the other read directly. The decoder also
  named `all_up` and folded it in with a `raw` where the encoder wrote one
  chain — and `CtxIdx` *adds*, so with no overlapping terms the two orders are
  the same sum.

**Three dead reloads fell out of that.** Both coders reloaded the row-6 cursor,
the row-5 cursor and the borrowed slot on an intern miss, and all three are
re-reads of values the intern cannot have moved: `ctx_id1` is a `uint16_t`
table and `row_cur` an array of pointers beside it, so a store to
`ctx_id1[sig1]` reaches neither. What the decompilation shows there is the
compiler reloading across a store it could not prove did not alias. The
seventeen streams agree byte for byte without them.

**The pair went from 653 lines to 539 and 94 shared to 73**; the ratchet from
325 to 293. The percentage is unchanged, which is the point the table has been
making all along: both halves shrink together because what comes out is
scaffolding.

### A claim of mine that needed measuring

Extracting `context_ids` left one obvious next step: `code_pixel` and
`decode_pixel` both write the same four stage-one neighbours into
`mode_symbol[1..4]` *and* into four `nb_sym` slots — 8, 10, 6, 5 in the encoder
and 0, 1, 3, 2 in the decoder — so reading `mode_symbol` and dropping the eight
scratch stores looked like the tidy that would let the last shared block merge.

Enumerating the reads first is what stopped it. `nb_sym` is 32 slots of the
decompilation's stack and `code_pixel` reuses eleven of them for a dozen
unrelated values, among them `nb_sym[5] = run_bucket[nb_sym[4]]` and
`nb_sym[6] = run_hit` — two of the four `neighbour_rank` reads, written a
hundred lines above its last call. So the comment I had written on
`neighbour_rank` earlier in this session — "it is the same four in the same
order everywhere that asks" — was an assertion, and the code around it gave
real reason to doubt it.

Instrumented, checking all four slots against `mode_symbol[1..4]` at every
call: **9,144,286 calls over three images, all four intact in every one.** So
the claim is true, and it is a measurement now rather than a reading of the
nesting. The substitution stays unmade: it would be behaviour-preserving *over
the corpus* rather than by construction, and those are not the same thing.

### The candidate table, borrowed as a dozen locals

`nb_sym` is 32 slots the model fills with the symbols a pixel is coded
against — and `code_pixel` reuses eleven of them for a dozen unrelated values
along the way. `nb_sym[1] = width - x` is how far a run may reach.
`nb_sym[3] = idx1` is its length. `nb_sym[0] = bucket` is the bucket it started
in. `nb_sym[4] = bit5` is a save across a `+= 8` that cannot move it. Reading
the body, every one of those lines looks like the neighbourhood being written,
and none of them is.

Ten now have names — `run_limit`, `run_len`, `bucket_top`, `run_bit`,
`run_hit`, `bucket` — and three of the ten were not values at all: two saves of
`amap` and one of `run_left`, spilled into slots and read straight back across
walks that touch neither, plus a `nb_sym[3] = first` that nothing ever reads.

**Naming them is what let four tools see the rest.** `unsave` found the three
spills, `uncopy` two redundant copies, `firstuse` two locals declarable where
they are first assigned — none of which could be seen while the values were
subscripts. And with the slots named, the two run scans were visibly one loop:
`run_scan` is both, and the two `goto run_copied`s it removes take the jump
count from 36 to 34.

**One of `uncopy`'s findings would have broken the program.** It offered to
fold `bucket_top` onto `bucket`, and `bucket` is decremented *below*
`bucket_top`'s last use — inside the loop, so it runs *before* it. The rule's
span was `code[i+1:last+1]`, the lines in the file, and a backward edge is
exactly what that reading cannot see; folding would have turned "is this the
first turn" into a tautology. The span now runs to the close of the innermost
loop still open at the last use. A four-line probe with that shape is what
caught it, and all four of the file's older probes still report.

The pair is **486 lines and 62 shared**, from 653 and 94 when the round began.

### A reroll that failed, and the read that explains why

The run-fill in `decode_pixel` writes the run's records with the symbol, head
word and flag word of the one before them — unrolled by two, with the odd one
in a tail, a pair count in a slot of the candidate table, and `2*pairs + 1` in
`done` so the tail runs exactly when the count is odd. The two paths together
are one loop of `fill` turns, and rewriting it that way **failed 12 of 110
checks.**

The arithmetic was not the problem; instrumented, `fill = 2*pairs + tail` holds
on all 146 fills in the corpus. Bisecting — first the save/restores alone, then
a single-step loop with the tail left standing — put the fault in what the
rewrite *deleted*: `nb_sym[1] = mode_symbol[1]`. That slot is read again
eighteen lines *below* the block, as `s1d`, to set two match flags. So it was
never scratch for the loop; it was the run's symbol, live past it. Named
`run_sym` and read there too, the reroll passes.

That is the counterpart to the `bucket_top` finding above and the same lesson
from the other side: a value in a borrowed slot has a lifetime the slot does
not advertise, and the only way to know it is to enumerate every read. **The
pair is 456 lines and 54 shared**, from 653 and 94.

**And that is the last of them.** `sig3` in the decoder was a context index
built from slot 1 and stored back into it, where the encoder's half already had
a local of that name; the encoder's `nb_sym[1] = r3->match2345` was a record's
flag word saved and read back twice. With those two named, every write to
`nb_sym` in either pixel coder is a candidate again — the four stage-one
neighbours and the twenty-two spatial ones — and the twelve borrowed uses are
gone.

### The last unnamed offset

`shape.py`'s first row counts raw-offset sites — "a place where the layout of
an object is written as arithmetic instead of declared" — and it had sat at 1
for the whole round: `(BmfImage*)(stream.buf + 16)`, a scratch image placed one
header's worth of bytes into the coded buffer. The 16 *is* `sizeof(BmfImage)`,
and spelled that way beside a `static_assert` it is a declaration.

The tool still counted it, because its pattern is "a cast of `base + N`"
whatever N is. That is the row's own distinction being missed by the row:
`sizeof(T)` is exactly the thing it wants to see instead of a number. It skips
those now, the tree reads **0**, and a one-line probe holding the literal form
proves the row can still report.

---

## Where this leaves it

Every counting tool in `tools/` reports zero against an instrument that was
repaired in nine places this session — `structs.bodies` (131 bodies where there
are 219), `structs.decl_types` (57 untyped names), `uncopy` (parameters,
`this`, declaration-initialisers, chained folds, and a span that could not see
a loop), `unspillpair` (wrapped declarations), `samecast` (parenthesised
operands), `degoto` (a denominator of 0 for 42 jumps), `deadcheck` (a
function's address is a use), and `shape` (the row above).

What is left is declined with a measurement, not with a shrug:

* **two encode/decode pairs**, from five. `predict_med`/`unpredict_med` walk in
  opposite directions because one must read original neighbours and the other
  reconstructed ones. `code_pixel`/`decode_pixel` diverge at the top — rank a
  known symbol against find the rank holding a target — and meet at the bottom;
  they are 455 lines and 55 shared, from 1044 and 123.
* **285 lines of copy**, from 1031, and what is left is those two pairs, two
  mutual inverses in `plane_predict.inc`, and a run of member declarations.
* **34 jumps and 23 labels**, from 42 and 27. `degoto` declines each with a
  reason and none is a plain `break` or `continue` — measured against a probe
  holding one of each.

---

## What "nothing to do" was worth as a claim

The three bullets above were written as the end of the round. Every one of them
moved afterwards, and none of them moved because a new tool was built — they
moved because the things they described were read again.

**"Two mutual inverses in `plane_predict.inc`"** was the second bullet's
throwaway clause. `interleave_plane` and `colour_transform` are an encode/decode
pair that nothing had ever *named* as one, so `pairshare.py` had never measured
them and no decision had ever been taken. A comment above them said the nineteen
lines that read the descriptor "are the whole of what the two share — the loops
under them are inverses and have nothing else in common", and that was a
sentence about spelling: the two blends were written four different ways
between them, one casting to `uint32_t` mid-expression, one summing into an
`int32_t` and casting the total, two splitting the sum across an extra local.
All four are the same thirty-two bits. Written unsigned throughout, each loop is
a blend and a sign, and they are `code_colour_plane<f_DEC>` — the thirteenth
merged pair, and the merge takes a signed overflow out of the forward direction
on the way.

**`code_pixel`/`decode_pixel` at "455 lines and 55 shared"** is 328 and 28. Both
bodies open by reading the same four neighbours — north, west, north-east,
north-west — and both parked them in the candidate array: slots 8, 10, 6, 5 in
one and 0, 1, 3, 2 in the other. Two arbitrary sets of slots for one set of
values. That is the whole reason twenty-eight identical lines did not look
identical, and why a `rank` written against one set could not be called from the
other. The round that wrote `neighbour_rank` had left it alone and said why:
`code_pixel` was borrowing eleven of those slots for unrelated values, two of
them among the four, so the claim had to be settled by instrumenting. The round
after *that* gave the eleven borrowed slots their own names — and nobody went
back to the note. `struct Neighbours` is those four; `open_pixel` is the
twenty-eight lines.

**"34 jumps"** is 27, and 23 labels is 19. `read_bmp`'s RLE4 arm was three
`while( 1 )`s with a label at the top of the innermost and five jumps back to
it, and the nesting *was* the dispatch — fall out of the innermost loop for an
escape, out of the middle for anything but an end of line, out of the outer for
anything but a delta. It is a five-way branch on two bytes now, which is the
shape the RLE8 arm above it already had. `sym_reduce.inc`'s BST insert and
`plane_choose.inc`'s coordinate descent went the same way, and the deepest
nesting in the tree went from 9 to 7.

**"285 lines of copy"** is 227.

And the gates moved too, in the direction that matters least and is worth the
most: `tools/narrow.sh` round-trips fourteen geometries the corpus does not
have — a one-pixel row, a one-pixel column, a single pixel, a packed depth at
width one. It found nothing. It exists because probing `unpredict_med` showed
that one of its four paths could be given a wrong answer with all 110 checks
staying green, and it does **not** close that particular gap either: no image
that narrow ever gets a MED predictor, because `search_filter` gives up below
`4 x 3`. The tool says so at the top rather than implying otherwise by being
green.

The pattern, one more time, is the one this document has been about from the
start: **a measurement is a claim with a date on it.** Nine instrument defects
were found in the round above by asking whether a tool could report. Four claims
were found here by asking whether a sentence was still true. Neither question
has a last answer, and "nothing left to do" is the one answer that is never a
measurement.

---

## The comment that declined a loop

Asking the same question of the *code* comments rather than the documents found
the largest single thing left. `alt_p2_model`'s three counter-bank blocks were
150 lines of `p2_update(direct[k], …)` written out eleven times apiece, under a
comment that said, in bold, that they were "not rolled into loops, and that is a
decision rather than an omission", and listed four exceptions at k = 0, 4, 9 and
10.

Every one of the four had dissolved, and none of them by being argued with:

* "the longhand nudges and mirror bumps" were longhand when the note was
  written. Two had been converted since; the last two went when `p2_nudge`
  learned its shift instead of having it fixed at 2.
* "k = 4 subtracts into the residual variable itself" — it does, and **nothing
  reads the residual afterwards**. A write-back nobody reads is `p2_update`.
* "k = 9 uses `res_t`, a *different* residual" — `res_t = res_c` two lines above
  and `res_c = res_t` two lines below. One residual under a spill's name.
* "k = 10's mirror bump straddles the join" — it does, and both arms reach it
  with `-res_c`, so it is one line after the join.

What the note said settled it was that hoisting would reorder counter updates
against each other, since `direct`, `mirror` and `rot` index one bank and can
name the same record for different k. That is true, and it is why nothing is
hoisted: each loop does the same five things in the same order the unrolled text
did. **The reason was sound and the conclusion had expired.** Three loops, and
the copy residue fell from 203 lines to 132 in one commit.

## `tools/unsave.py`

`magsum_s = magsum;` … `magsum = magsum_s;` — a value saved into a second name
and loaded straight back across a region that touches neither. Four of these
were in the tree and no tool could see them. `unspillpair.py`'s zero was honest:
its rule is that *every* assignment to the shadow must be a copy, so it can
merge the two names, and these slots fail that on their first line. The fix is
smaller than a merge — delete two statements — so the rule can be narrower and
the span can hold a loop, which two of the four do.

Its first version was wrong in the way that matters: it called the save dead
whenever the slot was not read *between* the two statements, and one of the four
loads back into a name the body goes on to use. Deleting that save would have
deleted the value. It counts reads on both sides now.

## The instrument, three times over

Chasing the residue down found three separate things `dupblock.py` was counting
that were not copy, and each of them was the largest entry left when it was
found:

* **a struct's field list.** `ModelBlock` opens with nine consecutive
  `uint32_t <name>;`, which normalise to one window three times over. The tool's
  own first line says it looks for "a run of *statements* written out more than
  once", and a field list is not statements — nothing can be factored out of it.
  Twenty-two lines. A first version of the skip matched one declarator and not
  several, so `double dv0, dv1, dv2;` came back as soon as anything above it
  moved; a second missed `alignas(16) int32_t hist_x[1024];`.
* **a block matching a shifted copy of itself.** Four field zeroings in a row
  put a three-line window at every line of themselves and every window
  normalises alike, so `bmp->biClrImportant = 0;` and its three neighbours were
  "a three-line run written out twice", at two starts three lines apart. That is
  one block. Nineteen lines.

Both are skips, and both were proved narrow before being believed: six identical
declarations beside six identical statements, and one six-line repetitive block
beside the same six lines split across two functions. The declarations and the
single block are not counted; the statements and the split pair still are.

## Three tools, and what each was built because of

`tools/narrow.sh`, `tools/unsave.py` and `tools/deadstore.py` were all written
after a measurement said something was unmeasured, and all three earned their
keep before they were believed:

* **`unsave.py`** finds a value saved into a second name and loaded straight
  back. `unspillpair.py`'s zero was honest — its rule needs *every* assignment
  to the shadow to be a copy, so it can merge the two names, and these slots
  fail that on their first line. Four round trips in the tree, two of them
  spanning a loop. Its first version called the save dead whenever the slot was
  unread *between* the two statements, and one of the four loads back into a
  name the body goes on to use.
* **`deadstore.py`** finds a store overwritten before anything reads it, which
  hides inside a live variable — `unwrite.py` asks about the *name*. **Its
  first eleven findings were all wrong**, for two reasons worth writing down:
  `return result;` is a name, a space and a name, which is a declaration's
  shape, so a live store two lines above a `return` that read it looked dead;
  and a store that *is* the body of a braceless `if` is not unconditional,
  which brace depth cannot see. Eleven candidates, zero survivors, and the rule
  is narrower for it. A tool whose first output is applied rather than checked
  would have deleted eleven live stores.
* **`narrow.sh`** round-trips fourteen geometries the corpus does not have, and
  then a second leg that is the stronger of the two. `testfiles/rle4.bmp` and
  `rle8.bmp` are real files that use encoded runs and end-of-line and nothing
  else, so the absolute-run reader — six exits across two nibble parities — and
  both delta opcodes were reached by no gate at all. Each pair is one opcode
  stream using *every* opcode at both parities beside the same pixels written
  out flat, with the stream decoded by an implementation that is not BMF's, so
  what the two agreeing says is that two readers of one format agree.

## Where this leaves it, again

* **45 lines of copy**, from 1031 and from the 285 that closed the round above.
  Forty-one of the lines that came off were never copy at all — see above.
* **27 jumps and 19 labels**, from 42 and 27.
* **thirteen merged encode/decode pairs**, from twelve; two declined, and both
  smaller — `code_pixel`/`decode_pixel` is 277 lines and 27 shared, from 1044
  and 179.
* **deepest nesting 7**, from 9, in four bodies of two hundred and fifty-six.
* **three new tools**, above.

What is left in the copy residue is twelve runs of three and four lines, and
most of them are now the *result* of naming rather than something to name: two
call sites of one helper look alike, two arms of one search look alike, two
`enum : uint8_t {` blocks look alike. That is the floor this measure has, and
saying so is worth more than driving the number down by writing worse code.

The one decline worth repeating is `estimate_cost`'s two accumulators. Rolling
them into one loop passes all 110 checks over all seventeen images — and
floating-point addition is not associative, so that is a statement about the
corpus and not about the program. It stays two loops, with the number written
down.

## Zero jumps

The round above closed at 27 jumps and 19 labels and called the rest
structural. Eighteen of the twenty-seven were still there when this one
started; none of them is now.

What made the difference was not a new technique. It was reading each one as a
claim about the program and then checking the claim.

* **`filter_search`'s `planes_done`** was reached by `if( nplanes_b <= 2 )
  goto`, sitting inside `if( plane_count > 2 )`, with both arms above it having
  just written `nplanes_b = plane_count`. `plane_count` is written in exactly
  two places in the tree, both at the top of a frame this path does not reach.
  So the test was `plane_count > 2 && plane_count <= 2`. The jump could not be
  taken and the variable was read nowhere else.
* **`alt_p2_model`'s `code_residual`** carried a comment saying a structured
  form needed "a loop around the `if` and a flag to say which way it was
  entered — one more level of nesting and one more variable, to remove one
  `goto` that already says what it does". It needed neither. The residual coder
  runs on every path that reaches it; the two entries disagree about exactly
  one thing, whether the record *below* the context was nudged first, and that
  is `ctx15 > 0` — the condition already written under the jump. With the jump
  gone the two nudges are visibly the same six lines twice over.
* **`expand_image`** ended in a three-step ladder — `read_palette`,
  `check_length`, `free_plane_buf` — where each arm fell into a different depth
  of one tail. Each step is a scope: four functions, and the alternate model's
  jump past the plane loop is one of them returning early.
* **`compress_image`** mirrored it, and `write_plane_descs` now sits beside
  `read_plane_descs` so the descriptor format reads as one thing in two
  directions rather than four levels of nested `if` in each frame.
* **`read_bmp`'s `done`**, five jumps into one tail, is three functions and a
  dispatch. Six locals went with it, each of which existed because a value had
  to outlive a jump.
* **`write_bmp`'s `write_rows`** was a label in the middle of the row loop with
  three jumps to it from two loops further in. The row is a function; the three
  are `return`; `row_i` was the last of the label's three carried arguments and
  is the row loop's own counter.
* **`add_weight`'s `rescaled`** was the tree's one jump *into* a block, from
  the search loop into the empty-list arm. Running out of live entries and
  having none to begin with are the same answer.
* **`decode_pixel`'s two.** `run_done` was skipping two lines that do nothing
  on the path that skipped them. `pixel_done` was the join of two paths that
  both mean "stage one did not produce the symbol", which is a function with
  the run length as its argument.

The eighteen sit at nine sites, and seven of the nine came off with a dead
store, a dead local or a redundant reload attached, because a jump is what had
been holding each of those in place. The two that did not are `write_rows`,
whose three carried arguments an earlier round had already taken, and
`pixel_done`, which was holding a scope rather than a value. `shape.py` reports every jump row at zero: none restart a loop, none
exit a block, none go sideways, none enter one.

## The gate that was not being run

Three commits in this round said "every counting tool still at zero" and
quoted `firstuse.py bmf.cpp`. `bmf.cpp` is 195 lines of `#include` with two
bodies in it, and `firstuse.py` is one of the seventy-seven tools here that
read the path they are given and nothing else. The zero was about a program
that was not there.

`tools/sweep.sh bmf.cpp` splices before it asks, which is why the sweep came
back FAIL on three tools while the hand runs were green — and all three
findings were this round's own work: two locals in `decode_pixel` and one in
`alt_p2_model` that could move to their first use now that no jump crossed
them; `rle_fits`, whose every use of its first parameter was a member access;
and the declined-pairs table in `bmf.cpp`, still saying `27 of 277` about a
decoder that had just lost twenty-five lines.

`structs.defs` says so now, once, when it is handed an include list rather
than a program — and the first thing that note caught was not a hand run at
all. It appeared in the middle of `tools/x32.sh`'s output, because that gate
asks `ptrwidth.py bmf.cpp`, and `ptrwidth.py` is one of the seventy-seven that
read the path they are given. The row the whole script exists to print, "0
pointers through a 32-bit integer", was a statement about the include list.
The comment on the line above it read "`bmf.cpp`, not `subs1.hpp`: the
decompilation is in the one file now", which had been two rounds out of date
when it was written. It splices now, and a planted `(int32_t)(uintptr_t)p`
shows the difference: the include list answers 0 and the unit answers 1.

`tools/proven.sh` had the same sentence and the same hole. Its whole job is to
replay each tool against old revisions and report which ones ever answer
something else — the evidence that a rule can fire at all. Its header said "a
tool splices `bmf.cpp`, so what it reads is every `.inc` beside it", which is
true of nineteen of the ninety-six; the other seventy-seven were handed the
include list at every revision, answered the same thing every time, and landed
in the not-proven bucket for a reason that bucket does not name. Each
revision's `bmf.cpp` is replaced by its spliced self after it is unpacked, and
over the same three revisions the count of tools that demonstrably answer
differently goes **21 → 28**: twelve gained — `defram`, `degoto`, `firstuse`,
`uncopy`, `unmemcast`, `unslot`, `unspill` and five more — and five lost, all
of them log readers whose old "difference" was a staleness note moving rather
than a rule firing. Those five are silent now, which is the honest answer for
a tool whose log does not describe the tree being replayed. It is a note on stderr and not a refusal: for a tool that
*rewrites* what it reads, one file is the right unit and the answer about
`bmf.cpp` is the right answer. What was wrong was what it got read as.

## Four zeros that could not have been anything else

Running the drivers turned up a class this project has met before and had not
looked for in its own shell scripts.

`sweep.sh` walks `tools/*.py`. It had never asked the sixteen `.sh` tools
anything, and four of them still named `subs1.hpp` — a file the tree stopped
having when it split into thirty-seven `.inc`s. Run today, each died on the
missing path and then printed its own zero: `0 kept, 0 reverted`, `round 1: `,
or — in `resign-drive.sh` — an empty `n` that `[ "$n" = 0 ]` read as *not*
zero, so the loop drove two hundred rounds of nothing.

Two `.py` defaults had gone the same way. `shape.py` with no argument, the
form its own usage line gives first, ended in a `FileNotFoundError` traceback
and an exit status of 0.

And then four tools whose zeros were unreachable by construction:

* **`resign.py`** matched warning lines with a literal `^subs1\.hpp:`. gcc has
  named one of the includes and never that since the split. `shape.py` and
  `retype_locals.py` were each fixed for this exact sentence, and each fix
  says so in a comment; this was the third reader of the same log and nobody
  went back for it. With the filename a group it finds a candidate again — and
  then a second layer under the first: `BMF_WARN=1 ./build.sh` passes
  `-Wsign-compare -Wunused-variable -Wshadow` and no conversion warning at all,
  so the log it reads has nothing of the kind in it. Its summary says which
  flags would give it something to answer.
* **`resign_group.py`** shared that regex and the group numbers under it.
* **`defram.py`** required a `// NNN bytes` tag on a frame's declaration.
  `shape.py`'s own frame census records that the tag went rounds ago and that
  frames are named instead. The pattern could not match a frame; with the tag
  optional it lifts a planted one.
* **`tools/driven.txt`**, where the driver records what it measured so the
  tools do not re-offer it, was stamped `# subs1.hpp <cksum>` where its reader
  wants `# sources <digest>`. Every verdict written since the split has come
  back stale, so the memo has printed nothing — which is the same silence as
  no memo at all, and is the defect the memo was built to fix.

`unmemcast`, `unslot`, `unspill` and `defram` all ask about reconstructed
stack frames, and `shape.py` counts none in the tree. `0 frame members`,
`0 slots`, `0 spill areas`, `0 of 0 aliases` — not one of those distinguishes
clean from absent, which is the third answer `sweep.sh` was taught to count
apart. They say `not applicable` now, and go back to counting when a frame is
planted beside them.

`sweep.sh` gained the test that would have caught the four drivers: a path
named on a line a shell tool *executes* has to exist. Comments are stripped,
because a comment naming `003/subs1.hpp` is history and history is allowed to
name what is gone. It is proven able to report — a planted `grep subs1.hpp`
fails the sweep — and proven quiet on the same name in a comment. It covers
the shell tools only: a `.py` names its file in a docstring, and those lines
are as often a placeholder (`input.hpp`, `one.inc`) as a path. The 132 usage
examples across 63 tools that named `subs1.hpp` were rewritten by hand in the
same round; what they need is a reader, and what the drivers need is a test.

## Two declines re-measured, and they were not the same kind

`MINIMAL-SYNTAX.md`'s Phase 7 measured 32 `while( 1 )`s with a break and
reported **0 convertible** — "not one of the 32 has its break at the top at
the loop's own nesting level; nineteen have it in the middle, twelve have no
top-level break at all". Two of them convert. They are worth telling apart,
because only one of them is the pattern this project keeps meeting.

`plane_choose`'s `descend` is a `while( dir*at[0] < dir*end[0] || dir*at[1] <
dir*end[1] )` now. **The body moved.** That loop was written out twice, once
per direction, with the second copy nested inside the first's and leaving it
by a `goto` past the bottom of both; the test that is now at the top of the
loop was not at the top of anything when the 32 were counted. A decline is a
measurement of a body, and bodies move.

`unmodel_plane_slow`'s row loop is a `do … while( (uint32_t)++bucket <
(uint32_t)height )`. **Nothing moved.** Its break was on the last line with no
statement after it, which is the shape the same paragraph names *first* when
it lists what would qualify. The sentence quoted above is true as written —
none has its break at the *top* — and then the paragraph goes on to say none
qualifies at all, which does not follow from it and was not true when it was
written.

The first is the cost of recording a measurement; the second is the cost of a
summary that says more than its measurement did.

## The pair that stays two functions, taken apart again

`code_pixel` and `decode_pixel` each wrote out the walk that codes a run's
length — one bit per bucket level from the top down, a bit coded only where
the level could still belong to a run shorter than the cap. They differ in
where the bit comes from and in nothing else, down to the order of the rescale
against the counter bump. The encoder's `first |= run_bit` accumulated the
mask where the decoder accumulated the bit, so its counter bump had to write
`n[run_bit != 0]`; with the bit normalised the two arms are the same three
lines. Both bodies carried a comment explaining that comparing their two
bucket names is not the tautology it reads as, which is one sentence now.

`ModelBlock::code_run_length<f_DEC>` is the fourteenth merged pair and sits
inside the one pair that stays two functions — which is the point worth
keeping. **A declined pair is a decision about two bodies, not a bar on naming
what they share.** `run_scan` came out of these two an earlier round; this is
the walk beside it. `code_pixel`/`decode_pixel` is 13 shared lines of 211,
from 27 of 277 at the start of this round: the share falls because what they
share keeps becoming something with a name.

It also took the tree's deepest nesting from 7 to 6 — the `if( idx1 > 1 )`
that replaced a `goto` two commits earlier was the seventh level, and the walk
it guarded is a call.

## Four guards on a constant

`write_bmp`'s palette writer tested its entry count four times, `ncol > 0`
twice and `ncol / 2` twice. The count is `1 << bits`, and `bits` reaches
`write_bmp` only through `expand_image`, which refuses any depth whose plane
count is outside 1..4 — so it is 1..32 there and 1..8 in this branch, and the
count is 2, 16 or 256. The four arms those guards protect are `at = 1`,
`at2 = 1` with `done = 1`, and a `pal_bytes` assignment that writes no
palette. None can be reached, and `hdrscan.sh` puts all 256 depth bytes
through this, which is what the reasoning rests on rather than a reading of
the code.

Two levels of nesting went with them and three names with those: `ncolours`
and `ncol` were the same expression assigned on consecutive lines and then
used interchangeably — `4*ncol` in one arm and `4*ncolours` in the next — and
`done` was `at2` copied on the following line, a copy that existed only
because the dead arm set the two differently. That is the shape the whole
round has: a dead branch is not only dead, it is load-bearing for the names
around it.

## Where this leaves it, a third time

* **0 jumps and 0 labels**, from 18 and 12 at the start of this round, and
  from 42 and 27 when the count was first taken.
* **39 lines of copy**, from 45; the ratchet moved with it.
* **fourteen merged encode/decode pairs**, from thirteen; two declined, and
  `code_pixel`/`decode_pixel` is 211 lines with 13 shared, from 277 and 27.
* **deepest nesting 6**, from 7, in eleven bodies of 280.
* **15 `while( 1 )`s**, from 32 when that class was last counted and 16 at the
  start of this round.
* **every counting tool zero, nothing silent, seven subjects absent** — three
  more than last round, and the three are tools that had been answering zero
  about frames the tree does not have.

The pattern this round is the one the last two had, arriving somewhere new.
Last round it was comments: a claim in a comment is a measurement with a date
on it. This round it was the instruments themselves — a default argument, a
filename in a regex, a stamp format, a glob that stops at `*.py`. A tool that
cannot fire reports the same number as a tool with nothing to find, and the
only way to tell them apart is to make it fire.

Which is also what the eighteen jumps were. Each of them was a claim: that
this exit is different from that one, that this variable has to outlive the
walk, that a structured form would need a flag. Seven of the nine sites came
off with a dead store, a dead local or a redundant reload attached, because a
jump is what had been holding each of those in place — and two of the claims
had simply stopped being true, one about a comparison that could not hold and
one about a loop that needed no flag at all.

So the answer to "is there anything left" is not a number. Every zero in this
document is a zero somebody has now watched report something else, and the
next round's work is whatever the round after this one finds those zeros were
wrong about.

---

## Stage two: a green probe that proves nothing

The last three sections were about instruments that could not fire.  This one is
about a probe that fires perfectly and still says nothing, which is harder to
notice because the probe *is* the gate and the gate is not broken.

`ModelBlock::pixel_context` wrote its two out-parameters three times on the way
through — `ctr_node = ctx0`, then `ctr_node = ctx1`, then the real pair in the
arm that returns a symbol, with a `ctr_fallback = fallback` in the arm that does
not.  Only the third write can be read: `offer_candidates` is the sole caller
and `continue`s on a negative return without touching either member.  Deleting
the other three is the obvious Phase 6 edit, and the obvious way to check it is
to poison them and watch the gate.

The poison passed.  So did a poison of the *live* store: `ctr_node ^= 1` is
byte-identical on all seventeen images.  And `ctr_node + 128` fails four checks.
Two of those three results are worthless and the third is worthless in the
opposite direction:

* **`^ 1` is a permutation of `bit_node[4096]`,** every entry of which starts
  memset to zero.  Applying a bijection to a uniformly-initialised table at
  every access relabels the whole table: each counter still receives the exact
  sequence of bits some counter received before, so not one byte moves.  It is
  not that the low bit is unused — instrumented, most of the index pairs the
  corpus touches have *both* halves live.
* **`+ 128` is not a permutation, it is an overrun.**  The index is
  `(pos << 7) + ctx2` with `pos` up to 31, so `+ 128` walks off the end of a
  4096-entry array and into the block behind it.  The four failing checks are
  memory corruption, and they would fail just as loudly if the value were never
  read at all.
* **`&= ~1` is the honest one.**  In bounds, and not injective — two contexts
  collapse onto one, so a live store has to move.  It fails ten checks.

With that as the control, setting each of the three suspect stores to 0 in turn
leaves all 110 green, and an execution counter says the three lines run 601k,
582k and 133k times on `t8g` alone.  Then they are dead, and they are gone.

The rule the round before this one was "a tool that cannot fire reports the same
number as a tool with nothing to find".  This is its other half: **a probe can
fire and still be uninformative, if what it perturbs is a symmetry of the thing
it is probing.**  A counter index is only worth perturbing in a way that is
neither a bijection over a uniform table nor a step outside the array — and
neither of those disqualifications is visible in the diff.

The same round found `nb_weights[1088]` addressed by a slot number that ranges
over 1920.  That one is not a defect anybody can demonstrate: the corpus reaches
593, a montage built to spread activity and gradient direction over 2500
independently-parameterised tiles reaches 980, and the union over all twenty
images is 1034 — fifty-three short of the bound.  So the array looks measured
rather than guessed, and the finding is a `BMF_ASSUME` under the `BMF_ASSERTS`
leg and a paragraph in §10.2, not a clamp.  A clamp would change the model on
exactly the inputs nobody has produced, and no reference stream would move to
say so.

---

## Stage two, finished: what the plan asked for, and what it was wrong about

Nine phases, four declines and three corrections.  The gate ran on every commit
and answered the same thing each time: 111 checks, 17 images, byte-identical.

**What landed.**  Phase 3 dissolved eleven aliases for five cursors in
`alt_p2_context`, ten fields reached by byte offset, and the arena's two punned
headers.  Phase 5 named the two mixer shapes, the ternary selector, the boundary
nudge, the gradient predictor and the row accessor — the last two turning sixty
lines of `c0[-2].val` into sixty lines that can be diffed against each other.
Phase 6 took `alt_p2_model` from one `if` around ninety lines down to an early
return with three names where one local had carried three values.  Phase 4 found
six redundant casts with the `-Wconversion` count as the oracle and left the
forty semantic ones alone, with the classes written into a build leg rather than
into forty inline markers.  Phases 7 through 9 named eleven constants, removed
four pieces of dead code and closed the last warning.

**Four things the plan asked for were declined, each with the reason recorded at
the code.**  `near_lossless_q` stays a knob no path can turn, because folding it
deletes the only description this tree has of BMF 2.01's near-lossless mode and
wiring a flag adds a mode no reference stream was taken under.  The BMP RLE
decoders share what they can already, and `write_nibbles` has no counterpart on
the writing side because a 4-bit image is stored packed.  `CtxIdx::digit`'s
unused `Radix` is not deleted but *checked* — it is the fact that sizes
`nb_id[]`, so what it wanted was enforcement.  And the
`exclusion_gen`/`mode_symbol` move is declined outright, because its premise is
false: those are read by `SymList` and by a free function, not only by
`ModelBlock`, so the move is the whole-program change the same appendix rejects
two entries above.

**Three of the plan's claims did not survive contact with the code.**
`p2_bump`'s shift is 1..3, not 2..3 — and the assert that says so failed fifteen
checks the moment it was written, because Phase 5's own comma-chain unfolding
had introduced the 1 an hour earlier.  `expand_predictor_mode0` had two call
sites, not one.  `p2_b1_reload`'s live entries were 5..7 as the plan said, but
the plan asked for verification against a binary that is not here, so what was
verified instead was the reachable range, the reader count, and ASan after the
shrink moved everything behind it.

**And one finding nobody asked for.**  `nb_weights` holds 1088 rows while
`nb_slot` names 1920 slots, with nothing between them.  The corpus reaches 593;
an adversarial montage reaches 980; the union over twenty images is 1034, fifty-
three short of the bound.  So the array looks measured rather than guessed, and
no input found so far crosses it — which is a different statement from "it
cannot be crossed", and the `BMF_ASSUME` is there to tell the difference.

The methodological note of the round is in the section above this one: a probe
can fire and still say nothing.  Three of the four instruments this round leaned
on had to be shown able to report before their zeros were worth anything, and
one of them — `covered.py` — was wrong twice in the same run, first about a body
written on its signature's own line and then about two methods sharing a name.

## Stage three: the monsters, and two tools that came out of taking them apart

Nobody asked for this round.  The plan was finished and the gate was green; what
was left was six functions between two hundred and five hundred and fifty lines
each, and the question of whether any of them had a seam that was a *stage*
rather than a step.  Four did.

| | was | is | what came out |
| --- | --- | --- | --- |
| `alt_p2_context` | 557 | 284 | `seat_nb_row`, `fill_row_inputs`, `step_bank`, `seat_symbol_context` |
| `write_bmp` | 286 | 81 | `write_bmp_palette`, `bmp_rle_encode` |
| `choose_plane_coding` | 347 | 205 | `choose_alpha_plane`, `fit_alpha_weights` |
| `search_filter` | 269 | 182 | `search_planes` |

The test that decided each one was the same: **does the block end where its
enclosing block does, and does it touch anything else of the frame.**
`fill_row_inputs` touched six cursors and nothing else, so those are what it
hands back.  `search_planes` took two lambdas and three locals with it because
nothing outside used them.  `bmp_rle_encode` needed only its own cursor, which
is why the retry that wrapped it — a `while( 1 )` with at most two turns, a flag
at the bottom and three `break`s out — could become one `if` at the call site.

**Where the seam was declined, it was declined out loud and in the file.**  The
four variants of `fill_row_inputs`' rows 4..6 stay adjacent, and so do the five
bank context words: each is one block per case, differing only in its terms, and
being able to diff them by eye is the entire reason the cursors are named
`c0`..`c4` and the fields spelled `d1(0)`.  Four or five small functions would
each read better alone and the set of them would read worse.  That is written at
`fill_row_inputs` and in `long.py`'s ratchet constant, because it is exactly the
kind of decision a later pass undoes for looking tidy.

**Ten things fell out of the moves that nobody was looking for.**  A dead store
into `bank_ctx[0]`, proved rather than argued — 12345 stored there leaves all
111 checks passing and stored nine lines down fails twelve.  `no_ref` assigned
twice thirty lines apart, which `const` now refuses.  `run4` written twice, the
second time as a register reused for a difference — and the original said so
itself, recomputing `run3 + pred4` for the level index rather than reading the
variable back.  `nb_dot` written out three times.  `hists + 3*1024` written in
three functions.  Two accumulators named for bits that held plane counts.  A
`can_rle` flag with three other names for the same bit.  And `n_p2 = bits_a`,
a bit count stored in a plane count, which is **left alone**: its only effect is
on an `!n_p2` twenty lines below, so a flag would say it better, but
`transform_cost` measures bytes written before the coder's flush and the trial
tile is as small as 4x3, so a zero is not ruled out by anything here.

**Two tools.**

`unnest.py` came from `code_banks`: two nested `if`s around all of its working
lines, nothing after either, 73 lines of code at five levels of indentation, and
no tool said so.  A decompiler makes this shape by default — a conditional jump
over a region becomes an `if` around the region, and nothing asks whether the
region ends where its block does.  Its first run had two false findings in eight
because `close_of` counted braces and `} else {` is net zero; reading the eight
one at a time is what caught it, and three more of the eight went the same way
once it was fixed.  Four real findings, all fixed.

`long.py` came from running the same census by hand four times in one round.
Every time a function came apart the next question was which one is longest now,
and the answer was a throwaway regex in a shell.  It counts *code* lines, so a
heavily annotated body is not the finding, and it carries a ratchet, because
"the longest function is 174 lines" tells a reader nothing they can act on while
"something is longer than the longest was" tells them a body grew back.

**Both were shown able to report before their zeros were counted.**  `unnest.py`
finds all four of its findings on the tree as it stood one commit earlier and
none on the tree now; `long.py` reports `alt_p2_context` sixty-seven lines over
on the tree of four commits ago.  That is the rule this project keeps
re-learning, and this round it was cheap: two `git show`s into a scratch file.

**Where this leaves it.**  Thirteen bodies between 87 and 129 code lines and one
at 174, which is the five context words.  Before the round the longest was 241
and the next 129, which is what "outlier" meant; there is not one now.  111
checks over 17 images on every commit, every counting tool in a 95-tool sweep at
zero, ASan clean over 44 runs, and `hdrscan` clean over 8704.

## The gate had a hole in it the width of a pointer

The round above ended "111 checks over 17 images on every commit, every counting
tool at zero".  Both halves were true and neither was the whole gate.

`build.sh` documents **seven** builds.  `test.sh` runs one of them.  Nothing ran
the other six, and `BMF_BITS=32 ./build.sh` had not compiled since Phase 6 --
three rounds -- because that phase added eight `static_assert`s on struct
offsets and wrote every number at 64 bits.  It was found by running the leg,
which had not happened since; it was confirmed by building the same leg in a
worktree at the commit before Phase 6, where it compiles.

Every one of the eight offsets moves with the pointer width, so both sets are
written down under a `UINTPTR_MAX` guard.  The 32-bit numbers are the more
faithful pair -- `_pad22` and `_pad25` are the decompilation's record of a
32-bit x86 frame, and the width they were measured at is the width they
describe.

**And with the leg building, it passes 111 of 111 byte-identical.**  That is
worth more than the fix: the arithmetic that reaches the range coder does not
depend on how wide an address is, which a decompilation of 1997 x86 has no
reason to guarantee and every reason to break.  `tools/x32.sh` had been written
to ask exactly this and had been unrunnable the whole time.

`tools/legs.sh` runs all seven now and exits non-zero when one does not answer
what it should.  Pointed at a deliberately broken assert it immediately found
two more things:

  * **`BMF_CONV=1` exited 0 whatever happened.**  Its count is
    `grep -c ': warning: '`, and a failed `static_assert` is an `error:` line,
    so a build that did not compile answered "183, at the ratchet" with a
    successful status.
  * `legs.sh`'s own check had to read that status rather than the last line,
    which is the same defect one level up.  It was written the wrong way first.

**The CONV leg is a ratchet now instead of a leg calling itself one.**  Its
comment has said "a *new* narrowing that nobody decided on shows up as a count
that went up" since it was written, with no number to go up from -- and this
round took it from 198 to 225 without anybody noticing, because 29 of the new
ones were conversions inside frame declarations that four extractions had
emptied.  Deleting those left 196, two below where the round started.  Both
widths are recorded and the leg says over, under or at.

The thirty unused locals the extractions left behind are the other half of that
story, and they say something about the sweep.  `unused.py` reports zero and is
right to: it reads the *spliced copy*, and the default build does not pass
`-Wall`.  The only thing in this tree that can see an unused local is
`BMF_WARN=1`, and for a round nothing ran it.  A tool reporting zero and a leg
nobody runs look identical from the outside, which is the same lesson as the
probe that proves nothing, arrived at from the other direction.

**What a round ends with now**: `./build.sh && ./test.sh`, `tools/sweep.sh`,
`tools/legs.sh`, and the three slow ones -- `asan.sh`, `fuzz.sh`, `hdrscan.sh`.

## Forty-two subscripts written the long way, and what finding them exposed

Asked where the unnecessary pointer accesses were, the honest answer turned out
to be forty-two, in three shapes:

```
*(freq+3)   ->  freq[3]      29 of them
*unfold     ->  unfold[0]    12
*slot       ->  slot[0]       1, and it was next to `slot[-1]` on the same line
```

`*unfold = 0;` sat directly above `unfold[255] = 0x80;`.  `code_symbol_tree`
reached one table three ways in forty lines -- `f0[j]` in a loop, `*(freq+3)` in
a rescale written longhand, `*freq` for element zero.  `update_binary_pair` did
the same in seven.  Hex-Rays writes an array access as pointer arithmetic
whenever the original's addressing mode was a base plus an offset, so the
spelling records the instruction and not the idea.

`tools/unstar.py` is the rule, and the half of it that matters is the half that
*declines*.  A cursor the body walks keeps its star: `*slot` inside the decode
loop is the counter the walk arrived at, not element zero of anything, and
`slot[0]` there would read worse.  So `*p` is reported only when the body never
moves `p` and subscripts it somewhere else -- one base spelled two ways in one
body, which is a thing a reader has to stop and reconcile.

**The pair that splits on that rule is the best argument for having one.**
`model.inc` and `sym_list.inc` carry the same line, `*slot = slot[-1];` and
`*list = list[-1];`.  A first reading called both of them cursors and left both,
on a body span that ran far past the method.  A second called both findings, on
a grep that truncated before the `list += n_live;` that moves one of them.  The
rule was right both times and the reader was not: `slot` never moves and is a
subscript now, `list` does and keeps its star.

**And it exposed something much larger than itself.**  `unstar.py` could not see
a function whose signature wrapped onto a second line.  Fixing that by hand
fixed the wrapping and still missed every in-class method -- and
`structs.bodies` had existed the whole time for exactly this, saying in its own
docstring that it was written after forty-three tools here turned out to be
reporting zeros about 138 of the program's 219 bodies.  All three of this
round's new tools were rolling their own regex and seeing 190 bodies where the
shared finder sees 331.

What that cost, once each tool was moved onto it:

  * `long.py`'s census had been measuring a subset -- thirty-four wrapped
    definitions invisible, `bmp_rle_encode` at 138 lines among them, and every
    method.  Its ratchet is 171 now and means something.
  * `unnest.py` had been **dropping findings silently**: when a guard's
    enclosing block opened on the second line of a wrapped signature it matched
    neither the loop pattern nor the function pattern, so the finding went
    nowhere.  Two came back, holding 39 and 24 lines, both now early returns.
  * `unstar.py`'s own walk test counted a *declaration* as a walk, but only when
    the spacing put the star against the type -- so one of two identical lines
    reported and the other did not, by where a space fell.

That is the round's lesson repeating for the fourth time, and it is worth
stating in one sentence: **a check that cannot see a thing and a check that sees
it and finds nothing print the same number.**  The 32-bit leg, the two `resign`
tools, `resign-drive.sh`, and now three body finders.  Every one of them was
green.

## What the spelling pass was worth the second time

`code_symbol_tree`'s rescale was twenty-six lines carrying seven accumulators --
`h2`, `acc`, `h4`, `sum4`, `acc6`, `acc8`, `h9` -- with the stores interleaved
in the order the registers came free.  Underneath all of it:

```
freq[k] -= freq[k]>>1    for k in 2..9, summing into freq[0]
```

That is `halve_up`, which this tree already calls through `halve_counts` from
four other places.  Six lines now.

**It only became visible after the dereference pass.**  The block reached its
table as `*(freq+2)` .. `*(freq+9)`, and spelled that way it is twenty-six
unrelated statements; spelled as subscripts it is one loop written out.  The
first-order value of that pass was that `freq[3]` reads better than
`*(freq+3)`.  This was the second-order one, and it was larger.

**One of the seven was `int16_t` where the rest were `uint16_t`.**  That only
matters above 32767, where the signed one goes negative and sign-extends into
the next term -- so the rewrite is exact under a bound and not otherwise.
Instrumented over the corpus the total reaches 16431 and that partial 16396,
both held down by the rescale firing at 16384, and the bound is a `BMF_ASSUME`
the asserts leg checks.  Shown able to fail before being trusted: at `< 0x1000`
the asserts leg fails twenty checks.

`reroll.py` was right to say nothing about this, and now says why.  Its rule is
textual identity once the *integers* are blanked, and no two of those twenty-six
statements are the same text under any blanking -- the decompiler renamed per
turn.  Widening it to blank identifiers is `dupblock.py`'s rule, which reports
rather than applies for exactly that reason.  A scan for the residue -- a window
touching `name[K]` for five or more consecutive `K` -- came back with eight, all
of them prose or a loop that is already a loop.

**And the conversion ratchet earned its keep in the same commit**: 196 to 192 at
64 bits, 183 to 179 at 32, with `legs.sh` saying "lower it" rather than passing
quietly.  Two commits earlier the same number had gone 198 to 225 unnoticed,
because there was no number to compare against.

## Three files that were a wrapper, and one that was a reason that had gone

Asked which `.inc` files hold nothing but a one-line function, the tree had two:

```
model_plane.inc      void model_plane(...)             { code_plane<0>(...); }
alt_p2_decode.inc    int32_t alt_model_p2_decode(...)  { return alt_model_p2<1>(...); }
```

Each is half of a pair whose other half already lived with the body it wraps.
`plane.inc` held `code_plane<f_DEC>` *and* `unmodel_plane`, so `model_plane` had
been separated from its own decode half; `alt_p1_code.inc` keeps its pair
adjacent under their template and the p2 file does now too, which is why it is
`alt_p2_code.inc` rather than `alt_p2_encode.inc`.

**`sym_list_decode.inc` was the more interesting one**, because it was not a
wrapper -- fifty lines of real body -- and it still should not have been a file.
It held one method of a class defined nine files earlier, and the reason it sat
that far down had gone: it reaches `rc.decode` and `rc.get_freq` and otherwise
only its own class's members, and `rc.inc` is included two files *before*
`sym_list.inc`.  The split was the size and the frame, and its own header
records what happened to both -- two copied bodies became calls to `promote` and
`rescale`, which emptied the frame.  Nothing re-asked the question afterwards.

36 files to 33.

**The check that makes a file move safe to claim.**  Splice both trees, strip
comments and whitespace, sort the lines, diff.  Identical means the same
statements in the same multiset -- what moved is position and prose, and nothing
else could have.  Both merges pass it, which is a stronger statement than the
gate alone: the gate says the streams did not move, and this says the program
did not.

**And the answer to "are there more".**  Asked of every `.inc`, two files have
no definition longer than four lines -- `ctxidx.inc`, which is `CtxIdx`'s seven
builder methods, and `ida.inc`, which is nine small numeric helpers.  Both are
modules of short bodies rather than wrappers around something else, so both
stay.  Every one of the 33 headers names its own file, checked.

`REVIEW.md` and `tools/split.py` still name all three of the removed files, and
still should.  The first reviews the tree at a past state and gives
`sym_list_decode.inc` as 194 lines; the second says in its own docstring that
its layout table is "a record of where each body went, not something that can be
run again", with thirty names in it that already did not exist.  A record that
is updated to match the present is not a record.

## The allocator round: what has a bound, and what does not

The question was which dynamic allocations have a maximum size under 256 MB, and
those became storage instead of calls.  What made it answerable is that this
program already refuses everything it cannot size: `read_bmp` takes a width and
height in 1..65535 and a row no larger than 0xFFFF bytes, `expand_image` refuses
the same coming back and a plane count outside 1..4, and the alphabet coder
refuses anything above 0x2000 symbols.  Every bound below is one of those.

**What moved, and what bounds it.**

| was | bound comes from | now |
| --- | --- | --- |
| `AltP1Block`, `AltP2Block` | `plane_count` ≤ 4 | `BlockPool<T, 4>` |
| `ModelBlock` | one at a time, measured | `BlockPool<T, 1>` |
| five-row rings ×3 | `kMaxWidth` | members of their block |
| `row0`/`row1` weight rows | `kMaxWidth + 4` | members |
| the frequency tables | one global pointer | one static array |
| `read_bmp`'s row scratch | the stride it accepts | one static array |
| alphabet map, codes, runs | `no_symbol + 1`, `kMaxWidth` | members |
| the symbol lists and entries | `no_symbol + 1`, fixed per list | members |
| the archive handle | one per run | one static |

**Two of those bounds are by construction rather than by measurement**, and
those are the ones worth naming.  The frequency table is reached through a
single global pointer, so a second one would have to overwrite the first --
there is nothing to count.  And `expand_alphabet`'s recursion is one level deep
because the recursive call sets `depth = 8`, which puts the alphabet it then
reads inside the arm that does not recurse; that is what lets its byte lists be
one static array rather than a stack of them.

**The rest stay, and stay for the right reason.**  Eleven allocations are sized
by the image -- `data_size`, `width * height`, `2 * height * width` -- and at
the widest input this program accepts those reach about 4 GB, which is over the
line by a factor of sixteen.  They are the pixels and the coded stream, and they
are exactly what a compressor should be allocating.

**What it cost.**  `.bss` went from about 1 MB to 86.1 MB, and the program's
floor is now the widest input it accepts rather than the input it was given: a
16-pixel-wide image carries the same rows a 65535-pixel one does.  What it
bought is roughly sixteen thousand fewer allocations on an 8192-symbol image --
`SymList::init` ran twice per symbol -- and a working set that is a
compile-time constant.

**And it broke a test leg, which is the part worth writing down.**  The
out-of-memory ladder ran from 4000 KB to 16000 KB looking for the rung where
`bmf_new` returns null.  With 86 MB in `.bss` every rung died mapping the
binary's own image before `main`, a SIGSEGV that reads as "died instead of
reporting" while the program is correct.  Two things were wrong and both are
fixed by measurement: the ladder's floor is read from the binary with `size`,
so it cannot go stale behind the pools, and the image it compresses had to grow
-- with the blocks static, `t1.bmp` needs so little heap that no rung separates
"cannot map the image" from "succeeds".

**Three loose ends, each caught by a leg within a minute of being made.**  The
p2 ring seating was written `buf[row++] = row_store[row]`, which reads and
increments `row` in one expression; `BMF_CONV=1` counted it.  `w` in
`layout_workspace` lost its last reader when `run_bucket_store` replaced
`bmf_new(w + 1)`; the same leg said so.  And `left`/`left2` went the same way
when their free loops became `free_sym_entries`.  A ratchet that counts
warnings finds this class faster than reading does.

## The codec is a class, and two of them run at once

The ask was that the actual codec become a class -- `main` and the file I/O may
stay outside, but compressing and expanding an in-memory image have to be
methods, and it has to be possible to make several of them in one process and
run them in opposite directions at the same time.

**What was in the way.**  Twenty-three file-scope mutable objects: the coded
stream, the range coder, the plane descriptors, the deadzone bounds, the model
tables, the geometry, the exclusion mask, the mode symbols.  Two codecs in one
process shared every one of them.  That is what a 1997 single-shot compressor
could afford, and it is what the decompilation had.

**Two classes and not one, and the split point was measured.**  `BMFState` in
state.inc holds the twenty-three; `BMFCodec` in codec.inc derives from it and
adds the three block pools.  The pools cannot live in `BMFState` because they
hold `AltP1Block`, `AltP2Block` and `ModelBlock` *by value* and each of those
holds a `BMFState*` back.  Splitting at that line is what lets every block
method stay inline where it already was: `BMFState` is complete before the first
block class is parsed, and `BMFCodec` is complete before the first thing that
needs a pool.

Which functions had to be state-level was counted rather than guessed.  Ten are
called from inside a block class, four of those are already methods of one, and
the transitive closure of the rest is fourteen.  So fourteen declarations sit in
`BMFState` and sixty-four are `BMFCodec` methods, and nothing had to be deferred
out of line or turned into a free function taking a `this`.

**The two entry points.**  `compress_to_memory` and `expand_from_memory` are
`open_memstream` and `fmemopen` over the file-taking functions rather than a
second copy of them: a `FILE*` onto a buffer is exactly the shape
`compress_image` and `expand_image` already took, so the file layer is untouched
and the codec gained an entry point instead of a fork.  `main` still opens the
paths and `read_bmp`/`write_bmp` are still the program's, which is what the ask
allowed for.

**`tools/parallel.cpp` is the check the class exists for**, and it earned its
place three times over: eight codecs in eight threads, every result compared
byte for byte against what one codec produced alone.

Its first version gave each thread a fixed job -- four compressing, four
expanding -- which puts both directions in the process at once but never puts
both through *one* codec, so no compress ever started from the state an expand
had left. It picks per step now: each thread's own generator names one of the
four images and one of the two directions, `kSteps` times, from a seed that is
fixed by default and printed on every line it writes. A failure is repeatable
by passing that seed back.

**And it was shown to report before its zero was believed.** Making
`BMFState::p2_coef` and `p2_rate` `static inline` in a scratch copy -- one word,
and exactly the sharing ThreadSanitizer found -- takes it to
`FAILED: 3 of 48 parallel results differ, at seed 20260821`.

1. `byte_list_ent` and `gap_list_ent` were still file-scope.  Two codecs at once
   wrote into the same 12 KB and `t8g` compressed to 43688 bytes in a thread
   against 43664 alone.  They had survived the census because the allocator
   round introduced them as *storage* rather than as state.
2. A heap `BMFCodec` had indeterminate members where a static one had been
   zeroed, and a codec that had coded once was not in its fresh state any more:
   three rounds on one codec disagreed with one round.  `reset()`, one `memset`
   of a standard-layout aggregate, called at both entry points.
3. **ThreadSanitizer named the third**, which the byte comparison could only say
   existed: `P2Coef::fold` writing `bmf_p2_coef` while `nb_dot` read it in
   another thread.  Those two tables read like tuning constants and are not --
   the model borrows them for the length of one image -- so they became
   genuinely `const` initialisers and every codec folds its own copy.

**What the legs and the tools found afterwards**, none of it by reading:

  * `-Wshadow` caught `P2Coef p2_coef` shadowing the new member of that name.
    That warning is in the leg precisely because this tree has no `this->` left
    to tell a member from a local.
  * Restoring the p2 tables as members without `alignas(16)` crashed the
    optimised build inside `nb_dot` and left `-O0` working: the code around them
    loads them with SSE.  The alignment is written down with the reason now.
  * Both entry points called `reset()` and *then* read the tail block out of the
    state it had just cleared, so the recompress channel `main` uses was
    silently unusable through them and an expanded tail leaked one block per
    stream.  It is an explicit optional argument on both now.
  * `methodise.py` reported `ref_transformed` and `alt_p2_start_row` -- a first
    parameter that is only ever a receiver.  The round that made them that shape
    is the round that made them free functions taking `this`; they are `BMFState`
    methods, which is where they can stay put, because `alt_p2_start_row` is
    about a block and is parsed before `BMFCodec` exists.
  * LeakSanitizer on the parallel test found 5 MB in four objects -- the test's
    own sample images, never freed.  Worth fixing rather than filtering: four
    unfreed images is a page of report standing between a reader and a leak the
    codec actually had.

**A tool that had to be taught, and a self-test that caught its author twice.**
`dupblock.py` counts runs of statements written out more than once, and it
already skips a field list because a struct's members are not statements.
Sixty-four sorted method declarations put two three-line stretches of
`codec.inc` into agreement, which took the residue two lines over its ratchet --
copy that no edit could remove, because the list *is* the class.  Raising the
ratchet would have recorded a class declaring its methods as a debt, so the skip
learnt what a prototype is instead.

The `--selftest` written alongside it disagreed twice, and both were real: the
pattern read `return f(x);` as a prototype -- `return` parsed as the return type
-- and the planted fixture wrapped its two copies in `void a() {` and
`void b() {`, which normalise to the same line, so it was measuring its own
braces.  The keyword guard also un-hid `return x;`, which the *original* field
pattern had been skipping since the day it was written.  Residue 46 -> 43, and
the ratchet came down to match.

**The one thing two codecs still share, and it is not settled by a test.**
Under `-DBMF_HIGH_ARENA` the bump pointer and the forty free lists are
file-scope by construction -- an allocator per codec would defeat what that leg
exists to prove -- so the four entry points take a spinlock.  That lock rests on
inspection, which is the opposite of how everything else here was justified, so
the code says so: `tools/parallel.cpp` passes against that leg with the lock
stubbed out, because eight threads allocating a few dozen times each will not
reliably interleave a bump pointer, and ThreadSanitizer *will not start against
that leg at all* -- its shadow reservation fails before `main`, reproducibly and
with ASLR off, while the same tree without the define runs under it fine.  With
no instrument that can report, what stands is the code: `bmf_arena_used +=
need+16` and the free list's head swap are both read-modify-write on shared
words and neither is atomic.  The default build compiles none of it, because
`malloc` is already thread-safe.
