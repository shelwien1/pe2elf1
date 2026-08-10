# BMF 2.01 — refactoring, round six

Round five closed with four things it had deferred and a list of what a round
six would start from. Both lists are done. This document is what round six did,
what the file measures now, and what is left — which for the first time in six
rounds is not a list of sweeps.

---

## 1. Where the file is

`python3 tools/shape.py`, verbatim:

```
subs1.hpp / bmf.cpp lines          17893 / 358
raw-offset sites                    648
  off `_this`                        40, in 9 functions
pointer casts                      4171
globals still at a 1997 address       0
frames                               17, 169180 bytes, 0 aliases
  slots carrying two names            0, 0 extra names, in 0 functions
  member runs walked as arrays        0 sites, 0 bases, 0 functions
structs                              16, 3 still ObjN
  fNN members / named ones           94 / 64
distinct vNN locals                 560
goto / LABEL_n:                     112 / 79
__fwd_* shims                         0
```

against round five's close:

| | round five | round six |
| --- | --- | --- |
| lines | 18 090 / 369 | **17 893 / 358** |
| raw-offset sites | 1089 | **648** |
| — off `_this` | 141 | **40** |
| pointer casts | 4700 | **4171** |
| structs | 17, 4 still `ObjN` | **16, 3 still `ObjN`** |
| `__fwd_*` shims | 94 | **0** |
| conversion warnings | 2160 | **2158** |

The gate is unchanged and green at every commit: 15 reference streams
byte-identical, a lossless round trip, a two-member archive, 15 refused
malformed inputs, the `ulimit -v` ladder exiting 7, `BMF_STRICT` at 0 and the
`BMF_WARN` ratchet at its floor.

---

## 2. The p2 model has a record type

The largest thing this round found was already written down and unused.
`algorithm_v2.md` §9 established the p2 family's working memory from the 56
places that copy one entry — **18 bytes a record, rows 144 bytes apart, eight
records to a row** — and then nothing in the source said so.

`alt_p2_context` read that table through **ten `ObjN` structs** whose members
sit at +0, +4, +18, +36, +38, +54 and +72: records 0 to 4, lanes 0 to 2.

```c
  v28->p2_row[4][1] = ... + v81->f36 - v81->f0;
  v28->p2_row[4][1] = ... + v81[2].lane[0] - v81->lane[0];
```

`struct P2Ctx { int16_t lane[9]; }` replaced all ten, and then reached three
more populations that were the same table under other spellings:

* **91 subscripts on `int16_t *` locals** — `v45[27]`, `*(v45 - 17)`,
  `v295[11]`, `v7[33]`. Every one divides into a record and a lane, which is
  the evidence: `27 = 3 × 9`, `-17 = -2 × 9 + 1`, `33 = 3 × 9 + 6`. Ten cursors
  qualified, each with a subscript of nine or more, so the stride is
  demonstrated per cursor rather than assumed from the table. `v282` and `v284`
  were left alone — their subscripts are 0 to 3, which is *consistent* with the
  record and does not show it.
* **23 unrolled copies** still written as four dwords and a word.
* **56 `bmf_copy(dst, src, 18)` calls** that round three folded before there was
  a type for what they moved.

The last two both become `((P2Ctx *)p)[i] = ((P2Ctx *)p)[j]`, and `bmf_copy`
went with its last caller — along with the `BMF_COPY_CHECK` build mode that
checked at run time what `sizeof` now answers.

> **What the fold asserts is only the reading.** Every rewrite is
> offset-for-offset: if eighteen were the wrong record size the code would still
> be correct, just oddly spelled. The lanes are numbered rather than named
> because what they hold is the open question `algorithm_v2.md` §9 leaves.

`alt_p1_model`'s table got the same treatment in round five (`P1Count`, sixteen
bytes), and this round finished it: 36 more cursors say
`&((P1Count *)_this)[idx]` instead of `(P1Count *)((uint16_t *)((uint8_t *)_this
+ 16 * idx))`, where the `(uint16_t *)` in the middle was the width of the first
field left over from before there was a record.

---

## 3. Fifty-one structs were not objects

Round five ended with 44 `ObjN`. Three are left, and only one round of reading
took them out:

* **Ten were one scalar.** `struct Obj13 { uint16_t f0; };` is `uint16_t *`, and
  `p->f0` is `*p`.
* **Seventeen were a run of one scalar.** `struct Obj9 { uint16_t f0;
  uint8_t _pad1[4]; uint16_t f6; uint8_t _pad3[4]; uint16_t f12; }` is
  `uint16_t *` read at `[0]`, `[3]` and `[6]` — the subscript is what the byte
  offset was hiding. A struct qualifies when every member is the *same* scalar
  and every offset is a multiple of its width; two different scalars means a
  layout, and that is `merge.py`'s job.
* **Ten were the p2 record**, §2.
* **`Obj97` was `BmfImage`** — a fourth recovery of the image header nobody
  merged. `compress_image` passes a `BmfImage *` to it, and `f0`/`f4`/`f12` are
  `width`, the low half of `stride`, and `data_size`.
* **Three got names rather than deletion.** `Obj0` and `Obj11` are what the p1
  and p2 sides allocate and pass around — `AltP1Block` and `AltP2Block`, 181
  uses between them — and `Obj12` is `alt_p2_filter`'s six weight blocks,
  `P2Weights`.

Renaming the two big ones surfaced **233 lines of orphaned header comment**:
rounds two and three merged several recoveries of one object into the first one
named, and `merge.py` deleted the struct bodies but left the `// ObjN --
recovered from N dereferences` blocks above them.

---

## 4. Two things that were only ever noise

**94 `__fwd_*` shims.** Round one gave every cross-function call a `static
inline` that took `void *` and cast on the way in, because the extraction did
not agree with itself about argument types. Both sides say the same thing now:

```c
  __fwd_bmf_destroy_archive_bmf_close_archive(Block);
  __bmf_close_archive((BmfArc *)Block);
```

211 call sites, and `uncast.py` then took 77 of the casts the compiler called
useless. One thing was worth the compiler catching: the substitution has to be a
*single* pass over the shim's body, because an argument expression can contain
another parameter's name — `a1[1]` is the caller's `a1` inside the value
substituted for `a0`.

**133 casts that said what the declaration says.** Hex-Rays casts before every
scaled dereference whether or not the cast carries information, and after five
rounds of retyping most carry none: `*((uint32_t *)_this + 1)` where `_this` is
a `uint32_t *` is `_this[1]`.

---

## 5. Cursors typed by what they read

Three shapes, all of them the same mistake in a different place — a declaration
that describes how a pointer was *produced* rather than how it is *used*.

* **`unwiden.py`**: `uint32_t *v13` whose every dereference is `*((uint16_t
  *)v13 + k)`. Five cursors, 111 casts became subscripts. Only a *narrowing* is
  taken — widening a cursor makes an index step further than it did, which is
  §7's first hazard rather than a spelling change.
* **`unindex.py`** gained `*(T *)((uint8_t *)p + N * v + K)`, where `K` places
  the member and `N` converts `v` into that member's elements, and only converts
  when `N` is a whole number of them.
* **62 scaled indices** of the shape `((uint32_t *)v385)[2 * n0x10 + 235020]`,
  where the constant is a member's offset divided by the access width. The
  variable part has to be rescaled into the member's elements — `n0x10` counts
  `uint32_t` on the left and `uint16_t` on the right, so the 2 becomes a 4 — and
  that factor is the whole risk.

---

## 6. Two walked extents that were declared as padding

§5.3 of round five found frame members whose declared size was four bytes and
whose walk covered kilobytes. The same thing is true of structs.

`ModelBlock::_pad16` is 26 524 bytes, and `layout_workspace` writes into it
twenty times. Its own writes give the boundaries: sixteen three-word records at
+1 051 680, a 24 KiB block it `memset`s, one record and 1536 bytes it `memset`s
at +1 076 352, and 48 more records at +1 077 894 — 96 + 24 576 + 1542 + 288 =
26 502, with 22 bytes left before the next declared member. Every record is
(40, 16, 512) or (4, 4, 72): two counts and a total.

`AltP2Block`'s allocator was the same story in miniature: all twenty of its raw
offsets are members with a loop variable in the index, and MSVC unrolled every
loop two elements at a time, which is why the constants come in pairs eight or
sixteen bytes apart.

> **The rule that keeps holding:** when a walk looks unbounded, the code that
> writes it knows the bound. Six rounds in, the padding is where the remaining
> findings live.

---

## 7. What is left

Nothing on this list is a sweep. Each one needs a reading of the model, which is
`algorithm_v2.md`'s open work rather than this document's.

* **`AltP1Block` stops at 216 bytes** and the code reaches +984, +1240, +1496
  and +3784 through it. Those are the p1 model's tables, and giving them members
  is §6's operation with the extents still to be read. 21 of the remaining raw
  offsets are one function's byte-indexing of this object.
* **`alt_p2_model`'s `n2`, `p2_rec` and `n0xF0`** carry 74 raw offsets between
  them, all with a *variable* byte offset — `*(uint16_t *)(n2 + v510 + 2)`.
  Turning those into subscripts needs `v510` proved even, which is a reading of
  where it comes from.
* **Three `ObjN` remain**: `Obj99` is `code_pixel`'s five-tap neighbour at an
  8-byte stride, `Obj130` a row-cursor view in `alt_p2_d8_decode_body`, `Obj32`
  a two-field view in the alphabet reduction. Each is a record like `P2Ctx` and
  each needs its stride established the way §2 established eighteen.
* **94 `fNN` members** still carry their offset for a name, and **560 `vNN`
  locals** still carry Hex-Rays' numbering. Both are answerable only by knowing
  what the values mean.
* **112 `goto`s over 79 labels.** `degoto.py` reports 0 candidates: none is a
  loop `continue`, a `break`, an early `return` or a shared error tail, and the
  one rewritable shape is exhausted. What is left is genuine irreducible flow.
* **2158 conversion warnings.** §2.5 of round five said this is a documented
  floor rather than a target, and the floor is now 1410 `-Wsign-conversion`,
  641 `-Wconversion`, 103 `-Wsign-compare`, 12 `-Wint-to-pointer-cast`, 4
  `-Wuseless-cast` and one `-Wmain`. The 12 int-to-pointer are the 32-bit
  addresses the model stores in `int32_t` members; the 4 useless casts are
  load-bearing at another expansion of the same macro.

---

## 8. Tools this round added

| tool | what it does |
| --- | --- |
| `unp2.py --ctx` | the 18-byte record fold, reusing §3.1's per-use offset reading |
| `unrec.py` | five unrolled stores that are one 18-byte move |
| `uncopyrec.py` | `bmf_copy(d, s, 18)` -> a record assignment |
| `unscalar.py` | a struct that is a run of one scalar -> a pointer to it |
| `unshim.py` | inline a `__fwd_*` and call the function it calls |
| `unindex.py` | `((T *)p)[expr + K]` and `*(T *)((uint8_t *)p + N * v + K)` -> a member |
| `unrecast.py` | drop the cast when the base already has that type |
| `unwiden.py` | give a cursor the type its dereferences read |
| `unspill.py` | a frame's spill area -> one member per slot (round five) |

Every one of them takes `--list` first and writes nothing until asked, and every
one was run a function or a struct at a time against the gate.
