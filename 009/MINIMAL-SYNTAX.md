# Minimal syntax: the plan, and what it came to

The rule for this round: **say the same thing with less of the language**.
`ctx` where the code says `this->ctx`, `plane_count` where it says
`::plane_count`, and so on down. Nothing here was meant to change what the
program computes, and every phase was gated on the fifteen streams coming out
byte for byte — `./build.sh && ./test.sh`, about 26 seconds a round.

This was a spelling round, not a structure round, with one exception (Phase 3)
that was called out as such.

**This document was written as a plan and is kept as a record.** Each phase
still says what it proposed; what follows the proposal is what the measurement
said when the rule met the file. Six of the nine numbers below moved, one phase
turned out to be worth nothing, and three of the four defects worth writing
down came from rules that were right in the plan and wrong in the tree. A plan
kept without its outcome is a plan that gets re-proposed.

| | plan | actual | |
| --- | --- | --- | --- |
| `this->` | 923 | **923** | all of them |
| `::name` for a global | 181 | **181** | all of them |
| `blk->` | 786 | **562** | the remaining 224 are locals, not receivers |
| `!= 0` / `== 0` in a condition | 85 | **56** | 5 declined on precedence, 11 left |
| declare at first use | 165 | **102** | the safe rule declines the rest |
| `= (x);` | 64 | **63** | |
| casts to the operand's own type | 16 | **41** | earlier phases exposed more |
| `if( (x) )` | 2 | **2** | |
| `while( 1 )` with a break | ~16 | **0** | see Phase 7 |

Lines 12,187 → 12,071. Warnings 4 → 4, shadows 0, and `-Wshadow` is now in the
ratchet.

---

## What was already swept, so the plan did not re-propose it

Every counting tool in `tools/` was run against the split tree before the plan
was written, and they all reported zero:

```
decast     no useless casts          untemp     0 single-use temporaries
uncast     0 useless casts removed   uncopy     0 locals that are only a copy
unrecast   0 sites                   unwrite    0 write-only locals
unlayer    0 pointer casts           unsave     0 saves across a dead region
unalias    0 frame aliases           unaliasvar 0 aliases of a single assignment
namelocal  0 locals nameable         degoto     0 candidates of 11 gotos
unjump     0 gotos into a block      untail     0 shared tails
undup      0 identical if/else arms  deadcheck  0 findings
```

**Three of those zeros were not zeros.** The tools read one file, and the unit
has been 37 files since the split; `deadcheck` reported nine live functions
dead because every call site was inside a class method one `#include` away,
and `unstale` said a hundred names in `ALGORITHM.md` were names the program no
longer had. `structs.py` now has `splice()` — read a unit the way the compiler
reads it, with an origin map back to `file:line` — and `defs()`, which finds
bodies at any brace depth, because `structs.bodies` stops at depth 0 and every
method in this tree sits one level in. Both checks use them, and both were
proven by injecting a defect and watching them report it.

That is the shape of the round's one general lesson: **a rule that cannot match
anything reports the same zero as a clean tree.** It happened four times — the
three tools above, and `methodise.py`, whose rule keyed on a parameter named
`_this` while the two functions it was written for called theirs `blk`.

And the compiler agreed where it could be asked: **`-Wuseless-cast` reports 0**
over the whole translation unit. So the 590 integer casts and 207
struct-pointer casts in the tree are not same-type casts; they all convert
something.

---

## Phase 1 — drop `this->` (923 sites, all taken)

The largest single class and the one the round is named for.

**It was not a blanket substitution.** `-Wshadow` named 18 places where a local
or a parameter had the same name as a member of the class it was in, across
thirteen names. 86 of the 923 uses were on one of those names, and for those
`this->` was load-bearing. The worked example was `model.inc:185` and
`model.inc:1239`, both of which read

```c
sym_cache = this->sym_cache;
```

where `sym_cache` was a member *and* a local of `code_pixel`. Dropping the
prefix makes it `sym_cache = sym_cache;` — a self-assignment that compiles
without a warning at our warning level and quietly stops loading the member.

**The order was rename, prove, then drop**, and it held:

1. thirteen locals renamed inside their own bodies, driven by the warning
   rather than by a pattern — `level_of` → `act_lvl`, `pred` → `guess`,
   `sym_cache` → `cache`, `depth` → `bpp`, and so on;
2. `-Wshadow` to zero, the gate green, ASan clean;
3. then all 923 prefixes in one pass.

`-Wshadow` joined `BMF_WARN=1`'s flag set here so a shadow reintroduced later
cannot be silent. **It has since earned that twice** — once in Phase 3, where
the conversion gave a body a local named `row0` beside the member of that name,
and once at the end of Phase 3's second half, where moving `init_symbol_list`
into `SymList` gave it a local `n` beside the member `n` and the count went
4 → 5 before anything else noticed.

Two things the plan got right and one it did not say. Right: the trap, and the
order. Not said: **the rename pass must not "restore" names it thinks it
mangled.** A first attempt re-expanded the shortened names afterwards and
turned `this->sel0_list` into `this->sel0_list_list`, because the new name is a
prefix of the old. The lookbehind already spares `this->`; the restore pass was
the defect and was deleted.

## Phase 2 — drop `::` (181 sites, all taken)

`::plane_count` and its 180 friends were the decompiler's way of writing "the
global", not a disambiguation. Two names had to be renamed first and `-Wshadow`
said exactly which: `__compress_image`'s third parameter was `coded_buf` and
shadowed the global of that name (now `extra_blk`), and `__bmf_open_archive`
declared an `int32_t rc` shadowing the program's one range coder (now
`seek_rc`, which is what it holds). After those two, all 181 came out together.

Nearly risk-free, as predicted, and the right phase to start with.

## Phase 3 — make the deep-plane functions methods (415 predicted, 562 taken)

**The one phase that changed structure.**

`__alt_p2_context` and `__alt_p2_model` took `AltP2Block*` first and walked it
on every line: 415 `blk->` between them, gone, and 18 call sites that now read
`blk->alt_p2_model(…)`. They are the only two methods in the tree defined out
of line, because at 400 and 1100 lines they are what the file split exists for.

Then three more the plan pointed at but did not count: `ModelBlock` had no
alloc/free pair of its own while `AltP1Block` and `AltP2Block` both do, so
`__layout_workspace` and `__free_workspace` became `ModelBlock` methods, and
`__alt_p2_d8_body` became a method template of `AltP2Block`. And after the tool
was widened (below), `__init_symbol_list` became `SymList::init` — 13 `list->`
and 10 call sites — and three one-line delegators were deleted outright rather
than methodised, since inlining each at its one or two call sites removes the
function, its forward declaration and the hop.

`blk->` is 786 → 224. **The 224 are not candidates**: in `model.inc`,
`model_plane.inc` and `p1.inc`, `blk` is a *local* holding a block the
enclosing function does not belong to, and `blk->x` there is an ordinary member
access on another object.

**The ordering matters and it was got wrong first.** While a function is free,
`blk->x` marks a member and bare `x` marks a local; once the prefix is dropped
they are the same text. So the eight locals whose names were already members
had to be renamed *before* the conversion, from a list built *while* `blk->`
still distinguished them. Renaming afterwards renames the member accesses too.

Two more defects worth keeping:

- **the lookbehind.** `(?<![>.\w])` also refuses to match after a greater-than,
  so in `d_run4b>band_hi` the old name survived and the build stopped. The
  correct spelling is `(?<![.\w])(?<!->)` — "not the tail of an identifier, and
  not after an arrow", and nothing about comparisons. Phase 1's renames used
  the buggy version and were audited afterwards; they are clean.
- **the scope of the strip.** `p2.inc` holds wrappers as well as a body, and
  the wrappers are free functions with a *local* named `blk`; stripping the
  whole file turned `blk->alt_p2_free(1)` into `alt_p2_free(1)`. The strip has
  to be scoped to the body being converted.

**The tool.** The plan said to widen `tools/methodise.py` from "a parameter
named `_this`" to "the first parameter is a pointer to a struct this file
declares, and every use of it is a member access". That was done last, after
the two functions had been converted by hand — which is the wrong order, and
the reason it is worth naming: for the whole middle of the round the tool
reported zero, and the zero meant only that the decompiler writes `_this` where
MSVC used `__thiscall` and not where the receiver was passed on the stack.

The widened rule's second half is what keeps it honest. `__write_bmp` has 23
`img->` and four uses that are not, so it takes an image rather than being a
method of one, and the rule declines it. One candidate is declined by judgement
rather than by rule, and the reason is in the tool: `__alt_model_p2_encode`
reads two fields of the image and is a 343-line encoder, so a method of
`BmfImage` would make the image the compressor — and its decode half takes a
`uint16_t*` and could not be the same kind of method.

## Phase 4 — `!= 0` and `== 0` in conditions (85 predicted, 56 taken)

`if( (Flags&0x80)!=0 )` is `if( Flags&0x80 )`. Mechanical, and the plan said
"no blockers found", and the plan was wrong.

**This one was done twice.** The first rule asked whether the operand balanced
when wrapped in parentheses, which says nothing about precedence, and it wrote

```c
if( (bpp&0x3Fu)<=4||(flags_b&0x10)==0 )      // A || !B
if( !((bpp&0x3Fu)<=4||(flags_b&0x10)) )      // !A && !B
```

because `==` binds tighter than `||`, so the `==0` was testing the right
operand alone. The fifteen streams still matched — it is in `expand_image.inc`,
decoder only — and eleven of the fifteen round trips broke, which is the gate
catching exactly the half of itself that could. The rule now requires the
operand to be a **primary expression**: a name with member and subscript
chains, or a fully parenthesised group, either behind an optional cast.

It declines 5 of 60 on that basis, and those five are the ones that were wrong.
Eleven sites are left in the tree: two are `fseek(…)!=0`, which is a status
code and not a bitmask; one is `(cand==0)` used as a *value* and not a
condition; the rest are operands of `||` and `&&` where the rewrite is what
broke the round trips.

## Phase 5 — declare at first use (165 predicted, 102 taken)

Two passes: 48 where the single assignment is at the same brace depth as the
declaration, and 54 more where it is deeper and every use is inside that block,
so the declaration moves *in* rather than down.

The plan's 165 came from a looser measurement. The safe rule declines the rest,
and the reasons are worth keeping: 78 first mentions are not a plain
assignment, 32 locals are assigned more than once, and 12 have a `goto` or a
label between the declaration and the assignment.

**This partly undoes an earlier round, on purpose.** `tools/compact_locals.py`
merged 4,559 one-per-line declarations into comma lists, which was right when
the alternative was a wall of declarations at the top of a 600-line body.
Neither rule wants the other's cases; the order is this phase first,
`compact_locals.py` over whatever is left.

Three defects, all caught by the compiler rather than the gate:

- **a declaration inside `struct X { … }` is a member, not a local.** Moving
  one into a method that assigns it deletes the member; `SymEntry::cnt` went
  that way. The guard has to be that the *innermost* enclosing block is a
  struct body — the first attempt asked whether *any* enclosing block was,
  which excluded every local of every method and quietly halved the phase.
- **`do { … } while( x );`** puts the condition outside the block's declarative
  region even though it is inside the braces, so a name tested there cannot be
  declared inside. `alt_model_p2_encode`'s `xf2` went that way.

Neither is visible to a rule that only counts assignments, which is why the
phase is two narrow passes and not one broad one.

## Phase 6 — the small classes (82 predicted, 106 taken)

- **63** `= (x);` — a parenthesised whole initialiser (plan: 64)
- **41** casts of a variable to its own declared type (plan: 16) — more,
  because Phases 2 and 4 exposed some by removing what was wrapped around them
- **2** `if( (x) )`

`-Wuseless-cast` does not see the second class because the operand is an lvalue
and the cast is formally a conversion; `tools/decast.py` does not either. Its
rule wants "the operand is a plain identifier whose declared type is the cast
type".

## Phase 7 — `while( 1 )` with a break (32 sites, **0 taken**)

The plan expected roughly half of the 32 to qualify — the break at the top
makes a `while (c)`, the break at the bottom a `do … while (c)` — and called
this the lowest-value phase and the first to drop.

Measured: **none of them qualify.** Not one of the 32 has its break at the top
at the loop's own nesting level; nineteen have it in the middle, twelve have no
top-level break at all (they break out of an inner `switch` or return), and the
one remaining candidate, in `reduce_alphabet.inc`, has a statement *after* its
break, so a `do … while` would need that statement duplicated. Which is the
plan's own criterion for leaving a loop alone: more syntax, not less.

So the phase is a decline, and the reason is now measured rather than guessed.
Left in the plan as "expect roughly half", the next round re-proposes it.

---

## Declined, with the reason

Recording these matters as much as the phases: without it the next round
re-proposes them.

**`-Wold-style-cast`, 1471 sites.** Turning `(uint8_t)x` into
`static_cast<uint8_t>(x)` is 12 more characters at every one of 1471 sites.
It is the house style of a lot of C++ and it is the opposite of this round's
rule. Declined on the round's own terms.

**Dissolving the eight `__frame` structs, 972 `__frame.` prefixes.** The single
largest remaining class, and the one that must not be touched. Eight frames are
left — in `choose_plane_coding`, `cost_candidate`, `decode_symbol_list`,
`expand_image`, `model.inc` (two), `reduce_alphabet` and `search_filter` — and
they are the survivors of a sweep that already lifted everything liftable.
`tools/defram.py --list` offers zero. `tools/README.md` records four rules
learned by breaking this exact thing, the first of which is that an address
taken of one member pins the whole frame. These bodies index off the ends of
their locals deliberately; the frame is what makes the neighbours theirs to
index.

**Renaming `__frame` to something short.** 972 sites, six characters each, and
no semantic risk at all — but `defram.py`, `reframe.py`, `unalias.py`,
`unslot.py`, `unspill.py` and `liftframe.py` all key on the name `__frame`.
The saving is cosmetic and the cost is six tools. Declined unless those tools
are being revised anyway.

**Removing the `(uint32_t)` casts in mixed-sign comparisons.**
`tools/explicitcmp.py` *added* those, deliberately, to write out the conversion
the compiler was already performing. They are minimal syntax pointing the other
way, and the one site it still reports is a real one. Leave them.

**Removing the `unread_*` parameters, 50 of them.** They are parameters the
body never reads, and the name says so. Removing them means changing 50
signatures and every call site, to delete arguments the donor pushed — which
loses the record of what the donor's calling convention was for no reduction in
what a reader has to hold. The name is already the minimal way of saying it.

**Methodising `__alt_model_p2_encode`, `__write_bmp`, `__model_plane`,
`__transform_planes`, `__choose_plane_coding`, `__search_filter`,
`__expand_image`, `__compress_image`, `__reduce_alphabet`.** All take a struct
pointer first; none is a method. Seven fail the rule outright — they pass the
pointer on, or do arithmetic with it, so it is an argument and not a receiver —
and `__alt_model_p2_encode` passes the rule and is declined by judgement, as
above. `python3 tools/methodise.py bmf.cpp` is what re-derives this list.

---

## What "done" meant, and what it says now

Each phase ended with `./build.sh && ./test.sh` green — the fifteen reference
streams byte for byte. The round ends with the full suite, which is what every
round in this tree ends with:

```
./build.sh && ./test.sh                  83 checks, both widths and clang
./build.sh -DBMF_HIGH_ARENA && ./test.sh 82 of 83, every allocation above 4 GB
./tools/x32.sh                           22 of 22 across the two widths
./tools/asan.sh                          nothing in 40 runs over 17 images
./tools/fuzz.sh 400                      nothing
./tools/sweep.sh bmf.cpp                 every counting tool at zero
BMF_WARN=1 ./build.sh                    4, and zero shadows
```

The warning count is the ratchet, and Phase 1 moved it: `-Wshadow` joined the
set, so the target after Phase 1 was not "still 4" but "4, and zero shadows".
That is where it is.

**What is left is the declines and nothing else.** Every phase is either taken
in full or measured and declined with the measurement written down, the sweep
answers about the whole unit rather than one file of it, and each of its checks
has been shown a defect it must report and reported it.
