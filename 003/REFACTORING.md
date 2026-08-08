# Refactoring BMF 2.01

A plan for turning the decompilation into source you would be willing to
maintain, and to build for x86-64.

`ALGORITHM.md` says what the program *does*. This says what to do to the code,
in what order, and how to know each step did not break it. Every number below
was measured on the tree at the commit this file was added; the commands that
produced them are in [Appendix A](#appendix-a--how-the-numbers-were-measured)
so they can be re-measured rather than trusted.

---

## 1. Where this is now

| | |
| --- | --- |
| `subs1.hpp` | 25 460 lines, 216 bodies (102 real, 114 one-line `__fwd_*` shims) |
| largest bodies | 1861, 1576, 984, 933, 801 lines; 37 over 200 |
| globals | 293, all references pinned to `blob.inc` |
| — named only by their address | 237 (`__dword_443384`, `__byte_44339E`, …) |
| — carrying a name IDA guessed | 55 (`__Buffer`, `__n256_2`, …), several of them wrong |
| pointer casts | 7242 |
| `LOBYTE`/`HIWORD`/`BYTE1`… | 298 |
| `goto` / `LABEL_n:` | 174 / 127 |
| `__hexrays_frame` buffers | 34, with 935 aliases bound into them |
| x86-64 | compiles, links, **segfaults on the first image** |

The previous rounds of work (see `tools/README.md`) fixed the vocabulary —
stdint types, one declaration per global, no WinAPI, no Intel intrinsic
dispatcher, no raw addresses, a real `RangeCoder` class. What is left is the
part that needs *understanding* rather than substitution.

---

## 2. The gate, and the hole in it

Everything below rests on one property: **a refactoring step is correct if the
compressed stream is byte-identical to the one the previous build produced, and
every image still round-trips.** No test can tell you a rename was
*meaning-preserving*, but this tells you it was *behaviour*-preserving, which is
the property that matters.

```
./build.sh && ./test.sh ./bmf          # 10 images, encode + decode + compare
                                       # plus: md5 of every .bmf must not move
```

**The hole:** that gate exercises **64.5 % of `subs1.hpp`**. 22 bodies — 7198
lines, **28 % of the file** — never run at all. The reason is structural, not
accidental:

```c
__dword_441090 = 1;               // -S, slow but efficient   (subs1.hpp)
...
__dword_443384 = __dword_441090;
if ( __dword_443384 ) { /* -S back end */ } else { __sub_408510(...); }
```

The command line pins `-S`, so `__dword_443384` is always 1, so the **fast back
end is dynamically dead**: `sub_408510` (1576 lines) and its callees
`sub_40CF80`, `sub_40BEE0`, `sub_40E860`, `sub_40FAC0`, `sub_40DEB0`,
`sub_40B330`, `sub_40A8A0`, `sub_40AF40`, `sub_40F450`, `sub_40CC50`,
`sub_40E590`, `sub_40BBC0` — 6415 lines — are compiled, reachable from `main`,
and never executed. The near-lossless family (`sub_4111B0`, `sub_410AC0`) adds
783 more, because `-E` defaults to 0. 27 of the 114 `__fwd_*` shims exist only
to call into this dead set.

Refactoring inside that 28 % is unverifiable. **Phase 0 exists to close this
before anything else starts.**

---

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

58 of the 293 declarations **overlap the one after them**. Some are IDA
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
contiguously. These are not four objects. They are **four strength-reduced base
pointers into one array**, and the array is the 16-byte per-plane descriptor
table that `ALGORITHM.md` §8 places at `0x44339C`. `__Buffer` is
`&planes[0].field - 32`; it is not a buffer and it is not even an object.

Turning those four into four independent variables would compile, run, and
silently corrupt the descriptors. **Extent and aliasing analysis has to come
before de-blobbing, not after.**

### 4.2 Pointer typing and structure recovery are one job

The x86-64 problem is 4371 `cast to pointer from integer of different size`
warnings across 3965 lines, and they nearly all look like this:

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
| `_this` | `0x64`–`0x5C75AC` | 75 | 205 | `sub_40A8A0`, `sub_40E590`, `sub_416860`, `sub_416C90`, … | the model block: a `0x44000`–`0x44144` header (52 fields, 405 refs across all pointers) then tables out to 6.7 MB. Allocated by `sub_414F60` / `sub_4149C0` |
| `lpAddress` | `0x11021`–`0x447B0` | 33 | 104 | `sub_419610`, `sub_422DB0` | an alternate-model working set — **role not established** |
| `n5_2` | `0x5D8`–`0xEEA` | 7 | 97 | `sub_424D90`, `sub_4259F0` | a ~3.8 KB record in the alternate model family — **role not established** |
| `a1` | `0xC20`–`0x65E7B0` | 32 | 96 | `sub_417980`, `sub_41CAB0` | reaches the same 6.7 MB extent as `_this`, so almost certainly the model block under another name — confirm before merging |
| `v56` | `0x44000`–`0x440F4` | 21 | 51 | `sub_423600` | the model block's header only |
| `n0x10_2` | `0x3960C`–`0xE5836` | 14 | 46 | `sub_41CAB0` | inside the model block's tables |
| — | `0x44339C`, 16 B stride | — | ~90 | many | the per-plane descriptors, reached through the four false bases in §4.1 |

Two things this table says that matter for sequencing. First, **the model block
is reached under at least four different pointer names** (`_this`, `a1`, `v56`,
`n0x10_2`) in functions that never see each other's signatures — so its struct
has to be declared once and applied everywhere at once, not discovered
per-function. Second, the two families whose role is unknown
(`lpAddress`, `n5_2`) sit inside the alternate model code `ALGORITHM.md` §9
flags as unread; **structure recovery there is blocked on reading it**, which is
why Phase 4 orders them last.

---

## 5. Order of work

Each phase ends green: build, 10 images round-trip, stream digests unmoved.
Nothing moves to the next phase with the gate red.

```
0  widen the gate ───────────────────────► everything depends on this
1  names you already know
2  frames → real locals
3  un-pin the globals from the blob
4  objects + pointers, per object  ──────► this is the x64 work
5  casts and the Hex-Rays vocabulary
6  control flow
7  the dark 28 %: decide
```

### Phase 0 — Widen the gate

**Why first.** A third of the file cannot currently be refactored safely, and
you will not notice which third while you are in it.

1. Add a `BMF_MODE` escape to the CLI (env var or a hidden argument) that lets
   the test harness set `-S`/`-F`/`-E`/`-Q` again, without changing the
   documented `bmf c` / `bmf d` behaviour. The pins in `subs1.hpp` become
   defaults rather than constants.
2. Extend `test.sh` to a mode matrix: `{-S, fast} × {-E 0, -E 2} × {-Q 1, -Q 9}`
   over the same 10 images. Record a digest per cell. Expect crashes in some
   cells — the pristine decompilation segfaults outside `-S`; a *reproducible*
   crash is still a usable gate, and fixing those crashes is worth doing early
   because it is the same pointer bug the x64 build hits.
3. Add images that exercise what these 10 files do not: 16-bit, palette, 1×N and N×1,
   an image whose width forces every BMP row-padding case.
4. Re-measure coverage. **Target ≥ 90 % before Phase 3 begins.**

**Gate:** coverage number, plus the existing digest set unchanged.
**Size:** small — a day, mostly in `test.sh`.
**Risk:** low. Nothing in `subs1.hpp` changes except un-pinning two constants.

### Phase 1 — Names you already know

**Why here.** It is free, it is zero-risk, and every later phase is read-heavy.
Do not defer naming until "after the structure work" — the structure work is
what naming makes possible.

`ALGORITHM.md` §8 and §10 already establish ~25 globals and ~20 functions.
Apply exactly those, nothing speculative:

```
__Buffer_0      -> out_cursor            sub_402FE0 -> compress_image
__Buffer_1      -> packer_word           sub_403820 -> expand_image
__n256          -> packer_acc            sub_407EF0 -> colour_transform
__n8            -> packer_free_bits      sub_4108C0 -> predict_med
__n4_5          -> plane_count           sub_4043E0 -> search_filter
__n2            -> plane_predictor       sub_411700 -> estimate_cost
__n256_0        -> near_lossless_max     sub_4159E0 -> code_pixel
__n7_0          -> filter_quality        sub_42AB20 -> read_bmp
__dword_441090  -> opt_slow_mode         sub_42B0C0 -> write_bmp
```

Note what several of the IDA names are: `__n8`, `__n256`, `__n2` are the *value
last assigned* to the variable, not its meaning. `__Buffer` is not a buffer
(§4.1). Treat every inherited name as a guess.

**Tooling:** a scope-aware `rename.py`. 117 locals across 44 functions shadow a
global name and reach it through `::`; a textual rename corrupts those.
`collect_globals.py` already contains the shadow detection to reuse.

**Gate:** digests unmoved. A rename that changes a digest is a rename that hit
something it should not have.
**Size:** ~45 identifiers, one commit each or one commit per group.
**Risk:** very low, and entirely caught by the gate.

### Phase 2 — Frames → real locals

**Why before the global work.** Self-contained per function, no cross-file
consequences, and it removes 935 lines of alias boilerplate that otherwise sits
between you and every body you are about to read. It is also the cheapest place
to build confidence in the gate.

```c
alignas(16) uint8_t __hexrays_frame[26712];
int32_t &v85 = *(int32_t *)(__hexrays_frame + 24608);
uint8_t * &v98 = *(uint8_t * *)(__hexrays_frame + 26668);
```

Each buffer is one function's reconstructed stack frame; each alias is a real
local. Replace the alias with a declaration and delete the buffer.

**The one hazard, and it is real:** the aliases are the *only* thing keeping
those locals adjacent. If a body writes past the end of one alias into the next
— and this code does index past array bounds in places — splitting them changes
behaviour. That is exactly what the gate catches, per function.

**Method:** one function at a time, build + gate after each, exactly as
`compact_locals.py` was applied. Where a frame cannot be split, leave it and
record why in a comment.
**Size:** 34 functions. Expect 3–6 to resist.
**Risk:** medium per function, contained by doing them one at a time.

### Phase 3 — Un-pin the globals from the blob

**Goal.** Delete `blob.inc` and `BMF_BLOB_BASE`. Each global becomes a real
definition:

```c
static int32_t  plane_count;
static uint8_t  ctx_group_flags[15] = { 0x01, 0x03, ... };
static PlaneDesc planes[MAX_PLANES];
```

**Order inside the phase — this is the part to get right:**

1. **Extents.** For every address, find what the code actually touches: the
   largest offset reached from it and the element size used. The `[0x10000]`
   bounds IDA emitted are fiction. Static analysis gets most of it; a debug
   build with an access log over `blob1` gets the rest, and the 64.5 %→90 %
   coverage from Phase 0 is what makes that log trustworthy.
2. **Merge false neighbours.** Any set of addresses the code walks as one
   region (§4.1) becomes one object. Expect this to *reduce* the count well
   below 293 — the four `0x44337C`–`0x443394` bases collapse into one
   `planes[]`, and the 58 overlaps are where to start looking.
3. **Initialisers.** `blob.inc` holds `.rdata` and `.data`; a global that starts
   non-zero needs its bytes lifted out of the array into a real initialiser.
   Emit them from `mkdata.py`, do not hand-copy.
4. **The 39 relocations.** `blob1_relocs[]` lists slots holding absolute
   pointers into the blob. Once objects are real, these become `&object` —
   ordinary initialised pointers, no `bmf_blob_relocate()`, and no 4-byte
   assumption. This is the first place Phase 3 and Phase 4 touch.
5. **Delete the blob.**

**A decision this forces.** `bmf.cpp` still documents a "hybrid" build where
`blob1` *is* the loaded PE's data segment, shared with code still running in the
original image. De-blobbing ends that permanently. If the hybrid build is still
wanted as a cross-check, take the measurements it can give *before* this phase
— afterwards it is not recoverable without reverting.

**Gate:** digests unmoved — and this is the phase where that is most valuable,
because the failure mode is silent corruption of adjacent state rather than a
crash.
**Size:** large. The tooling (extent analysis, emitter) is most of it; the
mechanical rewrite is generated.
**Risk:** high. Do it in address order, in batches, with a gate per batch.

### Phase 4 — Objects and pointers, together

**This is the x86-64 work.** Per §4.2, take one object at a time; for each:

1. Declare the `struct` from the offset family.
2. Retype every variable that points at it — parameters, locals, and the
   globals that hold it — from `int32_t` to `T *`.
3. Rewrite `*(uint32_t *)(p + 278736)` as `p->field`.
4. Delete the `__fwd_*` shim for every call whose argument types now agree. All
   114 exist only to launder `void *` through a wrong signature; they should
   nearly all disappear here.

**Order**, and the reasoning is in §4.2's table:

1. **BMP header** — smallest, self-contained, and it is a layout you can look
   up rather than infer. Do it first to establish the idioms on a case where
   you cannot be wrong about the answer.
2. **Plane descriptors** — 16 bytes, documented in `ALGORITHM.md` §6.2, and
   fixing them is what retires the four false bases from §4.1.
3. **Model block** — `_this`, `a1`, `v56`, `n0x10_2` and the header's 52 fields,
   all in one change. Large, but every one of its users is already known.
4. **`lpAddress` and `n5_2`** — last, and only after `ALGORITHM.md` §9's open
   questions about the alternate model families are closed. Recovering a struct
   you cannot name the fields of produces `field_0x11021`, which is not
   progress.

**Progress metric, and it is a good one:**

```
g++ -m64 ... -c bmf.cpp 2>&1 | grep -c 'cast to pointer from integer'
```

4371 today. It should fall monotonically, per object, to 0. When it reaches 0
the `-m64` build should be tried on every gate image — expect it to work, and
expect the first attempt not to.

**Gate:** the `-m32` digests must stay unmoved throughout — the 32-bit build is
the reference until the 64-bit one round-trips, and only then does `-m64` join
the matrix.
**Size:** the largest phase. The model block alone is ~650 references.
**Risk:** high, but the compiler is a strong assistant here: every site you miss
is a warning.

### Phase 5 — Casts and the Hex-Rays vocabulary

Most of the 7242 casts are consequences of Phases 3 and 4 and will already be
gone; this phase is what is left.

- `LOBYTE(x) = v` → a named `uint8_t` field or a shift/mask with a comment.
  298 uses. Many are field access into a struct Phase 4 recovered — do those
  there, and leave only genuine bit-packing here.
- `*(uint64_t *)p = 0x0606060606060606LL` → `memset(p, 0x06, 8)` where it is a
  fill — Phase 4's structs are what make those visible.
- The `M128I`/`M128F` wrapper unions in `bmf.cpp` exist so Hex-Rays' `.m128i_i32[]`
  member syntax compiles. Once the SIMD code is readable, switch to intrinsics
  and delete them.

**Risk:** low. Almost all of it is local and gate-checked.

### Phase 6 — Control flow

174 `goto`s into 127 labels. Hex-Rays emits these where it could not recover a
loop or an early exit; most are one of four shapes (loop `continue`, loop
`break`, early `return`, shared error tail) and rewrite mechanically.

Deliberately last: a `goto` is ugly but honest, and rewriting control flow before
the types are right is how you introduce a bug the gate cannot distinguish from
a layout change.

### Phase 7 — The dark 28 %: decide

By now the fast back end is either covered (Phase 0 succeeded in reaching it) or
still dark. Two honest options, and the choice should be explicit:

- **Restore it.** Un-pin the mode for real, make `bmf c -f` work, refactor it
  under the widened gate. Right if BMF's fast mode is wanted.
- **Delete it.** 7198 lines, and `prune_unreachable.py` already exists to do it
  once the pin becomes a compile-time constant. Right if this is to be a
  reference implementation of `-S -Q9` and nothing else.

What is not acceptable is refactoring it blind and claiming it works.

---

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
- **Inherited names lie.** `__n8`, `__n256`, `__n2`, `__n0x800000` are the last
  value assigned, not the meaning. `__Buffer` is not a buffer. Rename from the
  code, not from the name.
- **Coverage is not correctness.** 90 % line coverage of a codec still misses
  the branch that only a 5-plane 16-bit image takes. Add images when a phase
  touches something the corpus does not reach.
- **Do not trust a green gate on a change you did not understand.** Ten images
  is a signal, not a proof — and it is a thinner one than it sounds, because
  they are ten *similar* images. Arithmetic that runs identically on all ten can
  still differ on the eleventh. This is the single strongest argument for doing
  Phase 0 properly rather than quickly.

---

## 7. Sequencing summary

| phase | changes | gate risk | blocked by |
| --- | --- | --- | --- |
| 0 widen the gate | `test.sh`, 2 pins | low | — |
| 1 known names | ~45 identifiers | very low | — |
| 2 frames | 34 functions, 935 aliases | medium | 0 |
| 3 de-blob | 293 → fewer objects | **high** | 0, 2 |
| 4 objects + pointers | 3965 lines, 114 shims | **high** | 3 |
| 5 casts | ~7000 → few hundred | low | 4 |
| 6 control flow | 174 gotos | medium | 4 |
| 7 dark 28 % | 7198 lines, 22 bodies | — | 0 |

Phases 1 and 2 can run in parallel with 0. Nothing else can be reordered: 3
needs the coverage, 4 needs real objects to point at, 5 and 6 need the types.

---

## Appendix A — how the numbers were measured

```sh
# bodies, sizes, globals, casts, gotos, frames
wc -l subs1.hpp
grep -c 'blob1 + 0x' subs1.hpp                       # 293 globals
grep -c 'static inline .*__fwd_' subs1.hpp           # 114 shims
grep -oE '\((const )?[A-Za-z_][A-Za-z0-9_]* *\*+\)' subs1.hpp | wc -l
grep -c 'goto ' subs1.hpp ; grep -c '__hexrays_frame + ' subs1.hpp

# x86-64 work list
g++ -m64 -march=x86-64 -msse2 -std=c++17 -fno-strict-aliasing -fpermissive \
    -fno-rtti -fno-exceptions -O0 -DNDEBUG -c bmf.cpp -o /dev/null 2>x64.log
grep -c 'cast to pointer from integer' x64.log       # 4371
grep -E 'warning: (cast|invalid conversion)' x64.log | \
    grep -oE '^[^:]+:[0-9]+' | sort -u | wc -l       # 3965 lines

# coverage
g++ -m32 ... -O0 --coverage bmf.cpp -o bmfcov
for f in testfiles/*.bmp; do ./bmfcov c $f o.bmf; ./bmfcov d o.bmf o.bmp; done
gcov -f -n -o . bmfcov-bmf.gcno                      # 64.51 % of 18711 lines
```

Overlap and offset-family analysis: `tools/` has no script for these yet;
writing them is the first task of Phase 3 and Phase 4 respectively.
