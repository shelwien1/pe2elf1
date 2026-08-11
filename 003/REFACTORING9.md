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
subs1.hpp / bmf.cpp lines            17787     17616
raw-offset sites                        22        12
byte offsets on a typed base           121         0
pointer casts                         2137      1545
fNN members / named ones             93/121     5/162
distinct vNN locals                    554       545
  bodies still carrying one              —   13/102
  vNN uses                                —      9541
goto / LABEL_n:                     112/79     81/55
conversion warnings (ratchet)         1455      1331
```

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

A figure a comment can move is not a measurement, and a figure that cannot move
is not one either. That is the whole of §10.

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

No new tools. Two fixes and one scan that lives in this document rather than in
`tools/`, because it is eleven lines and its answer is now zero:

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

### What is actually left

Thirteen bodies, and they are the model itself: `alt_p2_model` (481 distinct
names), `alt_p2_context` (184), `choose_plane_coding` (177), `alt_p1_model`
(104), `decode_pixel` (100), `alt_model_p2_encode` (91), `search_filter` (81),
`code_pixel` (78), `alt_model_p2_decode` (74), `cost_candidate` (72),
`alt_model_p1_encode` (60), `reduce_alphabet` (54) and `alt_model_p1_decode`
(47).

This is where round eight's claim finally is true. Their locals are intermediate
values of an arithmetic whose meaning is `algorithm_v2.md`'s subject, not a
sweep's — and the difference from the forty-five bodies above is not size but
that those had a *shape*: a margin mirror, a cursor rotation, a fold table, an
inverse. These have an equation.
