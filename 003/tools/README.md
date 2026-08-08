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
| `compact_locals.py` | merged one function's one-per-line locals into comma lists — `int32_t v5; int32_t v8;` → `int32_t v5, v8;` |

    python3 tools/unify_types.py subs1.hpp
    python3 tools/collect_globals.py subs1.hpp
    python3 tools/prune_unreachable.py subs1.hpp          # report
    python3 tools/prune_unreachable.py subs1.hpp --apply  # rewrite
    python3 tools/compact_locals.py subs1.hpp out.hpp __sub_402EF0 2

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
