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
subs1.hpp / bmf.cpp lines          17787 / 358
raw-offset sites                   22
  off `_this`                      1, in 1 functions
pointer casts                      2137
globals still at a 1997 address    0
frames                             17, 169180 bytes, 0 aliases
  slots carrying two names         0, 0 extra names, in 0 functions
  member runs walked as arrays     0 sites, 0 bases, 0 functions
  frames that dissolve outright    17, 0 aliases
structs                            22, 0 still ObjN
  fNN members / named ones         44 / 121
distinct vNN locals                554
goto / LABEL_n:                    112 / 79
__fwd_* shims                      0
```

against 37 raw offsets, 2541 pointer casts and 83 `fNN` members where round
seven's last commit left them. (§1 of that document quotes 44 / 2822 / 93,
from earlier in the round.) The conversion-warning ceiling went 1975 → 1455,
and §4.1 is the one place this round it moved the wrong way.

Nothing here was a sweep. Every tool in `tools/` reports zero against the file
at the end of this round — and for the first time that includes `unwrite`,
which had been declining four dead loads because its test for "has a call on
the right" counted every cast. The numbers moved because four objects were
re-read, not because anything was found by pattern.

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

## 4.1 Where the index lands

`AltP2Block` holds `uint16_t f940072[62208]`, and it was reached four ways:
`f940072[4 * k + r]`, a `uint16_t *` cursor at `cur[470036 + 4 * d + r]`, a
`uint32_t` index at `((uint32_t *)block)[2 * k + 235018 + 2 * c]`, and twice
with the byte offset written out. 470036 · 2, 235018 · 4 and 940072 are one
number — the table's own offset, folded into whatever unit the surrounding
index happened to be in. Recognising that is the whole of the change.

`P2Freq` is four counters, and three things say four rather than two or eight:
`rescale_three_way` halves `c[1 .. 3]` and adjusts `c[0]`, every flat reader
indexes by `4 * k`, and the allocator writes eight `uint16_t` a pass over
`0x1E60` = 7776 passes. That last is 15552 records, and §4 has already said
what 15552 is.

The three cursor constants are records `k - 1`, `k` and `k + 1` — the p2 model
updates three adjacent records, the way the p1 model updates three adjacent
nodes. `p2_freq_at` names that relation, and is the one place 940072 still
appears. Both symbol coders and `rescale_three_way` take a `P2Freq *` now, and
the two places that compose the index at the call site — one per direction —
read as four chosen weights plus a constant, which is `alt_p2_context`'s sum
written out.

**The first attempt divided by zero on every stream**, and the cause is worth
keeping. `&f940072[4 * a + 4 * b + 4 * c]` became `&freq[a + 4 * b + 4 * c]`:
the scanner stripped the leading factor and left the others, because it matched
`4 * ` as a prefix rather than distributing over a sum. The index was wrong by
four times everything after the first term. A bracket-aware sweep for a stray
`4 *` anywhere inside a `freq[...]` found exactly one site — which is also the
only place the encoder composes the index at the call, so the error and the
interesting statement were the same statement.

---

## 4.2 Names for things that already had meanings

Sixteen members were documented in a comment beside their offset and still
carried the offset as the name, so the comment was doing the identifier's work.
`sym_cache`, `pix_cur`, `grad`, `p2_ctr`, `sel0_list`, `sel1_list`,
`escape_list`, `sel_cur`, `sym_code` and `alpha_map` are the plain half of
that; `ctx`, `ctx_pair`, `fold`, `unfold` and `fold_hi` needed something
established first.

`ctx` is `f278704`, the number §4 composes and `freq[]` is indexed by.
`f278708` and `f278712` are `ctx` plus one of two entries of `f278944`, and
the symbol coders read them as `a2[bit]` — one array, so `ctx_pair[2]`, and
the coders take a `const uint32_t *` rather than a `uint8_t *` with
`4 * (a3 & 1)` done by hand.

`fold` and `unfold` are the pair `alt_init_tables` builds, and both blocks have
it. The second is 0, -1, 1, -2, 2, … — the zigzag that folds a signed residual
onto a non-negative symbol — and the coders use both ends: `n = fold[residual]`,
code `n`, reconstruct with `unfold[n] + predictor`.

`fold_hi` is a measurement rather than a reading, and the way it was measured
is the point. The 256 bytes between `fold` and `unfold` had no writer anywhere
in the file, so the first probe asked whether they were non-zero after
`alt_init_tables`. **That probe was worthless**: `bmf_new` is `malloc`, so a
non-zero byte there says nothing about who put it there. The second wrote a
sentinel over them immediately before the call and checked immediately after —
gone, at index 0 and at index 255, in both blocks. So the call writes 512 bytes
and not 256. The split into two members is the readers': some index `fold`,
some `fold_hi`, none crosses. What separates the two index spaces is not
established, which is why this is not `fold[512]`.

Nine table entries can be named this way at all because the round's earlier
sections gave the model enough shape to say what a table is *for*. That is the
order the work went in, and it is why §4.1 could not have come first.

---

## 4.3 Three records the comments had already described

Four `ModelBlock` tables are read only as `t[3 * k]`, and the comment beside
them has said since round five that each triple is two counts and a total.
`BitCtr` makes that the type: 16, 4096, 257 and 48 records rather than 48,
12288, 771 and 144 words, with both bit coders taking a `BitCtr *`.

It is `w[3]` and not two named counts and a total, because
`encode_context_bit` reaches `w[w[0] - 1]` — it steps by the value of the first
count. That is inside the record only while `w[0]` stays under four, which the
seeds (40, 16, 512) actively suggest it does not. A `__builtin_trap()` on
`w[0] > 3` at that site fires on none of the fifteen images, so the three words
are one addressable run and the record has to admit it.

`AltP2Block`'s `f278640` and `f278648` are `bias[4]`. Nothing reads either as
an integer — `alt_p2_filter` takes two floats out of the first and
`alt_p2_context` one out of the second — and the only writes are the four
`= 0` that clear both at the start of a plane. `uint64_t` was the width of the
clearing, not of the data.

**The strides went wrong twice here, and both times identically**: an index in
element units is a third of itself in record units. The seeding loops step
`6 * n` and had to become `2 * n`; four constant indices — `t[1]`, `t[2]`,
`t[3]` — are record 0's second and third words and record 1's first, not
records 1, 2 and 3. `triage.sh` caught both, and the second as a *stream
difference* rather than a crash, which is the one a less exact check would have
let through.

---

## 4.4 Two things that were always five

`alt_p2_model`'s outer loop runs `n5` from 0 to 4 and opens each pass with
`((uint32_t *)block)[n5 + 69669]`. 69669 · 4 is +278676, the first of six
`int32_t` scalars, so five of them are one array — `bank_ctx[5]` — and the
sixth is not part of it. They arrived as scalars because `alt_p2_context`
writes them one at a time.

What the pass then works on was

    (P2Count *)((uint8_t *)&((uint32_t *)block)[v75] + (n5 << 17)) + 71178

and every constant in it is a name: 71178 records is +284712, which is
`p2_ctr`, and `n5 << 17` is 131072 bytes, which is 32768 records. So it reads
`&p2_ctr[32768 * n5 + bank_ctx[n5]]` — five banks of 32768, which is exactly
what the struct comment had worked out from the index widths two hundred lines
above, now said by the code that uses it rather than only about it.

And one branch, in `alt_init_tables`:

    if ( (a2 + 1 <= a2 + 2 || (uint32_t)(v19 - v18) < 0xFE)
      && (v18 <= v19 || (uint32_t)(v18 - v19) < 0xFE) )

`v18` and `v19` are `a2 + 2` and `a2 + 1`, so the first half is
`a2 + 1 <= a2 + 2` and the second falls to `1 < 254`. That is MSVC's
"do these two ranges overlap" test for a vectorised loop, and it cannot be
false. **The `else` is the evidence rather than a casualty of removing it**: it
writes the same 256 values in two passes instead of one, which is what an
overlap fallback is, and what says the branch belonged to the compiler. Both
went, the one loop stayed, and `unused.py` took the two loop variables with
them.

---

## 4.5 The three records the round was for

Two of these had been "the question" in `algorithm_v2.md` since round six, and
they answer once the layout stops lying about them.

**`P2Ctx`** was `int16_t lane[9]`, and the last two bytes are not a lane. Both
pixel bodies end a record with the same block, and it is a complete
description: `lane[0]` is the pixel scaled by 16, `lane[2]` is its horizontal
gradient, `lane[3] = lane[5] = lane[6] = lane[7] = |lane[2]|` with
`lane[4] = |lane[2]| / 2`, byte 16 is the three-way sign of the gradient and
byte 17 is 2. The `|x|` arrives as `(WORD2(x) ^ x) - WORD2(x)` — the branchless
absolute value — which is what made lanes 3..7 look like five unrelated
quantities: they are one quantity, five times.

`mag` is read 32 times and `sign` exactly once, and finding that one read is
what the retype was for. It was spelled `(uint8_t)lane[8]` — one past the eight
lanes — so it compiled, and read the right two bytes, only because the
declaration claimed nine. It assigns to `ctx_w[4].sel`: the three-way sign of a
record's gradient *is* the fifth base-3 digit of the next context. **The commit
before this one said `sign` had no readers**, on a survey that could not see a
reach the declaration was hiding; shrinking the array is what turned it up.

**`P2Freq`**'s fourth counter is not a count. `alt_p2_encode_symbol` codes a
three-way alphabet out of `f[0..2]` and ends `*chosen = step + *chosen`, so
`step` is the size of an update — and the rescale *lowers* it, by half above
256 and by 32 above 32. A learning rate that decays as the record matures,
seeded at 4096 against frequencies summing to 7680.

**`BitCtr`** adapts its forgetting instead: `n[bit] += 8` always, and passing
`limit` halves the pair and raises `limit` by 64 to a ceiling of 0x4000. Its
`n[n[0] - 1] += 4` — which §4.3 could only justify by measuring that it stayed
in range — turns out to be reachable only from a half-initialised state where
`n[0]` is not a count at all but the first bit the record ever saw, plus one.

Three probes carry these: the addend differing from `step`, the halving branch
failing to lower it, and `n[0] > 2` at the half-warm site. None fires on any of
the fifteen images. The first two matter because `LOWORD(n32) = step` leaves
the high half of `n32` alone, so "the addend is `step`" is a claim about what
is in those bits and not about where they came from.

---

## 4.6 The five p2 digits

With `mag`, `sign`, `ctx_w` and the band named, `alt_p2_context`'s five
selectors read directly. Four of the five are the same test:

    (d <= band_hi) + (d < band_lo)

which is above / inside / below — a base-3 digit. `band_lo` and `band_hi` are
`f278720` and `f278724`, set once to −(16q + 7) .. 16q + 8 for
`q = plane_desc[0].w12`. Two lines earlier the same `q` sets `deadzone_hi` and
`deadzone_lo` to ±(4q + 1), the dead zone `p2_bump` uses. **One tolerance at
two scales**: the narrow one decides whether a counter update counts, the wide
one which context an error falls in — and both widen as the coding gets
lossier.

Group 4 is `v201[-1].sign`, the previous record's stored digit, taken as a
digit. Group 0 buckets `n1840` at the fixed 272 and 1840. Groups 1–3 take three
different differences against the band.

**One claim here was wrong and had to be corrected in the commit after it.** I
read `n1840` as a weighted sum of the neighbourhood's `mag`, because such a sum
ends on the line immediately before `n1840` is used. It assigns `n960_1`;
`n1840` is set forty lines earlier from a counter prediction. The four
selectors I read *off the selector statements themselves* were right; the one I
inferred from what sat above it was not, and that is the whole difference.

---

## 4.7 A stride that was out of phase, and what it was hiding

`alt_p2_context` reaches its neighbourhood through eight cursors Hex-Rays
typed `int16_t (*)[8]` — a **sixteen**-byte stride over **eighteen**-byte
records. `v293[3][6]` was three sixteens and six twos: the byte arithmetic was
right and the subscripts meant nothing.

Decoding all 72 reaches against the real record size lands every one of them on
a field boundary — 0 misaligned, across all eight cursors. A sixteen-byte
stride hitting eighteen-byte fields 72 times running is not a coincidence, so
the cursors are `P2Ctx *` and each reach says which record and which lane.

That is what settled the last two open questions, and neither needed dataflow
tracing in the end — only a type that was not lying.

**Group 3 of the p2 context** was `((uint8_t *)v204[1])[0]`. Byte 16 is `sign`:
`ctx_w[3].sel = v204->sign`. So groups 3 and 4 are both a stored sign digit,
read back off two different rows.

**Why a record holds one magnitude five times.** Each of `lane[3 .. 7]` is the
input to a *different* fixed-tap linear predictor over the same neighbourhood —
taps 21/12/16/22/20/14 for one, 17/21/15/25/9/22/19 for another,
17/15/21/18/16/22/19 for a third, each with its own `ctx_bias` accumulator, and
two plainer reductions beside them. Five copies because five filters consume
it; they diverge because `alt_p2_model` writes each filter's residual back into
its own lane.

So the five digits take `lane[0]` twice, `sign` twice, and a predicted level,
and the magnitudes reach them only through the predictions.

A ninth cursor walked the same grid in *words* — `v46`, an `int16_t *` where a
record is nine subscripts and a lane is one, so `v46[-54]` and `v46[-53]` are
the same record. Its 45 reaches decode as records −6 .. 0 at lanes 0 and 1.
With it, every cursor into the neighbourhood table steps by a record.

Pointer casts 2183 → 2141.

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

* **22 raw offsets**, from 37. What survives is a name plus a genuinely
  computed offset. Four of them are the same statement in four coder bodies —
  `*(uint16_t *)(cur - 8) = *(uint16_t *)(other + 6)`, a four-word guard row
  copied backwards — and naming it waits on knowing what the sixteen bytes in
  front of a row are for.
* **112 `goto`s over 79 labels**, unchanged, and `degoto.py --why` still
  accounts for all 60 rejected labels: 22 shared tails, 11 jumping into a
  block, 10 whose `goto` is not the whole of an `if`, 8 backward, 5 jumping
  out, 4 whose skipped region something else enters. Every one needs a flag or
  a copy of the body.
* **46 `fNN` members and 559 `vNN` locals**, against 93 and 559 at the round's
  start — 36 distinct names, and the largest is 11 sites. The locals have not
  moved at all, and will not until the model is read: 503 of the 559 are in
  `alt_p2_model` and 251 in `alt_p2_context`. What is left in the p2
  block is not where the index lands, which §4.1 answers, but what the four
  counters in a record *are*, and that is the same question as the 18-byte
  `P2Ctx` record rather than a separate one.
* **1455 conversion warnings** — 834 `-Wsign-conversion`, 513 `-Wconversion`,
  104 `-Wsign-compare`, 4 `-Wuseless-cast`, 5 `-Wint-to-pointer-cast` and one
  `-Wmain`. §5 is the reason to keep holding it there, and this round is the
  first where lowering it was mostly a by-product of typing things correctly
  rather than an aim.

  §4.1 put 28 of those back, and that is the one place the ceiling moved the
  wrong way. The rise is +26 in `alt_p2_model` and +3 in
  `alt_p2_d8_decode_body`, the two bodies it rewrote, and nowhere else — and it
  is the same count whichever way `CtxWeight::w` is signed, so it is arithmetic
  becoming visible rather than a typing choice left unmade. A ratchet that only
  ever falls stops being a measurement; what it is for is that a rise has to be
  accounted for, and this one is.
* **What the shared record means**, unchanged from round seven §8. The bucket
  table's last record and the frequency table's first are the same sixteen
  bytes; §4 now says what the *index* into such a table is made of, which is
  the nearest thing to progress on it.

---

## 8. Tools this round added

| tool | what it does |
| --- | --- |
| `triage.sh` | four images against their reference streams, for bisecting |
| `unwrite.py` | a call is an identifier followed by `(`, not any `(` |
| `unused.py` | stop the walk-back at a comment; count deletions |
| `unaliasvar.py` | a local that is one assignment of another local |

`unaliasvar.py` is the round's one new refactoring tool, and what it cost is
the useful part. The fold is safe on a narrow condition, and the condition took
three tries — each miss changed a reference stream, and `triage.sh` with a
per-body bisection found each in minutes:

* `v = ...` is not the only way to write a local. `v += 144`, `++v` and
  `LOWORD(v) = x` all do, and counting only the first calls a variable
  single-assignment while three other statements change it.
* A name assigned once in a body is not therefore a *local*.
  `plane_alt_model = v16` looks exactly like an alias; folding it hid a write
  four other functions read.
* Two aliases can name each other — `a = b` and `b = a` — which is a rotation
  carried across a loop, not a copy.

67 folded, 51 declarations dropped after them, 117 lines gone.

The other two are fixes to tools that had been reporting zero while work
remained, and both were made findable by this round rather than by the file.
`unwrite.py` declined four dead loads because its test for "has a call on the
right" counted every cast. `unused.py` missed five declarations because it
walks back to a statement's first line by looking for a `;` above, and a
comment line has none — so it climbed into the paragraph above the declaration
and the type match failed silently. Round eight put comments above enough
declarations to turn that from theoretical into five misses, and it also
printed those five as "deleted" while the file was byte-identical afterwards.

A tool reporting zero is only evidence if the zero can be wrong in a way you
would notice. Round seven's tools all still report
zero; what this round needed was not another pattern but a faster way to ask
"did that break it", because §5's failure took a bisection over six changes and
the gate is minutes per attempt.

It removes its output file first, and that line is the tool. `bmf c` **appends**
a member to an archive that already exists, so a second run against a stale file
produces a two-member stream and reports a difference that is entirely the
script's own doing. That cost one wrong bisection here: a known-good tree
reported a changed stream, which for several minutes looked like evidence about
the tree.
