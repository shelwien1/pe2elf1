# Round 10 — the residue that has a shape

Nine rounds have taken this file from a Hex-Rays dump to something with types,
records, methods and a gate. What is left is not evenly distributed: it is four
or five populations, each with a reason it survived, and each with a different
kind of answer. This plan names them, counts them, and says how each one is
checked.

Everything below is a measurement taken against `subs1.hpp` at `964d86e`. The
commands are given so the numbers can be re-taken rather than believed — which
is the standing lesson of `REFACTORING9.md` §30–§45, and which this survey
promptly confirmed twice: **two of the tools that report zero report zero
because of their own spelling, not because there is nothing left.**

---

## 0. How this survey was taken

```
python3 tools/shape.py                      # the standing census
grep -oE '\b(LOBYTE|HIBYTE|BYTE[123]|LOWORD|HIWORD|WORD1)\(' subs1.hpp | wc -l
grep -oE '\b_this\b' subs1.hpp | wc -l
grep -oE '\*\([A-Za-z_0-9]+ \*+\)&' subs1.hpp | wc -l   # 303; 301 outside comments
grep -oE '\bblk\s*\+\s*[0-9]{2,}' subs1.hpp | wc -l
python3 tools/methodise.py subs1.hpp        # says 0
python3 tools/unloword.py subs1.hpp         # says 0
python3 tools/liftframe.py subs1.hpp        # says 0, with reasons
```

`grep -o | wc -l` and not `grep -c`, which is what the first draft of this
document used and which counts **lines that match, not matches**. Three of its
figures were wrong that way, all of them low: `_this` at 151 when it is 172,
the packed stores at 272 when they are 301, and `read_bmp`'s header references
at 44 when they are 49. A line with two `_this` on it is not one `_this`, and
this file has plenty. Anything below quoted as a count of *sites* is a
`grep -o`; anything quoted as a count of *lines* says so.

The last three commands are the interesting ones. Phase A is why.

---

## 1. The scoreboard

| population | today | target | phase |
| --- | --- | --- | --- |
| extractor macros in code (3 more are comment mentions) | **100** | 0 | B |
| — partial writes, `LOWORD(x) = e` / `LOBYTE(x) = e` | 85 | 0 | B |
| — of those, into a 32-bit local | 83 | 0 | B |
| — the distinct locals they write | 51 | 0 | B |
| — of those, the 4-byte ones `unloword.py` considers | 49 | 0 | B |
| `_this` occurrences | **172** | 39 after D, 27 after F | D, F |
| — in `alt_p1_alloc` (66) + `alt_p2_alloc` (65) | 131 | 0 | D |
| — outside any function body (comments, asserts) | 19 | audited | F |
| `*(T *)&…` casts in code | **301** | see below | C |
| — of which the target is a struct member | 193 | — | C |
| — same width as the member (signedness only) | 115 | 0 | C |
| — wider than the member (a real packed store) | 24 | 24, named | C |
| — a pointer store, `*(T **)&…` | 10 | 0 | C, D |
| — narrower than the member | 1 | 0 | C |
| — the classifier could not type | 43 | 0 unknown | C |
| raw `blk + N` indices | **12** | 0 | D |
| pointer casts (all kinds, `shape.py`) | **1205** | falls; no number | C, D |
| frames | **9** | 9, re-asked | E |
| `goto` / `LABEL_n:` | **49 / 33** | falls; no number | G |
| records with a positional member name | **3** | 0 | F |
| conversion-warning ratchet | 1038 | *rises* — see §2 | — |

Two rows deliberately carry no target. "Pointer casts under 1000" and "gotos
under 30" were in the first draft and both were invented — a round number that
nothing in the survey supports is a target you can hit by picking easy sites,
which is the opposite of what these phases are for. What C and D are worth is
measured by the four cast rows above them, and what G is worth is a `goto` that
became a `break`; the totals are a consequence, not a goal.

The ratchet is the one row expected to move the wrong way, and §2 says why that
is the correct outcome rather than a regression.

---

## 2. The ratchet will rise, and that is the point

`build.sh`'s `BMF_WARN` count is a ratchet: `test.sh` fails when it goes above
`warn.txt`. Phases B, C and D all **remove casts that were hiding a
conversion**, and a hidden conversion is worse than a counted one:

```c
*(int32_t *)&this->ctr_node = ctx0;      // ctr_node is uint32_t
this->ctr_node = ctx0;                   // the same store, now countable
```

The first is silent. The second earns a `-Wsign-conversion`. Nothing about the
program changed; what changed is whether the file admits what it is doing.

So Phase C raises the ceiling in `warn.txt` **deliberately, in the same commit
that earns the warnings, with the count stated**. What must not happen is the
ceiling drifting up as a side effect of an unrelated change — which is what the
ratchet exists to catch, and it still does. Every rise gets a line in this
document saying which phase bought it and for what.

---

## Phase A — the two tools whose zero is a spelling

**This goes first because it unblocks B and D**, and because a rule that cannot
fire is worse than no rule: it occupies the slot where a working one would go.

### A.1 `methodise.py` cannot see a function that returns a pointer

Its candidate regex is

```python
r'\s*(.+?)\s+%s\s*\(' % re.escape(nm)
```

— return type, **whitespace**, name. Three of the five bodies with a `_this`
parameter are declared `T *__name(`, where the character before the name is
`*` and not a space. They are silently not candidates:

```
int32_t *__alt_p1_alloc(AltP1Block *_this, …)      66 uses of _this
uint8_t *__alt_p2_alloc(AltP2Block *_this, …)      65 uses
FILE    *__bmf_close_archive(BmfArc *_this)         2 uses
```

`(.+?)[\s*]` instead of `(.+?)\s+` makes all three candidates.

The positive control needs care, because the obvious one is not available: the
tool's own docstring example, `__rescale_three_way`, was methodised rounds ago
and is `P2Freq::rescale_three_way()` today — it has no `_this` parameter left
to match, so it cannot test the regex in either direction. **That is the same
trap this phase is about**, one level up: an example that stopped being an
instance of what it illustrates.

The control that can be taken is the classification of every body, not one
body's: run `candidates()` over the file before and after, and require the set
to grow by exactly the three named above and change in no other way. The two
correct declines — `update_binary_pair` (a `uint16_t *`) and `alt_p2_filter`
(a `float (*)[4]`) — are the negative half of that, and they are declined for
a reason the regex change does not touch, so they must still be declined
afterwards.

**Taken, against a scratch copy of the tool** (the fix itself is Phase A's work
and is not applied here):

```
before: []
after : ['__alt_p1_alloc', '__alt_p2_alloc', '__bmf_close_archive']
gained: 3    lost: 0
   __bmf_close_archive    receiver BmfArc
   __alt_p1_alloc         receiver AltP1Block
   __alt_p2_alloc         receiver AltP2Block
```

The other two `_this` bodies are correct declines and stay: `update_binary_pair`
takes a `uint16_t *` into a counter block, and `alt_p2_filter` takes a
`float (*)[4]`. Neither has a record to be a method of. What they need is
Phase F's answer — a name — not Phase D's.

### A.2 `unloword.py` disqualifies 48 of its 49 candidates on one rule

It finds 49 **locals** — the 85 partial writes in §1 land on 51 distinct names,
two of which it drops for being `int64_t` rather than 32-bit — and retypes
none. The disqualification is measured, and it is one line: *a read
this rule cannot account for*. The example is

```c
int32_t rows_left, pred, north, northwest;
pred = (uint8_t)*(p - 1);          // full write
LOBYTE(pred) = (uint8_t)north;     // partial write
if ( pred < north )                // ← this read kills the candidate
```

The rule is right to be nervous: narrowing `pred` changes what `pred < north`
compares, *if* the top half was ever nonzero. Here it never is, and the reason
is syntactic rather than a value analysis — **every full write assigns an
expression already cast to the narrow width.**

That extension is worth two rules, in this order:

1. *the full write's right-hand side carries an explicit narrowing cast*, or is
   a literal that fits. **Measured: 9 of the 49 locals**, in seven bodies
   (`decode_pixel` ×2, `code_pixel` ×2, `predict_med`, `unpredict_med`,
   `alt_init_tables`, `write_bmp`, `model_plane`);
2. *the right-hand side's own declared width is already ≤ the narrow width* —
   `st = this->step` where `step` is `uint16_t`, `w2 = p_i->height` where
   `height` is `uint16_t`, `acc = *(freq + 3)` where `freq` is `uint16_t *`.
   `unloword.py` already computes a member→width map for exactly this, in
   `members()`; it is not consulted on the read side. Spot-checking the 40
   declined right-hand sides, this is most of them, but **the number is a
   prediction and Phase B is where it gets measured** — it is not a promise.

Anything left after both rules stays declined and gets a line in the tool's
`--all` output saying which read stopped it. A rule that declines out loud is
the deliverable; a rule that declines silently is what A.1 is about.

**Check for Phase A.** A tool change with no source change must be a no-op:
`tools/sweep.sh` clean, and `tools/proven.sh` re-taken for both tools — the
point of that script being that a rule which "either always finds nothing or
never looks" is indistinguishable from a broken one until you replay it against
a revision where the work existed.

---

## Phase B — the extractor macros

100 uses in code, and they are not one thing:

| form | count | what it is | answer |
| --- | --- | --- | --- |
| `LOWORD(x) = e` | 70 | a 16-bit local held in a 32-bit register | retype the local |
| `LOBYTE(x) = e` | 15 | the same at 8 bits | retype the local |
| `LOWORD(e)` read | 7 | a mask on a value that is already narrow | delete the mask |
| `BYTE1/BYTE2/HIBYTE/HIWORD(e)` read | 8 | a byte *of* a word — a real field | name the field |

The first two are Phase A.2's output, applied. The third falls out with them.

Two of the 85 partial writes are not a narrowing at all: `LOWORD(x) = …` on a
local declared `PixRec *`. Those are a register holding an address in one
lifetime and a value in the next, and `unloword.py` already declines them by
name — its docstring records that proposing one turned a green build into an
`-fpermissive` conversion. They stay until somebody splits the lifetimes, which
is not this phase.

The fourth is different and must not be swept in with the rest: `HIBYTE(magic)`
and `BYTE2(magic)` in `expand_image` are reading the version digits out of the
four-byte member magic — `'\x81\x8a' '2' '0'`. That is a **record**, not a
register: the right answer is a two-byte magic and two version bytes, checked
against `compress_image`'s `fwrite("\x81\x8A" "20…")`, and then the arithmetic
`((BYTE2(magic) << 8) - 12288) | (HIBYTE(magic) - 48)` becomes what it actually
tests, which is `major == '2' && minor == '0'`.

**Check.** The gate. Every one of these is a width change and the fifteen
streams are the instrument that says a width change was harmless. `BMF_STRICT`
stays at 0. `BMF_WARN` may move; §2.

---

## Phase C — the casts

301 `*(T *)&…` casts in code. 193 of them have a struct member as their target
and can be classified by comparing the cast's width against the member's
declared width; the other 108 are `&array[i]` and similar, and are Phase D's
problem or nobody's.

**115 same width.** Pure signedness. `*(int32_t *)&this->ctr_node = ctx0` where
`ctr_node` is `uint32_t` is `this->ctr_node = ctx0` with a warning instead of a
cast. Thirteen of them are `int32_t` written as `int32_t` — not even a
signedness change, just noise Hex-Rays emitted. These delete.

**24 wider than the member.** These are the real thing and they must survive as
something readable:

```c
*(uint32_t *)&rec->match  = …   // match is uint8_t[6] — four of the six
*(uint64_t *)&blk->sel    = …   // sel is int32_t[2]  — both, in one store
*(uint32_t *)&p_i->_pad8  = …   // BmfImage +8..11, which is why _pad8 exists
```

MSVC merged adjacent byte or word stores into one, and the cast is Hex-Rays
saying so. The answer is not to keep the cast, and not to split it into six
stores that may not be the same instruction sequence — it is to give the run a
named union member, the way `PlaneDesc` and `BmfImage` already do, and store to
that. Each is a small reading job ending in a `static_assert`.

*(An earlier draft used `*(uint32_t *)&_this->has_ref` as the example here.
That was wrong: `has_ref` is declared `int32_t`, so the cast is the same width
and it belongs in the 115 above. The lesson is the phase's own: the width of
the member is a thing to look up, not to infer from the shape of the cast.)*

**1 narrower**: `*(uint16_t *)&img_at->stride`, the low half of a 32-bit
stride, which is Phase B's fourth row wearing different clothes — name the
field.

That one was **seven** in the first draft, because the classifier took `\*+`
and then compared `sizeof(uint8_t)` against the member, turning six
`*(uint8_t **)&…` pointer stores into "narrow" casts. They are their own row
now: **10 pointer stores**, six of them the `row0`/`row1` seeds D.1 is about
and the rest waiting on the same question — what those slots hold. A classifier
that ignores pointer depth will mis-bucket every one of them, which is the
concrete reason for the next paragraph.

**43 the classifier could not type**, because the member's declaration is in a
form its regex does not read (`P1Ctx`, `BmfImage`, `PixRec` as the member type,
and ten `double` casts whose target is not a member at all). These need reading
rather than counting, and several are likely the same packed-store story.

**The classifier should become `tools/uncastwidth.py` before any of Phase C is
done**, because everything above came out of a throwaway script with at least
one known defect, and a table in prose is a table that goes stale — which is
`REFACTORING9.md` §30 in one sentence. Its first job is to reproduce the four
numbers above; where it disagrees, it is right and this section is wrong.

**Check.** The gate for all of it. For the 24, additionally: the `static_assert`
that pins the union's layout, and `BMF_STRICT` at 0 — a packed store written
wrongly is exactly the sort of thing `-fpermissive` used to absorb.

---

## Phase D — `_this`, and the blocks it hides

### D.1 The two allocators become methods

With A.1 fixed, `__alt_p1_alloc` and `__alt_p2_alloc` are methods of
`AltP1Block` and `AltP2Block`. 131 of the file's 172 `_this` occurrences are in
those two bodies and all 131 become `this->`, which is not by itself an
improvement — it is what makes the next step possible, because a method can
drop the receiver where a free function cannot.

What is left in `alt_p2_alloc` afterwards is the actual reading job, and it is
small and specific:

```c
*(uint8_t **)&_this->row0[2 * p] = (uint8_t *)_this;   // ← this one
((float (*)[4])_this)[14][2] = 1.0f;                   // ← and this one
```

The first seeds `row0`/`row1` — declared `int32_t *` — with the block's own
address, which says those rows do not hold `int32_t` at all but a pointer-sized
sentinel the neighbourhood walk compares against. Settling that settles the
type, and the type settles **six** casts: the four in the unrolled pair loop and
the two in its tail. (A seventh `*(uint8_t **)` in the file is a comment
mentioning the shape, not an instance of it.)

The third writes 1.0f at byte 232 of the block through a cast of the whole
object, and the comment beside it already works out that byte 232 is
`14 * 16 + 2 * 4` — row 14, lane 2 of the first weight block. A member at that
offset with a `static_assert` says the same thing in a form the compiler
checks, and deletes the cast.

### D.2 The free lists stop counting in dwords

```c
void **__alt_p2_free(void **blk, int8_t do_free)
{
  free(*(blk + 69689));    // 69689 * 4 == 278756 == offsetof(AltP2Block, buf)
  free(*(blk + 69690));
  …
  free(*(blk + 69665));    // 278660 == offsetof(AltP2Block, row0)
  free(*(blk + 69666));    // 278664 == row1
```

Twelve indices across the two free functions, every one of which is a member
this file has already named. They are `AltP2Block::free()` and
`AltP1Block::free()`, freeing `buf[0..4]`, `row0` and `row1` by name — and then
the eight call sites stop casting to `void **` to make the call.

This is the item that best shows what "already recovered" is worth: nothing in
the struct changes, the offsets are already asserted, and the only reason the
function still counts in dwords is that nobody re-read it after the struct
landed. **`shape.py` should grow a row for it** — a raw index into a typed
block is a countable thing and there are twelve of them.

**Check.** The gate. `tools/proven.sh` for `methodise`, because A.1 makes a
tool fire that has never fired, and a rule's first firing is the one that has to
be replayed.

---

## Phase E — the frames, with a different question asked

`liftframe.py` reports **0 to offer, 5 tried and reverted, 4 declined**, and
those three groups are three different problems. The tool is not stuck; it is
answering "can these members become ordinary locals?" and for all nine the
answer is no. **The question is wrong**, and Phase E is about asking a better
one. Nothing here is mechanical, and nothing here should be attempted by a
rule.

### E.1 `ReadBmpFrame` is not a frame, and the struct it wants already exists

This is the cheapest item in the plan and the one that best shows what the
first nine rounds bought, because **the reading is finished and written down —
it is the code that has not caught up to its own comment.** Above the two
`fread`s in `read_bmp` there is already this:

```
//   frame +36  bmp_info_hdr[0]  biSize          checked == 40 below
//         +40  bmp_info_hdr[1]  biWidth
//         +44  bmp_height       biHeight
//         +48  bmp_planes       biPlanes        checked == 1 below
//         +50  bmp_bits         biBitCount
//         +52  bmp_compression  biCompression
//         +56  _pad2[12]        biSizeImage and the two pixels-per-metre
//         +68  bmp_clr_used     biClrUsed
//         +72  _pad3[4]         biClrImportant
```

and it ends: *"The struct that says the same thing in one piece is `BmpHeader`,
above `write_bmp`, which builds this on the way out."*

`BmpHeader` is 54 bytes, `#pragma pack`ed, with four offsets pinned by
`static_assert`, and the writer already fills it field by field. The reader
`fread`s the same bytes into a scattered stack frame and reaches them as
`__frame.bmp_info_hdr[1]`.

The two reads confirm the layout exactly, and they do it by naming the lengths
rather than by anyone's inference:

```c
fread(__frame.bmp_file_hdr, 0xEu, 1u, fp)    // 14 == BITMAPFILEHEADER
fread(__frame.bmp_info_hdr, 0x28u, 1u, fp)   // 40 == BITMAPINFOHEADER
```

14 bytes covers `bmp_file_hdr[5]` and `bmp_off_bits[4]`; 40 covers everything
from `bmp_info_hdr` through `_pad3`, padding included. Both runs are exact.

So E.1 is: `read_bmp` reads one `BmpHeader`, the same type the writer uses. The
`_pad` runs stop being padding and become `biSizeImage`, `biXPelsPerMeter`,
`biYPelsPerMeter` and `biClrImportant` — fields the reader skips, which is a
different statement from fields nobody identified. §46's header checks then read
against names instead of offsets. Forty-nine references reach the eight header
slots (`bmp_file_hdr`, `bmp_off_bits`, `bmp_info_hdr`, `bmp_height`,
`bmp_planes`, `bmp_bits`, `bmp_compression`, `bmp_clr_used`); two of them are
the `__frame.bmp_info_hdr[1]` that is `hdr.biWidth`. `_pad2` and `_pad3` have
**zero** references, which is what "fields the reader skips" means measured
rather than asserted.

The one thing to watch: the two `fread`s are inside a `||` chain whose
short-circuit order is the validation order, and `BmpHeader` is one struct where
the frame is two reads. Either keep the two reads (into `&hdr.bfType` and
`&hdr.biSize`) or prove the merged read cannot change which check fires first.
The former is smaller and is what the format does anyway.

### E.2 The four union frames are role overlays, and the roles have names

```c
union {
    uint8_t buf[32768];
    struct { uint8_t _buf_head[4096]; int32_t hist_a[1024];
             int32_t hist_b[1024];    int32_t hist_c[5120]; };
};
```

`liftframe.py` declines these correctly — every member is inside a union, so
there is nothing to lift. What is available instead is the argument the union
is standing in for: whether the two views are live at the same time. If they
are not, the overlay is MSVC reusing a buffer and the two lifetimes can be two
declarations. If they are, the overlap is load-bearing and the union should say
*which* bytes are shared and why, rather than presenting a 32 kB buffer and
three histograms as alternatives.

That argument is per-frame, wants `gdb` or a probe rather than a grep, and
`choose_plane_coding` is the one to do first — it is the body §43's stack
overflow was in, so it has been read most recently.

### E.3 The five that revert stay, with their reversion recorded

`cost_candidate`, `expand_image`, `read_bmp`, `reduce_alphabet`,
`search_filter`: lifting each one aborts a specific image with a specific
status, and `liftframe.py --retry` re-takes that table rather than trusting it.
E.1 dissolves `read_bmp`'s. The others stay until somebody has the reading that
explains *which* neighbour the body walks into — and that reading is the
deliverable, not the lift.

**Check.** The gate, and `tools/asan.sh` — a frame is what makes a deliberate
walk over neighbours legal, so a frame change is the single most likely way to
turn one into an overflow. That is written at the top of `asan.sh` and it is
why §43 exists.

---

## Phase F — the names that are still positions

Three records still carry members named for their offsets, plus one parameter:

* `PlaneDesc::w0`, `w4`, `w8`, `w12`. `w8` is already aliased as `plane_count`
  and read through that name **136** times; `w12` has a comment saying it is the
  near-lossless quantiser; `w4` is the 512 the magic check compares against;
  `w0` is a union tag whose four bytes are named inside it. Three names to
  settle, each already worked out in prose next to the field.

  While there: the comment on `plane_count` says it is "read as a scalar 149
  times" and the count is 136 today. It was true when it was written. That is
  the §30 failure in a single line, sitting in the record this phase is about,
  and fixing the number is part of the phase rather than an aside.
* `P2Count::w2` — named in a comment as the value `b0` scales, which is a name.
* `AltP2Block::f278656` — a weight block still called by its offset.
* `alt_p2_filter`'s parameter is `float (*_this)[4]` and is not a receiver.
  It is the weight block; calling it `_this` is the decompiler's guess and
  keeping it is this file agreeing with the guess.

`tools/rename.py` does the mechanics. The care needed is the one it got wrong
in §39: a comment that says "predictor-2" because it means `::plane_predictor
== 2` must not become "nrefs-2". Renaming reads comments, and comments are
prose about a program rather than instances of an identifier.

**Check.** `tools/unstale.py`, which fails on a backticked name the source no
longer has, plus the gate.

---

## Phase G — the `goto` residue

49 gotos and 33 labels, concentrated in six bodies:

| body | gotos | labels |
| --- | --- | --- |
| `read_bmp` | 10 | 2 |
| `unmodel_plane_slow` | 7 | 3 |
| `expand_image` | 6 | 5 |
| `reduce_alphabet` | 4 | 3 |
| `search_filter` | 4 | 4 |
| `compress_image` | 4 | 4 |

(The first draft gave these as 12/11/10/8/8, which was gotos and labels added
together and attributed by a line-scan that put some of them in the wrong body.
The table above counts each separately, from `structs.bodies`.)

`shape.py` splits the 49 by kind — 15 restart a loop, 32 exit N blocks, 2 go
sideways to a join, **0 jump into a block**.

That last zero is what makes the phase tractable: none of these is the
irreducible kind, so no rewrite has to invent a state variable. It does *not*
follow that all 49 are a one-line `break` — the 2 that go sideways to a join
are the shape `degoto.py` got wrong in §41 and now rejects, and they may want
restructuring rather than a keyword. 47 of the 49 are the easy kind; the plan
should not round that to 49.

This phase is last because a wrong rewrite here is the hardest to see — the
region that "closed the enclosing `while` and opened an `if`, summing to zero
braces" cost a debugging session in §41 — and because E.1 removes some of
`read_bmp`'s ten for free.

**Check.** The gate per rewrite, and `degoto.py`'s balanced-prefix guard, which
is what §41 added.

---

## What this plan does not propose

* **No new behaviour.** Round 9 ended with the bug-fixing goal met: §46 records
  seven defects repaired, §47 three, §48 five and §49 one. Everything above is
  shape. A change that moves a stream is a bug in the change.
* **No reformatting sweep.** The `;` on its own line after every `{`, the
  `if ( x )` spacing — that is Hex-Rays' and it is harmless. Changing it would
  produce the largest diff in the project's history and hide whatever else was
  in it, which is the reason `methodise.py` leaves bodies where they are.
* **No renaming for taste.** `shape.py`'s "names Hex-Rays chose and nobody
  changed" is 0, and "conventional ones kept" is 56 of 74 — `i`, `k`, `p`, `q`
  are conventional and stay.
* **No lifting a frame that reverts** without the reading that says why it
  reverted. Five of those exist and the table recording them is more valuable
  than a lift that passes the gate by luck.

---

## Order, and why

```
A  the two blocked tools          (unblocks B and D; no source change)
B  the extractor macros           (needs A.2)
C  the casts                      (needs its own tool first — see Phase C;
                                   raises the ratchet — §2)
D  _this, the allocators, freeing (needs A.1)
E  the frames                     (independent, and the phase that is reading)
F  the positional names           (independent of all of them)
G  the gotos                      (last; E.1 removes some of its work)
```

Only B, D and G are mechanical end to end — a rule proposes, the gate decides.
**C is two phases wearing one name**: 115 cast deletions that are mechanical,
and 24 packed stores that are 24 small reading jobs, and it should be split the
moment `uncastwidth.py` exists to draw the line. **E is not mechanical at all,
and pretending otherwise is how a plan produces a lift that passes the gate and
means nothing.**

F was listed as needing D in the first draft, on the grounds that D "changes
what `alt_p2_filter` is called". It does not — A.1 declines that body and D
never touches it. F depends on nothing.

If only one phase gets done, it should be A: until it is, two of this project's
rules report zero for the wrong reason, and that is the failure mode the last
three rounds were entirely about.
