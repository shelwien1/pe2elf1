# Refactoring BMF 2.01, round two

`REFACTORING.md` is the record of the first round. This was the plan for round
two, and it is now also its record: **all three goals are done.** §0 is what
happened; the rest is the plan as it was written, corrected where the work
proved it wrong.

The three goals were:

1. **`blob.inc` disappears.** The data segment of a Windows executable should
   not be a build input.
2. **The SIMD intrinsics become C**, and the compiler vectorises it.
3. **The code stops being ugly.** 5603 pointer casts and 347 conversions the
   build has to pass `-fpermissive` to accept are not a style complaint.

The gate did not change: **byte-identical compressed streams against the
committed references, and every image round-trips.** Every step below was
answerable to it, and every step passed it.

Every number is reproducible; Appendix A gives the command for each. Round
one's lesson about tools that report success (§7) applies to this document too,
and its first draft failed that test in three places — see Appendix B.

---

## 0. What happened

| | before | after |
| --- | --- | --- |
| `blob.inc` | 343 794 bytes, 65 892 initialisers | **gone** |
| globals reached through `blob1` | 78 | **0** |
| data arrays | 86, 50 832 bytes | 21, 19 924 bytes (19 584 of it zero bss) |
| initialised data bytes | 50 832 + 65 892 | **340** |
| SIMD intrinsic calls | 558 | **0** |
| `M128*` wrapper unions | 4 | 1, and no vector member |
| conversions needing `-fpermissive` | 347 | **0** |
| `subs1.hpp` | 23 807 lines | 19 711 |
| `bmf.cpp` | 890 lines | 782 |
| loops vectorised at `-O2` | 8 | 19 |

**Goal 2 first, because §2.3's warning turned out to be the whole of it.**
Reordering one horizontal sum in `alt_p2_filter` moves three of the fifteen
streams; `-ffast-math` moves the same three. So the translation was done body
at a time with the reduction order preserved exactly, and `bmf_hsum4` is where
that order lives. Twelve of the fifteen images notice nothing, which is why
the whole corpus has to run every time.

What the translation found is worth more than the line count. The p2 family is
**normalised LMS**: predict from seven weight rows against seven inputs, take
the error, step each weight by `rate * error * input / (running mean square +
floor)`. `alt_p2_context` does it once, `alt_p2_model` twice side by side and
then a third time against its own prediction. Three constant tables came out of
it — `bmf_p2_coef[7][4]`, `bmf_p2_rate[7][4]`, `bmf_p2_mix[4][6]` — and none of
the first two is constant: both p2 model bodies save them on entry, fold or
flatten rows, and restore on exit.

**Goal 1 turned out to be two measurements, not 43 recoveries.** §3.3 assumed
each global needed its extent, its type and its initial value recovered, with
the initial value "the one that cannot be guessed". It can:

  * *Everything above 0x44294C is zeroes.* One unbroken run, and all 41
    surviving globals inside it. There were never any initialisers: it is bss.
  * *Everything below is dead.* Poisoning 43 184 bytes of `blob.inc` with 0xCC
    leaves all fifteen streams byte-identical.

So the survivors became references into `bmf_bss`, 19 584 zero bytes at their
original relative offsets, and `blob.inc` went — with the whole relocation
layer, which the same experiment showed does nothing. Poisoning then trimmed
four arrays that had been sized by "distance to the next global": 30 024 bytes
to 32, 10 292 to 4, 6 592 to nothing at all, 1 968 to 4.

It also found a real defect. The 64-byte guard tails were load-bearing:
nineteen small arrays had been split out of the per-plane record table and
were stale copies of it, and the code reads across them. Folding them back
into `bmf_bss` put the table together and let the tails go.

**Goal 3 is done: the file compiles without `-fpermissive`.**
`BMF_STRICT=1 ./build.sh` drops `-fpermissive` and counts. Six type decisions
took most of them: `hist_scratch` is a `uint8_t *`, two `rc_begin_*` locals are
offsets rather than pointers, twenty-three `M128` lanes hold addresses,
`symbol_list_update` returns nothing, and three of `Obj10`'s fields —
`f56[14]`, `f1051664[4]`, `f1078232`, `f1078688` — and four of `ModelBlock`'s
— `f76`, `f80`, `f84`, `f88` — are the row cursors they are used as.

§4.2 was right that the metric is not satisfiable by cosmetics, and enforcing
that is most of what took the last 213 off. The shortcut that would have taken
it from 243 to 179 in one pass was tried and reverted (§4.2's postscript). What
actually did it was reading the functions: the row-cursor tables in `Obj1`,
`Obj4`, `Obj8`, `Obj10`, `Obj19`, `Obj31`, `Obj69` and `ModelBlock` are
pointers, together with the two hundred locals they flow through; a dozen
register spills where MSVC kept a byte or a boolean in a register that held a
cursor are the moves they always were; and eleven forwarding shims now take
what they forward to.

Two of those retypes changed behaviour rather than just the count, and both
were caught by the gate rather than by reading:

* Typing `Obj10::f1078208` as `uint32_t *` passes all fifteen images and
  **segfaults the out-of-memory ladder**, because `f1078208 + 24 * n` steps 24
  bytes on a byte pointer and 96 on a `uint32_t *`. That is the only time this
  round the ladder caught something the images did not.
* Typing `ModelBlock::f6059436` as `uint16_t *` -- which is what the same field
  is called in `Obj10` -- moves three streams, for the same reason at a
  different scale. It is `uint8_t *`.

`-fpermissive` stays on the build line because `g.bat` had it, and because a
future extraction that reintroduces the old conversions should still build
while `BMF_STRICT=1` says what it cost.

---

## 1. Where this was when the plan was written

| | |
| --- | --- |
| `subs1.hpp` | 23 807 lines |
| `blob.inc` | 343 794 bytes, 65 892 initialisers |
| globals still declared as references into `blob1` | **78**, none unreferenced |
| — of those, written but never read | 35 (§3.2, §7) |
| arrays already moved out of it | 86, 50 832 bytes |
| SIMD intrinsic calls | **558**, 33 distinct, in 16 bodies |
| — of those, single-precision float arithmetic | 402 (`mul_ps` 189, `add_ps` 157, `sub_ps` 28, `div_ps` 28) |
| pointer casts | 5603 |
| raw-offset dereferences | 238 |
| conversions that need `-fpermissive` | **347** (+12 `-Wint-to-pointer-cast`, which do not) |
| `goto` / `LABEL_n:` | 113 / 81 |
| distinct `vNN` locals | 560 |
| recovered structs still `ObjN` with `fNN` members | 88 |
| vectoriser reports at `-O2` | 8 loops + 67 SLP blocks |
| vectoriser reports at `-O3` | 38 loops + 103 SLP blocks |

The last two lines are not the argument for goal 2 — they are the correction to
it. GCC 13 **does** vectorise at `-O2`; see §2.5. What it cannot do is vectorise
code that has no loops in it, and 558 hand-written intrinsic calls are exactly
that.

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

By kind, exhaustively — the categories below sum to 558:

| kind | n | what it is |
| --- | --- | --- |
| single-precision arithmetic | 402 | `mul_ps` 189, `add_ps` 157, `sub_ps` 28, `div_ps` 28 |
| lane shuffles, packs, unpacks | 53 | `unpacklo_ps` 11, `movelh_ps` 11, `srli_si128` 10, `shuffle_ps` 5, `movehl_ps` 5, six more |
| integer vector arithmetic | 34 | `add_epi32` 24, `slli_epi16` 4, `add_epi8` 4, `xor_si128` 1, `cmpeq_epi32` 1 |
| `_mm_prefetch` | 34 | no C equivalent |
| vector loads | 17 | `load_si128` 14, `loadl_epi64` 3 |
| converts and extracts | 10 | `cvtsi128_si32` 5, `cvtsi32_si128` 3, `cvtepi32_pd` 1, `movemask_pd` 1 |
| double-precision and float bitwise | 8 | all of them inside `estimate_cost` and `log_two_lane` |

The 402 are the whole of the win: straight-line vector expressions that are
loops written out. The 53 shuffles are the awkward ones (§2.3). The 34
prefetches have no C spelling — `__builtin_prefetch`, or drop them and measure.
The 8 double-precision ops are a self-contained problem in one function and can
be left until last.

### 2.2 The shape they translate into

`alt_p2_filter` is the model for the whole phase. Verbatim from `subs1.hpp`,
with only the line wrapping changed:

```c
  v5 = _mm_add_ps(
         _mm_add_ps(
           _mm_add_ps(
             _mm_add_ps(
               _mm_add_ps(
                 _mm_add_ps(_mm_mul_ps((__m128)__xmmword_441120, a2[0]),
                            _mm_mul_ps((__m128)__xmmword_441130, a2[1])),
                 _mm_mul_ps((__m128)__xmmword_441140, a2[2])),
               _mm_mul_ps((__m128)__xmmword_441150, a2[3])),
             _mm_mul_ps((__m128)__xmmword_441160, a2[4])),
           _mm_mul_ps((__m128)__xmmword_441170, a2[5])),
         _mm_mul_ps((__m128)__xmmword_441180, a2[6]));
  v6 = _mm_add_ps(v5, _mm_movehl_ps(v5, v5));
  v6.m128_f32[0] = v6.m128_f32[0] + M128F(_mm_shuffle_ps(v6, v6, 1)).m128_f32[0];
```

That is a **weighted sum of seven four-wide vectors against seven constant
vectors, then a horizontal sum of the four lanes** — a 28-term dot product.
Written as C it is two loops and a reduction, the coefficients are a
`float[7][4]` instead of seven blob constants, and the compiler emits the same
instructions — plus it can now see what it is.

The vertical part translates with nothing to think about: the `_mm_add_ps`
chain is left-to-right, which is what `for (i = 0; i < 7; i++) s += c[i]*a[i];`
also does. The horizontal part is where the care goes.

### 2.3 The one thing that can go wrong — and it is measured, not assumed

**Float addition is not associative.** The horizontal sum above adds lanes in a
specific order: `movehl_ps(v5, v5)` brings the high half down, so the add gives
lane 0 = `v5[0]+v5[2]` and lane 1 = `v5[1]+v5[3]`; `shuffle_ps(v6, v6, 1)` then
selects lane 1. The result is `(v5[0]+v5[2]) + (v5[1]+v5[3])`. A C reduction
written as `for (i = 0; i < 4; i++) s += v[i];` sums `((v[0]+v[1])+v[2])+v[3]`
and is **a different number** in the last bits.

This is not a worry, it is a fact about this file, and the experiment is cheap.
Replacing that one horizontal sum with the left-to-right order and running the
gate:

| | |
| --- | --- |
| streams unchanged | 12 of 15 |
| `t8g` | changed, same length (42 896) |
| `t8p` | changed, same length (43 664) |
| `x_ep` | changed, **330 684** bytes against a reference of 330 656 |

Building the unmodified file with `-ffast-math` breaks the same three images
(`x_ep` at 330 676). So:

* **The gate does catch reassociation.** A last-bit difference in
  `alt_p2_filter` moves a decision, the decision moves a symbol, and the stream
  stops matching. This is the opposite of the usual situation with float
  refactoring and it should be exploited: translate, let the gate arbitrate.
* **Twelve of the fifteen images did not notice.** That is the reason for small
  batches and for never running a subset of the corpus. A translation that
  passes `t24` and `t32` has been told nothing.

Two rules follow:

* **Never `-ffast-math`, `-funsafe-math-optimizations` or `-Ofast`.** Measured
  above: they break three images today. They license exactly the reassociation
  the gate exists to catch, and turning them on would mean regenerating the
  references, which means giving up the property that makes this whole project
  checkable.
* **Preserve the reduction order explicitly** where one exists. A horizontal
  sum written as `(v[0]+v[2]) + (v[1]+v[3])` compiles to the same tree the
  intrinsics build and stays bit-exact.

### 2.4 The `M128*` unions go too

`bmf.cpp` carries `M128I`, `M128F`, `M128D` and `M64` — unions that exist only
because MSVC's `__m128` has named members (`.m128_f32[0]`) and GCC's does not.

Their size is easy to misread. The wrapper *names* appear in `subs1.hpp` only
five times (`M128F(` three, `M128D(` two), because their real work is implicit:
457 lines mention `__m128` (307 occurrences of `__m128` itself, 535 counting
`__m128i` and `__m128d`), and those lines reach into the unions **657** times
through member spellings — `.m128_i32` 203, `.m128i_i32` 156, `.m128_f32` 112,
and nine more.

They are a translation artefact of a translation artefact: MSVC's spelling of a
type that only exists because of the intrinsics. When the intrinsics go, so do
these, and `bmf.cpp` loses its largest remaining block of scaffolding. The 657
member accesses are the measure of the job, not the five wrapper calls.

### 2.5 Making sure it actually vectorised — and what `-O2` really does

```
BMF_OUT=bmf.vec ./build.sh -fopt-info-vec 2>&1 | grep -c vectorized
```

Do **not** write `BMF_OUT=/dev/null`. `build.sh` runs `rm -f "$OUT" ./*.o`,
so that deletes `/dev/null` and the build then fails for an unrelated reason —
which is how the first draft of this document came to record "0 loops
vectorised at `-O2`".

The real baseline, GCC 13.3 on this tree:

| | loops | SLP blocks | total messages |
| --- | --- | --- | --- |
| `-O2` (the build's) | 8 | 67 | 75 |
| `-O3` | 38 | 103 | 141 |

Three things follow:

* **`-O2` already auto-vectorises.** `-ftree-loop-vectorize` and
  `-ftree-slp-vectorize` are both *enabled* at `-O2` in GCC 12 and later; the
  difference from `-O3` is `-fvect-cost-model`, `very-cheap` against `dynamic`.
  Adding `-ftree-vectorize` to `build.sh` would therefore be a no-op. If this
  phase wants the `-O3` behaviour without `-O3`, the knob is
  `-fvect-cost-model=cheap` (or `dynamic`), and it has to be justified by a
  measurement rather than added on principle — `g.bat`'s `-O2` is worth keeping
  for fidelity.

  **Measured after the translation, and declined.** `cheap` and `dynamic` both
  take the count from 19 loops to 41 and `-O3` to 50, but four passes over the
  four largest images are 17.8 / 20.1 / 16.9 s at `-O2`, `cheap` and `-O3`
  against 19.0 / 18.3 / 17.0 and 17.6 / 16.9 / 14.1 on repeat runs: the spread
  within one binary is larger than the spread between them. There is no
  measurement to justify the change, so `build.sh` still says what `g.bat`
  said.
* **Most of what is reported is SLP, not loops.** 103 of the 141 messages at
  `-O3` are `basic block part vectorized`. Counting "vectorized" therefore
  measures straight-line packing more than it measures loops, and the number to
  watch as intrinsics turn into loops is the `loop vectorized` count on its own.
* **`-O3` passes the gate byte-identically** — checked, all 15 streams and the
  archive, 942 532 bytes of binary against `-O2`'s 913 860. That shows GCC is
  not reassociating floats here. It is *not* evidence for §2.3's claim, which is
  about what happens when the source order changes; §2.3 measures that directly.

And measure the runtime. The corpus takes about 15 s; if translating 400
intrinsics into C makes that materially worse, the translation is wrong
somewhere and the `loop vectorized` count will say where.

### 2.6 What this hands to Phase B

Less than the first draft claimed, and the honest accounting is worth having.

74 distinct `__xmmword_*` constants exist. **37 of them are read by a body that
contains intrinsics** (31 only there, 6 also elsewhere); the other 37 are read
by non-SIMD code or, in eighteen cases, not read at all. 24 of the 74 are still
reached through `blob1`; 50 already sit in `bmf_*` byte arrays.

Translating a constant's users turns it into an ordinary `static const
float[4]` or `int32_t[4]` with a real initialiser, which is worth doing on its
own: a vector constant's extent is 16 bytes by construction, and its type
follows from the arithmetic around it.

But **Phase A does not take the 24 blob1 entries off Phase B's list.** §3.2
does, for a reason that has nothing to do with intrinsics, and it takes all 24
at once.

---

## 3. Phase B — `blob.inc` disappears

### 3.1 What is left

343 794 bytes of generated C, 65 892 byte initialisers, holding whatever the
first round did not move. Against it: **78 globals declared as references into
`blob1`**, and — contrary to what round one's Phase 3 note implies —
**every one of them is referenced somewhere.** There are no free deletions in
that list. (`REFACTORING.md` Phase 3 says "77"; the extra one is
`__bmf_half_half`, which has no `typedef` line and so slips past the two-line
regex in `tools/extents.py`. Its summary line, `77 globals: 2 const, 43 scalar,
32 var`, classifies by *subscript shape* — `scalar` means "never subscripted",
not "referenced" — and misreading it is how the first draft of this document
invented 34 dead globals.)

The machinery around it is `bmf_addr(va)`, `bmf_blob_relocate()` and
`bmf_data_relocate()` — an address-translation layer that exists so a global
can be reached by the virtual address it had in `BMF.exe`.

### 3.2 The 24 SIMD constants go first, and they are free

`__init_sse_constants()` (`subs1.hpp:7603`) is 24 assignments and nothing else:

```c
  __xmmword_445760[0] = __xmmword_439A00;
  ...
  __xmmword_4458D0 = __xmmword_439B00;
```

Every one of the 24 `__xmmword_4457xx`/`4458xx` slots still in `blob1` is
**written at startup from a `bmf_xmmword_439A*` array that has already moved out
of the blob** — 17 distinct sources, several used twice. So:

* **None of the 24 needs an initialiser recovered.** Whatever `blob.inc` holds
  at those addresses is overwritten before anything reads it. §3.3's hard step
  does not apply to them at all.
* **18 of the 24 are never read anywhere else.** `__xmmword_4457C0` through
  `__xmmword_4458D0` are written by `__init_sse_constants` and that is their
  only appearance. They and their assignments can go outright.
* **The remaining 6** — `__xmmword_445760` … `__xmmword_4457B0`, seven uses
  each — become direct references to their `bmf_xmmword_439A*` source, and
  `__init_sse_constants` goes with them.

That is 24 of 78 removed, the whole SIMD half of the blob, from one function
that says what it is doing. Do it before the reading starts.

Do not generalise the argument. 35 of the 78 are written and never read, but
only these 18 are safe to delete on that basis; see §7.

### 3.3 The 54 that are left, one at a time

Each needs three things, and the third is the work:

1. **Its extent.** How many bytes belong to it — the address of the next global
   is an upper bound, and `tools/extents.py` already computes these. Its
   `const N` / `var` / `scalar` column says which extents are knowable from the
   source at all: 43 of the 77 it sees are never subscripted, 2 have only
   constant subscripts, and 32 are indexed by an expression and so have no
   static extent.
2. **Its type.** What the code reads out of it. `plane_count` is an `int32_t`;
   `exclusion_mask` is a byte array indexed by symbol; `__byte_44339E` is
   indexed `[16 * plane]`, so 16 bytes per plane. Most of these are already
   typed at their use sites.
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
| `plane_count` | 154 |
| `__byte_44339E` | 47 |
| `__n3_0` | 29 |
| `__byte_44339D` | 26 |
| `__byte_4433AD` | 21 |
| `__n4_3` | 21 |
| `__n4_4` | 19 |
| `exclusion_mask` | 18 |

`plane_count` alone is 154 uses of an `int32_t` whose initial value is
irrelevant: **both** entry points compute it from the depth byte before
anything reads it — `compress_image` at `subs1.hpp:23333` and `expand_image` at
`21955`, each `((depth & 0x3F) + 7) >> 3`. The decoder path is the one to check
in every such case, because it is the one a corrupt stream reaches first.

Several of the others are the same: **a global whose first access is a write
does not need its initialiser recovered at all**, and finding those is worth
doing before the extraction work starts. §7 is the qualification.

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
same job as §3.3 and belongs in the same phase. §3.2's 17 SIMD sources are the
easy end of it: each is 16 bytes with a known type.

One of them should simply go: `bmf_pout_of_memory_handler`, 10 292 bytes,
exists because `set_new_handler` writes a function pointer into MSVC's CRT
new-handler slot. That slot is the array's first word, and it is the only word
anything touches — `bmf_new` reads it (`REFACTORING.md` §6) and
`set_new_handler` writes it. The other 10 288 bytes are CRT state nothing
references.

---

## 4. Phase C — the ugliness that is left

### 4.1 The casts, and what they are for

5603 pointer casts. They are not all noise — `structs.py` writes
`*(uint8_t *)&p->f8` to reach a byte inside a word, and that cast carries
information. Three groups do not, but only one of them is large:

* **Casts on the `M128*` unions.** 158 value casts — `(__m128)__xmmword_441120`
  and friends — plus the `(__m128 *)`-shaped pointer casts, 240 for the family
  as a whole. Phase A's.
* **Casts in the blob and moved-array declarations.** One per global: 78 for
  the `blob1` references, 86 for the moved arrays. The whole declaration block
  (lines 1–3910) holds 166 casts of the 5603, so Phase B's contribution here is
  small — its value is elsewhere.
* **Casts between an integer and a pointer.** The `-fpermissive` conversions,
  §4.2, which are the ones that indicate a wrong type rather than a narrow
  access.

That leaves 5437 casts inside function bodies, most of them the informative
kind. Measure after A and B before deciding what to do about the rest, and do
not treat the total as a target (§7).

### 4.2 `-fpermissive` is the honest metric

The build emits 359 warnings. Dropping `-fpermissive` turns **347** of them into
errors, and that is the number this phase is against:

| n | diagnostic | `-fpermissive`? |
| --- | --- | --- |
| 297 | `invalid conversion from X to Y` | yes |
| 37 | `cast from X to Y loses precision` | yes |
| 6 | `ISO C++ forbids comparison between pointer and integer` | yes |
| 7 | `comparison between distinct pointer types … lacks a cast` | yes |
| 12 | `cast to pointer from integer of different size` | **no** — `-Wint-to-pointer-cast` |

The 297 conversions by direction: **156 int → pointer, 97 pointer → int, 44
pointer → pointer.** (GCC splits them four ways in its output — 150 / 90 / 32 /
25 — but that split is only about which side it chose to print an `{aka …}` for,
and says nothing about the direction.)

Every one of the 347 is a place where the decompilation put an integer where a
pointer belongs, or the reverse, or compared two things that are not the same
kind. **The goal for this phase is stated exactly: the file compiles without
`-fpermissive`.** That is a binary condition, the compiler adjudicates it, and
it cannot be argued with — the same property that made `-Wunused-variable` and
`-Wuseless-cast` the right drivers in round one.

It is also the right metric because it is not satisfiable by cosmetics. Each
conversion is a type that is wrong, and fixing it means deciding what the thing
actually is.

**Postscript, from doing 47 of them.** Three decisions took 47 sites off the
count: `hist_scratch` is `uint8_t *` because `coded_buf` is; two locals in the
`rc_begin_*` pair are offsets into `model_geometry` rather than pointers; and
twenty-three `M128` lanes hold addresses, which the union now says with a
`char *m128_p[4]` member instead of an `int` read and a conversion.

The rest is one shape — **a slot Hex-Rays typed `int32_t` that holds a
pointer, or the reverse** — and it does not come off one declaration at a time.
Two measurements say so:

* Retyping `Obj10::f56[14]` (a set of row cursors, and unambiguously addresses:
  the code writes `*(uint8_t *)(f56[5] - 2)`) compiles and passes the gate, and
  takes the count **from 299 to 329**, because the `int32_t` locals those
  elements flow into are now wrong in the other direction.
* `tools/retype_locals.py` reads `strict.log` and offers every local whose
  complaints all agree on one source type — 62 of them. Applying all 62 leaves
  the file not compiling at all (192 errors). Applying them **one at a time and
  keeping only what still compiles keeps exactly one.**

So a field, the locals it flows into, and the calls between them are one unit,
and the unit is a function or two rather than a declaration. That is the shape
of the remaining 239, and it is why the count is a scoreboard and not a
burndown. `retype_locals.py` stays because the worklist it prints is the right
starting point for each of those functions, not because the rewrite it offers
can be taken.

**And this is where "not satisfiable by cosmetics" has to be enforced rather
than asserted.** Sixty-one of those 239 are a pointer stored into an `int32_t`
or `uint32_t` field — `_this->f56[6] = v28`, `v24->row[1] = v29` — and writing
`(uint32_t)(v28)` at each of them compiles, passes the gate, and takes the
count from 243 to 179 without a single type being decided. That was tried and
reverted. A cast at the store says "an address goes in this integer slot",
which is true and is exactly the defect; the fix is that the slot should not be
an integer. If the count ever falls quickly, this is what to check first.

The 12 `-Wint-to-pointer-cast` warnings are worth pulling out first anyway,
because they are the narrowest and most mechanical group — but they are not part
of the `-fpermissive` count, and they are not the 64-bit blocker either.
`REFACTORING.md` §Phase 4 used to establish that the 64-bit gap is the
program's **32-bit pointer fields**, which cannot be widened because every
object is walked with variable offsets as well as constant ones, and which the
low arena in `bmf.cpp` addressed instead. **Round 11 closed that gap** — the
objects are reached by name, `tools/ptrwidth.py` reports zero, the arena is
gone, and `tools/x64.sh --high` proves it by putting every allocation where a
four-byte pointer cannot reach. These 12 were part of it after all.

### 4.3 The `vNN` names

560 distinct. `rename.py --in FUNC` makes them renameable one body at a time,
and round one showed the cost: about ten names per session of reading, each
argued from what the code does with it. At that rate this is not a phase, it is
a background task, and it should be treated as one — a body at a time, when
that body is being read for another reason.

The exception is the `ObjN` structs' `fNN` members, 88 structs' worth. Those
are not read-one-at-a-time work: `algorithm_v2.md` establishes what several of
the objects *are*, and naming a struct's fields is worth doing in one pass per
struct when its writer is understood. `BmfImage` and `BmpHeader` are the
precedent.

### 4.4 `goto` and `LABEL_n:` — re-examined twice, and the conclusion stands

113 `goto` statements and 81 labels. Round one's Phase 6 concluded these are
irreducible: of 123 gotos then, two qualified for `degoto.py`'s rewrite, 21
were backward jumps that are loops Hex-Rays could not name, and 100 were
forward jumps that are either not the whole of an `if` or jump into a region
with other entries.

That conclusion was reached before the frames became structs and before the
alt-model bodies were merged, both of which changed the control flow's shape,
so it was worth re-running. It has been, before this round and again after it:
**`tools/degoto.py` reports 0 candidates of 112 gotos.** The count fell from
123 to 111 as the intrinsics went, and the candidate count from 2 to 0. The
conclusion holds and there is nothing here — this subsection exists to say so.

---

## 5. Order, and what depends on what

```
  Phase A  intrinsics → C          removes the M128 unions, 158 value casts,
     │                             and 657 member accesses
     ▼
  Phase B  blob.inc → typed C      §3.2 removes 24 blob globals outright;
     │                             the other 54 one at a time
     ▼
  Phase C  -fpermissive → clean    what is left is the real type errors
```

The arrows are sequencing, not dependency. A genuinely blocks nothing in B, and
B blocks nothing in C. The order is A → B → C because:

* A is where the reading pays off twice: translating the p2 arithmetic is how
  `algorithm_v2.md` §9 gets written, and §9 is what tells you whether a
  translation is right beyond "the gate passed".
* C last because it is the only one whose finish line is a compiler flag rather
  than a count, and a flag should be measured against a file the other two have
  already cleaned. B removes 164 declaration casts before C starts counting.

Within each phase: **one object, one body, or one intrinsic family at a time,
gated.** Round one's evidence for that is `tools/struct-sweep.sh` — 130 rounds,
of which a dozen failed the gate and were reverted, and one of the failures was
caught only by an image the sweep's filter had been skipping. §2.3's experiment
is the same lesson from the other direction: three of fifteen images noticed a
real regression.

---

## 6. What the gate needs

It is in good shape — 15 images with committed reference streams, a two-member
archive, 15 refused inputs with pinned exit statuses, and an out-of-memory
ladder. The corpus runs in about 15 s.

* **Nothing, for Phase A — checked, and this is the good news.** The obvious
  worry was round one's: 268 lines of `alt_model_p1_decode` were broken for a
  fortnight because no image ran them. So the question was asked before planning
  rather than after, and the answer is as strong as it could be:

  **All 558 intrinsic call sites lie on lines that execute during a gate run.**
  Not one is on an uncovered line, in any of the 16 bodies.

  | body | line coverage |
  | --- | --- |
  | `alt_p2_model` | 1591 of 1591 — 100 % |
  | `alt_p2_context` | 817 of 817 — 100 % |
  | `alt_p2_filter` | 129 of 129 — 100 % |
  | `choose_plane_coding` | 622 of 656 — 94.8 % |
  | `estimate_cost` | 31 of 48 — 64.6 % |

  The two bodies that are not at 100 % are still fully covered *where the
  intrinsics are*. `choose_plane_coding`'s 34 unreached lines contain none.
  `estimate_cost`'s unreached lines are its `n2 < 2` guards and its scalar
  odd-bin tail — all 15 call sites pass 512 or 1024, so the tail cannot run;
  it is scalar `log()` arithmetic with no intrinsics in it. If a translation
  changes that function's shape, the tail still has no test, and the honest
  move is to say so rather than to assume an odd histogram never happens.

  Combined with §2.3 — a deliberate reassociation fails the gate on three
  images — Phase A is as well gated as anything in this project.
* **A `loop vectorized` count in the plan's numbers**, so "the compiler
  vectorises it" is a measurement and not an intention. Count `loop vectorized`
  separately from `basic block part vectorized`; §2.5 explains why the combined
  figure misleads. *Done: 8 before the translation, 19 after.*
* **A no-`-fpermissive` build target**, failing at first, as Phase C's
  scoreboard: `BMF_STRICT=1 ./build.sh` counting the errors it gets. Today that
  is 347. *Done, and it is in `build.sh`; the count is 0.*

  One trap in writing it, worth recording because it inflated the count by one
  for the whole phase: `opts=("${opts[@]/-fpermissive/}")` leaves an **empty
  element** in the array rather than removing it, and g++ reports that as
  `error: : linker input file not found`. Build the array without the flag
  instead.
* **A poisoning check would have been worth having in round one.** Filling an
  array or a region with 0xCC and running the gate answers "is this read?" in
  one build, and it is what settled `blob.inc`, four oversized arrays and the
  64-byte guard tails. It belongs next to `deadcheck.py` as a tool rather than
  as a thing done by hand.

---

## 7. What would make this fail

* **Translating floats by pattern instead of by reading.** A horizontal sum has
  an order and a reduction loop has a different one, and §2.3 measured what that
  costs. The gate catches it, but a tool that rewrites 400 sites and fails the
  gate tells you nothing about which of the 400 was wrong. Small batches, whole
  corpus.
* **Treating "written and never read" as "deletable".** 35 of the 78 blob
  globals are only ever assigned. Eighteen of them are safe (§3.2) because
  `__init_sse_constants` is the only writer and nothing reads that address
  range. The other 17 are not: `__n2_0 = 2; __n4 = 4;` at 0x44571C and
  0x445720, `__byte_445440[0] = …`, and fourteen more are adjacent slots in one
  data-segment region, and `REFACTORING.md` Phase 3 measured
  exactly this failure — **a store through one name has to be visible through
  another**, which is why the all-at-once blob split segfaults. Write-only is a
  hint about where to look, not a licence.
* **Deciding a blob global's type from its name.** `__n4_3`, `__n256_2`,
  `__n15` are named for the last constant compared against them
  (`REFACTORING.md` §6), and a wrong type silently changes the initialiser
  bytes' meaning. The type comes from the use sites, the initialiser comes from
  the blob, and both have to agree.
* **Chasing the cast count.** 5603 is not a target. §4.1 accounts for where the
  removable ones are and they are a few hundred; the rest carry information.
  `-fpermissive` is the metric; the cast count is a side effect.
* **Believing a tool that reports success.** Round one had `unused.py` report
  "no warnings" while 66 sat in the file, for two independent reasons at once.
  Every count in this document is reproducible by the command in Appendix A,
  and Appendix B lists the three places where the first draft of this document
  failed that standard.

---

## Appendix A — how the numbers were measured

From `003/`, GCC 13.3, `-m32`:

| number | command |
| --- | --- |
| intrinsic calls, by name | `grep -o '_mm_[a-z0-9_]*' subs1.hpp \| sort \| uniq -c \| sort -rn` |
| intrinsics per body | `tools/structs.py`'s `bodies()` + the same regex |
| blob globals | `grep -n 'blob1 +' subs1.hpp`, minus the two comments and `bmf_addr`'s `return` |
| a global's uses | count `\bNAME\b` on every line but its declaration |
| extents and subscript shape | `python3 tools/extents.py` |
| `-fpermissive` count | compile without `-fpermissive`, `-fsyntax-only`, count `error:` |
| vectorisation | `BMF_OUT=bmf.vec ./build.sh -fopt-info-vec`, count `loop vectorized` and `basic block part vectorized` separately |
| coverage | `BMF_STATIC=0 BMF_GC=0 BMF_OUT=bmf.cov ./build.sh --coverage -O0`, run `test.sh ./bmf.cov`, `gcov -f bmf.cpp` |
| intrinsics on covered lines | intersect the `_mm_` line numbers with `#####` lines in `subs1.hpp.gcov` |
| reassociation experiment | rewrite `alt_p2_filter`'s horizontal sum, `./build.sh`, `./test.sh` |
| goto candidates | `python3 tools/degoto.py subs1.hpp` |

`gcov` needs the profile files named after the source: build with `BMF_OUT=x`
and the `.gcno`/`.gcda` arrive as `x-bmf.gcno`; rename to `bmf.gcno`/`bmf.gcda`
before running `gcov bmf.cpp`. Do not pass `-fprofile-dir`, which mangles the
`.gcda` name further.

## Appendix B — what the first draft of this document got wrong

Kept because it is the same failure mode §7 warns about, and because two of the
three were numbers that would have shaped the work.

1. **"77 globals declared, 43 referenced" and "the 34 dead ones go first."**
   Both came from misreading one line of `tools/extents.py` output — `77
   globals: 2 const, 43 scalar, 32 var` — where `scalar` means "never
   subscripted". There are 78 declarations and none is unreferenced. An entire
   subsection proposed deleting globals that are all in use, on a rule ("the
   name appears twice, in its `typedef` and itself") that does not hold because
   the `typedef` names `t_NAME`, not `NAME`.
2. **"0 loops vectorised at `-O2`."** Measured with
   `BMF_OUT=/dev/null ./build.sh`, which `rm -f`s `/dev/null` and then fails.
   `-O2` vectorises 8 loops and 67 blocks. The draft's conclusion — add
   `-ftree-vectorize` to `build.sh` — would have been a no-op, since GCC 12+
   enables it at `-O2` already.
3. **"74 constants feed these bodies … Phase A takes 74 entries off Phase B's
   list, 24 of them from the harder half."** 37 are read by an intrinsic-bearing
   body, and the 24 in `blob1` are the *easiest* entries rather than the hardest:
   `__init_sse_constants` overwrites all of them at startup from arrays that
   already moved, so none needs an initialiser recovered, and 18 are never read
   at all. Phase A is not what removes them.

---

## Appendix C — what is left

Goals 1 and 2 are done. Goal 3 is at 299 of 347, and the three things below are
what round three would be.

* **The casts.** 5 448 of them, and §4.1's accounting still holds: the ones
  Phase A and Phase B were going to remove are gone, and what is left is
  mostly the informative kind plus the several hundred this phase added
  spelling out conversions the decompilation left implicit. A pass over those
  now that the types underneath them are right would find some that no longer
  say anything.

* **`bmf_bss` should stop being one object.** 19 584 zero bytes with the
  original relative offsets is what made goal 1 safe to finish, not where it
  should end. The per-plane record table is the clearest piece: sixteen bytes a
  plane, bytes at +0/+1/+2 and int32 at +4/+8/+12, which `__byte_44339C`,
  `__byte_44339D`, `__byte_44339E`, `__dword_4433A0`, `__dword_4433A4` and
  `__dword_4433A8` are the six fields of. Naming it is one struct and about
  twenty rewrites.

* **The `vNN` names, still a background task.** 560 distinct, and §4.3's rate
  has not changed: about ten a session of reading. The p2 family is now
  readable enough to make it worth doing there first — `alt_p2_filter` and the
  LMS blocks in `alt_p2_context` and `alt_p2_model` say what they compute, and
  the locals around them do not.

What this round settled that was open:

* `estimate_cost` measures a histogram the caller builds, and the two-lane
  accumulation is load-bearing. `algorithm_v2.md` §9's question about the p2
  family has an answer: normalised LMS, three passes.
* `hist_scratch` is read: it is `coded_buf + coded_size - 4096`, the histogram
  scratch at the end of the coded buffer.
* The 64-byte guard tails were not "nobody has explained" — they were holding
  nineteen stale copies of one table together.
