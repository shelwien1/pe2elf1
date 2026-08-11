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

`python3 tools/shape.py`, verbatim, with the two rounds before it beside it:

```
                                   round 8   round 9   round 9 end
subs1.hpp lines                      17787     17616         17243
bmf.cpp lines                            —         —           365
raw-offset sites                        22        12             5
  off `_this`                            —         1             0
  in functions                           —         1             0
byte offsets on a typed base           121         0             0
pointer casts                         2137      1545          1270
  to a scalar                            —         —           541
  to a record                            —         —           371
  to a scalar, of an address             —         —           353
  to a record, of an address             —         —             5
fNN members / named ones             93/121     5/162         0/172
distinct unexplained locals            554       591             0
  bodies still carrying one              —   8 of 102     0 of 104
  uses                                    —      6302             0
locals named for a callee parameter      —         —             0
  declarations / bodies                   —         —           0/0
names Hex-Rays chose and nobody changed  —         —             0
  conventional ones kept / bodies joined  —         —         56/74
goto / LABEL_n:                     112/79     81/55         49/33
  restart a loop / exit N blocks         —         —         15/32
  sideways to a join / to neither        —         —           2/0
  jump into a block                      —         —             0
conversion warnings                   1455      1331          1065
```

`python3 tools/checktable.py` compares that table against `shape.py --rows`
and exits non-zero if any row disagrees. It was written because the table had
been wrong seven times: four because the measure behind a row was wrong — §10
and §12 and §21 are those — and three because somebody copied a number into it
and the file kept changing underneath. The first four needed thought. The
last three are a diff, and a document that says "verbatim" should be checked
against the tool rather than reread.

Making it checkable took two changes to `shape.py` and none to its numbers: a
`--rows` mode that prints the table tab-separated, because a long label leaves
one space before its value and a short one leaves twenty and no parser should
have to guess; and splitting the two rows whose value was two numbers and a
clause, because a row with one value is a row a checker can compare.

**Not one Hex-Rays name is left in either file.** Checked by running the
pattern over the comment-stripped text of `subs1.hpp` and `bmf.cpp`, not by
reading it off the row that is supposed to say so — because that row had to be
corrected three times before it did.

Four of those numbers were wrong when this section was first written, and the
correction is §10's subject. `shape.py` hardcoded `SRC = 'subs1.hpp'` and
ignored the path it was given, so the same file answered for every version it
was asked about; its `goto` count was `src.count('goto ')`, which counts the
comments that *explain* a jump no longer there; and its label count anchored at
column zero, which misses the labels that are indented. `degoto.py`'s headline
had the same defect.

`distinct vNN locals` was not wrong, it was useless: it counts spellings across
the whole file, so it cannot move until a name is gone from every body that uses
one, and naming every local in a function leaves it unchanged. The two rows
beneath it are the ones that measure that work — and §11 is what they measure.

It has since had to be corrected three more times, each for a different reason
and each found the same way — by noticing that the row said something I could
have satisfied without doing anything:

1. Naming a model body's intermediates `t0`, `t1`, `t2` in sequence is a small
   honest improvement — contiguous, and marked as that body's rather than
   Hex-Rays' — but it does not explain anything, and a row that counted only
   `vNN` would have read it as progress. It counts `[vt]NN` now, which moved
   the figure from "5 of 102 bodies, 5979 uses" to **8 of 102 and 6302**.
2. Hex-Rays does not only spell a name `vNN`. It also names a local after the
   first constant it sees stored in it, so `n1840_1`, `n0x7FFFFFFF_10` and `n2`
   are the same kind of non-name — **198 of them, in 28 bodies**, never
   counted. Adding them moved a row reading `448, 1 of 102` to `637, 28 of
   102`. That is the same lesson from the other side: the first correction was
   about a measure I could satisfy by renaming, this one about a measure that
   only ever went down because it could not see the rest.
3. The headline counted comments while the two rows under it stripped them, and
   this file quotes the old names on purpose — `// was int32_t v312` beside the
   padding that replaced it. So the row read **81 with every body clean**.
4. Hex-Rays' names also turn up behind a prefix. `rename.py` puts the function's
   name in front of a local that collides with a global, and `p_` goes on a
   pointer, so `__code_pixel_n0x2000`, `p_n15` and eleven `p_n15_N` are the same
   residue in a disguise `\bn0x…` cannot match — there is no word boundary
   before the `n`. **22 of them, in seven bodies, while the row read zero for
   the third time.** Found by the retyping driver picking one of them as a
   candidate, which is not a way of finding things anyone should rely on.

A figure a comment can move is not a measurement, and a figure that cannot move
is not one either. That is the whole of §10, and §12 is four more of it.

`tools/sweep.sh` runs every tool in the directory and prints what each still
finds; all of them report zero. It also checks that the file did not change
while being measured, which is not a formality — see §10.

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

* ~~**`AltP1Block::cursor` and `::buf`**~~ Done, on the third attempt, and the
  third attempt is the only one worth describing because the first two failed
  the same way and this one could not.

  The two that failed tried to rewrite the *dereferences* by regex, which needs
  the regex to decide whether `*` is a dereference or a multiplication. It
  cannot: `16 * v37` and `*v37` differ only in what precedes the star, and one
  bad substitution is a silent semantic change of exactly the class §5 spent an
  afternoon on. The third attempt gives that job away. Once the array is
  `P1Ctx *`, a `P1Ctx` cannot be dereferenced to a scalar, cannot be
  subscripted to a scalar and cannot be multiplied — so **every** site that is
  not pure record arithmetic becomes a compile error with the operator named in
  the message. Ninety-one of them, and not one had to be guessed at.

  What the compiler *cannot* catch is the arithmetic, because `p + 8` compiles
  either way and means eight times as much. So that half is enumerated first —
  forty-six distinct forms, every one constant or `2 * v`:

  ```
   9  X->cursor[0] += 2       21  X->cursor[0][1]      10  X->cursor[4][13]
   9  X->cursor[1] += 2       10  X->cursor[0][-7]     10  X->cursor[2][-3]
   ...                         5  X->cursor[0][-2]      1  X->buf[0][2 * v21]
  ```

  — converted in one pass, and then re-derived by a scan that asserts the list
  is empty. The scan found `v5 += 8`, eight bytes that had become eight
  records, *before* the gate did.

  That is the method, and it is the round's second one: **the compiler finds
  what changed meaning loudly; a scan you write yourself finds what changed
  meaning quietly.** §5's bisection is what you need when you have neither.

  What comes out is the p1 neighbourhood in the same vocabulary as the other
  two models — `cursor[0]->sym` and `->mag`, `cursor[0][-1].mag` for the error
  one pixel back, `cursor[4][6].mag` for six pixels forward on another plane —
  and the row start as six record copies mirroring the row end into the right
  margin. The file's three record types are 2, 8 and 18 bytes and none of them
  is spelled as a number anywhere.

* ~~**`alt_p1_context` reaches its two neighbour blocks through `int32_t *`.**~~
  Done. Both parameters are `AltP1Block *`, `a2[49]` and `a2[50]` are
  `cursor[0]` and `cursor[1]`, `a2[2]` is `f8`, and fifteen raw offsets became
  `cursor[0][-1].mag` and `[-1].sym`, `[-2].sym` — the error magnitude one
  pixel back and the samples one and two pixels back. This one needed no stride
  reasoning at all: only the *base* was mistyped, not the element.
* **Zero byte offsets on a typed base**, from 121 at the start of the round.
  Every pointer in the file whose target is a record is typed as that record,
  and the stride scan of §2 reports zero for the third time.
* **81 gotos and 55 labels**, from 112 and 79. Five rules did it, and each
  came from reading `degoto.py --why` rather than the file:

  | rule | tool | gotos |
  | --- | --- | --- |
  | a shared tail small enough to copy | `untail.py` | 9 |
  | a jump into a block is a disjunction | `unjump.py` | 10 |
  | a forward jump over a region | `degoto.py` | 1 |
  | an `if` whose arms agree is not a decision | `undup.py` | 6 |
  | an unconditional `goto` ending a block is an `else` | — | 1 |
  | one statement duplicated by hand | — | 1 |

  `undup.py`'s six are §10's, and the rule generalises past `goto`s: MSVC
  emitted its own aliasing proofs as control flow, so `colour_transform`,
  `interleave_plane` and `expand_image` each carried a copy loop written out
  twice behind a six-line test whose two arms were the same code. Three of the
  five were spelled as `if`/`else` and two with jumps into the kept copy.

  The `else` is the one worth reading, because it is the shape a decompiler
  produces most often and the one a tool is least able to claim. In
  `__init_model_tables` a block ended `goto LABEL_21;` and `LABEL_21` was the
  first line after the code that followed the block — so the block and that
  code are the two arms of an `if`, and the label was only ever the join. No
  condition is inverted and nothing moves; the `goto` is deleted and an `else`
  is written. What makes it hand work rather than a rule is that "the label is
  the join" has to be read, not matched.

  `untail.py` and `unjump.py` are the round's two new tools and both were wrong
  on their first run in a way the file has taught before: `untail` copied one
  function's tail onto another function's `goto`, because `LABEL_19` exists in
  seven bodies and the search was not scoped to one; and its first relaxation
  — allowing a braceless `if` inside the run — made three runs look terminated
  at `if ( fwrite(...) != n ) return 0;` when control carries straight on. A
  `return` ends a run only when it is unconditional.

  What is left is 42 labels and it is genuinely structural: 17 reached by more
  than one `goto`, 9 where the `goto` is not the whole of an `if`, 6 backward,
  5 that leave a block, 3 whose skipped region something else enters, and 2
  that enter one. Removing any of those needs a flag or a duplicated body large
  enough that the duplication is the cost.
* ~~**44 `fNN` members**~~ Five, and all five are recorded as having no
  readers. The other 39 were named, and the method for the ones whose name
  collided across structs -- `f8`, `f12` and `f16` exist in three of them -- is
  §8's again: rename the member in its *declaration* and let the compiler
  enumerate the uses, because `->f8` on a `SymList` stops compiling while
  `->f8` on a `ModelBlock` carries on.

  Three of the readings are worth more than the names. `ctx_id1/2/3` with their
  `_used` counters are a chain of *interned* context ids: a neighbourhood
  signature indexes a table of `0xFFFF` slots, an unclaimed slot takes the next
  id, and that id plus a few more bits is the key to the next level. A context
  is not a number the coder computes, it is one it hands out. `sym_rev` looked
  write-only until the six reads of `*((uint16_t *)block + k + 3029720)` turned
  out to be the member's own offset; it holds `k`'s low thirteen bits reversed,
  which is what spreads consecutive symbols across the counter array. And
  `CtrPair` was the wrong noun entirely -- `SymPair { last, prev }` is a
  two-entry move-to-front over symbols, seeded to a value no pixel can have.

* **`vNN` locals**, and this is the item round eight said was answerable only
  by knowing what the values mean. Two things turned out to be mechanical after
  all. Sixty-odd became record cursors in §3, §4 and §8 and their reads became
  field names, without a naming pass. And 171 were not values at all:

  ```c
  v17 = v16->cnt;                     n251_1->cnt = v16->cnt;
  n251_1->cnt = v17;          →       v16->cnt = n251_1->cnt;
  v20 = n251_1->cnt;
  v16->cnt = v20;
  ```

  `untemp.py` folds a local assigned once and read once, which is
  `unaliasvar.py`'s idea with a harder safety question — the right-hand side
  has inputs, and something between the two lines might change them. Its
  conditions are in its docstring and two of them were learned the hard way:
  precedence is not a property of the characters around the name, and a read
  deeper than its assignment is inside a loop the expression would be moved
  into. Four streams segfaulted for the first and the gate caught the second.

  What is left are the names whose *values* are unexplained, which is
  `algorithm_v2.md`'s work and not a sweep's.
* **1341 conversion warnings.** The ratchet fell 114 this round and most of it
  was a by-product: a `match[0]` read through a typed field does not need the
  cast that a raw byte read did. Seventeen came from the deleted aliasing proofs
  of §10, which existed to compare pointers against integers holding addresses,
  and the last four from `bmf.cpp` — where one of the four was not a defect at
  all, and taking the warning at its word cost a stream. §10 again.

---

## 9. Tools

Two new folding rules, `uncopy.py` and `unhoist.py`, and four fixes to guards
that had gone quiet — §12. Otherwise two fixes and one scan that lives in this
document rather than in `tools/`, because it is eleven lines and its answer is
now zero:

| tool | change |
| --- | --- |
| `unwrite.py` | carry the declaration state across comma-continued lines |
| `untail.py` | new: copy a shared tail small enough to be copied |
| `unjump.py` | new: a jump into a block is a disjunction |
| `untemp.py` | new: a local assigned once and read once is the expression |
| `namelocal.py` | new: a local that is one assignment of a member takes its name |
| `undup.py` | new: an `if` whose arms agree is not a decision (§10) |
| `unsave.py` | new: a spill slot is not a variable (§11) |
| `unreload.py` | new: one member, loaded five times (§11) |
| `uncopy.py` | new: a local that is only a copy of another is not a local (§12) |
| `unhoist.py` | new: put back the load the scheduler moved (§12) |
| `explicitcmp.py` | new: write out the conversion a comparison already performs (§15) |
| `resign.py` | new: give a local the signedness of the values it holds (§16) |
| `resign_group.py` | new: give a set of locals that must agree the same signedness (§16) |
| `resign-drive.sh` | new: apply the retypes that measurably pay, and only those (§16) |
| `rename.py` | `--member`, because the member-safe patterns exclude `__frame.X` (§16) |
| `build.sh` | stamp `warn.log` with the source checksum, so a stale log is refusable (§16) |
| `unify_types.py` | write only when there is something to change (§15) |
| `decast.py` | unchanged, and it earned its keep the moment 156 declarations moved |
| `proven.sh` | new: which tools' answers depend on the file at all (§10) |
| `outpath.py` | new: a generator's argument is the file to write, and must look like one |
| `sweep.sh` | new: run every tool, and fail if the file moved (§10) |
| `collect_globals.py` | do nothing, and write nothing, when there is nothing to collect |
| `prune_unreachable.py` | `alignas(16) ... = {` is not a function called `alignas` |
| `hex_constants.py` | an already-converted constant is not a count mismatch |
| `deblob` `foldif` `reframe` `retype` | print the usage line instead of a traceback |
| `runarray` `unalias` `unmemcast` `unspill` | say zero out loud instead of nothing |
| `triage.sh` | unchanged, and it paid for itself four times over |

The stride scan is in §2. It is not committed as a tool because a scan that
reports zero and cannot report anything else is a claim, and the claim belongs
in prose where it can be read alongside what it does not cover — arrays of
pointers, and addresses held as integers.

---

## 10. The zeros were not measurements

Every round of this project has ended with some version of "every tool in
`tools/` reports zero", and the sentence has been carrying more weight than it
could bear. The obvious way to check it is to ask the whole directory at once:

```
for t in tools/*.py; do python3 $t subs1.hpp; done
```

That command destroys the decompilation. Three of the sixty tools are `mk*.py`
generators that take the file to **write** in the argument position every other
tool takes the file to **read**, so it runs them with `subs1.hpp` as their
destination and leaves it a 96×96 bitmap. It cost nothing, because the file had
been committed a minute earlier — which is luck, not a safety property.

What the sweep showed once it could run is that the zeros were not zeros:

| tool | what it was actually doing |
| --- | --- |
| `collect_globals.py` | rewriting the file when it had nothing to collect, stacking a header announcing "0 declarations for 0 objects" above the real one |
| `prune_unreachable.py` | dying on `assert 'two definitions of alignas'` before printing a line |
| `deblob` `foldif` `reframe` `retype` | a traceback where the rest of the directory prints a usage line |
| `hex_constants.py` | exiting on the first already-converted constant, so it never reached the ones still in decimal |
| `runarray` `unalias` `unmemcast` `unspill` | printing nothing at all, which reads exactly like a tool that crashed |

Nine tools out of sixty, and not one of them had ever answered the question it
was being counted as answering.

Two of the failures are worth the space. `prune_unreachable.py` finds a
function definition by looking for the last `name(` before a top-level `{`,
which reads `alignas(16) static uint8_t ctx_group_flags[32] = {` as a definition
of a function called `alignas` and `#pragma pack(push, 1)` as one called `pack`.
A brace after an `=` opens an initializer and a preprocessor line is never part
of a signature; with those two lines added it runs, and says 90 definitions and
nothing unreachable — the first time that has been checked rather than assumed.

And `hex_constants.py` was hiding a finding behind its crash. Its four
unreachable constants were all in a table called `p2_b1_seed`, and every one of
that table's seventeen entries is an IEEE-754 single: `169.2f`, `1.0f`,
`0.001f`, `0.0001f`, `0.1f`, `2.0f`, `576.0f`, `2.6f`, `26896.0f`, `0.013f`,
`5041.0f`, `529.0f`, `0.0002f`, `10.0f`. Fifteen of them appear *verbatim* as
float literals in `alt_p2_model` a few hundred lines below:

```c
_this[14][1] = 169.2f;
v26 = (1.0f - (v24 / (v23 + 576.0f))) * 2.0f;
const float floor_a = 26896.0f * f278656[14][2];
... / (f278656[7 + j][k] + ms_scale * 529.0f);
```

It is MSVC's `.rdata` constant pool for that function. Hex-Rays put the loads
back inline, so the pool has no float reader left, and the name was backwards:
it is `p2_float_pool`. The only thing that still touches it reads it a byte at
a time — `P2Count::b1` is seeded from bytes 4..11, which are the low halves of
`1.0f` and `-2784.44f`, `00 00 80 3F 02 07 2E C5`. Those are not counter seeds
and were never meant to be. It does not matter, and that is the interesting
part: `b1` is written on every counter update and read nowhere in the program,
so the sole use of the table is a store no one loads.

The table keeps `int32_t` and its address for that reason. Retyping it to
`float[17]` would be more honest about the data and would have to be trusted to
round-trip seventeen literals back to the same bits; hex with the float beside
it says the same thing and cannot be wrong.

### The eight zeros that had never moved

`sweep.sh` fixes the mechanics. It does not fix the deeper problem, which is
that a tool reporting zero and a tool that cannot report anything look the
same. `tools/proven.sh` is the check for that: run each tool — today's tool —
against old revisions of the file, and ask whether its answer ever *changes*. A
tool whose last line is identical across the whole history either always finds
nothing or never looks.

Over six revisions spanning the project's 329 commits, 43 of 51 answer
differently somewhere. Eight do not, and a list of eight is short enough to
audit by hand, which a list of fifty-one zeros is not:

| tool | why its zero is right |
| --- | --- |
| `decast.py`, `uncast.py` | the compiler agrees — all four remaining `-Wuseless-cast` warnings were in `bmf.cpp`, which neither tool scans |
| `unbss.py` | there is no `bmf_bss`; every global that lived in it has storage of its own, and the last mention of it was a stale comment in `bmf.cpp` |
| `uncursor.py`, `unmemcast.py`, `unspill.py` | `shape.py` says 17 frames, 0 aliases, 0 spill slots, 0 member runs walked as arrays — the shape is gone |
| `retype_locals.py` | its worklist *is* `strict.log`, and `BMF_STRICT` reports 0 |
| `unused.py` | came off the list the same afternoon: nothing to do until §10's collapses, then six names in one run and four in the next |

That last row is the point of the distinction. "Has never reported anything" is
not "is broken" — it is "has never been shown to work", and the two need
different responses.

The audit also cost a stream. Three of the four `-Wuseless-cast` warnings in
`bmf.cpp` were real; the fourth was `__PAIR64__`'s cast of `low` to `uint32_t`,
and removing it moved `x_ep` by four bytes. The compiler is right that at two of
the three expansions `low` already arrives unsigned, and wrong that the cast is
therefore removable, because a macro is textual and the third expansion passes a
signed value that sign-extends through the OR. **A ratchet is a scoreboard, not
an instruction.** The fix was to stop the two innocent call sites from casting
`low` themselves — they pass a `uint8_t *`, so the macro's cast performs that
conversion instead of repeating one — which leaves the macro correct and no
expansion useless.

---

`tools/sweep.sh` is the answer to the sentence. It runs every tool, checksums
the file before and after, and **fails if the file changed** — a read-only
sweep that modifies what it is measuring has measured nothing. It also fails if
any tool prints nothing, because a tool that is silent when it finds nothing
cannot be told from one that died. `tools/outpath.py` is what makes it safe to
run: a generator writes a `.bmp`, so a destination not named like one is a
mistake and is refused.

It takes about a minute, so it is not in `test.sh`; the gate stays fast. The
point is not that it is automatic. The point is that "every tool reports zero"
is now a command with an exit status instead of a claim in a commit message.

---

## 11. Naming the bodies, and what the names were hiding

Round eight said the `vNN` locals were answerable only by knowing what their
values mean, and §8 above repeated it. That is true of the *last* ones and was
never true of the first ones, and the way to tell which is which is to start.

Forty-five bodies went from carrying `vNN` names to carrying none:
`predict_med`, `unpredict_med`, `alt_init_tables`, `colour_transform`,
`interleave_plane`, `transform_planes`, `alloc_image`, `model_planes`,
`init_symbol_list`, `unmodel_plane`, three `alt_model_p*_d8_*` wrappers,
`free_workspace`, `pixel_context`, `update_binary_pair`, `encode_context_bit`,
`decode_context_bit`, `alt_p1_encode_symbol`, `alt_p1_decode_symbol`,
`alt_p2_decode_symbol`, `alt_p2_encode_symbol`, `rc_begin_encode`,
`rc_begin_decode`, `layout_workspace`, `alt_p1_alloc`, `alt_p2_alloc`,
`encode_symbol_tree`, `decode_symbol_tree`, `symbol_list_update`,
`encode_symbol_list`, `decode_symbol_list`, `expand_alphabet`, `read_bmp`,
`init_model_tables`, `compress_image`, `write_bmp`, `expand_image`,
`alt_p1_context`, `alt_p1_d8_encode_body`, `alt_model_p1_d8_decode`,
`model_plane`, `unmodel_plane_slow`, `alt_p2_d8_decode_body` and
`alt_p2_d8_encode_body`.  58 of 102 bodies to 13, and 13 969 `vNN` uses to
9541.

**Almost every one of them turned out to be readable from its shape alone.**
`predict_med` is MED, the LOCO-I median edge predictor: `up` trails `p` by
exactly one row and steps with it, so the three loads are north, west and
northwest, and the branch tree is MED's three outcomes. `colour_transform` and
`interleave_plane` are inverses with the same three predictors and the same
seven-bit weights. `encode_symbol_tree` and `decode_symbol_tree` are a level
chosen by cumulative count and then a walk down a binary tree of counter pairs.
None of that needed the algorithm to be understood first; understanding fell
out of naming, not the other way round.

### What was not a variable

The names hid a category, and it is a large one. Of everything renamed, a
substantial fraction turned out not to be values at all:

| shape | tool | count |
| --- | --- | --- |
| a spill saved and restored across a region that cannot change it | `unsave.py` | 30 |
| one member loaded again between every pair of uses | `unreload.py` | ~40 |
| a running sum stored back after every term, eighteen of them dead | — | 4 blocks |
| an `if` whose two arms are the same code | `undup.py` | 5 |
| a local that is a second name for a parameter nothing assigns | — | ~25 |
| a loop counter doubled as `k_before = k` with `while (k_before + 1 < N)` | — | 7 |
| a pointer and a byte offset walking the same buffer in step | — | 3 |
| a local declared and never assigned, passed to a parameter nothing reads | — | 6 |
| a declaration with no use at all | `unused.py` | 60 |

`unsave.py` is the one worth keeping. MSVC spills a register before a loop and
reloads it after; Hex-Rays names the spill slot, and the loop arrives wrapped in
a save and a restore of a value nothing in it touches. Reading the body as if
that were a variable means looking for a reason a value is being carried across
a region — when the reason is that there is no reason.

Two more were single locals doing two jobs, which is the same artefact seen from
the other side: `update_binary_pair`'s `n0x8000` held a running total for the
first half of the body and a walk mask for the second, and `rc_begin_encode`
held a table position twice, once as a pointer and once as a byte offset. A
register doing two jobs is why its Hex-Rays name is a constant.

### Two findings, and one correction

`p2_b1_seed` is `p2_float_pool` — §10. `AltP1Block::fold` and `::fold_hi` hold
the same 256 bytes, which closes a question the struct had carried open for
several rounds; the quantiser walk fills the low half and the closing loop fills
the high half by inverting `unfold` outright, and with `-E` at 0 the buckets are
one residual wide so those are the same map computed two ways. Measured with a
probe, on every stream: 0 of 256 differ.

The correction is `predict_med`. The first draft of its commentary said the
residual histogram it builds feeds `cost_candidate`. It does not: nothing reads
`hist_scratch` back, because `model_planes` reuses that region as
`__model_planes_buf` a few lines later — and `predict_med` itself is unreachable
in this build, both call sites being under `plane_predictor == 1`, which `-E`
being 0 rules out. `unpredict_med` *is* reachable, because a decoder still has
to read what an encoder with `-E` produced, which is what `testfiles/med32.bmp`
exists to exercise. The body is there to be read, as the definition of what the
inverse undoes.

One more structure came out of it. `expand_alphabet`'s frame was recovered as
three untyped arrays — `uint64_t v28[2]`, `int32_t v29`, `uint32_t v30[91]` —
and is sixteen `SymList`s. `sizeof(SymList)` is 24 and `ent` is at +20, which
makes both loops that walk it legible at once: the free loop stepped back six
words at a time from one past the last list taking `[5]`, and the zeroing loop
wrote bytes 20 + 48i and 44 + 48i, which are `lists[2i].ent` and
`lists[2i+1].ent`.

### The four accumulator blocks

Worth its own note because it is the largest single artefact found. All four
`alt_model_p1_*` bodies open each row with the same twenty terms — ten `.mag`
values into `ctx[3]` and ten into `ctx[4]`, from the two rows above and this
row's left margin, even offsets to one and odd to the other. MSVC kept both
running sums in registers and stored *every partial back*, so each arrived as
twenty stores of which eighteen were dead and nineteen locals holding one
accumulator at nineteen points. Two of the four also arrived as byte offsets on
a `uint8_t *` — a record is `sym` then `mag`, so byte 2k+1 is record k, and the
stride scan of §2 cannot see it because the gcd of −3, −1, 1, 3, 5, 7, 9 and 11
is 1.

The four `(int8_t)` casts on the left-margin loads stay, and this round can say
why rather than leaving it open. A probe counting how often a margin magnitude
exceeds 127 reports **21 of 1536** on `testfiles/altp1.bmp`. The sign shows.
That is the *opposite* of §3's finding for `PixRec`'s three `movsx` loads, where
the writers are comparisons and the seed is 1 so the sign cannot show — two sets
of casts kept for two different reasons, and only one of them a transcription.

Getting that number took two tries. `bmf c` appends, so reusing one archive
path across six images decodes the same first member six times and reports
identical counts for all of them; `tools/triage.sh` removes the file first, and
this is why.

### What was actually left

Thirteen bodies, and they were the model itself: `alt_p2_model` (481 distinct
names), `alt_p2_context` (184), `choose_plane_coding` (177), `alt_p1_model`
(104), `decode_pixel` (100), `alt_model_p2_encode` (91), `search_filter` (81),
`code_pixel` (78), `alt_model_p2_decode` (74), `cost_candidate` (72),
`alt_model_p1_encode` (60), `reduce_alphabet` (54) and `alt_model_p1_decode`
(47).

This is where round eight's claim finally was true. Their locals are
intermediate values of an arithmetic whose meaning is `algorithm_v2.md`'s
subject, not a sweep's — and the difference from the forty-five bodies above
was not size but that those had a *shape*: a margin mirror, a cursor rotation,
a fold table, an inverse. These had an equation.

All thirteen are done, and so is every other body — §§12–13 are how. Two of
them turned out to have a shape after all, and it was the same one both times:
a mixed-radix index and an unrolled tree, each written by the decompiler as a
pile of unrelated scalars. The claim that the last bodies are "an equation, not
a shape" was half right. It was right that no sweep would name them. It was
wrong that there was nothing under them but arithmetic.

---

## 12. Four guards that made a rule quieter rather than wrong

Two of this round's rules are about names that are not names.

`uncopy.py`. MSVC keeps a value in more than one register across a long
straight-line block, and Hex-Rays names every register. `alt_p2_context`
reaches six reference-row cursors through nineteen names: `v93` and `v97` are
both `v282`; `v81`, `v92`, `v95` and `v69` are all `v281`. Naming those
nineteen by hand would have produced nineteen names for six things, and a
reader who trusted the names would think the rows were different rows. A copy
folds when it is `X = Y;`, `X` is assigned once and never through its address,
and `Y` is not assigned between the copy and `X`'s last use. It later learned
to see through a round trip between two pointer types, which is the residue of
the stride fix in §2 — `(P2Ctx *)((int16_t *)v282)` is the same copy with the
old spelling wrapped round it.

`unhoist.py`. The scheduler moves loads early and Hex-Rays names each one, so
one bank-context expression arrives as a pile of one-use locals with the
expression itself two screens down. I found that out by naming eleven of them
`k0`, `k1`, `k2`, `j1`, `j2` — which is exactly the spelling §10 already
records does not explain anything. `v121` is not a quantity this algorithm has
a word for; it is the second operand of a term, moved. So it goes back.

It only touches names spelled `vNN`/`tNN`. `Depth = p_i_img->depth & 0x3F`
reads once as well, and inlining that would delete a word someone chose on
purpose.

### The guards

Both rules, and `unsave.py` with them, guard against the cases where folding
would change the meaning. Four of those guards were wrong, and all four were
wrong in the same direction:

| guard | what it missed |
| --- | --- |
| `unreload.types()` | a local declared on a continuation line of a multi-line declaration — so most of the two biggest bodies had no types at all, and three rules read that as "not a local here" |
| `unhoist.statements()` | a bare `{` does not end in `;`, so it swallowed the statement after it, and the first assignment inside every block stopped looking like an assignment |
| `unhoist` read counting | counted *statements*, and one wrapped expression names the same local six times — so a six-read load looked single-use |
| `&NAME\b` | matches `&x->m`, `&x.m` and `&x[i]`, none of which is the variable's address. `alt_p2_model` reads its counters as `*(uint16_t *)&v533->w2`, so every counter pointer looked address-taken |
| `\bNAME\b` | matches `blk->band_lo`, which is a *member* and not the local of that name. It put a use before the local's own assignment and disqualified the copy — and in `unaliasvar.py` the same pattern was in the rewriter, where it turned `blk->band_hi` into `blk->band_hi0` and stopped the build |

Fixing them, on a file where every rule had reported zero:

```
uncopy    0 sites  ->  33, then 15 more
unhoist   0 sites  ->  63, then 48 more, then 13
unsave    0 sites  ->   1
```

Only one of the four was a correctness bug — the read counting, which would
have made six loads out of one. The other three made a rule *quieter*. That is
the failure mode this round keeps producing, and it is worse than a wrong
answer, because §10's whole apparatus is built to catch a tool that stops
saying anything: `sweep.sh` fails if a tool prints nothing, `proven.sh` fails
if a tool's answer never changes. Neither of them fires on a tool that still
answers, still changes, and answers about a third of the file.

There is no general check for this. What there is, is the habit: when a rule
reports zero, ask what it *cannot* see, and go and look at a place it should
have fired.

Four of the five are one mistake: a pattern that names an identifier has to say
it is not reaching through one. `&x` is not `&x->m`; `x` is not `y->x`.

### And the claim was still being read rather than checked

`unaliasvar.py` had been reporting **four** the whole time. Nothing hid it —
`sweep.sh` prints every tool, and I had been reading `sweep.sh | tail -2`,
which shows the summary and not the tools.

So the summary is the check now. Every tool's last line must contain a zero, and
the sweep exits non-zero naming the ones that do not; `addrmap.py`, `shape.py`
and `unify_types.py` answer with a map, a table and a census, have no count in
them to be zero, and are named in the script as exemptions rather than passing
by accident. Verified against the file as it stood fourteen commits earlier,
where it exits 1 and names `unaliasvar`, `uncopy`, `unhoist` and `unsave` —
because a check that has never failed is a claim.

The check earned itself immediately. Introducing one `(size_t)` cast made
`unify_types.py` -- which maps `size_t` onto `uint32_t` like every other
spelling -- rewrite the file during the next sweep, and the sweep failed rather
than reporting a quiet zero. Two things were wrong and it found both: the cast
should have used the project's spelling, and a survey tool should not write when
it has nothing to change.

The sweep also runs every tool against a *copy* now, beside the real file rather
than in `/tmp` (`addrmap.py` resolves the repository from the path it is given).
That closes a window: one sweep failed with "a tool wrote to the file it was
asked about" and never reproduced, and a tool killed by the 300-second timeout
part-way through a write is the shape that fits. A killed tool is reported as
killed rather than counted as reporting nothing, and nothing in the sweep can
reach the decompilation at all.

### Checking a guard rather than asserting it

`unhoist`'s write check had a second hole: the patterns stop at the first `[`
or `->`, so they catch `cx0 = ...` and miss `cx0[-4].err = ...` — and the
second is exactly what changes the value of `v = cx0[-4].err`. That one is a
correctness hole, so it wanted checking and not asserting.

Replaying the stricter rule against the file as it stood at each of the four
revisions where the rule fired: 76 sites then against 127 now, 4 against 52, 65
against 144, 48 against 48 — strictly more in every case, and exactly one site
in the whole history that the stricter rule refuses. That one is
`v228 = blk->ctx_w[0].sel`, blocked because `blk->ctx_pair[1] = ...` writes
through `blk`; the two members cannot alias and the inline was right.

It is left blunt. Distinguishing them means deciding when two member chains off
one base can alias, which unions make a real question, and the whole cost of
not deciding is one load.

### What the two rules were worth

They removed 190 names between them without a single rename, and they are why
`alt_p2_model` was namable at all: it went from 482 to 421 before anyone read a
line of it.

---

## 13. What the last two bodies turned out to be

`alt_p2_context` picks one of 1600 neighbourhood slots, and the index arrived
in five different disguises: a `P2Ctx *` that only ever held `16 * gB`, an
`int16_t` subscript on it carrying two more digits, a stack slot carrying a
fourth shifted left by six, three loose comparisons for the fifth, and a
`uint8_t *` for the sum. Written out it is one line:

```
nb_slot = 320 * band + 64 * gA + 16 * gB + 4 * gC + gD
```

with `gA` in [0,5) and the other three in [0,4), so the digits pack exactly.
Each is a count of how many of its own thresholds one ratio has passed, and
`band` — five ratio bands of the coded length — picks the threshold row all
four read.

`P2Ctx::lane[8]` was an array because Hex-Rays could not see it any other way,
and 506 reaches into it read `lane[4]`, `lane[5]`, `lane[6]`, `lane[7]` without
saying that those are four directions. `alt_p2_model`'s prologue writes seven
of them in a row and says what they are; `alt_p2_context` corroborates from the
other side, since its four weighted context sums are seven taps each over lanes
4, 5, 6 and 7 — one sum per direction, which is not a thing anyone builds
unless those four lanes are the four directions. They are `val`, `dval`, `err`,
`aerr`, `dleft`, `dup`, `dupleft`, `dupright` now.

I expected to keep the array arm in a union for the sites that subscript it
with a running index. There are none — the grep that found four was matching
the tail of `Block_plane[k++]`.

`alt_p2_model` walks a binary tree of counters, fully unrolled, eleven levels
from 0x4000 down to 0x10, touching three nodes at each: the node itself, its
mirror at `0x7FF0 ^ bit`, and its rotation through `p2_ctx_rotate`. Hex-Rays
had those 35 pointers as `v531`..`v568` in scheduler order, so `v542` and `v544`
are the two halves of one level and `v542` and `v539` are different levels.
Each is then updated by the same three lines — load `w2`, take the residual
against `p2_pred`, store `p2_bump` — 96 times over, which is 192 more names.

The generator that named those had the same bare-`{` bug I had just fixed in
`unhoist.statements()`. It is a genuinely easy mistake to make twice.

---

## 14. Structure: what a `goto` was standing in for

Every jump removed this round was one of three things.

**A branch condition written as a jump.** `predict_med` and `unpredict_med`
both write the median-edge predictor as a pair of `goto`s over a shared
statement. It is an `else if`: northwest outside `[min(west, north),
max(west, north)]` predicts the far end of it, inside predicts the plane
through all three. The two arms differ only in which way round the comparisons
go, because which of west and north is the smaller is what the arm is testing.

`write_bmp` is the same thing three times. Its RLE encoder flushes pending
literals when a repeat run starts, when the literal run hits its cap, and at
the end of a row, and each arrived as a jump into the middle of another arm.
The condition is whether an absolute run pays for its two-byte header — from
two literals in 4-bit mode, three in 8-bit — which the decompilation spelled
`Size != 1` in one arm and `Size >= 3` in the other and jumped between them.
`nib ? Size != 1 : Size >= 3` says it once.

**A loop skip.** `if (height == 1) goto` past the row loop is the row loop
under an `if`.

**A join with several predecessors that all do the same thing first.**
`unpredict_med`'s `LABEL_29` has three, and every one of them sets `rows_left`
and returns early on a single row before jumping. Hoisting the common part
makes the jump a fallthrough.

What stays is the fourth shape, and it stays on purpose: `write_bmp`'s
`LABEL_72` is the row terminator, reached from five places inside three nested
loops. A forward jump to a single join point is what that is for, and it now
says so in a comment rather than by being the only one left.

`shape.py` has two more rows for exactly that reason. The bare count says how
many jumps are left and not whether any of them should be, which is the same
defect §10 is about — so it now says what shape they are:

```
goto / LABEL_n:                    49 / 34
  restart a loop / exit N blocks   16 / 29
  jump into a block / sideways      1 / 3
```

45 of the 49 are a loop restart or an exit out of nested blocks to one join,
which is what C has `goto` for. The four on the second row are three exits to a
function's single return path and one jump into a block in `read_bmp`'s RLE
decoder, where the label is the top of the decoder loop and every predecessor
is a `continue` from two loops in. Those are the honest residue, and the row
exists so that a later round can tell at a glance whether it has grown.

One thing worth recording: removing a `goto` duplicates the statement it jumped
over, and in `predict_med` that took the conversion count *up* by three. The
ratchet caught it before I did. Casting the four `LOBYTE` assignments
explicitly took it to 1323 instead, below where it started. It happened again
in `alloc_image`, which is enough to call it a rule: **a `goto` you delete is a
statement you copy, and the ratchet is what tells you which.**

---

## 15. What is left

Everything with a target is at it. What remains has no target, and saying so
precisely is the point of this section rather than leaving the reader to infer
it from a table of zeros.

**1238 conversion warnings, and no `-Wsign-compare` at all.** The sign
comparisons are gone, and not by retyping anything. `while ( x < width )` with
`x` unsigned and `width` signed does not compare a signed value with an
unsigned one — C converts `width` to unsigned first, and the comparison is
between two unsigned values. That conversion is the entire content of the
warning: the code means something a reader has to know the usual arithmetic
conversions to see. `tools/explicitcmp.py` writes it where it happens, taking
its sites from `warn.log` and the operand's extent from the column GCC points
at the operator. 62 sites, no semantic change, and the two claims it does
*not* make are in its docstring — it does not decide whether a comparison is
right, which is a question about whether the signed side can go negative.

What is left is 1070, and `shape.py` now says what they are made of rather
than only how many there are — the same defect the `goto` row had:

```
conversion warnings                1070
  signedness, same width            567
  narrowing 32 -> 16                306
  narrowing 32 -> 8                 165
  narrowing 64 -> 16                 18
  a negative constant into an unsigned type    5
  narrowing 64 -> 8                   5
  narrowing 16 -> 8                   3
  narrowing 64 -> 32                  1
```

The rows add up to the ratchet by construction, including rows for
`-Wsign-compare` and `-Wuseless-cast` that are currently absent — a breakdown
that quietly drops two of what it is breaking down is exactly what §10 is
about, and the first version of this dropped two.

The narrowings were described here as "into fields the 1997 layout fixes at
eight or sixteen bits", and half of that is true. `shape.py --narrow` asks
where each one lands rather than asserting it:

```
  247  into a struct member
   97  through a pointer or subscript
   53  another store, mostly LOWORD/LOBYTE
   51  into a local
   50  not a store on this line
```

Under half land in a struct member — `freq_tbl->w[0] = …` where `w` is
`uint16_t[8]` because the record is sixteen bytes, and no declaration can fix
those. The rest are a store through a pointer, a `LOWORD(x) = …` on a local
that *cannot* narrow because something else reads it whole (§17's hazard, from
the other side), a store into a local that is deliberately narrow, and
expressions that are not a store at all. None of those is a declaration's
mistake either — but they are not one group, and saying they were was a guess
that the measurement contradicts.

The 567 same-width ones are where the destination is not a local either rule
can reach, or where one can and the flip has been *measured* not to pay.

Getting there took six extensions, each from watching the rule stop: both
directions of flow, every width rather than the 32-bit pair, past an ordering
comparison whose other side is already cast, whole components at once, an edge
between locals that never convert against each other at all, and offering a
component both of its two signednesses instead of the one a headcount picks.
The two rules feed each other — a group flip creates single candidates and vice
versa — so they were alternated until both stopped, which took 1238 down to
1070.

The rate is what says where to stop rather than any argument about it: the last
rounds cost fifteen minutes of rebuilds each and returned one or two warnings.
The rules are still correct and still find candidates; what has run out is the
supply of candidates that pay.

Retyping struct members was tried and abandoned, which is worth recording so
the next round does not try it again: the member clusters are mixed-direction.
`->ctx_w` receives `int32_t` seven times and `uint32_t` four; `->cursor` four
and four. Flipping a member that is written both ways round trades one warning
for another, and the biggest cluster in the file is exactly that shape.

The same tool would zero those too, and it is not going to, which is worth
being explicit about because the distinction is thin and the temptation is not.
A cast on a comparison *adds* something: `x < width` and `x < (uint32_t)width`
look like the same operator and are not, and no reader recovers that without
knowing the conversion rules. A cast on an assignment adds nothing — `x = y`
with the two declared types visible already says the value is being narrowed,
and writing `(uint16_t)` in front of it moves no information anywhere. It would
move the number, which is the entire objection: §10 is about measures you can
satisfy without doing the work, and putting 1090 casts in this file to make a
scoreboard read zero is the purest example of one this project could produce.

The number goes down when the *types* are right, and §16 is 64 of them.

**49 jumps, 45 of which are what `goto` is for.** The breakdown row in §1 is
there so a later round can see whether the other four have grown.

**11 raw-offset sites**, all of them a byte buffer being indexed — the counter
has said since round three that it is a counter and not a target, and
`exclusion_mask[symbol]` will never become a member.

**1387 pointer casts**, none of which the compiler thinks is useless.

What is *not* left: a Hex-Rays name, anywhere in either file; an `fNN` member;
a raw offset off `_this`; a frame that carries two names in one slot; a rule in
`tools/` that finds anything.

---

## 16. Retyping: 1238 to 1174, and four corrections from the gate

Hex-Rays picks `int` or `unsigned int` per register and not per quantity, so a
count assigned only from `int32_t` expressions arrives declared `uint32_t` and
every assignment to it converts. That is not a narrowing anyone intended; it is
a declaration disagreeing with its own right-hand sides, and the fix is the
declaration. `tools/resign.py` retyped 156 locals over three passes.

Retyping leaves a tail, and it is worth doing rather than leaving: eleven casts
became useless when the type under them changed, and four comparisons became
mixed that were not before — including three where the `(int32_t)` that
`explicitcmp.py` had added a commit earlier now pointed the wrong way and had
to come off entirely. 1174 to 1156, and five deliberate wraps say so: `fold[128] = (uint8_t)-1` is
the fold table's odd codes counting down from -1, which in a byte is 255. That
cast earns its place by the same test as the comparison casts -- a negative
constant reaching an unsigned type is either a mistake or a deliberate wrap,
and the compiler naming the value it becomes is not the same as the reader
knowing which. A sixth attempt at one was itself useless and came straight back
out; a useless cast is worse than the warning it silences.

Every correction to the rule came from the gate, and each one is a different
part of it:

**The streams.** The first version proposed 153 flips and four streams moved.
Bisecting found `alt_p2_model`'s `e_top`, which is a *residual*: `e_top <
deadzone_lo` asks whether the error is below a negative bound, and declaring it
unsigned makes every negative error enormous. So the rule will not touch a
local that is an operand of `<`, `>`, `<=` or `>=` — that is where signedness
changes the answer rather than the bits, and the other side is usually a plain
local with nothing to protect it. 153 candidates became 91 and the streams held.

Worth noting what made the *rest* safe: `explicitcmp.py` had already put a cast
on every mixed comparison in the file, so those sites convert the same way
whichever side is flipped. The two rules only work in that order.

**The ratchet.** The warnings *into* a local are half the story — it also flows
into locals still declared the old way, and each of those becomes a conversion.
Counting one half proposed eighteen flips that put the total *up*. The rule
counts both halves now and requires a net gain; the ones it still cannot see
through are why `resign.py` reports rather than counts (below).

**The strict pass.** Taking one name out of an eight-row comma list left
`int32_t a, b, ;`, and where the name carried a trailing comment,
`uint32_t ;   // a record index`. That compiles under `-fpermissive` and
nowhere else, so it failed the gate's strict pass while the streams stayed
byte-identical.

**And one that no part of the gate could have caught**, because it made the
tool do nothing at all: `warn.log`'s line numbers mean nothing against any
other version of the file, and running against a stale one retypes whatever now
sits on those lines — 17 wrong locals, +43 warnings. `build.sh` stamps the log
with the source's checksum now and the tool checks it. mtime was the first
attempt and is not the check: a `cp` of the source is *newer* than a log that
describes it exactly, so the guard fired through an entire bisection and
reported the change innocent twice.

### Both directions

The first rule looked only at what flows *into* a local: the warnings that name
it as a destination. That misses the mirror case, and the mirror case is where
the range coder lives. `rc.encode(cum, cum_hi, tot)` takes unsigned cumulative
counts, and its callers pass locals Hex-Rays declared `int32_t` — so the
conversion is on the way *out*, at the call, and the local's own assignments do
not warn at all.

GCC's column points at the expression being converted, so a bare identifier
there is the local and the flip goes toward the destination's type. What the
rule will not do is flip a local that converts on the way in *and* on the way
out: that one cannot be helped, because the flip only moves which end warns.

### Locals that have to agree

What single-local retyping cannot reach is a local converted on the way in
*and* on the way out. Whichever way it is declared, one end converts — so
flipping it alone always trades, and the driver measures that and refuses.

Those locals are not independent of each other. `cum = ...; cum_hi = cum +
*slot; rc.encode(cum, cum_hi, tot)` is three names that have to agree, and
Hex-Rays declared them from three registers. Flipping any one trades; flipping
all three removes every conversion between them.

`tools/resign_group.py` builds the graph whose nodes are locals and whose edges
are the signedness conversions between two locals of one body, takes its
connected components, and proposes making each component agree — 167 locals in
59 groups, in a file where every one of them had already been measured not to
pay alone. The target is the component's majority signedness; a tie goes to
unsigned, which is what this codec's counts and cumulative frequencies mostly
are. Every member has to pass the same `safe()` test the single-local rule
uses, and one member failing disqualifies the group: half a group agreeing is
worse than none of it.

**Signedness only, never width.** A component whose members are not all the
same width has no single signedness to agree on, and the majority vote proposes
a *truncation* instead. `uint8_t <- half (int32_t)` in `alt_init_tables` got as
far as a green gate before that check existed, and it is worth being precise
about what that means: fifteen byte-identical streams did not approve a
truncation, they failed to exercise it. A gate is evidence about the inputs it
runs, and the rule has to be right about the ones it does not.

### Locals that have to agree and never meet

The graph above has one blind spot, and it took reading the leftover
single-local candidates to find it rather than watching the rule fail. Six of
the thirty were `alti2` … `alti7` in `alt_p1_model`: six registers holding one
quantity, each converting into the same struct member, and **none of them
assigned from any other**. There is no conversion edge between any two, so the
components rule saw six components of one node each, and the single rule
measured each flip against the five siblings still declared the old way and
correctly refused all six. Every one of them was a trade alone; the six
together were worth ten warnings.

So the graph gets a second kind of edge, drawn not from a conversion but from a
name: same stem, same body, same width, differing only in the trailing number.
That is a Hex-Rays register-numbering artefact rather than anything in the
program, which is exactly why it is safe to key on — the numbers came from the
allocator, and locals that share a stem in one body are one quantity spilled
across several registers far more often than not. The edge only proposes; every
member still has to pass `safe()`, and the driver still has to measure a fall.

**And the majority vote was itself the last defect.** `alti0` and `alti1` were
already `uint32_t` and the other six `int32_t`, so the headcount said the
component should agree at *signed* — proposing to flip two correct declarations
to reach a worse total. A component has exactly two possible answers, so the
rule stopped voting and now offers both as separate candidates, letting the
ratchet pick. That is the same move as §15's: replacing a heuristic that
decides with a measurement that decides, and keeping the heuristic only for
what to try. The pair of fixes landed together and the first group they found
was the `alti` sextet — it needed the sibling edge to be seen at all and the
both-directions change to be proposed the right way round.

### Why `resign.py` proposes rather than counts

It joins `shape.py`, `addrmap.py` and `unify_types.py` in the sweep's
report-rather-than-count list, and this one needs its reason written down
because unlike those three it does have a count.

Whether a type change removes conversions or merely moves them is settled by
the compiler. The rule's net-effect test sees a local flowing into other
*locals*; it cannot see it flowing into a struct member or a call argument, and
after the passes that paid there are still 29 single locals and 23 groups left
that do not. So its list is a list of things to try, the ratchet is what says
whether trying them worked, and a non-empty list is not a defect. That is a different kind of
tool from `uncopy` and `unhoist`, whose conditions are decidable from the text,
and the sweep should not pretend otherwise.

---

## 17. Eleven raw offsets, and a register wider than anything read it

§1's `raw-offset sites` row had sat at eleven since the round began, and the
reason it never moved is that eleven is small enough to read and each one is a
different judgement — so no rule fires and the row looks permanent. Reading
them turned out to be worth it: six were spellings, one was hiding a jump into
a block, and three were a shape with a rule in it.

### The six spellings

`*((uint16_t *)row + 1) = 24 * alt_freq_limit` sat between five lines of the
form `*(uint16_t *)&row[4] = 205`. One header field of six written in a
different idiom from its neighbours is not a distinction, it is a scheduling
accident, and `&row[2]` makes the six read as one block.

`__frame.slot[2 * j + 3] = (void *)(dst_b + 1)` had `__frame.slot[2 * j + 2] =
dst_a + 1` directly above it. The cast is to the type the assignment already
converts to, so it does nothing but make two adjacent lines look like two
different operations.

`next = (PixRec *)(recw + 2)` steps eight bytes through an `int32_t *` view of
a record that `static_assert` fixes at eight, which is `_this->row_cur[5] + 1`
with the arithmetic done by the type instead of by hand. And
`*((uint32_t *)wp + 1) = 0x01010101` writes four bytes at offset four of a
`PixRec`, which is `match[2]` — the same store the decoder writes as
`*(uint32_t *)&rec->match[2]`, so the encoder and decoder now say it the same
way.

### The one that was a goto

`ArgList_6` was one register carrying two cursors. `unmodel_plane_slow`
reconstructs a plane at one, two, three or four bytes per pixel, and the
four-byte branch walks the output with `*ArgList_6++` while the two-byte branch
walks it with `*(uint16_t *)ArgList_6` and then puts the pointer back by hand:

```c
ArgList_6 = (uint32_t *)((uint8_t *)ArgList_6 + 2);
```

That line is the declared element size being wrong, which is §2's lens pointed
at a cursor that never had a chance — the same name is a `uint32_t *` twenty
lines earlier and genuinely is one there.

What kept them one name is that the two branches shared a two-line tail —
`ArgList_5 = (uint8_t *)ArgList_6; goto LABEL_74;` — and the four-byte branch
reached it with a `goto` into the middle of the two-byte branch's block. That
was **the last jump into a block in the file**, and it existed to save two
lines. Duplicating them separates the lifetimes, and the second cursor can
then be a `uint16_t *` that walks with `*out16++`. One `goto` and one label
went with it, and the `restart a loop` count fell by one as well: the tail's
own `goto LABEL_74` was a backward jump that the duplication made forward.

### `LOWORD(x) = e` is a local that is not 32 bits

The three left over were a shape, and it is the same shape three times:

```c
uint32_t row16;
if ( !byte_rows && bits == 4 )
  row16 = ((img_w + 7) >> 1) & 0xFFFFFFFC;
else
  LOWORD(row16) = (uint16_t)(((bits + 7) >> 3) * img_w);
row_bytes = (uint16_t)row16;
```

Both arms assign one sixteen-bit quantity. MSVC wrote one of them through the
whole register and the other through its low half because it knew the top half
was dead, and Hex-Rays wrote the half-write as a macro and every read back as a
mask. So the local is a `uint16_t`, and saying so removes the macro, the masks
and the question of which half is live. `tools/unloword.py` is the rule;
seven locals in six bodies, and it reports zero now.

Signedness comes from where the value lands, not from a default.
`plane[3]->cursor[0]->dval = l7c` puts it in an `int16_t`, so `l7c` is one too
and the store stops converting — picking unsigned everywhere would have traded
a narrowing warning for a signedness one and counted it as progress.

The hazard is a local whose value is *used* at full width, and there is one in
the file: `LOWORD(w_new) = w_top + (w_new >> 2)` reads all thirty-two bits to
compute the sixteen it stores, and narrowing `w_new` would change the shift.
So a read the rule cannot account for disqualifies the local rather than being
ignored, which is the opposite of the reflex §12 is about.

### Three corrections, and where each came from

**Five of the eight were invisible.** The rule skipped any local declared in a
continuation line — `int32_t val1, l7a, l4a, l5a,` on one line and `seed2` on
the next — because it asked whether *that* line starts a declaration, and a
continuation does not. It read the name as a use, decided the local was read
at full width, and refused it silently. Scanning declaration *runs* took the
find from three to eight, and those five are `seed2`, `seed3`, `l7c` and two
more of the same pair-of-arms shape. A rule that is wrong about what a
declaration is will always be wrong quietly, because its output is a shorter
list and not an error.

**One was arithmetic wearing a store's clothes.** `LOWORD(st) = st - down`
ends in a semicolon and has an `=` before it, which was the whole of the first
version's test for "the entire right-hand side" — so `down` was taken for a
local whose only read is a sixteen-bit store, when it is read at full width to
compute one. It happens that narrowing it would have been harmless, because the
low sixteen bits of a difference depend only on the low sixteen bits of its
operands; that argument is not one the rule makes, so it now requires the `=`
to be the last thing before the name and refuses `down`. A false rejection with
a reason beats an acceptance with a coincidence.

**And one the gate caught rather than the rule.** `wp` in `code_pixel` is a
register holding an address in one lifetime and a symbol in the next, and
narrowing it around the address produced `uint16_t x = (uint16_t *)p` — which
compiles under `-fpermissive` and nowhere else, so the strict pass failed while
all fifteen streams stayed byte-identical. The rule declines a local with a
pointer-valued full write now. `wp`'s pointer lifetime had become dead two
edits earlier, when the store through it moved to `__frame.sym9->match[2]`, so
deleting it is what makes the local narrow — but that is a deadness argument
and this rule does not make those either.

Replaying the guarded rule against the file that still had the live pointer
store refuses exactly `wp` and keeps the other six, which is the check §12 asks
for: a rule reporting zero on the file it just changed proves nothing, and the
file from *before* the change is the only place the guard can be seen working.

### One mask that had to move with the local

Narrowing `row16` moved its truncation from the read to the write, and the
compiler said so — `((img_w + 7) >> 1) & 0xFFFFFFFC` may change value on the
way into sixteen bits. The answer is not a cast. The mask rounds down to a
multiple of four and was written thirty-two bits wide because the register was;
at the width the value actually lands, it is `& 0xFFFC`, the two spellings
agree bit for bit, and the compiler can then see the value fits instead of
warning that it might not. The ratchet held at 1070 without being asked to.

### What the last six are

Two are a variable byte offset into a block whose layout at that offset depends
on a length computed at run time — `*(uint32_t *)((uint8_t *)blk + pad_len + 4)`
— and no declaration expresses that. Three are `(uint8_t *)(at + 16)` in
`cost_candidate`, where `at` is a signed pixel offset and the base is an image
row, so the arithmetic is deliberately in bytes. The last is
`(uint16_t *)(tbl + 4 * (node + span))`, a counter table indexed in pairs,
which §13's binary-tree walk describes and which a `struct { uint16_t f[2]; }`
would express — that one is a candidate for a later round rather than a thing
that cannot be done.

---

## 18. The last jump into a block, and a row that was over-reporting

§17 removed one of the two badly shaped jumps by splitting a cursor. The
other was in `read_bmp`, and it is the clearer case because nothing about it
was a type.

`read_bmp` has three decoders — RLE8, RLE4 and uncompressed — and four jumps
that all mean the same thing: *the image is complete, stop decoding and hand it
back.* The label they jump to sat **inside the uncompressed decoder's loop**,
in the arm of an `if` that ends the last row:

```c
      Src_6 -= ElementCount;
      if ( --y < 0 )
      {
LABEL_61:
        img = __frame.img_f;
        break;
      }
```

Three of the four jumps come from the other two decoders, so reaching that is a
jump into a block followed by a `break` out of a loop the jumper was never in.
It works — `break` leaves the block it is written in and control falls to
`fclose` — and it is unreadable.

What made it look like an exit rather than a `break` is `img = __frame.img_f`,
and that line is a reload, not an assignment. `img` is written once, at the
allocation; `__frame.img_f` is written three times and every one of them is
`__frame.img_f = img`. So the whole label body is `break;` with a spill
restore in front of it, which is §12's kind of artefact wearing a control-flow
disguise. With the reload gone the label is the function's success exit and
belongs where the exit is, just above `fclose`, and the fourth jump — the
natural one that used to fall into it — becomes an ordinary forward `goto`.

**The bare `goto` count went up by one and that is the improvement.** Turning
a fallthrough into an explicit jump adds a `goto` and removes the last jump
into a block in the file. A total that moves in the wrong direction while the
thing it summarises gets better is the argument for the breakdown rows, made
by the rows themselves.

### The row was reporting two problems that were not there

That left two jumps under `sideways`, and reading them found nothing to fix.
One is `write_bmp`'s row terminator, which five paths reach and which the file
already documents as a join. The other is `compress_image`'s "this image is
under sixteen bytes, skip the whole compression path and write it raw", which
jumps forward to the raw-write path that an `if` also falls into.

Both are joins, and the classifier's own comment says so: *"forward at the same
depth to something that is **not** a join"* is the bad shape. It never checked
the second half of its own sentence. A label reached from two places is a join
whatever the depths are; one reached from a single `goto` and from nowhere else
is a branch written inside out. Counting predecessors — the `goto`s naming it,
plus one if the statement above it can fall through — separates them, and both
of the file's remaining sideways jumps are joins.

They are counted apart rather than dropped. A row that stopped mentioning them
would be a breakdown that quietly omits part of what it breaks down, which is
§10's whole subject, and the four shape rows still sum to the headline.

So the shape of every jump left in the file is one a `goto` is for: fifteen
restart a loop, thirty-two leave nested blocks for one join, two go sideways to
a join, and none goes into a block. The row that had been reading `1/3` since
the round began now reads `2/0` and `0`, and one of those two changes is work
and the other is the measure catching up with its own claim — which is worth
keeping separate, because only the first one changed the program.

### And the exit `untail.py` then offered to copy

Making `LABEL_61` a return tail put it in range of a rule that had been
reporting zero all round. `untail.py` replaces a `goto` to a straight-line
return with the statements themselves, and `fclose(Stream_v); free(…);
return …;` reached from five places is exactly its pattern.

The answer is no, and the reason is the number five rather than anything about
the statements. Every tail this file has copied so far went to two places —
the pairs of `fclose(arc->fp); … return nullptr;` in the archive paths are what
two copies of a release look like. Five copies of a release put the same
`free` in five places where one later edit has to keep all five right, which is
the situation `goto cleanup` exists for. So the rule takes a `--sites` cap,
default two, set at what the file has actually accepted rather than at what
made this one case go away.

Replaying the capped rule against the last forty revisions of `subs1.hpp`
changes nothing: every tail it has ever proposed had two sites, so the cap
costs no work that was ever done and refuses exactly the one case that
prompted it. That replay is the point — a cap justified only by the case it
was written for is a cap that will quietly refuse the next real one.

It also had to stop reporting `0 shared tails, 5 gotos`, which is a line that
answers with a zero and a number belonging to something it just declined. The
gotos counted are those of the tails proposed, and what the cap turned down is
named on its own line.

---

## 19. One address, three spellings, and the record nobody had written down

§17 left one raw offset flagged as "a candidate for a later round rather than a
thing that cannot be done". Following it up found something better than one
offset: three functions walking one data structure, each with the arithmetic
written out a different way, and only one of the three visible to any measure
this project has.

The structure is the binary tree of counter pairs the symbol coders use below
level 1. `encode_symbol_tree` codes each bit of `sym - level_geom[lvl].first`
down it, `decode_symbol_tree` walks the identical tree and reassembles the
symbol, and `update_binary_pair` walks it again to age the counts. Here is
how each of them found the pair for `node` at width `span`:

```c
pair = (uint16_t *)(((uint8_t *)&freq[2 * level_geom[lvl].tbl_base + 8]) + 4 * (span + node));
pair = freq + 2 * level_geom[sym].tbl_base + 2 * span + 2 * node + 8;
tbl  = (uint8_t *)((int32_t)(_this + 2 * level_geom[lvl].tbl_base + 8));   pair = (uint16_t *)(tbl + 4 * (node + span));
```

Those are the same address. The first is `uint16_t` arithmetic re-cast to
bytes, the second is the whole thing in `uint16_t` units with the factor of two
distributed by hand, and the third launders a pointer through an `int32_t` and
then steps in bytes. **Only the third counted as a raw offset**, which is why
the other two survived every round that has looked at this file: a measure that
keys on byte arithmetic sees the one function that does byte arithmetic, and a
tool that looks for a wrong element size — §2's lens — sees nothing wrong with
a `uint16_t *` stepping by two.

What was missing is that four bytes there is a record:

```c
struct FreqPair {
  uint16_t f[2];
};
```

A count for the bit being 0 and a count for it being 1, indexed by the bit,
which is why it is an array of two and not two names — every one of the three
walks does `pair[go]` with `go` the bit just coded. With the record and one
accessor for where a level's tree starts, all three walks say

```c
pair = bit_tree(freq, lvl) + span + node;
```

and the `int32_t` round trip, the `4 *`, the `2 *` and the byte casts are all
gone. Three of the file's remaining pointer casts went with them.

### What the accessor had to say out loud

`bit_tree` bases at `&freq[2 * tbl_base + 8]`, and the header it skips is *ten*
`uint16_t`, not eight — `rc_begin_encode` seeds a total, an escape weight and
eight fixed counts, and the pairs start at byte 20 while the accessor points at
byte 16. That is not an off-by-one: **the tree is 1-indexed.** All three walks
start at `span = 1`, so the first pair any of them touches is element 1, and
element 0 is the header's last two words which nothing ever reaches through
this type.

That was invisible while the three spellings disagreed, and it is the kind of
fact a record makes you state: writing `bit_tree(freq, lvl) + span + node`
forces the question of what element 0 is, where `+ 4 * (span + node)` on a byte
pointer does not. The seeding loop in `rc_begin_encode` now writes
`seed[i].f[0] = 60; seed[i].f[1] = 36;` over `(FreqPair *)&row[20]`, which is
the same 0x7A pairs it always wrote and which now visibly starts one element
into the tree.

### Two smaller things it turned up

`SymPair`'s comment opened "A binary counter pair: two `uint16_t`, both seeded
0x2000" and then, two lines later, "Not a counter pair, whatever the name
says". Both halves were left in when an earlier round corrected itself, so the
record's documentation led with the claim it exists to deny. Introducing the
type that *is* a counter pair, forty lines below it, is what made that
unreadable enough to fix.

And `deadcheck.py` reported `bit_tree` as never called, from three call sites.
Its call-site pattern is `\b(__[A-Za-z0-9_]+)\s*\(` — every body recovered from
the binary carries the `__` prefix, so a helper that was never in the binary is
invisible to it as a callee. The answer is the convention the file already
has: every accessor introduced by this project — `p2_pred`, `p2_bump`,
`sym_list_count`, `bmf_plane_desc` — is `static inline`, which both
`deadcheck.py` and `addrmap.py` skip, and which is what they should be skipped
for. `addrmap.py` made the same point from the other side and more precisely:
it reported one named body with no recorded address, because a function that
was never at an address in 1997 should not be claiming a name that maps to one.

The false positive is left as it is. A plain `static` helper would be reported
dead rather than silently missed, and being pointed at the convention is the
right outcome for that.

### The measure that would have found it, and did not

§19 was found by reading, which is not a method, so the next move is the rule
it suggests. `shape.py` counts raw offsets and only one of the three spellings
was one; §2's lens takes the gcd of a cursor's *constant* subscripts and none
of the three has any; the compiler is happy with all of them. A duplicated
address computation survives every measure here unless two copies happen to be
read side by side.

The arithmetic is decidable. `tools/unspell.py` evaluates a pointer expression
to a base and a map from variable to *byte* coefficient — following casts
rather than stripping them, because after `(uint8_t *)` a `+ 4` is four bytes
and before it, on a `uint16_t *`, it was eight — and reports two sites that
reach one address by different text.

**It does not find §19.** Two of the three spellings are
`2 * level_geom[lvl].tbl_base + …` and `2 * level_geom[sym].tbl_base + …`, and
`lvl` in one function and `sym` in the other are the same value under two
names. The third is in `update_binary_pair`, where the block arrives as
`_this` rather than `freq`. Nothing textual can know those denote the same
thing, so the rule that would have caught §19 is not this one and probably is
not a rule at all — it is having read both functions.

That is worth stating plainly rather than quietly shipping a tool named after a
find it cannot make. The first version of this section claimed otherwise, and
checking the claim against the revision before §19 is what corrected it: the
tool ran, reported, and the three spellings were not in the report.

### What it did find

It reports a different duplicate of exactly the same kind, and one that eight
rounds of reading had missed:

```
row  {'': 20, 'i': 4}
   rc_begin_decode   row[2 * i + 10] = 60;
   rc_begin_encode   *(uint16_t *)&row[4 * i + 20] = 60;
```

**`rc_begin_encode` and `rc_begin_decode` seed the same table**, one walking a
508-byte row as bytes and the other walking the identical row as 254 words —
the same ten header fields, in the same order, with the same values, and the
same 0x7A pairs after them. The decoder's spelling is the right one, because
every field in that row is sixteen bits, and converting the encoder to match
removed ten pointer casts including the `(uint8_t *)` on the allocation and the
`(uint16_t *)` putting it back. Its return type went with them: it returns the
table, which is `uint16_t *`, and the four call sites that discard it did not
care either way.

Running the tool back through the file's history says the pair has been there
since round eight and before. That is the argument for it — not the find it
was named for, but a find of that shape that reading did not make in eight
rounds of looking.

It reports and does not rewrite. Which spelling wins is a judgement — usually
the one whose pointer type matches the record, but that is an argument about
the data and not something the arithmetic settles.

Two things it declines rather than guesses at. A site whose index will not
parse — a shift, a call, a `k++` — is skipped, because an address it cannot
evaluate is not an address it can compare. And a line that its two scans both
see is deduplicated before the comparison: `p = &q[i + 1];` is a subscript and
a store, and a line reported against itself is the tool finding its own
duplicate rather than the file's.

---

## 20. Ten names for one cursor, and a mask family that swapped

`unmodel_plane_slow` still carried ten locals called `ArgList` through
`ArgList_10`, with a comment saying they had shared one stack slot and that
Hex-Rays named every use after it. The comment was right about where the names
came from and said nothing about what they are, which is why they survived a
round that claims no Hex-Rays names are left: the *pattern* `\bv\d+` and
`\bn0x` do not match `ArgList_7`, and §1's row measures the pattern.

What they are is one output cursor at the width the plane's depth calls for.
The function reconstructs a plane at four, three, two or one bytes a pixel or
packed below a byte, and each width walks the same buffer through its own type.
So `out_at` is where the next branch starts and the last one left off, and
`out32`, `out_ent`, `out16`, `out8` and `out_bits` are the five views of it —
which is why every branch begins by casting `out_at` and ends by casting back.
The remaining four are the input base, the destination buffer, the expansion
buffer that gets freed, and the row walker.

### One of the ten was two things

`ArgList_4` held `flags & 0x20` for the first half of the function and
`(int32_t)ArgList_3` for the second — a mask and a pointer in one slot, which
is the artefact §12 is about wearing a type. The pointer half is dead: both
`ArgList_4 = (int32_t)ArgList_3; … ArgList_3 = (uint8_t *)ArgList_4;` pairs
save and restore across a region that never writes `ArgList_3`, the same shape
as `read_bmp`'s `img = __frame.img_f` in §18. Four lines out, and the name is
a mask again.

### The mask family, and why it was renamed twice

Those masks are six bits of `flags`, each gating one "fold this weight into
that one" step. Hex-Rays called them `m1`..`m4` here and `m1`..`m6` in
`model_plane`, which is the encoder's mirror of the same loop — **and the two
numberings disagree.** `m1` is `flags & 1` in the decoder and `flags & 8` in
the encoder; `m2` is the other way round. Two mirror functions whose masks
swap names is the same defect §19 is about, one thing written two ways, and it
is worse here because both spellings are single letters and a digit. Naming
each for the bit it tests — `f_b0` … `f_b5` — makes every line check itself
against the constant beside it and makes the two functions line up.

**The first attempt renamed them file-wide and stopped the build**, which is
the part worth keeping. `m1` and `m2` are also members of
`AltModelP1EncodeFrame`, and a frame member is reached as `__frame.m1` —
precisely what `rename.py`'s member-safe pattern excludes, so the declaration
moved and the uses did not. It also renamed the encoder's `m1` to the
decoder's meaning, which would have compiled: two functions where `f_b0` is
`flags & 1` in one and `flags & 8` in the other is worse than what it replaced.

`rename.py` refuses that now. It already had `--member` for renaming a frame
member deliberately and `--in FUNC` for scoping to one body; what it lacked was
a check on the name being renamed *away* from, so a whole-file rename of a name
that is some frame's member is refused with the frames named and the two ways
out spelled in the message. The guard is one function, `frames_declaring`, and
asking it about `m1` answers `AltModelP1EncodeFrame` — which is where the four
declarations went.

### §1's row, corrected for the fifth time

§20's ten `ArgList` names sat in a file whose §1 row reads `distinct
unexplained locals: 0`, and the reason is the one this document keeps
recording: the row measures a pattern, and `ArgList_7` is not that pattern.

Hex-Rays has a third way of naming a local, and it is the one nothing here
counted. Besides `vNN` and naming a local after the first constant stored in
it, it names an argument after the **callee's parameter** — so a pointer passed
to `fread` becomes `Buffer`, the count becomes `ElementCount`, the file becomes
`Stream`, and the copies get `a`, `b`, `c` and `_N`. That vocabulary is MSVC's
CRT headers, and the file is full of it: `Src_6`, `Buffera_5`, `Bufferc_3`,
`Sizea_1`, `ElementCount_4`, `lpAddress_2`, `Blockb_9`.

**71 spellings, 127 declarations, 27 bodies**, while the row above them read
zero through an entire round of naming work.

The new row lists its vocabulary rather than matching a shape, which is the
opposite of how the other rows work and is deliberate. A capital letter is not
the test: `InName` and `OutName` in `bmf_compress` are names somebody chose,
`Colours` and `Palette` and `Grey` say what they hold, and `gA` … `gD` in
`alt_p2_context` were named by §4. Counting every capitalised local would have
put those in the row and made it a measure of typography. Counting the CRT
vocabulary counts the names Hex-Rays wrote and nothing else.

### And then named

All 127, in all 27 bodies. Most of it is what the names always were once
somebody looked: `Stream_v` is a `FILE *` and is `fp`, `ElementCount` is a
`fread` count and is what is being counted, `Sizea_1` in `read_bmp` is the byte
an RLE8 run repeats. Three parameters turn out never to be read at all —
`rc_begin_decode`'s flag and the two that pass it one — and say so now, which
is a fact about the program and not a naming choice.

Two of the bodies had more than names in them.

**`search_filter` had eleven copies of two frame members.** `Blockb_1` …
`Blockb_9` are nine reloads of `__frame.Blockb` and `Srca` … `Srca_5` five of
`__frame.Srca_7`, each assigned and used a line or two later with nothing
writing the member in between — and one of the six call sites *already* passed
the member directly, which is exactly what the other five look like now.
`uncopy.py` does not see these: it looks for a local that copies another
*local*, and a copy of a member needs the member proved unchanged across the
gap, which is not decidable from its text. The four with several dereferences
keep a local, because four repetitions of `__frame.tile_img` is a reload
written out rather than removed.

**`compress_image`'s masks were the fifth naming trap**, and `write_bmp` was
the sixth: `rename.py` had to be corrected twice more before this section could
be finished, which is a rate worth recording rather than hiding.

### `rename.py`, three defects deep

Renaming is the operation this project performs most and its tool had three
things wrong with it, each found by a stopped build rather than by reading:

1. **A whole-file rename of a name some frame declares** moves the member's
   declaration and leaves its `__frame.X` uses behind — §20's `m1`.
   `frames_declaring` refuses it now and names the frames.
2. **`--member` was rejected as a malformed `OLD=NEW`.** The flag is read off
   `sys.argv` where it is used but was left in the positional list, so the one
   documented way out of defect 1 did not run.
3. **A body with both a local and a frame member of one name** — `write_bmp`
   has a local `Buffera` and a `WriteBmpFrame::Buffera` — renamed the member's
   declaration along with the local, because a member's declaration is a bare
   identifier even though its uses are not. Lines inside a frame struct are
   skipped now unless `--member` says the member is the target.

All three are the same shape: `__frame.X` is deliberately *not* a use of `X`,
and every place that fact matters had to be told separately. The pattern is
right and the exception list around it was three entries short.

### The same disguise, a fourth time

The row read zero the day it was written, and five names were sitting in the
file. `rename.py` puts the function's name in front of a local that collides
with a global, so `__expand_image_Buffer_1` is `Buffer_1` behind a prefix —
which is **exactly** §1's fourth correction, the one about `__code_pixel_n0x2000`,
recurring on a new pattern in the same week it was written. The suffix was too
short as well: `Buffer_copy` and `Block_plane` are one word past `_\w{1,2}`.

So the pattern takes an optional prefix and a suffix of any length, and the
five are named: `expand_image`'s pair is the archive's magic word and the copy
of it that gets compared to `0x8A81`, `compress_image`'s is the 0/1 that
becomes bit 7 of the header, and `Block_plane` is the four-element array of
`AltP1Block *` that `plane1`, `plane2` and `plane3` are references into.

Four rounds of writing "the row is zero" have now been followed by four
findings that the row could not see, and the pattern in *that* is worth more
than any of the four: **a row whose test is a spelling will be defeated by a
spelling.** The two rows that have never had to be corrected — the ratchet and
the stream comparison — are the two that ask the compiler and the output rather
than the text.

---

## 21. Stop testing the spelling

Four rounds of "no Hex-Rays names are left" were followed by four findings that
the row could not see, the last of them on the day its own pattern was written.
The conclusion at the end of §20 — *a row whose test is a spelling will be
defeated by a spelling* — has an answer, and it was in the repository the whole
time.

**The first commit of `subs1.hpp` is the Hex-Rays output.** `addrmap.py` maps
every current body to the address in `BMF.exe` it was decompiled from, and the
original's bodies are still called `sub_<address>`. So the two join, and a
local declared under the same name in both is *by construction* a name nobody
here has chosen. No pattern, nothing to hide behind a prefix, and the answer
is exact and finite rather than a guess that has to be corrected when the next
disguise turns up. `tools/unnamed.py`.

Asked the first time, it said **21 names, 65 declarations** — in a file whose
table had just been made to read zero twice over.

### What every spelling had missed

Forty-two of the sixty-five were `a1` through `a7`: Hex-Rays' name for a
parameter it could not name, which is the single most recognisable thing the
decompiler does and which no row had ever counted. `alt_p2_model(AltP2Block
*a1, int32_t a3, uint8_t a4, int32_t a5)` had 131 uses of `a1` alone. The rest
were numbered copies — `i_1`, `i_2`, `j_1`, `j_3`, `k_2`, `p_i_1`, `p_i_2`,
`this_4`, `buf_1`, `buf_3` — where MSVC spilled one quantity twice and
Hex-Rays numbered the halves.

Naming them is mostly reading the one call site. `alt_model_p2_d8_encode`'s
`a3`, `a5` and `a6` are `src`, `height` and `out`; `alt_p2_encode_symbol`'s
`a2` and `a3` are the context pair and the symbol, which the caller already
spells `pair` and `code`. `alt_p2_model`'s three are the block, the sample and
the residual — `(AltP2Block *)blk2, val2, code2, val2 - pred2` at the call.

**Fourteen parameters are never read at all.** Four of those are one thing: an
`int8_t` second parameter tested as `(a2 & 1) != 0` in `bmf_destroy_archive`,
`free_workspace`, `alt_p2_free` and `alt_p1_free` — MSVC's scalar-deleting
destructor flag, "and free the object too", which the 1997 compiler emitted for
every class with a virtual destructor. That is now `do_free` in all four, and
saying it once is worth more than the four names it replaces.

### The row that reports what it kept

Sixty-six declarations *are* still Hex-Rays' spelling and stay: `i`, `j`, `k`,
`m`, `n`, `result`, `buf`, `_this`. The decompiler wrote what anyone would
have, and changing them would be churn.

They are counted and reported rather than filtered out of the total, and the
list is in the tool where a reader can see it. A measure that quietly excuses
part of what it measures is §10's subject, and "0, plus 66 I decided not to
count" is a different claim from "0" — the row makes both.

That leaves five rows in §1 that ask something other than a pattern: the
ratchet asks the compiler, the stream comparison asks the output, the sweep
asks every rule, `triage.sh` asks which stream moved, and this one asks the
decompiler's own file. None of the five has ever needed correcting. Every row
that has needed correcting — four times, on one line — asked the text what it
looked like.

### The same reference, asked a second question

Once the decompiler's own output is a thing the tools can read, it answers more
than names. `unnamed.py --diff` reports, for each body, how much of it is
still the decompiler's text verbatim — not a count and never zero, because a
short function Hex-Rays got right should stay that way, but a way of asking
*which bodies have had nothing done to them.* That is a question no rule can
answer and that reading seventeen thousand lines answers badly.

Asked the first time it found exactly one body at 1.00 — unchanged, line for
line, through nine rounds:

```c
uint16_t *__init_counter_node(uint16_t *_this)
{
  *(_this + 1) = 8;
  *(_this + 2) = 2;
  ...
  *_this = 22;
}
```

Eight `uint16_t` written through a pointer into a record this file has had
declared since round nine — `CounterNode`, a total and seven counts — and the
caller was casting a `CounterNode *` to `uint16_t *` to hand it over. With the
parameter typed, the cast goes and the body says what the seeds are: seven
counts and their sum. Twenty-two is not a magic number, it is `8 + 2*4 + 3*2`.

The second-least-changed body was the same shape: `rescale_counter_pair` took
a `uint16_t *` and walked `*_this`, `*(_this + 1)` and `*(_this + 2)`, which is
`BitCtr` — `n[0]`, `n[1]`, `limit` — a record with a `static_assert` on its
size and an offset assertion on `limit`. Both callers cast to hand it over.

Nothing is above 0.50 now and the median is 0.13. The top of the list is short
dispatch and cleanup functions where a high ratio means the decompiler had
little to get wrong, not that anything is owed.

---

## 22. The pointer-cast row, and the fifteen casts nothing could see

`pointer casts` is the last figure in §1 that was a bare total, and applying
this round's own lesson to it found two miscounts and one small class of work.

**It counted comments.** The row read `src`, not the comment-stripped text, and
this file quotes the code it replaced on purpose — 28 of the casts it was
counting are in prose about casts that are gone. That is the *same* defect §10
records for the `goto` row, in the row directly above it, uncorrected for three
rounds after the first one was.

**It counted `sizeof(void *)`.** Thirty-three of them, one per layout
assertion — `static_assert(sizeof(void *) != 4 || sizeof(AltP1Block) == …)` —
which is the file saying the 32-bit layout is what it assumes and is not a cast
at all. A lookbehind fixes it.

So 1367 was 1290, and the breakdown says what they are:

```
pointer casts                      1290
  to a scalar                       541
  to a record                       391
  to a scalar, of an address        353
  to a record, of an address          5
```

The 353 `(uint32_t *)&x` are naming a byte inside an object — `*(uint32_t
*)&rec->match[2]` writes four match flags at once because the original wrote
four bytes at once, and no declaration replaces that. The 391 to a record are
the types doing their job at a call boundary. The row is a description now
rather than a target.

### Fifteen casts two cast-removing tools could not see

```c
__transform_planes((BmfImage *)(uint16_t *)__frame.tile_img, ...)
__alt_p1_context((AltP1Block *)(uint8_t **)_this, ...)
__bmf_destroy_archive((BmfArc *)(FILE **)arc, 1)
```

The inner cast is consumed by the outer one before anything can be done with
it. `uncast.py` and `decast.py` both exist to remove casts and both report
zero here, for a reason worth writing down: both are driven by
`-Wuseless-cast`, which fires only when the target type is **identical** to the
operand's. `(uint16_t *)` applied to a `uint8_t *` is not identical to
anything, so the compiler is silent and stays silent — these are useless in a
way it has no word for.

The safety argument is not the compiler's either. On this target a pointer is
four bytes and a cast between pointer types computes nothing, so `(A *)(B *)x`
reaches the same address as `(A *)x` whether `x` is a pointer or an integer.
What `tools/unlayer.py` will not touch is a cast with anything between it and
the next — an inner cast whose result is indexed or dereferenced is doing work,
and the absence of anything between the two closing parens is what says it is
not.

---

## 23. Two tools that had been reporting success on the code they never reached

### `compact_locals.py`, six blind spots deep

Hex-Rays gives every local its own line, and `compact_locals.py` has existed
since round four to merge runs that share a type. Asked how much it had left
to do, the file had **368 pairs of adjacent same-type declarations** still on
separate lines — and running the tool over every body removed **zero**.

It was not broken. It was stopping early, six different ways, and each way it
stopped it reported "0 declaration lines removed", which reads exactly like
nothing left to do:

1. **A trailing comment.** `is_decl` tested `line.endswith(';')` on the raw
   line, so `uintptr_t result;   // the return value` was not a declaration and
   ended the block. After the naming rounds the *first* local in most bodies
   is documented, so most bodies ended at their first line.
2. **A comment on its own line.** These blocks are half prose now; the first
   paragraph was the first statement.
3. **Its own wrapped output.** A merged declaration continues onto more lines
   ending in `,`; neither branch matched one, so a second run stopped at
   whatever the first had merged.
4. **The frame struct.** `struct alignas(16) …{ … } __frame;` sits in the
   middle of the declaration block with locals above and below it. The seventeen
   framed bodies are the largest in the file, so the tool had never run on the
   code that most needed it.
5. **`static_assert`.** A declaration, not a statement, and how this file pins a
   frame's layout — so it sits between the frame and the locals below it.
6. **Hex-Rays' bare `;`.** The empty statement at the top of a body. In a framed
   body it sits *after* the frame and its aliases, with twenty more locals under
   it.

Plus two narrower ones in the same pass: `INIT` did not allow `const` after the
stars (`BmfImage *const img = …`) or a reference (`AltP1Block *&plane1 = …`),
both of which are how this file writes a view bound once — and a signature that
carries the `{` *and* the first declaration on one line, which started the block
in the middle of a declaration.

**282 declaration lines removed**, and nothing outside the frame structs is
mergeable now. Every one of those eight was found by asking a question the tool
does not answer — "how many adjacent same-type declarations are left?" — and
comparing it against what the tool said it had done. A tool that reports zero
is making a claim about the file, and the only way to check it is a second
measure that does not share its blind spots.

The frame structs are deliberately excluded from the residual: a frame's member
order *is* its layout, and merging two members would be a change to the program
rather than to how it reads.

### `bmf.cpp` had eight definitions nothing used

`deadcheck.py` asks this of `subs1.hpp`'s bodies. Nothing asked it of the
header half, and eight had gone dead there: `BMF_STANDALONE` and `_WINDOWS_`,
both switches for `#include`s that are commented out, and `DWORD1`..`DWORD3`
with `SDWORD1`..`SDWORD3`.

Those last six are the argument for the rule. The comment above them says the
bodies "also index DWORD lanes directly" — no body does any more, and **the
three signed ones never could have**: they expand to `SDWORDn`, which this file
has never defined. Three macros that would not compile if anyone used them,
under a comment explaining why they were needed. An unused definition is not
just clutter; it is a claim about the program that nobody has had to keep true.

`tools/undef.py` reports zero now, and reports eight against the revision
before.

---

## 24. What `LODWORD` was hiding

Every one of the 33 `LODWORD`/`HIDWORD` uses in the file was on one of three
members of `choose_plane_coding`'s frame — `q0` and `q1` declared `int64_t`,
`d0` declared `double`. The question they raise is whether those are mistyped
variables or genuine 64-bit fields, and the answer is neither: **they are one
stack slot with two lifetimes of different widths, and both declarations are
right for one of them.**

Before the 3×3 solve the three slots are six independent `int32_t`: two plane
offsets, a row stride, the plane count carried across the search, and the
weight the search is tuning. From `q0 = *(int64_t *)&x2[2]` down they are what
their declared types say — the solve reads all three as `double`, and
`*(double *)&__frame.q0` is that reading written at the site.

The whole-slot reads in the *early* lifetime look like a contradiction and are
not: `img_a[__frame.q0 + 20]` indexes with all sixty-four bits, but pointer
arithmetic on this target discards everything above thirty-two, so it is the
low half — the row stride — whichever way it is written.

So the frame declares a union, which is what the rest of this frame already
does for the slots MSVC shared. Nothing moves: eight bytes either way. The
halves have names taken from what writes them, and `LODWORD(__frame.q1)`
becomes `__frame.plane_a`.

One of the six is still two things in one place: `wt_slot` holds the weight
through the search and then, on the last write before the solve begins, the
end-of-pixels pointer. That is said in a comment rather than split, because
splitting it would move a byte.

### Three macros went with them

With the 33 uses gone, `LODWORD` and `HIDWORD` had none — and removing those
left `DWORDn`, which existed only to define them, with none either.
`tools/undef.py` reported each in turn, which is the cascade an unused-
definition rule is for: the first removal is a judgement, the two after it are
just arithmetic.

`LOWORD`, `LOBYTE`, `HIWORD`, `HIBYTE`, `BYTEn` and `WORDn` stay — 77 uses of
`LOWORD` alone, and every one of them is a genuine partial write to a register
MSVC used at two widths, which §17's rule narrows where it can and leaves
alone where something reads the whole.

---

## 25. Two methods, and a fifty-three-line duplicate

### `SymEntry::set`

The symbol lists write `sym` and then `cnt` to the same entry nineteen times
and never one without the other — MSVC stored the pair with two `mov`s and
Hex-Rays wrote down two statements. Eight more of those pairs are a *copy*:
`p->sym = q->sym; p->cnt = q->cnt;` is `*p = *q;`, and `uncopyrec.py` cannot
see it because that rule is about the 18-byte `bmf_copy` calls and matches on
the call.

`SymEntry` gets a `set(s, c)`, which leaves it an aggregate and three bytes —
the `static_assert` under it is what says so.

### `SymList::rescale`

`encode_symbol_list` and `symbol_list_update` both halve every count, re-sort
what the halving moved, and drop the tail that reached zero. They did it in
**fifty-three lines that differ in one identifier** — the local holding the
count that triggers it, `top` in one and `count` in the other. Diffing them
with that name normalised gives no other difference at all.

Both walk from a local `head`, and `head` can only be `ent`: each body assigns
it three times and always from that member. So the block moves onto the record
whole, and both call sites become one line.

**The ratchet fell five, from 1070 to 1065**, and that is the clearest possible
statement of what a duplicate costs: those five conversions were being counted
twice because they were written twice. `unused.py` then found two declarations
and 28 names orphaned by the move.

### Both tools that police this needed telling about methods

`deadcheck.py` reported `rescale is never called`, from two call sites. Its
call pattern is `\b(__[A-Za-z0-9_]+)\s*\(` — every body recovered from the
binary carries the `__` prefix, and a method reached as `p->rescale(x)` carries
nothing. This is the same blind spot §19 hit with `bit_tree`, where the answer
was the file's `static inline` convention; a method cannot take that route, so
the tool now reads `->name(` and `.name(` as calls too. Checked by mutating a
body's call site in a copy and confirming it is still reported dead.

`checktable.py` caught the two §1 figures the change moved, on the same run.

---

## 26. `_this` was the receiver

MSVC's `__thiscall` puts the object in `ecx`, and Hex-Rays writes it back as a
first parameter called `_this`. Twenty bodies in this file carried one. For
sixteen of them the type is a record this project has already recovered — so
the parameter is not a parameter, and saying so moves every call site from
`__f(x, …)` to `x->f(…)`.

The twentieth is `update_binary_pair`, whose `_this` is a `uint16_t *` into a
counter block rather than a record. It stays a function, which is the honest
answer: there is nothing for it to be a method of.

`tools/methodise.py` does one at a time, because rewriting the call sites
renumbers the file under the next one. The body does not move — a diff that
relocated four thousand lines would hide whatever else it did — and `_this`
becomes `this` rather than nothing, because a body that declares a local with a
member's name would silently change meaning if the prefix simply vanished.

### Two things the gate found and the rule had not

**A call whose arguments wrap.** `decode_pixel` calls `__decode_context_bit`
with a five-line first argument, and a per-line scan cannot see a parenthesis
that closes on another line — so it left the call alone while the function it
named stopped existing. The rewrite works on the whole text now.

**`(&rec[1])->f(x)`.** Taking an address only to follow it is the shape this
change removes, not one it should introduce. `__f(&rec[1], x)` is
`rec[1].f(x)`, in the 29 places that had it.

The 20 casts the change made redundant — `((ModelBlock *)blk)->code_pixel(x)`
where `blk` is already a `ModelBlock *` — took the ratchet up 20 and were
removed by `uncast.py`, which is exactly what it is for. Back to 1065.

### Three tools that all assumed a body is `__name`

The methods are named after their receiver now — `blk->update_model()`,
`rec->encode_symbol(pair, code)` — and getting there needed the same fix in
three places, each a version of one assumption:

* **`rename.py`**, twice. `--member` changes the *rewrite* to reach through
  `->` but not the *existence check*, so a name that only appears as
  `x->name(…)` — every method — was refused as "no such identifier". And
  without `--member` the declaration renames and the calls do not.
* **`deadcheck.py`** reported every new method dead, because its call pattern
  is `\b(__[A-Za-z0-9_]+)\s*\(`.
* **`addrmap.py`**, and this one mattered most. Its `DEF` pattern requires
  `__name(`, so after the methodising commit `real_bodies` returned a shorter
  list than before it — which is exactly the "this commit did more than rename"
  case it declines to guess through. The chain broke and eight bodies lost
  their address.

That last one is worth being explicit about, because nothing would have
complained: `unnamed.py` joins the file to the decompiler's output *through*
that map, so eight bodies would simply have stopped being checked and the row
would have gone on reading zero. `checktable.py` is what caught it —
`conventional ones kept / bodies joined` fell from 64/74 to 50/66, and a
denominator that drops is a measure losing its subject.

One body genuinely has no address: `SymList::rescale`, §25's extraction, was
never one function in BMF.exe — it was two copies of part of two. `addrmap.py`
names it with the other things this project wrote rather than leaving it in a
list of addresses nobody has found.
