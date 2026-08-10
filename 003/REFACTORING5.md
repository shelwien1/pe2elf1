# BMF 2.01 — refactoring plan, round five

Round four ended with no `__m128` anywhere, no frame aliases, no shared stack
slots, `BMF_STRICT` in the gate, and both big objects' sizes pinned by a
`static_assert` against what their allocator asks for. What it did **not** do is
make the arithmetic stop being arithmetic, and it added a spelling of its own:
1857 `__frame.` prefixes where there used to be 265 alias declarations.

Four complaints this round, in the words they arrived in, and what each one
measures to today:

| the complaint | what it is | how many |
| --- | --- | --- |
| "`_this+ofs` with typecasts" | raw byte offsets off a typed base | **1389** sites |
| "mistyped globals" | 4 of 43 globals carry the wrong type; 19 more carry an address for a name | **4 + 19** |
| "context frames in functions" | `__frame.x` in the body, 538 of them through a cast | **1857** |
| "non-stdint types are used again" | `char` 1056, `unsigned char`/`unsigned int` 24 | **1080** |

All four are one thing seen from four sides. A byte offset needs a cast; the
cast needs a type; the type Hex-Rays reaches for is `char *`; and a member whose
type is wrong is reached by offset because its name does not fit. The plan
below takes the *type* first, because it is the only one of the four with a
compiler check attached, and the other three shrink as a consequence.

---

## 1. Where the file is

`python3 tools/shape.py`, verbatim:

```
subs1.hpp / bmf.cpp lines          19098 / 746
raw-offset sites                   1389        ← §3
  off `_this`                      161, in 11 functions
pointer casts                      5131        ← §2, §3
frames                             19, 169580 bytes, 0 aliases
  slots carrying two names         0, 0 extra names, in 0 functions
structs                            85, 77 still ObjN                  ← §3.1
  fNN members / named ones         240 / 60
distinct vNN locals                560
goto / LABEL_n:                    112 / 79
__fwd_* shims                      94
```

and four counts it does not keep, measured by Appendix B:

```
`__frame.` in bodies               1857, 538 of them through a cast   ← §5
globals                            43, 19 named after an address      ← §4
`nNN` locals                       231 distinct, 50 of them `n0xNNN`
__fwd_* shims                      every one `void *` in and a cast out
```

`shape.py`'s cast count is `\(\w+\s*\*+\s*\)` — a one-word type. Allowing
`const` and `unsigned char` finds 5253; the difference is §2.4's 24 and the
`const void *` the CRT shims take.

And the four numbers that are new this round, because round five has a checker
round four did not:

```
-Wsign-conversion                  1578
-Wconversion                        645
-Wsign-compare                      103
-Wuseless-cast                       43
                                   ----
                                   2369 warnings the compiler will point at
```

`BMF_STRICT` reached zero in round three and stayed there, and it only ever
measured one thing: a conversion `-fpermissive` had to forgive. These four
measure what is left, they are the compiler's opinion rather than a regex's,
and **every one of them is a place where the decompilation says a type it does
not mean.**

---

## 2. Phase A — the type vocabulary

### 2.1 Hex-Rays' type names are dead and still declared

`bmf.cpp` carries `defs.h`'s vocabulary: `uint`, `uchar`, `ushort`, `ulong`,
`ll`, `ull`, `int8`, `sint8`, `uint8`, `int16`, `sint16`, `uint16`, `int32`,
`sint32`, `uint32`, `int64`, `sint64`, `uint64`, `_BYTE`, `_WORD`, `_DWORD`,
`_QWORD`, `__int64`, `__int32`, `__int16`, `__int8` — 26 names.

> **`subs1.hpp` uses none of them. Every count is 0.**

They survive because `bmf.cpp`'s own helpers use a handful of them
(`_BYTE`/`_WORD`/`_DWORD` in the `LOBYTE`/`WORDn` macros, `ull` in the overflow
check). Rewrite those helpers in `stdint` and the whole block goes — with one
caveat worth stating, because it is the reason the block was kept this long:
`_BYTE` and friends are what a *future* re-extraction would land on, and
deleting them means a re-extracted body will not compile until it is
translated. That is the right trade now: nothing has been re-extracted in four
rounds, and a vocabulary nothing uses is a vocabulary nobody checks.

### 2.2 `char` — 1056, and three different jobs

Bucketed by how many stars follow the keyword, so the three rows partition the
1056:

| shape | count | what it is |
| --- | --- | --- |
| `char *` | 893 | Hex-Rays' "pointer to memory" |
| scalar `char` | 142 | a value in arithmetic, §2.3 |
| `char **` | 21 | a pointer to one of the above |

713 of the 893 are `(char *)` casts rather than declarations — the
byte-arithmetic idiom §3 is about — but that is a subset of the first row, not
a fourth.

**`char *` is not a byte pointer, it is a decision not to say.** The file
already has 641 `uint8_t *` casts spelling the same address, so which one a
site uses is not a distinction the code makes.

The reason a wholesale retype is *safe* is worth measuring rather than
asserting, because `char` is signed on this target and the two types differ the
moment an element is read as a value:

> **There are zero `*(char *)` dereferences in the file.** Every `char *` is
> re-cast to a width before anything is read through it. 75 sites subscript a
> `char *` local directly, and those are the ones to look at one at a time —
> most are stores of a value the `uint8_t` type describes better than `char`
> does.

`-Wsign-conversion` fires 1578 times and this is a large part of it.

The exception is real and should stay: **`bmf.cpp` hands `char *` to the CRT.**
`strcpy`, `strrchr`, `fopen`'s name, `printf`'s format — those are `char *`
because C says so, not because Hex-Rays could not decide.

### 2.3 Scalar `char` in arithmetic

142 of them, and they are not one thing either:

* **`-(char)x & 31`, 15 sites.** A shift count, truncated to eight bits and
  negated. This is `x86`'s shift-count masking showing through, and `char` is
  doing modular arithmetic rather than holding a character. `(-x) & 31` on an
  `int32_t` is the same value **for every `x`**, not just for small ones: `& 31`
  reads five bits and the truncation to eight preserves them. Checked
  exhaustively over −100000…100000 rather than argued, because the argument is
  the kind that is right until the mask is `& 63`.
* **`(char)(uintptr_t)ptr`, 3 sites.** A pointer's low byte. `(uint8_t)` says
  the same thing without the signedness question.
* **the rest** are `int8_t` or `uint8_t` depending on what they are compared
  against, and `-Wsign-compare`'s 103 warnings are where the compiler already
  knows which.

### 2.4 `unsigned char` and `unsigned int` — 24 sites, all of them a pointer

```c
  v21 = (unsigned int)(lpAddress->f278736[0]);
  v22 = *(uint32_t *)(v21 - 10);
  ...
  lpAddress->f278736[0] = (unsigned char *)(v73);
```

A row cursor converted to an integer so that byte arithmetic can be done on it,
and converted back to store it. `f278736[0]` is a `uint8_t *`; `v73` is a
`uint8_t *`; the integer in the middle is the only thing that is not. §3.3 is
the same finding at 289 sites, and these 24 are the ones that also spell the
type in two words.

### 2.5 The scoreboard, and how it becomes a gate

`test.sh` already runs `BMF_STRICT=1 ./build.sh` and fails on a non-zero count.
Add a second step in the same shape — `BMF_WARN=1 ./build.sh`, counting the four
families — and the phase has a number to work against.

> **Not a number that goes to zero.** `BMF_STRICT` could, because every
> conversion `-fpermissive` forgave was a defect. These four cannot: a
> decompilation of 1997 x86 reinterprets memory on purpose and some of these
> warnings *are* the program. **The deliverable is a documented floor** — every
> warning left is either fixed or has a one-line reason next to it — and the
> gate is a ratchet on the count rather than an assertion that it is zero.

---

## 3. Phase B — `_this + ofs`, and the casts it needs

1389 sites. Round four took this from 1514 and stopped where the plan said it
would: §3.3 and §3.4 wanted the p1 and p2 models read first. This round says
what to read them *for*, because three of the four biggest groups turn out to
be the same table seen from different functions.

### 3.1 The 77 `ObjN` structs are 77 names for a handful of things

> **Every one of the 77 is a pointer-cast target. Not one is ever declared as a
> value.** They are not objects; they are `reinterpret_cast` with a name.

34 of them are produced by a subscript cast off just four bases — `v90` (33
sites), `plane` (10), `Block_plane` (2), `v3` (1) — and 22 of those come off one
expression:

```c
  v551 = (Obj46 *)(&v90[4 * (v81 ^ 0x800) + 284712]);
  v552 = (Obj47 *)(&v90[4 * (v81 ^ 0x400) + 284712]);
  v554 = (Obj48 *)(&v90[4 * (v81 ^ 0x200) + 284712]);
```

`v90` is `(char *)v578 + v78` with `v578` an `Obj11 *`, and `+284712` is
`Obj11::f284712` — the `P2Count[163840]` table round four named. So each of
these is `&obj->f284712[v78 / 4 + (v81 ^ mask)]`, and the structs agree:

```c
struct Obj46 { const char f0; uint8_t _pad1[3]; const char f4; };
struct P2Count { int8_t b0; uint8_t b1; int16_t w2; };   // four bytes
```

`Obj46::f0` is `P2Count[0].b0`, `Obj46::f4` is `P2Count[1].b0`, and the
`*((int16_t *)p + 1)` beside every one of them is `P2Count[0].w2`. The readers
prove it — they are the expression round four already identified (`v530` here
is an `Obj47 *`, whose declaration is character-for-character `Obj46`'s, which
is the point):

```c
  v340 = v550 - ((v339 + (1 << ((v530->f0 + 31) & 31))) >> (v530->f0 & 31));
```

which is `((1 << ((b0 + 31) & 31)) + w2) >> (b0 & 31)`, the same five-bank read
`alt_p2_context` does.

**So the deliverable is: 22 structs deleted, their pointers become
`P2Count *`, and 24 sites say which counter they mean.** The XOR is the
context-perturbation the model uses to reach a neighbouring counter, and naming
it is the first thing in this file that would say what the p2 model *does*.

The remaining 55 want the same treatment one at a time: a struct used four
times is a declaration, a `static_assert`, and two casts, and what it is worth
depends entirely on whether the thing it points at has a name yet.

### 3.2 `Obj26` is a cursor, and `alt_p1_model`'s table is still unnamed

```c
  v90 = (Obj26 *)((uint16_t *)((char *)_this + 16 * (v89 + _this->f12[30])));
  ((uint16_t *)v90)[n5_2 + 1901] += 6;
  v90->f3800 += 6;
```

`Obj26` declares three `uint16_t` at +3784, +3800, +3816 — sixteen apart, so
records 236, 237 and 238 of a 16-byte table, field +8. The `[n5_2 + 1901]`
beside it is byte `2 * n5_2 + 3802`, which is field +10 of record 237.

This is REFACTORING4.md §3.3's finding — "record 237 + v8, field 4 of a
16-byte-record table at +0" — and it is still open for the same reason: writing
`rec[v8 + 237][4]` is not more readable than the arithmetic *until the record
has fields with names*. 115 sites read `f3784`/`f3800`/`f3816`; 85 raw offsets
sit beside them. `algorithm_v2.md` §9 is where the read goes, and this round
should do it rather than defer it a fourth time.

It is not the largest group — `alt_p2_model` has 322 raw offsets to
`alt_p1_model`'s 85, and `alt_model_p2_decode` (109), `alt_model_p2_encode`
(107) and `alt_p2_d8_encode_body` (104) also beat it. What makes it the one to
do first is that it is the only one of the five whose blocker is a *reading*
rather than a tool: the other four are §3.1's and §3.3's, and both of those are
mechanical once the type is right.

### 3.3 289 integers that hold addresses

```c
  v21 = (unsigned int)(lpAddress->f278736[0]);
  v22 = *(uint32_t *)(v21 - 10);
```

Nine functions, and four of them account for 258: `alt_model_p2_decode` (75),
`alt_model_p2_encode` (75), `alt_p2_model` (58), `alt_p2_context` (50).

Every one of these forces two casts — one to get the address into the integer
and one to get it back out — so this is 289 sites generating ~578 of the 5253
pointer casts. Retyping the local to `uint8_t *` deletes the pair: the
assignment becomes `v21 = lpAddress->f278736[0];` with no cast at all, and
`*(uint32_t *)(v21 - 10)` keeps the one cast it needs — the one that says how
wide the read is — instead of also needing one to make `v21` addressable.

**§7's first hazard applies at full force**: the moment the local is a pointer,
`v21 - 10` scales. It scales by one on `uint8_t *`, which is what the integer
was doing, so the retype is exact — *provided* the target type is a byte
pointer and not the `uint32_t *` the cast beside it mentions. Getting that
backwards is a four-byte stride where a one-byte one belongs, and the gate will
say so on the first image.

### 3.4 303 scaled-index sites

186 of the `((T *)p)[expr + K]` shape and 117 of `(T *)p + K`, and **they are
two different things** that the constant tells apart. `K × width` is either an
offset from the object's base or an offset inside a record:

```c
  ((uint32_t *)v385)[2 * v386 + 69677]   // 69677 x 4 = +278708, a member
  (uint16_t *)v387 + 470040              // 470040 x 2 = +940080, a member
  ((char *)a2)[4 * i + 2]                // 2, a field in a four-byte record
```

**282 are members and 21 are record fields** (Appendix B splits them at
`K × width < 256`, which separates the two cleanly here — the smallest member
offset in the set is 257 and the largest record field is 4).

The 282 are members reached by dividing the offset by the width and hoping the
reader multiplies it back. `unoffset.py` does not see them because the base is
not `_this` and the index is not constant; `unlane.py`'s approach does apply,
and extending it to a variable index plus a constant is the tool work this
section needs.

The 21 want the opposite treatment: `((char *)a2)[4 * i + 2]` is `rec[i].f2` of
a table `a2` contains, so what they need is the record declared — the same
thing §3.2 needs and the same thing round four did for `P2Count`.

### 3.5 What is left off `_this`

161 sites in 11 functions, led by `alt_p1_model` (48), `alt_p2_alloc` (31) and
`symbol_list_update` (24).

`alt_p2_alloc`'s 31 are **not** one thing: 14 of them seed the two counter
tables round four named, and the other 17 land on `+232`, `+278720`, `+278724`,
`+278732`, `+278736`, `+278756`, `+278944`, `+278948`, `+279984`, `+280752`,
`+280753` and `+278784` through `+278848`. Every one of those *is* a member of
`Obj11` today.

> **So the blocker is the tool, not the code.** `merge.parse` returns `None`
> for `Obj11` because `Obj11` has a union, so `unoffset.py` sees *zero* `Obj11`
> members and folds nothing in any of the eleven functions that take one.
> Teaching `merge.parse` to walk a union — each layer at the union's own offset,
> the first layer to name a byte wins — is the smallest change in this plan with
> the largest reach.

One of the 17 is worth a second look on its own: `*(uint32_t *)((char *)_this +
232) = 0x3F800000` writes a `1.0f` at offset 232, which is inside the
`_pad0[278528]` `Obj11` declares as padding. Either the pad is not all padding
or that write is not to an `Obj11`.

---

## 4. Phase C — the globals

43 file-scope globals. **19 are still named after the address they had in
BMF.exe or the constant they were seen holding**: `__dword_439880`,
`__byte_439890`, `__byte_4398A0`, `__dword_439B7C`, `__byte_439BC0`,
`__byte_439BC8`, `__byte_439BD0`, `__dword_439BD8`, `__dword_443388`,
`__byte_445700`, `__n8_1`, `__n8_0`, `__dword_44108C`, `__dword_441090`,
`__n2_4`, `__dword_441098`, `__n7_0`, `__n7_1`, `tbl44573C`.

A name that is an address says two false things: that the address matters, and
that nobody knows what the value is for. Six of the nineteen are `constexpr` —
they are the command-line settings the mode reduction pinned (`__n7_0` is -Q9,
`__n2_4` is the -T mode) and their *values* are documented in REFACTORING.md
§2. Those want a name each and nothing more.

Four are mistyped rather than misnamed:

| global | declared | what it is |
| --- | --- | --- |
| `packer_acc` | `int32_t` | a bit accumulator: six of its 79 uses cast it to `uint32_t` to shift it right |
| `packer_free_bits` | `int32_t` | a shift count: six uses cast it to `char` for the `-x & 31` idiom of §2.3 |
| `__dwLowDateTime` | `int32_t` | holds a pointer — `(void *)__dwLowDateTime` twice, `(uintptr_t)` once |
| `bmf_pout_of_memory_handler` | `uint8_t[4]` | a function pointer, and the last of round three's triple-declared tables |

And one class of pure noise: **17 casts of a `uint8_t` field to `uint8_t`**.
`level_geom`'s three members became `uint8_t` in round three and the 17
`(uint8_t)level_geom[k].half` sites did not follow. `ctx_group_flags`,
`__byte_439890`, `__byte_4398A0` and the three `__byte_439BCx` have the same
thing, two sites each — 29 in all.

> **`-Wuseless-cast` does not find any of them.** Its 43 are every one a
> *pointer* cast: 14 `uint8_t*`, 11 `uint16_t*&`, 8 `void*&`, and nine others.
> GCC will not call `(uint8_t)x` useless when `x` is a `uint8_t`, because
> integer promotion has already made the operand an `int` by the time the cast
> is applied, so the cast is a genuine narrowing as far as the front end is
> concerned. Checked on a four-line program, not inferred.

So this class needs a regex and the 43 need the compiler, and they are two jobs
rather than one. Both are cheap; neither checks the other.

---

## 5. Phase D — the context frames

Round four deleted 265 alias declarations and put the member in the body, which
is what its §5 asked for. The result is 1857 `__frame.` prefixes, and the
complaint is fair: a body that says `__frame.v246` where it used to say `v246`
has traded one kind of noise for another. What makes the trade worth keeping is
that the noise is now *in one place per site* instead of split between a
declaration and a use — and that it made two things visible that were not.

| frame | `__frame.` uses | of them cast | bytes |
| --- | --- | --- | --- |
| `choose_plane_coding` | 550 | 0 | 41 456 |
| `search_filter` | 255 | 0 | 164 |
| `code_pixel` | 214 | **213** | 164 |
| `decode_pixel` | 182 | **181** | 164 |
| `cost_candidate` | 147 | 0 | 26 712 |
| `reduce_alphabet` | 141 | **79** | 66 064 |
| … 13 more | 368 | 65 | |

### 5.1 The 538 casts are five spill arrays

```c
  ((ModelBlock * &)__frame.slot[7]) = (ModelBlock *)((int32_t)Blockaa_1);
  v10 = ((uint8_t * &)__frame.slot[2]);
  v13 = (void *)(((uint32_t &)__frame.slot[4]) & *(uint32_t *)v10);
```

`decode_pixel`'s `sym[32]`, `code_pixel`'s `sym[32]`, `reduce_alphabet`'s
`slot[19]`, `decode_symbol_list`'s `list[8192]` and `alt_p2_context`'s `sub[6]` —
**87 elements reached by a constant index, with two to five distinct types
between them.** That is not an array. It is the stack area MSVC packed several
locals into, and round three declared it as an array because an array is what
the byte range looked like.

Splitting one into named members is exactly what round four's `unslot.py` did
for the 25 shared *slots*, and the same discipline applies: propose, run the
whole gate, keep or put back. The difference is that these are array elements,
so the split changes the member list rather than the type of one member.

Two of the five need a decision before a sweep can touch them:

* **`decode_symbol_list`'s `list[8192]`** is both. `list[0..7]` are seven
  scratch locals with three types between them; `list[8..8191]` is the symbol
  list the function is named for. Two members, not 8192 and not seven.
* **`reduce_alphabet`'s `slot[19]`** is the only one with *walks* as well: ten
  of the nineteen are reached by a constant and nine sites index it by an
  expression — `__frame.slot[2 * j + 2]` — which is why its declaration already
  says "one array, three bases". A split has to keep the walked run contiguous,
  so the question is which of the nineteen the walk reaches, and that is a read
  of the alphabet reduction rather than a sweep.

### 5.2 Two frames are entirely dead

`alt_model_p2_encode`'s frame is 324 declared bytes — 336 with the padding
`alignas(16)` adds, which is what its `static_assert` pins — of `_gapN` and
nine `slotN[16]`, and **nothing in the body reads it**. `transform_planes`'s is
76 declared and 80 pinned, same story.
Both are what is left after four rounds of lifting members out one at a time,
and both can go with their `static_assert`s.

### 5.3 A member whose extent is a lie

`reduce_alphabet` declares `char buf[4]` and reads
`*(uint32_t *)&__frame.buf[8 * v11]` — twelve times, with `v11` running over
the alphabet. `choose_plane_coding` has five more of the same shape. The frame
is 66 064 bytes and the four in `buf` are the only ones with a number attached,
so the walk is into whatever follows. Round three's rule applies — *when a walk
looks unbounded, the frame usually knows the bound* — and here the bound is the
next declared member.

---

## 6. Order

```
  Phase A   the types                 the only phase with a compiler check;
     │                                everything downstream is easier once a
     ▼                                byte pointer is a byte pointer
  Phase B   _this + ofs               3.1 first: it deletes 22 structs and
     │                                needs nothing but arithmetic.  3.2 wants
     ▼                                algorithm_v2.md §9 and should not be
  Phase C   the globals               deferred a third time
     │
     ▼
  Phase D   the frames                5.2 is free, 5.1 is a sweep, 5.3 is a read
```

A before B because §3.3's 289 retypes are a type decision, and doing them while
`char *` still means "some memory" is how a one-byte stride becomes a four-byte
one. C is independent and is the cheapest thing here — §4's 43 useless casts
are an afternoon. D last, because 5.1's sweep is cheaper once the frames stop
being the only place a type is written down.

Within each phase: **one function, one struct, one frame at a time, gated** —
and the gate now includes the strict build and should include the warning
count.

---

## 7. What would make this fail

The five from rounds two and three, all still live:

* **Pointer arithmetic scales.** `f1078208 + 24 * n` steps 24 bytes on a byte
  pointer and 96 on a `uint32_t *`. §3.3 retypes 289 integers into pointers and
  every one of them is this hazard.
* **Two names for one field can disagree**, and the merge is the moment to
  resolve it by reading. §3.1 merges 22 structs into one; the merge is only
  safe because the *readers* agree, not because the declarations do.
* **Signedness is part of a field's type.** §2.2's `char *` → `uint8_t *` is
  1000 sites of exactly this, and `-Wsign-conversion` is the first tool this
  project has had that can see it.
* **A merge retypes fields and the locals downstream cascade** — into
  `-fpermissive` conversions the gate can now see, because round four put
  `BMF_STRICT` inside it.
* **A rewrite that keeps the offset can still change the width.** Round four hit
  this in a new shape: rewriting `m128_i32[2]` before `m128_i64[1]` collapsed
  two views onto one subscript and widened a store. One stream moved.

And three this round adds:

* **A warning count is not a defect count.** Some of these 2369 are the program
  — a decompilation reinterprets memory on purpose. Taking a warning to zero by
  adding a cast is worse than leaving it, because the cast hides it from the
  next round. The deliverable is a floor with reasons, not a zero.
* **`char *` is load-bearing at the CRT boundary.** `strcpy`, `strrchr`,
  `fopen`, `printf` take `char *` because C says so. A sweep that retypes every
  `char *` in the file will break those, and it will break them at compile time,
  which is the good case — but only if `BMF_STRICT` is on, because
  `-fpermissive` forgives exactly this.
* **A measurement of the declarations is not a measurement of the code.** This
  is round four's, and it cost a whole item: 156 of 265 frame aliases shared
  their member's name, which looked like a reason not to fold them and was not.
  §5's 1857 and §3.1's 77 are declaration counts. Check the *sites* before
  deciding either is worth its section.

---

## 8. What the gate needs

Nothing new in what it runs — fifteen streams, the two-member archive, fifteen
malformed inputs, the out-of-memory ladder, `BMF_STRICT` — and one addition to
how it is run:

* **`BMF_WARN=1 ./build.sh` counts `-Wconversion -Wsign-conversion
  -Wsign-compare -Wuseless-cast` and `test.sh` fails when the count rises.** A
  ratchet rather than a target, for the reason §7 gives. Round three lost
  `BMF_STRICT`'s zero twice before it was inside the gate; this is the same
  lesson applied before it costs anything.

Coverage is not a worry for any of these phases and it is worth saying why:
§2's types are in every function; §3.1's table is read on every image by the p2
model; §3.3's four functions are the p2 encoder and decoder;
`REFACTORING2.md` §5 measured `alt_p2_model`, `alt_p2_context` and
`alt_p2_filter` at 100 % line coverage and `choose_plane_coding` at 94.8 %.

---

## Appendix A — how the numbers were measured

| number | command |
| --- | --- |
| the §1 table | `python3 tools/shape.py` |
| raw offsets by function | `python3 tools/shape.py --this` |
| one line per frame | `python3 tools/shape.py --frames` |
| the four warning families | `-Wconversion -Wsign-conversion -Wsign-compare -Wuseless-cast`, counted per `[-Wname]` tag |
| type spellings in code | strip `//` first, then count — `char` inside a comment is not a type |
| `char` by star count | `re.finditer(r'\bchar\b(\s*\**)')` and bucket on `count('*')`: 142 / 893 / 21 |
| `*(char *)` dereferences | `grep -c '\*(char \*)'` — 0, which is why §2.2's retype is safe |
| whether `-(char)x & 31` is `(-x) & 31` | a four-line program over −100000…100000, not an argument |
| whether `-Wuseless-cast` sees a `(uint8_t)` on a `uint8_t` | a four-line program; it does not |
| what `-Wuseless-cast` does see | `sed -E "s/.*to type '([^']*)'.*/\1/"` over its output: all 43 are pointer casts |
| why `unoffset.py` folds nothing in `Obj11` | `merge.parse(src, 'Obj11')` returns `None` — it cannot walk a union |
| Hex-Rays type names still used | `grep -c '\bnameformula\b'` per name over both files; all 26 are 0 in `subs1.hpp` |
| globals and their types | the `^static …;` regex in Appendix B |
| `ObjN` never declared as a value | Appendix B |
| `__frame.` uses and casts per frame | Appendix B |
| conversions | `BMF_STRICT=1 ./build.sh` (0 today, and inside `test.sh`) |
| what still folds to a member | `python3 tools/unoffset.py subs1.hpp --list` |
| what still lifts out of a frame | `python3 tools/frame-sweep.sh --arrays` (0 of 13) |
| members still read through a cast | `python3 tools/unmemcast.py subs1.hpp --list` |

## Appendix B — the four experiments this plan rests on

**Every `ObjN` is a cast target and never an object:**

```
python3 - <<'EOF'
import re
s = open('subs1.hpp').read()
code = '\n'.join(l.split('//')[0] for l in s.split('\n'))
objs = re.findall(r'^struct (Obj\d+) \{', s, re.M)
val  = [n for n in objs if re.search(r'\b%s\s+\w' % n, code)]     # `ObjN x;`
print(len(objs), 'structs,', len(val), 'ever declared as a value')  # 77 0
EOF
```

**Which of them are the p2 counter table** — the base, the offset and the
element width all have to agree:

```
python3 - <<'EOF'
import re, collections
code = [l.split('//')[0] for l in open('subs1.hpp')]
c = collections.Counter()
for l in code:                       # the cast, then anything, then the index
    for m in re.finditer(r'\((Obj\d+) \*\)[^;]*?\[4 \* [^\]]*\+ 284712\]', l):
        c[m.group(1)] += 1
print(len(c), 'structs,', sum(c.values()), 'sites')   # 22 24
EOF
```

The `[^;]*?` in the middle is not laziness: half the sites put an `(int32_t)`
between the cast and the address — `(Obj44 *)((int32_t)&v90[4 * v92 + 284712])`
— which is §3.3's integer-holding-an-address in its shortest form.

`+284712` is `Obj11::f284712`, the base is `(char *)obj + v78`, and the element
is four bytes — which is `sizeof(P2Count)`.

**What `K` means in §3.4's 303**, which is the difference between a member and a
field and is the constant alone:

```
python3 - <<'EOF'
import re
W = {'uint8_t':1,'int8_t':1,'char':1,'uint16_t':2,'int16_t':2,
     'uint32_t':4,'int32_t':4,'uint64_t':8,'int64_t':8,'float':4,'double':8}
A = re.compile(r'\(\(\s*(\w+)\s*\*\)\s*(\w+)\)\[([^\]]*?)\+\s*(\d+)\]')
small = big = 0
for l in open('subs1.hpp'):
    for m in A.finditer(l.split('//')[0]):
        if m.group(1) in W:
            off = int(m.group(4)) * W[m.group(1)]
            small, big = (small + 1, big) if off < 256 else (small, big + 1)
print(small, 'record fields,', big, 'member offsets')     # 21 165, +117 from the other shape
EOF
```

The gap between the two populations is wide — the largest field offset is 4 and
the smallest member offset is 257 — so the 256 threshold is reading the data
rather than choosing where to cut it.

**Integers that hold addresses**, which is where §3.3's 289 comes from: a local
declared as an integer and then dereferenced.

```
python3 - <<'EOF'
import re, sys, collections
sys.path.insert(0, 'tools'); import structs
lines = open('subs1.hpp').read().split('\n')
tot = 0
for a, b, nm, _ in structs.bodies(lines):
    txt = '\n'.join(l.split('//')[0] for l in lines[a:b + 1])
    ints = set()
    for m in re.finditer(r'^\s+(?:u?int(?:32|ptr)_t|unsigned int)\s+([\w, *]+);', txt, re.M):
        ints |= {d.strip() for d in m.group(1).split(',') if not d.strip().startswith('*')}
    tot += sum(1 for m in re.finditer(r'\*\(\s*[\w ]+\**\s*\*\)\(\s*(\w+)\s*[-+]', txt)
               if m.group(1) in ints)
print(tot, 'dereferences of an integer local')          # 289
EOF
```

**Which frame members are a spill area rather than an array** — a constant
index every time, and more than one type between the elements:

```
python3 - <<'EOF'
import re, sys, collections
sys.path.insert(0, 'tools'); import structs, unalias
lines = open('subs1.hpp').read().split('\n')
for a, b, nm, _ in structs.bodies(lines):
    fr = unalias.frame_of(lines, a, b)
    if not fr: continue
    const, var, types = collections.defaultdict(set), collections.Counter(), \
                        collections.defaultdict(set)
    for i in range(fr[1] + 1, b + 1):
        code = lines[i].split('//')[0]
        for m in re.finditer(r'__frame\.(\w+)\[([^\]]*)\]', code):
            (const[m.group(1)].add(int(m.group(2))) if m.group(2).strip().isdigit()
             else var.update([m.group(1)]))
        for m in re.finditer(r'\(\(([\w ]+\**)\s*&\)__frame\.(\w+)\[\d+\]\)', code):
            types[m.group(2)].add(m.group(1).strip())
    for mem in const:
        if len(types.get(mem, ())) > 1:
            print('%-22s %-8s %2d slots, %d types%s'
                  % (nm.lstrip('_'), mem, len(const[mem]), len(types[mem]),
                     ', and %d walks' % var[mem] if var[mem] else ''))
EOF
```

Five members, and the walk count is the reason the fifth is harder than the
other four:

```
alt_p2_context         sub       6 slots, 2 types
reduce_alphabet        slot     10 slots, 5 types, and 9 walks
decode_symbol_list     list      7 slots, 3 types
decode_pixel           sym      32 slots, 3 types
code_pixel             sym      32 slots, 4 types
```

`REFACTORING3.md`'s Appendix B is the standing warning and it still applies:
every count in a plan should be reproducible by a command that does not go
through the tool being checked. Round four's plan had five claims the work
disproved — two guesses a neighbouring line settled, one miscount, one
instruction the tree appeared to disprove and did not, and one prediction the
sweep beat — and the ones that survived were the ones with an experiment
attached.
