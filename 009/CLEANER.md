# Refactoring plan: the repeated blocks

The first minimal-syntax round took out the prefixes — `this->`, `::`, `blk->`,
923 + 181 + 562 of them — and everything else that was one token too many. That
round is done and recorded in `MINIMAL-SYNTAX.md`, and the tree is 12,136 lines
in 36 files with four warnings and no shadows.

What is left is not spelling. It is **the same block written out again**, which
is what the decompiler does with a loop it has unrolled and a helper it has
inlined. Saying the same thing with less of the language now means saying it
*once*.

Every number below was measured against the tree at the commit that adds this
file, and each phase names the measurement so it can be re-taken.

| what | sites | lines it costs now | after |
| --- | --- | --- | --- |
| the counter update, `alt_p2_model.inc` | 115 of 117 | ~345 | 115 |
| its temporaries, in one declaration | 233 of 268 | 3 lines of 2,500 chars | ~35 names |
| the bit packer, `compress_image.inc` | 6 | 69 | 6 |
| the bit unpacker, `expand_image.inc` | 6 | 61 | 6 |
| declaration walls over 100 locals | 6 bodies | 1,105 locals | — |
| lines over 200 characters | 42 | — | — |

---

## Phase 1 — `p2_update`: one counter update, 115 times

**The finding.** `alt_p2_model.inc` is 1,120 lines, and 117 of them call
`p2_bump`. **103 are exactly this**, twelve more are the same three statements
with one unrelated line scheduled between them, and two are a variant:

```c
wd0800c = d0800->weighted;
ed0800c = res_c-p2_pred(wd0800c, d0800->rate);
d0800->weighted = p2_bump(wd0800c, ed0800c, 2);
```

Read it once and it is a counter update: take the weight, take the residual
against its prediction, bump it. The only things that vary across the 115 are
the counter, the residual (`res_c`, `res_s`, `nres1`, `nres2`, `nres3`,
`nres5`, `neg_b`, `neg_c`, `neg_d`) and the shift, which is 2 or 3.

```c
static inline void p2_update(P2Count* p, int32_t res, int32_t shift) {
  const int32_t w = p->weighted;
  p->weighted = p2_bump(w, res-p2_pred(w, p->rate), shift);
}
```

and every one of the 103 becomes `p2_update(d0800, res_c, 2);`.

**What it takes with it.** The two temporaries per block are named after the
counter — `wd0800c`, `ed0800c` — and they are declared in one statement of 268
names spread over three lines of about 2,500 characters. **233 of those 268
(86%) are these temporaries.** Measured: none of them is read outside its own
three lines, so they go when the blocks do, and the declaration becomes 35
names.

That is ~230 lines and 233 declarations out of one file, and what is left says
what the code does: eleven banks, four counters each, updated with one of three
residuals.

**What it must decline.** Two of the 117 subtract into the residual instead of
into a temporary — `nres3 -= p2_pred(wm0400z, m0400->rate);` and its twin at
562 — so the residual is *modified* and read again later. The helper takes the
residual by value and would drop that write. They stay as they are, and the
difference is worth a comment at each: this is a counter update that also
consumes the residual.

The inline form is separate and also stays: about twenty sites update
`mir_top[]` as `((uint32_t)(-res_s-p2_pred(w, b)+2)>>2)+w`, the same arithmetic
with the bump folded in and a different rounding term. Not this helper.

**Order of work.** The 103 exact blocks first, in one pass, gated. Then the
twelve interleaved ones one at a time — each needs its stray statement (`res_c =
res_s;`, `go = lowbits<3;`, `neg_c = -res_c;`) moved out first, and moving a
statement across a counter update is a change the gate has to check.

**Gate.** The streams, both widths. This file is the deep-plane model: every
32-bit image in the corpus reaches it, and `x_ep`, `t32`, `med32`, `altp1`,
`xform1` and `xform2` all read it in the decoder as well.

## Phase 2 — `pack_bits` and `unpack_bits`: the header's bit stream

`compress_image.inc` writes the per-plane descriptors a few bits at a time, and
each write is this, six times over, 69 lines:

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

`expand_image.inc` has the mirror of it six times, 61 lines. Both become one
helper each and a call per site: `pack_bits(dc, 8)` and `dc = unpack_bits(8)`.

**What makes this more delicate than it looks.** The six copies are not
textually identical — some use `bits_left`, one `free_bits`, some
`packer_free_bits` directly, and the widths are 6 or 8. They are the same
function with the accumulator's spill written out, so the helper has to take
the width and nothing else, and the six call sites have to agree about which
variable holds the count. The three names are one variable; that is the first
thing to fix, and it is Phase 5 of the last round applied to a global.

**Gate.** The streams, and `tools/hdrscan.sh`: this is the code that writes and
reads the two header bytes that script enumerates completely.

## Phase 3 — the declaration walls

Six bodies declare more than a hundred locals each:

```
choose_plane_coding   222 locals in 657 lines
decode_pixel          176 in 630        code_pixel   174 in 627
search_filter         123 in 581        alt_p2_context 112 in 440
cost_candidate         88 in 201
```

Phase 1 removes 233 of them by removing what they are for. For the rest the
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

42 lines are over 200 characters and 16 are over 400. They are two kinds:

- **the declaration walls** — Phases 1 and 3;
- **the context words** — six lines of `alt_p2_context.inc`, up to 664
  characters, each a chain of eleven masked terms.

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

Three specific ones, all measured:

- **`bits_left`, `free_bits`, `packer_free_bits`** are one counter under three
  names in one function (Phase 2 depends on this);
- **`q10a`, `q10b`** were the same shape in `alt_p2_context.inc` and are gone;
  the pattern is worth a sweep of the other files for `xxxa`/`xxxb` pairs that
  are one value saved across a call;
- **`__frame.sym0` … `sym31`** in `model.inc` look like a 32-entry array
  written as 32 members, and the first draft of this plan said a third of them
  were locals in disguise. Measured, that is wrong and worth writing down: 122
  writes and 124 reads across the 32, **every one written more than once**,
  every one mentioned in both halves of the model, and a median span of 634
  lines between a member's first mention and its last. They are the model's
  state, not scheduling noise. What is left is the *name*: `sym10` and `sym31`
  say nothing, and this is one place where the decompiler's numbering survived
  a round that renamed everything else. A name each, from what the sites do
  with them, is the work — not a lift.

---

## Declined, with the measurement

**Merging the three plane bodies in `alt_model_p1.inc`.** They look like a
three-times-unrolled loop — `blk1`/`blk2`/`blk3`, `err1`/`err2`/`err3`, 43/42/41
lines each — and they are not. Renaming the suffix away and diffing gives 0.42
and 0.53 similarity, and **one line in 42 matches positionally**. The planes
genuinely differ: the first carries the colour transform, the third the
near-lossless drift check. A loop over them would be a switch inside a loop,
which is the unrolling with extra steps.

**Dropping the `__` prefix from the 100 recovered functions.** It marks what
came out of the binary as against what this project added — `bmf_new`,
`p2_pred`, `sym_in_top` — and that distinction is load-bearing in every tool
here: `deadcheck.py`, `unnamed.py` and `addrmap.py` all key on it. 100
signatures and every call site, to lose a marker three tools read. No.

**`-Wold-style-cast`, now 1,172 sites.** Declined last round at 1,471 for being
12 more characters at every site; the number went down because other work
removed casts, and the argument is unchanged.

**The 161 `*(T*)&x` puns.** Most are a member read at a narrower width —
`*(uint8_t*)&d4000[1].rate` — which is what the original did and what the
struct says the member is. `tools/uncastwidth.py` reports on exactly this and
answers zero. The ones worth taking are the handful where the pun is of a
*local* whose declared type could simply be changed, and `tools/retype_locals.py`
is the rule for that; it reports zero too.

**The eight `__frame` structs, 970 prefixes.** Declined twice with reasons, and
now declined by rule: `tools/liftframe.py` refuses a frame whose members the
body indexes across, which is what these are.

**The five encode/decode pairs that were not merged.** 3% to 20% shared lines,
measured pair by pair in `bmf.cpp`'s header. Unchanged.

---

## Order, and what each phase costs

| phase | sites | lines saved | risk | gate |
| --- | --- | --- | --- | --- |
| 1 — `p2_update` | 115 | ~230 + 233 declarations | **medium** | streams, both widths |
| 2 — `pack_bits` | 12 | ~110 | medium | streams, `hdrscan.sh` |
| 3 — declare at first use | ~200? | ~150 | low | streams, ASan |
| 4 — wrapping | 42 lines | 0 | none | streams |
| 5 — one name per thing | 3 classes | small | low | streams |

Phase 1 first: it is the largest by a factor of three, it is mechanical, and it
makes Phase 3's measurement meaningful. Phase 4 can go at any time. Phase 2
needs Phase 5's first item done inside it.

Total: about 500 lines and 240 declarations out of 12,136, and — the point —
three helpers where there are now 116 copies of them.

## What "done" means

Each phase ends with the fifteen — now seventeen — reference streams byte for
byte:

```
./build.sh && ./test.sh                  93 checks, both widths and clang
./build.sh -DBMF_HIGH_ARENA && ./test.sh 92 of 93
./tools/x32.sh                           23 of 23
./tools/asan.sh                          nothing in 44 runs over 19 images
./tools/fuzz.sh 400                      nothing
./tools/sweep.sh bmf.cpp                 every counting tool at zero
BMF_WARN=1 ./build.sh                    4, and zero shadows
```

And for Phase 1 specifically, the check that has caught two defects this
session: the crops of `20000171A.bmp` that reach colour transforms 1 and 2,
compared against a build of the first commit of the decompilation. A helper
that is subtly not the block it replaced will move a stream there before it
moves one in the corpus.
