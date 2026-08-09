# tools/

One-shot migrations that were run over `subs1.hpp`, kept because the file is
generator output: if it is produced again from the extractor, these are what
has to be replayed on top.

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
    python3 tools/compact_locals.py subs1.hpp out.hpp __sub_402EF0 2

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

One thing to know if you re-run this: `bmf` opens its output with `"a+b"` — the
original appends images to an archive rather than replacing one — so a harness
that reuses one output path accumulates streams and every digest it computes is
different from the last.  Delete the output before each run.

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

69 objects converted; 81 are on the skip list. `REFACTORING.md` §Phase 4 has the
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
filled in. All 55 are mapped.

Matching bodies by the constants they contain is the obvious alternative and
is in this file's history. It does not work: an encoder and its decoder test
the same numbers, so it mapped `encode_context_bit` and `decode_context_bit` to
one address and gave no sign that anything was wrong.
