# BMF 2.01 — refactoring, round eight

Round seven ended with a list of five things left, and said of the largest of
them — 93 `fNN` members and 559 `vNN` locals — that both were "answerable only
by knowing what the values mean, which is `algorithm_v2.md`'s work". That was
right, and it is why this round is mostly `algorithm_v2.md`'s work: the two
alternate-model blocks carried eight overlapping readings of two objects, and
taking them apart answered a question about the model that six rounds of
sweeps had not.

The finding is in `algorithm_v2.md` §9.1 and it is short: both alternate
families compose their table index as a **base-3 number**, nine digits for p1
and five for p2, each digit the sign of a difference between causal
neighbours. `3^9 · 32 = 629856` is the number of counter nodes `alt_p1_alloc`
allocates. The table size was never an independent fact; it is the context
space, and the layout was hiding it behind four spellings of the same 216
bytes.

---

## 1. Where the file is

`python3 tools/shape.py`, verbatim:

```
subs1.hpp / bmf.cpp lines          17759 / 358
raw-offset sites                   26
  off `_this`                      1, in 1 functions
pointer casts                      2329
globals still at a 1997 address    0
frames                             17, 169180 bytes, 0 aliases
  slots carrying two names         0, 0 extra names, in 0 functions
  member runs walked as arrays     0 sites, 0 bases, 0 functions
  frames that dissolve outright    17, 0 aliases
structs                            20, 0 still ObjN
  fNN members / named ones         78 / 87
distinct vNN locals                559
goto / LABEL_n:                    112 / 79
__fwd_* shims                      0
```

against 37 raw offsets, 2541 pointer casts and 83 `fNN` members where round
seven's last commit left them. (§1 of that document quotes 44 / 2822 / 93,
from earlier in the round.) The conversion-warning ceiling went 1975 → 1442.

Nothing here was a sweep. Every tool in `tools/` reports zero removals against
the file at the end of this round, exactly as at the start -- `unwrite` finds
two write-only loads and declines both, and that is the whole of it. The
numbers moved because four objects were re-read, not because anything was
found by pattern.

---

## 2. The two expressions the p2 counter is made of

`P2Count` is four bytes — a shift `b0`, an unread `b1`, a counter `w2` — and
between them two expressions over it are most of what the three big model
bodies say.

```c
static inline int32_t p2_pred(int32_t w2, int32_t rate) {
  return (w2 + (1 << ((rate + 31) & 31))) >> (rate & 31);
}
static inline int16_t p2_bump(int32_t w2, int32_t err, int32_t shift) {
  const uint32_t kick = 32u * (uint32_t)((err > deadzone_hi) - (err < deadzone_lo));
  return (int16_t)((uint32_t)w2 + ((kick + (uint32_t)err + (1u << (shift - 1))) >> shift));
}
```

150 sites and 117. Hex-Rays spelled the first nine ways: three add the two
terms the other way round, five reach the rate through a `LOBYTE` copy into a
wider temporary, and one kept the rounded counter in a named temp and shifted
it three times inside one statement. All nine are the same arithmetic, and the
`LOBYTE` copies fold away because both masks are mod 32 — a copy can only carry
the byte it writes into the shift.

**The one thing a helper could have changed here is the shift**, and it is
worth saying how that was settled rather than assumed. Passing an unsigned
counter to an `int32_t` parameter turns a logical shift into an arithmetic one,
and the two spellings look identical. `-Wsign-conversion` answers it: after the
substitution, no diagnostic in `warn.log` quotes either call, so no site passes
a `uint32_t` and every original `>>` was already on `int`.

The substitution also matched the helper's own body, which is a hazard worth
recording because the compiler cannot see it:

```c
static inline int32_t p2_pred(int32_t w2, int32_t rate) {
  return p2_pred(w2, rate);          // was the expression itself
}
```

It builds clean, and it tail-calls into an infinite loop. The gate found it as
a hang rather than a crash, which took longer to read than a segfault would.

---

## 3. Four readings of the p2 planes, and five of the p1 ones

`AltP2Block` had four overlapping views of +278736..+278855 and `AltP1Block`
had five of +0..+215. They are the same object twice, and the same three
members answer both.

What settles it is not the offsets but the *operations*. Three of `AltP2Block`'s
four readings contain the same three `memcpy`s and the same rotation:

```
memcpy(f278760_p, f278756, Size)   memcpy(*(uint8_t **)&f278760[0], f278756, ...)   memcpy(f278736[6], f278736[5], Size)
```

are one statement written three ways, and the five-way pointer rotation that
ends a row appears in all three. `alt_p2_alloc` allocates exactly five buffers
of `18 * width + 234` bytes — `sizeof(P2Ctx)` times `width + 13` — and derives
five cursors 144 bytes in, which is one row of eight records. So the twenty
bytes are five planes and the twenty before them are their cursors:

```c
uint8_t  *cursor[5];
uint8_t  *buf[5];
CtxWeight ctx_w[5];     // nine, in AltP1Block
```

`AltP1Block` went the same way and further: it now has **no unions at all**,
where the round started with four. `f0[8]` and `f4`/`f8`/`f12[]` were the same
eight words twice — `f0[k]` is `f12[k - 3]` for k >= 3 — and what made them
look like two readings is a costume, §5.

Just over 1200 references were rewritten across the two blocks -- 497 in the
p2 one, 674 in the p1 one, and 43 more when `f0` followed. Six new
`static_assert`s pin where each run starts, because what made four readings
possible in the first place was that nothing said where the pointers stopped.

---

## 4. The weights, which are place values

The eighty bytes after the p2 planes were `uint32_t f278760[24]`, and the
hundred and forty-four after the p1 header were 144 bytes of declared padding
in one reading and `f12[5 .. 40]` in another. Both are groups of four:

```c
struct CtxWeight { int32_t sel; uint32_t w[3]; };
```

The evidence is an index, not a size. Five sites read `t[t[4g] + 4g+1]` for
g = 0..4 in the p2 block and nine read `f12[6 + f12[5]]` and its eight
relatives in the p1 block — a slot, then three slots one of which that slot
chooses. Seven more p2 sites reached the same weights through raw
`((uint32_t *)block)[sel + 69695]` indices, one constant per group, which is
the same selection with the base folded into the constant.

Then the values say what the sum is for. `alt_p1_alloc` writes 0, 32·3^g and
64·3^g for g = 0..8, and `alt_p2_alloc` writes 0, 64·3^g and 128·3^g for
g = 0..4. Those are place values: nine ternary digits at 32 counters each, five
at 64. Both then check against a table size that was already in the file and
had never been connected to anything. `3^9 · 32 = 629856` is the `CounterNode`
count two hundred lines above, `0x99C60 · 16` bytes of them. `3^5 · 64 = 15552`
is `f940072`, declared `uint16_t[62208]` and seeded by a loop that runs
`0x1E60` = 7776 times writing eight words a pass -- 15552 records of four.
Neither size is an independent fact about the model; each is its context
space, and the layout was what kept that from being visible.

`alt_p1_context` then ends with the sum itself, which had been one statement
nested nine deep:

```c
result = (int32_t)&(*(uint8_t **)&_this->ctx_w[8].w[v29])[16 * v37
       + 8 * (*((uint8_t **)_this + (uint32_t)*(uint8_t **)&_this->f0[5] + 6) == nullptr)
       + (uint32_t)&(*(uint8_t **)&_this->ctx_w[6].w[v41])[ ... ]]
```

Every `&(*(uint8_t **)&w)[rest]` is `w + rest`. The nesting order is the order
the registers freed up in, not an order the arithmetic has, and written flat it
is the nine chosen weights added together plus three terms below the place
values.

---

## 5. A costume, and the one cast that was not one

`alt_p1_context` writes and reads its selectors through `uint8_t **`:

```c
*(uint8_t **)&_this->ctx_w[0].sel = (uint8_t *)(((216 - (uint32_t)v4) >> 31) + ...);
v19 = *(uint8_t **)&_this->ctx_w[3].sel == nullptr;
```

Nothing there is a pointer. `v4` is a byte read from a plane, the selector is a
sign, and the whole function is integers in pointer type because MSVC kept them
in address registers. 27 writes, 10 reads and 8 more casts on `cursor` came
off, and five locals typed `uint8_t *` — never dereferenced in the function —
became `int32_t`.

**One of those casts was load-bearing, and dropping it broke four streams.**

```c
_this->ctx_w[0].sel = ((216 - (uint32_t)v4) >> 31) + ((22 - (uint32_t)v4) >> 31);
```

`(216 - (uint32_t)v4) >> 31` is a **logical** shift: the sign bit as a 0-or-1
flag, which is exactly what a base-3 digit is. With `v4` retyped to `int32_t`
the same text is an **arithmetic** shift, so the flag is 0 or −1, the selector
lands at −1 or −2, and `w[-1]` reads whatever is in front of the group. One
stream changed, one segfaulted, two divided by zero.

The rule that came out of it is narrow and checkable: `(int32_t)x` on an
`int32_t` is identity and can go; `(uint32_t)x` never can, because it is what
decides whether a shift is logical. The rewrite strips only the first.

This is the same hazard §2 checked for in `p2_pred` and found absent. There it
was reasoned about before the substitution and the compiler confirmed it; here
it was not, and the gate charged four streams for it.

---

## 6. The frequency record, in the two coders that had nine names for it

`FreqRec` — `w[0..6]`, `b14`, `b15` in sixteen bytes — was recovered in round
seven from the encoder's counter grid. Both pixel coders still reached the same
bytes by hand: `__decode_pixel` as `uint64_t *` under five names, `__code_pixel`
as `uint8_t *` under three and `uint16_t *` under a fourth. Every offset either
of them touches is 0, 2, 4, 6, 8, 10, 12, 14 or 15, which is what says all nine
are the record. 59 reaches became fields, and `n15_39` -- a `uint16_t *` alias
used only as `n15_39[k]` -- folded into the cursor it aliased and went.

`__code_pixel`'s frame slot could not follow them, and why not is worth
keeping. `CodePixelFrame` is a union whose `sym[32]` view is handed to
`pixel_context` whole, so `sym4` cannot become a pointer and cannot be split
in two — and it holds a loop count in one branch of an `if` and a record
address in the other. What makes the second branch safe to rewrite is that the
address is also in `n15_36`, assigned as the first statement of that branch and
never reassigned, so the seventeen later reloads of the slot are the same value
by construction. That is a fact about the block, not a dataflow inference.

**The retype is the dangerous half of this**, because `p + 15` is fifteen bytes
before it and fifteen records after, and nothing in the build says so. The tool
refuses to write the file while a subscript, a cast or plain arithmetic on a
retyped cursor is left. It stopped three; a fourth shape, `*(uint8_t *)(p + 15)`,
got past the first version of that check *and* past the compiler, and was
caught by reading the result. It is in the check now.

---

## 7. What is left

* **26 raw offsets**, from 37. What survives is a name plus a genuinely
  computed offset. Four of them are the same statement in four coder bodies —
  `*(uint16_t *)(cur - 8) = *(uint16_t *)(other + 6)`, a four-word guard row
  copied backwards — and naming it waits on knowing what the sixteen bytes in
  front of a row are for.
* **112 `goto`s over 79 labels**, unchanged, and `degoto.py --why` still
  accounts for all 60 rejected labels: 22 shared tails, 11 jumping into a
  block, 10 whose `goto` is not the whole of an `if`, 8 backward, 5 jumping
  out, 4 whose skipped region something else enters. Every one needs a flag or
  a copy of the body.
* **78 `fNN` members and 559 `vNN` locals.** The members are concentrated now:
  the three largest are `f940072` (42 sites), `f6059432` (39) and `f278704`
  (37). The first is the p2 counter table §4 just sized, so what is left there
  is not where the index lands but what the four counters in a record *are* —
  and that is the same question as the 18-byte `P2Ctx` record, not a separate
  one.
* **1442 conversion warnings** — 822 `-Wsign-conversion`, 517 `-Wconversion`,
  99 `-Wsign-compare`, 4 `-Wuseless-cast`, 5 `-Wint-to-pointer-cast` and one
  `-Wmain`. §5 is the reason to keep holding it there, and this round is the
  first where lowering it was mostly a by-product of typing things correctly
  rather than an aim.
* **What the shared record means**, unchanged from round seven §8. The bucket
  table's last record and the frequency table's first are the same sixteen
  bytes; §4 now says what the *index* into such a table is made of, which is
  the nearest thing to progress on it.

---

## 8. Tools this round added

| tool | what it does |
| --- | --- |
| `triage.sh` | four images against their reference streams, for bisecting |

One, and it is not a refactoring tool. Round seven's tools all still report
zero; what this round needed was not another pattern but a faster way to ask
"did that break it", because §5's failure took a bisection over six changes and
the gate is minutes per attempt.

It removes its output file first, and that line is the tool. `bmf c` **appends**
a member to an archive that already exists, so a second run against a stale file
produces a two-member stream and reports a difference that is entirely the
script's own doing. That cost one wrong bisection here: a known-good tree
reported a changed stream, which for several minutes looked like evidence about
the tree.
