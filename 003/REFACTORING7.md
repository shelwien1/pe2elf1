# BMF 2.01 — refactoring, round seven

Round six closed by saying its remaining list "is not a list of sweeps": every
tool in `tools/` reported zero, and what was left needed a reading of the model
rather than another pass. This round did that reading. All seven of the items
§7 named are done — including the one this document itself argued for leaving
alone, in an argument built on a layout that turned out to be the artefact.
Two things mattered more than any of them: a warning count one above its floor
caught a byte that had silently become a word, and an anomaly that had been
written down as a curiosity turned out to be the evidence.

---

## 1. Where the file is

`python3 tools/shape.py`, verbatim:

```
subs1.hpp / bmf.cpp lines          17692 / 358
raw-offset sites                   101
  off `_this`                      7, in 4 functions
pointer casts                      3021
globals still at a 1997 address    0
frames                             17, 169180 bytes, 0 aliases
  slots carrying two names         0, 0 extra names, in 0 functions
  member runs walked as arrays     0 sites, 0 bases, 0 functions
  frames that dissolve outright    17, 0 aliases
structs                            17, 0 still ObjN
  fNN members / named ones         92 / 79
distinct vNN locals                560
goto / LABEL_n:                    112 / 79
__fwd_* shims                        0
```

against round six's close:

| | round six | round seven |
| --- | --- | --- |
| lines | 17 833 / 358 | **17 692 / 358** |
| raw-offset sites | 504 | **101** |
| — off `_this` | 27 | **7** |
| pointer casts | 3986 | **3021** |
| structs | 16, 3 still `ObjN` | **17, 0 still `ObjN`** |
| conversion warnings | 2075 | **1994** |

`ObjN` is zero. Every recovered struct in the file has a name that says what it
is, which is the end of a thread round two opened with 77 of them.

The gate is unchanged and green at every commit: 15 reference streams
byte-identical, a lossless round trip, a two-member archive, 15 refused
malformed inputs, the `ulimit -v` ladder exiting 7, `BMF_STRICT` at 0 and the
`BMF_WARN` ratchet, which moved down nine times this round and up none.

---

## 2. The ratchet caught a defect the gate did not

`ModelBlock::f1078240` was a `uint8_t *` whose comment said "a row cursor". It
is the alphabet map: `expand_alphabet` builds it as whole words and
`unmodel_plane_slow` reads one word per pixel through fourteen sites, all
spelled `*(uint32_t *)(f1078240 + 4 * i)`.

Thirteen of them. The fourteenth had no cast:

```c
  *ArgList_8++ = *(this_4->f1078240 + 4 * *(uint16_t *)(this_4->f56[0] + 8 * v80++ + 64));
```

With the member a `uint8_t *`, that dereference loads **one byte** — the low
half of the low half — which is exactly right, because this branch is the one
guarded by `this_4->f8 == 8` and its output is one byte a pixel. Retyping the
member to `uint32_t *` widened it to a word without moving the offset.

**The gate stayed green.** Fifteen reference streams byte-identical, the round
trip, the archive, the malformed inputs, the OOM ladder — none of them reach an
8bpp plane through that branch. What noticed was `BMF_WARN`: 2000 conversion
warnings against a ceiling of 1999, one narrowing that had not been there
before.

Two things follow, and the second is the useful one.

* §7's fifth hazard — *a rewrite that keeps the offset can still change the
  width* — has a converse worth writing down: **the width can change without
  the offset moving at all**, and then nothing about the address arithmetic
  looks wrong. The only trace is the type of the value.
* A warning count is not a defect count, which §7 already said. It is also not
  *only* a style measure. Held at a floor, it is a second oracle that fires on
  a class of change the reference streams do not cover, and this round is the
  first time it caught something they missed.

---

## 3. The symbol list, in all five places that walk it

Round six established `SymEntry` — three bytes, `{uint16_t sym; uint8_t cnt;}`
— from `init_symbol_list`'s `bmf_new(3 * a3)`, and left it there. Five
functions walk those entries and none of them said so: the symbol was
`*(uint16_t *)p`, the count `p[2]`, the step `p += 3`.

Three of the five are the same code three times — find or insert, swap the
entry one place forward while it outweighs its predecessor, halve every count
when one passes 251:

| | header type | field spelling |
| --- | --- | --- |
| `symbol_list_update` | `SymList *` | `_this->live`, `_this->f12` |
| `encode_symbol_list` | `uint32_t *` | `*(_this + 1)`, `*(_this + 3)` |
| `decode_symbol_list` | `uint32_t *` | `a1[1]`, `a1[3]` |

The first column is what types the other two. Field for field, at the same
offsets, in the same order, they are one function; `symbol_list_update` already
carried the type because round five had a caller that gave it one.

The fourth walk is inlined into `init_model_tables` — the insert path only,
without the sort — and the fifth is in `unmodel_plane_slow`, packing 24-bit
codes into three-byte pairs. Both were found by looking for `p[2]` beside
`p += 3`, not by a tool.

`Obj32` turned out to be `SymEntry` under another name, which is how the
`ObjN` count reached zero.

---

## 4. Arrays of lists, and one list inside the object

`ModelBlock` held four `uint8_t *` and a `uint32_t **` that were one structure.

* `f1078208` and `f1078212` are `SymList *`. Fourteen `+ 24 * k` sites become
  `[k]`, and the allocators say what they allocate: `bmf_new(24 * n + 4)` is a
  count word followed by `n` lists, which is where `free_workspace`'s
  `*(v3 - 1)` reads `n` from.
* `f1078216` and `f1078220` are `sel[2]` — two adjacent members of one type
  that `f1078232` walks. That is why the reset was spelled `_this + 1078216`
  and why the end test compared a cursor against the same address.
* **A twenty-fourth list sits inside the object.** `init_model_tables`
  initialises `_this + 1078184` and stores its address in `f1078224`. That
  address lands two bytes into `_pad16`, and the `void *` that used to be
  `f1078204` is twenty bytes further on — this list's `ent`, which is why
  `free_workspace` freed it by itself. It is `SymList escape` now.

One allocator's zeroing loop was unrolled two lists at a time with a scalar
tail for the odd one; between them they cover 0 .. n-1, so it is one loop. The
register the unroll saved and restored keeps its guard rather than being
reasoned away.

---

## 5. Padding that was two tables and four record grids

Round six's §6 rule — *when a walk looks unbounded, the code that writes it
knows the bound* — paid out twice more.

**`_pad25`, 440 bytes.** Four readers and four writers reach it at +1078244 and
+1078308, and the writers give the bounds exactly:

* `ctx_state[64]` inverts `ctx_group_flags`: it stores `s` at
  `ctx_group_flags[s]` for the fifteen states, and the largest entry in that
  table is 63 — so 64 bytes, ending precisely where the next table starts.
* `ctx_bucket[375]` is [5][5][15]. The filling loops run `n5 < 5`,
  `v8 + 1 < 5` and `v5 + 1 < 15` and write at `v5 + 15 * v8 + 75 * n5`; the
  readers arrive with `state + n15_5`, where `n15_5` is `15 * a + 75 * b` built
  from two four-way neighbour comparisons. One writer stepped a base pointer by
  one per state instead of adding the index, which is the same address and is
  why it looked like a third spelling.

**Four three-word grids.** `f1051680`, `f1051776`, `f1076352` and `f1077894`
are all indexed `3 * k`, and a record is the (count, count, total) triple
`encode_context_bit` and `decode_context_bit` take a pointer to. Three of them
were `uint8_t[]` that nothing ever read a byte of. The site that hid the
arithmetic worst,

```c
  (uint16_t *)&((uint32_t *)this_3)[12 * n15_12 + 269089 + 6 * flags + 3 * v40] + 3 * X + 1
```

is record `8 * n15_12 + 4 * flags + 2 * v40 + X + 1` of the 257-record grid.
Every term in it was already a whole number of records; only the base was
written in a different unit.

---

## 6. Counters, strips and rotations

The p1 counter table deserves its own paragraph, because getting it wrong was
the most instructive thing in the round. `P1Count` put `total` at +8, and
`16 * 237 + 8` is 3800 — so `((P1Count *)_this)[k + 237].total` lands on the
right word for the wrong reason. It is the table at +3800 read half a record
out of phase, and the give-away was in the file the whole time: the old comment
noted that `w[]` is never read at those indices and `bin[]` never past 2, and
recorded it as a curiosity rather than as evidence.

`init_counter_node` settles it. `alt_p1_alloc` calls it at
`(uint16_t *)_this + 8 * k + 1900` — +3800, stride sixteen — for 0x99C60 values
of k, and the callee writes eight words: `[0] = 22` and `[1..7] = 8, 2, 2, 2,
2, 3, 3`, where 22 is the sum of the other seven. A node is a total and seven
counts. So `w[0..3]` at record `k + 237` are node k-1's *last* four counts,
which is exactly why nothing reads them.

`CounterNode counters[629856]` sits at +3800; the count is the allocator's own
loop bound and `0x99C60 * 16` fills the object to its last byte. The 251
subscripts re-base onto it, and the three the code touches at a time stop being
236, 237 and 238 and become -1, 0 and +1 — a node and its two neighbours, which
is what they always were. `P1Count` has no users left.

> **This is the round's own correction.** §8 of this document, written three
> commits earlier, argued that the grid should be left as arithmetic because a
> member here would have to start at +3792 and contradict the allocator. That
> argument was built on a layout that was itself the artefact. A reading that
> explains an anomaly away is worth less than one that explains it.

Three tables whose element size was in the reader's head rather than the
declaration.

* **`n2` in `alt_p2_model`** carried 62 raw offsets, and §7 said it needed
  `v510` proved even. `v510` and `v511` were proved last round; the one
  remaining variable offset is `v78`, and `v78 = n5 << 17` at its only
  assignment — even by construction, no probe needed. The constants are in
  `uint16_t` units off three cursors that are already `uint16_t *`, so
  `(uint8_t *)(n0xF0 + 470040)` was casting a `uint16_t *` sum to bytes in
  order to add byte offsets back on. Two reads are genuinely not `uint16_t`:
  `*(n2 ± 4)` fetches a shift count, four bytes away being the neighbouring
  record's first byte, and those stay explicit.
* **The two pixel coders' record** is sixteen bytes: eight words, of which the
  last is only ever touched as `+14` and `+15`. `FreqRec` declares both
  spellings on the same bytes, because the code uses both, and the two 64-bit
  moves that copy one become an assignment. The reload that made this look
  impossible — `__frame.sym1 = (int32_t)freq_tbl` and back twenty lines later
  — is a save and restore of the *same* pointer with nothing writing the slot
  in between, so it is one cursor after all.
* **`model_tables`** has said "handed out in 254-entry strips" since round
  three; twelve sites still spelled the multiply, with the strip count hidden
  inside constants that had to be divided first. `model_strip(k)` writes it
  once, and 32512, 4064, 2032 and 254 are 128, 16, 8 and 1 strips.
* **`p2_ctx_rotate`** is the same in miniature: nine sites reached its four
  `int32_t` through `*(int32_t *)((uint8_t *)p2_ctx_rotate + (ctx & 0xC))`. The
  comment on the table already said `(ctx >> 2) & 3`.

`cost_candidate` is the other direction — a number declared as an address. Its
three callers pass `(uint8_t *)nullptr`, `(uint8_t *)1` and `(uint8_t *)2` for
the candidate index, and the body does `(n2 + 1) % 3 - n2` with it. Typing it
`int32_t` unlocked the eighth parameter: `a8` is an array of four-word cost
records, one per candidate, and fourteen `*(uint32_t *)(a8 + 16 * n2 + 4)`
became `a8[4 * n2 + 1]`.

---

## 7. One tool, for a shape the last one missed

`unrec.py` folds the 18-byte record copy MSVC emitted as five load-stores in a
row. Where it had registers to spare it did not emit that: it issued some loads
early and interleaved them with the stores, so five statements became eight and
`unrec` saw none of them.

```c
    v22 = *(uint32_t *)(v21 - 10);
    v23 = *(uint32_t *)(v21 - 6);
    *(uint32_t *)(v21 + 18) = *(uint32_t *)(v21 - 18);
    *(uint32_t *)(v21 + 22) = *(uint32_t *)(v21 - 14);
    v24 = *(uint16_t *)(v21 - 2);
    *(uint32_t *)(v21 + 26) = v22;
    *(uint32_t *)(v21 + 30) = v23;
    *(uint16_t *)(v21 + 34) = v24;
```

The five stores cover +18 .. +35 and their sources cover -18 .. -1:
`((P2Ctx *)v21)[1] = ((P2Ctx *)v21)[-1]`. Sixteen of them across the four p2
body functions, 80 stores.

`tools/unrechoist.py` requires both ends on a record boundary **and requires
them not to overlap** — the second is the only reason folding a block whose
loads and stores interleave is safe at all, since it makes the order of the
eight statements irrelevant. Only the stores are replaced; the loads stay,
because whether a hoisted value is still wanted downstream is `unwrite.py`'s
question. Two of them are, and they are still there.

---

## 8. What is left

* **Frame slots with two meanings.** `shape.py` reports 0 for "slots carrying
  two names" because these are one name with two *roles*, which it does not
  measure. Three of them are done: `compress_image`'s header slot is a
  `BmfImage` and then deinterleave scratch, in branches separated by a return;
  `expand_image`'s four words are a packer mask, then a header, then scratch,
  in three phases; and both are modelled as a union with one arm per role,
  which needs no liveness proof because it claims none. What is left is the
  harder kind — `cost_candidate`'s `v91` holds an address, then a scratch
  value, then a cost, and `alt_p2_model`'s `v508` is a pointer and a strip
  index within one expression. Those do need a liveness argument each, and
  there is no tool shape for it.
* **The bucket record, and what it says about two other members.** Sixteen
  bytes per context bucket at `ModelBlock + 96 + 16 * bucket`: five counts,
  their total, a scaled weight, and two bytes — a level and the `1 << (5 -
  level)` it derives. The grid is anchored at the object and the bucket counter
  starts at zero, so **record 0 is +96 .. +111** — which `f56[10..13]` also
  claims, and which the `f1051664[k] = f56[10 + k]` copy immediately after the
  loop reads back. That copy is either four row cursors or record 0's four
  dwords and it cannot be both, so one of `f56`'s length and `f1051664`'s type
  is wrong. **That copy is dead** — `f1051664` is written in two places and
  read nowhere, in this file or in `bmf.cpp` — so the collision costs nothing
  at run time and the gate will never speak to it. The same sixteen-byte grid
  also carries `FreqRec`, at record 188 (+3104), so whatever the region from
  +96 is, the bucket walker and the pixel coders share it. The finding is in
  the file as a comment rather than as a member: after §6, an overlap that
  looks like a coincidence is precisely the thing not to declare around until
  it is settled, and settling this one needs run-time evidence rather than
  another reading.
* **101 raw offsets, 92 `fNN` members, 560 `vNN` locals, 112 `goto`s.** The
  offsets are down from 1389 over five rounds; what remains is in bases that
  are genuinely computed — a name plus a variable byte offset, with nothing
  either end to say what the stride is. `degoto.py` still reports 0 candidates.
* **1994 conversion warnings** — 1256 `-Wsign-conversion`, 635 `-Wconversion`,
  99 `-Wsign-compare`, 10 `-Wint-to-pointer-cast`, 4 `-Wuseless-cast` and one
  `-Wmain`. §2 is the reason to keep holding it there.

---

## 9. Tools this round added

| tool | what it does |
| --- | --- |
| `unrechoist.py` | an 18-byte record copy whose loads MSVC hoisted above its stores |
| `unrecast.py --bare` | the cast with no dereference behind it, to a fixed point |

One new tool and one existing one taught a shape it had been walking past.
That is the honest measure of where this round's work was: `unrec`,
`uncopyrec`, `unp2`, `unscalar`, `unshim`, `unindex`, `unrecast`, `unwiden`,
`unoffset`, `uncast`, `unused`, `unwrite`, `unhoist`, `uncursor`, `dedup`,
`arrayify` and `degoto` are all run against the file at the end of this round
and all of them report zero, exactly as they did at the end of round six — and
the file still lost 403 raw offsets and 965 pointer casts in between.

`unrecast`'s addition is worth one line of its own. It dropped
`*((uint32_t *)p + 1)` when `p` was already a `uint32_t *` but never looked at
the bare `(uint16_t *)p + 2`, and there were 136 of those. They arrive stacked
— `(uint16_t *)(uint16_t *)n0xF0` — so removing the inner one leaves the outer
one matching a pattern it did not match a moment earlier. It repeats until it
stops changing now, which is the difference between a report of zero meaning
*none left* and meaning *none this pass*.
