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
subs1.hpp / bmf.cpp lines            17787     17616         17343
raw-offset sites                        22        12            11
  off `_this`                            —         1             0
byte offsets on a typed base           121         0             0
pointer casts                         2137      1545          1387
fNN members / named ones             93/121     5/162         0/171
distinct unexplained locals            554       591             0
  bodies still carrying one              —    8/102         0/102
  uses                                    —      6302             0
goto / LABEL_n:                     112/79     81/55         49/34
  restart a loop / exit N blocks         —         —         16/29
  jump into a block / sideways           —         —           1/3
conversion warnings (ratchet)         1455      1331          1238
```

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
| `unify_types.py` | write only when there is something to change (§15) |
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

What is left is 502 `-Wconversion` and 736 `-Wsign-conversion`, which are the
decompilation's type mixture proper — a value narrowed or resigned on
assignment, several hundred times.

The same tool would zero those too, and it is not going to, which is worth
being explicit about because the distinction is thin and the temptation is not.
A cast on a comparison *adds* something: `x < width` and `x < (uint32_t)width`
look like the same operator and are not, and no reader recovers that without
knowing the conversion rules. A cast on an assignment adds nothing — `x = y`
with the two declared types visible already says the value is being narrowed,
and writing `(uint16_t)` in front of it moves no information anywhere. It would
move the number, which is the entire objection: §10 is about measures you can
satisfy without doing the work, and putting 1238 casts in this file to make a
scoreboard read zero is the purest example of one this project could produce.

The number goes down when the types are right, and that is a round of its own.

**49 jumps, 45 of which are what `goto` is for.** The breakdown row in §1 is
there so a later round can see whether the other four have grown.

**11 raw-offset sites**, all of them a byte buffer being indexed — the counter
has said since round three that it is a counter and not a target, and
`exclusion_mask[symbol]` will never become a member.

**1387 pointer casts**, none of which the compiler thinks is useless.

What is *not* left: a Hex-Rays name, anywhere in either file; an `fNN` member;
a raw offset off `_this`; a frame that carries two names in one slot; a rule in
`tools/` that finds anything.
