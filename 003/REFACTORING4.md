# Refactoring BMF 2.01, round four

Rounds one to three are recorded in `REFACTORING.md`, `REFACTORING2.md` and
`REFACTORING3.md`. Round three finished what it set out to do: `bmf_bss` is
gone, every walked run is a declared array, no frame is pinned, and the five
recoveries of the p2 workspace and the four of the p1 one are each a single
struct. The passes it left behind — `dedup.py`, `deadcheck.py`, `degoto.py`,
`unused.py` — all report nothing.

The code is still ugly, and four things make it so:

1. **`__m128` types where there is no SIMD.** Zero intrinsics remain in the
   file. What is left is 29 by-reference parameters, 18 struct members and
   4 globals wearing a 16-byte vector type — and **the parameters are dead**.
2. **`_this + ofs` with a cast on every access**, still 1514 sites.
3. **References that should be tables and variables** — 384 of them. 336 are
   frame aliases; **20 are file-scope tables declared three times each**, which
   is the plainest case in the file and the one this document first missed; and
   20 more are views into `plane_desc` that round three left behind, three of
   them already dead.
4. **The frames themselves** — 22 structs, 169 788 bytes.

They are the same problem round three named, one layer further in: *a type that
records how a value moved rather than what it is.* `__m128` records an XMM
register; `__frame` records a stack slot; `_this + ofs` records an addressing
mode. None of the three records a fact about BMF.

The gate does not change: **byte-identical compressed streams against the
committed references, every image round-trips, 15 refused inputs, and the
out-of-memory ladder** — and, since round three lost it twice without noticing,
**`BMF_STRICT=1 ./build.sh` reporting 0** alongside it, not instead of it.

---

## 1. Where this is

`python3 tools/shape.py` prints this, so every number below can be re-derived
without trusting the document:

| | |
| --- | --- |
| `subs1.hpp` / `bmf.cpp` | 19 400 / 787 lines |
| raw-offset sites | **1514** (161 off `_this`, in 11 functions) |
| pointer casts | 5191 |
| globals at a 1997 address | 0 |
| reference declarations | **384** — 336 frame aliases, **20 file-scope tables**, 20 `plane_desc` views, 8 other |
| frames | **22**, 169 788 bytes, **336 aliases** |
| — slots carrying two names | 25 slots, 60 extra names |
| — runs walked as arrays | 0 |
| structs | 86, **79 still `ObjN`** |
| `fNN` members | **236**, against 59 named |
| distinct `vNN` locals | 560 |
| `goto` / `LABEL_n:` | 112 / 79 |
| `__fwd_*` shims | 94 |
| `__m128` occurrences | **199** on 160 lines — 0 intrinsics, 29 parameters in 15 functions (21 more on shims), 18 struct members, 4 globals, the rest locals and casts |

---

## 2. Phase A — the `__m128` types, and why they are there

### 2.1 There is no SIMD

`grep -cE '_mm_|__builtin_ia32' subs1.hpp` is **0**. Round two translated all
558 intrinsics to C. Every `__m128` in the file is Hex-Rays' record of a
16-byte register or a 16-byte-aligned access, and the code reads them one lane
at a time:

```c
  v6  = a1->f278528[13].m128_i32[0];
  v7  = (int16_t *)a1->f278528[13].m128_i32[1];
  v14 = *(int16_t *)(v289->f278528[13].m128_i32[3] + 10);
```

327 `m128_i32`, 135 `m128_f32`, 73 `m128_i16`, 27 `m128_p`. Not one of them is
a vector operation.

### 2.2 The 29 parameters are a dead thread — measured

`main` declares two of them and never initialises them:

```c
  __m128 v3, v4;
  ...
  if ( Mode == 'C' ) __bmf_compress(v3, v4, argv[2], argv[3]);
  else               __bmf_decompress(v3, v4, argv[2], argv[3]);
```

and they are passed down **fifteen functions** and **21 forwarding shims** --
`bmf_compress`, `bmf_decompress`, `compress_image`, `expand_image`,
`search_filter`, `transform_planes`, `model_planes`, `model_plane`,
`unmodel_plane`, `alt_model_p2_d8_encode`, `alt_model_p2_d8_decode`,
`alt_p2_d8_encode_body`, `alt_p2_d8_decode_body`, `alt_p2_model`,
`alt_p2_context` -- and `alt_p2_context`, at the bottom, never reads them.

**The experiment:** poison all 31 sites where an `__m128` value enters the
program -- the 29 `__m128 a = a__ref;` copies and `main`'s two uninitialised
locals -- with `__builtin_memset(&a, 0xA5, 16)`, and run the gate.

> **PASS.** Fifteen streams byte-identical, archive, malformed, out-of-memory
> ladder. Nothing in the program observes any of these 29 values.

That is round two's poisoning technique, and it settles the whole thread at
once. Of the 29:

| | |
| --- | --- |
| never read at all | **25** |
| `alt_p2_model::a2` | read 4 times — **after being overwritten** (§2.3) |
| `search_filter::a3`/`a4` | stored to memory as `m128_u64[0]` -- and `search_filter` **is** reached: a `__builtin_trap()` at its top fires on `altp1`, `med32` and `noise24`, so the poison really does land in that memory and no stream moves |

### 2.3 `alt_p2_model::a2` is a local `float`

The one parameter with lane reads is written before any of them:

```c
13390:  a2.m128_f32[0] = (float)v577;          // the sample
13408:  v19.m128_f32[0] = a2.m128_f32[0] - v18;
13428:  const float err_a = (a2.m128_f32[0] - v20) * 2.5999999f;
13467:  err = a2.m128_f32[0] - pred;
```

`a2` is a parameter being reused as a scratch register, which is what MSVC did
with XMM0 and what Hex-Rays faithfully recorded. It is one `float`, it holds
the sample the model is predicting, and it should say so.

### 2.4 The four `__xmmword_*` globals are single floats

```c
alignas(16) static uint8_t bmf_xmmword_439B50[16] = {
  0x52,0x49,0x1d,0x3b, 0x52,0x49,0x1d,0x3b, 0x52,0x49,0x1d,0x3b, 0x52,0x49,0x1d,0x3b,
};
```

`0x3b1d4952` is `0.0024f`, four times. `439B10` is `0.023f`; `439B40` is
`0.05f`. **Round two already recovered two of these** as
`bmf_p2_rate_reset = 0.0024f` and `bmf_p2_ms_rate = 0.023f` — the broadcasts
are the same constants arriving by the other road, and the only thing they are
passed to is the dead thread.

### 2.5 `Obj11::f278528[21]` names bytes the struct already names

The array is read with **constant** indices at all but five sites, and
`f278528[13].m128_i32[0]` is offset 278528 + 13×16 = **+278736** — which the
union's second layer already declares as `f278736[10]`. The same bytes have
names, twice over.

The five exceptions are in `alt_p2_filter`, which walks `f278528[j].m128_f32[k]`
with a loop variable. That is a genuine array — of `float[4]`, not of
`__m128`.

Eighteen `__m128` members in all, 47 elements: fourteen in `Obj11`, `Obj12`'s
six sub-model pointers (which round three declared from `alt_p2_filter`'s own
parameter type, so those are `__m128 *` because the *callee* says so), and
three in `Obj35`.

### 2.6 The order

1. **Delete the thread.** 29 parameters across 15 functions, and the 21
   `__fwd_*` shims that carry them. Nothing reads them; the poison run is the proof, and it is
   cheap to re-run after each function.
2. **`alt_p2_model::a2` becomes `float sample`.** Its four readers are in one
   function.
3. **`search_filter::a3`/`a4` become the constant they write.** The trap probe
   settles the one thing §7 would otherwise hedge on: the function runs on
   three of the images, so the poison genuinely reaches its two `m128_u64[0]`
   stores and nothing downstream reads them. `xorps xmm, xmm` before a 16-byte
   store is the idiom to look for, so `0` is the likely value; either write it
   or, if the destination has no reader at all, drop the store and say which.
4. **The `__xmmword_*` globals go with the thread** — they have no other
   reader.
5. **`f278528[21]` becomes `float coef[21][4]`** for `alt_p2_filter`'s sake,
   and the constant-index sites move to the names the union already gives them.
   `Obj12`'s six and `Obj35`'s three go the same way, and that is the last
   `__m128` in the file.

`bmf.cpp`'s `M128` union and the `#define __m128 M128` go with it, and so does
the `<xmmintrin.h>` include — except for the MXCSR flush-to-zero bits, which
are a real property of the original and stay.

---

## 3. Phase B — `_this + ofs`, the 1514 that are left

Round three took this from 1750 to 1514 and `_this` from 330 to 161.
`unoffset.py` has 14 sites left, every one a narrower-than-member read. The
rest are not one problem:

Partitioned by function and base, so the rows sum to 1514 and nothing is
counted twice:

| where | sites | what it is |
| --- | --- | --- |
| `alt_p1_model` | **85** | one counter table, computed indices (48 off `_this`) |
| `layout_workspace` off `a1` | 50 | the p1 allocator, still taking a `uintptr_t` |
| `alt_p2_alloc` | 31 | two counter tables |
| row cursors (`v45`, `v46`, `v12`, `v13`) | **252** | literal byte offsets off a cursor |
| the rest | **1096** | locals whose object has no struct |

### 3.1 `layout_workspace` is the one that is simply wrong

```c
int32_t __layout_workspace(uintptr_t a1, int32_t a2, int32_t i, int32_t a4, int32_t a5)
```

It takes an **integer** for a pointer and writes 50 offsets through it. This is
`alt_p2_alloc` before round three retyped it, and the same fix applies: the
caller passes a `ModelBlock *`, so the parameter should be one. §6's signedness
hazard applies — check the comparisons before assuming the width is all that
matters.

### 3.2 The two counter tables, whose extents check each other

`alt_p2_alloc` fills a table of 8-byte records at +284712 with `j < 0x14000`,
and a table of 16-byte records at +940072 with `n0x1E60 < 0x1E60`:

> **284712 + 0x14000 × 8 = 940072 exactly.** The first table ends where the
> second begins.

Two independent loop bounds and one arithmetic identity. Declaring them needs
`Obj11` rebuilt as overlapping regions, because `f284704` and `f940064` are the
`__m128` view of the same bytes — **and Phase A deletes those**, which is why
this phase comes second. Appending a union instead of rebuilding puts the
tables 940 080 bytes too far along; the gate said so within a minute.

### 3.3 `alt_p1_model`'s table wants a name, not a subscript

Its 48 sites are `*(uint16_t *)((char *)_this + 16 * v8 + 3800)`, which is
record 237 + v8, field 4 of a 16-byte-record table at +0. Writing that as
`rec[v8 + 237][4]` is not more readable than the arithmetic, and §6 is explicit
that the metric is whether a reader can tell what the object is. This one wants
the p1 model read first — `algorithm_v2.md` §9 is where that goes.

### 3.4 The row cursors

182 sites of the shape

```c
  v45 = lpAddress->f278528[13].m128_p[0];              // = f278736[0]
  *(uint32_t *)(v45 + v39 - 126) = *(uint32_t *)(v45 + v39 + 108);
```

A row cursor and a signed byte displacement — which is what a predictor's
neighbourhood *is*. These are not a defect to remove but a structure to name:
`-126`, `+108`, `-122`, `+112` are pixel positions relative to the cursor, and
naming them (`NW`, `N`, `NE`, …) would say more than any retype. This wants
`algorithm_v2.md` §9 as well, and should wait for it.

---

## 4. Phase C — the twenty tables declared three times

This is the plainest thing in the file and the plan's first draft did not see
it, because it went looking for frame aliases and stopped when it found 336.
There are **384** reference declarations. 336 are frame aliases (§5), 20 are at
file scope, 20 are views into `plane_desc`, and 8 are the two `bss_exclusion`
buffers and six `Obj0 *` bindings into a plane array.

Each of the twenty file-scope ones is a table said three ways:

```c
alignas(16) static uint8_t bmf_dword_439880[16] = {   // 0x439880
  0x04,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x0c,0x00,0x00,0x00,0x08,0x00,0x00,0x00,
};
typedef int32_t t_dword_439880[4];
static t_dword_439880& __dword_439880 = *(t_dword_439880*)bmf_dword_439880;
```

A byte image, a typedef for the real type, and a reference that reinterprets
one as the other. What it means is

```c
static int32_t __dword_439880[4] = { 4, 8, 12, 8 };
```

— one declaration instead of three, and the values legible instead of
little-endian hex. Twenty tables, 60 declarations becoming 20.

The three-way form is a leftover of round one's `collect_globals.py`, which had
to keep the byte image because the globals still lived at their 1997 addresses
and something might stride between them. Round three ended that: `bmf_bss` is
gone and no global is at an address any more. The reason for the indirection
went with it.

**What to check per table, since the byte image is what is verified today:**

* **the element type**, from the typedef — `t_dword_439880` is `int32_t[4]`,
  so the sixteen bytes are four dwords and the hex should become decimal;
* **the extent**, which is not always the buffer's. `bmf_ctx_group_flags[32]`
  holds fifteen non-zero bytes and seventeen zeros, and 32 is the distance to
  the next 1997 global, not the table's length. The zeros may be padding or
  may be entries;
* **the name.** Four of the twenty are the `__xmmword_*` broadcasts §2.4
  decodes as `0.023f`, `0.05f` and `0.0024f`, and two of those already have
  names elsewhere in the file. They go with Phase A, not here.

The gate answers each one: a table whose bytes are re-expressed wrongly moves a
stream immediately, and these are read on every image.

### 4.1 And twenty views into `plane_desc`

Round three declared the plane-descriptor table and left the twenty names it
arrived as bound to it, so that the ~221 subscript sites could move in their
own time. 178 of them did. What is left is:

| | |
| --- | --- |
| **already dead** | `__n512`, `__byte_4433AC`, `__byte_4433BD` — no readers at all; delete the declarations |
| **worth keeping** | `plane_count` (155 uses) and `__n3_0` (29) — `plane_desc[0].w8` and `plane_desc[4].src_plane` say less than the names do, so these stay as named references and the *name* is the deliverable |
| **to fold** | the other fifteen, 42 uses between them — every one an index multiplied by 16 several statements earlier, in a local with other readers |

The fold is the one thing round three named and did not finish, and the reason
is in its §0: a substitution cannot do it, because the multiply is hoisted into
a local that is read for other purposes. Each of the fifteen wants its local
split first.

---

## 5. Phase D — the 336 frame references

336 of the file's 384 reference declarations are frame aliases. 140 are the
plain `T &v = __frame.m;` form and the rest carry a cast or a subscript.

`defram.py --list --arrays` offers 249 of them across 13 frames, and
`tools/frame-sweep.sh --arrays` — which lifts one frame, runs the whole gate,
and puts it back if anything moves — **kept 0 of the 13** on the tree as it
stands. Every one of them holds workspace arrays, `buf[4096]` in front of
`v72[1024]`, and the code walks past their ends. `search_filter` moves five
streams if its frame goes; `model_planes` dies on five of the eight images.
That adjacency is the program's, not Hex-Rays': round one's `reframe.py` put
these frames back after one of them segfaulted for exactly this reason.

So the honest reading is that **this phase is not a rewrite, it is a rename.** The
declarations say `__frame.x` instead of a type, and that is worth fixing, but
the fix is not "make them locals" — it is:

1. **Name the frames.** A frame that survives is an aggregate local, and
   `struct PlaneScratch { … } scratch;` says something `struct alignas(16) { … }
   __frame;` does not. 22 frames, 22 names, each from the function that owns it.
2. **Fold the alias into the member.** Once the struct has a name, the alias
   `int32_t (&v72)[1024] = __frame.v72;` goes and the body says `scratch.hist`
   where it said `v72`. 336 declarations deleted, and one rename per alias
   through its function -- mechanical once the name exists, and a rename is
   answerable to the compiler rather than to the gate.
3. **Split the 25 double-booked slots that can split.** Round three tried and
   the gate kept one of six; the other five are genuinely one variable with two
   Hex-Rays names. Those five want their *name* chosen, not their storage.

Retry the sweep after Phase A regardless: deleting 47 `__m128` elements from
`Obj11` changes what is adjacent to what, and the sweep costs one command.

---

## 6. Order

```
  Phase A   __m128 -> nothing          the thread first, then the members
     │                                 (deleting members changes adjacency)
     ▼
  Phase B   _this + ofs                layout_workspace, then the tables
     │                                 Phase A has to go first: the tables
     ▼                                 overlap the __m128 members
  Phase C   20 tables -> 20 arrays    independent of everything; four of the
     │                                twenty are Phase A's broadcasts
     ▼
  Phase D   frames get names          retry the sweep after A
```

A before B because §3.2's tables overlap the `__m128` members Phase A deletes.
A before D because the same deletion changes which frame members are adjacent,
which is the only thing between the 13 offered frames and the gate. C is
independent and is the cheapest thing here; do it first if a short session is
all there is.

Within each phase: **one function, one struct, one frame at a time, gated** —
and the gate now includes the strict build.

---

## 7. What would make this fail

The five hazards rounds two and three measured, all of which are still live:

* **Pointer arithmetic scales.** `f1078208 + 24 * n` steps 24 bytes on a byte
  pointer and 96 on a `uint32_t *`. The out-of-memory ladder caught this one
  when fifteen images did not.
* **Two names for one field can disagree**, and the merge is the moment to
  resolve it by reading. `ModelBlock::f6059436` against `Obj10::f6059436` moved
  three streams.
* **Signedness is part of a field's type.** `int32_t f0` against `uint32_t f0`
  and one `f0 > -10` is four streams, and no warning fires.
* **A merge retypes fields and the locals downstream cascade** — into
  `-fpermissive` conversions the gate cannot see. Round three lost
  `BMF_STRICT`'s zero twice.
* **A rewrite that keeps the offset can still change the width.** A `uint16_t`
  store through a `uint32_t` member compiles, passes seven images, and corrupts
  the heap.

And two this round adds before it starts:

* **A dead value is not a removable one, and the poison only speaks for paths
  the corpus runs.** For `search_filter` a `__builtin_trap()` probe settled it
  -- the function runs on three images, so the poison covers its stores. Do
  that probe before trusting a poison result about any function whose
  reachability is not obvious, and prefer replacing a value with the constant
  it should be over deleting the write.
* **Deleting a struct member moves everything after it.** Phase A removes 640
  bytes of `__m128` members from `Obj11`. Every `static_assert` on the file's
  offsets is what stands between that and silence — do not relax one to make a
  build pass.

---

## 8. What the gate needs

Nothing new, again, and the reason is worth stating: Phase A's thread runs
through `bmf_compress` and `bmf_decompress`, so every image exercises it, and
Phase B's tables are `alt_p2_alloc`'s, and the allocator runs once per plane on
every image. Phase C's twenty tables are read on every image by construction --
they are the model's constants. Phase D's frames are in the hottest functions
in the file; `REFACTORING2.md` §5 measured `alt_p2_model`, `alt_p2_context` and
`alt_p2_filter` at 100 % line coverage and `choose_plane_coding` at 94.8 %.

Two additions to how it is run rather than to what it is:

* **`BMF_STRICT=1 ./build.sh` belongs in the gate script**, not beside it.
  Round three lost its zero twice and found out late both times.
* **The poison run is the cheapest check this round has.** Re-run it after each
  function the thread leaves; a parameter that stops being dead is a parameter
  that was carrying something.

---

## Appendix A — how the numbers were measured

| number | command |
| --- | --- |
| the §1 table | `python3 tools/shape.py` |
| raw offsets by function | `python3 tools/shape.py --this` |
| one line per frame | `python3 tools/shape.py --frames` |
| what still folds to a member | `python3 tools/unoffset.py subs1.hpp --list` |
| what still lifts out of a frame | `python3 tools/defram.py subs1.hpp --list --arrays` |
| conversions | `BMF_STRICT=1 ./build.sh` (0 today) |
| intrinsics | `grep -cE '_mm_\|__builtin_ia32' subs1.hpp` (0) |
| reference declarations | Appendix B — `grep -c` counts *lines*, and some of these regexes match twice on one |
| the twenty file-scope tables | `grep -cE '^static \w+& \w+ = ' subs1.hpp` |
| the `plane_desc` views and their users | Appendix B |
| what a frame lift costs | `tools/frame-sweep.sh` (5 kept of 10 after §9) |
| shared slots and their names | `python3 tools/unslot.py subs1.hpp --list` |
| what the `f278528` layer became | `python3 tools/unlane.py subs1.hpp --list` |
| what the thread cost | `python3 tools/dethread.py subs1.hpp --list` |
| `__m128` parameters and their lanes | Appendix B |
| the poison run | Appendix B |
| whether a function is reached | `__builtin_trap()` at its top, then `./test.sh` |

## Appendix B — the two experiments this plan rests on

**Which `__m128` parameters are read**, and how:

```
python3 - <<'EOF'
import re, sys, collections
sys.path.insert(0, 'tools'); import structs
lines = open('subs1.hpp').read().split('\n')
for a, b, nm, sig in structs.bodies(lines):
    for p in re.findall(r'const __m128 &(\w+)__ref', sig):
        body = '\n'.join(lines[a:b + 1])
        lanes = collections.Counter(re.findall(r'\b%s\.(m128_\w+)\[(\d)\]' % p, body))
        print('%-26s %-4s lanes=%d' % (nm.lstrip('_'), p, sum(lanes.values())))
EOF
```

**Whether anything observes them** — poison every copy and run the gate:

```
python3 - <<'EOF'
import re
p = 'subs1.hpp'; lines = open(p).read().split('\n')
for i, l in enumerate(lines):
    m = re.match(r'^(\s*)__m128 (\w+) = (\w+)__ref;$', l)
    if m:
        lines[i] = l + '  __builtin_memset(&%s, 0xA5, 16);' % m.group(2)
    if re.match(r'^\s*__m128 v3, v4;$', l):
        lines[i] = l + '\n  __builtin_memset(&v3, 0xA5, 16); __builtin_memset(&v4, 0xA5, 16);'
open(p, 'w').write('\n'.join(lines))
EOF
./build.sh && ./test.sh          # PASS, 31 sites poisoned
```

**The reference taxonomy**, which is where this document's first draft went
wrong -- it counted 336 frame aliases with one regex, 350 lines with another,
and reported the difference as "14 other" without checking that 336 + 20 > 350:

```
python3 - <<'EOF'
import re, sys
sys.path.insert(0, 'tools'); import shape
lines = open('subs1.hpp').read().split('\n')
DECL = re.compile(r'^\s*(?:static\s+)?[A-Za-z_][\w ]*[\w*]\s*\(?&\s*\w+\)?(?:\[\d+\])?\s*=')
allr  = set(i for i, l in enumerate(lines) if DECL.match(l))
frame = set(i for i, l in enumerate(lines) if shape.ALIAS.search(l))
fs    = set(i for i, l in enumerate(lines) if re.match(r'^static \w+& \w+ = ', l))
print(len(allr), len(frame), len(fs), len(allr - frame - fs))    # 384 336 20 28
EOF
```

`REFACTORING3.md`'s Appendix B is the standing warning and it still applies:
every count in a plan should be reproducible by a command that does not go
through the tool being checked. Round three's plan had eleven claims that the
work disproved, and the ones that survived were the ones with an experiment
attached.

---

## 9. What the work did, and where the plan was wrong

Written after the fact, so that the next round starts from what happened rather
than from what was planned. Every number below is one command; Appendix A says
which.

| | before | after |
| --- | --- | --- |
| `__m128` occurrences | 160 | **1**, and it is a sentence in a comment |
| `.m128_` lane accesses | 562 (§2.1's four) | 209, every one in `choose_plane_coding` |
| reference declarations | 384 | 274 |
| `plane_desc` views | 20 | 1 |
| file-scope tables said three times | 20 | 1 |
| frames | 22 | 19 |
| frame aliases | 336 | 265 |
| slots carrying two names | 25, 60 extra | **0** |
| raw-offset sites | 1514 | 1462 |
| `BMF_STRICT` conversions | 0 | 0, and the gate checks it now |

**Phase A is finished.** The 29-parameter thread, its 21 shims, the four
`__xmmword_*` broadcasts, `Obj11`'s eighteen `__m128` members and the union
layer that reached 336 bytes of it are all gone, and so are `__m128`, `__m128d`,
`__int128`, `_OWORD` and `_LONGLONG`. What survives is `choose_plane_coding`'s
six sixteen-byte spill slots, which really are one slot each holding four ints
or two doubles depending on the statement.

**Phase C is finished.** `untable.py` converted fifteen; four of the remaining
five were the `__xmmword_*` broadcasts, which Phase A deleted outright. The one
left is `__dword_439B7C`, which is walked as
`*((uint8_t *)&__dword_439B7C + v83 + 3)` and so stays a table with a typedef.
§4.1 is finished too.

**Phase B is finished to the plan's own limit.** §3.1 is done and §3.2 came
free with Phase A. §3.3 and §3.4 say in the plan itself that they want
`algorithm_v2.md` §9 read first, and they still do.

**Phase D is done except for §5 item 2, which the tree disproved** — see below.

### Five things the plan got wrong

1. **§2.6 item 3 guessed a constant that is a register copy.**
   `search_filter`'s `a3`/`a4` were said to be `xorps`-zeroed and stored. They
   are not: they are scratch for a 64-bit move, and the *fifth* copy of the same
   four-store loop was already written without them. Reading the neighbours
   settled in a minute what a guess would have got wrong.

2. **§4.1 said to keep `__n3_0`.** It is `plane_desc[4].src_plane`, and at its
   28 sites it is read exactly the way records 1, 2 and 3's `src_plane` already
   are — in four functions it stood next to those spellings as the odd one out.
   The name said nothing that the field does not.

3. **§4.1 counted comment mentions as uses.** Six views were dead in code, not
   three.

4. **§5 item 2 does not apply to this tree.** The instruction was to fold each
   frame alias into its member. Measured: **156 of the 265 aliases have the same
   name as their member**, so folding them writes `__frame.x` where the body
   says `x` and gains nothing; the other 109 point at members called `slotN`,
   `sym[k]` or `sub[k]`, which say less than the alias does. The alias is doing
   its job — it makes a frame member read like the local it was. What was worth
   doing instead is item 3 and the fifteen `slotN` members that now carry the
   name of the one local left in them.

5. **§5 said the sweep would keep nothing.** It kept five frames, three of them
   outright, once the shared slots were split — and two of those five were being
   hidden by a `defram.py` bug (duplicate `_gapN` names) that looked like a pin.

### One bug the work found

`plane_desc[HIDWORD(v208) + v205.m128_i32[1] + 1].b3` was
`__byte_44339F[16 * HIDWORD(v208) + v205.m128_i32[1]]` before round three folded
it, and only the first of the two terms got divided by sixteen. A
`__builtin_trap()` on `v205.m128_i32[1] != 0` passes the whole gate, so every
reference image picks colour transform 0 and the two spellings agree there; an
image that picks 1 or 2 writes sixteen or thirty-two records past the table.
Found by re-deriving the scale factor of every index in that commit's diff —
the other 177 hold.

### What is left, in the order it wants doing

1. **`algorithm_v2.md` §9**, which three deferred items now wait on: §3.3's
   `alt_p1_model` table, §3.4's 252 row-cursor displacements, and naming
   `Obj11`'s remaining `fNNNN` members. The 1462 raw offsets do not get smaller
   without it.
2. **`Obj11` and `ModelBlock`'s middle**. Both now have their ends pinned by a
   `sizeof` assert against the allocation. `ModelBlock`'s +3104 region and
   `Obj11`'s +278676..+278735 are named offsets with no roles yet.
3. **The seven frames that stay.** They hold workspace arrays the code walks
   past the end of; `alt_p2_context`, `decode_pixel`, `code_pixel` and
   `decode_symbol_list` segfault the moment their members stop being adjacent.
   That adjacency is the program's, so the deliverable is a comment saying which
   walk needs it, not a lift.
4. **77 structs still called `ObjN`** and 240 `fNNNN` members against 60 named.
