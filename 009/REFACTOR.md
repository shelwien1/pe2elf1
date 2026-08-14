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

Every item was gated on the fifteen streams byte for byte at both pointer
widths -- `./build.sh && ./test.sh`, now 110 checks over 17 images, and
`tools/x32.sh`, 23 of 23.

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

`alt_p1_free` freed `this` and returned it, and `__alt_model_p1_d8_decode`
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

  * the alphabet reaches 8193 symbols: `__rc_decode_flat(8193)` returns 0..8192
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

Also: the prefetch that asked for the cache line holding a local the next line
writes, one `w2 = w2;`, and a pointer `__search_filter` spilled to its frame
and reloaded thirteen times -- `p_i_2`, `img_c` and the `img` parameter are one
pointer.

## Phase 2 -- reference aliasing

**Done.**

`plane_desc[5]` held four planes.  Slot 0 was a header slot with three
unrelated things parked in the weight words: `plane_count` and
`near_lossless_q` were references onto two of them, and `__expand_image` parsed
the archive version word into the third -- while `__compress_image` wrote 512
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

**Two of eight dissolved; six left where the tool left them.**

`DecodePixelFrame` and `CodePixelFrame` dissolve.  An earlier round had already
recovered the neighbour array out of them, and what was left was that array,
four bytes of gap, thirty-two of pad, and no other member -- 212 uses of
`__frame.sym` are now `nb_sym`.

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

`__rc_begin` was three jobs in ninety lines.  `packer_rewind` gives the
packer's spare bytes back (and is where "`packer_free_bits` goes transiently
negative" is now written down), `begin_plane_stream` sets up the alternate
model's limits, level geometry and 1024 seeded strips, and the coder's own init
is the third.  `__rc_end`'s table free is `end_plane_stream`.

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

`__reduce_alphabet`'s narrow arm is `reduce_narrow_alphabet`.  The wide arm
keeps its two labels, which is what the plan asks for -- `code_dense` and
`alphabet_done` are a shared fast path and a convergence point, and structuring
them adds nesting rather than removing it.

**Not done**, and each for a reason rather than for lack of time:

  * `__choose_plane_coding`'s candidate table (582 lines).  The candidates are
    not uniform -- they differ in which descriptors they write and which cost
    accumulator they read -- so table-driving them needs the per-candidate cost
    body extracted first, and that body reaches six locals of the enclosing
    frame.  It is a real piece of work, not a mechanical one.
  * `alt_p2_model`'s per-bank counter walk and its magnitude coding -- 311 and
    220 of that function's 635 lines.  Both are single loops already; extracting them means threading
    about thirty locals through a signature, and a thirty-parameter method is
    not an improvement over a labelled block.
  * the `CodedStream` / session-object consolidation of the coder's globals.
    This is the largest single item left in the plan and the one with the
    widest blast radius; it wants its own round with its own gate budget.

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

`__alloc_image` returns `BmfImage*`, builds the header's two decision bytes as
themselves, and `palette()` is the accessor four sites spelled by hand.  Its
palette clear had a guard that could not be false and a `nullptr` that could
not be passed to `memset`.

**The counter in a float slot: named, not removed.**  Each weight row keeps a
use count in the first `float` of its sixteenth line, and two sites reached it
through pointers of different signedness.  `nb_row_uses` is one accessor with
one type, so the pun has one home -- but it is still a pun.  Removing it means
giving the row a struct, which means retyping every `float (*)[4]` in the p2
model and every `[j][k]` on one.  That is a wider edit than this earns, and
saying so is better than implying it was done.

**`-fno-strict-aliasing` stays.**  The plan's last Phase 6 line is to remove it
once the punning is gone, and the punning is not all gone -- `nb_row_uses` and
`mir_top`, which walks both a `P2Count` and a `P2Freq` as words, are still
there.  Removing the flag with those in place would be trading a gate for a
line in a build script.

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
