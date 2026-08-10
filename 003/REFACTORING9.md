# BMF 2.01 — refactoring, round nine

Round eight closed with the tools all reporting zero and a note that what was
left "is no longer refactoring". That was wrong, and the reason it was wrong is
this round's only real idea: **every tool in `tools/` looks for a shape, and a
pointer whose declared element size is a lie has no shape.** It compiles, it
runs, it produces byte-identical streams, and it spells a record as a number.

Three arrays carried that lie. `ModelBlock::f56` was ten `uint8_t *` over
eight-byte records. `AltP2Block::cursor` and `::buf` were five each over
eighteen-byte records. Between them they accounted for most of the file's
remaining raw offsets, all of the "cursors whose subscripts share a factor",
and — this is the part worth keeping — a description of the pixel model's
neighbourhood that had been sitting in plain sight as the constants 27, 19, 11
and 3.

---

## 1. Where the file is

`python3 tools/shape.py`, verbatim, with round eight's numbers beside it:

```
                                   round 8   round 9
subs1.hpp / bmf.cpp lines            17787     17842
raw-offset sites                        22        13
byte offsets on a typed base           121         0
pointer casts                         2137      1758
fNN members / named ones             93/121    44/121
distinct vNN locals                    554       553
goto / LABEL_n:                     112/79    112/79
conversion warnings (ratchet)         1455      1403
```

Every tool reports zero: `unused.py`, `unwrite.py`, `unaliasvar.py`,
`uncursor.py`, `unoffset.py`, `unindex.py`, `unlane.py`, `unrec.py`,
`decast.py`, `unslot.py`, `unstruct.py`, `untable.py`, `unwiden.py`,
`unspill.py`, `unscalar.py`, `unshim.py`, `unhoist.py`, `uncopy.py`,
`unmemcast.py`, `unalias.py`, `defram.py`, `unframe.py`, `dedup.py`,
`arrayify.py`, `runarray.py`. So did they at the end of round eight, with 379
more pointer casts and 121 more byte offsets in the file. That is the whole
argument for §2.

---

## 2. The lens: a stride that does not match the type

The scan is eleven lines. For every pointer local, collect its constant
subscripts, take their gcd, multiply by the declared element size, and print
anything where the product is not the element size:

```
decode_pixel        v106  uint16_t * gcd 4   7 subscripts => 8-byte element
code_pixel          v111  uint16_t * gcd 4   7 subscripts => 8-byte element
decode_pixel        v76   uint8_t *  gcd 2   5 subscripts => 2-byte element
alt_p2_context      v86   int16_t *  gcd 18  3 subscripts => 36-byte element
alt_p2_context      v73   int16_t *  gcd 9   3 subscripts => 18-byte element
```

Nine cursors, and every one of them was a record cursor with the record spelled
out. It reports zero now.

The gcd is what makes it a measurement rather than a guess: `v76[26]`,
`v76[18]`, `v76[10]`, `v76[2]` and `v76[34]` share a factor of two, which says
the element is at least two bytes, and *all five being congruent to 2 modulo 8*
says the element is eight and the field is at byte 2. Neither fact requires
knowing what the record is for.

The lens does not find everything. `f56` itself is an array of pointers, not a
pointer, so no local's subscripts are involved; the `+ 56`, `8 * n`, `- 16` and
`+ 64` on it had to be read. And an address kept in a `uintptr_t` — §6 — has no
element size at all to be wrong about.

---

## 3. `ModelBlock::f56` is ten `PixRec *`

`PixRec` was already declared and already known to be eight bytes: a symbol at
+0 and six "matches this neighbour" flags at +2..+7. What was not known is that
the ten row pointers are the only thing that ever reaches it, and that they are
never anything else.

`layout_workspace` allocates `8 * width + 128` per row buffer and sets
`f56[j + 5] = f56[j] + 64`, so each buffer is the width plus sixteen records
with eight of left margin, and each cursor starts eight records in. Then it
seeds every record — including both margins — to "matches all six neighbours",
which is why a read off either end of a row contributes one rather than
whatever `bmf_new` left there. The alternate models do the same thing with
their own record (§4) and it is worth stating as a shared idiom: **BMF pads its
rows and initialises the padding to the value that makes the edge behave like
the interior.**

With the array typed, the constants stop being constants:

| was | is |
| --- | --- |
| `*(uint16_t *)(f56[5] + 8 * n15_18 - 8)` | `f56[5][n15_18 - 1].sym` |
| `f56[6] + 8 * n15_18 - 8 * n15_14` | `f56[6] + n15_18 - n15_14` |
| `*(uint16_t *)(f56[0] + 8 * v65 + 64)` | `f56[0][v65 + 8].sym` |
| `v49[26] + v49[18] + v49[10] + v49[2] + v49[34] - 5` | five `match[0]`s of records 0..4 |
| `*(v28 + 34) - *(v28 - 30)` | `v28[4].match[0] - v28[-4].match[0]` |
| `*(v61 - 29) + *(v61 - 21) - 2` | `v61[-4].match[1] + v61[-3].match[1] - 2` |

The last three lines are the finding. `grad[0..3]` are four sliding windows
over the match flags, and the byte offsets say which flag and how wide:

* `grad[0]` and `grad[1]` sum `match[0]` — "same as the pixel above" — over
  eight records of `f56[6]` and `f56[7]`, the two rows above. The seed at a row
  start is five terms because the three to the left are off the row; the
  per-pixel update adds the record entering at +4 and drops the one leaving
  at −4.
* `grad[2]` sums `match[1]` — "same as the pixel to the left" — over four
  records of the current row.
* `grad[3]` sums `match[0]` over eight records of the current row.

And `match[2..5]`, which round eight recorded as having no established reader,
have five: they are read one and two records back and weighted 2, 4, 8, 16 and
32 into the context indices both coders build. Every flag in the record is
accounted for now.

One thing stayed as it was. Three of the eight loads through `f56[7]` are
`movsx` where five are `movzx`, so three terms keep an `(int8_t)` cast. Every
writer of those bytes is a comparison and `layout_workspace` seeds them to one,
so the sign cannot show; the cast stays because the instruction is what is
being transcribed.

---

## 4. `AltP2Block::cursor` and `::buf` are `P2Ctx *`

Same shape, larger. `alt_p2_alloc` takes five buffers of `18 * width + 234` —
`sizeof(P2Ctx)` times `width + 13` — and the row start sets every cursor to
`buffer + 144`, which is eight records of left margin with five of right.
`2 * (9 * i) + 144` is `i + 8`. `+= 18` is `++`. `- 18` is `- 1`.

The odd offsets resolve into the record's own fields, and the order they appear
in is itself the reading:

```
ctx_bias[0] += 32 * cursor[0][-1].lane[6];    // was  - 6
ctx_bias[1] += 32 * cursor[0][-1].lane[7];    // was  - 4
ctx_bias[2] += 32 * cursor[0][-1].lane[4];    // was  - 10
ctx_bias[3] += 32 * cursor[0][-1].lane[5];    // was  - 8
```

Four of the eight lanes of the record behind the cursor, each feeding its own
accumulator across the row. `- 20` and `- 2` are `sign` two records and one
record back — the three-way sign `alt_p2_model` writes — and they index
`ctx_w[2]` and `ctx_w[1]`, whose `w[3]` is exactly three wide. `+ 16` and
`+ 17` are `sign` and `mag` of the record being written.

315 sites, 83 locals, and one variable disappeared into the change: `n8` held
`8 * n15_11` as a byte offset, which in records is a run length, so it is
`run` now.

---

## 5. Two traps, one shape

Both were the same mistake and only one of them announced itself.

**`*(uint32_t *)(f56[5] + 4)`** was four *bytes* — `match[2]`, the second half
of a record copy — and after the retype it was four *records*. Two of the four
triage streams segfaulted immediately.

**`*(uint16_t *)(plane[1]->cursor[0] + 2) = v113`** was two *bytes* and became
two *records*. Nothing crashed. Fourteen of fifteen reference streams stayed
byte-identical and `x_ep` grew by three per cent. The sweep that respelled
every other reach could not see this one because its pattern matched
`x->y->cursor[k]` and not `plane[1]->cursor[k]` — a subscript in the middle of
the base expression, which is a shape the other 315 sites never had.

The second one is the one worth writing down, because finding it took a method
rather than a reading, and the method generalises:

1. **Hash the inputs, not the state.** The first attempt hashed a window of
   memory around the cursor at every `alt_p2_model` call and reported a
   divergence three million calls before the real one — the window included the
   record about to be overwritten, which is stale in both builds and stale
   differently. Hashing only what the call reads moved the first divergence
   3.5 million calls later, onto the actual fault.
2. **Bisect on the hash, then print the call.** Every 500 calls to a checkpoint
   line, `diff` the two traces, narrow, then print the arguments and the
   neighbourhood for the twenty calls around it. That gave one call where the
   record state was identical and the returned context index was 47 against 16.
3. **Walk back one function at a time.** The index is `(n3536_4 + v186 + 7) >> 4`
   and `n3536_4` is the filter's output over `p2_row`, so print `p2_row`. Its
   first four rows differed by exactly 496 in every one of sixteen entries,
   which is a single scalar added to all of them — `bias`, one line, one read.
4. **Log the writes to the field that differs.** `bias` is
   `cursor[0]->lane[1]` at a row start, which the row-start block zeroes. Log
   every write of that field in both builds and diff. The zero that never
   arrived was two records away.

Reverting was the alternative and it was close. What made it not the right call
is that the failure was *localised* — one stream, one field, a constant
difference — and a localised failure is evidence, not noise.

---

## 6. Addresses kept as integers

`v28 = (uintptr_t)(lpAddress->cursor[0])` is the same record copy as everywhere
else in the alternate p2 model, with the same three loads MSVC left in front of
it. But the address went into a `uintptr_t` rather than a pointer, so
`v28 - 14` is integer arithmetic, no scan for a pointer's byte offsets can see
it, and the stride lens has no element size to disagree with.

`unwrite.py` could not reach the loads either: `v29` is assigned twice, once
through `LOWORD`, and read never. Four of those are gone and the four addresses
are `P2Ctx *`.

The general form is worth stating: **an integer holding an address is invisible
to every tool here.** `tools/shape.py` counts raw-offset sites off `_this` and
casts, not arithmetic on an `int32_t` that happens to be a pointer. There is no
scan for it in `tools/` and this round did not write one; the four were found
by reading the residue after everything else had been typed.

---

## 7. `unwrite.py` could not see half the declarations

`int32_t i, v5, n4, n4_1, v15, v16, v31, v32, v33, v39,` has no semicolon, so
the whole-statement regex did not match it and none of the names on it existed
as far as the scan was concerned. Sixty-eight of MSVC's spill lists are
declared across two or more lines. Every local on them was invisible.

This is the same defect `unused.py` had in round eight, fixed there and not
looked for here. With the state carried down, thirty-nine write-only locals
appear, thirty of them one thing:

```
v29 = v24->cursor[0];
v30 = *(uint32_t *)(v29 - 14);
v31 = *(uint32_t *)(v29 - 10);
((P2Ctx *)v29)[1] = ((P2Ctx *)v29)[-1];
```

`v30` and `v31` have no readers. MSVC loaded the record into registers, decided
to copy it whole, and left the loads in. All thirty are in the four bodies that
shift eight records at a row start.

**A tool that reports zero is evidence only about the shapes it can see.** Both
of the last two rounds found that the same way: not by a new pattern, but by
noticing that a tool's *input* was narrower than the file.

---

## 8. What is left

* **`AltP1Block::cursor` and `::buf` are still `uint8_t *`**, though `P1Ctx` is
  declared and every reach through them is now a subscript rather than a cast.
  The record is established — the sample at +0 and `abs32(v56 - v54)`, the size
  of the prediction error, at +1, written one line apart in
  `alt_model_p1_decode` — and the row-start blocks are record copies:

  ```c
  P1Ctx *const here = (P1Ctx *)_this->cursor[0];
  here[0] = here[-1];  here[1] = here[-2];  ...  here[5] = here[-6];
  ```

  which is the row end mirrored into the right margin, the same idiom §3 and §4
  found in the other two models.

  Retyping the array itself was attempted twice and reverted twice. What stops
  it is not the record but the arithmetic: a two-byte stride makes `*p` and
  `a * b` the same three characters to a regex, and a substitution that turned
  `16 * v37` into `16 v37->sym` is a silent semantic change of exactly the class
  §5 spent an afternoon on. It needs a converter that parses expressions rather
  than matching them. What is left after this round is 41 subscripts of the form
  `cursor[k][±even]` and `[±odd]`, which are readable against the record above
  and no longer hidden inside a cast.
* ~~**`alt_p1_context` reaches its two neighbour blocks through `int32_t *`.**~~
  Done. Both parameters are `AltP1Block *`, `a2[49]` and `a2[50]` are
  `cursor[0]` and `cursor[1]`, `a2[2]` is `f8`, and fifteen raw offsets became
  `cursor[0][-1]`, `[-2]` and `[-4]` — which, against §8's record, is the error
  magnitude one pixel back and the samples one and two pixels back. It needed
  no stride change, because the cursor stays a byte cursor; only the *base*
  was mistyped.
* **49 byte offsets on an `AltP1Block` row cursor**, and eight more on a local
  that holds one. All are the p1 record: `cursor[0] - 2`, `- 4`, `- 6` are the
  samples one, two and three pixels back, `cursor[0] + 8` and `+ 10` are two
  records forward, and `*(uint16_t *)(cursor[0] - 8) = *(uint16_t *)(cursor[1] + 6)`
  is a whole record copied from the row above into the left margin — the same
  idiom `f56` and `AltP2Block::cursor` use, still spelled in bytes because §8's
  first bullet is not done.
* **112 gotos and 79 labels**, unchanged for four rounds, and `degoto.py`
  reports nothing reducible. What is left are MSVC's shared tails.
* **553 `vNN` locals and 44 `fNN` members.** Round eight said this is
  answerable only by knowing what the values mean; §3 and §4 are what that
  looks like when it works — thirteen `vNN` in `alt_p2_context` became `P2Ctx`
  cursors and their reads became field names, and none of it needed a new
  naming pass.
* **1403 conversion warnings.** The ratchet fell 52 this round and every step
  was a by-product: a `match[0]` read through a typed field does not need the
  cast that a raw byte read did.

---

## 9. Tools

No new tools. Two fixes and one scan that lives in this document rather than in
`tools/`, because it is eleven lines and its answer is now zero:

| tool | change |
| --- | --- |
| `unwrite.py` | carry the declaration state across comma-continued lines |
| `unaliasvar.py` | unchanged; it found one more fold after §4 |
| `triage.sh` | unchanged, and it paid for itself four times over |

The stride scan is in §2. It is not committed as a tool because a scan that
reports zero and cannot report anything else is a claim, and the claim belongs
in prose where it can be read alongside what it does not cover — arrays of
pointers, and addresses held as integers.
