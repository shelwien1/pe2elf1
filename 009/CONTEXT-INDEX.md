# Review: `CtxIdx`, and the contexts it has not reached

`ctxidx.inc` gives a context index three accessors — `bit<Pos>`, `bits<Pos,W>`
and `raw` — so that a packed model context can be read as the fields it is
made of instead of as a chain of multiplies. Eleven sites use it: ten in
`model.inc` and `nb_slot` in `alt_p2_context.inc`. Eighty-six masked terms
across nine lines of `alt_p2_context.inc` do not.

This is a review of both halves: whether the eleven are spelled as well as they
could be, and what converting the rest would actually cost. It ends with one
finding that is not about spelling at all.

Everything below was measured against the tree at the commit that adds this
file. Line numbers are `alt_p2_context.inc` unless another file is named.

---

## 1. The finding: `nb_id` is four entries short

Not a style point, so it goes first.

`AltP2Block::nb_id` is declared `int16_t nb_id[1916]`, and the index built for
it is

```c
nb_slot = CtxIdx{}.bits<4, 2>(gB).bits<2, 2>(gC).bits<0, 2>(gD)
                  .raw(64*gA).raw(320*band);
```

`gB`, `gC` and `gD` are each 0..3, `gA` is 0..4, and `band` is 0..5. The
largest index the expression can produce is

```
63 + 64*4 + 320*5  =  63 + 256 + 1600  =  1919
```

which is four past the end. Instrumenting the index and running the corpus:

```
noise24  t24  t32  t8g  t8p  x_ep  20000171A     max nb_slot 1919
altp1  med32                                     max nb_slot 1918
rle8                                             max nb_slot 1823
xform1  xform2                                   max nb_slot 1854
DLRAW  f05_200  rle4  t1  x_ai  x_ci             (never reached)
```

**Eight of the seventeen corpus images index past the end today** — six reach
1919 and two reach 1918, and 1916 onwards is already past it. So
`nb_id[1916..1919]` is read and written on half the corpus, every run.

It does no damage, and the reason is worth stating exactly. The declaration
after it is `uint8_t _pad2[8]`, and four `int16_t` is eight bytes: the overrun
lands in the padding and stops there, precisely. That is not luck — it is what
the recovered layout is telling us. **6 bands × 320 = 1920**, and
`1920 * 2 = 3840 = 1916 * 2 + 8`. The array is 1920 entries; the struct
recovery split the last four off as padding because nothing it could see
indexed them.

So the repair is one declaration, it is size-neutral, and every offset in the
struct stays where it is:

```c
  int16_t nb_id[1920];        // 6 bands * 320
- uint8_t _pad2[8];
```

Two notes on why this survived. ASan cannot see it: the overrun is inside one
heap object, and intra-object overflow is not something ASan instruments for a
plain member array. And the gate cannot see it either — the bytes it touches
are padding, so nothing observable changes. The same shape as the colour
transform defect: a thing that is wrong, that the corpus reaches constantly,
and that no check in the tree is looking at.

**Done.** `nb_id[1920]`, `_pad2` gone, `sizeof(AltP2Block)` measured before and
after at 1064560 either way.

---

## 2. `nb_slot`, and the proposed `bits<6, 4>(gA + 5*band)`

The suggestion is to fold the two `raw` terms into one field:

```c
.raw(64*gA).raw(320*band)   ->   .bits<6, 4>(gA + 5*band)
```

The factoring is right and the width is not. `320 = 5 * 64`, so

```
64*gA + 320*band  =  64 * (gA + 5*band)
```

and `gA + 5*band` is 0..29 with `gA` 0..4 and `band` 0..5. That needs **five**
bits, not four: at `W = 4` the mask is 0..15, so the fourteen of the thirty
`(gA, band)` pairs whose sum reaches 16 are truncated and land on a wrong slot.
`bits<6, 5>` is arithmetically exact for all thirty — bits 6..10 are free,
nothing else in the word reaches them — and it would produce the same 1919
maximum as today.

Whether it is *better* is a different question, and I would say not quite, for
one reason: `gA` is a base-5 digit and `band` a base-6 digit, and `bits<6,5>`
says "a five-bit field". Those differ in what happens when a digit goes out of
range. A field masks — silently, to a wrong slot in the same neighbourhood. A
radix carries — into the next digit, which is also a wrong slot but a
detectably wrong one. The file already records a measured instance of exactly
this: writing `bits<6, 2>(gA)` was tried, and the one neighbourhood in the
corpus that reaches `gA == 4` landed in slot 0 instead of 256; three streams
moved.

What the site actually wants is an accessor that says *radix*:

```c
  // A mixed-radix digit: `v` counts in units of `Stride`, and `Radix` is how
  // many values it has.  Not a bit field -- `Stride` need not be a power of
  // two and `Radix` need not be `Stride`'s next one.
  template<uint32_t Stride, uint32_t Radix>
  constexpr CtxIdx&digit(uint32_t v) {
    return raw(Stride*v);            // and, in a checked build, assert v < Radix
  }
```

with which the site reads

```c
nb_slot = CtxIdx{}.bits<4, 2>(gB).bits<2, 2>(gC).bits<0, 2>(gD)
                  .digit<64, 5>(gA).digit<320, 6>(band);
```

Same length as today, and the same binary: built both ways, `cmp` says the two
executables are byte-identical, which is what `raw(Stride*v)` behind a
`constexpr` should give and is worth checking rather than asserting. What it
adds is the two numbers this review had to recover by reading the threshold
table — that `gA` has five values and `band` six. Note that `digit<320,6>` also
states the array bound: `320 * 6 = 1920`, which is §1.

**Recommendation:** add `digit`, convert this one site, decline `bits<6,5>`.

**Done.**

---

## 3. The other ten `CtxIdx` sites

All in `model.inc`. Four are one context each in both halves of the model —
`decode_pixel` spans 702..1330 and `code_pixel` 1331 on, so the decoder's copy
comes first: 795/1418 (`nb`), 845/1473 (`nb2`), 852/1479 (`sig1`), 864/1494
(`sig2`). The last two, `sig3` at 1503 and `amap` at 1547, are in the encoder
only.

By accessor: three are `bit<Pos>` chains alone, six are a `bit` chain with one
`raw` for a term that is not a field, and one (`sig3`) is `bits` with a `raw`.
That is what the accessor was written for and eight of the ten are fine as
they are.

Two are worth noting, and the second is a small piece of work rather than a
remark:

- **1503**, `sig3 = CtxIdx{}.bits<0,4>(frame.sym10).raw(16*id2)` — a four-bit
  field with a lazily allocated context id above it. `id2` has no static bound
  (it is handed out on first sight of a signature), so `raw` is right and
  `bits` would be a claim the code cannot make.
- **873** *(done)*, in the decoder, was that same context unconverted:
  `frame.sym1 = 16*id2+(frame.sym1&0xF);` — the same four-bit field and the
  same `16*id2`, written as a multiply and a mask, in place. The encoder's half
  said what it is and the decoder's did not, which was the one asymmetry in
  these ten. It is `CtxIdx{}.bits<0, 4>(frame.sym1).raw(16*id2)` now.

No change proposed for the other nine.

---

## 4. The eighty-six terms `CtxIdx` has not reached

Nine lines of `alt_p2_context.inc`, 408 lines long, hold every one of them:

| line | index | masked terms | bit positions | other fields |
| --- | --- | --- | --- | --- |
| 238 | `ctx0_lo` | 5 | 15..19 | `<<13`, `<<11` |
| 243 | `ctx0` | 6 | 20..25 | |
| 248 | `ctx0` | 6 | 20..25 | |
| 270 | `ctx1` | 12 | 15..25 | `<<13`, two `q` terms |
| 274 | `ctx1` | 12 | 15..25 | `<<13`, two `q` terms |
| 296 | `ctx2` | 11 | 15..25 | `<<13`, `<<11` |
| 299 | `ctx2` | 11 | 15..25 | `<<13`, `<<11` |
| 311 | `bank3` | 11 | 15..25 | `<<13`, `<<11` |
| 322 | `bank4` | 12 | 15..25 | `<<13` |

The line numbers and the last two names are as they stood when this was written.
`bank3` and `bank4` were the words *and* the bank numbers they shifted down to,
because the shift was folded into the chain; the five stages are `step_bank`
calls now and the words are `ctx3` and `ctx4`, one shift for all five.  Nothing
about the terms below changed -- this is a record of a round, not of the file.

**Eighty-three of the eighty-six** are the same shape:

```c
(a neighbour, or a difference of neighbours) & 0x2000000
```

The other three are `& 0xFFFFF800`, which is not a bit test at all — it is the
`sum_all` quantiser at the end of lines 270, 274 and 322, and the last part of
this section is about it.

**A mask by a single power of two keeps one bit of a value, and for these
values that bit is the sign.** `P2Ctx::val`, `dval` and `err` are `int16_t`, so
each promotes to `int` with bits 16..31 all copies of its sign; a sum of four
of them cannot exceed `4 * 32767 < 2^17`, so bits 17 and up are still sign
copies. Masking bit 25 of such a sum is therefore exactly "is this difference
negative", shifted to bit 25.

By operand, the eighty-three are: **8** a plain member access, **11** a bare
local, and **64** an expression. The eleven bare locals are not the easy case
they look like — every one of them (`d_run0`, `d_up`, `d_up5`, `dtop2`,
`dvsum2`, `lap`) is itself a difference against a `run`, so they belong with
class 3 below and not with class 1.

Which makes each line a bit-packed context word, and `CtxIdx` the thing that
says so. `ctx1` at 270, converted:

```c
ctx1 = CtxIdx{}
    .bit<25>(cx0[-1].val+ra0->val-ra0[-1].val-run0 < 0)
    .bit<24>(ra2->dval+ra1->dval-2*ra0->dval < 0)
    .bit<23>(dv_now+cx0[-3].dval-run0-(cx1[2].dval-cx1[5].dval) < 0)
    .bit<22>(dv_now+cx0[-5].dval-run0 < 0)
    .bit<21>(dv_now+cx1->dval+rb1[2].dval-rb2[2].dval-run0 < 0)
    .bit<20>(dv_now+cx0[-1].dval+rb0->dval-rb0[-1].dval-run0 < 0)
    .bit<19>(cx2[-1].dval+dv_now+ra0->dval-ra2[-1].dval-run0 < 0)
    .bit<18>(dv_now+cx1[2].dval+ra0->dval-ra1[2].dval-run0 < 0)
    .bit<17>(d_up5 < 0).bit<16>(d_up < 0).bit<15>(d_run0 < 0)
    .bits<13, 2>((run0>2256)+(run0>1056)+(run0>144))
    .bits<11, 2>((sum_all>55)+(sum_all>10)+(sum_all>24));
```

Fourteen lines instead of one, and each line is one question about the
neighbourhood. The last two rows are the interesting ones, because they are
already in the file twice, in two different spellings:

```c
// line 270, and 274:
(((uint32_t)(55-sum_all)>>20)&0xFFFFF800) + q10 + q24
// lines 296, 299, 238 -- the same three-threshold count at bit 11:
(((uint32_t)(33-sum_all)>>31)+((uint32_t)(12-sum_all)>>31)+((uint32_t)(4-sum_all)>>31))<<11
```

`(uint32_t)(K-x)>>31` is `x > K`. `((uint32_t)(K-x)>>20)&0xFFFFF800` is the
same bit already shifted to position 11 — the compiler's two ways of putting a
comparison where it belongs, and `q10` and `q24` are two more of them added in.
So both spellings are `((x>a)+(x>b)+(x>c))<<11`, and one of them can be read.

## 5. What has to be true before converting, and how to know it

`x & (1<<k)` is `(x < 0) << k` only while `|x| < 2^k`. That is a claim per
term, not per file, and it splits into three classes:

1. **One `int16_t` member, masked at bit 16 or above** — 7 of the 83, and one
   more at bit 15 behind a `(uint16_t)` cast, which is the same argument one
   bit down. Provable from the promotion alone: bits 16..31 are the sign, and
   bit 15 of the 16-bit value is. No range argument.
2. **A sum of members, masked at bit 17 or above** — provable from the widths
   and the number of terms while there are at most four of them:
   `4 * 32767 = 131068 < 2^17`. Coefficients count — `2*x + y + z` is four
   terms — and so does the bit: at bit 16 a four-term *positive* sum can set
   the bit without being negative, so bit 16 needs class 3's argument even for
   a sum of members.
3. **Anything carrying a `run`, and every mask at bit 15 or 16** — the largest
   class. `run0 = pred0 + run_s`, `run1 = pred1 + run0`, `run2 = pred2 + run1`,
   and `p2_pred` returns a rounded right-shift of a weight, so the bound is
   pixel-scale (`val` is `16 * sample`, ≤ 4080) but it is an argument about the
   model rather than about a type. All eleven bare locals are here, as are the
   seven masks at bit 15 and the seven at bit 16.

   Two of those are provable anyway and show what the class needs: line 322's
   `((uint16_t)run3-(uint16_t)cx0[-4].dval-(uint16_t)dv_now4)&0x8000` casts to
   sixteen bits first, so bit 15 *is* that difference's sign whatever the
   magnitudes are; line 296's `(208-rb0->val)&0x8000` is not — with `val`'s
   type alone the result reaches 32976, and only the model's `val = 16*sample`
   keeps it under 32768.

Class 3 is the one that needs measuring rather than reasoning: instrument each
`run` and each masked expression, and record the maximum magnitude over the
corpus and over crops that reach the other coding paths. And the measurement is *evidence*, not proof — the corpus
did not reach colour transform 1 or 2 for eleven rounds either. Where a bound
cannot be argued from types, the honest conversion is to keep the mask and say
why in a comment, not to write `< 0` and hope.

## 6. Cost, and what I would do

The nine lines are the densest code in the tree and they are read by the
encoder and the decoder in lockstep; a single flipped bit position moves every
stream that reaches the deep-plane model, which the gate would catch, and a
single *wrong-but-equal-on-the-corpus* bound would not.

In order:

1. **Fix `nb_id[1920]`** (§1). One declaration, size-neutral, and it removes a
   real out-of-bounds access from eight of seventeen images.
2. **Add `digit<Stride,Radix>` and convert `nb_slot`** (§2). One site, no
   arithmetic change, and it writes down the two radices.
3. **Convert the `<<13` and `<<11` rows** — the threshold counts. Seven of the
   nine lines have them; 243 and 248 have neither. They are already fields, so
   `bits<13,2>` and `bits<11,2>` say what they are, and converting them also
   collapses the two spellings of the `sum_all` count into one. No range
   argument needed: a sum of three comparisons is 0..3.
4. **Convert the class-1 and class-2 masks** (§5), one line at a time, gating
   each — the sign tests at bit 17 and above, which is most of the eighty-three.
5. **Leave class 3 masked** until each `run` bound has been measured, and write
   the bound in the comment when it is.

What I would not do: convert the `raw` terms in `model.inc`, or replace the
`raw`s in `nb_slot` with `bits`. Both would be claims the code does not make.

## 6.1 What the six items came to

All six are done, and two of them did not come out the way this section
predicted.

**Item 4 was wrong about its own size.** "Most of the eighty-three" convert:
the rule, written out as `tools/ctxidx.py`, converts **18**. Sixty-five stay
masked, because every term in six of the nine lines carries a `run` and the
masks reach down to bit 15. Three lines pay and are converted — `ctx0_lo` (4 of
5 terms) and both spellings of `ctx0` (6 of 6 each) — and the other six are
left alone, because eleven `.raw((expr)&0x…)` around one `.bit<>` is the line
it started as with scaffolding on it. The tool refuses those rather than
leaving that judgement to whoever runs it.

The rule also got sharper while being written. "Bit 17 and above" is one bound
for every weight; the true one is per weight, since a sum of `w` members is
under `w * 2^15`: one member is a sign from bit 15, two from bit 16, three or
four from bit 17. That is what the tool applies, and it is what makes
`cx0[-1].err & 0x10000` convert.

**Item 5 found four masks that are not sign tests.** Instrumenting every
declined term over the corpus and taking the largest `|expr|`:

```
ctx1  bit 15  d_run0                          36157   past the bit
bank3 bit 15  run2-cx0[-3].val                35550   past the bit
bank3 bit 24  … - ((uint32_t)(…)>>1) …          unsigned throughout
bank4 bit 15  (uint16_t)run3 - …              69572   past the bit as an int
```

Twelve more come within a factor of four of their bit. So "leave class 3
masked" was not caution: converting any of those four would have changed what
the program computes, and the corpus would have caught it only because the
corpus happens to reach them. The measurement is in `alt_p2_context.inc` above
the six lines.

**Item 3 came out sideways.** The `<<13` rows are already fields and only one
line's could become `bits<13,2>` — the line that was converted. What the other
six had was the *bit-11* row written four different ways: inline
`(uint32_t)(K-x)>>31`, the same shifted early as `>>20 & 0xFFFFF800`, and
through `q10`, `q24`, `q39` and `q9`, which were saved into `q10a`/`q10b` and
restored around other work. All four are now
`((sum_all>a)+(sum_all>b)+(sum_all>c))<<11`, and the six locals are gone.

That one hid something. `sum_all = -sum_all` sat between the `q` locals'
assignment and their use, so `q39` and `q10` carried the *positive* sum past a
line that negated it — which is what the saving was for. Written out, the six
thresholds cancel with the negation, and it is gone too: `bank3` and `bank4`
now compare the same `sum_all` everything else does. Getting this wrong is what
five moved streams looked like, twice, before it was right.

## 7. A note on `field<Pos, W>`

`ctxidx.inc` had a fourth accessor, `field<Pos,W>`, which masked a value *in
place* rather than shifting it — exactly the shape of the eighty-three sign
tests above. It was deleted in the round that made `deadcheck.py` read the whole
translation unit, because nothing used it, and that was right: it was dead.

It is worth recording that the conversion in §4 would not bring it back either.
`field<25,1>(x)` and `x & 0x2000000` are the same statement — "keep bit 25 of
this" — and neither says what the bit *is*. `bit<25>(x < 0)` is the one that
does, and it is the whole point of doing the work.
