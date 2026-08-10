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
3. **References that should be tables and variables** — 336 of them, all onto
   frame members.
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
| frames | **22**, 169 788 bytes, **336 aliases** |
| — slots carrying two names | 25 slots, 60 extra names |
| — runs walked as arrays | 0 |
| structs | 86, **79 still `ObjN`** |
| `fNN` members | **236**, against 59 named |
| distinct `vNN` locals | 560 |
| `goto` / `LABEL_n:` | 112 / 79 |
| `__fwd_*` shims | 94 |
| `__m128` mentions | **160** — 0 intrinsics, 29 parameters in 15 functions (21 more on shims), 18 struct members, 4 globals, 51 locals |

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

**The experiment:** poison every `__m128 a = a__ref;` copy in the file, all 31
of them, with `__builtin_memset(&a, 0xA5, 16)`, and run the gate.

> **PASS.** Fifteen streams byte-identical, archive, malformed, out-of-memory
> ladder. Nothing in the program observes any of these 29 values.

That is round two's poisoning technique, and it settles the whole thread at
once. Of the 29:

| | |
| --- | --- |
| never read at all | **25** |
| `alt_p2_model::a2` | read 4 times — **after being overwritten** (§2.3) |
| `search_filter::a3`/`a4` | written to memory as `m128_u64[0]`; the poison reaches that memory and no stream moves |

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
3. **`search_filter::a3`/`a4` become the constant they write.** The poison says
   the value is not observed; what it *should* write is the question, and
   `xorps xmm, xmm` before a 16-byte store is the idiom to look for. If nothing
   reads the destination, say so and drop the store.
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

| where | sites | what it is |
| --- | --- | --- |
| `alt_p1_model` | 48 | one counter table, computed indices |
| `layout_workspace` off `a1` | 50 | the p1 allocator, still taking a `uintptr_t` |
| `alt_p2_alloc` | 31 | two counter tables |
| row cursors (`v45`, `v46`, `v12`, `v13`) | 182 | literal byte offsets off a cursor |
| the rest | ~1200 | locals whose object has no struct |

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

## 4. Phase C — the 336 references

Every reference declaration in the file is now a frame alias; there are no
others. 140 are the plain `T &v = __frame.m;` form and the rest carry a cast or
a subscript.

`defram.py` offers 249 of them and **the gate has refused every batch,
sixteen times.** The frames that will not dissolve hold workspace arrays —
`buf[4096]` in front of `v72[1024]` — and the code walks past their ends.
`search_filter` moves five streams if its frame goes. That adjacency is the
program's, not Hex-Rays': round one's `reframe.py` put these frames back after
one of them segfaulted for exactly this reason.

So the honest reading is that **§4 is not a rewrite, it is a rename.** The
declarations say `__frame.x` instead of a type, and that is worth fixing, but
the fix is not "make them locals" — it is:

1. **Name the frames.** A frame that survives is an aggregate local, and
   `struct PlaneScratch { … } scratch;` says something `struct alignas(16) { … }
   __frame;` does not. 22 frames, 22 names, each from the function that owns it.
2. **Fold the alias into the member.** Once the struct has a name, `__frame.v72`
   can become `scratch.hist` and the alias goes. That is 336 declarations
   deleted and 336 use-sites-per-name rewritten, mechanical once the name
   exists.
3. **Split the 25 double-booked slots that can split.** Round three tried and
   the gate kept one of six; the other five are genuinely one variable with two
   Hex-Rays names. Those five want their *name* chosen, not their storage.

The 10 frames that do dissolve outright have already been offered and refused
as a group; they should be retried one at a time after Phase A, because
deleting the `__m128` members changes what is adjacent to what.

---

## 5. Order

```
  Phase A   __m128 -> nothing          the thread first, then the members
     │                                 (deleting members changes adjacency)
     ▼
  Phase B   _this + ofs                layout_workspace, then the tables
     │                                 Phase A has to go first: the tables
     ▼                                 overlap the __m128 members
  Phase C   frames get names           retry the dissolves after A
```

A before B because §3.2's tables overlap the `__m128` members Phase A deletes.
A before C because the same deletion changes which frame members are adjacent,
which is the only thing standing between the ten clean frames and the gate.

Within each phase: **one function, one struct, one frame at a time, gated** —
and the gate now includes the strict build.

---

## 6. What would make this fail

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

* **A dead value is not a removable one.** The poison run says nothing observes
  the `__m128` thread *on this corpus*. `search_filter` is reachable and its
  `m128_u64[0]` stores land in memory; if no test image drives that path, the
  poison proves nothing about it. Check reachability before deleting a store,
  and prefer replacing the value with the constant it should be over deleting
  the write.
* **Deleting a struct member moves everything after it.** Phase A removes 640
  bytes of `__m128` members from `Obj11`. Every `static_assert` on the file's
  offsets is what stands between that and silence — do not relax one to make a
  build pass.

---

## 7. What the gate needs

Nothing new, again, and the reason is worth stating: Phase A's thread runs
through `bmf_compress` and `bmf_decompress`, so every image exercises it, and
Phase B's tables are the p2 model's counters, which `REFACTORING2.md` §5
measured at 100 % line coverage. Phase C's frames are in the hottest functions
in the file.

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
| `__m128` parameters and their lanes | Appendix B |
| the poison run | Appendix B |

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

`REFACTORING3.md`'s Appendix B is the standing warning and it still applies:
every count in a plan should be reproducible by a command that does not go
through the tool being checked. Round three's plan had eleven claims that the
work disproved, and the ones that survived were the ones with an experiment
attached.
