# Refactoring BMF 2.01

A plan for turning the decompilation into source you would be willing to
maintain, and to build for x86-64.

`ALGORITHM.md` says what the program *does*. This says what to do to the code,
in what order, and how to know each step did not break it. Every number below
was re-measured on the tree at the commit that last touched this file; the
commands that produced them are in [Appendix A](#appendix-a--how-the-numbers-were-measured)
so they can be re-measured rather than trusted.

---

## 1. Where this is now

| | | at the start |
| --- | --- | --- |
| `subs1.hpp` | 18 609 lines | 25 462 |
| bodies | 179 (84 real, 95 `__fwd_*` shims) | 215 |
| globals in `blob.inc` | 164 | 293 |
| pointer casts | 5680 | 7336 |
| `goto` / `LABEL_n:` | 123 / 90 | 174 / 127 |
| `__hexrays_frame` | **0** | 24 buffers, 935 aliases |
| x86-64 int↔pointer | 1211 warnings, 1708 lines | 4371 / 3965 |
| line coverage | **89.97 %** | 64.5 % |

The previous rounds of work (see `tools/README.md`) fixed the vocabulary —
stdint types, one declaration per global, no WinAPI, no Intel intrinsic
dispatcher, no raw addresses, a real `RangeCoder` class. What is left is the
part that needs *understanding* rather than substitution.

---

## 2. The gate

Everything below rests on one property: **a refactoring step is correct if the
compressed stream is byte-identical to the one the previous build produced, and
every image still round-trips.** No test can tell you a rename was
*meaning-preserving*, but this tells you it was *behaviour*-preserving, which is
the property that matters.

```
./build.sh && ./test.sh ./bmf     # 10 images, encode + decode + compare,
                                  # each stream against testfiles/ref_<name>.bmf
```

Both halves are automated: the round-trip, and the comparison against the ten
committed reference streams. A run with a missing reference fails, and a stream
that differs from its reference fails. `tools/mkrefs.sh` regenerates them, and
is to be run only when a change is *meant* to move the output.

### 2.1 One mode, and only the code that implements it

BMF read six switches from its .ini and its command line. This program has one
mode: `bmf c` pinned `-S` and `-Q9` at run time and let the other four keep the
values BMF.exe's data segment starts them at. That left about a third of the
file compiled, reachable from `main`, and never executed — and code no test runs
cannot be refactored.

They are constants now, at the top of `subs1.hpp`:

```c
static constexpr int32_t __dword_44108C = 1;   // -F  use filters
static constexpr int32_t __dword_441090 = 1;   // -S  slow but efficient
static constexpr int32_t __n2_4         = 0;   // -T  filter template
static constexpr int32_t __dword_441098 = 1;   // -N  pack the output
static constexpr int32_t __n7_0         = 9;   // -Q  filter search quality
static constexpr int32_t __n7_1         = 0;   // -E  max error, near-lossless
```

Every branch that tested them is folded to the side that runs, and what that
leaves unreachable is deleted: the `-T1` template builder and the `-T2` reader,
the `-Q<9` tile cap, the fast-mode encoder back end (`sub_408510` and the eight
bodies it reaches), the fast-mode decoder back end (`sub_40CF80`), and the
near-lossless encoder and expander. 38 bodies, and the globals that went with
them.

Two of those bits arrive from the stream rather than from the constants — bit 2
of the descriptor is `-S`, and the 4-bit field beside it is `-E` — so folding
them was a decision, not a substitution. A stream that asks for fast mode or for
`E>0` is one this build no longer has the code to expand, and is **refused with
a message** at the point the field is read rather than decoded wrongly.

| | before | after |
| --- | --- | --- |
| `subs1.hpp` | 25 462 lines | 17 968 |
| bodies | 215 | 178 |
| globals | 293 | 170 |
| line coverage | 64.5 % | **89.9 %** |

That is the coverage gap closed by deletion rather than by testing more modes,
and it is the right way round: a mode this program does not offer is not a thing
to keep working, it is a thing to remove.

### 2.2 The linker as the dead-code check

`build.sh` compiles `-ffunction-sections -fdata-sections` and links
`--gc-sections`, so a function nothing reaches never reaches the binary whether
or not it has been deleted from the source. `BMF_GC=list ./build.sh` prints what
was discarded, which is a better dead-code report than a textual call graph: it
sees through the branches the constants fold away.

It is a report, not an authority. It currently discards one body whose source is
deliberately kept — `sub_410650`, the predictor-mode-0 expander, whose call
sites the optimiser proves unreachable at `-O1` and above but not at `-O0`. That
reasoning may be exploiting undefined behaviour rather than a real
impossibility, and `ALGORITHM.md` §9 lists predictor mode 0 as a mode whose
meaning is not established. Deleting a body needs a better reason than "`-O1`
thinks so".

### 2.3 What the gate still does not reach

10.1 % of the file, concentrated in two bodies: `sub_427740` (290 lines) and
`sub_410310` (130). Both sit in the alternate model families `ALGORITHM.md` §9
flags as unread. Neither is dead — nothing has shown their entry condition to be
impossible — so they are refactored last and with more care than the rest, or
their entry condition is established first and the corpus grown to reach them.

Ten similar images is a signal, not a proof. Add images when a phase touches
something the corpus does not exercise.

## 3. What "done" looks like

1. No global is a reference into a byte array. Each is a definition with its own
   type, its own extent, and an initialiser where the original had one.
   `blob.inc` is deleted.
2. No integer holds a pointer. `-m64` builds warning-clean and round-trips every
   test image.
3. The recurring `base + constant` families are `struct`s, and the variables
   that walk them are typed pointers to those structs.
4. Names say what things are. No `__sub_41CAB0`, no `v187`, no `n0x800000`.
5. `casts`, `LOBYTE`-family macros, `goto`, and `__hexrays_frame` are gone or
   reduced to the handful of places where they are genuinely the clearest
   expression.

Item 2 is the only one with a mechanical pass/fail; the rest are judgement, and
the gate protects them rather than defining them.

---

## 4. The two things that make this harder than it looks

### 4.1 The "globals" are not all globals

36 of the 169 remaining declarations **overlap the one after them**. Some are IDA
guessing an array bound (`__dword_438AFC[0x10000]` running over
`__dword_438B00`), which is harmless. Others are not:

```c
*(uint64_t *)((char *)&__n4_5          + n16 * 4) = ...;   // 0x443394
*(uint64_t *)((char *)__n256_2         + n16 * 4) = ...;   // 0x44338C
*(uint64_t *)((char *)&__dword_443384  + n16 * 4) = ...;   // 0x443384
*(uint64_t *)((char *)&__Buffer        + n16 * 4) = ...;   // 0x44337C
```

Four separate "globals", eight bytes apart, each written eight bytes at a time
at the same stride — with `n16` stepping 16, 8 they cover `0x44339C`–`0x4433DC`
contiguously, which is the 16-byte per-plane descriptor table `ALGORITHM.md` §8
places at `0x44339C`.

All four *are* real objects: `__Buffer` is the coded buffer's base pointer
(`__Buffer = malloc(...)` a few hundred lines away), `__n4_5` is the plane
count, and so on. That is what makes this nastier than a mislabelled variable.
The compiler strength-reduced a walk over the descriptor table into four base
pointers, and picked as those bases the addresses of four unrelated variables
that happen to sit 32 and 64 bytes below it. The code therefore depends on
**where these globals are relative to each other and to a table none of them
names**.

Give them independent definitions and the linker will place them wherever it
likes; the writes still happen, 32 and 64 bytes past each, into whatever is
there. It compiles, it runs, and it corrupts. **Extent and aliasing analysis has
to come before de-blobbing, not after** — and for regions like this one the
answer is a single `struct` covering `0x44337C`–`0x4433DC`, not four variables
and a table.

### 4.2 Pointer typing and structure recovery are one job

The x86-64 problem was 3784 `cast to pointer from integer of different size`
warnings across 3233 lines (1211 across 1708 now — see Phase 4), and they look
like this:

```c
*(uint32_t *)(_this + 278736) = v17 + 144;
```

Two defects in one expression: `_this` is an `int32_t` holding a pointer (the
x64 bug) and `278736` is a field offset (the structure). You cannot fix either
alone — retyping `_this` to `char *` leaves `+ 278736`, and naming the field
requires a struct that `_this` must already point to. So they are one pass, done
**per object**, converting every function that touches that object at once.

The objects, by traffic. The right-hand column is what is *known*, not what is
guessed — `ALGORITHM.md` §9 lists the alternate model families as not worked out,
and this table does not pretend otherwise:

| pointer | offset range | distinct | refs | touched by | what is known |
| --- | --- | --- | --- | --- | --- |
| `_this` | `0x64`–`0x5C75AC` | 72 | 200 | `sub_416860`, `sub_416C90`, `sub_419430`, `sub_4229E0`, … | the model block: a `0x44000`–`0x44144` header (52 fields, 405 refs across all pointers) then tables out to 6.7 MB. Allocated by `sub_414F60` / `sub_4149C0` |
| `lpAddress` | `0x11021`–`0x447B0` | 33 | 104 | `sub_419610`, `sub_422DB0` | an alternate-model working set — **role not established** |
| `n5_2` | `0x5D8`–`0xEEA` | 7 | 97 | `sub_424D90`, `sub_4259F0` | a ~3.8 KB record in the alternate model family — **role not established** |
| `a1` | `0xC20`–`0x65E7B0` | 32 | 96 | `sub_417980`, `sub_41CAB0` | reaches the same 6.7 MB extent as `_this`, so almost certainly the model block under another name — confirm before merging |
| `v56` | `0x44000`–`0x440F4` | 21 | 51 | `sub_423600` | the model block's header only |
| `n0x10_2` | `0x3960C`–`0xE5836` | 14 | 46 | `sub_41CAB0` | inside the model block's tables |
| — | `0x44339C`, 16 B stride | — | ~90 | many | the per-plane descriptors, reached through the four false bases in §4.1 |

Three things this table says that matter for sequencing.

**The model block is reached under at least four different pointer names**
(`_this`, `a1`, `v56`, `n0x10_2`), just under 400 references across a dozen
functions that never see each other's signatures. Its struct has to be declared once and applied
everywhere at once, not discovered per-function.

**The two families whose role is unknown** (`lpAddress`, `n5_2`) sit inside the
alternate model code `ALGORITHM.md` §9 flags as unread, so structure recovery
there is blocked on reading it — which is why Phase 4 orders them last.

**The gate covers this work.** The two functions §2.3 lists as still unreached
are `sub_427740` and `sub_410310`; neither is a model-block user. Phase 4's
largest object is entirely inside covered code. It is Phase 3, which touches
every global whether covered or not, that carries what exposure remains.

---

## 5. Order of work — status

Each phase ends green: build, ten images round-trip, every stream byte-identical
to its reference. That held for every commit; the gate has never been red at a
commit boundary.

```
0  the gate, and one mode      done      §2
1  names you already know      done      39 identifiers
2  frames -> real locals       done      0 frames left
3  un-pin the globals          part      the pointers are out; the blob is not
4  objects + pointers          part      every local typed; the fields are not
5  casts and the vocabulary    part      the useless ones are gone
6  control flow                not done  and not for the reason the plan gave
```

### Phase 0 — done

§2. Reference streams committed and the comparison made fatal; the mode made
constant and everything the other modes reached deleted; `--gc-sections` so what
is dead cannot ship. 25 462 → 17 968 lines, coverage 64.5 → 89.9 %.

### Phase 1 — done

39 identifiers from `ALGORITHM.md` §8 and §10, 1042 occurrences, nothing
speculative. `tools/rename.py` is scope-aware and refuses rather than guesses.

### Phase 2 — done

All 24 frames gone. **The plan said "expect 3–6 to resist" and 16 did.** Those
frames have bytes no alias names — `sub_405CF0` names 29 220 of 41 456 — and the
code reaches that slack by running off the end of the alias next door. They keep
their layout as a struct with explicit padding and lose only the casts, each
carrying `static_assert(sizeof(__frame) == N)` so a layout that moves is a
compile error rather than something ten images have to notice.

### Phase 3 — the pointers are out, the blob is not

Done: the four false bases from §4.1 retired, so no global's position matters to
another; five globals that held addresses in `int32_t` words are real pointers
outside the blob (`coded_buf`, `out_cursor`, `packer_word`, `hist_scratch`,
`model_tables`); the error message table decoded and inlined, which is what
`__exit_402E40` used nine relocated pointers for.

Not done: 164 globals are still slices of `blob.inc`. The blocker is extents.
`tools/extents.py` says 120 of them are never subscripted and 44 are subscripted
by an expression — and for those 44 the declared `[0x10000]` is a guess, so
whether `__dword_4398C0[i]` is one dword or the first of thirteen consecutive
ones that Hex-Rays split into thirteen names is not answerable from the source.
It is answerable at run time: give every global its own definition with a guard
gap and see what breaks. That is the next step and it is a day's work, not an
open question.

### Phase 4 — every local is typed; the fields are not

3784 → 1211 int↔pointer warnings. `tools/retype.py` converted every local and
parameter used as a pointer base: 189 candidates, `char *` where the variable is
only ever an address and `uintptr_t` where the code also masks or tags it. There
are no candidates of that kind left.

What remains is one shape:

```c
*(uint8_t *)(*(uint32_t *)(_this + 76) + 6)
```

The local is typed; the **field** at `+76` is not. It is a pointer kept in a
`uint32_t` inside a structure this program allocates itself, so fixing it means
changing that structure's layout, which means knowing what the structure is.
§4.2 said pointers and structures are one job and this is where that bites: the
two biggest holders are the model block and the alternate model families
`ALGORITHM.md` §9 lists as unread. **This part is reading, not rewriting**, and
it is the largest single piece of work left in this document.

### Phase 5 — partly, and smaller than it looked

The 38 casts GCC calls useless are gone with the retyping. The rest of the 5680
are not redundant: they are the type punning the decompilation is made of, and
most will fall out of Phase 4's structures rather than of a pass of their own.
The `LOBYTE`/`HIWORD` family is down to 265 uses; those that are field access
into a recovered struct should go with that struct, and only genuine bit-packing
should be rewritten here.

### Phase 6 — not done, and the plan was wrong about why

The plan said the 123 `goto`s are "most of them one of four shapes (loop
`continue`, loop `break`, early `return`, shared error tail) and rewrite
mechanically". Measured: **not one label in the file is followed by a `return`,
a `break` or a `continue`.** 55 are followed by an assignment and 21 by an `if`.
There are no unused labels and no `goto` whose label is the next line.

This is irreducible control flow — the cases Hex-Rays could not structure — and
restructuring it needs each function read and understood. It is correctly last,
but it is not the mechanical pass the plan promised, and doing it before Phase 4
finishes would be introducing bugs the gate cannot distinguish from a layout
change.

## 6. Things that will bite

- **The code reads memory it does not own.** It indexes past array bounds,
  aliases one type through another, and depends on adjacency. Every phase that
  changes layout — 2, 3, 4 — can change behaviour without changing meaning.
  This is why the gate is a *byte-identical stream*, not "it still compresses".
- **`bmf` opens its output `"w+b"` and reads it back**, and that read-back is
  not only walking an archive — it initialises writer state. A harness that
  reuses one output path, or a "cleanup" that removes the read, breaks things
  in ways that look like a refactoring bug. (Both were hit; see
  `git log 4c0a19c`.)
- **Inherited names lie, and so do first readings.** `__n8`, `__n256`, `__n2`
  are the last value assigned, not the meaning. But the opposite error is just
  as easy: an earlier draft of §4.1 concluded `__Buffer` was not a buffer,
  because its address is used as a base for writes 32 bytes away. It is a
  buffer — `__Buffer = malloc(...)` — and it is *also* a strength-reduced base.
  Read every use before renaming, not the first few.
- **Coverage is not correctness.** 90 % line coverage of a codec still misses
  the branch that only a 5-plane 16-bit image takes. Add images when a phase
  touches something the corpus does not reach.
- **Do not trust a green gate on a change you did not understand.** Ten images
  is a signal, not a proof — and it is a thinner one than it sounds, because
  they are ten *similar* images. Arithmetic that runs identically on all ten can
  still differ on the eleventh.
- **Deleting beats testing, where the code is not wanted.** Phase 0 closed a
  35-point coverage gap by removing five modes this program does not offer,
  not by building a matrix to exercise them. Reach for that first: a mode with
  no interface is not a feature to keep working, and a test that keeps dead
  code alive is a cost with no return.

---

## 7. What is left

| | work | why it is not done |
| --- | --- | --- |
| 3 de-blob | 164 globals | 44 extents unknowable from the source; needs the guard-gap run |
| 4 structures | ~1211 warnings | the model block and the alternate model families have to be *read* first |
| 5 casts | ~5680 | most fall out of 4; the rest is `LOBYTE`-family bit-packing |
| 6 control flow | 123 gotos | irreducible; needs each function understood, and needs 4 |

The order among these is forced. 3's guard-gap run is independent and can go
next. 4 gates 5 and 6. 6 last, still.

Two of this document's own predictions were wrong, and both were wrong in the
same direction — assuming a mechanical pass where the code had something real to
say:

* Phase 2 expected 3–6 frames to resist splitting; 16 did, because the frames
  carry unnamed slack the code runs into.
* Phase 6 expected the `goto`s to be four rewritable shapes; none of them is.

The gate caught the first before it shipped. The second was caught by measuring
before starting, which is the cheaper of the two.

## Appendix A — how the numbers were measured

```sh
# sizes and vocabulary
wc -l subs1.hpp                                       # 18609
grep -o 'blob1 + 0x' subs1.hpp | wc -l                # 164 globals
grep -c 'static inline .*__fwd_' subs1.hpp            # 95 shims
grep -oE '\((const )?[A-Za-z_][A-Za-z0-9_]* *\*+\)' subs1.hpp | wc -l   # 5680
grep -c 'goto ' subs1.hpp                             # 123
grep -c '__hexrays_frame' subs1.hpp                   # 0

# x86-64 work list
g++ -m64 -march=x86-64 -msse2 -std=c++17 -fno-strict-aliasing -fpermissive \
    -fno-rtti -fno-exceptions -O0 -DNDEBUG -c bmf.cpp -o /dev/null 2>x64.log
grep -c 'cast to pointer from integer' x64.log        # 1211
grep -E 'warning: (cast|invalid conversion)' x64.log | \
    grep -oE '^[^:]+:[0-9]+' | sort -u | wc -l        # 1708 lines

# dead code, as the linker sees it
BMF_GC=list ./build.sh 2>&1 | grep 'removing unused section'

# coverage
g++ -m32 -march=k8 -msse2 -mfpmath=sse -std=c++17 -fno-strict-aliasing \
    -fpermissive -fno-rtti -fno-exceptions -O0 -DNDEBUG -U_FORTIFY_SOURCE \
    -D_FORTIFY_SOURCE=0 --coverage bmf.cpp -o bmfcov
for f in testfiles/*.bmp; do
  rm -f o.bmf o.bmp                       # bmf opens its output "w+b"
  ./bmfcov c "$f" o.bmf && ./bmfcov d o.bmf o.bmp
done
gcov -n    -o . bmfcov-bmf.gcno                       # 89.97 % of 13313 lines
gcov -f -n -o . bmfcov-bmf.gcno                       # per function
```

Body sizes, global extents and overlaps, and the `base + constant` families in
§4.2 were all measured with throwaway brace-matching scripts, not with anything
in `tools/`.  What is in `tools/` is what got run repeatedly: `foldif.py`,
`rename.py`, `unframe.py`, `retype.py`, `extents.py`, `mkrefs.sh`. Turning the last two into real scripts is the first task of
Phase 3 and Phase 4 respectively; they are also what would keep the numbers in
this document honest as the work proceeds.

One caveat on the coverage figure: `gcov` counts *instrumented* lines (13 313
of the file's 18 609) and counts inlined copies separately, so its per-function
percentages do not sum the way source lines do. The line counts in §2 are source
lines, measured separately by matching braces over the function list.
