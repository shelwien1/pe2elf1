# Refactoring plan: minimal syntax

The rule for this round: **say the same thing with less of the language**.
`ctx` where the code says `this->ctx`, `plane_count` where it says
`::plane_count`, and so on down. Nothing here is meant to change what the
program computes, and every phase is gated on the fifteen streams coming out
byte for byte — `./build.sh && ./test.sh`, about 26 seconds a round.

This is a spelling round, not a structure round, with one exception (Phase 3)
that is called out as such.

## What is already swept, so this plan does not re-propose it

Every counting tool in `tools/` was run against the split tree before this plan
was written, and they all report zero:

```
decast     no useless casts          untemp     0 single-use temporaries
uncast     0 useless casts removed   uncopy     0 locals that are only a copy
unrecast   0 sites                   unwrite    0 write-only locals
unlayer    0 pointer casts           unsave     0 saves across a dead region
unalias    0 frame aliases           unaliasvar 0 aliases of a single assignment
namelocal  0 locals nameable         degoto     0 candidates of 11 gotos
unjump     0 gotos into a block      untail     0 shared tails
undup      0 identical if/else arms  deadcheck  0 findings
```

And the compiler agrees where it can be asked: **`-Wuseless-cast` reports 0**
over the whole translation unit. So the 590 integer casts and 207
struct-pointer casts in the tree are not same-type casts; they all convert
something. The easy classes are gone, and what follows is what those rules do
not cover.

Two tools need repair before they can contribute: `unnamed.py` fails with
`cannot read 40ce575:003/subs1.hpp` (it resolves a path from a revision that
predates the split), and `methodise.py` reports zero because its rule requires
a parameter literally named `_this` — see Phase 3.

## The census

Measured over `bmf.cpp` and the 36 `.inc` files, 12,224 lines.

| what | sites | where it concentrates |
| --- | --- | --- |
| `this->` | 923 | `altp1.inc` 437, `model.inc` 359, `altp2.inc` 99 |
| `blk->` (a parameter, not a member) | 786 | `alt_p2_model.inc` 247, `alt_p2_context.inc` 168 |
| `::name` for a global | 181 | `plane_count` 56, `packer_free_bits` 25, `coded_buf` 23 |
| `!= 0` / `== 0` in a condition | 85 | `altp1.inc` 23, `model.inc` 22 |
| `= (x);` — parenthesised whole initialiser | 64 | spread |
| declarations of a local assigned exactly once | 165 | spread |
| `while( 1 )` with a `break` | 32 | `read_bmp.inc` 7, `model.inc` 5 |
| casts to the operand's own declared type | 16 | `alt_model_p1.inc`, `arc.inc`, `model_plane.inc` |
| shadowed names (`-Wshadow`) | 20 | `model.inc` 15 |

---

## Phase 1 — drop `this->` (923 sites)

The largest single class and the one the round is named for.

**It is not a blanket substitution.** `-Wshadow` names 18 places where a local
or a parameter has the same name as a member of the class it is in, across
thirteen names:

```
alpha_map  ctx_bucket  ctx_state  depth  depth_raw  fp  group_ctr
level_of   pred        sel0_list  sel1_list  sym_cache  sym_rev
```

86 of the 923 uses are on one of those names, and for those `this->` is
load-bearing: it is the only thing distinguishing the member from the local.
The worked example is `model.inc:185` and `model.inc:1239`, both of which read

```c
sym_cache = this->sym_cache;
```

where `sym_cache` is a member *and* a local of `code_pixel`. Drop the prefix
and it becomes `sym_cache = sym_cache;` — a self-assignment that compiles
without a warning at our warning level and quietly stops loading the member.
That is the failure this phase has to be built to refuse.

**Order of work.**

1. Rename the thirteen locals — `sym_cache` → `cache_at`, `pred` → `guess`,
   and so on — one file at a time, gating each.
2. Get `-Wshadow` to zero, and **add `-Wshadow` to `BMF_WARN=1`'s flag set** so
   it stays there. A shadow reintroduced later silently re-arms the trap.
3. Only then drop all 923 prefixes mechanically.

Doing it in the other order is the mistake: a mechanical drop first would be
86 silent changes of meaning, and the gate would catch some of them as moved
streams and none of them as the reason.

**One warning about writing the tool that does this.** The list above has to
come out of `-Wshadow`, and GCC quotes the name in that message with the
*locale's* quotation marks — `‘sym_cache’`, not `'sym_cache'`. It does so even
under `-fdiagnostics-plain-output`, which turns off colour and carets and does
not touch the quoting. A pattern written with ASCII quotes matches nothing and
reports a clean tree. This was met twice: once while measuring for this plan,
and once before that by `tools/unused.py`, whose docstring already says
"a tool that depends on a flag staying in a string is one edit from being wrong
again" and whose pattern accepts both. Copy that pattern.

**Gate.** `-Wshadow` at zero, then the streams. Worth running `tools/asan.sh`
after this one as well: renaming a local moves the stack slot it gets, and
`tools/README.md` records that these bodies read off the ends of their locals
on purpose.

## Phase 2 — drop `::` (181 sites)

`::plane_count` and its 180 friends are the decompiler's way of writing "the
global", not a disambiguation — nothing shadows any of them except two names,
and `-Wshadow` says exactly which:

- **`coded_buf`** — `__compress_image`'s third parameter is `void* coded_buf`
  and the global is `uint8_t* coded_buf`. That is why the body writes
  `::coded_buf` 23 times. Rename the parameter — it is the pass-through
  auxiliary block of §1 of `ALGORITHM.md`, so `extra_blk` — and the 23 go.
- **`rc`** — `__bmf_open_archive` declares `int32_t rc, live;`, shadowing the
  program's one range coder. It is harmless because the function does not code
  anything, and it is worth renaming anyway: `rc` means the coder in every
  other file, and a reader who sees `rc` here has to check.

After those two renames all 181 `::` prefixes are noise and come out together.

**Gate.** The streams. This one is close to risk-free — a name resolving to the
same object either way — which makes it the right phase to do first if a
warm-up is wanted.

## Phase 3 — make the deep-plane functions methods (415 of the 786 `blk->`)

**This is the one phase that changes structure**, and it should be taken or
declined deliberately rather than drifted into.

`__alt_p2_context(AltP2Block* blk, …)` and `__alt_p2_model(AltP2Block* blk, …)`
are 168 and 247 `blk->` prefixes between them. They are methods of
`AltP2Block` in everything but declaration: the first parameter is the block,
every line walks it, and no call site passes anything else.

Turned into methods, `blk->` becomes `this->` and then — by Phase 1 — nothing.
415 prefixes gone and two functions that read as what they are.

**What blocks it.** `alt_p2_context.inc` has three locals whose names are
already members: `band_hi`, `band_lo`, `nb_id`. Same hazard as Phase 1, same
fix, and it must be done first. `alt_p2_model.inc` has none — its 127 locals
are all clear of the member names.

**The tool.** `tools/methodise.py` exists for exactly this and reports zero,
because its rule looks for a parameter named `_this` and these are named `blk`.
Widening it to "the first parameter is a pointer to a struct this file
declares, and every use of it is a member access" is a small change and makes
the tool able to answer the question for the other candidates too — there are
several free functions taking `ModelBlock*`, `SymList*` and `BmfArc*` first.

**Gate.** The streams, plus `tools/x32.sh`: a method's `this` is passed
differently from an explicit first argument on some ABIs, and the two pointer
widths agreeing is the check that nothing in the call sequence mattered.

## Phase 4 — `!= 0` and `== 0` in conditions (85 sites)

`if( (Flags&0x80)!=0 )` is `if( Flags&0x80 )`. `if( (hdr_flags&4)==0 )` is
`if( !(hdr_flags&4) )`. Purely mechanical, no blockers found.

The one judgement call: leave the comparison in where the operand is a
**pointer or a float**, because `if( fp )` and `if( fp != nullptr )` read
differently to different people and the tree only has 3 of those. Bitmask tests
are the target.

## Phase 5 — declare at first use (165 sites)

165 one-line declarations name a local that is assigned exactly once, at a
statement of the form `name = …;`. Each pair collapses to one line and the
type moves next to the value that explains it.

**This partly undoes an earlier round, on purpose, and the tension should be
recorded rather than resolved silently.** `tools/compact_locals.py` merged
4,559 one-per-line declarations into comma lists, which was right when the
alternative was a wall of one-per-line declarations at the top of a 600-line
body. Declaring at first use is better still where the local has exactly one
assignment, and neither rule wants the other's cases. The order to apply them
in is: this phase first, `compact_locals.py` over whatever is left.

**What it must decline.** A declaration cannot move past a `goto` that jumps
over it — 49 `goto`s and 82 labels remain — and it cannot move into a block
narrower than its uses. The rule is "the single assignment and every use are in
the same block as the declaration", and anything else is left alone.

**Gate.** The streams, and `tools/asan.sh` again: moving a declaration changes
the order the compiler assigns stack slots in, which is the same hazard
`tools/README.md` flags for `compact_locals.py`.

## Phase 6 — the small classes (82 sites)

Three rules with nothing interesting to say about them, worth one pass and one
gate between them:

- **64** `= (x);` — a parenthesised whole initialiser, `a = (b+c);` → `a = b+c;`
- **16** casts of a variable to its own declared type — `(AltP1Block*)blk1`
  where `blk1` is `AltP1Block*`. `-Wuseless-cast` does not see these because
  the operand is an lvalue and the cast is formally a conversion;
  `tools/decast.py` does not either. Its rule wants widening to "the operand is
  a plain identifier whose declared type is the cast type".
- **2** `if( (x) )`

## Phase 7 — `while( 1 )` with a break (32 sites)

Where the break is the first or last statement, the loop is a `while (c)` or a
`do … while (c)`. Where it is in the middle, it stays — rewriting that shape
costs a duplicated condition or a flag, which is more syntax, not less.

Expect roughly half to qualify. This is the lowest-value phase in the list and
the one to drop first if the round is cut short.

---

## Declined, with the reason

Recording these matters as much as the phases: without it the next round
re-proposes them.

**`-Wold-style-cast`, 1471 sites.** Turning `(uint8_t)x` into
`static_cast<uint8_t>(x)` is 12 more characters at every one of 1471 sites.
It is the house style of a lot of C++ and it is the opposite of this round's
rule. Declined on the round's own terms.

**Dissolving the eight `__frame` structs, 972 `__frame.` prefixes.** The single
largest remaining class, and the one that must not be touched. Eight frames are
left — in `choose_plane_coding`, `cost_candidate`, `decode_symbol_list`,
`expand_image`, `model` (two), `reduce_alphabet` and `search_filter` — and they
are the survivors of a sweep that already lifted everything liftable.
`tools/defram.py --list` offers zero. `tools/README.md` records four rules
learned by breaking this exact thing, the first of which is that an address
taken of one member pins the whole frame. These bodies index off the ends of
their locals deliberately; the frame is what makes the neighbours theirs to
index.

**Renaming `__frame` to something short.** 972 sites, six characters each, and
no semantic risk at all — but `defram.py`, `reframe.py`, `unalias.py`,
`unslot.py`, `unspill.py` and `liftframe.py` all key on the name `__frame`.
The saving is cosmetic and the cost is six tools. Declined unless those tools
are being revised anyway.

**Removing the `(uint32_t)` casts in mixed-sign comparisons.**
`tools/explicitcmp.py` *added* those, deliberately, to write out the conversion
the compiler was already performing. They are minimal syntax pointing the other
way, and the one site it still reports is a real one. Leave them.

**Removing the `unread_*` parameters, 50 of them.** They are parameters the
body never reads, and the name says so. Removing them means changing 50
signatures and every call site, to delete arguments the donor pushed — which
loses the record of what the donor's calling convention was for no reduction in
what a reader has to hold. The name is already the minimal way of saying it.

---

## Order, and what each phase costs

| phase | sites | risk | gate |
| --- | --- | --- | --- |
| 2 — `::` | 181 | lowest | streams |
| 4 — `!= 0` | 85 | low | streams |
| 6 — small classes | 82 | low | streams |
| 1 — `this->` | 923 | **medium** | `-Wshadow` zero, then streams, then ASan |
| 5 — declare at first use | 165 | medium | streams, then ASan |
| 3 — methodise | 415 | **high** | streams, then `tools/x32.sh` |
| 7 — `while(1)` | ~16 | low | streams |

Phases 2, 4 and 6 first: they are nearly free and they shake out any problem
with the harness before the interesting ones. Phase 1 next, because everything
else reads better once it is done and because Phase 3 depends on its rename
step. Phase 3 last of the substantial ones, since it is the only one that a
reviewer might reasonably want to reject on grounds other than correctness.

Total: about 1,890 sites, of which 1,519 are the `this->`, `blk->` and `::`
prefixes that the round is really about.

## What "done" means

Each phase ends with `./build.sh && ./test.sh` green — the fifteen reference
streams byte for byte. The round ends with the full suite, which is what every
round in this tree has ended with:

```
./build.sh && ./test.sh                  83 checks, both widths and clang
./build.sh -DBMF_HIGH_ARENA && ./test.sh 82 of 83, every allocation above 4 GB
./tools/x32.sh                           22 of 22 across the two widths
./tools/asan.sh                          nothing in 40 runs
./tools/fuzz.sh 400                      nothing
BMF_WARN=1 ./build.sh                    and the count has not gone up
```

The warning count is the ratchet, and Phase 1 moves it: `-Wshadow` joins the
set, which means the target after Phase 1 is not "still 4" but "4, and zero
shadows".
