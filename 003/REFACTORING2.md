# Refactoring BMF 2.01, round two

`REFACTORING.md` is the record of the first round and its numbers are current.
This is the plan for what is left, and it has three named goals:

1. **`blob.inc` disappears.** The data segment of a Windows executable should
   not be a build input.
2. **The SIMD intrinsics become C**, and the compiler vectorises it.
3. **The code stops being ugly.** 5603 pointer casts and 330 conversions the
   build has to pass `-fpermissive` to accept are not a style complaint.

They are listed in that order because that is their order of importance, and
attacked in the reverse of it, because goal 2 removes a third of goal 1's work
and a chunk of goal 3's.

The gate does not change: **byte-identical compressed streams against the
committed references, and every image round-trips.** Everything below is
answerable to it.

---

## 1. Where this is now

| | |
| --- | --- |
| `subs1.hpp` | 23 807 lines |
| `blob.inc` | 343 794 bytes, 65 892 initialisers |
| globals still inside `blob1` | 77 declared, **43 referenced** |
| arrays already moved out of it | 86, 50 832 bytes |
| SIMD intrinsic calls | **558**, 33 distinct, in 16 bodies |
| — of those, float arithmetic | 402 (`mul_ps` 189, `add_ps` 157, `sub_ps` 28, `div_ps` 28) |
| pointer casts | 5603 |
| raw-offset dereferences | 238 |
| `-fpermissive` conversions | ~330 |
| `goto` / `LABEL_n:` | 113 / 81 |
| distinct `vNN` locals | 560 |
| recovered structs still `ObjN` with `fNN` members | 89 |
| loops the compiler vectorises at `-O2` | **0** |
| loops the compiler vectorises at `-O3` | **141** |

Two of those lines are the whole argument for goal 2. The compiler will
vectorise a hundred and forty-one loops in this file the moment it is given
loops instead of intrinsics — and it vectorises nothing today, because `-O2`
does not and because there is nothing to vectorise.

---

## 2. Phase A — the intrinsics become C

### 2.1 What is actually there

558 calls, but they are not spread evenly and they are not all the same problem:

| body | intrinsics | lines |
| --- | --- | --- |
| `alt_p2_model` | 225 | 1971 |
| `alt_p2_filter` | 148 | 177 |
| `alt_p2_context` | 89 | 1019 |
| `choose_plane_coding` | 29 | 867 |
| `estimate_cost` | 12 | 69 |
| eleven others | 55 | |

**Three bodies hold 82 % of them, and all three are the p2 alternate model.**
That matters for sequencing: the p2 family is also the least understood part of
the file (`algorithm_v2.md` §9), so this phase and that reading help each other
— translating the arithmetic is how you find out what it computes.

By kind:

* **402 float arithmetic** — `mul_ps`, `add_ps`, `sub_ps`, `div_ps`. These are
  straight-line vector expressions and they are the whole of the win.
* **34 `_mm_prefetch`** — no C equivalent; `__builtin_prefetch`, or drop them
  and measure.
* **~90 shuffles, packs and horizontal reductions** — `movehl_ps`,
  `shuffle_ps`, `unpacklo_*`, `srli_si128`, `cvtsi128_si32`. These are the
  awkward ones, §2.3.
* **~30 integer vector ops** — `add_epi32`, `slli_epi16`, `add_epi8`.

### 2.2 The shape they translate into

`alt_p2_filter` is the model for the whole phase. It currently reads:

```c
v5 = _mm_add_ps(
       _mm_add_ps(
         _mm_add_ps(
           _mm_add_ps(
             _mm_add_ps(
               _mm_add_ps(_mm_mul_ps(__xmmword_441120, a2[0]),
                          _mm_mul_ps(__xmmword_441130, a2[1])),
               _mm_mul_ps(__xmmword_441140, a2[2])),
             _mm_mul_ps(__xmmword_441150, a2[3])),
           _mm_mul_ps(__xmmword_441160, a2[4])),
         _mm_mul_ps(__xmmword_441170, a2[5])),
       _mm_mul_ps(__xmmword_441180, a2[6]));
v6 = _mm_add_ps(v5, _mm_movehl_ps(v5, v5));
v6.m128_f32[0] = v6.m128_f32[0] + M128F(_mm_shuffle_ps(v6, v6, 1)).m128_f32[0];
```

That is a **dot product of seven four-wide vectors against seven constant
vectors, then a horizontal sum**. Written as C it is two loops and a reduction,
the coefficients are a `float[7][4]` instead of seven blob constants, and the
compiler emits the same instructions — plus it can now see what it is.

### 2.3 The one thing that can go wrong, and why the gate catches it

**Float addition is not associative.** The horizontal sum above adds lanes in a
specific order: `(0+2, 1+3)` from `movehl`, then `+shuffle(...,1)`. A C
reduction written as `for (i = 0; i < 4; i++) s += v[i];` sums `0+1+2+3` and is
**a different number**, in the last bits, on some inputs.

This project is unusually well placed to catch that. The floats feed
`estimate_cost` and the p2 model, both of which decide what gets coded, so a
last-bit difference changes a decision and the stream stops matching its
reference. **A rounding difference here is a hard gate failure, not a silent
drift.** That is the opposite of the usual situation with float refactoring and
it should be exploited: translate aggressively, let the gate arbitrate.

Two rules follow:

* **Never `-ffast-math`, `-funsafe-math-optimizations` or `-Ofast`.** They
  license exactly the reassociation the gate would then catch as a regression,
  and turning them on would mean regenerating the references, which means
  giving up the property that makes this whole project checkable.
* **Preserve the reduction order explicitly** where one exists. A horizontal
  sum written as `(v[0]+v[2]) + (v[1]+v[3])` compiles to the same tree the
  intrinsics build and stays bit-exact.

### 2.4 The `M128*` unions go too

`bmf.cpp` carries `M128I`, `M128F`, `M128D` and `M64` — unions that exist only
because MSVC's `__m128` has named members (`.m128_f32[0]`) and GCC's does not.
There are 457 uses of `__m128` in the file and 203 of `.m128_i32` alone.

They are a translation artefact of a translation artefact: MSVC's spelling of a
type that only exists because of the intrinsics. When the intrinsics go, so do
these, and `bmf.cpp` loses its largest remaining block of scaffolding.

### 2.5 Making sure it actually vectorised

The point of the exercise is the compiler doing the work, so check that it did:

```
BMF_OUT=/dev/null ./build.sh -fopt-info-vec 2>&1 | grep -c vectorized
```

Baseline today is 0 at `-O2` and 141 at `-O3`. Two things follow:

* `-O2` does not auto-vectorise in GCC 13. The build's `-O2` comes from
  `g.bat` and is worth keeping for fidelity, so the phase ends by adding
  **`-ftree-vectorize`** to `build.sh` rather than moving to `-O3`.
* **`-O3` already passes the gate** — checked: byte-identical streams, 942 532
  bytes against `-O2`'s 913 860. So the optimiser is not reassociating floats
  today and there is no reason to expect it to start. That is the evidence for
  §2.3's claim rather than a hope.

And measure the result. The corpus runs in 19 s; if translating 400 intrinsics
into C makes that materially worse, the translation is wrong somewhere and the
`-fopt-info-vec` count will say where.

### 2.6 What this hands to Phase B

74 distinct `__xmmword_*` constants feed these bodies — **24 still reached
through `blob1`, 50 already sitting in `bmf_*` byte arrays**, which is to say
they are spread across both halves of Phase B's list (§3.1 and §3.6).

Translating their users turns each into an ordinary `static const float[4]` or
`int32_t[4]` with a real initialiser. A vector constant is the easiest kind of
global to move: its extent is 16 bytes by construction, and its type is whatever
the arithmetic around it says — which is the one thing §3.3 says cannot usually
be guessed, and here it can. **Phase A takes 74 entries off Phase B's list as a
side effect**, 24 of them from the harder half. That is why it goes first.

---

## 3. Phase B — `blob.inc` disappears

### 3.1 What is left

343 794 bytes of generated C, 65 892 byte initialisers, holding whatever the
first round did not move. Against it: **77 globals declared as references into
`blob1`, of which only 43 are referenced anywhere.**

The machinery around it is `bmf_addr(va)`, `bmf_blob_relocate()` and
`bmf_data_relocate()` — an address-translation layer that exists so a global
can be reached by the virtual address it had in `BMF.exe`.

### 3.2 The 34 dead ones go first

Thirty-four declarations name a global nothing uses. They cost nothing to
delete and they shrink the problem by 44 % before any thinking is required.
The check is the same shape as `deadcheck.py`'s: a declaration whose name
appears twice in the file — its `typedef` and itself — is unreferenced.

### 3.3 The 43 live ones, one at a time

Each needs three things, and the third is the work:

1. **Its extent.** How many bytes belong to it — the address of the next global
   is an upper bound, and `tools/extents.py` already computes these.
2. **Its type.** What the code reads out of it. `plane_count` is an `int32_t`;
   `exclusion_mask` is a byte array indexed by symbol; `__byte_44339E` is
   16 bytes per plane. Most of these are already typed at their use sites.
3. **Its initial value.** This is the one that cannot be guessed: a global BMF
   initialised in its data segment has to keep that value, and the only source
   is `blob.inc` itself. So the move is: read the bytes out of the blob at that
   address, emit them as a C initialiser next to the declaration, delete the
   blob reference.

That last step is mechanical enough to tool — `tools/deblob.py` exists and did
the first 86 — but each object needs its type decided first, and a wrong type
silently changes the initialiser's meaning. Hence one at a time, gated.

### 3.4 Ordering: by traffic, and the top of the list is easy

| global | uses |
| --- | --- |
| `plane_count` | 153 |
| `__byte_44339E` | 46 |
| `__n3_0` | 28 |
| `__byte_44339D` | 25 |
| `__byte_4433AD` | 20 |
| `__n4_3` | 20 |

`plane_count` alone is 153 uses of an `int32_t` whose initial value is
irrelevant — `compress_image` computes it from the depth before anything reads
it. Several of the others are the same: **a global whose first access is a
write does not need its initialiser recovered at all**, and finding those is
worth doing before the extraction work starts.

### 3.5 When the last one goes

`bmf_addr`, `bmf_blob_relocate`, `bmf_data_relocate`, `bmf_reloc_slots`,
`BMF_BLOB_BASE` and `blob.inc` all go together, and `build.sh` stops having a
generated file as an input. The addresses do not disappear from the record:
`tools/addrmap.txt` maps every body to its address in `BMF.exe`, and the
equivalent for data should be written down before the blob is deleted, not
after.

### 3.6 The 86 that already moved are not done

They live in `subs1.hpp` as `alignas(16) static uint8_t bmf_NAME[n + 64]` with
byte initialisers — out of the blob, but still untyped byte arrays with a
64-byte tail nobody has explained. 50 832 bytes of them, the largest being
`bmf_dword_439BD8` at 30 024. Giving those real types and real extents is the
same job as §3.3 and belongs in the same phase.

One of them should simply go: `bmf_pout_of_memory_handler`, 10 292 bytes,
exists because `set_new_handler` writes a function pointer into MSVC's CRT
new-handler slot. `bmf_new` reads that one word (`REFACTORING.md` §6). The
other 10 288 bytes are CRT state nothing touches.

---

## 4. Phase C — the ugliness that is left

### 4.1 The casts, and what they are for

5603 pointer casts. They are not all noise — `structs.py` writes
`*(uint8_t *)&p->f8` to reach a byte inside a word, and that cast carries
information. But three groups do not:

* **Casts around `blob1` references.** Every one of the 77 globals is reached
  through a cast that Phase B deletes outright.
* **Casts on the `M128*` unions.** `(__m128)__xmmword_441120` and the `M128F(…)`
  wrappers — Phase A's.
* **Casts between an integer and a pointer.** The `-fpermissive` conversions,
  §4.2, which are the ones that indicate a wrong type rather than a narrow
  access.

Measure after A and B before deciding what to do about the rest. Two of the
three groups are already someone else's phase.

### 4.2 `-fpermissive` is the honest metric

The build passes `-fpermissive` and gets ~330 warnings:

```
150  invalid conversion from int32_t to <pointer>
 90  invalid conversion from <pointer> to int32_t
 32  invalid conversion (other)
 25  invalid conversion, both aka
 15  cast loses precision
 12  cast to pointer from integer of different size
```

Every one of those is a place where the decompilation put an integer where a
pointer belongs or the reverse. **The goal for this phase is stated exactly:
the file compiles without `-fpermissive`.** That is a binary condition, the
compiler adjudicates it, and it cannot be argued with — which is the same
property that made `-Wunused-variable` and `-Wuseless-cast` the right drivers
in round one.

It is also the right metric because it is not satisfiable by cosmetics. Each
conversion is a type that is wrong, and fixing it means deciding what the thing
actually is.

The 12 `cast to pointer from integer of different size` are worth pulling out
first because they are the narrowest and most mechanical group, not because they
are the 64-bit blocker. They are not: `REFACTORING.md` §Phase 4 establishes that
the 64-bit gap is the program's **32-bit pointer fields**, which cannot be
widened because every object is walked with variable offsets as well as constant
ones, and which the low arena in `bmf.cpp` addresses instead. Fixing these 12
makes the file cleaner and does not move that.

### 4.3 The `vNN` names

560 distinct. `rename.py --in FUNC` makes them renameable one body at a time,
and round one showed the cost: about ten names per session of reading, each
argued from what the code does with it. At that rate this is not a phase, it is
a background task, and it should be treated as one — a body at a time, when
that body is being read for another reason.

The exception is the `ObjN` structs' `fNN` members, 89 structs' worth. Those
are not read-one-at-a-time work: `algorithm_v2.md` establishes what several of
the objects *are*, and naming a struct's fields is worth doing in one pass per
struct when its writer is understood. `BmfImage` and `BmpHeader` are the
precedent.

### 4.4 `goto` and `LABEL_n:`

113 and 81. Round one's Phase 6 concluded these are irreducible — 21 backward
jumps that are loops Hex-Rays could not name, and 100 forward ones that are
either not the whole of an `if` or jump into a region with other entries.

That conclusion should be re-examined once, and only once, and for a specific
reason: **it was reached before the frames became structs and before the
alt-model bodies were merged.** Both changed the control flow's shape. If
`degoto.py` still finds two candidates out of 123, the conclusion stands and
this line can be deleted from the plan.

---

## 5. Order, and what depends on what

```
  Phase A  intrinsics → C          removes 21 blob constants, the M128 unions,
     │                             and ~200 casts before B and C start
     ▼
  Phase B  blob.inc → typed C      removes the bmf_addr layer and ~77 globals'
     │                             worth of casts
     ▼
  Phase C  -fpermissive → clean    what is left is the real type errors
```

A before B because A deletes blob entries. B before C because C's metric counts
conversions that B removes. C last because it is the only one whose finish line
is a compiler flag rather than a count, and it should be measured against a file
the other two have already cleaned.

Within each phase: **one object, one body, or one intrinsic family at a time,
gated.** Round one's evidence for that is `tools/struct-sweep.sh` — 130 rounds,
of which a dozen failed the gate and were reverted, and one of the failures was
caught only by an image the sweep's filter had been skipping.

---

## 6. What the gate needs

It is in good shape — 15 images with committed reference streams, a two-member
archive, 15 refused inputs with pinned exit statuses, and an out-of-memory
ladder. Three additions this round needs:

* **Nothing, for Phase A — checked, and this is the good news.** The obvious
  worry was round one's: 268 lines of `alt_model_p1_decode` were broken for a
  fortnight because no image ran them. So the question was asked before planning
  rather than after, and the answer is that the three bodies holding 82 % of the
  intrinsics are **fully covered**:

  | | lines run |
  | --- | --- |
  | `alt_p2_model` | 1591 of 1591 — 100 % |
  | `alt_p2_context` | 817 of 817 — 100 % |
  | `alt_p2_filter` | 129 of 129 — 100 % |

  Every float operation Phase A touches in those bodies executes during a gate
  run, and a rounding difference in any of them moves a stream. Phase A is as
  well gated as anything in this project.

  The exception is `estimate_cost` at **64.6 %**, which is the one SIMD body
  with unreached lines — its scalar tail, for histograms with an odd bin count.
  Translating it needs a histogram of odd length to reach that path, and there
  is no evidence one occurs; check before touching it.
* **A `-fopt-info-vec` count in the plan's numbers**, so "the compiler
  vectorises it" is a measurement and not an intention.
* **A no-`-fpermissive` build target**, failing at first, as Phase C's
  scoreboard: `BMF_STRICT=1 ./build.sh` counting the errors it gets.

---

## 7. What would make this fail

* **Translating floats by pattern instead of by reading.** A horizontal sum has
  an order and a reduction loop has a different one. The gate catches it, but a
  tool that rewrites 400 sites and fails the gate tells you nothing about which
  of the 400 was wrong. Small batches.
* **Deciding a blob global's type from its name.** `__n4_3`, `__n256`, `__n15`
  are named for the last constant compared against them (`REFACTORING.md` §6),
  and a wrong type silently changes the initialiser bytes' meaning. The type
  comes from the use sites, and the initialiser comes from the blob, and both
  have to agree.
* **Chasing the cast count.** 5603 is not a target. Two thirds of them belong
  to other phases and the rest carry information. `-fpermissive` is the metric;
  the cast count is a side effect.
* **Believing a tool that reports success.** Round one had `unused.py` report
  "no warnings" while 66 sat in the file, for two independent reasons at once.
  Every count in this document should be reproducible by a command that does not
  go through the tool being checked.
