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
| `subs1.hpp` | 22 113 lines | 25 462 |
| bodies | 179 (84 real, 95 `__fwd_*` shims) | 215 |
| globals in `blob.inc` | **78** | 293 |
| `goto` | 121 | 174 |
| pointer casts | 5680 | 7336 |
| `goto` / `LABEL_n:` | 121 / 88 | 174 / 127 |
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
2. ~~No integer holds a pointer.~~ **Withdrawn, on evidence — see Phase 4.**
   Every one of this program's 159 objects is walked with variable offsets as
   well as constant ones, so widening a field moves the arrays indexed after it.
   What replaces it: *no integer holds a pointer where the object permits it*
   (done — every local, parameter and global that could be typed, is), and a
   64-bit build gets its addresses into 32 bits by confining the heap rather
   than by widening the fields.
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
0  the gate, and one mode      done   §2
1  names you already know      done   39 identifiers
2  frames -> real locals       done   0 frames left
3  un-pin the globals          done   86 of 163 out; the other 77 cannot move
4  objects + pointers          done   every local typed; fields cannot be
5  casts and the vocabulary    done   2 of 3 items withdrawn on evidence
6  control flow                done   2 of 123; the rest is irreducible
```

"Done" here means finished, not finished-as-written. Phases 3, 4, 5 and 6 each
met evidence that contradicted the plan, and each entry below says what the
evidence was and what was done instead. §7 collects the four corrections.

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

### Phase 3 — 86 globals out, 78 left, and a map of why

Done: the four false bases from §4.1 retired, so no global's position matters to
another *there*; five globals that held addresses in `int32_t` words are real
pointers outside the blob (`coded_buf`, `out_cursor`, `packer_word`,
`hist_scratch`, `model_tables`); the error message table decoded and inlined,
which is what `__exit_402E40` used nine relocated pointers for.

**The all-at-once split does not work, and that is now measured rather than
assumed.** `tools/deblob.py` gives every global its own definition with a guard
gap; it segfaults on the first image. Three variants say what kind of failure:

| guard | contents | result |
| --- | --- | --- |
| 64 bytes | zero | SIGSEGV in `sub_416860` |
| 64 bytes | the bytes that followed in the data segment | SIGSEGV, same place |
| 4096 bytes | the bytes that followed in the data segment | SIGSEGV, same place |

A read running off a global's end would be fixed by the second and certainly by
the third. Neither is, which leaves **writes** crossing the boundaries: some of
what Hex-Rays presents as globals is a smaller number of larger tables split
into one name per access site, and a store through one name has to be visible
through another.

So the tool was run one global at a time instead — split it, build, run the ten
images, keep the answer — and the result is a map rather than a guess:

**86 of the 163 are independent and are now definitions of their own.** 77 are
not. `tools/blob-independence.txt` is the per-global answer, and it is the
worklist for the objects Phase 4 has to recover: a global marked SHARED is one
that something writes across.

The 39 absolute pointers `blob1_relocs` holds all turned out to live in globals
that moved, so `blob.inc`'s own rebasing no longer reaches them; the generated
block rebases them itself, resolving a target to a moved definition where there
is one and back into `blob1` where there is not. They pass through unread on
this corpus — the string tables belong to modes that are gone — but leaving raw
addresses in data nothing relocates is the kind of thing that works until it
does not.

`blob.inc` is down to 78 globals from 293 at the start. It cannot go to zero
until the 77 shared ones are understood as the tables they are.

### Phase 4 — done as far as this program allows; the goal was wrong

3784 → 1211 int↔pointer warnings. `tools/retype.py` converted every local and
parameter used as a pointer base: 189 candidates, `char *` where the variable is
only ever an address and `uintptr_t` where the code also masks or tags it. There
are no candidates of that kind left.

What remains is one shape, and it is the shape §4.2 warned about:

```c
*(uint8_t *)(*(uint32_t *)(_this + 76) + 6)
```

The local is typed; the **field** at `+76` is not.

**The offsets are not the obstacle.** `tools/objects.py` does the alias analysis
— two names are the same allocation if one is assigned from the other or passed
where the other is the parameter, closed transitively through the `__fwd_*`
shims — and the field maps come out clean: 159 objects, the largest with 174
dereferences over 45 offsets, nearly all `uint32_t` at 4-byte-aligned offsets,
and 2 to 7 offsets per object where a byte and a word overlap (`LOBYTE`-style,
not a contradiction).

**The obstacle is that widening a field moves everything after it**, so an
object has to convert in every function at once, and its allocation size has to
move with it. `this_4` is the example: the cleanest map in the file, 34 constant
offsets, no variable ones, used in exactly one function — and still not
convertible alone, because two lines above its first use is
`this_4 = (int32_t)this_1`, and `this_1` is the same allocation under another
name somewhere else.

#### Why it cannot be finished, measured

Widening a field moves every field after it. That is survivable if an object is
only ever addressed at constant offsets — the struct absorbs the move. It is not
survivable if the object is also walked with a variable offset, because the walk
indexes the arrays the move displaced.

Across all 159 objects: **1683 constant-offset dereferences and 2184
variable-offset ones, and not one object has only the first kind.** There is no
object in this program whose fields can be widened.

`BmfArc` is the exception that shows the rule. The eight bytes `bmf c` and
`bmf d` allocate for the open stream and an image count — `malloc(8)` addressed
by offset, with a `FILE *` stuffed into four bytes — is a struct now, with a
`static_assert` that it is still eight bytes at 32 bits. It converted because it
is the one object in the file with no variable-offset access, and it was the
first thing a 64-bit build faulted on.

#### What a 64-bit build needs instead

Not wider fields: a heap the narrow fields can address. `bmf.cpp` has that now —
a low arena under `malloc`, compiled for 64-bit targets only, so the 32-bit
build the streams are verified against allocates exactly as it did. With it and
`BmfArc`, the 64-bit binary compiles, runs, opens its files, and reaches
`model_plane` before it meets the model block: fields at `+80`–`+92` holding
pointers, and the same object read at `+1078692`.

Getting past that is the same loop — find the object the fault is in, and either
convert it (if it has no variable-offset access) or teach the arena to cover
what it points at. It is not blocked on understanding the algorithm. It is
blocked on nothing except doing it, one fault at a time, and it will not produce
typed fields at the end.

### Phase 5 — done, and two thirds of it should not have been attempted

The 38 casts GCC calls useless went with the retyping. The 8-byte repeated-byte
stores into the model geometry table were runs of consecutive `uint64_t` writes
of one value; 12 of them are 4 `memset`s now, which says what they are.

The other two items are deliberately not done, and the reasons are worth keeping
because both look like work until you try them:

* **`LOBYTE`/`HIWORD` → shift and mask.** The plan proposed this for the uses
  that turn out to be field access into a struct Phase 4 recovered. Phase 4
  recovers no structs, so every one of the 265 uses is genuine bit-packing, and
  `LOBYTE(x) = v` says more than `x = (x & ~0xFFu) | (v & 0xFF)`. Rewriting them
  would trade a well-defined macro for arithmetic that carries less meaning.
* **The `M128I`/`M128F` unions → intrinsics.** Those unions *are* how MSVC's
  member syntax is spelled portably; GCC's `__m128` has no named members. There
  is no lvalue intrinsic for `x.m128_f32[2] = v`. The plan conditioned this on
  the SIMD code being readable first, and it is not.

### Phase 6 — done to the extent the code allows: 2 of 123

`tools/degoto.py` rewrites the one shape with a structured equivalent — a
forward `goto` that is the whole of an `if`, over a region nothing else enters,
becomes the inverted `if` with the region inside it. It checks the conditions
rather than assuming them: the label reached by no other `goto`, no label inside
the region reached from outside, the region brace-balancing so it can become a
block.

Two qualify. The other 121 are 21 backward jumps — loops Hex-Rays could not name
— and 100 forward ones that are either not the whole of an `if` or jump over a
region with its own entries. The earlier measurement stands: **no label in this
file is followed by a `return`, a `break` or a `continue`**, none is unreachable,
and none has its `goto` on the line above. There is no fifth shape waiting to be
found; this is irreducible flow, and rewriting it blind would introduce bugs the
gate cannot tell from a layout change.

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

## 7. Where the plan was wrong

Every phase is finished. Four of them are not finished the way this document
first described, because the code said otherwise, and that is the more useful
half of the record.

| phase | the plan said | the code said |
| --- | --- | --- |
| 2 | 3–6 frames will resist splitting | 16 did — they carry bytes no alias names and the code runs into them |
| 3 | split every global; extents are the unknown | splitting all at once segfaults on *writes* crossing boundaries. One at a time: 86 move, 77 are parts of larger tables |
| 4 | recover the objects and widen the pointer fields | 1683 constant-offset dereferences against 2184 variable-offset ones, and **no object has only the first kind**. Widening is not available; `BmfArc` is the one exception and it converted |
| 6 | the `goto`s are four rewritable shapes | none of the 123 is any of them. One other shape exists; it fits 2 |

Two of those were caught by measuring before starting, which cost nothing. One
was caught by the gate, which is what it is for. One — Phase 3's — was caught by
running the migration and having it crash, and that turned out to be the
cheapest way to get the map.

### What is genuinely left

Not a phase: a decision, and it is small.

The 64-bit build compiles and runs and gets as far as `model_plane`. Taking it
further is the loop in §Phase 4 — fault, find the object, convert it or extend
the arena — repeated until the images round-trip. It is mechanical and it is not
blocked on anything, but it ends with a working 64-bit binary whose fields are
still `uint32_t`, because this program's objects will not let them be anything
else.

Whether that is worth having is the only open question in this document.

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
