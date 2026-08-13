# Round 11 — the second pointer width

Ten rounds asked one question of one target. `test.sh` says fifteen streams are
byte-identical and `BMF_STRICT=1 ./build.sh` says zero conversions need
`-fpermissive`, and both have been true for six rounds. Both are also nearly
blind to the largest single class of assumption left in the file, because on
i386 that class is not an error:

* `(int32_t)p` is not a narrowing when a pointer is four bytes wide.
* `&p[-n]` with `n` unsigned is exactly `p - n`, because `ptrdiff_t` is 32 bits.
* `((uint8_t **)blk)[k]` and `((uint32_t *)blk)[k]` are the same address.
* `284712` and `offsetof(AltP2Block, p2_ctr)` are the same number.
* `sizeof(SymList)` and `24` are the same number, and `4 * n + 16` is the right
  size for an array of `n + 4` pointers.

Every one of those is a fact about i386 written in a place the compiler is not
checking. This round asked the same questions of x86-64, and answered them.

**Result: `tools/x64.sh --high` reports 22 of 22 cases agreeing with the
32-bit build, and it is in `test.sh`.** Fifteen images compress to the *32-bit*
reference stream byte for byte and expand back to what the decoder should
write; the two-member archive reads back to its last member; five malformed
inputs are refused with the program's own exit codes; and the out-of-memory
ladder still reports and exits 7 on a target whose records are bigger — all of
it with every allocation at an address a four-byte pointer cannot name, which
is what makes it a claim about this file rather than about where Linux happens
to put a heap.

---

## 1. What the scoreboards could and could not see

Six instruments, and the order matters — each is silent about the class the
next one sees, and the last one is the only one that is *evidence* rather than
an absence of complaint.

| instrument | what it sees | found this round |
|---|---|---|
| `BMF_STRICT=1 BMF_BITS=64 ./build.sh` → `tools/ptrwidth.py` | a pointer *narrowed*: the compiler diagnoses it | 112 |
| `tools/rawoffset.py`, `tools/negindex.py` | a number that is an offset; an index negated in an unsigned | 32 + 4 |
| `tools/x64.sh` | the program giving a different answer | the rest |
| `BMF_BITS=64 tools/fuzz.sh` | a *crash* on an input nobody chose | 1, and none of the above sees it |
| `tools/x64diff.sh` | a different *answer* on an input nobody chose | the last one |
| `tools/x64.sh --high` | a truncation that survives, by putting every allocation where a four-byte pointer cannot reach | see section 3c |

The middle row is the interesting one. Round nine's census had a row reading
`raw-offset sites 0`, and it was a spelling: the measure looked for
`_this + ofs`, which is what Hex-Rays emits for a *small* member, and every one
of those had been named. What it could not see was the same thing written as an
index into a byte view of the record —

```c
bankp = (uint8_t *)blk + (bank << 17);
d4000 = (P2Count *)&bankp[4 * (ctxw ^ 0x4000) + 284712];
```

— thirty times over in one body. `tools/rawoffset.py` works the other way
round from a grep: it reads the offsets the file *states* (the `// +N` on each
member, the `__builtin_offsetof(...) == N` in the static_asserts) and looks for
those numbers in the code. Its row is in the census now, directly under the row
that said zero.

The fourth row is where the rest came from. `tools/x64.sh` builds the same
source as x86-64 and runs it; the failures were read one at a time with an
ASan build and a `printf`, and each one, once found, was a class rather than a
site.

---

## 2. The classes, and what closed each

### 2.1 A pointer in a 32-bit slot — 112 sites

`tools/ptrwidth.py` reads `strict64.log` and sorts them into the four things
they turn out to be. Only one kind can be rewritten without reading, which is
why the tool is a classifier and not a rewrite:

| kind | count | the fix |
|---|---|---|
| `roundtrip` — `(T *)((int32_t)p)`, the integer never reaching a variable | 26 | delete the cast (`--apply`) |
| `slot` — `x = (int32_t)p`, stored | 39 | the *slot's* type |
| `arg` — a pointer handed to an `int32_t` parameter | 14 | the callee's signature |
| `arith` — inside a comparison or an expression | 33 | read it |

The fact that the operand is a pointer comes from gcc and not from a regex,
which is what keeps `--apply` off the one shape that must never be touched: an
integer, where the cast is a real narrowing and deleting it changes the value.

Three of the `slot` clusters were the same defect at scale:

* **Frame unions whose arms only line up at four bytes.** `decode_pixel` and
  `code_pixel` each keep a 32-entry symbol history in a union whose other arm
  is the locals MSVC spilled into the same bytes; four of those locals were
  declared as pointers. Give a pointer eight bytes and the fifth entry of a
  thirty-two-word history is two words. Two of them were `this`, spilled and
  reloaded fifteen times between them.
* **`__search_filter`'s sixty-four bytes.** `plane_i`, `costs[4]`, `pi`,
  `tile_buf`, `flag8`, `best_flags` and a 28-byte pad are not seven locals:
  they are `PlaneDesc saved[4]`, written through three *other* members with
  indices that walk out of all three (`__frame.dims[17]` is fourteen words past
  `dims[2]`). Ten loops became two lines each. The seven locals are the union's
  other arm, and the boundary between the two lifetimes is `free(tile_buf)`.
* **The interleave, four times over.** `search_filter` carries three copies and
  `expand_image` and `compress_image` one each; all of them spilled the read
  cursor, the row counter and the row offset into frame slots, and carried the
  stride in a `uint8_t *` that they counted down by subtracting one from an
  address. `off = (int32_t)&stride[off]` is `off += stride`.

`BMF_SPILL_PAD` is the general answer where a union has to stay: nothing on
i386, four bytes elsewhere, so a 32-bit name declared beside a pointer keeps a
pointer's stride and the i386 layout the static_asserts pin does not move.

### 2.2 A size or a stride written as the number it is on i386

`SymList` holds a pointer, so it is 32 bytes and not 24 on x86-64, and the
padding in front of `SymListBlock::list` is 8 and not 4. The file stated that
layout four more times in numbers: `bmf_new(24 * n + 4)` at four sites,
`(uint32_t *)list - 1` to step back to the count, and three copies of a loop
that freed the sixteen lists' entries by walking six words at a time and taking
word five. All four are the record's own arithmetic now.

Six more allocations asked for a number that is a 32-bit `sizeof`: `0x99D4D8`
for `AltP1Block`, `0x7BA230` for `ModelBlock`, `0x103E30` for `AltP2Block`, and
three strides — including `4 * img_w + 16` for a buffer of *pointers*, which is
half the memory it needs.

### 2.3 A record addressed by width rather than by name

Fourteen sites reached `AltP1Block` as `((uint8_t **)blk)[k]`; the indices
decode against the layout the struct already states, so
`((uint8_t **)blk)[4 * ctx[0] + 950]` is `blk->counters[blk->ctx[0]]`. Four
sites reached `ModelBlock`'s frequency records as
`&this->row_cur[4 * bucket_idx + 10]` — `row_cur` is ten pointers ending where
`grid` begins, and `FreqRec` is sixteen bytes, so four of `row_cur`'s slots make
one record. `free_workspace` freed the five row buffers as
`((void **)blk)[i + 14]`. `layout_workspace` seeded the run buckets through
`*(uint8_t *)(*(uint32_t *)&blk->run_bucket + x + 1)`.

And thirty in `alt_p2_model` plus one in `alt_p2_d8_decode_body`, which is
section 1's `rawoffset` row.

### 2.3a A pointer laundered through `uintptr_t`

`(uintptr_t)p` is exact on every target, and `(int32_t)` of it is an ordinary
narrowing of an integer — so `(int32_t)(uintptr_t)p` truncates the pointer and
*neither* target diagnoses anything. The hop launders it. Four were left after
the 112 the compiler found:

* `rc_begin_decode` returned `(int32_t)(uintptr_t)out_cursor`, and none of its
  three call sites reads the value.
* `cost_candidate` parked `desc` in `__frame.off_up`, a slot that carries a
  *row offset* eleven lines later — a spill nothing reloads.
* `compress_image` took the low byte of `coded_buf` as a flag and then
  overwrote it with 1 if the pointer was non-null.
* `model_plane` aligned `hist_scratch` with `& 0xFFFFFFF0`, which drops the top
  half of any address above four gigabytes. That one is a defect and not just a
  spelling; it works today because the allocation happens to be low.

`tools/ptrwidth.py --laundered` is the rule, and it is counted into that tool's
total, because a tool that reads the compiler's diagnostics and stops there
would report zero while these four stood.

Two more matched it and are not defects: `(uint8_t)(uintptr_t)hist & 0xF` takes
the pointer's low four bits, which every target preserves. They are written
`(uintptr_t)hist & 0xF` — the mask says everything the cast did.

### 2.3b A pointer parked in a name

The hop does not have to be a chain. Put the address in a *variable* and the
cast and the narrowing land on different lines, with a name between them — so
the rule above, which reads one line at a time, sees nothing:

```
pair_ctx = (uintptr_t)h0;                 // __alt_p2_model
...
__update_binary_pair(model_strip((uint32_t)pair_ctx - 1), (a4 - 1) >> 1);

step_v = (uintptr_t)p2_rec;               // step_v is a uint32_t
...
return step_v;
```

Two of them, and both are the same artifact: one machine register held a small
integer at one point in `__alt_p2_model` and an address at another, and
Hex-Rays merged the two roles into one variable. Which is also why both are
dead. `pair_ctx`'s store cannot reach any read of it — the block it sits in has
no `goto`, `break` or `continue` out of it and its only exit is the `return`
below, so the next read is on the path where that branch was not taken.
`step_v`'s store reaches only that same `return`, and no caller of
`__alt_p2_model` uses its result.

Dead is not absent. `(uint32_t)pair_ctx` is the identity on i386 and is not on
x86-64, and under `-DBMF_HIGH_ARENA` both are half an address that nothing
happens to read. The address role keeps the name it already had — `h0` and
`p2_rec` are right there — which is what `node` in `AltP1Block::update_model`
was given in round nine, where the merged variable was `result` and the
mis-read cost eight bytes of phase.

`tools/ptrwidth.py --parked` is the rule: a name assigned `(uintptr_t)…` that
is either declared narrower than a pointer or read through a narrowing cast.
It first reported four; the other two are `(uintptr_t)hist & 0xF`, where the
mask has already discarded every bit the narrowing would, so the constant's
width decides and not the target's. That exemption is the rule's, not a reader's
— `& ~15` keeps the top half and is still reported.

Four locals were mistyped `uintptr_t` beside them and are now what their
siblings already were: `result` (plane zero's context word, `int32_t` like
`ctx0`…`ctx6`), `code` and `code1` (both from an `int32_t` decode, `int32_t`
like `code0`, `code2` and `code3`), and `off3` (a plane number, `int32_t` like `off0`,
`off1`, `off2`). None was a pointer; each was a value whose width changed with
the target for no reason.

### 2.4 An index negated in an unsigned type

`&p[-n]` with `n` a `uint32_t` is `p + 0xFFFFFFFF...`. On i386 the index
converts to a 32-bit `ptrdiff_t` and the value is exactly `-n`. Four sites,
each of them the next thing `tools/x64.sh` died on, and one of them
(`cursor[0] + back`, with `back = -i`) spelled as an addition rather than a
subscript, which is why `tools/negindex.py`'s rule does not see it.

The same wrap in a different costume closed the first x64 crash:
`reduce_alphabet`'s distinct-symbol table is reached as
`*(uint32_t *)&buf[4 * sym - 4]`, and for symbol 0 that is `0xFFFFFFFC`.

### 2.4a A union slot read at the wrong width

`choose_plane_coding`'s frame carries three unions, each holding two 32-bit
quantities before its 3x3 solve and one 64-bit one after. Three sites in the
first phase read the *64-bit* name:

```c
px = &((uint8_t *)img_a)[__frame.q0 + 16 + n_planes + x3[2]];
```

`q0` is `row_stride` and `plane_b` together, so the index is
`row_stride + (plane_b << 32)`. On i386 the index converts to a 32-bit
`ptrdiff_t` and the top half falls off, which is why it has always worked.
With a 64-bit pointer it is eight gigabytes of offset, `px` starts past
`data_end`, and the DC-offset histogram never runs at all: `pos` stays -1,
`plane_desc[k].dc` comes out 0 instead of 42, and three of the fifteen streams
differ by a few dozen bytes **while still round-tripping perfectly**.

That last clause is the whole reason section 3b exists.

### 2.5 Not a defect at all

`rle4` and `rle8` looked like the last two failures for an afternoon. An
RLE-compressed BMP does not come back byte for byte — BMF decodes the runs on
the way in and re-encodes them with its own splitting — and `test.sh` has
compared those two against `testfiles/out_<name>.bmp` since round four.
`tools/x64.sh` was comparing against the input. Two of the three images that
were "still broken" were the instrument.

---

## 3. What is in the gate now

```
./build.sh                          # -m32, as before
BMF_BITS=64 ./build.sh              # the same source as x86-64
BMF_STRICT=1 BMF_BITS=64 ./build.sh # strict64.log, ptrwidth.py's worklist
BMF_WARN=1  BMF_BITS=64 ./build.sh  # warn64.log, ratcheted by warn64.txt
tools/x64.sh --high                 # 22 cases, and test.sh runs it
```

The logs are named after the target because the `cksum` stamp
(`tools/buildlog.py`) says which *version* of the source a log describes and
could never have said which *target*.

`test.sh` gained one line, and it asks the *strong* form: `tools/x64.sh
--high`, with every allocation above 4 GB. The argument for that is section 3c
and its control. `BMF_X64_GATE=0` skips it, for a host with no 64-bit
toolchain, and `x64.sh` falls back to the ordinary 64-bit build, with a line
saying so, where the arena cannot be placed.

Every other instrument takes `BMF_BITS=64` in front of it, and all of them are
clean on the 64-bit build:

```
BMF_BITS=64 tools/asan.sh        no report in 40 runs over 17 images
BMF_BITS=64 tools/fuzz.sh        400 mutants: 288 refused, 112 accepted,
                                 0 reported -- the 32-bit tally exactly
BMF_BITS=64 tools/hdrscan.sh     no report in 7680 runs: both one-byte header
                                 fields, every value, 15 streams
tools/x64.sh                     22 of 22, streams compared
tools/x64.sh --high              22 of 22 with every allocation above 4 GB
BMF_HIGH=1 tools/x64diff.sh 400  400 mutants through *both* builds, the 64-bit
                                 side above 4 GB: same exit code, same bytes
```

Those first three are re-takes. They were run once before section 3c and
reported clean about an allocator AddressSanitizer cannot instrument; these are
against real `malloc`.

And both targets produce byte-identical streams at `-O0`, `-O1` and `-O3` --
six builds, ninety streams. That is the check that nothing left in the coding
path depends on undefined behaviour the optimiser is free to reinterpret,
which for a file this full of reinterpreted memory is not a foregone
conclusion.

The fuzz line is the same tally the 32-bit run gives, mutant for mutant, which
is the check that section 3a's `plane_b` was the whole of what it found.

`build.sh`'s header used to say "-m32 is not negotiable" and gave three
reasons: i386 calling conventions on the moved entry points, CPUID helpers in
i386 inline asm, and the address blob. All three were true when it was written
and all three were gone by round seven. **A reason in a comment is a
measurement, and that one had not been re-taken in four rounds.**

---

## 3a. What the fuzzer found that no static rule could

Running `tools/fuzz.sh` under `BMF_BITS=64` — 400 mutants of 33 seed files
through a 64-bit ASan build — reported two crashes where the 32-bit run
reported none, and both were the same site. Neither mutant touches a header:
`altp1.186.bmp` differs from `testfiles/altp1.bmp` in five *pixels*.

`choose_plane_coding`'s frame carries `uint32_t plane_b`, which is
`plane_desc[2].src_plane - x3[2]` — a difference of plane numbers, so it can be
negative — and `plane0 + __frame.plane_b` is then a pointer plus 0xFFFFFFFF-ish.
Its neighbour in the same union, `plane_a`, carries the note "Signed: it is a
difference of plane numbers and the second reference can sit before the current
plane", added in an earlier round to kill eight sign conversions. The same
argument was true of `plane_b` and nobody made it.

No static rule here catches that. `tools/negindex.py` looks for a *subscript*;
this is an addition. `tools/ptrwidth.py` sees no truncation, because there is
none. `tools/rawoffset.py` sees no offset. The compiler is silent on both
targets. What found it is running the program on inputs nobody chose, on the
target where the mistake has a consequence — which is the whole argument for
`BMF_BITS=64 tools/fuzz.sh`, and it is the fourth instrument in section 1's table.

---

## 3b. The instrument that never ran

`tools/x64.sh` compared each 64-bit stream against `testfiles/$n.bmf`, guarded
by `[ -f ]`. The reference streams are called `testfiles/ref_$n.bmf` -- which
is what `test.sh` calls them -- so the guard was false for every image and the
comparison, the entire reason that script exists, never ran once. It reported
"23 of 23" and what it had checked was 23 round trips.

A round trip is a weak claim here: the encoder and the decoder are the same
build, so a model that has drifted agrees with itself. The three streams the
`q0` defect changed decode perfectly.

Two smaller faults came with it. The glob was `testfiles/*.bmp`, which picks
up `out_rle4.bmp` and `out_rle8.bmp` -- not inputs, but what the decoder is
expected to *write*, and `test.sh` excludes them. And a missing reference was
a skip; it is a failure now, for the reason above.

The count is 22 of 22 with the comparison running, the out-of-memory ladder
added, and `out_rle4`/`out_rle8` no longer counted as inputs.

---

## 3c. The safety net that was hiding the whole class

`bmf.cpp` carried, for 64-bit targets only, an arena *below* 4 GB and a
`#define malloc` onto it. Its note said why:

> BMF's structures keep pointers in 32-bit fields. At 32 bits that is merely
> untyped; at 64 it is lossy. The clean fix is to widen the fields, and
> REFACTORING.md section 4 records why that cannot be done here: every one of
> the 159 objects is walked with variable offsets as well as constant ones, so
> moving a field moves the arrays after it.
>
> This is the other way, and it is the only one left: put every allocation the
> program makes below 4 GB, so a 32-bit field holds a whole address.

Every clause of that is now false. `tools/ptrwidth.py` reports zero, and every
one of those objects is reached by name. But while it stood it did something
worse than being unnecessary: **with every allocation under 4 GB, truncating a
pointer to 32 bits is the identity and nothing fails.** The scoreboard this
round drove to zero was measuring a program that could not have failed the
test.

So the arena is gone, and what replaces it is the check it was standing in
for. `-DBMF_HIGH_ARENA` puts every allocation at 0x200000000000 — an address an
eight-byte pointer can name and a four-byte one cannot — so any surviving
32-bit pointer field faults on its first dereference. `tools/x64.sh --high`
runs the whole corpus that way, and the fifteen streams still come out byte for
byte.

It cost the other instruments too, and quietly. The arena was a bump allocator
with `#define malloc` over it, so on the 64-bit build **AddressSanitizer had no
redzone on a single one of the program's allocations** — every heap overflow it
is there to find went into the next object in the arena and was never seen.
`BMF_BITS=64 tools/asan.sh`, `tools/fuzz.sh` and `tools/hdrscan.sh` were all
reporting clean about an allocator ASan cannot instrument. They are re-taken
here against real `malloc`.

The control says what that is worth. One truncated store, on a slot that is
reloaded a few lines later:

```c
__frame.slot7 = (ModelBlock *)(uintptr_t)(uint32_t)(uintptr_t)blk1;
```

reads **22 of 22 on the default heap and 18 of 22 with the high arena**. The
ordinary run is blind to the class the whole round is about; only the second
one is evidence.

`bmf_page_alloc` came with it: `(raw + BMF_PAGE - 1) & ~(BMF_PAGE - 1)` with
`BMF_PAGE` an `unsigned int` is a mask of 0x00000000FFFFF000, which takes the
top half of the address off with the low twelve bits. It had always worked
because the arena kept every allocation low. `BMF_PAGE` is a `uintptr_t` now.

---

## 4. The rules this round added

| tool | asks | target |
|---|---|---|
| `tools/ptrwidth.py` | which pointers still go through a 32-bit integer, and which of the six kinds each is | 0 |
| `tools/rawoffset.py` | which numbers in the code are a member offset the file itself states | 0 |
| `tools/negindex.py` | which negative indices are negated in an unsigned type | 0 |
| `tools/x64.sh` | whether the other pointer width gives the same answers | 22 of 22 |
| `tools/x64diff.sh` | whether the two agree on inputs nobody chose | 400 mutants, no disagreement |
| `tools/x64.sh --high` | whether any 32-bit pointer field survives, by putting every allocation where one cannot reach | 22 of 22 |

`BMF_WARN=1 BMF_BITS=64 ./build.sh` writes `warn64.log`, and `warn64.txt` is
its ceiling — a ratchet like `warn.txt` and over a different population, since
a `ptrdiff_t` narrowed to an `int32_t` count is a conversion there and an
identity here.

The three Python rules are in `tools/sweep.sh`, which holds a counting tool to
zero, and `tools/proven.sh` says all three answer differently against older
revisions of the file — the check that a rule reporting zero is reporting about
the file rather than about itself. Each was replayed against the file from *before* its fix and reports what
it is supposed to find — the standing rule being that a rule reporting zero on
the current file has proved nothing. `--parked`, added last, reports 2, 2, 1, 0
across the four revisions it was replayed at and 0 here. The three shell
instruments have the same property and it is stated where each of them lives:
`x64.sh` moves from 22 of 22 to 18 of 22 on one reintroduced truncation,
`x64diff.sh` names the mutant and the byte counts, and `--high` is the
difference between those two runs.

Two of `ptrwidth.py`'s own flags could not be replayed at all until this round:
`--laundered` and `--parked` read the file and nothing else, but they sat
*after* the check that `strict64.log` describes the file being read, so against
any old revision they printed that note instead of an answer. A rule that
cannot be asked about an old file is a rule that cannot be proved, which is the
same failure `proven.sh` exists to catch — and `proven.sh` could not catch this
one, because the tool's *last line* still moved for other reasons.

---

## 5. What this round did not do, and what is left

* **`warn64.txt` is a ceiling and not a target.** 945 conversions against 927
  on i386, and they are not the same population: a `ptrdiff_t` narrowed to an
  `int32_t` count is a conversion on one target and an identity on the other.
  47 of the 64-bit warnings come from a 64-bit-wide source against 23 on i386,
  and the two sets overlap almost entirely — the 23 are `int64_t` and they are
  the *same* 23 on both targets, spelled `long long int` there and `long int`
  here. What exists only on x86-64 is the other 24, at 23 distinct
  expressions, and they are three shapes and no more:

  ```
  12  bits_* = 8 * (out_cursor - coded_buf);   one per cost probe
   5  coded_bytes = out_at - buf, slot = cur - base, idx, n_quads, uu
   3  fread twice, fwrite once                 a size_t return
   1  offsetof(SymListBlock, list) + sizeof(SymList) * n
  ```

  Every one is a length or a count bounded by `data_size`, which is a
  `uint32_t` the header carries, and every one has the conversion at the
  assignment where a reader can see it. Lowering the ceiling means reading the
  other 898, which is round five's job done again on a second target, and a
  round of its own.

  The numbers in this bullet were 952/933 and "47 … the eight distinct sites",
  and only one of those survived a re-measurement: the ceilings had moved,
  and "eight sites" was never right — it counted the lines quoted rather than
  the sites found. The 47 came back to 47 by coincidence, from 52, when §2.3b
  removed five.
* **`-march`.** Both targets build at `k8`; whether the p2 filter's float
  arithmetic still agrees at a wider baseline is not asked here.
* **Anything that is not 4 or 8 bytes of pointer, or is big-endian.** The
  `sizeof(void *) != 4 ||` guard on every layout assertion is the honest
  statement: i386 is pinned, x86-64 is checked by running it, and nothing else
  is claimed.
