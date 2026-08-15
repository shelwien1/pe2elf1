# bmf_refactoring_plan.md, implemented

A record of the round: what the plan asked for, what was done, and -- the part
worth keeping -- what was declined and on what measurement.  Same shape as
`CLEANER.md` and `REVIEW.md` before it, and like them not registered in
`tools/unstale.py`'s `DOCS`.  That tool's rule is that a document naming
something the program does not have is stale, which is the right rule for
`ALGORITHM.md` and `MODELS.md` and the wrong one for a history: registering
this file reported ten names as stale, and all ten are names this round
removed, which is what it is here to say.

The plan was written against `bmf__.cpp`, a 10 281-line single translation
unit.  This tree is the same program already split into 38 files, so Phase 4's
module split was largely done before the plan arrived and the other phases
landed on a tree that had had two cleanup rounds already.  Where the plan
described something that was no longer there, that is said below rather than
quietly counted as done.

Every item was gated on the streams byte for byte at both pointer widths --
`./build.sh && ./test.sh`, now 110 checks over 17 images, and `tools/x32.sh`,
23 of 23.  Re-run at the end over the whole round: ASan clean across 44 runs of
19 images, `hdrscan.sh` clean across 8,704, `fuzz.sh` clean across 400 mutants
of 36 seed files with no report and no allocator complaint, `tools/sweep.sh`
with every counting tool at zero -- and that last one means something different
now than it did when this paragraph was first written, because the sweep was
measuring the include list.  The compiler is at zero warnings, from four when
the plan arrived.

One number in this paragraph was wrong for most of the round: `fuzz.sh` runs
400 mutants and not 200, which is its documented default and what every run of
it printed.  Recorded here rather than quietly corrected, because a gate
paragraph that misstates its own gate is the thing this document is against.

---

## Phase 0 -- the harness

**Done.**

`test.sh` gained a seventh leg: expand the *reference* stream rather than the
one this build just wrote.  The round-trip leg feeds the decoder whatever the
encoder produced, so a change that moves both halves the same way passes it;
this leg's input is frozen, so it answers about the decoder alone.  That is the
plan's "orig-c -> new-d" direction.  The other direction is already the streams
leg: bytes equal to the reference are bytes the old decoder read.

Proved it can report before trusting its zero -- pointed at the wrong image, 2
of 2 failed.

`tools/perf.sh` is Gate C.  Writing it turned up why the obvious form does not
work: the identical binary compared against its own recorded numbers reported
+1.15%, then +4.52%, then +3.62% geomean minutes apart.  That is a shared
container whose available CPU moves more than the thing being measured.  So it
keeps the baseline *binary* and runs the two alternately, one image at a time,
with the order inside each pair swapped every other run.  Pairing took the
self-comparison from +4.5% to -0.9%; the order swap is there because running
the candidate second every time reported t24 and t32 decode 6-8% slower for a
binary compared against a copy of itself.

`alt_p1_free` freed `this` and returned it, and `alt_model_p1_d8_decode`
passed that on to a caller that discards it.  Both return `void`.

## Phase 1 -- mechanical renames

**Done**, except the `__` prefix.

`__exit_402E40` is `bmf_fatal`, and the message table in its body is the
`BmfExit` enum -- the table defines the codes, so there was nothing to invent.
Two of the eight are never raised (the original had an option parser and an
interrupt check; this build has neither) and stay, because removing them would
renumber the five that are.

`__byte_445440` was named by role, and finding the role moved the answer.  It
was never a 544-byte object: it is byte 8192 of the exclusion mask, and the
mask was never 8192 bytes either.  Three measurements say so --

  * the alphabet reaches 8193 symbols: `rc_decode_flat(8193)` returns 0..8192
    and the count is that plus one, so a symbol index reaches 8192;
  * an empty `SymPair` slot holds 8192 and `pixel_context` rejects it by
    finding that byte stamped, so the sentinel and the largest symbol share
    slot 8192 on purpose;
  * the generation reset clears `(alphabet+15)>>4` sixteen-byte blocks, which
    for the widest alphabet writes 8208 bytes.

The third is the binding constraint and is why `blob-independence.txt` records
the tail as SHARED.  One array now, sized by the widest clear, with `no_symbol`
naming the sentinel.  This is also the plan's Phase 2 "split the blob into two
typed arrays" item, answered: they are not two arrays.

**The 16 dead parameters.**  `decls.inc` recorded the opposite decision -- that
they stayed because the signatures match BMF.exe's and `tools/addrmap.py` keys
on that.  Checked: it does not.  `addrmap.py`'s `DEF` pattern captures the
identifier before the `(` and stops; it never reads a parameter list.  The map
survives the drop intact, so the trade that note described was not on offer.
`tools/deadparam.py` does it, and had to reach a fixpoint -- four of the
sixteen were not unread but *forwarded*.

**The `__` prefix: declined.**  `CLEANER.md` declined it because `deadcheck.py`,
`unnamed.py` and `addrmap.py` key on it.  That claim was checked this round and
holds: `addrmap.py`'s `DEF` requires either `__name(` or `Type::name(` to
recognise a definition at all.  The plan pins the rename to "once the module
split fixes visibility", and the split is done but the build is still one
translation unit, so the visibility argument has not arrived either.  Doing it
means teaching three tools a new way to find a definition; that is the work,
and it is not done.

*(Done later in the round, and this paragraph is wrong in the way it is most
worth keeping: the claim was "checked this round and holds", and checking it
properly is what found that `addrmap.py` had been printing an empty map.  See
"the `__` prefix is gone" below.)*

Also: the prefetch that asked for the cache line holding a local the next line
writes, one `w2 = w2;`, and a pointer `search_filter` spilled to its frame
and reloaded thirteen times -- `p_i_2`, `img_c` and the `img` parameter are one
pointer.

## Phase 2 -- reference aliasing

**Done.**

`plane_desc[5]` held four planes.  Slot 0 was a header slot with three
unrelated things parked in the weight words: `plane_count` and
`near_lossless_q` were references onto two of them, and `expand_image` parsed
the archive version word into the third -- while `compress_image` wrote 512
there that nothing ever read.  All three are their own storage; the array is
`[4]` and 207 subscripts lost their `+1`.

`tools/planedesc.py` did the re-index and refuses to guess at a fifth shape:
`plane_desc[next_plane]` and `plane_desc[pl2]` are not missing a `+1`, they are
subscripts whose variable already counts slots, and no syntax tells the two
apart.  Each is listed for a human and the answer recorded in the tool.

Nested subscripts were the tool's own bug, and worth recording because the two
halves cancel: sorting innermost-first and editing in that order moves the
outer one's closing bracket *and* reinstates the original inner text, so nested
sites come out quietly unshifted.  It rewrites recursively now.

`bmf_plane_desc(off)` had one usage cluster -- eight `uint64_t` stores over two
turns of a countdown loop, at offsets 56 down to 0.  That is
`4*sizeof(PlaneDesc)`, which is one `memcpy`.

`void* plane[4]` with three `AltP1Block*&` aliases is an `AltP1Block* plane[4]`.

## Phase 3 -- stack frames

**Two of eight dissolved; six left where the tool left them.**  *(Later in the
round a seventh came partly apart -- `search_filter`'s ten members outside its
union -- and three of the four recorded failures turned out to describe an
experiment the tool could no longer run.  See "the four frames that were tried
and reverted" below.)*

`DecodePixelFrame` and `CodePixelFrame` dissolve.  An earlier round had already
recovered the neighbour array out of them, and what was left was that array,
four bytes of gap, thirty-two of pad, and no other member -- 212 uses of
`frame.sym` are now `nb_sym`.

The other six are `tools/liftframe.py`'s recorded results, and they are results
rather than an absence of effort: four were tried and reverted (`cost_candidate`
and `search_filter` abort on altp1 while compressing, `reduce_alphabet` on
DLRAW, `expand_image` exits 3 while decompressing) and two declined because
every member is inside its union.  `ReduceAlphabetFrame` moved to file scope so
its two arms could split, which is a scope change and not a dissolution.

The plan's `static_assert(offsetof(...))` step was not taken.  The offsets
available to assert are this build's, not BMF.exe's, so the assertion would
restate the layout rather than check it -- and it differs by pointer width, so
it would need to be two.  What actually catches a layout change here is the
gate: the frames whose layout is load-bearing fail it, which is how the four
above were found.

`__PAIR64__`'s one live site was two 32-bit stores after all, so the macro went
with it.

## Phase 4 -- module split

**The split was already done; its two named traps are now handled.**

`rc_begin` was three jobs in ninety lines.  `packer_rewind` gives the
packer's spare bytes back (and is where "`packer_free_bits` goes transiently
negative" is now written down), `begin_plane_stream` sets up the alternate
model's limits, level geometry and 1024 seeded strips, and the coder's own init
is the third.  `rc_end`'s table free is `end_plane_stream`.

It also returned `tbl` *uninitialised* when no alternate model was named, with
a comment that the caller reads it only under the same flag.  It returns null:
that is what the caller was already behaving as if it got.

`bmf_p2_coef` and `bmf_p2_rate` are session state and `P2Coef::fold`/`restore`
says so.  Writing it corrected two file headers, which both claimed the decoder
does not borrow the tables -- it does, in six lines identical to the encoder's,
260 lines from the six that undo them.

## Phase 5 -- monster functions

**Four of the seven.**

`AltP1Block::update_model`, 535 lines to 110.  Diffing the nine selector blocks
first, as the plan says to, found three things: eight are identical; the ninth
differs in one expression, and that difference is the same one-weight shift
`ctx_alt` already shows, so it is a parameter rather than a special case; and
every block's base is `ctx[0]` under nine different names, which is MSVC
spilling a register once per pair of bumps.  The ninth block's tail was written
longhand only because it also left a value in the return, and all five call
sites discard that.

`AltP1Block::ctx_of`'s three neighbour arms are three formulas, not one three
times -- but four shapes recur across them and are named: `cross_grad`,
`nb_resid`, `west_grad`, `north_grad`.

`alt_p2_model`'s two SIMD-shaped nests are `nlms_track_two_rows` and
`nlms_predict_and_correct`, which is also where the `[7][4]` shape is said out
loud: rows 0..6 taps, 7..13 the running mean square that makes it *normalised*
LMS, 14 the shared scalars.

`transpose_image_in_place` is five open-coded sites.  Three of them wrote
`*(uint16_t*)&img->stride`, which is not the same store as the other two's --
it leaves the top half of a 32-bit field alone.  What makes them one function
is that the *value* is sixteen bits wide at all five, and the reads on these
paths are `(uint16_t)img->stride` to match.

`ring_advance` is the five-row rotation both models hand-rolled.  The padding
copies stay at the call sites: the two mirror different cells from different
offsets, and folding them in would be one helper with two shapes behind a flag,
which is not one helper.

`reduce_alphabet`'s narrow arm is `reduce_narrow_alphabet`.  The wide arm
keeps its two labels, which is what the plan asks for -- `code_dense` and
`alphabet_done` are a shared fast path and a convergence point, and structuring
them adds nesting rather than removing it.

**Not done**, and each for a reason rather than for lack of time:

  * `choose_plane_coding`'s candidate table (582 lines).  The candidates are
    not uniform -- they differ in which descriptors they write and which cost
    accumulator they read -- so table-driving them needs the per-candidate cost
    body extracted first, and that body reaches six locals of the enclosing
    frame.  It is a real piece of work, not a mechanical one.
    *(Done later in the round.  What it was waiting on was `deadparam.py`
    missing four numbered `unread` parameters, not the six locals.)*
  * `alt_p2_model`'s per-bank counter walk and its magnitude coding -- 311 and
    220 of that function's 635 lines.  Both are single loops already; extracting them means threading
    about thirty locals through a signature, and a thirty-parameter method is
    not an improvement over a labelled block.
    *(Done later in the round.  "About thirty locals" was a guess; the compiler
    said 33, and not one of them is read after the loop, so they moved.)*
  * the `CodedStream` / session-object consolidation of the coder's globals.
    This is the largest single item left in the plan and the one with the
    widest blast radius; it wants its own round with its own gate budget.
    *(Done later in the round -- see the Phase 7 record.  The blast radius was
    126 substitutions the compiler checks, which is not the same as 126
    risks.)*

## Phase 6 -- layout and punning

**Done except the last line of it.**

`p2_float_pool` is gone.  Seventeen `int32_t` of float bit patterns whose one
live use read *bytes* out of the middle -- offsets 4 through 11, the
little-endian bytes of elements [1] and [2].  Those eight bytes were the whole
of what the table was for; the other nine elements had no reader.  So it is the
eight bytes, written down and indexed by rate.  The plan predicted the values
and the constants confirm them.  A rate of 0 or 1 reloading to zero is
deliberate -- it interacts with the `(uint8_t)--countdown` wrap -- so it is
preserved, not fixed.

`abs32` was `(uint32_t)-x`, which negates before it converts and is signed
overflow at `INT32_MIN`.

The `AltP2Block` union existed to pin fields after `p2_row` at the original's
offsets: its scalar view opened with 112 bytes of filler, 112 being
`sizeof p2_row`.  Sequential members now.  The one overlap ever exercised is
`memset(p2_row, 0, 15)` -- fifteen, not sixteen, three and three-quarter
floats, preserved rather than tidied because rounding up changes what the model
sees on a plane's first sample.

`p2_ctr`'s `(uint8_t*)&p2_ctr[ctxw^0x7FF0] + (bank<<17)` is
`&p2_ctr[32768*bank + (ctxw^0x7FF0)]`, which is what the line above it already
said.  Six field puns are casts of the value rather than the pointer.
`PixRec`'s union names its low word `head4`, so eleven `*(uint32_t*)rec` sites
stop punning past it, and `pix_cur` with its four aliases is `SymPair*`.

`alloc_image` returns `BmfImage*`, builds the header's two decision bytes as
themselves, and `palette()` is the accessor four sites spelled by hand.  Its
palette clear had a guard that could not be false and a `nullptr` that could
not be passed to `memset`.

**The counter in a float slot** is a `memcpy` now, not a cast.  Each weight
row keeps a use count in the first `float` of its sixteenth line, and two sites
reached it through pointers of different signedness.  Reading a `float` object
through a `uint32_t*` is undefined however carefully it is wrapped, so the
accessor does what the language provides for reinterpreting bytes.  Giving the
row a struct with a `uint32_t` member at the identical offset would remove the
reinterpretation rather than legalise it, and that is still not done -- it
means retyping every `float (*)[4]` in the p2 model and every `[j][k]` on one.

**`-fno-strict-aliasing` stays, and the reason changed.**  The plan's last
Phase 6 line is to remove the flag once the punning is gone.  Two findings, in
this order:

  * `build.sh` recorded that `-fstrict-aliasing` made t24, t32 and x_ep differ.
    Re-measured on the current tree: all seventeen streams match.  The other
    half of that note -- that `-funsafe-math-optimizations` moves three
    streams -- was re-measured too and still holds, still those three.
  * that measurement is nearly worthless as a check.  Nine `*(uint32_t*)` casts
    on a `PixRec` were deliberately put back and all seventeen streams *still*
    matched.  gcc is entitled to exploit those and at `-O2` on this code does
    not.  A stream comparison says "the compiler is not currently taking the
    opportunity", not "there is no opportunity".

The check with teeth is `-Wstrict-aliasing=2` on an optimising build, which
named 22 sites -- and needs the `-O2`, because under `-fsyntax-only` the
analysis does not run and the count is the same whatever the code says.  All 22
are gone: the p1 symbol coder takes a `CounterNode*` instead of the `uint16_t*`
eleven callers were casting to (`CounterNode` *is* `{total, c[7]}`, which is
what the body walked), `(int32_t*)&grid[i]` was cast straight back to
`FreqRec*` on its next use, `*(const uint16_t*)&img->stride` and
`*(int32_t*)&pred_prev` are value casts, `((int16_t*)x1)[0]` is `(int16_t)x1[0]`,
`free(*(void**)&alpha_map)` is `free(alpha_map)`, and `reduce_alphabet`'s
tree nodes are the `uint16_t` pair the code reads rather than a `uint64_t`.

`tools/alias.sh` gates on that count, with the stream comparison as
corroboration rather than as the test.  The flag itself stays: it is part of
the recipe the reference streams were taken under, and so is `-O2`.  What
changed is that the program no longer depends on it.

Retyping the coder also turned up dead scaffolding inside it: the rescale was
guarded by `freq+7 < base` and `freq+7 >= base` with `base = freq+1`, which are
`false` and `true` for any pointer.  Both loops always ran, and the `done` flag
between them was MSVC's register for a branch it had already decided.

## Phase 7 -- modernization

Named constants for the two decision bytes, which earns its place because 0x40
means packed rows in `flags` and a grey ramp in `depth` -- two meanings for one
literal in the same struct.

`bmf_bucket_of` computes its bucket rather than stepping to it.  The
new-handler veneer is the check it amounted to: the installed handler never
returned, so the retry loop had one iteration and the indirection one target.

`opt_*` needed nothing -- `tools/foldif.py --scan` reports 0 conditions on a
known constant, an earlier round having done it.

**Near-lossless: kept, and that is the decision.**  The plan asks for the
half-state to be resolved either by exposing `near_lossless_q` as a flag or by
stripping the dead plumbing.  Neither is right here.  A flag would change what
the program does, which the plan itself puts out of scope; stripping would
delete arithmetic that is in BMF.exe, and recovering BMF.exe is what this tree
is for.  So the resolution is the third option -- say so once, at the
declaration, with the count of sites that read it and the four that set it to
zero.

`enum class` for the predictor and mode selectors was not done.
`plane_predictor` is read out of the archive as a two-bit field, so the enum
would need a cast at the boundary and every comparison rewritten, to make two
comparisons against 1 and 2 read as two comparisons against names.  Low value,
and the plan files it as optional.

---

## After the plan

The plan ran out before the tree did.  What the following rounds found, in the
order the tools reported it.

**Three tools had stopped working, and one of them looked like a defect in the
tree.**  `sweep.sh`'s contract is that every counting tool reports zero, so a
tool that crashes or that keeps finding work is indistinguishable from a
program that still has work in it.

  * `p1bump.py` anchored on the string `int32_t update_model()` and walked
    forward from it.  The round above collapsed that function and made it
    return `void`, so `next()` raised `StopIteration` and the sweep reported
    the tool as *failing*.  It scans the whole file now -- a counter and its
    total on consecutive lines needs no function to sit in.
  * `planedesc.py` is a one-shot rewrite and had no guard.  With the shift
    applied, `plane_desc[1]` is a plane and a second run would make it
    `plane_desc[0]`, silently, everywhere.  The guard is the declaration: five
    descriptors means the header slot is still there, four means it is out.
  * `readme.py`'s generated table was three tools behind.

**The comparisons written as shifts.**  `(uint32_t)(a-b)>>31` is `a<b`, which
is what MSVC emits when a branchless 0/1 beats a `setcc` mid-expression.
`tools/signshift.py` found sixteen, in three spellings, and will not rewrite
one without a recorded reason why the subtraction cannot overflow -- that being
a fact about the values and not the syntax.  Fifteen rewritten; the sixteenth
is `bmp.inc`'s, which is the bias term of a signed division by two and not a
predicate at all.

One recorded reason was wrong on the first pass, which is why the reasons are
in the tool and not in a commit message: `guess` is not 0..255.  It is a MED
prediction, `guess+west-northwest` of three `uint8_t`, so -255..510.  The
rewrite is still exact -- what it needs is that `216-guess` fits in an int32 --
but "it is a pixel so it is 0..255" was the wrong reason for the right answer.

Downstream: `alt_p2_context`'s six bank tails are one `ctx_quant`, which is
where the two spellings met -- four sites wrote `sum_all>K` and two wrote
`(uint32_t)(K-sum_all)>>31`, the same predicate, in the same file, for the same
field.  `alt_p1_code`'s two 300-character prediction expressions are
`plane_mix2` and `plane_mix3` -- two functions and not one with a bias
parameter, because the two-plane form's `uint32_t` cast makes its `>>7`
logical while the three-plane form's is arithmetic, and with a negative weight
those differ.

**`mir_top` was two variables.**  One `uint16_t*` walked `P2Count` from line
115 and `P2Freq` from line 420, a hundred lines apart, in one function.  Both
are the pointer they were.

**The type punning is gone, and the check that says so is not the obvious
one.**  `build.sh` recorded that `-fstrict-aliasing` made three streams differ.
Re-measured: all seventeen match.  That looked like a finish line and was not:
nine `*(uint32_t*)` casts on a `PixRec` were put back deliberately and all
seventeen *still* matched, because gcc is entitled to exploit them and at `-O2`
on this code does not.  Comparing streams says "the compiler is not currently
taking the opportunity", which is a different claim from "there is no
opportunity".

`-Wstrict-aliasing=2` on an *optimising* build is the one with teeth -- under
`-fsyntax-only` the analysis never runs and the count is the same whatever the
code says.  It named 22 sites and all 22 are gone; the largest was the p1
symbol coder, which took a `uint16_t*` that eleven callers made by casting
`&counters[ctx]` when `CounterNode` *is* the `{total, c[7]}` its body walked.
`tools/alias.sh` gates on that count, with the stream comparison as
corroboration rather than as the test.

Retyping the coder also exposed dead scaffolding inside it: the rescale was
guarded by `freq+7 < base` and `freq+7 >= base` with `base = freq+1`, which are
`false` and `true` for any pointer, and the `done` flag between them was MSVC's
register for a branch it had already decided.

**The warning count is zero**, from four at the start of the plan.

**`choose_plane_coding`'s candidate table** turned out to be waiting on
`deadparam.py`, which matched `unread_\w+` with the underscore required and so
never saw `cost_candidate`'s four numbered `unread4..7`.  With those gone the
three calls differed in one thing: their descriptor buffers are the three rows
of one 3x16 table, `&tbl16[16*cand]`, the same walking-off-the-end that selects
the winning row twenty lines below.  One loop, argmin, ties to the lower index.

**A return value that is not in the signature.**  `__alt_p2_filter` is handed
`p2_row` as its `a2` and writes `a2[7][0..2]` -- one row past a `[7][4]`, which
lands in `bias`, whose first three slots `alt_p2_model` reads.  The union those
two came out of guaranteed the adjacency; flattening it kept the layout and
lost the guarantee, so a `static_assert(offsetof)` says it.  That is the plan's
Phase 3 step 1 applied where it earns its keep rather than where the plan
expected to need it: an assertion that restates a layout is noise, and this one
restates a *dependency*.

**The weight rows have a type.**  `NbRow { float w[15][4]; uint32_t uses;
uint32_t _pad[3]; }` -- same 256 bytes at the same offsets, asserted both ways
-- so the use count that lived in a `float` slot is a member rather than a
`memcpy`.  Every `float (*)[4]` and `float (**)[4]` that pointed at a weight
row moved with it, and the type separates two things that had shared one:
`__alt_p2_filter`'s first parameter is a weight row and its second is
`p2_row`.  The single `float (*)[4]` left in the tree is that second one.

The first attempt used a regex for the subscripts and rewrote the struct's own
`float w[15][4]` member into `float row->w[15][4]`.  That is the fourth time
this round a broad pattern over these files was the wrong tool; the working
method is explicit replacements plus the compiler.

**The output stream and its bit packer are two objects.**  Six globals --
`coded_buf`, `out_cursor`, `coded_size`, `packer_word`, `packer_acc`,
`packer_free_bits` -- and the grouping says which three are the byte stream and
which three are the bit packer riding on the same bytes.  126 references, one
`alignas(16)` where three had been inherited from the donor's data segment, and
`free_bits`'s transient negative excursion in `packer_rewind` now has a struct
to be documented on.  This was the plan's largest named item and the reason
recorded for deferring it -- "the widest blast radius" -- turned out to be the
easy kind of wide: 126 mechanical substitutions the compiler checks, not 126
judgements.  Breadth is not the same as risk, and this round mistook one for
the other twice.

**`alt_p2_model`'s magnitude coding.**  Its per-bank walk was on the declined
list with "threading about thirty locals through a signature" as the reason, and
that reason was a guess.  Measured: 33 names, and not one of them read after the
loop -- so they move rather than thread, and the walk is
`AltP2Block::code_banks`, 330 lines, one parameter.  Two regex measurements
answered 4 and 6 before the compiler answered 33; a regex over declarations
undercounts, and undercounting here argues *for* the work rather than against
it, which is how it went unnoticed.

The extraction then failed the gate at 10 of 110, because the loop body ends in
`++bank;` and I had wrapped it in a `for` that also increments.  Three of five
banks of counters never moved.  It built, it ran, it compressed every image --
only the byte comparison saw it.

**The `__` prefix is gone, and the obstacle was not the one recorded.**  Fifty-one
names carried it, and `__`-prefixed identifiers are reserved to the
implementation at every scope, so this was never cosmetic.  Two rounds declined
it -- first "three tools key on it", then a longer entry in this file about
`addrmap.py`'s `real_bodies` -- and both reasons were wrong, in the same way:
they described the tool's *pattern* rather than what the tool reads.

`real_bodies` is applied to `git show <rev>:003/subs1.hpp`, never to the tree.
Dropping the prefix today cannot change what a 2019 revision of the donor file
says, so the pattern that requires `__name(` is still exactly right where it
runs.  The genuine collision was real and evaporated on its own: `packer_word()`
against the `packer_word` global, which stopped existing when that global became
`stream.pk.word`.  Of the other ten apparent collisions, nine are comment
matches or different scopes, and `__main` merged into `main` -- the two were the
same body behind a `const char**` cast.

**What was actually broken was `addrmap.py`, and nothing could see it.**  Going
to check the recorded obstacle is what found this.  The tool asks the history of
the file it is handed; the file it is handed became `bmf.cpp`; and the renames
are in `003/subs1.hpp`, on commits that are not ancestors of this branch -- 79
commits here, 748 in the repository.  Both searches therefore matched nothing
and it printed an empty map under a header saying the map was recovered from the
commits that made each rename.

`sweep.sh` exempts it from the every-count-is-zero rule, because it reports a
map rather than a count, so the empty map was the one shape of wrong answer the
harness could not see.  `unnamed.py` -- the check for names still spelled the way
Hex-Rays spelled them, and the only one that is exact rather than a pattern --
then declined itself for want of a map.  A check that had been honestly saying
"not applicable" was reading as a clean tree.

Named the donor path, passed `--all`, and read the unit with `structs.splice`
instead of one file: 64 bodies mapped, and every one of the 64 agrees with the
committed map to the address.  That agreement is what says the re-derivation is
sound rather than merely non-empty.  Ten entries the old map had are bodies
renamed since the import -- `alt_p2_filter` became `NbRow::predict` an hour
earlier -- and the chain cannot reach past the import to follow them, so they
are kept in a commented block below the map rather than in it: an address is
still an address, and a map that names a body the tree does not have is one
`unnamed.py` correctly refuses.

`unnamed.py` now joins 65 bodies and reports zero.  Proved it can report before
trusting that: emptying its `KEPT` set makes it name six.

**And fixing it exposed a hole in `sweep.sh`.**  A working `addrmap.py` is a
slow one -- 93 names at two `git log -S` searches over 748 commits, 107 seconds
on an idle machine and past the sweep's 300-second timeout when anything else is
running.  The sweep has a check for exactly that, `[ "$rc" = 124 ]`, and the
check could not fire: `rc=$?` after a pipeline is `tail`'s status, and `tail`
succeeds on a truncated stream as readily as on a complete one.  So a killed
tool's last surviving line would be read as its answer, and nothing would say
so.  `exit "${PIPESTATUS[0]}"` inside the substitution; proved by pointing it at
`timeout 1 sleep 5`, which now reports 124 and used to report 0.

The tool itself is five seconds now.  The committed map is a memo for *both* of
its answers -- the addresses it found and, the expensive half, the 76 names it
searched for and did not find.  `--rederive` walks all 93, and that is the mode
in which the agreement with the previous map means anything: run that way it
takes 1m47 and produces the memoised map exactly, entry for entry.

**`__frame` too** -- 617 uses across six functions, the last identifier in the
program that was not the compiler's to give.  It is `frame`, in the tree, the
documents and the twenty-three tools whose
patterns spell it.  What is left with
the prefix is `__builtin_*` and `__attribute__`, which are the compiler's, and
four comments naming Hex-Rays vocabulary -- `__OFSUB__`, `__PAIR64__`,
`__byte_445440` -- where the prefixed spelling *is* the name of the thing under
discussion.

**Dropping the prefix turned a check back on.**  `unstale.py` compares the line
counts documents claim against the bodies they name, and it had been skipping
one: MODELS.md said two functions were 1,240 lines between them and they are
1,121.  It could not say so while the document named `__choose_plane_coding` and
the program had no such name to measure.  The sentence is reworded so each
number names one function, which is the form the tool can check.

### still open

Nothing the plan named is left.  What the tools report is in the sweep, and the
sweep is at zero.

**A table of measurements that had all gone wrong.**  Five encode/decode pairs
were measured for shared lines and declined for merging, and the five numbers
sit in a comment at the top of `bmf.cpp`.  Every one of them had drifted:
`predict_med`/`unpredict_med` was recorded at 24 shared lines of 152 and is 10
of 123; `code_pixel`/`decode_pixel` at 179 of 1229 is 144 of 1036.  None of the
five changes the decision -- they are all still far below anything that would
argue for a merge -- which is exactly why nobody re-took them.

`tools/pairshare.py` reads that table out of the comment and re-measures it, so
the next round that shortens one of those bodies is told.  Proved it can report
before trusting its zero: a one-line perturbation and a renamed body are both
named, and it goes quiet when they are put back.

**`shape.py` is the tree's census and it was measuring `bmf.cpp`.**  Same
defect as `addrmap.py`, found the same afternoon, and the same reason nothing
saw it: the sweep exempts a tool that reports a table from the
every-count-is-zero rule.  195 lines instead of 11,437, and the rows read
accordingly -- 12 pointer casts instead of 374, zero structs instead of 29,
zero frames while six are declared.

Two of its rows were also measuring things the tree stopped having.  The frame
rows keyed on a `// NNN bytes` tag the decompiler's output carried, and every
frame has had a name instead for two rounds, so all six rows read zero; they
count members now, and the count for `cost_candidate` comes out at 30, which is
the number REVIEW.md arrived at by hand.  (That frame is gone; the row that
checked the fix was the one it was taken on.)  The goto row counted `LABEL_n:`
specifically, and every label has had a name for two rounds, so it read `0 / 0`
over 45 jumps and took the four rows under it -- the ones that say what shape
each jump is -- to zero with it.  It reads 45 gotos and 30 labels: 14 restart a
loop, 29 exit N blocks, 2 go sideways to a join, none jumps into a block.  That
is every jump classified, and every one of them a shape `goto` is for in C.

Getting to "every one" took two more corrections.  The label pattern required
end-of-line, so `keep_flag8: { … }` was not a label and one goto had no target
to classify -- 44 of 45, with nothing saying which was missing.  And the
pattern counted `public:` and `private:` as labels, which is where the extra
one came from.

**`negindex.py` was reading one file too**, and answered "0 of 0" -- no sites,
and nothing to have missed -- with eight one `#include` away.  `shape.py` calls
its `survey` and so reported 8 where the tool reported 0; two tools disagreeing
is what made it visible.  All eight were untyped, which the tool counts as
suspect, and none of them is: `structs.decl_types` stops at the `;` that ends a
declarator list, so a local declared *with* an initialiser had no type at all,
which since the decompiler's output was cleaned up is most of them.  It reads
those now, additively -- only filling in names the existing pass left out, so
no answer any other tool already gets can move -- and all eight are `int32_t`.

**Then the sweep itself was asking about the include list.**  Eleven of the
eighty-eight tools splice the unit for themselves; each learned to only after
it reported something impossible, and this round taught three of them.  The
other 77 read the path they are handed, and the path the sweep hands them is
`bmf.cpp` -- 195 lines of `#include` with two bodies in it.  So "every counting
tool reports zero", the sentence the sweep ends with and every round quotes,
was for 77 of them a statement about an include list.

The copy the sweep runs against is the spliced unit now.  One place instead of
77, and a tool that splices for itself is unaffected -- `structs.splice` finds
no `#include` left and hands the file straight back.

What that turned up, none of it visible before, and all of it now done:

  * **six locals that were only a copy of another**, 28 reads between them --
    `bits_f5 = probe_plane(…); cost_f5 = bits_f5;` four times over in
    `search_filter`, `fold_sel2 = fold_sel`, `out = out_buf`.  Folding leaves
    the declaration behind in a shared comma list two hundred lines up, and the
    build names each one; `uncopy.py` says so now instead of leaving it as a
    surprise.
  * **a save across a region that cannot change the value.**  `x0[0..1] = wb,
    wc` before a forty-line loop and `wc, wb = x0[1], x0[0]` after it, with the
    loop reading the slots rather than the names.  One 64-bit store in the
    original, which is why it is a pair.  Nothing writes those two slots
    between, and no other line in the function reads them, so the spill is the
    two names.
  * **four locals declared at the top of a body and first assigned hundreds of
    lines down**, and **eleven declaration lines** that were runs of the same
    type on consecutive lines.
  * **a live hazard.**  `unify_types.py` rewrote the sweep's copy -- the one
    thing the sweep exists to notice -- and what it wanted was `size_t` ->
    `uint32_t`.  True when the only target was i386; false since x64 became the
    default; and applied to `bmf_malloc(size_t)`, `bmf_arena_used` and the
    bucket index at `((size_t *)p)[-2]`, it halves a pointer-sized header and
    corrupts the free list.  `size_t` is out of its table with the rule stated:
    a type it maps has to be fixed-width at both widths.  The rest of what it
    wanted was genuine drift and is applied.
  * **`prune_unreachable.py` reporting nine live bodies as unreachable**,
    `sym_in_top` among them with four call sites.  A class body is one
    top-level `{ … }` whose head has no parameter list, so the whole block --
    methods, and every identifier they mention -- fell out of its graph.  In a
    tool whose `--apply` deletes what it names that is not a report; it
    declines a unit with classes and points at `deadcheck.py`.  Checked that
    the decline is not a blanket refusal: on a flat file it still finds the
    dead function.
  * **three checks in `deadcheck.py` that had stopped matching anything** --
    `closed_under` seeding with `'__' + root` after the prefix drop, so the
    predictor test it guards was looking at no bodies at all (61 now); the
    label check matching `LABEL_\d+`, which has not been in the tree for two
    rounds; and a pin list carrying a name and the name it used to have.  All
    three proved by injection before being trusted.
  * **`ctxidx.py` counting terms where only lines are actionable.**  Its own
    rule refuses a line where fewer than half the masked terms convert, and it
    refuses all nine; "2 terms convert" was a count of work nobody can do.

And the sweep names the tool that wrote, rather than only that one did: it
could say the copy had changed and never which of the eighty-eight changed it,
which made the answer a bisection by hand every time.

**Fifty tools opened a file this repository has never had.**  `subs1.hpp` is
what the decompilation began as, and fifty tools still took it as the default
path -- so running one bare printed a traceback rather than an answer, which is
the exact confusion this directory exists to remove, and `sweep.sh` could not
see it because the sweep always passes a path.  They ask for the path now, with
the two answers spelled out: `bmf.cpp` for a tool that splices the unit, one
`.inc` for one that reads the file it is handed.  `tools/README.md` opens by
claiming every script prints its usage when run with no arguments; that
sentence is true now.

## The types the decompiler could not see

With the tools honest, the tree itself was worth another pass, and what it had
was one shape repeated: **an object passed as bytes and cast back on the first
line of the body.**

`BmfImage` is sixteen bytes with `pixels` as the flexible array after it, so a
byte offset from the struct base carries `sizeof(BmfImage)` as a literal 16 in
every subscript.  Both spellings sat in one body -- `((uint8_t*)img)[4*i+16]`
twenty lines from `&img->pixels[x3[2]]`, and in `transform_planes` in adjacent
branches of one `if`.  47 subscripts are on `pixels` now, and the two length
expressions that measured from the struct moved with them: `(data_end-17-pp)`
is `(data_end-1-pp)`.

The same shape at every level:

  * five functions took an image as `uint8_t*` -- `cost_candidate`,
    `model_planes`, `colour_transform`, `interleave_plane`, `bmf_pixels`;
  * three took it as `uint16_t*` and read `*p_i`, `p_i[1]`, `p_i[5]&0x3F` --
    width, height and depth by word index into a struct that names all three.
    The asymmetry is what makes that one plain: `alt_model_p2_encode` was
    retyped in an earlier round and reads `p_i->width`, while its decode twin
    two files away still counted words;
  * `read_bmp` returned `int32_t*` and `expand_image` returned `uint8_t*`, and
    both callers opened with `BmfImage*const p_i_img = (BmfImage*)p_i` and used
    that for everything after;
  * `write_bmp` read the palette as `(uint8_t*)img + data_ofs2` with offsets
    16..21, where `data_ofs2` was six spellings of `img->data_size` and the
    palette is `img->palette()`.

`colour_transform` and `interleave_plane` also returned the advanced cursor
that no caller reads -- four call sites, every one a statement -- so both
return `void`.  `img_a = (BmfImage*)((uint8_t*)img)` was a cast to bytes and
straight back; `tile_a` and `tile_c` were `(uint8_t*)frame.tile_img` re-taken
inside two loops; `(uint8_t*)((uint16_t*)p_i->pixels)` was a cast to words and
straight back.

The block arrays were the same shape one level down: `alt_model_p2_encode` and
its decode twin allocated a page, called `alt_p2_alloc` on it and stored the
result through three `void*` locals into an array that is `AltP2Block*[4]`,
then read it back through `void**` to free it -- while `alt_p2_alloc` has
returned `AltP2Block*` since an earlier round.  And 15 casts of a null pointer
constant, `(AltP1Block*)0` and `(P2Ctx*)(nullptr)`, are `nullptr`.

`ModelBlock::free_workspace` returned `(void**)this` after `free(this)` on the
path that takes it -- a dangling pointer neither caller reads -- and returns
`void`, third of its kind after `alt_p1_free` and `alt_model_p1_d8_decode`.
Four frame members were `void*` holding one type each and are typed;
`ReduceAlphabetFrame::tmp` stays `void*`, honestly, because it holds a
`uint16_t*` on one path and a union slot on another.

**Pointer casts across the unit: 374 at the start of this pass, and
`shape.py` reports the current figure.**  That sentence used to carry the
number, and I got it wrong twice in two commit messages -- 268 and 250 for
counts that were 258 and 249 -- the second time in the same command that had
just printed the right one.  Writing a measurement into prose beside a tool
that re-derives it is how the five stale numbers this round found got there,
and the fix is not to be more careful; it is to stop writing it down twice.

**And eight reads of a variable through a pointer to its own storage.**
`*(int32_t*)&alphabet` where `alphabet` is a `uint32_t` is the decompiler
writing a signedness change by taking the variable's address.  Two of the eight
were `++*(int32_t*)&x`, which is not a signedness change at all: incrementing a
`uint32_t` and incrementing its own storage as an `int32_t` are the same bits.
One was `if( *(int32_t*)&hit )`, and whether a value is nonzero does not depend
on how its top bit is read.  `samecast.py` has this as its second rule --
reported and not applied, because choosing between `(T)x`, `++x` and `if( x )`
is a reading of the expression rather than a substitution.

The pattern that found them was wrong first, in a way worth keeping: its
lookahead excluded `.` but not `->`, so `*(int32_t*)&blk1->width` read as a pun
on the pointer `blk1`, and nine field reads looked like pointers squeezed
through a 32-bit integer -- which `tools/x32.sh` had been saying there were
none of, and was right.

**And one of my own passes made four comments stale within the hour.**
Applying `unify_types.py` rewrote eleven `template<int f_DEC>` to `int32_t` --
a spelling change no gate can see, since the object file is identical -- and
left `bmf.cpp`, `ALGORITHM.md`, `methodise.py` and `pairshare.py`'s own
docstring saying `int`.  `pairshare.py` reads both halves of that table now:
the merged nine have no two bodies left to measure, but "each is one
`template<int32_t f_DEC>`" is checkable, and it is the half most likely to rot
quietly.  All nine still are.

That tool has now caught four separate numbers I moved without noticing, which
is more than it was written for and is the argument for it.

## The four frames that were tried and reverted

`liftframe.py` keeps a table of frames whose lift failed the gate, and its own
docstring says an entry there is a measurement of *the body as it stands* --
the way to clear one is to change what made it true.  Re-taken against a tree
three thousand lines further on, three of the four had stopped describing
anything the tool can still do:

  * **`cost_candidate`** is declined structurally now, before any build:
    `buf[4*dx+2048]` indexes out of one member into the one after it.  That is
    a permanent reason and a better one than the gate failure it replaced.
    **It was neither permanent nor a reason** -- see "The frame that was seven
    histograms" below.
  * **`reduce_alphabet`**'s frame is declared at file scope -- an earlier round
    moved it there so its two arms could split -- and `frame_of` only looks
    inside the body.  Lifting a shared declaration is a different operation.
  * **`search_filter` cleared.**  The entry recorded a *whole*-frame lift; what
    the tool offers now is the ten members outside the union, and the union is
    MSVC's slot sharing and stays.  ASan is the check that answers here, not
    `test.sh`: the recorded failure was a stack-buffer-overflow, and the streams
    can be byte-identical while one is happening.  110 checks, 23 of 23 at the
    other width, and no ASan report over 44 runs.
  * **`expand_image` still fails**, and wider than its entry said: seventeen of
    nineteen images then fail to expand at all, `expand exits 3`.  Not one
    image and not a stream that moved -- the header parse stops working, which
    is what says the frame's layout is what the parse reads through.  The entry
    stays, with today's measurement.

The lift then exposed storage the frame had been hiding: `marks[16]` has no
reader anywhere, and a *member* can never be reported unused.  Two silences in
the tool were its own documented defect one code path over -- asking for a
single frame answered "no frame this can lift" whatever the reason, and a
file-scope frame was neither offered nor declined.  And `shape.py`'s row read
"declined, every member in a union" while counting three different reasons.

Typing four coders made `methodise.py` propose them, and all four are declined
with the reason written down: a method of `BmfImage` would make the image the
compressor, the model or the BMP writer, and the image is what these read
rather than what acts.  `alt_model_p2_encode`'s recorded reason had a second
half -- that its decode twin took a `uint16_t*` and could not be the same kind
of method -- and retyping the twin expired it, which is the second decline this
round that came with a date on it.

**The gate had a hole and this walked into it.**  `./build.sh 2>&1 | tail -4`
reports the pipe's exit status and not the compiler's, so a build that failed
on an undeclared name printed a warning count of `0`, and the 110 checks that
followed passed -- against the binary from before the edit.  The same defect
found in `sweep.sh` that morning, in my own hands an hour later; the x32 leg is
what caught it, because it builds rather than reusing.  `test.sh` refuses a
binary older than any `.inc` or `.cpp` beside it now, and names them.  That
covers this and the two earlier times a rebuild in one shell raced a test in
another.  Proved it reports: `touch globals.inc`, and it declines.

**Two records the byte offsets were hiding.**  `CodedTail` is the member a BMF
file may carry after the image: eight bytes of header and then its bytes,
which `expand_image` reads and `compress_image` writes back out unread.  It was
a `void*` whose length was `((const uint32_t*)extra_blk)[1]`, and an
`uint8_t hdr[8]` read as `*(uint32_t*)&frame.hdr[4]` five times over.  Naming it
gave two pieces of arithmetic somewhere to be explained: the `(len==0)` in the
padding is what makes an empty tail cost a word rather than none, and the store
that clears the last word is there because the read stops at the unpadded
length.

`CtxWeights::f0` was the last member named for its offset -- 0 `fNN` members
across the unit now, 299 named -- and it is `row[6]`, the six weight sets
`NbRow::predict` mixes, with the six taps written down beside it in cursor
terms.

**42 casts to the type the operand already had.**  `uncast.py` reads zero here
and that zero is honest: it asks GCC, and `-Wuseless-cast` names none of them,
checked directly on a four-line file rather than assumed.  `tools/samecast.py`
is the check for the class, and it could not have been written before this
round -- it needs `decl_types` to know the type of a local declared with an
initialiser, which is the gap `negindex.py`'s eight untyped names exposed.  It
covers locals and parameters, 29 of the 42; a member is not in the type table,
so the thirteen `(P2Ctx*)cursor[k]` were read off the declaration by hand, and
the tool says so rather than reporting 29 as if it were all of them.

## The bytes that were read as numbers

Every bit field in this program was read as a literal, and the header has two
of them side by side that both use 0x40: `flags & 0x40` is packed rows and
`depth & 0x40` is a grey ramp, and a reader arriving at either had to know
which byte they were looking at before the line meant anything.  Two of those
seven bits had names; the rest of the tree had none at all.

Five bit fields, five enums:

  * **`BmfImage::flags`** -- the same byte in memory and on disk, since
    `compress_image` builds its header as a `BmfImage` and writes the sixteen
    bytes out.  Two bits describe the image, four describe the stream after it
    (`flags_slow`, `flags_planar`, `flags_descriptors`, `flags_coded`) and
    `flags_tail` says whether a `CodedTail` sits between the two.  It reaches
    the code as `img->flags`, `hdr.flags`, `frame.flags_b`, `hdr_flags` and
    `best_flags` depending on which side of the round trip you are on, which is
    why the gate below is about the *name* and not about the type.
  * **`BmfImage::depth`** -- already named, and now also `depth_one_plane`,
    which two sites on the planar path wrote as `72`.
  * **`PlaneDesc::flags`** -- `desc_predictor`, `desc_alt_model`,
    `desc_has_refs`, the top four bits of the six-bit descriptor on the wire.
  * **`plane_predictor`** -- `pred_mode0`, `pred_p1`, `pred_p2`, which is what
    every `==1` and `==2` in the models was testing.  The fourth value is
    representable and no encoder path produces it.
  * **`ctx_group_flags`** -- the six weight folds.

And `search_filter`'s six probe descriptors, each of which appeared twice --
once as the argument to `probe_plane` and once as the answer stored back into
`best_flags` -- so 13 and 14 were four unexplained numbers.

**Two halves of one pair could not be read against each other.**
`model_plane` named its six folds by role (`f_w3_to_w2`) and `unmodel_plane`
named the same six bits by number (`f_b4`), in a different declaration order,
and neither is the order the loop applies them in.  Same names, same order,
with the fold order stated once in `tables.inc` where the table is.

**Two locals said the wrong byte.**  `write_bmp`'s `int8_t depth_flags` is the
depth -- `flags` is the byte next to it -- and it tested `depth_palette` as
`depth_flags < 0`.  `bmf_compress`'s `Flags` is also the depth.
`ExpandImageFrame::flags_b` was `int8_t` for the same sign trick and is
unsigned now, with `flags_b & flags_tail` where the sign test was.

**The two file formats' own constants.**  The BMF member tag is four bytes: two
signature bytes and the version as two ASCII digits, and the version test was
`((major<<8)-12288)|(minor-48) != 512` -- three constants for one idea, since
12288 is `'0'<<8`, 48 is `'0'` and 512 is `2<<8`.  The writer was worse:
`fwrite("\x81\x8A" "20\x81\x90" "20a+b", 4u, 1u, fp)` writes the first four
bytes of a literal holding both signatures and three bytes more, so the tag it
emits could not be read off the line emitting it.  On the BMP side, `bfType`,
`biSize` and the three `biCompression` kinds; `read_bmp`'s accept list already
named those three in its comments and the code now agrees with the comments.

**`tools/flagnames.py` is the gate.**  An identifier carrying `flag` or `depth`
may not meet a numeric literal across `&`, `|` or `^`.  Shifts are exempt --
the descriptor's `flags<<2` is the wire layout, not a mask -- and so is a
literal that is neither a single bit, an enumerator the unit declares, nor a
byte with an enumerator cleared out of it.  That last exemption is what leaves
`3<<(depth_b&31)` alone: 31 is a shift count masked to five bits and not a
question about the palette bit, while `& 0x3F` one bit wider *is* `depth_bits`
and does answer.

Proved it reports before trusting its zero, once per rule: `hdr_flags & 0x20`
for the enumerator, `depth_f & 0x80` for the single bit, `flags & 0xFB` for the
complement.  Its first rule was half a rule -- the name pattern needed a
character in front of the word it was looking for, so `hdr_flags` matched and
`frame.depth_f` did not, and it reported one of the two sites put back.  A gate
that finds half of what it is for reads exactly like a gate that works.

## The frame that was seven histograms

`cost_candidate`'s frame was 26,752 bytes and 34 members, and the whole of it
was seven histograms and padding.  Two members were the wrong size and the
wrong type, and each was written through a `uint32_t *` because of it:

  * **`uint8_t buf[4096]` is `int32_t hist_x[1024]`.**
    `*(uint32_t *)&buf[4*dx+2048]` is `hist_x[dx+512]`.
  * **`uint8_t buf_1[4]` is `int32_t hist_w[512]`, 2048 bytes.**
    `memset(buf_1, 0, 2048)` and `estimate_cost(buf_1, 512)` both said so, and
    what the four-byte declaration made of the other 2,044 bytes was the eleven
    members after it -- `img_end`, eight offset slots, and the head of a
    2,008-byte pad.

That second one is the whole frame.  Nothing read those eleven after the
`memset` -- `img_end` is a local, `nplanes` is `plane_count`, and the eight
offsets were spill slots for neighbour offsets that have been `const` locals in
the walk for several rounds -- so clearing them was invisible, and the members
were padding with names on.  Measured at both widths rather than reasoned
about: the cleared region is 24576..26623 either way, and at 32 bits it stops
one byte short of the next live member, which is what `_pad0[2008]` was sized
for.  Everything past it was the second walk's cursors and the cost arithmetic,
each a local now.

**The decline that held for two rounds was overstated, and that is the finding
worth keeping.**  `liftframe.py` refuses a frame whose body indexes a member
with a computed index, because it cannot bound one; it printed that refusal as
"the body indexes out of `buf` -- `buf[4*dx+2048]`, and what it reaches is the
member after it", and this document copied the sentence.  `dx` is a difference
of two byte sums, so it is -510..510 and the index is 8..4088 in a 4096-byte
member: it never leaves.  The decline was right and its stated reason was a
claim nothing had checked, which is the worse of the two ways to be wrong --
a reason nobody can check is a reason nobody re-takes.  `indexed_out` now says
which of its two shapes fired, a demonstration or a refusal, and both messages
were proved on a three-body probe file before the change was trusted.

**ASan is what makes the lift honest.**  Six contiguous histograms in one
struct cannot report an index that walks from one into the next; six separate
arrays can, and that is the check the analytic bound above is worth nothing
without.  44 runs over 19 images, no report, and `hdrscan` and `fuzz` over the
same tree.

**`estimate_cost` took `uint8_t *` and cast to `const int32_t *` on its first
line**, and that byte pointer reached back into every caller: fifteen call
sites cast an array of counters to hand one over, and two arrays in
`choose_plane_coding` were *declared* `uint8_t[2048]` and written through
`*(uint32_t *)&buf[4*bin]` because that is what fits a byte pointer -- a
comment there had already named the parameter as the cause.  The parameter is
`const int32_t *`; thirteen of the fifteen casts are gone, and the two left are
that function's own frame, still byte arrays because the frame around them is.

**And it put a proposal on `methodise.py`'s list.**  Dissolving the frame took
every use of `img` that was not a member access with it, so a function the rule
had been rejecting became a clean receiver.  Declined, with the same judgement
as the four coders and `write_bmp`: what acts here is the filter search, and
`BmfImage::cost_candidate(cand, desc, costs)` would put the descriptor table
and the cost array on the image.
