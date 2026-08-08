# tools/

One-shot migrations that were run over `subs1.hpp`, kept because the file is
generator output: if it is produced again from the extractor, these are what
has to be replayed on top.

Both are idempotent — running them on an already-migrated file changes
nothing — and neither touches `bmf.cpp` or `blob.inc`.

| script | what it did |
| --- | --- |
| `unify_types.py` | rewrote the type vocabulary onto `<cstdint>`: `_DWORD`/`unsigned int`/`size_t` → `uint32_t`, `unsigned __int16`/`_WORD` → `uint16_t`, `__int128`/`_OWORD` → `__m128i`, and so on, leaving no multi-word type name in the file |
| `collect_globals.py` | moved the per-function typedef + global declaration pairs to the top of the file, sorted them by their address in BMF.exe's data segment, and kept one declaration per object instead of one per using function |
| `prune_unreachable.py` | deleted the function definitions nothing can reach from `main`, and then the globals-block entries no surviving body names |

    python3 tools/unify_types.py subs1.hpp
    python3 tools/collect_globals.py subs1.hpp
    python3 tools/prune_unreachable.py subs1.hpp          # report
    python3 tools/prune_unreachable.py subs1.hpp --apply  # rewrite

All three assert loudly rather than guessing: `collect_globals.py` refuses to
run if two functions disagree about an address's type, if two addresses claim
the same IDA name, or if a name it is about to rewrite could be a local rather
than the global.  `prune_unreachable.py` works from a call graph, not a
heuristic — taking a function's address counts as reaching it — and is worth
re-running whenever the command line changes, since it is what says which
bodies the new one leaves stranded.

The gate for both was `./test.sh`, plus a run of every image in `testfiles/`
through every switch combination against a binary built from the untouched
sources, comparing the compressed stream, the recovered image, the exit status
and the console output of each. All 100 image/mode pairs matched, including the
modes where this decompilation crashes — it crashes identically.
