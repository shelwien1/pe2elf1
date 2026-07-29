# Plan 4 — retire `std::vector` and `std::string`

Sixteen sites. The goal is that `xadpcm.cpp` stops including `<vector>` and
`<string>`, the coding path contains no hidden allocation, and every buffer's
size is either a contract constant or a number the codec computed before it read
its first coded byte.

**What this is worth, stated up front so the benchmark is not a surprise:** not
speed. Twelve of the sixteen are cold — argv parsing, the file table, the `-t`
verify buffers — and the other four allocate once per *segment*, not per block.
The measurable wins are (a) `assign(n, 0)` currently zero-fills up to 8 MB per
segment for buffers whose zero may not even be load-bearing, (b) the decode side
sizes two containers from attacker-supplied header fields, and (c) the core
drops its last standard-container dependency, which is the same move
`d5c_logistic.inc` describes at its foot. Treat a flat benchmark as success.

**Hard acceptance criterion, as always: byte-identical output.** All 24 md5s from
`./md5s.sh` unchanged, in both build modes. This plan must not cost a byte —
unlike the logistic port, there is no reason here for a single bit to move.

---

## 1. Inventory

`ba` = `pm.block_align`, a `u16` from the fmt chunk, so `ba <= 65535`;
`chans <= MAX_CHANS` (8); `bps` in [2,5]. `sizeof(Seg)` is 344.

| # | site | holds | worst-case size | when |
|---|---|---|---|---|
| **V1** | `xad_codec_ima.inc:47` `vector<u8> cbuf[MAX_CHANS]` | per-block nibble scratch | `ba+8` per ch → **512 KB** | per segment, inside the coroutine |
| **V2** | `xad_codec_ima.inc:48` `vector<int> code[MAX_CHANS]` | per-block codes | `ba*8/bps+8` ints per ch → **8 MB** at bps=2 | per segment, inside the coroutine |
| **V3** | `xad_codec_ms.inc:17` `vector<u8> nib` | per-block nibbles | `ba*2+8` → **128 KB** | per segment, inside the coroutine |
| **V4** | `xad_front.inc:41` `Xad::src` | the whole container image | = input file, unbounded | member, whole run |
| **V5** | `xad_front.inc:42` `in_names`, `in_sizes` | one per input file | `argc` | encode, cold |
| **V6** | `xad_arc.inc:16` `Arc::segs` | segment table | encode: input/44. decode: header field, capped 2^24 → **5.4 GB** | cold |
| **V7** | `xad_arc.inc:20` `Arc::names`, `sizes` | member file table | decode: capped 2^20 entries × 4096 name bytes → **4 GB** | cold |
| **V8** | `xad_arc.inc:42` `vector<size_t> cand` | scan hits | as V6 | one pass, cold |
| **V9** | `xadpcm.cpp:304` `vector<u8> coded` | `-t`: archive held back until verified | compressed size | `-t` only |
| **V10** | `xadpcm.cpp:320` `vector<u8> back` | `-t`: the decode-back image | = input size | `-t` only |
| **V11** | `xadpcm.cpp:384` `vector<char*> a` | positional args | `argc` | cold |
| **S1** | `xadpcm.cpp:356` `string dir` | out path + separator | PATH_MAX-ish | cold |
| **S2** | `xad_drive.inc:32` `out_dir` | same, kept on the driver | | cold |
| **S3** | `xad_front.inc:44` `name` | name for a diagnostic | | cold |
| **S4** | `xad_front.inc:332` `string s` | member name being parsed | capped 4096 | decode, cold |
| **S5** | `xad_util.inc:57` `safe_name(const string&)` | basename of a member | capped 4096 | cold |
| **S6** | `xad_drive.inc:78` `string path` | `out_dir` + member name | | cold |

Already fixed arrays, and the precedent this plan follows:
`XadDrive::inpbuf`/`outbuf` (`ALIGN(4096) byte [1<<16]`), `Params::coef`,
`Codec::ch[MAX_CHANS]`, everything in `xad_logistic.inc`.

---

## 2. Three mechanisms, and the rule for picking one

**(A) Plain fixed array.** Where the bound is a contract constant that already
exists in `xad_prelude.inc` / `xad_wav.inc`, and the worst case is small.
Sub-rule: **≤ ~4 KB if it lands on the coroutine stack, ≤ a few MB if it lands
in BSS.** BSS is demand-zero — untouched pages cost address space, not RAM — so a
generously sized static array is nearly free, and `xad_counter.inc` already leans
on exactly that for the match tables.

**(B) A block from the existing `Arena`, sized at `Codec::init`.** Where the
bound is runtime geometry that is nonetheless settled before the first coded
byte. `Codec::reset_stats()` already runs the whole binding pass **dry** — `take()`
hands out nothing and only accumulates — sizes the arena from the result, and
re-binds. Adding a buffer to `bind_tables()` is three lines and it inherits the
dry pass, the single mmap, the per-segment `rewind()`, and `check()`.

**(C) A small owning buffer, `Buf<T>`.** Where the size is genuinely input- or
attacker-driven and a maximal array is not an option. Append and index is all any
of these sites asks for. 1.5× growth, so N appends stay amortized O(N);
`abort()` on allocation failure, which is what `vector` does today under
`-fno-exceptions` anyway, only with a message. Non-copyable, so an accidental
by-value pass of an `Arc` becomes a compile error instead of a silent 5 GB copy.

| site | mechanism | why |
|---|---|---|
| V1, V2, V3 | **B** | geometry known at init; 8 MB is too much to reserve blindly and exactly right when sized from the actual `ba`/`chans`/`bps` |
| V4, V6, V7, V8, V9, V10 | **C** | input-sized; a maximal array is 5.4 GB |
| V5, V11 | **C**, or **A** at `argc` | trivially small either way; C keeps one mechanism |
| S1–S6 | **A** — a fixed `char[]` path buffer | member names are already capped at 4096 by the header validator, and paths have an OS limit |

---

## 3. Why **not** IDX `Table()`

`Table(Type, Name, Size)` emits a fixed array member in the Const build and a
`new[]` pointer allocated in `M_Init()` in the Debug build. It exists so a
**model table** whose row count is an IDX `_Volume` expression has one spelling
in both build modes.

Not one of the sixteen sites has an IDX-derived size:

* V1/V2/V3 are sized from `block_align`, `chans` and `bps` — values read out of
  a RIFF header at runtime. No `_Volume` can express them.
* V4/V6/V7/V8/V9/V10 are sized by the input.
* The nearest thing to a fit is `Codec::nib_rows`, which *is*
  `max(M0_Md_Volume, I0_Ib_Volume)` — but maximised **over the segments actually
  present**, so `Table()` could only express it by always allocating the larger
  of the two even for a pure-IMA stream.

And the tree does not use `Table()` at all today: all four IDX modules emit only
`MakeIndex`. Adopting it means writing `anew<T>()` and `tbl_n()` (neither exists
here), threading `M_Init()`/`M_Quit()` into the codec's lifecycle, and taking
`new[]` for storage the tree deliberately moved to one mmap'd `Arena` with a dry
measuring pass, huge pages, per-segment `rewind()` and an overrun `check()`.

**Recommendation: do not adopt `Table()` for this.** Mechanism B is the same idea
— declare a size once, let something else lay it out — implemented better for
this codec and already in the tree. The thing that would change the answer is a
table whose row count becomes a pure IDX Volume with no runtime max over it;
none is.

---

## 4. The dangerous part: V1–V3 and the 64 KB coroutine stack

`Codec` is a member of `Xad<MD> : RC<MD> : Coroutine`. `code_data_ima` and
`code_data_ms` run *inside* that coroutine's call chain — when the range coder's
window runs dry deep in the bit tree, `yield()` copies `stkptrH - stkptrL` bytes
into `Coroutine::stk[STKPAD]`, **64 KB, with no bound check**, and an overflow
there is an intra-object overwrite ASan cannot see. `xad_rc.inc` already carries
that warning and the `stk_probe()` that measures it; the current high-water is
**1345 of 65560 bytes**.

Today only the vector *handles* are on that stack — three pointers each, about
400 bytes for all of V1–V3. **Turning them into automatic arrays puts up to
8.6 MB there and destroys the coroutine.** It would not crash cleanly; it would
present as a wild jump.

So V1–V3 go to the arena (B) or to BSS (A) — never to a local array. This is the
one line of this plan that, if ignored, silently breaks everything.

### The `assign(n, 0)` question

All three are created zero-filled today. Whether that zero is load-bearing has to
be **established, not assumed** — `put_bits` ORs into `cbuf`, so a stale high
nibble in the final partial byte of a block would leak into the output, and that
is exactly the class of bug that round-trips fine while changing the archive.

Procedure, in this order:
1. Move to arena/BSS storage and keep an explicit `memset(p, 0, n)` where
   `assign` was. Verify all 24 md5s. This isolates the storage change.
2. Then narrow or drop the memset. Verify again. **If any md5 moves, the zero
   was load-bearing** — put it back, sized to the bytes the block actually uses.

Step 2 is optional. Step 1 is the plan; step 2 is the only place a speed win
lives, and it is worth at most one memset per segment.

### Sizing

`arc_geometry()` already walks every segment to compute `maxsym`/`maxnib`. Extend
it with `maxba`, `maxch` and `minbps`, pass them into `Codec::init` beside
`maxsym`/`maxnib`, and bind in `bind_tables()`:

```
cbuf_all : maxch * (maxba + 8)                  bytes
code_all : maxch * (maxba*8/minbps + 8)         ints
nib      : maxba*2 + 8                          bytes   (MS only; overlaps nothing)
```

For the shipping test files (`ba` 2048, 2 ch, 4 bps) that is 4 KB + 32 KB + 0,
against the 8 MB a blind maximal array would take. The dry pass sizes it, the
`check()` catches a geometry that changed after init, and `rewind()` already runs
per segment.

---

## 5. Order of work

Each step ends with `./md5s.sh` and a diff against the recorded 24. Nothing moves
to the next step with a changed md5.

1. **`Buf<T>` in `xad_util.inc`.** New type, no users yet. Non-copyable,
   move-assignable, `data()`/`size()`/`push()`/`resize()`/`clear()`, 1.5× growth,
   `abort()` with a message on OOM.
2. **V11 and S1–S6** — argv and the path strings. Smallest, coldest, and it
   proves the `char[]` path handling before anything harder depends on it.
3. **V9, V10** — the `-t` buffers. Self-contained in `compress()`.
4. **V4 `src`** — the largest single buffer, but only `assign`/`data`/`size` are
   used on it, so it is a direct swap.
5. **V6, V7, V8** — `segs`, `names`, `sizes`, `cand`. `names` needs `Buf<T>` over
   a fixed-width name record rather than over `string`.
6. **V5** — `in_names`/`in_sizes`; note `in_sizes.back() += l` in
   `XadDrive::feed_files`, which needs a `back()` or an explicit index.
7. **V1–V3** — last, deliberately. It is the only step that can change the
   archive, so everything before it is already proven by the time it lands.
8. **Drop `#include <vector>` and `<string>`** from `xadpcm.cpp`, and add a
   comment saying what replaced them, the way `<cmath>`'s removal is recorded.

---

## 6. Risks, and the check for each

| risk | check |
|---|---|
| V1–V3 land on the coroutine stack | `-v` reports the stack high-water; it must stay ~1345 bytes, not jump |
| the `assign` zero was load-bearing | the two-step procedure in §4; any md5 move means it was |
| arena overrun from a wider second geometry | `Arena::check()` already aborts on it; the dry pass must include the new blocks |
| `Buf` copied by value somewhere `vector` was | make it non-copyable — the compiler finds them all |
| `.data()` on an empty `Buf` | V9 already guards with `!coded.empty()`; `Buf::data()` returns null when empty, same as `vector` |
| a fixed path array truncates a long path | build the path with a bounded `snprintf` and fail loudly on truncation, rather than silently writing to the wrong file |
| OOM now aborts instead of terminating | same observable outcome under `-fno-exceptions`, better message; note it in the source |
| memory got *worse* | `/usr/bin/time -v` max RSS on `wavs2` and on a 20 KB input, before and after — the 20 KB case is the one a blind maximal array would ruin |

---

## 7. Verification protocol

```
./mk.sh && ./md5s.sh          # Debug build: 24 md5s
./mk.sh release && ./md5s.sh  # Const build: the same 24
./verify.sh                   # round-trips, recorded sizes, malformed input
./ktest.sh                    # kernel differential test
./sqtest.sh                   # logistic tables, 8 ST_SCALE values
```

plus, for this change specifically:

* `-v` on both test files, both directions: **coroutine stack high-water
  unchanged**, model arena sizes unchanged or smaller.
* max RSS before/after on a 2.5 MB and a 20 KB input.
* an ASan + UBSan build over the whole round-trip suite — hand-rolled buffers are
  exactly what ASan is for, and the tree has run it before.
* the compiler/flag matrix once at the end, not per step.

---

## 8. Deliberately not changed

* **`Arena`.** It is the destination, not a target.
* **`FILE*` I/O and the 64 KB `inpbuf`/`outbuf`.** Already plain arrays.
* **`Params::coef[MS_MAXCOEF][2]`, `Codec::ch[MAX_CHANS]`,** and every table in
  `xad_logistic.inc` / `xad_ima.inc` / `xad_ms.inc`. Already fixed arrays.
* **The archive format.** Not one byte of it.

## 9. Noted while surveying, out of scope unless asked

The decode-side header caps are `nseg <= 2^24` and `nfile <= 2^20`, validated
before either table is sized. At the cap those are a 5.4 GB and a 4 GB
allocation from a ~40-byte malicious header. `Buf` does not make that worse — it
is there today — but the natural tightening is to bound `nseg` by
`total/44` and `nfile` by `total`, both of which are known by then. Say the word
and it becomes step 5a; otherwise this plan leaves the caps exactly as they are.
