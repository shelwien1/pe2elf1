# Refactoring plan: the repeated blocks

**Done.** All five phases are in, each gated on the seventeen reference streams
at both pointer widths. The tree is **11,787 lines**, down from 12,136, with the
same four warnings and no shadows; every counting tool in `tools/` reports zero.

The first minimal-syntax round took out the prefixes — `this->`, `::`, `blk->`,
923 + 181 + 562 of them — and everything else that was one token too many. That
round is recorded in `MINIMAL-SYNTAX.md`.

What was left is not spelling. It is **the same block written out again**, which
is what the decompiler does with a loop it has unrolled and a helper it has
inlined. Saying the same thing with less of the language means saying it *once*.

Every number below was measured, and each phase names the measurement so it can
be re-taken. The `after` column is what the tree says now.

| what | sites | it cost | after |
| --- | --- | --- | --- |
| the counter update, `alt_p2_model.inc` | 115 of 117 | ~345 lines | 115 calls |
| its temporaries, in one declaration | 230 of 268 | 3 lines of 2,443 chars | 39 names, 296 chars |
| the inline fold, same file | 21 | 21 lines | `p2_nudge` |
| the bit packer, `image_compress.inc` | 6 | 67 lines | 6 calls |
| the bit unpacker, `image_expand.inc` | 6 | 61 lines | 6 calls |
| `sym0`…`sym31`, both frames | 64 members | 74 lines of union | `sym[32]` |
| declaration walls over 100 locals | 7 bodies | 1,406 locals | 5 bodies, 745 |
| lines over 200 characters | 42 | — | 29 |

Four new tools carry the rules so the next round can re-take them rather than
re-derive them: `p2update.py`, `firstuse.py`, `abpair.py`, and `ctxidx.py
--wrap`. Each is in `sweep.sh` and each now answers zero.

---

## Phase 1 — `p2_update`: one counter update, 115 times

**Taken: 114 by `tools/p2update.py`, the 115th by hand.** 103 exact and 11
after a hoist, checked per site; the twelfth interleaved block straddles a
brace and was folded by hoisting the residual choice into `res_m0010`. 21
more sites became `p2_nudge`. `alt_p2_model.inc` is 1,120 lines -> 895, and
its declaration wall 268 names -> 39.

**The finding.** `alt_p2_model.inc` is 1,120 lines, and 117 of them call
`p2_bump`. **103 are exactly this**, twelve more are the same three statements
with something scheduled between them, and two are a variant:

```c
wd0800c = d0800->weighted;
ed0800c = res_c-p2_pred(wd0800c, d0800->rate);
d0800->weighted = p2_bump(wd0800c, ed0800c, 2);
```

Read it once and it is a counter update: take the weight, take the residual
against its prediction, bump it. The only things that vary across the 115 are
the counter, the residual and the shift, which is 2 at 32 sites and 3 at 83.
Eleven names carry the residual — `res_c` at 82 sites, then `res_s`, `res_t`,
`nres1`…`nres5`, `neg_b`, `neg_c`, `neg_d` — and six more sites negate one in
place (`-res_c`, `-res_s`, `-res_t`) instead of using the `neg_*` local that
holds the same value.

```c
static inline void p2_update(P2Count* p, int32_t res, int32_t shift) {
  const int32_t w = p->weighted;
  p->weighted = p2_bump(w, res-p2_pred(w, p->rate), shift);
}
```

and every one of the 103 becomes `p2_update(d0800, res_c, 2);`.

**Why the helper may read `p->rate` at all.** Eight of the 115 do not read
`p->rate`; they read `*(uint8_t*)&d4000[-1].rate`, and `rate` is `int8_t`, so
those eight take the byte *unsigned* where the helper would take it signed. That
is the one place a mechanical fold could change what the program computes, and
it does not, because `p2_pred` uses the rate only as two shift counts and masks
both: `(weighted+(1<<((rate+31)&31)))>>(rate&31)`. A negative `int8_t` read
unsigned differs by 256, and 256 is a multiple of 32, so both masks are
unchanged. Without that, Phase 1 would have to keep the pun at eight sites.

**What it takes with it.** The two temporaries per block are named after the
counter — `wd0800c`, `ed0800c` — and they are declared in one statement of 268
names spread over three lines of 2,443 characters. **230 of those 268 (86%) are
these temporaries**, and the declaration goes to 39 names -- 38, plus the one
the braced fold below needs. Measured: no member of the 230 is read outside the
block that owns it.

One of the 230 is not named after its counter: one error temporary is called
`nb_slot`, which is also a local of `alt_p2_context` meaning a slot
index into `nb_id[]`. Two unrelated things under one name, in two files — a
Phase 5 item that Phase 1 happens to delete.

That is ~230 lines and 230 declarations out of one file, and what is left says
what the code does: **44 counters — eleven banks of four** — each updated from
one of eleven residuals at a shift of 2 or 3.

**What it must decline.** Two of the 117 subtract into the residual instead of
into a temporary — `nres3 -= p2_pred(wm0400z, m0400->rate);` and its twin on
`nres1` — so the residual is *modified* and read again later. The helper takes the
residual by value and would drop that write. They stay as they are, and the
difference is worth a comment at each: this is a counter update that also
consumes the residual.

Two inline forms are separate and also stay. **21 sites** fold the whole update
into one `+=` on the *next* counter in the bank:

```c
*(uint16_t*)&d0800[1].weighted += (uint32_t)(res_c-p2_pred(d0800[1].weighted, d0800[1].rate)+2)>>2;
```

— all 21 with the same `+2)>>2`, which is `p2_bump`'s rounding without its
dead-zone kick, so they are a *different* update and not `p2_update` written
compactly. A second helper could take them, and it should be a second helper.
**Two more** sites do the same thing to `mir_top[]`, at `+2)>>2` and `+4)>>3`.
The other 35 `mir_top[]` writes in the file are `+=` of a frequency step and
have nothing to do with any of this.

**Order of work.** The 103 exact blocks first, in one pass, gated. Then eleven
of the twelve interleaved ones one at a time — each has one stray statement
scheduled into the middle (`res_c = res_s;`, `go = lowbits<3;`, `neg_c =
-res_c;` and eight more of that shape, all of them setting up a *later* block's
residual), and moving a statement across a counter update is a change the gate
has to check.

The twelfth is not that, and it is worth doing last: at the `m0010` counter the
two temporaries are assigned in **both arms of an `if`/`else`** — `em0010b =
neg_d-…` in one, `em0010b = nres4-…` in the other — and the bump sits after the
join. Nothing can be moved out; the fold has to hoist the residual choice into
a variable and call the helper past the brace. That is a control-flow edit, not
a re-ordering, and it is the one site in the file where a mechanical pass
should stop and hand over.

**Gate.** The streams, both widths. Probed with a one-shot `fprintf` on entry
and run over the corpus, **13 of the 19 images enter this body while
compressing** — all six 32-bit ones, both 24-bit ones, and four of the six
8-bit ones — **and 7 enter it while expanding**: `out_rle8`, `rle8`, `t8g`, `t8p`,
`x_ep`, `xform1`, `xform2`.

The gap between 13 and 7 is the thing to understand before trusting the gate.
`t32`, `med32`, `altp1`, `t24` and `noise24` reach this body in the encoder and
never in the decoder, because the encoder runs it as a *candidate* and
`choose_plane_coding` then costs it out. For those five, a defect here cannot
produce a wrong decode — it changes which path wins and therefore the
compressed size, which the reference streams catch just as flatly. So the gate
covers both failure modes, but only seven images exercise the decode half; if a
change to this file passes and something still feels unproven, those seven are
the whole decoder-side evidence there is.

## Phase 2 — `pack_bits` and `unpack_bits`: the header's bit stream

**Taken: all twelve.** `image_compress.inc` 272 -> 210 lines,
`image_expand.inc` 408 -> 352. Phase 5's first item is done inside this one:
the three names for the count are one global now, and nothing has to be kept
in step. `hdrscan.sh` came back clean -- no report in 8,704 runs, both
one-byte header fields at every value over 17 streams.

`image_compress.inc` writes the per-plane descriptors a few bits at a time, and
each write is this, six times over, 67 lines:

```c
if( bits_left<8 ) {
  *(uint32_t*)packer_word = packer_acc|(2*(word_dc<<((31-bits_left)&31)));
  packer_word = (uint32_t*)out_cursor;
  out_cursor += 4;
  bits_left = packer_free_bits+24;
  packer_acc = word_dc>>(packer_free_bits&31);
} else {
  shifted = word_dc<<(-bits_left&31);
  bits_left -= 8;
  packer_acc |= shifted;
}
packer_free_bits = bits_left;
```

`image_expand.inc` has the mirror of it six times, 61 lines. Both become one
helper each and a call per site: `pack_bits(dc, 8)` and `dc = unpack_bits(8)`.

**What makes this more delicate than it looks.** The six copies are not
textually identical — four guard on `bits_left`, one on `free_bits`, one on
`packer_free_bits` directly, and the widths are 4, 6 and 8. They are the same
function with the accumulator's spill written out, so the helper has to take
the width and nothing else, and the six call sites have to agree about which
variable holds the count. The three names are one variable; that is the first
thing to fix, and it is Phase 5 of the last round applied to a global.

The 4-bit copy is the one to read first, because it does not look like the
others at all: it stores `packer_acc` with no merge term and then sets
`packer_acc = 0`. It is `pack_bits(0, 4)` — four zero bits — and every
difference from the general form is what `word == 0` collapses. If it did not
unify there would be five sites, not six.

Two things make the helper sound, and both should be checked before the first
edit rather than after. The spill branch computes the new count and the
carry-out shift from `packer_free_bits` while the guard tested `bits_left` or
`free_bits`; that is only correct because the preceding `packer_free_bits = …`
leaves the two equal at every one of the six. And the decoder's mask comes from
`frame.mask`, a frame member holding 255, used for the 8-bit reads while the
4- and 6-bit reads write `0xF` and `0x3F` inline. A helper takes `(1u<<n)-1`
and `frame.mask` goes with it — one more constant that had acquired a name.

**Gate.** The streams, and `tools/hdrscan.sh`: this is the code that writes and
reads the two header bytes that script enumerates completely.

## Phase 3 — the declaration walls

**Taken: 865, by `tools/firstuse.py`** -- 214 where the assignment is at the
declaration's own depth and 651 one block in. Bodies declaring more than a
hundred uninitialised locals: 7 -> 5, and 1,406 such locals -> 745. A second
pass finds nothing. Four guards had to be added, each for a defect the pass
produced and one of which the build accepted: see the tool.

Seven bodies declare more than a hundred locals each — counting every name in
every declaration statement of the body, comma lists included:

```
alt_p2_model          409 locals in 1118 lines
choose_plane_coding   218 in 658        decode_pixel  203 in 631
code_pixel            202 in 628        alt_p2_context 133 in 441
search_filter         130 in 582        update_model  111 in 539
```

1,406 locals, and the head of that list is the point: `alt_p2_model` declares
more than the next two together, and 230 of its 409 are Phase 1's temporaries.
Phase 1 is therefore most of this phase's work already, and it is confined to
one body — the other six are untouched by it. For the rest the
rule is the last round's Phase 5 — declare at first use — which took 102 and
declined the others because they are assigned more than once, or in a loop, or
across a `goto`. Re-running it after Phase 1 is worth it: the file it declined
them in has changed by a third.

What this phase should *not* do is split the walls into one declaration per
line. `tools/compact_locals.py` merged 4,559 of those into comma lists two
rounds ago, and undoing that wholesale trades one unreadable shape for another.
The measure is whether a name can be declared where it is first assigned, not
whether the declaration is on its own line.

## Phase 4 — the lines that do not fit on a screen

**Taken: the six context words, by `ctxidx.py --wrap`.** Lines over 200
characters: 42 -> 29, over 400: 16 -> 6. The four `tables.inc` data rows and
the 23 long expressions elsewhere were left, for the reasons below.

42 lines are over 200 characters and 16 are over 400. They are four kinds, and
only two of them are work:

- **the declaration walls**, 9 lines — Phases 1 and 3 take these, including the
  two longest lines in the tree at 996 and 988 characters;
- **the context words**, 6 lines of `alt_p2_context.inc`, 468 to 664
  characters, each a chain of exactly eleven masked terms;
- **four rows of `tables.inc`**, 213 to 427 characters, which are initialiser
  data — a coefficient row is one line because it is one row, and breaking it
  up to fit a column limit makes the table harder to read, not easier. Not
  work;
- **23 long expressions** elsewhere — `plane_choose.inc:107` and 224–554,
  `alt_p1_code.inc`'s six, `model.inc:1525`, `alt_p1_block.inc:244` and `:282`. Each
  is one statement that is genuinely that long, and each would need its own
  judgement about where to break. Not a pass; not this plan.

The second kind was reviewed in `CONTEXT-INDEX.md` and mostly declined: the
terms carry a `run`, so the masks are not provably sign tests, and four of them
provably are not. What is left to do there is **wrapping**, not rewriting. One
term per line, with the bit position leading, turns a 664-character line into
fourteen readable ones without touching what it computes:

```c
ctx1 = (cx0[-1].val+ra0->val-ra0[-1].val-run0)&0x2000000   // bit 25
     | (ra2->dval+ra1->dval-2*ra0->dval)&0x1000000          // bit 24
     | ...
```

This is formatting, so it is the cheapest phase in the plan and the one to do
when the gate is being run for something else anyway.

## Phase 5 — one name per thing

**Taken: two of the three.** The count's three names went with Phase 2. The
`sym` lift is below and is done: 246 named reads became `sym[N]` and both
unions collapsed, `model.inc` 1,961 -> 1,895 lines (and to 1,882 after Phase 3). The `xxxa`/`xxxb` sweep
is `tools/abpair.py` and **found nothing to do** -- 54 `a`/`b` families are
left and every one is two genuinely different things. Two are assigned the
same expression and are still two things: `search_filter` measures
`8*(out_cursor-coded_buf)` into `bits_a` at one point and `bits_b` at
another, and runs one plane loop with `pl_a` and a second with `pl_b`. The
tool names both so the judgement is re-takeable instead of re-derived.

Three specific ones, all measured:

- **`bits_left`, `free_bits`, `packer_free_bits`** are one counter under three
  names in one function (Phase 2 depends on this);
- **`q10a`, `q10b`** were the same shape in `alt_p2_context.inc` and are gone;
  the pattern is worth a sweep of the other files for `xxxa`/`xxxb` pairs that
  are one value saved across a call;
- **`frame.sym0` … `sym31`** in `model.inc` do not merely *look* like a
  32-entry array. They are one, and the source already says so:

  ```c
  union {
    uint32_t sym[32];
    struct { int32_t sym0; /* … */ int32_t sym31; };
  };
  ```

  Both bodies fill the members one at a time and then hand the array off whole
  — `psym = pixel_context((uint32_t*)frame.sym);` at `model.inc:1303`, and
  the same call at `:1936`. `pixel_context` reads `nb[sym_pos]`, a run-time
  index. So the 32 writes are an unrolled fill of a neighbour array, and the
  array half of the union is already declared and already used.

  Two earlier drafts of this entry got it wrong in opposite directions, and the
  reason both times was the measurement. Pooling the two frames says "every one
  written more than once, median span 634 lines" — but `decode_pixel` and
  `code_pixel` declare *separate* structs that happen to spell their members
  the same way, so pooling counts `sym5` in one function and `sym5` in the
  other as one name. Per function: **51 of the 64 members are written exactly
  once and never read through their own name** — 28 of 32 in `decode_pixel`, 23
  of 32 in `code_pixel` — and the median span between a member's first and last
  mention is a single line.

  The thirteen members that *are* read by name are all at the low end —
  `sym0`…`sym3` in `decode_pixel`, `sym0`…`sym6` plus `sym8` and `sym10` in
  `code_pixel` — which is what a neighbour array looks like: the nearest few
  get used directly, the rest only through the index. They read as `sym[0]`,
  `sym[3]` and so on with nothing lost.

  So the work here is the lift, not a naming pass: `frame.sym[N] = x` at the
  fill sites, then delete the named half of the union. Naming the 32
  individually — which is what this entry said to do — would have been work
  spent making a fill loop harder to see.

---

## Declined, with the measurement

**Merging the three plane bodies in `alt_p1_code.inc`.** They look like a
three-times-unrolled loop — `blk1`/`blk2`/`blk3`, `err1`/`err2`/`err3`, spans of
40/34/37 lines — and the reason not to merge them is that the planes genuinely
differ: the first carries the colour transform, the third the near-lossless
drift check. A loop over them would be a switch inside a loop, which is the
unrolling with extra steps.

**The similarity figure is a caveat on this decline, not support for it.**
Renaming the suffix away and diffing gives 0.51 for planes 1 against 2 and
**0.70 for 1 against 3**, with 9 of 37 lines identical in position. That is
higher than the pairs merged in the last round were, and an earlier draft of
this file quoted 0.42/0.53 and "one line in 42", which does not reproduce. So
this decline rests on reading the three bodies, not on a distance: if the
merge is ever attempted, 1-against-3 is the pair to try, and the switch it
needs is the thing to look at before writing any of it.

**Dropping the `__` prefix.** 57 bodies carry it, out of 60 `__` names in the
tree. It marks what came out of the binary as against what this project added —
`bmf_new`, `p2_pred`, `sym_in_top` — and that distinction is load-bearing in
every tool here: `deadcheck.py`, `unnamed.py` and `addrmap.py` all key on it.
57 signatures and every call site, to lose a marker three tools read. No.

**`-Wold-style-cast`, now 1,377 warnings over 1,365 sites** (`g++
-Wold-style-cast`, identical at both widths). Declined last round at 1,471 for
being 12 more characters at every site; the number went down because other work
removed casts, and the argument is unchanged.

**The 161 `*(T*)&x` puns.** Most are a member read at a narrower width —
`*(uint8_t*)&d4000[1].rate` — which is what the original did and what the
struct says the member is. `tools/uncastwidth.py` reports on exactly this and
answers zero. The ones worth taking are the handful where the pun is of a
*local* whose declared type could simply be changed, and `tools/retype_locals.py`
is the rule for that; it reports zero too.

**The eight `frame` structs, 970 prefixes.** Declined twice with reasons, and
now declined by measurement: `tools/liftframe.py` offers zero of them. It is
worth reading its reasons rather than summarising them, because they are not
all the same reason. It declines `choose_plane_coding` and `decode_symbol_list`
outright — "every member is inside its union", so there is nothing to lift that
is not already aliased — and it *tried* `cost_candidate`, `expand_image`,
`reduce_alphabet` and `search_filter`, reverting each when the lifted build
failed at run time on `altp1` or `DLRAW`. The remaining two are `model.inc`'s,
which the tool does not reach and which Phase 5 now says to lift into their own
array instead.

**The five encode/decode pairs that were not merged.** 3% to 20% shared lines,
measured pair by pair in `bmf.cpp`'s header. Unchanged.

---

## What each phase cost, and what it took

| phase | sites | planned | taken | gate it passed |
| --- | --- | --- | --- | --- |
| 1 — `p2_update`, `p2_nudge` | 136 | ~230 lines + 230 declarations | 225 lines, 229 declarations | streams, both widths |
| 2 — `pack_bits`, `unpack_bits` | 12 | ~116 lines | 118 lines | streams, `hdrscan.sh` 8,704 runs |
| 3 — declare at first use | 865 | ~150 lines | 0 lines, 661 locals out of five walls | streams, ASan |
| 4 — wrapping | 6 lines | 0 lines | 0 lines, 13 fewer over 200 chars | streams |
| 5 — `sym[32]`, the count, the a/b sweep | 2 of 3 | ~70 lines | 66 lines | streams |

**349 lines out of 12,136**, and — the point — three helpers where there were
127 copies of them, plus a fourth for the 21 inline folds.

Phase 3 is the one whose planned number was most wrong, and in an instructive
direction. The plan guessed "~200?" from what the previous round had declined;
the rule as written takes **865**, because the previous round ran two narrow
passes by hand and this one ran them everywhere. It saves no lines at all --
each declaration moves rather than disappears -- and it is still the largest
phase, because what it buys is that a name is introduced where it means
something. Five walls shed 661 locals between them.

The one thing that would have gone differently with hindsight: Phase 5's `sym`
item should have been read before it was planned. The plan called it a naming
problem and it was a lift, and the two-line union declaration that says so was
sitting in the file the whole time.

## What this round found and did not take

Two things turned up in `filter_search.inc` while Phase 2 was being traced
through the packer's globals. Neither is in this plan, both are measured, and
the next round should decide about them rather than rediscover them.

**The cost probe, 7 sites of about 14 lines.** `search_filter` chooses a filter
by trying each one: set `plane_desc[…].flags`, call `model_planes`, measure
`8*(out_cursor-coded_buf)`, flush and reset the packer to `coded_buf`, and keep
the flag if the cost improved. That is the same block as Phase 1's and Phase
2's -- a helper written out once per case -- and it is the largest one left.
The 13 `packer_acc = 0` / `packer_word = (uint32_t*)coded_buf` pairs in the file
are its resets.

**Five dead cost expressions, and a no-op `if`.** Each probe writes

```c
best_cost = plane_desc[0].desc_word-packer_free_bits+bits_f0+32;
deep = 0;
*(uint32_t*)packer_word = packer_acc;
if( !deep )
  best_cost = bits_f0;
```

`deep` is assigned 0 four lines above the test at all five sites, so the first
expression never survives; and one probe ends `if( best_cost==0x7FFFFFFF )
best_cost = 0x7FFFFFFF;`. `foldif.py` reports zero on both because its rule
wants a condition that is *literally* constant, not one assigned a constant
three lines up. That is the gap worth closing, more than the six lines are
worth deleting: a rule that only sees `if( 0 )` will keep saying zero here.

Whether the dead expression is genuinely dead in BMF.exe or whether `deep` was
a flag some build set is a question for whoever takes it -- the compressed
streams say the program behaves as if `deep` is 0, which is not the same as
saying the original had no other value for it.

## What "done" means

Each phase ended with the fifteen — now seventeen — reference streams byte for
byte. Run against the finished tree:

```
./build.sh && ./test.sh                  93 checks, 17 images, both widths and clang
./build.sh -DBMF_HIGH_ARENA && ./test.sh 92 of 93
./tools/x32.sh                           23 of 23, 0 pointers through a 32-bit integer
./tools/asan.sh                          nothing in 44 runs over 19 images
./tools/hdrscan.sh                       nothing in 8,704 runs
./tools/fuzz.sh 400                      400 mutants, 284 refused, 116 accepted, 0 reported
./tools/sweep.sh bmf.cpp                 every counting tool at zero
BMF_WARN=1 ./build.sh                    4, and zero shadows
```

Two of those earned their place during this round rather than before it.
`BMF_WARN=1` caught Phase 3 moving four declarations past a label — a `goto`
entering a scope past an initialiser, which g++ takes under `-fpermissive` with
a warning, so `test.sh` stayed green while four locals became uninitialised on
one path. And `hdrscan.sh` is Phase 2's gate for the reason the phase exists:
it enumerates every value of the two header bytes `pack_bits` and `unpack_bits`
write and read, which no sampling gate can claim.

For Phase 1 there was one more check available and worth naming: the crops of
`20000171A.bmp` that reach colour transforms 1 and 2, whose reference streams
came from an independent build and are in `test.sh`'s seventeen. A helper that
is subtly not the block it replaced moves a stream there before it moves one in
the rest of the corpus.
