# tools/

This began as six one-shot migrations run over `subs1.hpp`, kept because the
file is generator output: if it is produced again from the extractor, these are
what has to be replayed on top. It is a good deal more than that now — the
sweeps, the gates, the replay harness — and the sections here cover the ones
that need more than a docstring to explain.

**Every script documents itself.** `python3 tools/<name>.py` with no arguments
prints its usage, and the module docstring is the reasoning: what the rule is,
what it declines rather than guesses at, and what it got wrong before it got it
right. The sections below cover the scripts that need more than that; the table
at the end of this file covers all of them.

That table is **generated**, by `tools/readme.py`, from each script's own first
line. This file used to say instead that most of these would stay unnamed here,
because "a table of eighty-seven rows maintained by hand is a table that goes
stale, which is the subject of half of `REFACTORING9.md`" — which was true of
a table maintained by hand, and is an argument for deriving one rather than for
having none. `tools/readme.py` is in the sweep, so a script added, renamed, or
given a different first line fails the sweep until the table is regenerated:

```
python3 tools/readme.py            # what has drifted
python3 tools/readme.py --apply    # rewrite the table
```

The consequence worth knowing: **a tool's row here is that tool's own first
line.** If a row reads badly the fix is in the script, not in this file.

These are the entry points — the rest are run by one of them, or by hand on a
file (the count said "three" for four rounds while the block under it listed
six, which is the kind of thing the table below now makes hard to do):

```
./tools/sweep.sh          # run every tool against a copy; all should report zero
./tools/proven.sh         # and whether those zeros have ever been able to move
./tools/asan.sh           # every test image under AddressSanitizer
./tools/fuzz.sh           # mutated inputs through the same
./tools/hdrscan.sh        # and the header bytes small enough to enumerate
./tools/x32.sh            # the same corpus at the other pointer width
./tools/x64diff.sh        # and whether the two widths agree on inputs nobody chose
./tools/triage.sh         # bisect a stream that moved, by name
```

`asan.sh` is the one to run after lifting a frame: these bodies walk off the
ends of their locals on purpose, and a frame is what makes the neighbours
theirs to walk.

`fuzz.sh` is the same instrument pointed at input nobody wrote by hand, and it
is the one to run after changing anything a stream's header reaches. The round
that fuzzed for crashes alone got 2022 runs and no findings out of the `-O2`
build; the first 400 through the ASan build reported 26. `REFACTORING9.md` §47
is what they were.

`hdrscan.sh` is the one that does not sample. A `.bmf` member header has two
one-byte fields, so 512 runs a stream covers both completely — and what that
buys is a claim fuzzing cannot make: every value was tried, not enough of them
to feel confident. It found four defects §47's fuzzer had not, one of them a
SIGFPE reachable from four values in 256. §48.

## the original six

All are idempotent — running them on an already-migrated file changes
nothing — and none touches `bmf.cpp` or `blob.inc`.

| script | what it did |
| --- | --- |
| `unify_types.py` | rewrote the type vocabulary onto `<cstdint>`: `_DWORD`/`unsigned int`/`size_t` → `uint32_t`, `unsigned __int16`/`_WORD` → `uint16_t`, `__int128`/`_OWORD` → `__m128i`, and so on, leaving no multi-word type name in the file |
| `collect_globals.py` | moved the per-function typedef + global declaration pairs to the top of the file, sorted them by their address in BMF.exe's data segment, and kept one declaration per object instead of one per using function |
| `prune_unreachable.py` | deleted the function definitions nothing can reach from `main`, and then the globals-block entries no surviving body names |
| `name_raw_addrs.py` | put the addresses Hex-Rays baked into expressions — `BMF_BLOB(0x00443394)` — back onto the globals that own them |
| `hex_constants.py` | wrote the pattern constants in hex — `117901063` → `0x07070707`, `1065353216` → `0x3F800000 /* 1.0f */` |
| `compact_locals.py` | merged one function's one-per-line locals into comma lists — `int32_t v5; int32_t v8;` → `int32_t v5, v8;` |

    python3 tools/unify_types.py subs1.hpp
    python3 tools/collect_globals.py subs1.hpp
    python3 tools/prune_unreachable.py subs1.hpp          # report
    python3 tools/prune_unreachable.py subs1.hpp --apply  # rewrite
    python3 tools/name_raw_addrs.py subs1.hpp
    python3 tools/hex_constants.py subs1.hpp
    python3 tools/compact_locals.py subs1.hpp out.hpp __bmf_open_archive 2

`name_raw_addrs.py` runs after `collect_globals.py`, since it needs the globals
it rewrites onto to exist.  Its substitution is address-identical by
construction — `BMF_BLOB(va)` expands to `(int)(blob1 + va - BMF_BLOB_BASE)` and
the global declared at `va` is `*(T*)(blob1 + va - BMF_BLOB_BASE)`, so
`&global == (T*)BMF_BLOB(va)` — and it refuses to write the file if any address
is left over.  Where an address is the start of an object it takes that
object's address; where it lands inside one (`0x00445668` is eight bytes into
the table at `0x00445660`) the offset is written out instead of declaring an
overlapping global.  `0x0044337D` is the one address that got a global of its
own: it is byte 1 of the dword array based at `__Buffer`, indexed like an array,
and `__byte_44339D` and `__byte_4433AD` next door are declared the same way.
`bmf.cpp` no longer defines `BMF_BLOB`, so a freshly extracted `subs1.hpp` will
fail to compile until this has been run over it.

`hex_constants.py` is the same idea for constants that are not addresses.
Hex-Rays prints every literal in decimal, which hides a byte fill written next
to `0x606060606060606LL` (`117901063`), a float stored through an int pointer
(`1065353216`), a reciprocal multiplier (`715827883`), and the BMP signature
(`19778`). 19 sites, rewritten as the same value in hex. It leaves the rest of
the file's long decimals alone: they are byte offsets into heap structures
(`_this + 278736`) and stack frames (`__hexrays_frame + 66000`), where hex would
reveal that 278736 is 16 × 17421 and not much else.

Worth recording, since it is the question that prompted the script: **none of
those constants is an address.** Every literal in the file falls outside
BMF.exe's image (`0x00400000`–`0x00450000`) — the decompilation reaches the data
segment only through the globals block, and nothing sneaks in as a number.

The first three assert loudly rather than guessing: `collect_globals.py`
refuses to run if two functions disagree about an address's type, if two
addresses claim the same IDA name, or if a name it is about to rewrite could be
a local rather than the global.  `prune_unreachable.py` works from a call
graph, not a heuristic — taking a function's address counts as reaching it —
and is worth re-running whenever the command line changes, since it is what
says which bodies the new one leaves stranded.

`compact_locals.py` is the odd one out: it takes one function at a time and
writes to a separate output file, because unlike the others it is not a
migration that has to be replayed but an edit you apply, test, and keep or
throw away.  `mode 1` merges only *adjacent* declarations of the same type, so
the decompiler's ordering survives; `mode 2` sorts by type first, so each type
appears once and the block gets much shorter.  Neither moves a declaration
across one that has an initialiser — an initialiser can name an earlier local,
so those split the block into segments that are compacted independently — and
neither touches `alignas` declarations or function-pointer declarators, whose
syntax does not survive a comma list.

Reordering declarations is safe by the language but it does change the order
the compiler assigns stack slots in, and this is decompiled code that reads a
good deal of memory it does not own, so it is worth testing.  See
"how this was applied" below.

The gate for the first three was `./test.sh`, plus a run of every image in
`testfiles/` through every switch combination against a binary built from the
untouched sources, comparing the compressed stream, the recovered image, the
exit status and the console output of each. All 100 image/mode pairs matched,
including the modes where this decompilation crashes — it crashes identically.

## how `compact_locals.py` was applied

Function by function, in one pass over every body in the file, rebuilding and
testing after each one: mode 2 first, mode 1 if mode 2 broke anything, and the
function left alone if mode 1 broke it too.  The gate for a single function was
a build, then every image in `testfiles/` compressed and decompressed again,
checking that the recovered image is byte-identical to the original *and* that
the compressed stream is byte-identical to the one the previous state of the
file produced — the second half is what catches a change that still round-trips
but no longer encodes the same way.

81 of the 215 bodies had something to merge, and all 81 took mode 2; nothing
had to fall back to mode 1 and nothing had to be skipped.  4559 declaration
lines went away.

One thing to know if you re-run this: `bmf` opens its output with `"a+b"` — it
appends a member to the archive rather than replacing one — so a harness that
reuses one output path accumulates streams and every digest it computes is
different from the last.  Delete the output before each run.  (Changing the
mode instead is the wrong fix, and was made and reverted once: appending is how
a multi-image archive is built, and `bmf d` reads every member back.)

## `structs.py` — give an object a struct and use it

`objects.py` reports the field maps. This turns one into a declaration and
rewrites every access to it:

```
python3 tools/structs.py subs1.hpp --list        # objects by traffic
python3 tools/structs.py subs1.hpp --apply 0     # the busiest one not skipped
python3 tools/structs.py subs1.hpp --skip 0      # record it as declined
```

The names that denote one allocation come from the same alias analysis
`objects.py` uses. Constant offsets become members, the widest access at an
offset wins, gaps become `_pad`, and the struct carries a `static_assert` on
its size. Everything else about the object is left alone: a variable-offset
walk becomes `*(T *)((char *)p + (intptr_t)(expr))` and still walks the same
bytes.

**This is only sound because the target is 32-bit.** A pointer is four bytes
there, which is the width the decompiled code assumes every field to be, so the
recovered layout is the layout the code already had. On a 64-bit target the
same struct would move every field after the first pointer, and the walks would
index the wrong things. The `static_assert` is what says so out loud.

Four things it has to read rather than assume, each of which produced a
compiling, wrong file before it was handled:

- **the declared type of every name.** `p + 3` steps three bytes when `p` is
  `char *` and twelve when it is `void **`. Offsets are scaled accordingly, on
  both the survey and the rewrite side.
- **whole function bodies, not lines.** The decompiler wraps expressions; a
  dereference split over three lines is one dereference.
- **matched brackets.** An index can contain a subscript —
  `this_3[12 * k + 6 * (v39[n + 27] & v39[n + 19])]` — and stopping at the
  first `]` finds only the inner one.
- **what precedes a `*`.** `*v5` is a dereference and `16 * v5` is not.

It declines rather than guesses: a name stepped with `p += k` (a cast is not an
lvalue), or one whose declaration it cannot find (a name left at its old type
takes `nm->f8` into a file that will not build).

## `struct-sweep.sh` — apply, gate, keep or revert

```
tools/struct-sweep.sh 10        # ten objects, largest first
```

Each round backs the file up, applies the busiest object not on
`tools/struct-skip.txt`, builds, and runs the gate. A round that fails is undone
completely and the object is written to the skip list, so the next round moves
past it instead of retrying it forever. State lives in the file and the skip
list, so the sweep is resumable.

It runs the gate with `BMF_TIMEOUT=60` rather than `test.sh`'s five minutes: a
rewrite that loops forever should cost ten seconds to reject, not fifty minutes.
Run the full gate before committing.

73 objects converted; the sweep is exhausted. `REFACTORING.md` §Phase 4 has the
categories and what each one means.

## `addrmap.py` — which address in BMF.exe a name came from

```
python3 tools/addrmap.py subs1.hpp > tools/addrmap.txt
```

`ALGORITHM.md` used to be able to say that function names *were* the donor's
addresses. Every rename since is a small loss of that, and this recovers it —
from the record, not from the code.

Two sources, in order. Commit messages carry `tools/rename.py`'s own output, so
`sub_412850 -> decode_context_bit` is a statement of what happened. Where a
rename predates that habit, the tool pairs the file's two revisions across the
commit that made it: a token substitution does not reorder or remove bodies, so
the k-th non-shim definition before is the k-th after. If the commit did more
than rename, the two lists are aligned and only stretches with equal counts on
each side are read.

Every pair is checked against the file — the new name present, the old one
gone — and names it cannot resolve are printed as unresolved rather than
filled in. All 71 are mapped.

Matching bodies by the constants they contain is the obvious alternative and
is in this file's history. It does not work: an encoder and its decoder test
the same numbers, so it mapped `encode_context_bit` and `decode_context_bit` to
one address and gave no sign that anything was wrong.

## `reframe.py` — give a plain-split frame its layout back

```
python3 tools/reframe.py subs1.hpp pre2.hpp --list
python3 tools/reframe.py subs1.hpp pre2.hpp __write_bmp
```

Phase 2 split all 24 `__hexrays_frame` buffers into locals; 16 could not take it
and got a struct with explicit padding instead. The other 8 kept the split
because the gate accepted them — and for one of them, `alt_model_p1_decode`, the
gate accepted it only because nothing in the corpus ran that function. Its
aliases named 84 of 116 bytes and the code writes into the rest; with each local
in its own storage those writes landed on unrelated stack. It segfaulted the
first time an image reached it.

All 8 have that shape. This converts them rather than reasoning about which are
safe: the frame's offsets come from the revision before Phase 2 — the only place
they still exist — the types come from the current file so later retyping and
struct recovery are kept, and the result carries `static_assert`s on both the
total size and the offset where the named part ends.

Two things it has to cope with, both created by later phases. A slot's name may
have four different alias spellings by now, so the owner is taken as the last
identifier in the initialiser rather than by matching each shape. And a slot may
have no declaration at all, because later work stopped using it — that becomes
padding, since the bytes are still part of the layout.

All eight are converted, and with them all 24 frames. `model_plane` looked like
it would need declaration-list surgery and did not: one of its members has a
trailing comment, which a pattern anchored at the end of the line cannot see
past, and the tool assumed a frame's declarations sit together when later
phases had moved some apart. Two properties of the tool, mistaken for
properties of the code.

## `shape.py` — count what is still written as arithmetic

```
python3 tools/shape.py                  # the table REFACTORING3.md §1 quotes
python3 tools/shape.py --this           # raw offsets off `_this`, by function
python3 tools/shape.py --frames         # one line per frame
python3 tools/shape.py --bss            # the bmf_bss globals, grouped
python3 tools/shape.py --overlap Obj8 Obj11 Obj19 Obj31 Obj69
```

This reads nothing and rewrites nothing; it is a scoreboard, the way
`BMF_STRICT=1 ./build.sh` was round two's. Round three's defect is an object
whose layout is known by address arithmetic rather than by declaration, and the
three shapes that means in this file are counted at the top of the script as
`P1`, `P2` and `P3`:

```
   (T *)(base + …)          (T *)((char *)base + …)
   ((T *)base + …)          — the same, with the offset scaled
   … + (char *)base         — Hex-Rays' reversed form, where the index came first
```

`base` has to be a plain identifier, so `(char *)v->f4 + n` is not counted;
allowing member expressions raises the total by about 60. Comments are stripped
first — `deadcheck.py` below learned that lesson the expensive way.

The reason it exists is that a plan full of counts nobody can re-derive is a
plan that gets believed. Round two's first draft had three numbers in it that
were wrong, and each was wrong because the count came from reading rather than
from a command. Everything in REFACTORING3.md §1 is one of these sub-commands.

**It is a counter, not a target.** `exclusion_mask[symbol]` is a byte buffer
being indexed and will never become a member; the number moving is a signal that
something changed, not that something improved.

## `deadcheck.py` — report code nothing can reach

```
python3 tools/deadcheck.py subs1.hpp
```

Four kinds of unreachable code have been found in this file, each after the
previous one had been declared the last:

- a label with no `goto` left (`LABEL_47`, orphaned by a deletion);
- an always-true test as a statement (`if ( 1 ) goto LABEL_52`, left by the
  mode-folding pass, with everything up to the label dead behind it);
- a test on a global the dispatch pins, in **either** spelling —
  `if ( !plane_predictor )` and `if ( plane_predictor ) … else`. The first
  search wrote only the first spelling and missed a 22-line else;
- a body nothing calls.

Two of those were found by checks written *after* the case they would have
caught, which is the wrong order. This is the check standing on its own: run
against the tree as it was before any of the four were removed it reports all
four, and against the tree now it reports none.

What it cannot derive it is told, and says so: that the alternate model
subsystem is entered only under predictor 1 or 2, and that `unpredict_med`'s
two call sites both guard on predictor 1. Each fact is checked where it is
used, and the closed set of bodies it applies to is computed rather than
listed.

One thing it learned the hard way: it reads code with comments stripped. A
comment describing deleted code quotes it, so the first run found the very
thing it was written to find, in the note saying that thing was gone.

## `defram.py`, `runarray.py`, `merge.py`, `unbss.py` — round three

Four rewriters and a sweep, one per shape REFACTORING3.md names.  Each has its
own docstring; what they share is that their rules are static analysis over
decompiled code that indexes past the ends of things, so the rules propose and
the gate decides.

```
python3 tools/defram.py subs1.hpp --list      # frame members that can lift
python3 tools/runarray.py subs1.hpp --list    # runs walked as arrays
python3 tools/merge.py subs1.hpp --plan ModelBlock Obj10
python3 tools/unbss.py subs1.hpp --list       # globals still at a 1997 address
tools/frame-sweep.sh                          # lift, gate, keep or revert
```

Between them they took `bmf_bss` from 60 globals to none, 602 frame aliases to
396, and 24 walked runs to 10.  Four things they were wrong about first, all
four caught by the gate rather than by reading, and all four now in their
docstrings because the next person will believe the rule otherwise:

- **An address that escapes does not stop at the member it came from.**
  Lifting the forty members after `alt_p2_context`'s `&v275` segfaults every
  image but the one-plane one.  An address-taken member pins the whole frame.
- **An array member reaches its neighbours without naming them.**
  `model_planes` writes a four-word image descriptor through `uint16_t p_i[2]`
  into the three members after it.
- **`&x[i]` is `x + i`.**  Reading it as address-taking pinned six frames that
  have nothing wrong with them.
- **Two four-byte pointers of different types at one offset is the dangerous
  merge, not two members of different widths.**  `merge.py`'s first version
  compared widths and silently took one of `Obj10::f6059436` and
  `ModelBlock::f6059436`; round two moved three streams on exactly that.
- **A probe that measures where the compiler put the lifted members has
  recorded the build, not the layout.**  The fifth one, and it took a bug
  report to find, three rounds and a repository after the lift that caused it.
  `choose_plane_coding`'s six-member run — `acc0 acc1 d4 tbl16 tbl64a tbl64b`,
  256 bytes — was lifted, and ASan reported the zero loop above them running
  136 bytes past the end on any 24bpp or 32bpp image.  The repair declared a
  256-byte buffer and bound three of the six names into it *at the offsets a
  probe had just measured* — `tbl16` at 56, where the frame has `d4` — and
  left the other three outside.  Every write was then in bounds, ASan went
  quiet, and every read of the second and third 64-byte table pointed at
  bytes nothing had written.  The streams did not move, because every image
  in the corpus reads the first table.

  What makes it the fifth rule rather than a footnote: the measurement was
  real and the reasoning from it was backwards.  Where a lifted member *is*
  says nothing about where the body needs it; that comes from what indexes
  across it, which here was `&tbl16[16*xform]` and a zero loop whose 192
  bytes are exactly three 64-byte tables.  Both were in the source the whole
  time.  The check after a lift is not "does the gate pass" — it is "does
  anything index out of a member, and does the corpus reach the code that
  does".

### What the four rewriters had wrong, and how each was caught

Nine rules, every one of which produced a plausible wrong answer first.  They
are in the scripts' docstrings; collected here because the pattern matters more
than any of them: **six of the nine were caught by the gate, two by the
compiler, and one by the heap allocator.  None by reading.**

| rule | what it cost |
| --- | --- |
| an escaping address does not stop at its own member | 40 members lifted, every image but the one-plane one segfaulted |
| an array member reaches its neighbours without naming them | `model_planes` passed three tests, died on five images |
| `&x[i]` is `x + i`, and `&p->f` is a field of the pointee | eight frames pinned for no reason |
| a fixed-count `fread`/`fwrite` inside the member's width is bounded | two more |
| two four-byte pointers of different types at one offset is the dangerous merge | `merge.py` silently took one; round two moved three streams on it |
| a scalar at an array's offset is its element 0 | eight compile errors |
| `uint8_t  *` and `uint8_t *` are one type | the above, again |
| an extra view belongs to the member that spans it, keyed by its own offset | three views dropped, three images segfaulted |
| a rewrite that keeps the offset can still change the width | `uint16_t` store became `uint32_t`, heap corruption |

The two that are worth carrying into any future round: **signedness is part of
a field's type** (`int32_t f0` against `uint32_t f0`, and one comparison
against `-10`, is four streams), and **when a walk looks unbounded the frame
usually knows the bound** — a run that ends in a pad is a run whose length is
the pad.

## every script in this directory

Generated by `tools/readme.py` from each script's own first line; run
`python3 tools/readme.py --apply` after adding or renaming one. The last column
is read out of `sweep.sh`'s skip list, not restated here: `yes` means the sweep
holds that tool's count to zero, and `no` means it is a generator, a module
something else imports, or a tool that answers with proposals rather than a
count. A `.sh` script is not in the sweep at all — the sweep is what several of
them run.

<!-- readme.py: generated, do not edit by hand -->

| script | what it is | in the sweep |
| --- | --- | --- |
| `abpair.py` | Two names for one value, told apart by a trailing `a`/`b` | yes |
| `addrmap.py` | Recover which address in BMF.exe each named function came from | yes |
| `arrayify.py` | Collapse a run of consecutive same-type members into the array it is | yes |
| `asan.sh` | Run every test image under AddressSanitizer |  |
| `buildlog.py` | Read a build log, or say plainly that it is about another file | yes |
| `checktable.py` | Check §1's table against what `shape.py` says right now | yes |
| `collect_globals.py` | Collect subs1.hpp's global declarations into one sorted, deduplicated block | yes |
| `compact_locals.py` | Compact one decompiled function's block of local declarations | yes |
| `ctxidx.py` | Turn a masked context word into a `CtxIdx` chain, one line at a time | yes |
| `deadcheck.py` | Report code nothing can reach | yes |
| `deadparam.py` | Drop the parameters nothing reads, and the arguments that fed them | yes |
| `deblob.py` | Give every blob global its own definition, and see what breaks | yes |
| `decast.py` | Delete the casts that cast a thing to the type it already has | yes |
| `dedup.py` | Collapse recovered structs that are byte-for-byte the same declaration | yes |
| `defram.py` | Lift the frame members that are provably not part of a layout | yes |
| `degoto.py` | Turn `if (c) goto L; ... L:` back into `if (!c) { ... }` | yes |
| `dethread.py` | Delete the `const __m128 &` parameter thread | yes |
| `explicitcmp.py` | Write out the conversion a signed/unsigned comparison already performs | yes |
| `extents.py` | What each blob global is actually used as | yes |
| `firstuse.py` | Declare a local where it is first assigned, not at the top of the body | yes |
| `foldif.py` | Fold an `if` whose condition is now a known constant | yes |
| `frame-sweep.sh` | Lift one frame, run the gate, keep it or put it back |  |
| `fuzz.py` | Fuzz.py -- deterministic mutants of the test corpus | no |
| `fuzz.sh` | Run mutated inputs through an AddressSanitizer build |  |
| `hdrscan.sh` | Every value of the member header's one-byte fields, under ASan |  |
| `hex_constants.py` | Write subs1.hpp's pattern constants in hex instead of decimal | yes |
| `liftframe.py` | Turn one function's frame struct back into ordinary locals | yes |
| `merge.py` | Make several recoveries of one object into one struct | yes |
| `methodise.py` | Turn a function whose first parameter is its receiver into a method | yes |
| `mkaltp1.py` | Write testfiles/altp1.bmp — the image that reaches `alt_model_p1_decode` | no |
| `mkmed32.py` | Write testfiles/med32.bmp — the image that reaches `unpredict_med` | no |
| `mknoise.py` | Write testfiles/noise24.bmp — data that does not compress | no |
| `mkrefs.sh` | Regenerate testfiles/ref_<name>.bmf, the streams test.sh compares against |  |
| `mkrle.py` | Write the two run-length-encoded BMPs in testfiles/ | no |
| `name_raw_addrs.py` | Rewrite subs1.hpp's raw BMF_BLOB(0x...) addresses as named globals | yes |
| `namelocal.py` | Name a local after the member it is one assignment of | yes |
| `negindex.py` | Find `p[-u]` where `u` is unsigned — a subtraction that only i386 undoes | yes |
| `objects.py` | Group the names that denote the same allocation, and describe each object | yes |
| `outpath.py` | Refuse to write a generator's output over something that is not its output | no |
| `p1bump.py` | Fold `update_model`'s counter bumps into `CounterNode::bump` | yes |
| `p2freq.py` | Fold `alt_p2_model`'s rescale-then-bump pairs into `p2_freq_add` | yes |
| `p2update.py` | Fold `alt_p2_model`'s written-out counter updates into `p2_update` | yes |
| `perf.sh` | Gate C: has this change made the program slower? |  |
| `planedesc.py` | Take `plane_desc`'s header slot out, and re-index the four planes onto zero | yes |
| `proven.sh` | Which tools' answers depend on the file at all? |  |
| `prune_unreachable.py` | Delete the functions in subs1.hpp that nothing can reach any more | yes |
| `ptrwidth.py` | Classify the pointers this file still puts through a 32-bit integer | yes |
| `rawoffset.py` | Find numbers in the code that are a struct member's byte offset | yes |
| `readme.py` | Check that README.md lists every script in this directory, and what it is | yes |
| `reads.py` | What does a tool actually read? | yes |
| `reframe.py` | Give a plain-split frame back its layout, without giving back its casts | yes |
| `rename.py` | Rename identifiers in subs1.hpp, one whole-word at a time | yes |
| `resign-drive.sh` | Apply every retype that measurably reduces the conversion count |  |
| `resign.py` | Give a local the signedness of the values it actually holds | yes |
| `resign_group.py` | Give a set of locals that must agree the same signedness | yes |
| `retype.py` | Retype the locals a function uses as pointers, from int32_t to char * | yes |
| `retype_locals.py` | Give a local the type the compiler says its assignments have | yes |
| `runarray.py` | Declare a run of frame members as the array the code walks it as | yes |
| `shape.py` | Measure the things REFACTORING3.md is about, so its numbers can be checked | yes |
| `split.py` | Split bmf.cpp into one file per class and per large function | no |
| `struct-sweep.sh` | Give the largest remaining objects a struct, one at a time, keeping only the ones the gate accepts |  |
| `structs.py` | Give one of the program's objects a struct, and use it | no |
| `sweep.sh` | Ask every tool in this directory what it still finds |  |
| `tidy_structs.py` | Clean up what `structs.py` had to write conservatively | yes |
| `triage.sh` | Compress a few images and compare against their reference streams |  |
| `unalias.py` | Fold a frame alias into the member it binds, REFACTORING4.md §5 item 2 | yes |
| `unaliasvar.py` | Fold a local that is one assignment of another local | yes |
| `unbss.py` | Give a `bmf_bss` global storage of its own | yes |
| `uncast.py` | Delete the casts the compiler calls useless | yes |
| `uncastwidth.py` | Classify `*(T *)&x.m` by the width of the cast against the width of `m` | yes |
| `uncopy.py` | Delete a local that is only ever a copy of another local | yes |
| `uncopyrec.py` | Turn an 18-byte `bmf_copy` into a record assignment | yes |
| `uncursor.py` | One cursor for a run of record moves, instead of one reload each | yes |
| `undef.py` | Definitions in bmf.cpp that nothing uses | yes |
| `undup.py` | Collapse an `if`/`else` whose two arms are the same code | yes |
| `unframe.py` | Turn one function's `__hexrays_frame` buffer back into ordinary locals | yes |
| `unhoist.py` | Put back a load the compiler hoisted out of the expression that uses it | yes |
| `unify_types.py` | Rewrite subs1.hpp's type vocabulary onto <cstdint> | yes |
| `unindex.py` | Turn `((T *)p)[expr + K]` into the array member of `p` that `K` lands in | yes |
| `unjump.py` | Turn a `goto` into the block it jumps into | yes |
| `unlane.py` | Turn `p->f278528[i].m128_XX[k]` into the member of `Obj11` at that offset | yes |
| `unlayer.py` | Drop a pointer cast the next cast immediately overrides | yes |
| `unloword.py` | Give a local the width its writes and reads agree on, not the register's | yes |
| `unmemcast-sweep.sh` | Retype one frame member, run the gate, keep it or put it back |  |
| `unmemcast.py` | Give a frame member the type its readers cast it to | yes |
| `unnamed.py` | Names still spelled the way Hex-Rays spelled them, asked of the source | yes |
| `unoffset.py` | Turn `*(T *)((char *)p + N)` into the member of `p` that lives at N | yes |
| `unp2.py` | Give the p2 counter table's cursors the record type they point at | yes |
| `unrec.py` | Fold an unrolled 18-byte record copy into one assignment | yes |
| `unrecast.py` | Drop the cast when the base already has that type | yes |
| `unrechoist.py` | Fold an 18-byte record copy whose loads MSVC hoisted above its stores | yes |
| `unreload.py` | Load a member once where the decompilation loads it over and over | yes |
| `unsave.py` | Delete a local that saves a value across a region that cannot change it | yes |
| `unscalar.py` | Delete a struct that is a run of one scalar, and point at the scalar | yes |
| `unshim.py` | Inline the `__fwd_*` shims and call the function they call | yes |
| `unslot.py` | Give a frame slot's extra names their own storage | yes |
| `unspell.py` | One address, written more than one way | yes |
| `unspill.py` | Give a frame's spill area one member per slot | yes |
| `unstale.py` | Names the algorithm documents spell that the program no longer has | yes |
| `unstruct.py` | Turn a struct that is really an array back into one | yes |
| `untable.py` | Say a file-scope table once instead of three times | yes |
| `untail.py` | Replace a `goto` to a straight-line return with the statements themselves | yes |
| `untemp.py` | Inline a local that is assigned once and read once | yes |
| `unused.py` | Delete the locals nothing uses, with the compiler saying which | yes |
| `unwiden.py` | Give a cursor the type its dereferences read, not the one it was declared | yes |
| `unwrite.py` | Delete a local that is written once and never read | yes |
| `x32.sh` | What the 32-bit build still gets wrong |  |
| `x64diff.sh` | Do the two pointer widths answer the same on inputs nobody chose? |  |

<!-- end readme.py -->
