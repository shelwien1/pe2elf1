# A review of every `.inc` file

One section per file, thirty-six of them, each saying what was still ugly in
that file and what would fix it.

**Implemented.** Every section has been acted on, each change gated on the
seventeen reference streams at both pointer widths. The tree is **11,363 lines**,
down from 11,787; lines over 200 characters 29 → 22; `plane_choose`'s 53 type
puns are named doubles; and all 33 `LABEL_<n>` are gone. **Eight** sections'
verdicts changed on contact with the code, and those are marked **`corrected`**
where they appear — the review was wrong about the same proportion of its
findings that it was wrong about when it reviewed itself.

The three that changed most:

* **`bmp_read`'s `LABEL_44` is not five `continue`s.** Substituting them
  compiles, passes all 93 checks including every RLE image, and sends two of the
  five to the wrong loop — the label sits at brace depth 5 and the jumps at 8, 8,
  5, 5 and 4. Named, not restructured.
* **`advance_row` is 25 lines, not 29.** The two activity sums that follow it
  differ between the three copies: one reads two magnitudes through `(int8_t)`
  and the others as the `uint8_t` they are declared. They stay at the call sites.
* **`filter_cost`'s frame had already been tried** by `liftframe.py` and
  reverted; what it needed was not a lift but eight `const` locals.

And one gap the work opened rather than closed: `deadcheck.py` skipped `static
inline` bodies when building its call graph, so a call made *from* one never
became an edge. `p2_rescale` was the first inline helper to call a method, and
it made that method look dead from its only call site. Fixed, and the fix is
checked by injecting a body with no callers and seeing the report fire.

Every number was measured against the tree at the commit that adds this file.
The measurements are reproducible from `tools/`, or from the one-off scans the
sections name.

**What "ugly" means here.** The tree compiles clean, passes 93 checks at both
pointer widths, and every counting tool in `tools/` reports zero. So this is not
about defects. It is about the four things a reader still has to work around,
all of them the decompiler's fingerprints rather than the program's decisions:

1. **one operation written out N times** — the largest single cost left;
2. **storage that exists because a register spilled** — frame slots and scratch
   arrays that are not variables of the algorithm;
3. **a save/restore or a reload around a call that cannot touch the value** —
   dead motion the reader has to prove is dead;
4. **`LABEL_<n>`** — thirty-three labels still named after the decompiler's
   basic blocks.

| the theme | where it is worst | sites |
| --- | --- | --- |
| one operation, written out N times | `alt_p1_block.inc` | **121** |
| | `alt_p2_model.inc` | 50 |
| | `filter_search.inc` | 6 blocks × 4 lines |
| | `image_expand.inc` | 10 |
| spill storage | `plane_choose.inc` | 6 arrays, 40 double puns |
| | `filter_cost.inc` | 30 frame members, 128 uses |
| dead save/restore | `alt_p2_model.inc` | **41** |
| | `alt_p1_block.inc` | 6 |
| `LABEL_<n>` | 11 files | 37 gotos, 33 labels |

---

## The infrastructure

### `ida.inc` — 39 lines

Nothing. This is the file that got smaller every round until only two macros and
one function were left, and each has a written reason for surviving. The header
explains what went and why, which is the right shape for a file that is mostly
an absence.

### `ctxidx.inc` — 77 lines

Nothing. Five methods, each one shift and one add, each `constexpr`, with the
`digit` method carrying the experiment that justifies its existence (`bits<6,
2>(gA)` moved three streams). The only note is a positive one: this is the
model the rest of the tree should be judged against — a small type whose
comment says what went wrong without it.

### `memory.inc` — 92 lines

**Done.** **No header comment.** Every other file starts `// name.inc -- what it is`;
this one starts `#if UINTPTR_MAX>0xFFFFFFFFu&&defined(BMF_HIGH_ARENA)`. A
reader opening it has to work out that it is the allocator and that the whole
first half only exists under `-DBMF_HIGH_ARENA`. Two lines would fix it.

Otherwise sound: a bucketed free-list over an `mmap`ed arena, whose entire
purpose is to prove the program does not care where its memory is, and which is
exercised by a gate (`./build.sh -DBMF_HIGH_ARENA`, 92 of 93 checks).

### `globals.inc` — 140 lines

**Done, as comment.** **Two globals are references into an unused array element.**

```c
static PlaneDesc plane_desc[5];
static int32_t &plane_count     = *&plane_desc[0].weight1;
static int32_t &near_lossless_q = *&plane_desc[0].weight2;
```

`plane_desc[0]` is not a plane — the planes are 1..4 — so its weight fields
were free, and the original parked two unrelated globals in them. Keeping the
aliases is right, because the *layout* is the program's and the two names have
to occupy those words. What is ugly is that nothing at the declaration says
that: a reader sees two references bound to a struct field and has no way to
know it is a deliberate overlay rather than an accident. Three lines of comment,
and this stops being a puzzle.

The rest — nine globals with `alignas(16)`, the exit-code table, the small
structs — is fine. `alignas(16)` on scalars is inherited from the original's
data segment and is worth a sentence saying so.

### `tables.inc` — 65 lines

**Done: six tables now name what indexes them.** Four rows are over 200
characters and one is 427, and that: a coefficient row is one line because it is one row. `CLEANER.md`
Phase 4 explicitly declined to wrap these.

One thing worth adding: several tables are indexed by a value whose range is
stated nowhere. `bmf_p2_thresholds[6][13]` is indexed by `band` (0..5) and by a
column number that `alt_p2_context.inc` picks by hand. A comment naming the
index of each table would save the next reader the cross-file search.

### `decls.inc` — 95 lines

**Done: the decision is recorded, the parameters stay.** **Seventeen parameters named `unread*` or `unused*`**, on eight different
functions, eight of them called `unread_flag`.

The stated reason (in `rc_io.inc`) is "kept in the signature because the call
sites pass it", which is circular — the call sites pass it because the signature
has it. Every one of these is a `static`-scope function in a self-contained
program, so all seventeen *could* go, along with the arguments at their call
sites.

The real trade-off is different and worth writing down instead: deleting them
makes the signatures stop matching BMF.exe's, which `tools/addrmap.py` and the
recovered-address map are keyed to. That is a decision about what the tree is
for, not a cleanup, and it should be made once and recorded — not re-derived at
each site.

---

## The range coder and the bit stream

### `rc.inc` — 166 lines

**Done.** **No header comment**, alone with `memory.inc` in that. The file opens on
`struct RangeCoder {` with no sentence saying it is the arithmetic coder, that
`kMarker = 0x97` is the original's, or that everything above `enc_init` is
private for a reason.

The code is otherwise the cleanest in the tree: fifteen small methods, named
constants instead of the four magic words Hex-Rays recovered, and `p()`/`set_p()`
wrapping the one global it has to share with the packer.

### `rc_io.inc` — 200 lines

Good shape after the round that put `pack_bits`/`unpack_bits` here. Two notes:

**`rc_begin` is 90 lines and does three unrelated things** — back the cursor
off the packer's partial word, lay out the level geometry, and seed 1,024
frequency tables. Only the first is about beginning a stream. The middle
section is 40 lines of `level_geom[n].half = …; level_geom[n].first = …;
memset(&model_geometry[at], n, 2^n)` written out for n = 2..7, which is a loop:

```c
for( int lvl = 2, at = 2; lvl<8; ++lvl ) {
    level_geom[lvl].half = 1<<(lvl-2);
    …
    at += 2*level_geom[lvl-1].half;
}
```

The seeding is a separate concern again and could be its own function.

**`rc_begin_decode(int8_t unread_flag)`** carries the circular justification
`decls.inc` discusses above.

### `bitctr.inc` — 166 lines

Nothing. Ten short methods, and a header that maps every member onto
`bcdr.cpp`'s hand-written equivalent name for name — including the one
structural difference (where the "seen once" mark lives). That table is the
most useful comment in the tree, because it turns a recovered counter into a
counter someone can check against a known one.

---

## The BMP file

### `file.inc` — 49 lines

**Everything in it is still named after an archive that no longer exists.** The
header says so plainly and well — the donor put several images in one file, the
walk and the append point and the member count are all gone — but the names did
not follow the explanation: the file is `file.inc`, the struct is `BmfArc`, and
the three functions are `__bmf_open_archive`, `__bmf_destroy_archive` and
`bmf_close_archive`, for a struct that is now

```c
struct BmfArc { FILE* fp; };
```

A one-field wrapper over a `FILE*` with three functions named for a container it
is not. The whole file could be `file.inc` and `BmfFile`, and the header comment
would still carry the history — which is the right place for it.

**`bmf_close_archive` always returns `nullptr`.** It returns `nullptr` on the
branch that closes and `f` on the branch where `f` is already null, so the two
returns are the same constant; its one caller discards it. It also leaves
`fp` pointing at a closed `FILE`, which is harmless only because the caller
frees the struct immediately afterwards. Returning `void` and clearing `fp`
would remove both questions.

**`fseek(f, 0, 2)`** uses the numeric whence where `bytes_left`, eleven lines
below it in the same file, uses `SEEK_END` and `SEEK_SET`. One file, two
conventions.

### `bmp.inc` — 105 lines

Small and mostly fine. One thing:

**`alloc_image` builds the header's `(depth, flags)` word by arithmetic on a
32-bit integer**, `word2 = ((uint8_t)bpp<<16)|0x40000000` and later
`word2 |= 0x00800000`. Those are `depth`, `flags & 0x40` (sub-byte packed) and
`depth & 0x80` (has a palette), written as bit positions in a word rather than
as the fields they are. Assigning `img->depth` and `img->flags` directly would
say the same thing and remove the only place in the tree where the header's two
decision bytes are addressed as one integer.

### `bmp_read.inc` — 252 lines, 10 gotos, 2 labels

The RLE decoders. The gotos are the ugliness and they are *structured control
flow in disguise*:

* **`LABEL_61`** is the success exit — `fclose(fp); free(pal_buf); return img;`
  — reached from four places. That is a single-exit function whose exit the
  decompiler duplicated into a label.
* **`LABEL_44`** is the top of the RLE8 opcode loop, jumped to from five
  places, all of which are "read the next opcode pair".

Both dissolve: `LABEL_61` into a `goto done` that stays a `goto` (a genuine
cleanup exit) or into RAII on `pal_buf`, and `LABEL_44` into the `while(1)` it
already sits inside — the five jumps to it are `continue`.

**`corrected`: `LABEL_44` is not five `continue`s.** The five jumps sit at brace
depths 8, 8, 5, 5 and 4 against a label at depth 5, so two of them are inside
nested loops that `continue` would target instead. Substituting compiles, passes
all 93 checks including every RLE image in the corpus, and is wrong. The label
is named `next_opcode` and stays a label, with that measurement beside it.
`LABEL_61` is `done`, a real single exit.

**Done. The 358-character line** was the acceptance test: eleven conditions
joined by `||`, each a different reason to refuse the file. It is one condition
per line with the reason beside it, which makes this the only place the accepted
format is stated — `MODELS.md` §1 had to reconstruct the list by hand.

### `bmp_write.inc` — 333 lines, 3 gotos, 1 label, 9 puns

`LABEL_72` is the row-writing loop, entered from three places that each set up
`row_i`/`rows3`/`stride3` first — the same four lines, twice verbatim. That is a
function taking three arguments, called three times.

The nine `*(T*)&` puns are narrow reads of header fields and are what the
original did; `tools/uncastwidth.py` reports zero on them, so they stay.

---

## The symbols

### `sym_list.inc` — 312 lines

The `SymList` structure and its three methods. Three gotos inside `rescale` and
`code_symbol`, all of them "the list is now sorted, take the top count" — a
loop exit written as a jump. `LABEL_37` in particular is jumped to from two
places that both mean "we did not have to move anything".

Substantively fine. `SymList::code_symbol` at 88 lines is long but it is one
algorithm: walk the list under exclusion, code, bump, move-to-front, rescale.

One naming point that no tool here catches: **`code_symbol` names two different
things** — this one, and `P2Freq::code_three_way` in `alt_p2_block.inc`, which is
the predictor-2 model's three-way coder. They are methods of different structs
so the compiler is content, but `tools/unstale.py` measures the wrong one when a
document says "`code_symbol` is N lines", and a reader searching for the name
finds both. `CLEANER.md`'s Phase 5 rule — one name per thing — applies, and
`abpair.py` cannot see it because it only looks at `a`/`b` suffixes.

### `sym_code.inc` — 298 lines

Clean. Eight bodies, the largest 102 lines, and the two template pairs are the
merged encode/decode forms. `estimate_cost` is the one worth a comment it does
not have: it is exactly the histogram's entropy in bits,
`(N·ln N − Σ nᵢ·ln nᵢ)·log₂e`, and the two-accumulator split over even and odd
bins is the original's pairing and changes only the rounding. Nothing in the
file says either.

### `sym_list_decode.inc` — 194 lines

**The frame is an 8,192-entry array with eight spill slots overlaid on its
front**, `list0` through `list7` with `BMF_SPILL_PAD` between them. That union
is honest — it records the layout — but it means the reader of a 194-line
function meets a 32 KB declaration first. Since the eight slots are used within
a few lines of each other and nothing takes the array's address across them,
`tools/liftframe.py`'s rule would apply if the array itself were separated from
the slots.

**`corrected`: left alone, and here is why.** `tools/liftframe.py` declined it
with the right reason — *every member is inside its union* — and looking closer
says the union is load-bearing rather than incidental. `frame.list` is
genuinely used as an array: `w = frame.list`, `rd = &frame.list[1]`, a work
list of `SymEntry*` written and read through two cursors. The eight slots
overlay its first words, and both are live in the same region — `frame.list[1]`
is written at line 148 and read at 158 while `frame.list0` is written at 149
and read at 159, and at 64 bits `list[1]` *is* `list1`, the same word under two
spellings.

So this is not a spill area with an array bolted on; it is one buffer the
original reuses, and separating the two needs a liveness proof about when the
work list stops being needed. That is a real piece of work and not a cleanup.
The one free improvement — spelling `frame.list[1]` and `frame.list1` the
same way — is left too, because the array spelling is what shows that word is
the work list's second slot.

### `sym_reduce.inc` — 353 lines, 4 gotos, 3 labels, 12 puns

`reduce_alphabet` is 350 of the 353 lines. Two notes:

**Nineteen frame members, of which `slot0`…`slot11` are spill slots** with
`BMF_SPILL_PAD` between them — the same shape as `sym_list_decode.inc`. Four
are written and read within eight lines of each other, which is the definition
of a spill, not a variable.

**`LABEL_12`/`LABEL_14`** are the two arms of "the alphabet is dense" versus
"the alphabet is sparse", reached by three jumps. Naming them, or lifting the
two arms into named helpers, would make the file's actual decision — *code the
gaps, or code the flags* — visible at the top instead of at line 211.

---

## The main model

### `model.inc` — 1,883 lines, 11 gotos, 7 labels, 248 `frame.`

The largest file, and the one with the most left in it.

**`code_pixel` and `decode_pixel` are 595 and 598 lines and share 179 of them**
(14%, measured in `bmf.cpp`'s header). They were declined as a template merge,
correctly. But the *stages* inside them are the same three stages, and two of
those are extractable without merging the bodies:

* the 32-slot neighbour fill (§5.4 of `MODELS.md`) is 48 lines in each and
  differs only in which record each slot reads;
* the escape ladder — set the exclusion mask, walk the neighbour list, fall
  through to the `SymList` chain — is the block at `1267` and `1867` that the
  duplicate scan finds, identical for four lines and near-identical for twenty.

**The `FreqRec` rescale is written out twice**, once in each body, 30 lines each:
halve `w[0]`…`w[4]`, re-sum into `w[5]`, floor `w[6]` at 256, floor `b15` at 15.
That is a method on `FreqRec` and it would read as one. **Done** — and it
returns the new sum, because both copies re-read `w[5]` afterwards to add the
kick.

**Done: the 32-slot fill is in slot order.** Both bodies interleaved the 32
stores with the 24 loads that feed them and gave every load a name — `h10`
through `h31`, `c4`…`c7`, `cache0`…`cache6` — because MSVC scheduled them that
way and every one was used exactly once. Nothing between them writes `row_cur`
or `sym_cache`, so the order was free. **39 locals** went with the straightening,
which is why `model.inc` is 1,883 → 1,771 lines rather than the ~60 the two
rescales alone would have given.

**Seven `LABEL_<n>`**, four of which are inside the run-length shortcut and all
of which mean "the run is over, resume the ladder".

The good news: after `CLEANER.md` Phase 5 the `sym[32]` array is an array, and
after Phase 3 the two 600-line bodies declare their locals where they are
assigned. What is left is genuinely the algorithm's size, plus the two
extractions above.

### `model_workspace.inc` — 138 lines

**The same free loop is written twice**, ten lines apart, once for each half of
the workspace:

```c
p = &lists[n];
do { --p; free(p->ent); } while( left );
free(sym_list_block(lists));
```

One helper, called twice. This is the smallest and easiest fix in the tree.

### `model_plane.inc` — 237 lines

The 100-line `FreqRec` seeding loop is the file's real content and is fine,
**`corrected`: not a `switch`, a naming.** The six flag locals were
`f_b0`…`f_b5`, named after the bit rather than what it does, and a `switch` over
fifteen groups would be worse than six `if`s. Each bit folds one selector weight
into another and zeroes the source, so they are `f_w4_to_w1`, `f_w4_to_w2`,
`f_w3_to_w1`, `f_w3_to_w2`, `f_w2_to_w1` and `f_w3_double` — and the order they
are tested in is the order the folds compose.

Otherwise nothing. The unrolled zeroing a reader might expect to find here is
in `plane.inc`, in `model_planes`.

### `plane.inc` — 56 lines

Small, and the whole file is the three-way model dispatch, which is exactly
right — `unmodel_plane` and `model_planes` are the two places the three
models are chosen between, and they read as that.

The one thing to change: **`model_planes` zeroes 1,008 bytes with eight
`bmf_zero16` calls** in a loop that steps by 112. That is the original's SSE
zeroing, it does nothing a `memset` would not, and it runs once per plane rather
than once per pixel, so the unrolling cannot be paying for itself.
`plane_choose.inc` has four more of the same, zeroing its 256-byte scratch.

---

## The alternate models

### `alt_p1.inc` — 166 lines

Fine. `alt_init_tables` builds the fold/unfold pair in buckets of
`2*near_lossless_q + 1`; at `E = 0` that is one and the mapping is the plain
zigzag, which nothing says. One sentence would remove the need to trace the
bucket arithmetic to find that out.

### `alt_p1_block.inc` — 854 lines

**The worst single case of "one operation, written out N times" left in the
tree.** `update_model` is 533 lines and 121 of them are

```c
node->c[slot] += n;
node->total   += n;
```

with `n` one of nine constants — 4 at 36 sites, 6 at 27, 3 at 27, 2 at 17, 7 at
10, and 11, 13, 17 and 5 once each. A two-argument helper takes ~121 lines out
of a 533-line body and leaves the structure — which counters get bumped, and by
how much — visible instead of buried.

Alongside it:

* **six `result = ctx[0];` reloads** of a member that the preceding block cannot
  have written. Same class as `alt_p2_model.inc`'s dead save/restore below.
* **the `model_strip` index is spelled out four times**:
  `(((tot & 0x7FFF) + node->c[0] - 2*node->c[slot]) >> 25) & 0xFFFFFFC0`. It is
  one expression with a name — the sign of a count difference, quantised to a
  64-entry stride.
* **the five-buffer row rotation appears in three files** — here in
  `d8_encode_body`, and in `alt_p1_code.inc` and `alt_p1.inc`. It is
  `advance_row()` now, and **`corrected`**: 25 lines, not 29 and not identical.
  The two activity sums that follow differ — `alt_p1_code` reads two mirrored
  magnitudes through `(int8_t)` and the other two read them as the `uint8_t`
  they are declared — so those stay at the call sites. Whether the difference
  can ever show is a question about residual magnitudes that the extraction did
  not need to answer.

`ctx_of` (101 lines) is dense but honest: it is nine selectors and an activity
sum, and it reads as that.

### `alt_p1_code.inc` — 349 lines

The merged encode/decode template, and the merge is well done — the header
explains what was one-sided and what was not, which is the model for how a
merged pair should be documented. The row rotation duplication above is its one
issue.

### `alt_p2.inc` — 287 lines

`NbRow::predict` is 48 lines of clean nested loops and is the best-reading code
in the tree's model files. `alt_p2_d8_body` is the merged pair.

**Sixteen levels of indentation** at the deepest point, the most in any file.
That is the `for j / for k` filter loops nested inside the row loop inside the
direction template; splitting the innermost weight update into a named helper
would take three levels off.

### `alt_p2_block.inc` — 347 lines

Good. `P2Freq::code_three_way` is 71 lines and is the three-way coder; `p2_pred`,
`p2_bump`, `p2_update` and `p2_nudge` are the helpers `CLEANER.md` Phase 1
added, each with the reasoning that makes it safe.

**`corrected`: the unrolling was not load-bearing, and it is measured.**
`alt_p2_alloc` seeded its tables with `do/while` loops stepping by two, over
163,840 counters and 15,552 frequency records — large enough that the unrolling
might have been paying for itself. It was not: rolled to single steps and the
two counter passes merged into one, `x_ep` takes 9.7s either way, three runs
each. 104 lines → 92.

### `alt_p2_context.inc` — 504 lines, one body

**Six lines of eleven masked terms each**, now wrapped one term per line with
the bit position labelled (Phase 4). `CONTEXT-INDEX.md` reviewed converting them
to `CtxIdx` and declined: the terms carry a `run`, so the masks are not provably
sign tests, and four provably are not.

What is left is the **28 `p2_row[j][k] = …` assignments** written out
individually. **`corrected`: they are documented, not tabled.** The rows do
group — 0..3 are `dval` differences at widening offsets, 4..6 the cross-plane
terms — and that is now said at the top of the block. A table of offsets driven
by a loop was the other option and is worse: the shapes differ between entries
(`nb0[-1] + nb1->dval - nb1[-1]` against `nb1[-2] + nb1->dval - nb2[-2]`), so
the table would be 28 rows of data plus a loop, which is the same information
one indirection further away.

### `alt_p2_model.inc` — 898 lines, one body

Two things, both measured, both large.

**41 dead save/restore pairs.**

```c
step_s = step_v;
grp[1].rescale_three_way();
step_v = step_s;
```

`step_v` is a local `uint32_t`; `&step_v` appears nowhere in the file; the call
is a method on a `P2Freq`. The save and the restore cannot do anything. All 41
are removable by inspection, and each removal takes two lines and one name.

**50 copies of the same rescale guard.**

```c
if( rec->f[2]+rec->f[1]+rec->f[0]>29696 ) rec->rescale_three_way();
rec->f[slot] += (n*rec->step)>>shift;
```

with `n/shift` one of a handful of pairs (3/4, 5/4, 6/4, 7/4, and the two
`&0xFFFC>>2` / `&0xFFF8>>3` variants). One helper — `p2_freq_add(rec, slot, n,
shift)` — takes all 50, and with them the 41 save/restores, since the call
disappears into the helper.

Together these are roughly 150 lines of a 898-line file, and they are the reason
the file still reads as a wall after Phase 1 took 225 lines out of it.

**A third, smaller one:** `mir_top` is re-aliased 28 times as a `uint16_t*` view
of a `P2Freq*`, and `mir_top[is_dec+1] += …` (21 sites) is the same operation as
`rec->f[is_dec] += …` (13 sites) written the other way. One spelling, and
`mir_top` goes back to being what its name says.

### `alt_p2_encode.inc` — 342 lines

Declined as a merge with its decoder at 20% shared lines (130 of 646), measured
in `bmf.cpp`'s header, and the decline is right — what the two share is the
declaration block and the loop scaffolding, not the body.

**The coefficient save/restore at the top is the thing to understand before
touching this file:**

```c
__builtin_memcpy(saved_p2_coef, bmf_p2_coef, sizeof saved_p2_coef);
for( k … ) { bmf_p2_coef[0][k] += bmf_p2_coef[4][k]; … bmf_p2_coef[4][k] = 0; }
```

It mutates four *global* coefficient tables, uses them for the whole encode, and
puts them back at the end. Nothing says why, and nothing says why the decoder
does not do the same. A reader has to diff the two files to notice the asymmetry
at all. One comment here and one in the decoder.

The 24 lines that do it are also a candidate for a small RAII type — save on
construction, restore on destruction — which would make "these globals are
borrowed for the duration" a thing the code says rather than a thing two
`memcpy`s 300 lines apart imply.

### `alt_p2_decode.inc` — 294 lines

The mirror, and the smaller of the pair because it has none of the encoder's
near-lossless drift control — it is told which code was used.

Nothing specific beyond its half of the asymmetry above. It is worth saying that
this is the *right* outcome for a declined merge: two files that are each
readable on their own, whose difference is one paragraph of comment, rather than
one file behind an `if( f_DEC )` at every third line. The nine pairs that *were*
merged are merged because their difference really was one call.

---

## The decision layer

### `filter_cost.inc` — 203 lines, 128 `frame.` uses

**Thirty frame members for a 203-line function**, six of them written and read
within eight lines — `off_a0`, `off_a1`, `off_b0`, `off_b1`, `r1_f`, `step_d`.
Those six are spills.

The nine `off_*`/`step_*` members are worse than spills: they are *loop-invariant
offsets* — `d1 - row - nplanes` and its five siblings — computed once and then
read through the frame in the hot loop. As plain `const int32_t` locals they
would say what they are (the eight neighbour offsets of a 3×3 window) instead of
reading as scattered state.

**`corrected`: it did not need a lift.** `tools/liftframe.py` has tried this
frame and reverted it — the lifted build aborts on `altp1` while compressing.
But the nine offsets are not frame *state* at all, they are loop invariants, and
eight `const` locals took them out without touching the frame. Thirty members
for a 203-line function was the complaint; nine of them were never state.

### `filter_search.inc` — 585 lines, 4 gotos, 186 `frame.`

**Six copies of the packer reset**, four lines each:

```c
packer_free_bits = 0;
packer_acc = 0;
out_cursor = coded_buf;
packer_word = (uint32_t*)coded_buf;
hist_scratch = coded_buf+coded_size-4096;
```

`packer_reset()` — five lines to one call, six times, and the fifth line
(`hist_scratch`) stops being something you have to remember to include.

**Seven copies of the cost probe**, about fourteen lines each: set the flags,
call `model_planes`, measure `8*(out_cursor-coded_buf)`, flush, reset, keep
the flag if it improved. This is the largest repeated block left in the tree
after `alt_p1_block.inc`'s, and it is a function of two arguments (the flag to
try, the plane).

**Five dead cost expressions.** Each probe computes

```c
best_cost = plane_desc[0].desc_word-packer_free_bits+bits_f0+32;
deep = 0;
…
if( !deep ) best_cost = bits_f0;
```

`deep` is assigned `0` three lines above the test at all five sites, so the
first expression never survives; one probe also ends
`if( best_cost==0x7FFFFFFF ) best_cost = 0x7FFFFFFF;`. `tools/foldif.py` reports
zero on all six, because its rule wants a *literally* constant condition rather
than one assigned a constant three lines up. **Widening that rule is worth more
than deleting the six lines** — a folder that only sees `if( 0 )` will keep
saying zero here.

### `plane_choose.inc` — 657 lines, 157 `frame.`, 53 puns

**The worst spill storage in the tree.** Six `alignas(16) int32_t x0[4]` …
`x5[4]` arrays, written 28 times by subscript and read as `double` **40 times**
and as `int64_t` 6 times:

```c
d1 = d4**(double*)x4-sum02_c*sum02;
wa = (int32_t)(((sum02**(double*)&frame.q0-sum01**(double*)x4)**(double*)x5 + …
```

These are not arrays. They are twelve `double` accumulators — the covariances of
a three-variable least-squares fit for the alpha plane's weights — spilled into
SSE-aligned integer slots by the original and materialised by the decompiler as
`int32_t[4]`. Named as what they are:

```c
double Sxx, Sxy, Sxz, Syy, Syz, Szz, Sxw, Syw, Szw;
```

the 40 puns vanish, the six arrays vanish, and the solve reads as a solve. This
is the single largest legibility win available anywhere in the tree, and it is
confined to about 120 lines.

**Twenty frame members named `s0`…`s10` and `p0`…`p5`.** Four are pure spills.
The rest are the hill-climb's loop state — the two weights' current values, the
two bounds of the step, the three row cursors — and every one of them has a name
waiting in what it holds. Right now the file's central algorithm (search two
weights by hill-climbing, cost each candidate from a histogram) is spelled in
s-numbers, and the four nested search loops cannot be read without a pencil.

**The 478-character line** is the declaration wall, which Phase 3 could not
reach because these are assigned more than once.

The header comment about the six aliased buffers is excellent and should stay
exactly as it is — it records a defect that the gate could not see.

---

## One image in, one image out

### `image_compress.inc` — 211 lines, 4 gotos, 4 labels

Much improved by Phase 2. What is left:

**`LABEL_22` is reached by a `goto` from an `else if` twelve lines below it** —
`if( plane_count==1 ) { if( depth&0x40 ) { LABEL_22: … } } else if( plane_count>2 ) goto LABEL_22;`.
That is `if( plane_count==1 ? (depth&0x40) : plane_count>2 )`, and written that
way the reader can see the condition instead of reconstructing it.

**`LABEL_76`/`LABEL_77`** are the raw-fallback path, entered from two places
that differ only in whether the transpose has to be undone first.

**`hdr` inherits `depth` and `flags` through a four-byte word copy**:
`*(uint32_t*)&hdr._pad8 = hdr_pad8;` copies `_pad8[2]`, `depth` and `flags` in
one store. That is why the written header carries `0x40` and a greyscale bit
nobody assigned. It is correct and it is invisible; one comment.

### `image_expand.inc` — 353 lines, 6 gotos, 5 labels, 79 `frame.`

**Ten copies of the failure exit:**

```c
fclose(arc->fp);
arc->fp = 0;
return nullptr;
```

A `fail()` helper — or, since the function has a single owner for `arc`, one
`goto fail` and one exit. Ten sites is enough that "did this path close the
file?" is a question a reader has to ask ten times.

The five `LABEL_<n>` are the format's branch structure — raw versus coded, short
path versus descriptors, planar versus interleaved. `MODELS.md` §9 lists them as
seven numbered steps; the code could be shaped like that list.

---

## Predictors and transforms

### `plane_predict.inc` — 350 lines

**The MED predictor is written twice**, once in `predict_med` and once in
`unpredict_med`, eight lines each and identical:

```c
if( pred<north ) {
  if( northwest<pred )       pred = north;
  else if( northwest<=north ) pred = north+pred-northwest;
} else { … }
```

`static inline uint8_t med(int N, int W, int NW)` and both call it. This is the
clearest extraction in the tree: the two copies are byte-identical after
renaming, the function is pure, and MED is a *named published predictor* whose
name should appear in the source exactly once.

**`colour_transform` has four modes in one 74-line body** — no transform,
by-weights, two-reference and three-reference — each a `do/while` over the
plane. They share nothing but the loop shape. Four small functions and a
`switch` would say what the four modes are, which is currently only visible in
`MODELS.md`.

**`expand_predictor_mode0` is an empty body.** It is the original's
counterpart to a predictor this build never selects, and that is worth one line
of comment rather than the reader wondering whether something was lost.

---

## What was done, and what it cost

Ranked as planned, with what each actually took.

| # | change | file | planned | taken | lines |
| --- | --- | --- | --- | --- | --- |
| 1 | drop the dead `step_v` save/restore | `alt_p2_model.inc` | 41 | 41, plus `ctxw`'s and a write-only `bank_off2` | −85 |
| 2 | `CounterNode::bump` | `alt_p1_block.inc` | 121 | 119, + `escape_bias` ×4, + 5 dead reloads | −128 |
| 3 | `p2_freq_add` | `alt_p2_model.inc` | 50 | 48, + `p2_rescale` for the other 2 | −143 |
| 4 | name the twelve covariances | `plane_choose.inc` | 40 puns | 53 → 0 | −32 |
| 5 | `med_predict()` | `plane_predict.inc` | 2 | 2 | −20 |
| 6 | `packer_reset()`, `probe_plane()` | `filter_search.inc` | 6 + 7 | 15 + 6 | −73 |
| 7 | `BmfFile::fail()` | `image_expand.inc` | 10 | 11 | −30 |
| 8 | one `memset` for the unrolled zeroing | `plane.inc`, `plane_choose.inc` | 12 | 12 | −20 |
| 9 | `free_sym_lists()` | `model_workspace.inc` | 2 | 2 | −14 |
| 10 | header comments | `rc.inc`, `memory.inc` | 2 | 2 | +22 |
| — | `FreqRec::rescale` | `model.inc` | not ranked | 2 × 30 lines | −63 |
| — | `advance_row()` | three `alt_p1_*` files | not ranked | 3 | −57 |
| — | the level-geometry loop | `rc_io.inc` | not ranked | 1 | −10 |
| — | name all 33 `LABEL_<n>` | 11 files | declined | done | 0 |
| — | the 32-slot neighbour fill | `model.inc` | not ranked | 2 × 48 lines, 39 locals | −49 |
| — | roll the seeding loops | `alt_p2_block.inc` | measure first | measured: 9.7s either way | −12 |
| — | wrap the acceptance test | `bmp_read.inc` | — | done | +12 |
| — | name the six flag bits | `model_plane.inc` | a `switch` | a naming | +7 |

**11,787 → 11,363 lines.** Four counts in the table above are larger than the
review predicted, in every case because doing the work found more of the same
shape next to it.

Three things are worth more than the line count.

**The gate caught one defect, in the change the review called low-risk.**
`advance_row`'s first cut wrote `cursor[1][-1]` where the original had `b0[3]`
after `b0 += 4` — four records off. It compiled, produced streams of exactly the
right length, and failed four checks. Nothing short of byte-comparison would
have seen it.

**`deadcheck.py` had a gap this work opened.** It skipped `static inline` bodies
when building its call graph, so a call made *from* one never became an edge.
`p2_rescale` was the first inline helper here to call a method, and it made that
method report dead from its only call site. The skip is right for the *report* —
an inline helper with no callers is one the compiler drops — and wrong for the
graph. Fixed, and checked by injecting a body with no callers and watching the
report fire.

**`methodise.py` found `fail()` immediately.** A free function whose first
parameter is only ever a receiver is a method, and the tool said so about the
helper I had just written. It is `BmfFile::fail()`.

Two things this review deliberately does **not** propose:

* **merging `code_pixel` and `decode_pixel`**, or the encode/decode pair of the
  predictor-2 model. Both were measured and declined, and nothing here changes
  the measurement.
* **renaming the `LABEL_<n>` labels wholesale.** Thirty-three of them are ugly,
  but most dissolve into structure once the surrounding block is fixed —
  renaming them first would preserve exactly what should disappear.
