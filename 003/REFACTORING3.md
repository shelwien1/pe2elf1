# Refactoring BMF 2.01, round three

## 0. What happened

This was written as a plan and then worked through. Phase B is finished; A and
C are part done and the parts that are not have measured reasons. Every number
below is `python3 tools/shape.py` before and after.

| | planned | before | now |
| --- | --- | --- | --- |
| raw-offset sites | fall by most of itself | 1930 | **1843** |
| — off `_this` | | 403 | **363** |
| globals at a 1997 address | 0 | 60 | **0** |
| `bmf_bss` | gone | 19 584 bytes | **gone** |
| frames | 0 | 24 | **23** |
| frame aliases | 0 | 602 | **396** |
| runs walked as arrays | 0 | 24 sites, 12 bases | **10 sites, 6 bases** |
| structs | fewer | 93 | 94 |
| — still `ObjN` | fewer | 88 | **87** |
| `fNN` members | named | 280 | **263** |

**Phase B is done.** All 60 globals left `bmf_bss` and the array itself is gone
(§3.1 has the details, corrected where the work refuted the plan). Two of them
could not separate from each other and now say so in an 8736-byte object
instead of by accident in a 19 584-byte one.

**Phase C is about a third done.** 206 of the 602 aliases lifted, three frames
dissolved, and six of the twelve walked runs are declared arrays — including
both p2 plane arrays and both p1 ones, which §5 said to do first.

**Phase A is partly done.** `ModelBlock` absorbed `Obj10` after both of their
disagreements were settled by reading; the plane-descriptor table is declared
and 178 of its subscript sites read as records. The two merges that are left
are blocked on things the plan did not know:

* **`Obj11` cannot absorb `Obj8`, `Obj19`, `Obj31` and `Obj69` by union.**
  `Obj11::f278528` is `__m128[21]` covering +278528..+278863, and the other
  four declare 15 individual fields inside those bytes. It is not a coarse
  recovery to be discarded either: `f278528[j].m128_f32[k]` is indexed with a
  *loop variable* in `alt_p2_filter`. The region is genuinely both, so the
  merge needs the union written by hand from a reading of the filter, not a
  union built from offsets.
* **`Obj92` cannot become `Obj0`.** Every offset in `alt_p1_alloc` is
  `*((int32_t *)_this + K)` for K ≤ 53, which is `Obj0`'s `f0`/`f4`/`f8`/
  `f12[51]` exactly — and retyping the parameter alone, with no offset touched,
  moves four streams. Something walks that pointer at the struct's own stride.
  §6's first hazard, in the place §2.3 said to start.

Four things the plan asserted turned out to be wrong, and each is corrected in
place below with the measurement that corrected it:

1. **"Nine frames dissolve outright."** Three do. The shared-slot and run-site
   tests miss two other ways a frame stays one object, and both were found by
   the gate rather than by reading — see §4.2.
2. **The alias count was 38 short**, because it missed the array bindings.
3. **Group C was not "six counter clamps"** but a threshold pair and four
   accumulators, and **group E was not ten scalars** but five and a table.
4. **`&x[i]` is not address-taking.** Reading it as such pinned six frames that
   have nothing wrong with them.

New tools: `defram.py` (lift frame members), `runarray.py` (declare a walked
run), `merge.py` (union two recoveries), `unbss.py` (give a global storage),
`frame-sweep.sh` (lift, gate, keep or revert). `shape.py` is the scoreboard.

---

`REFACTORING.md` and `REFACTORING2.md` are the records of rounds one and two.
Round two finished all three of its goals: `blob.inc` is gone, there are no SIMD
intrinsics left, and the file compiles without `-fpermissive`. What is left is
still ugly, and this round names the three things that make it so:

1. **`_this + ofs` with a cast on every access.** 1930 raw-offset sites, 403 of
   them off a parameter called `_this`. A struct member is not a cast.
2. **`bmf_bss` should be ordinary tables and variables.** 60 globals are
   references into one 19 584-byte array at their 1997 addresses.
3. **The frames should be ordinary locals.** 24 `struct alignas(16)` frames,
   170 320 bytes, reached through 564 reference aliases.

They are one problem seen three times: *an object whose layout is known by
address arithmetic rather than by declaration.* The remedy is the same in all
three — declare the layout, then let the compiler do the arithmetic — and the
danger is the same too, which round two measured twice and §6 records.

The gate does not change: **byte-identical compressed streams against the
committed references, every image round-trips, 15 refused inputs, and the
out-of-memory ladder.** Everything below is answerable to it.

---

## 1. Where this was

`python3 tools/shape.py` prints this table, so every number in this document can
be re-derived without trusting it.  **This is the state the plan was written
against; §0 has the same table as it stands now.**

| | |
| --- | --- |
| `subs1.hpp` / `bmf.cpp` | 19 728 / 787 lines |
| raw-offset sites | **1930** (see Appendix A for the three shapes counted) |
| — of those, off `_this` | 403, in 15 functions |
| pointer casts | 5383 |
| `bmf_bss` globals | **60**, in one 19 584-byte array |
| Hex-Rays frames | **24**, 170 320 bytes, 564 aliases |
| — frame slots carrying more than one name | 27 slots, 62 extra names, 13 functions |
| — runs of frame members used as an array | 24 sites, 12 bases, 6 functions |
| — frames that dissolve outright | 9, carrying 232 aliases |
| recovered structs still `ObjN` | 88 of 93 |
| `fNN` members | **280**, against 35 named ones |
| distinct `vNN` locals | 560 |
| `goto` / `LABEL_n:` | 112 / 79 |
| `__fwd_*` shims | 94 |

Two of those numbers are the shape of the whole round. **1930 raw-offset sites**
is what "the layout is not declared" costs at the use sites, and **280 `fNN`
members against 35 named ones** is why: the declarations that do exist are
recoveries of access patterns rather than of objects, so they carry offsets
where they should carry names.

---

## 2. Phase A — the workspaces get one struct each

### 2.1 What `_this + ofs` actually is

The 403 raw offsets off `_this` are concentrated in the allocators and the
models (`tools/shape.py --this`):

| function | sites | `_this` is declared |
| --- | --- | --- |
| `alt_p1_model` | 131 | `Obj0 *` |
| `alt_p1_alloc` | 58 | `Obj92 *` |
| `alt_p1_context` | 57 | `Obj25 *` |
| `alt_p2_alloc` | 57 | `char *` |
| `symbol_list_update` | 24 | `Obj6 *` |
| `expand_alphabet` | 19 | `Obj10 *` |
| `alt_p2_encode_symbol` / `_decode_symbol` | 26 | `Obj7 *`, `Obj13 *` |
| seven others | 31 | |

`alt_p2_alloc` is the clearest case, because it has no struct at all:

```c
char *__alt_p2_alloc(char *_this, int32_t i, int32_t n4)
{
  *(uint32_t *)(_this + 278728) = n4;
  for ( j = 0; j < 0x14000; ++j ) {
    *(uint16_t *)(_this + 8 * j + 284714) = 0;
    *(uint16_t *)(_this + 8 * j + 284718) = 0;
  }
  for ( k = 0; k < 0x14000; ++k ) {
    *(uint8_t *)(_this + 8 * k + 284712) = 5;
    *(uint8_t *)(_this + 8 * k + 284713) = 2;
    ...
```

That is a table of 8-byte records at +284712, written as arithmetic. The
excerpt's `+278728` is one of a run of dwords the same function fills a hundred
lines later, and that run says what the fields are:

```c
  v9 = 16 * near_lossless_max[0];
  *(uint32_t *)(_this + 278720) = -v9 - 7;
  *(uint32_t *)(_this + 278724) =  v9 + 8;
```

— a `±` range pair derived from the near-lossless setting, sitting beside the
`+278728` the excerpt writes. This one function touches **47 distinct offsets**,
from +4 to +940086, and the strides it indexes with are 8 and 2 — record sizes,
not arbitrary numbers. Across the file, `_this` takes **41 distinct constant
offsets above 1000**, and the strides applied to it are 16 (19 sites), 8 (9),
2 (8) and 4 (1).

`alt_p1_model` is the same shape with a struct that only covers the head:
`Obj0` declares `f0`, `f4`, `f8` and `f12[51]`, and everything past +212 arrives
as `*(uint32_t *)((char *)_this + ...)`. Its 131 sites are the largest single
concentration in the file.

### 2.2 Several of the structs that exist are views of one object

`structs.py` recovered a struct per *access pattern*, so the same workspace has
several. `tools/shape.py --overlap Obj8 Obj11 Obj19 Obj31 Obj69`:

| offset | `Obj8` | `Obj11` | `Obj19` | `Obj31` | `Obj69` |
| --- | --- | --- | --- | --- | --- |
| +278528 | `uint64_t` | `__m128[21]` | — | — | `__m128[7]` |
| +278640 | `uint64_t` | — | — | — | `uint64_t` |
| +278648 | `uint64_t` | — | — | — | `uint64_t` |
| +278660 | `int32_t *` | — | — | — | `int32_t *` |
| +278664 | `uint8_t *` | — | — | — | `uint8_t *` |
| +278668 | `uint8_t *` | — | — | — | `uint8_t *` |
| +278672 | `uint8_t *` | — | — | — | `uint8_t *` |
| +278736 | `uint8_t *[10]` | — | `uint8_t *[6]` | `uint8_t *` | `uint32_t[5]` |
| +278756 | — | — | — | `char *` | `char *` |
| +278760 | — | — | `char *` | `char *` | `uint32_t[24]` |
| +278764 | — | — | `char *` | `char *` | — |
| +278768 | — | — | `char *` | `char *` | — |

Agreeing where they overlap is suggestive. What settles it is that **one
expression in one function is cast to three of them**. `alt_model_p2_encode`
builds an array of plane workspaces in a run of frame slots and then reads the
same element back under three names:

```c
      *(&lpAddress + n4++) = (Obj11 *)v8;          // 15736, the store
      ...
      v18 = &lpAddress + n4_2;
      v25 = (Obj31 *)((int32_t)*(v18 - 1));        // 15770
      v14 = (Obj19 *)((int32_t)*(&lpAddress + n4_2 - 1));   // 15853
      v56 = (Obj8  *)((int32_t)*(v18 - 1));        // 15886
```

and the fifth name comes from the callees: `alt_p2_context` takes `Obj11 *`,
`alt_p2_model` takes `Obj69 *`, and both are called with elements of that array.
`alt_model_p2_decode` repeats all of it at 16677..16826. So `Obj8`, `Obj19`,
`Obj31` and `Obj69` are `Obj11` seen from four call sites, and this is evidence
rather than an alias rule.

The same is true on the other side: **`Obj10` and `ModelBlock` share 12 offsets
and both reach +6 059 436**, and they disagree at two of them —

| offset | `Obj10` | `ModelBlock` |
| --- | --- | --- |
| +1078684 | `void*` | `uint32_t` |
| +6059436 | `uint16_t *` | `uint8_t *` |

Round two found the second one the hard way: making `ModelBlock`'s match
`Obj10`'s moves three streams, because `f6059436 + 2` then steps four bytes
instead of two. Two declarations of one object is a bug waiting for a reader.

This is the same operation round two did to the image descriptor, where five
recoveries (`Obj14`, `Obj33`, `Obj71`, `Obj98`, and `write_bmp`'s raw offsets)
became `BmfImage`. It took reading `alloc_image`, which writes all four
descriptor words in one place.

### 2.3 The order to do it in

**Merge before naming.** Naming `Obj69::f278736` while `Obj8::f278736` still
exists means naming it twice and finding out later that one of them was wrong.

1. **`Obj11` absorbs `Obj8`, `Obj19`, `Obj31` and `Obj69`.** 11 + 5 + 5 + 16
   members against `Obj11`'s 13; the union is the plane workspace. §2.2's
   `lpAddress` run is the justification, and the disagreement to resolve is
   +278736, which is a pointer array in three of them and `uint32_t[5]` in
   `Obj69`.
2. **`ModelBlock` absorbs `Obj10`.** 22 members against 25, 12 shared, and the
   two disagreements above to resolve by reading rather than by picking.
3. **`alt_p2_alloc`'s `char *_this` becomes that struct**, and its 57 offsets
   become members. The allocator is the right place to start because it is where
   the layout is *written*, so it says what the fields are and how big they are.
4. **The p1 side.** `Obj1`, `Obj4` and `Obj25` all declare row cursors in the
   same span — `Obj1` as `row[5]` at +176..+192 and `cur[5]` at +196..+212,
   `Obj4` as `f176[10]` at +176..+212, `Obj25` as five separate `uint8_t *` at
   +196..+212 — and `Obj0::f12[51]` covers +12..+212, the same span read as
   dwords. `Obj92` has one member, `f0`. That is five views of the p1 workspace,
   and `alt_p1_alloc` (58 offsets, `Obj92 *`) is where it is written.
5. **Then the `fNN` names**, one struct at a time, when its writer is
   understood. 280 members against 35 named ones. `algorithm_v2.md` already
   establishes what several of these objects are.

The machinery from round one still applies: `tools/structs.py` recovers a struct
and rewrites its accesses, `tools/dedup.py` collapses byte-identical
declarations, `tools/arrayify.py` turns a run of same-type members into the
array it is. `tools/objects.py` is the one to run first — it groups names that
denote the same allocation (assignment or call, transitively, through the
`__fwd_*` shims) and prints each class's field map with the type conflicts
called out rather than resolved. It reports 54 classes with a dereference today,
down from round one's 159 because struct recovery has already absorbed most raw
offsets, and its largest class is `_this` with 26 offsets — Phase A's target.

What it cannot do is the last step. It relates *names*; §2.2's four views are
different struct *types* on names it has already joined, and deciding that two
recoveries are one object is a judgement about the evidence, not a closure.

### 2.4 What this is worth

The 1930 raw-offset sites are the count to watch, and it should fall by most of
itself: a member access replaces a cast *and* a multiply. The cast count (5383)
falls with it, and `algorithm_v2.md` §9 gets easier to finish, because the p2
workspace's fields are the thing it does not yet name.

---

## 3. Phase B — `bmf_bss` becomes tables and variables

### 3.1 What is in it

19 584 zero bytes with 60 globals at their 1997 addresses. Round two put them
there because the code strides between them and separating them one at a time
segfaults — `REFACTORING.md` Phase 3 measured that, and **41 of these 60 are
marked SHARED in `tools/blob-independence.txt`**. (The other 19 were never
tested one at a time; §3.2 uses that.) Keeping one object made goal 1 safe to
finish. It is not where this should end.

They are not 60 things. They are five — `tools/shape.py --bss` prints the
grouping, the address and the subscript shape of each:

**A. The plane-descriptor table, 0x44338C..0x4433DB — 20 globals.** *(Done:
`PlaneDesc plane_desc[5]`, 178 of the ~221 subscript sites converted; the 43
left are an index multiplied by 16 in a local that has other readers.)* Five
16-byte records based at **0x44338C**. Record 0 holds image-wide parameters;
records 1..4 are the four planes, so the plane `p` a reader sees is record
`p + 1`:

```
        +0                +1               +2               +3
  rec0  __n256_2 . . . . . . . . . . . . . . . . . . . . . . . .   (read as one dword)
  rec1  __byte_44339C     __byte_44339D    __byte_44339E    __byte_44339F
  rec2  __byte_4433AC     __byte_4433AD
  rec3                    __byte_4433BD
  rec4  __n3_1            __n3_0                            __byte_4433CF

        +4                +8               +12
  rec0  __n512            plane_count      near_lossless_max
  rec1  __dword_4433A0    __dword_4433A4   __dword_4433A8
  rec4  __n191            __n191_0         __n191_1
```

Three independent things say this is one table rather than 20 globals:

* the subscripts already in the code — `__byte_44339E[16 * plane]`,
  `__dword_4433A0[4 * plane]`, both stepping whole records;
* **field +1 is reached from two different origins, one record apart.**
  `transform_planes` reads it as `BYTE1(__n256_2[4 * n4_1])` after
  pre-incrementing `n4_1` from 0, so its index is 0-based on the *table* and its
  first iteration is record 1; `rc_begin_encode` reads the same field as
  `__byte_44339D[16 * p]`, 0-based on the *planes*. Two origins differing by
  exactly one record is what a header entry in front of the array looks like.
  (Both then feed the result back in as a record index —
  `__byte_44339E[16 * v14]` — so field +1 holds a plane number, which is a name
  waiting to be written down);
* and `alt_model_p2_encode` walks all four plane records with one loop, using
  four of the record-0 globals as the four field bases:

  ```c
        n16 = 16;
        do {
          v179[n16 + 1] = near_lossless_max[n16];                          // +12
          v179[n16]     = *(int32_t *)((char *)&::plane_count + n16 * 4);  // +8
          v178[n16 + 1] = (char *)__n512[n16];                             // +4
          v178[n16]     = (char *)__n256_2[n16];                           // +0
          n16 -= 4;
        } while ( n16 * 4 );
  ```

  `n16` = 16, 12, 8, 4 is records 4, 3, 2, 1 — the planes — and the four bases
  are record 0's four dwords.

So `plane_count`, the scalar read 154 times, is **field +8 of record 0**, and
`near_lossless_max`, `__n512` and `__n256_2` are its +12, +4 and +0. That is the
single most useful fact in this section and it is why there is no separate group
for them.

One question the table does not answer: records 1..4 use +0..+3 as four
separate bytes, and record 0's +0..+3 is read as a whole dword
(`__n256_2[0] - packer_free_bits + …` at 18433). Union, or a genuinely
different record 0. Reading the packer settles it.

**B. The level geometry, 0x445714..0x445733 — 21 globals.** *(Done:
`LevelGeom level_geom[8]`.)* A table of 4-byte
records with three bytes used in each, `{start, start/2, start - level}`.
`rc_begin_encode` writes six of them out longhand, one per level:

```c
    v6 = v4 + 2 * (uint8_t)__n16;
    __byte_445730    = (char)v6;        // where level 7's symbols start
    __byte_445732[0] = (uint8_t)v6 - 7; //   minus the level
    memset((char *)model_geometry + v6, 0x07, 64);
```

Eighteen of the 21 are those six records (levels 2..7, at 0x44571C, 0x445720,
0x445724, 0x445728, 0x44572C, 0x445730). The other three, at 0x445714..0x445716,
are the *same three fields* read with a variable index — `__byte_445714[4 * n]`
— which makes this the exact pattern round two found in the guard tails: a
strided table whose entries also exist as separate globals. Record 1, at
0x445718, is never named because nothing reads it individually.

**C. Six int32 at 0x4458E0..0x4458F7 — two things, not one.** *(Done:
`deadzone_hi`/`deadzone_lo` and `ctx_bias[4]`.)* The last two,
`__dword_4458F0` and `__dword_4458F4`, are the near-lossless error thresholds,
set together in `rc_begin_encode`:

```c
  v8 = 4 * near_lossless_max[0] + 1;
  __dword_4458F0 =  v8;
  __dword_4458F4 = -v8;
```

and read 125 and 123 times as `(x > +d) - (x < -d)` — a sign with a dead zone.
They are one pair and should be one pair of named constants. The four before
them, used 16 times each, are running bias accumulators: zeroed at the start of
a plane, decayed `>>= 3` each row and re-accumulated in `alt_model_p2_encode`,
then added one apiece into the four context sums `alt_p2_context` builds. Four
parallel accumulators is an array. **None of the six is marked SHARED**, which
makes this the cheapest group to move.

**D. Three real buffers.** `exclusion_mask` at 0x443440 (8192 bytes to the next
global, indexed by symbol), `__byte_445440` (544, declared `uint8_t[544]`),
`model_geometry` at 0x445660 (declared `int32_t[32]`, 160 bytes to the next
global; the symbol → level table `rc_begin_*` fills). These are already arrays;
they need a size and a home of their own, not a type.

*Measured:* `model_geometry` moves out clean. The other two segfault every
multi-plane image the moment either gets storage of its own, and they are
exactly the two that `blob-independence.txt` marks SHARED while
`model_geometry` is one of the nineteen it never tested. They are shared with
each other — 8192 bytes and then 544, with something reading across the
boundary — so they keep their adjacency in an 8736-byte object that says why.
Finding the reader is what would separate them.

**E. Five plain scalars and a table** — this said ten, and one of the ten was
subscripted. `__dword_44573C[n4]` steps four bytes, so the four globals after
it were the elements it was stepping onto; they are `tbl44573C[1]` .. `[4]`
now, and `init_model_tables` reading three of them as bare symbol values is the
same table read the other way. The five that really are scalars: `desc_slow_mode`, `__dword_443388`, `__byte_445700`,
`__n8_1`, `__n8_0`, `__dword_44573C`, `__n4_4`, `__n4_3`, `__n15`, `__n15_0`.
These are `static int32_t x;` and nothing else — `__byte_445700` is a rolling
stamp value `exclusion_mask` entries are compared against, `__n8_0`/`__n8_1` are
model parameters `rc_begin_*` sets to 8/8 or 64/16. Five of the ten are not
marked SHARED.

### 3.2 The order

**C, then E, then D, then B, then A.** C first because none of its six is marked
SHARED, so one split-and-gate says whether un-marked really means independent —
and that answer decides how much of E and D is free. Then the strided groups,
smallest first, because each one that leaves makes `bmf_bss` smaller and the
strides between what is left easier to see.

A is last because it is the largest and because it is the one whose shape was
wrong until it was measured: the first draft of this document had its record 0
written up as a separate group of four unrelated dwords. Twenty globals, one
table, and `plane_count` inside it.

A global that is provably not strided across can move on its own; round one
moved **86 of 163** out exactly this way, one at a time, and
`tools/blob-independence.txt` is the per-global record of which ones could.

**The names go with the move.** `__n4_3` is named for the last constant compared
against it (`REFACTORING.md` §6) and means nothing; a global being given real
storage is a global whose reader has just been read.

When the last one goes, `bmf_bss` goes, and with it the last thing in the file
that knows what address BMF.exe loaded at.

---

## 4. Phase C — the frames become locals

### 4.1 What they are

Hex-Rays could not name these. Round one's Phase 2 split all 24 into plain
locals; 16 could not take it and were given a `struct alignas(16)` with the
stack layout they had, and `tools/reframe.py` converted the other 8 after one of
them — `alt_model_p1_decode`, which no test image reached — segfaulted the first
time an image did. Each frame carries a reference alias per variable so the
bodies could keep their `vNN` names:

```c
  struct alignas(16) {   // 208 bytes, the frame Hex-Rays could not name
      uint8_t slot0[4];
      uint8_t slot4[4];
      uint8_t slot8[4];
      __m128 *v275;
      int16_t *v276;
      ...
  } __frame;
  static_assert(sizeof(void *) != 4 || sizeof(__frame) == 208, "frame layout moved");
  ...
  int32_t  &v246 = *(int32_t *)((char *)__frame.slot0);
  int16_t *&v247 = *(int16_t **)((char *)__frame.slot0);
  int32_t  &v248 = *(int32_t *)((char *)__frame.slot0);
```

That was the right move at the time: it made the layout checkable while the
types were still wrong. The types are right now, and what is left is 564 aliases
onto storage nothing needs to be contiguous.

### 4.2 Three kinds, and only one is work

**Three frames dissolve outright** — and this paragraph said nine, which is the
most useful thing the implementation found. The shared-slot and run-site tests
are not the whole of what keeps a frame one object. Two more things do, and the
gate found both:

* **An array member reaches its neighbours without naming them.**
  `model_planes` passes all three tests and dies on five of the eight images:
  its frame holds `uint16_t p_i[2]` at +20 with `v49`, `v50`, `v51` at +24, +28
  and +32, and the body writes a four-word image descriptor through `p_i`.
  Lifting array members anyway, in the six frames that have them, was rejected
  by the gate in all six — `buf[4096]` really does sit in front of `v72[1024]`
  and the body really does walk one past the end.
* **An address that escapes does not stop at the member it came from.**
  `alt_p2_context` takes `&v275` and hands it to `alt_p2_filter` as an `Obj12
  *`; lifting the forty members after it segfaults every image but the
  one-plane one. So an address-taken member freezes the *whole* frame, not
  itself. Only a shared slot is local to its own member.

`alt_p2_model` (77 aliases), `transform_planes` (5) and `model_plane` (12 of
17) pass all four tests, and `tools/defram.py` lifts them: each alias becomes
the declaration it stood for, and a `uint8_t _gapN[width]` takes the member's
place so that every surviving member keeps its offset and the `static_assert`s
still assert what they did.

The other 21 frames are pinned, and unpinning them is the third category below
rather than a separate problem. Declaring the runs freed four more frames —
`alt_model_p2_encode`, `alt_model_p2_decode`, `alt_model_p1_encode` and all but
the shared slots of `alt_model_p1_decode` — for 206 of the 602 aliases so far.

Three of those frames are large — 41 456, 32 824 and 26 712 bytes — but that is
because they hold real workspace arrays, which stay as arrays. Each has an
`alloca` beside it (`v3 = alloca(41424);` in `choose_plane_coding`, 32 788 and
26 672 in the other two) that is Hex-Rays' record of the frame's size; all four
`alloca` sites in the file assign to a local mentioned nowhere else, so they go
with the struct. (`reduce_alphabet`'s 66 064-byte frame, the largest in the
file, is *not* in this group: it has six run sites.)

**27 slots carry more than one name — 62 extra names, in 13 functions.** These
are MSVC's register reuse showing through, and they are not all the same
problem. Three slots in `alt_p2_context` carry 29 names between them and are
plain slot reuse across disjoint live ranges: ten `vNN` on `slot0`, twelve on
`slot4`, seven on `slot8`, and 23 of the 29 are `int32_t`. Those split
mechanically into separate locals. The hard ones are where the *types* differ:

```c
    char     &Buffer  = *((char *)&__frame.Buffera);   // write_bmp
    uint32_t &Buffera = __frame.Buffera;
    char *   &Bufferb = *(char * *)((char *)&__frame.Buffera);
```

— one slot holding a byte, a `width|height` dword and a copy buffer. Round two
split half a dozen of these by hand; `symbol_list_update`'s `n251` (a list base
*and* a count byte) is the worked example in REFACTORING2.md §4.2. Each is a
decision about which name means what, and the frame cannot go until it is made.

**24 sites walk a run of members as an array**, over 12 bases in 6 functions:

```c
      *(&lpAddress + n4++) = (Obj11 *)v8;
      v24 = (Obj1 *)*(&v92 + n4_2);
```

Those runs *are* arrays — `Obj11 *plane[4]` and so on — and declaring them that
way removes the site and the cast together. This is `arrayify.py`'s operation
applied to a frame instead of a struct, and it is also §2.2's evidence, so doing
it names the element type at the same time.

`tools/runarray.py` does it, and six of the twelve bases are done: both p2
plane arrays and both p1 ones. Two things it had to learn:

* **The run's element type comes from the aliases, not the members.** Hex-Rays
  declared the four p2 plane pointers as one `Obj11 *` and three `__m128 *`,
  and then aliased all four as `Obj11 *`.
* **The walk does not always start at the array.** The p1 fill loop walks from
  `Block` and the read loop walks from `v92`, the member *before* it,
  pre-incrementing from zero — two origins one element apart, which is the same
  shape as the plane-descriptor table's record 0 and the level table's base.
  Whichever variable the walk started from is what Hex-Rays recovered, and that
  is not always the array.

What it leaves behind is §2.2's argument in plain code:

```c
    v101 = alt_p2_context((__m128 *)plane[0], v2, v3, plane[2], plane[1]);
    v112 = alt_p2_context(v111,               v2, v3, plane[0], plane[2]);
    v121 = alt_p2_context(v120,               v2, v3, plane[0], plane[1]);
```

The six that are left are in `reduce_alphabet` and `unmodel_plane_slow`, and
`unmodel_plane_slow`'s four are one array reached from four bases at once.

### 4.3 Why this is worth doing

The frames are the reason a reader cannot tell a variable from a stack offset.
564 aliases is 564 places where the declaration says `__frame.x` instead of a
type. The 24 `sizeof(__frame)` assertions are load-bearing while the frames
exist — that is exactly §6's point — but they are guarding a layout that only
has to hold because the frame is one object, so nine of them stop being needed
the moment the frame they guard dissolves. And the 27 double-booked slots are
the only real bugs hiding in the file's shape: a slot with two meanings is one
misread away from a wrong one.

---

## 5. Order, and what depends on what

```
  Phase A  workspaces -> structs     merges first, then alt_p2_alloc,
     │                               then the fNN names
     ▼
  Phase B  bmf_bss -> tables         C, E, D, B, A; each move is a
     │                               reader read
     ▼
  Phase C  frames -> locals          nine dissolve, 12 runs become arrays,
                                     27 slots get split by reading
```

A before B because §3.1A's table is indexed by a plane number that lives in a
Phase A field. `rc_begin_encode` has the two in one expression:

```c
  *(uint32_t *)(_this + 278732) =
      (uint8_t)(__byte_44339E[16 * (uint8_t)__byte_44339D[16 * *(uint32_t *)(_this + 278728)]] & 8) >> 3;
```

— a workspace field at +278728 selects a descriptor record, a byte of that
record selects another, and the result goes back into the workspace at +278732.
Neither half is readable until both are declared, and Phase A is the half that
makes the subscripts mean something.

C is independent of both and can be interleaved — with one exception that argues
for doing part of it first: the run sites in §4.2 are where §2.2's `Obj11` array
lives, so declaring that run as `Obj11 *[]` is both the Phase C change and the
Phase A merge's first use site. The other eight clean frames are mechanical and
can go in at any point.

Within each phase: **one object, one global, one frame at a time, gated.** Round
two's evidence for that is in §6.

---

## 6. What would make this fail

Round two ended with three things that would have moved a stream if the gate had
not been run, and all three are in this round's path.

* **Pointer arithmetic scales.** Typing `Obj10::f1078208` as `uint32_t *`
  compiles, passes all fifteen images, and **segfaults the out-of-memory
  ladder**, because `f1078208 + 24 * n` steps 24 bytes on a byte pointer and 96
  on a `uint32_t *`. Every one of Phase A's members and Phase B's tables has
  this hazard. When a field is a cursor, the byte pointer is almost always the
  right answer; when it is not, the arithmetic around it says so.

* **Two names for one field can disagree.** `ModelBlock::f6059436` and
  `Obj10::f6059436` are the same bytes and were declared `uint8_t *` and
  `uint16_t *`; making the first match the second moves three streams. Phase A's
  merges will surface more of these — five are already visible in §2.2's two
  tables, three in the plane workspace (+278528, +278736, +278760) and two
  between `Obj10` and `ModelBlock` — and the merge is the moment to resolve
  them by reading, not to pick one.

* **A retype can be right and still be wrong at the use site.**
  `tools/retype_locals.py` offers every local whose conversions agree on one
  type — 62 of them — and applying all 62 leaves the file not compiling at all
  (192 errors); applying them one at a time and keeping only what compiles keeps
  *one*. The worklist it prints is useful. The rewrite it offers is not.

And two that are specific to this round:

* **Dissolving a frame changes the stack layout.** Nine of them are safe
  because nothing addresses across their members; the other fifteen have a
  reason to be one object, and the `static_assert` is what has been saying so.
  Delete the assert only when the struct goes with it.

* **Chasing the raw-offset count.** 1930 is not a target. An offset that stays
  because its object genuinely is a byte buffer — `exclusion_mask[symbol]`,
  `memset((char *)model_geometry + v6, 7, 64)` — is not the same defect as
  `*(uint32_t *)(_this + 278728)`. The metric is whether a reader can tell what
  the object is, and no counter measures that; the counts here are for noticing
  movement, not for finishing. Round two's §4.2 has the standing example: 61
  casts could have been added to make its scoreboard fall 243 → 179 with no type
  decided, and were not.

---

## 7. What the gate needs

Nothing new, and that is worth stating because it is not obvious. 15 images with
committed reference streams, a two-member archive, 15 refused inputs and the
out-of-memory ladder cover every path this round touches:

* Phase A's structs are read by every image. `REFACTORING2.md` §5 measured it:
  `alt_p2_model` 1591/1591 lines, `alt_p2_context` 817/817 and `alt_p2_filter`
  129/129 are 100 % covered; `choose_plane_coding` is 622/656 — 94.8 % — and
  none of its unreached lines is one of these.
* Phase B's globals include `plane_count` (154 uses) and the near-lossless
  threshold pair (125 and 123), which nothing can move without moving a stream.
* Phase C's nine clean frames are in the hottest functions in the file.

The one check that earned its place in round two was the out-of-memory ladder,
which caught a scaling error the fifteen images did not. **Run the whole gate,
every batch.** `BMF_MALFORMED=0 BMF_OOM=0` exists for iteration, not for
deciding.

`BMF_STRICT=1 ./build.sh` reports zero today and will keep doing so; the
companion scoreboard for this round is `python3 tools/shape.py`, which prints
§1's table. Neither replaces the gate — a count can improve while a stream
moves, which is the whole content of §6.

---

## Appendix A — how the numbers were measured

Everything in §1 comes from `python3 tools/shape.py`, in this tree, and the
sub-commands print the per-item detail behind §2, §3 and §4:

| number | command |
| --- | --- |
| the §1 table | `python3 tools/shape.py` |
| raw offsets off `_this`, by function | `python3 tools/shape.py --this` |
| one line per frame | `python3 tools/shape.py --frames` |
| `bmf_bss` globals, groups, subscript shapes | `python3 tools/shape.py --bss` |
| struct overlap | `python3 tools/shape.py --overlap Obj8 Obj11 Obj19 Obj31 Obj69` |
| conversions | `BMF_STRICT=1 ./build.sh` (0 today) |
| coverage | `BMF_STATIC=0 BMF_GC=0 BMF_OUT=bmf.cov ./build.sh --coverage -O0`, run `test.sh ./bmf.cov`, `gcov -f bmf.cpp` |

A **raw-offset site** is one of three textual shapes, all of which mean "an
object's layout written as arithmetic":

```
   (T *)(base + …)            and  (T *)((char *)base + …)
   ((T *)base + …)                 — the same, with the offset scaled
   … + (char *)base           — Hex-Rays' reversed form, where the index came first
```

`base` must be a plain identifier, so `(char *)v->f4 + n` is not counted; adding
member expressions to the pattern raises the total by about 60. Comments are
stripped before matching. The regexes are `P1`, `P2` and `P3` at the top of
`tools/shape.py`.

`REFACTORING2.md`'s Appendix A has the rest, and its Appendix B is the standing
warning: every count in a plan should be reproducible by a command that does not
go through the tool being checked. Round two's first draft failed that in three
places. This one is written against `tools/shape.py`, which is new and therefore
unproven; the *other* commands in this appendix — `grep`, `gcov`, the build —
are how to check it.
