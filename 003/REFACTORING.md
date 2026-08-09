# Refactoring BMF 2.01

A plan for turning the decompilation into source you would be willing to
maintain.

It began as a plan to do that *and* build for x86-64. Those turned out to pull
against each other — §Phase 4 — and the target is 32-bit.

`ALGORITHM.md` says what the program *does*. This says what to do to the code,
in what order, and how to know each step did not break it. Every number below
was re-measured on the tree at the commit that last touched this file; the
commands that produced them are in [Appendix A](#appendix-a--how-the-numbers-were-measured)
so they can be re-measured rather than trusted.

---

## 1. Where this is now

| | | at the start |
| --- | --- | --- |
| `subs1.hpp` | 23 861 lines | 25 462 |
| bodies | 179 (84 real, 95 `__fwd_*` shims) | 215 |
| globals in `blob.inc` | **78** | 293 |
| recovered structs | **73**, 2728 named field accesses | 0 |
| raw-offset dereferences | **523** | 1646 before Phase 4 |
| pointer casts | 5804 | 7336 |
| `goto` / `LABEL_n:` | 113 / 81 | 174 / 127 |
| `__hexrays_frame` | **0** | 24 buffers, 935 aliases |
| line coverage | **95.91 %** | 64.5 % |

The target is 32-bit. That is not a limitation left over from the port — it is
the decision that made Phase 4 possible, and §Phase 4 says why.

The previous rounds of work (see `tools/README.md`) fixed the vocabulary —
stdint types, one declaration per global, no WinAPI, no Intel intrinsic
dispatcher, no raw addresses, a real `RangeCoder` class. What is left is the
part that needs *understanding* rather than substitution.

---

## 2. The gate

Everything below rests on one property: **a refactoring step is correct if the
compressed stream is byte-identical to the one the previous build produced, and
every image still round-trips.** No test can tell you a rename was
*meaning-preserving*, but this tells you it was *behaviour*-preserving, which is
the property that matters.

```
./build.sh && ./test.sh ./bmf     # 15 images, encode + decode + compare,
                                  # each stream against testfiles/ref_<name>.bmf
```

Both halves are automated: the round-trip, and the comparison against the
fifteen committed reference streams. Three checks follow the images, and each
exists because the per-image loop is blind to something a refactoring can
delete: one builds an archive with two members and reads both back, one hands
the program fifteen inputs it should refuse and pins the exit status of every
one, and one starves it of memory. Every one of the three was added after a
defect it would have caught (§2.3, §6). A run with a missing reference fails, and a stream
that differs from its reference fails. Where an input is not reproduced byte for
byte — an RLE-compressed BMP comes back with BMF's own run splitting —
`testfiles/out_<name>.bmp` holds what the decoder is expected to write and the
round-trip is compared against that. `tools/mkrefs.sh` regenerates them, and
is to be run only when a change is *meant* to move the output.

### 2.1 One mode, and only the code that implements it

BMF read six switches from its .ini and its command line. This program has one
mode: `bmf c` pinned `-S` and `-Q9` at run time and let the other four keep the
values BMF.exe's data segment starts them at. That left about a third of the
file compiled, reachable from `main`, and never executed — and code no test runs
cannot be refactored.

They are constants now, at the top of `subs1.hpp`:

```c
static constexpr int32_t __dword_44108C = 1;   // -F  use filters
static constexpr int32_t __dword_441090 = 1;   // -S  slow but efficient
static constexpr int32_t __n2_4         = 0;   // -T  filter template
static constexpr int32_t __dword_441098 = 1;   // -N  pack the output
static constexpr int32_t __n7_0         = 9;   // -Q  filter search quality
static constexpr int32_t __n7_1         = 0;   // -E  max error, near-lossless
```

Every branch that tested them is folded to the side that runs, and what that
leaves unreachable is deleted: the `-T1` template builder and the `-T2` reader,
the `-Q<9` tile cap, the fast-mode encoder back end (`sub_408510` and the eight
bodies it reaches), the fast-mode decoder back end (`sub_40CF80`), and the
near-lossless encoder and expander. 38 bodies, and the globals that went with
them.

Two of those bits arrive from the stream rather than from the constants — bit 2
of the descriptor is `-S`, and the 4-bit field beside it is `-E` — so folding
them was a decision, not a substitution. A stream that asks for fast mode or for
`E>0` is one this build no longer has the code to expand, and is **refused with
a message** at the point the field is read rather than decoded wrongly.

| | before | after |
| --- | --- | --- |
| `subs1.hpp` | 25 462 lines | 17 968 |
| bodies | 215 | 178 |
| globals | 293 | 170 |
| line coverage | 64.5 % | **89.9 %** |

That is the coverage gap closed by deletion rather than by testing more modes,
and it is the right way round: a mode this program does not offer is not a thing
to keep working, it is a thing to remove.

### 2.2 The linker as the dead-code check

`build.sh` compiles `-ffunction-sections -fdata-sections` and links
`--gc-sections`, so a function nothing reaches never reaches the binary whether
or not it has been deleted from the source. `BMF_GC=list ./build.sh` prints what
was discarded, which is a better dead-code report than a textual call graph: it
sees through the branches the constants fold away.

It is a report, not an authority. It currently discards one body whose source is
deliberately kept — `expand_predictor_mode0`, the predictor-mode-0 expander, whose call
sites the optimiser proves unreachable at `-O1` and above but not at `-O0`. That
reasoning may be exploiting undefined behaviour rather than a real
impossibility, and `ALGORITHM.md` §9 lists predictor mode 0 as a mode whose
meaning is not established. Deleting a body needs a better reason than "`-O1`
thinks so".

### 2.3 What the gate still does not reach

4.1 % of the file — 546 unexecuted lines across the bodies:

| body | unexecuted |
| --- | --- |
| `unmodel_plane_slow` | 78 |
| `compress_image` | 65 |
| `expand_image` | 60 |
| `write_bmp` | 47 |
| `read_bmp` | 46 |
| `interleave_plane` | 40 |

Measured with the archive check included, which earlier runs of this had left
out — the per-image loop does not walk a multi-member archive, so
`compress_image`'s `feof` loop looked unreachable when it is exercised on every
run of the gate. **A coverage number is only as good as the harness it is
measured with**, which is the same lesson as the rest of §6 wearing different
clothes.

`sub_4118A0` headed this table with 103 and is off it: 111 of those lines were
a predictor-mode-0 branch **no dispatch can reach**. The call graph is closed —
that function is called only by `sub_4229E0` and `sub_4256F0`, those only by
the eight alternate model bodies, and `model_plane` and `unmodel_plane` reach
those only under `plane_predictor == 1` or `== 2`. A run over the corpus agrees:
164 entries, 83 at 1 and 81 at 2, none at 0. Deleted on §2.1's grounds.

Reaching that conclusion needed both halves. The measurement alone would only
have said "the corpus does not go there", which is what was said about
`alt_model_p1_decode` before an image reached it and found a bug. The call
graph is what turns that into *cannot*.

**The same shape appears in `unpredict_med`,** and it was written down as *not*
deletable before it was: one of its two call sites is guarded by
`plane_predictor == 1` and the other by a different variable, so the call graph
did not close, and the measurement behind it was 4 entries against
`sub_4118A0`'s 164. Two weak halves are not one strong one.

Then the question that was written down got asked. `n2_2` is
`*(uint32_t *)p_i` read back from `*(uint32_t *)p_i = n2_1`, and `n2_1` is
`__byte_44339E[16 * v37] & 3` — the predictor itself, stored and reloaded. Both
guards are the same guard. The test is always true, so it and its 45-line else
are gone.

**What is left uncovered is diffuse**, and that is the useful summary. The
unexecuted lines now form runs of at most 9, and mostly of 1 to 3.

More images of the ordinary kind will not move it. Four more format variants
were generated and measured — 24-bit with identical channels, an 8-bit
palette, a 4-bit and a 1-bit — and the marginal gain was **zero lines**. The
corpus already spans the format space; what is left is inside the paths those
formats share.

By kind, the 573 lines were 417 of assignment and computation, 107 of control
flow, 35 of allocation and error exit, and 14 of file-I/O checking. That last
50 needed a failing `malloc` or a truncated file, not another picture, and the
417 are deep inside branches of `unmodel_plane_slow`, `write_bmp` and
`interleave_plane` that the corpus reaches the *other* side of.

**The truncated file was worth having.** The gate now hands the program fifteen
inputs it should refuse — an empty archive, five truncations of a real stream,
two files of constant bytes, a missing name in either mode, a BMP whose pixels
run out, each file given to the mode that wants the other, and two malformed
command lines — and pins the exit status of every one. 576 → 551 unexecuted
lines, 95.69 % → 95.87 %, and the 25 are exactly where they should be:

| body | what the refusals reached |
| --- | --- |
| `exit_402E40` | 6 — **all of it**. The program's error reporter had never run. |
| `expand_image` | 9 — the `return nullptr` exits and the frees that unwind to them |
| `bmf_open_archive` | 3 — the two `fclose`-and-fail paths |
| `read_bmp`, `bmf_compress`, `bmf_decompress`, `main` | 7 — the `Can't open file:`, `Read error!` and usage exits |

A test suite that only ever hands a program valid input has not tested the
program, it has tested one half of it, and the half it skipped is the half that
runs when something is wrong. Six of these lines are a `printf` of a message
table; the value is not the six lines but that the table is now known to be
indexed correctly, which nothing had ever checked.

#### What the refusals found next: `operator new` is not `malloc`

`out_of_memory_handler` was the one body in the file with no coverage, and the
note against it said it "runs when `malloc` fails", which was a guess from its
name. Asking whether that was true took one command:

```
(ulimit -v 3000; ./bmf)                      # rc=1, usage — the loader is fine at this limit
(ulimit -v 8000; ./bmf c t8g.bmp o.bmf)      # rc=139, SIGSEGV, no message
(ulimit -v 15000; ./bmf c t8g.bmp o.bmf)     # rc=0
```

It could not run. `main` calls `set_new_handler(out_of_memory_handler)`, that
body stores the pointer into the global at 0x445930 — MSVC's CRT new-handler
slot — and nothing in the build read it, so the store was dead and a run that
could not get memory died of a null dereference instead of saying so.

**The disconnection was ours.** Every allocation came out of IDA as `__op_new`,
and the imported tree defined it in `bmf.cpp` as `malloc(n ? n : 1)`. A later
commit, *Call the C library directly*, inlined 65 of those to a bare `malloc`
on the grounds that a wrapper whose entire content is a call to the function of
the same name has no reason to exist. True of `fread`. False of this one:
`operator new` has a contract `malloc` does not, and the program is built on it.
MSVC's calls the installed handler on failure; BMF installs one; that is the
whole mechanism, and rewriting the call removed it.

`bmf_new` in `bmf.cpp` is that contract and nothing else — try, and on failure
call whatever handler the program stored, then try again — and the 46 sites go
through it. Reading the handler out of `__pout_of_memory_handler` is the point:
it puts `set_new_handler` back on a live path rather than routing around it.
Every stream is byte-identical, because nothing about a successful allocation
changed, and the failing run now says what happened:

```
File          t8g.bmp, image 320x240x8, size - 76800:Out of memory!   # rc=7
```

This is the third defect of its exact shape — a wrapper collapsed into the
library function it resembled, a mode changed from `"a+b"` to `"w+b"`, a frame
split into locals — and all three were invisible to a gate built from valid
input and successful runs. The pattern is not carelessness in any of the three
commits; each was locally reasonable. It is that **a refactoring gate has to
exercise the paths a refactoring can silently delete**, and failure paths are
first among them precisely because nothing routine goes there.

`test.sh` pins it with a ladder of `ulimit -v` values, asking two things: no
limit may kill the program with a signal, and at least one must produce the
diagnostic. Against the tree before the fix it reports
`oom: -v 8000 exited 139, not 7`.

Three checks were run to say that lever was spent — no body with zero coverage,
no folded-mode marker with a live `else`, no `!plane_predictor` guard left in
the subsystem — and **one of them was too narrow.** It searched for
`!plane_predictor` and `plane_predictor == 0`, and missed
`if (plane_predictor) … else`, which is the same test written the other way
round. `alt_init_tables` had one, with a 22-line else, and a `LABEL_47` that
lost its last `goto` when the earlier block went.

Re-run over the whole subsystem in both forms: that was the only one.

A fourth turned up right beside it, and by a different route. `near_lossless_max[0]`
is assigned 0 at four places and nowhere else, so it is pinned exactly as the
six mode switches are — and next to a test on it stood `if ( 1 ) goto LABEL_52`,
an always-true jump the folding pass left. Everything between the jump and its
label was unreachable, including the `n128_6 < 128` test the only live path
never evaluated. 13 lines, one label, one `goto`.

**A fifth came from the coverage number, not from the checks.** After the
allocation fix, one entry was left in the file with no coverage at all:
`__fwd_model_planes_interleave_plane`, a shim whose body runs perfectly well
from `expand_image`'s two call sites. What had never run was the *call*, and its
guard is `if ( Srca_2 != Srca_1 )` — where both locals are assigned `Srca_3`,
once, and neither is reassigned or has its address taken. The test is false on
every path.

It was not always. `Srca_2` got its own buffer inside the `-E` block, which
*One mode, and only the code that implements it* deleted; the tombstone comment
`// never taken: -E is 0` still stands three lines above the test that asked
whether that block had run. **Deleting a block does not delete the test that
asked about it,** and this one outlived the block by a fortnight, taking an
interleave, a `free` and a shim with it.

That is the last of the arithmetic in this section: 551 unexecuted lines after
the refusals, less the 4 that went with this block, less the 1 that
`out_of_memory_handler` stopped being, is **546 of 13 346 — 95.91 %**, with no
body in the file at 0.00 %.

The lever is spent now, and `tools/deadcheck.py` is why that is a statement
rather than an impression. It looks for all five shapes — labels with no
`goto`, constant tests as statements, tests on a pinned global in either
spelling, bodies nothing calls, and two locals compared that hold the same
thing — and it is verified the only way that means anything: against the tree
as it stood before each was removed it reports it; against the tree now, none.

Three of the five were originally found by checks written *after* the case they
would have caught. That is the wrong order, and it is the reason this one
exists as a file rather than as five greps in a commit message. The fifth check
is the first that has to follow a value rather than match a shape, and it is
kept deliberately narrow — one assignment, from a plain name, address never
taken — because a dead-code report is worth having only while every line in it
is dead.

These are format and descriptor combinations fifteen images still do not reach.
`alt_model_p1_decode` used to head this table with 268 lines and is not on it
at all now; what happened when it finally ran is §6's first entry.

**Every function in the file now executes under the gate.** That is the number
that matters after §6, more than the percentage: the category that hid the bug
was not "partly covered", it was *"never run at all"*, and it is now empty with
no exceptions. It held two entries for a fortnight and neither survived being
looked at: `out_of_memory_handler` could not be called, which is the defect
above, and `__fwd_model_planes_interleave_plane` was reached only from code that
could not run, which is the one below.

What is left is branches inside bodies that do run — a different and much weaker
kind of blind spot, because the frame, the types and the struct layouts around
them are all exercised by the paths that do.

Five of those fifteen are recent, and each was written to reach something
specific rather than to add another photograph:

| image | what it reaches | coverage |
| --- | --- | --- |
| `med32.bmp` | `unpredict_med`, which had 127 unexecuted lines and no way in | 90.04 % → 92.05 % |
| `rle8.bmp` | the run-length reader's encoded and absolute runs | |
| `rle4.bmp` | its nibble path and the delta escape | 92.05 % → 92.45 % |
| `noise24.bmp` | the store-it-raw fallback for data that does not compress | 92.45 % → 92.65 % |
| `altp1.bmp` | `alt_model_p1_decode`, 268 lines, **and a bug in them** | 92.65 % → 94.71 % |

`read_bmp` went from 101 unexecuted lines to 48 and off this table. The
generators are `tools/mkmed32.py`, `tools/mkrle.py` and `tools/mknoise.py`, so
the images are reproducible rather than opaque; what each one had to do to
reach its target is in the generator's docstring.

`unpredict_med` looked impossible — the corpus round-trips, so something must
invert `predict_med`, and not one of its 127 lines ran. Chasing it down turned
out to be the most useful thing this measurement produced. The dispatch counts,
on the ten images it started with:

| | encode (95 entries to `model_plane`) | decode (15 entries to `unmodel_plane`) |
| --- | --- | --- |
| alternate model families | 62 | **8** |
| the path §7.2 of `ALGORITHM.md` describes | 33 | **7** |

The encode side runs more often because `search_filter` encodes candidates it
then throws away; the decode side counts the planes actually in the streams.

So `unpredict_med` is not reached because **every plane whose predictor is MED
also has `plane_alt_model` set**, and the alternate family inverts the
prediction itself. And `alt_model_p1_decode` is not reached because nothing in
the corpus is finally coded as predictor 1 at a depth other than 8 — the other
three families all run.

That reverses an impression both documents gave. The alternate model families
are not a side path: they carry the majority of the planes, and they are the
part `ALGORITHM.md` §9 says is unread.

It also says exactly what a new test image has to do to reach `unpredict_med`:
keep the plane out of the 8-bit split, so the descriptor's depth stays 32, and
make the filter search pick MED without the alternate model. `tools/mkmed32.py`
writes one — four identical channels carrying `x*x/256 + y` — and 79 of those
127 lines run now. What it took to find it is worth recording: 25 synthetic
images across 4-, 24- and 32-bit, identical and independent channels, smooth,
noisy and structured. **None of them produced predictor 1 with the alternate
model at a depth other than 8**, which is the combination `alt_model_p1_decode`
needs. That was a measurement, not a proof of impossibility, and it is worth reading
what happened next as a caution about the difference. **The very next thing
measured contradicted it.** The dispatch counts showed `alt_model_p1_encode`
running twice during the corpus run — the encoder does construct that
combination while searching, it just never wins — so the combination was
reachable all along and the 35 images had simply missed a region of the input
space: three identical channels carrying one function and a *fourth* carrying a
different one. Seven of the next 31 generated images hit it.

The first one decoded to a segfault. §6.

Neither is dead — nothing has shown any entry condition to be impossible — so
these are refactored last and with more care than the rest, or their entry
conditions are established first and the corpus grown to reach them.

Fifteen images is a signal, not a proof. Add images when a phase touches
something the corpus does not exercise — `med32.bmp`, the two RLE images and
`altp1.bmp` are what that looks like when it is done deliberately — and the
last of them found a bug.

## 3. What "done" looks like

1. No global is a reference into a byte array. Each is a definition with its own
   type, its own extent, and an initialiser where the original had one.
   `blob.inc` is deleted.
2. ~~No integer holds a pointer.~~ **Withdrawn, on evidence — see Phase 4.**
   Every one of this program's 159 objects is walked with variable offsets as
   well as constant ones, so widening a field moves the arrays indexed after it.
   What replaces it: *no integer holds a pointer where the object permits it*
   (done — every local, parameter and global that could be typed, is), and a
   64-bit build gets its addresses into 32 bits by confining the heap rather
   than by widening the fields.
3. The recurring `base + constant` families are `struct`s, and the variables
   that walk them are typed pointers to those structs. **Done for 73 objects**
   — 2728 accesses name a field, 523 raw-offset dereferences remain, and the
   ones declined are listed with their reasons in Phase 4.
4. Names say what things are. No `__sub_41CAB0`, no `v187`, no `n0x800000`.
   The recovered structs are the open part of this: their layouts are known,
   their meanings mostly are not, and `ModelBlock` is the one that is.
5. `casts`, `LOBYTE`-family macros, `goto`, and `__hexrays_frame` are gone or
   reduced to the handful of places where they are genuinely the clearest
   expression.

Item 2 is the only one with a mechanical pass/fail; the rest are judgement, and
the gate protects them rather than defining them.

---

## 4. The two things that make this harder than it looks

### 4.1 The "globals" are not all globals

36 of the 169 remaining declarations **overlap the one after them**. Some are IDA
guessing an array bound (`__dword_438AFC[0x10000]` running over
`__dword_438B00`), which is harmless. Others are not:

```c
*(uint64_t *)((char *)&__n4_5          + n16 * 4) = ...;   // 0x443394
*(uint64_t *)((char *)__n256_2         + n16 * 4) = ...;   // 0x44338C
*(uint64_t *)((char *)&__dword_443384  + n16 * 4) = ...;   // 0x443384
*(uint64_t *)((char *)&__Buffer        + n16 * 4) = ...;   // 0x44337C
```

Four separate "globals", eight bytes apart, each written eight bytes at a time
at the same stride — with `n16` stepping 16, 8 they cover `0x44339C`–`0x4433DC`
contiguously, which is the 16-byte per-plane descriptor table `ALGORITHM.md` §8
places at `0x44339C`.

All four *are* real objects: `__Buffer` is the coded buffer's base pointer
(`__Buffer = malloc(...)` a few hundred lines away), `__n4_5` is the plane
count, and so on. That is what makes this nastier than a mislabelled variable.
The compiler strength-reduced a walk over the descriptor table into four base
pointers, and picked as those bases the addresses of four unrelated variables
that happen to sit 32 and 64 bytes below it. The code therefore depends on
**where these globals are relative to each other and to a table none of them
names**.

Give them independent definitions and the linker will place them wherever it
likes; the writes still happen, 32 and 64 bytes past each, into whatever is
there. It compiles, it runs, and it corrupts. **Extent and aliasing analysis has
to come before de-blobbing, not after** — and for regions like this one the
answer is a single `struct` covering `0x44337C`–`0x4433DC`, not four variables
and a table.

### 4.2 Pointer typing and structure recovery are one job

The decompiler writes the same expression twice over:

```c
*(uint32_t *)(_this + 278736) = v17 + 144;
```

Two things are unnamed here: `_this` is an `int32_t` holding a pointer, and
`278736` is a field offset. Neither can be fixed alone — retyping `_this` to
`char *` leaves `+ 278736`, and naming the field requires a struct that `_this`
must already point to. So they are one pass, done **per object**, converting
every function that touches that object at once.

This was originally written as the x86-64 problem — 3784 `cast to pointer from
integer of different size` warnings across 3233 lines, and later 1211 across
1708. That framing was the mistake §7 records: it made naming a field look like
it required widening it. It does not, on a 32-bit target, and the warning count
is no longer a measure of anything this document is trying to do.

The objects, by traffic. The right-hand column is what is *known*, not what is
guessed — `ALGORITHM.md` §9 lists the alternate model families as not worked out,
and this table does not pretend otherwise:

| pointer | offset range | distinct | refs | touched by | what is known |
| --- | --- | --- | --- | --- | --- |
| `_this` | `0x64`–`0x5C75AC` | 72 | 200 | `pixel_context`, `init_model_tables`, `sub_419430`, `sub_4229E0`, … | the model block: a `0x44000`–`0x44144` header (52 fields, 405 refs across all pointers) then tables out to 6.7 MB. Allocated by `rc_begin_encode` / `rc_begin_decode` |
| `lpAddress` | `0x11021`–`0x447B0` | 33 | 104 | `sub_419610`, `sub_422DB0` | an alternate-model working set — **role not established** |
| `n5_2` | `0x5D8`–`0xEEA` | 7 | 97 | `alt_model_p1_encode`, `sub_4259F0` | a ~3.8 KB record in the alternate model family — **role not established** |
| `a1` | `0xC20`–`0x65E7B0` | 32 | 96 | `layout_workspace`, `sub_41CAB0` | reaches the same 6.7 MB extent as `_this`, so almost certainly the model block under another name — confirm before merging |
| `v56` | `0x44000`–`0x440F4` | 21 | 51 | `alt_model_p2_decode` | the model block's header only |
| `n0x10_2` | `0x3960C`–`0xE5836` | 14 | 46 | `sub_41CAB0` | inside the model block's tables |
| — | `0x44339C`, 16 B stride | — | ~90 | many | the per-plane descriptors, reached through the four false bases in §4.1 |

Three things this table says that matter for sequencing.

**The model block is reached under at least four different pointer names**
(`_this`, `a1`, `v56`, `n0x10_2`), just under 400 references across a dozen
functions that never see each other's signatures. Its struct has to be declared
once and applied everywhere at once, not discovered per-function. What actually
happened is the safe half of that: the alias analysis connected the names it
could see connected, and the model block came out as several structs of the same
size rather than one. Six of them end at offset 278 772. Merging those is a reading
job, not a tooling one — see §Phase 4.

**The two families whose role is unknown** (`lpAddress`, `n5_2`) sit inside the
alternate model code `ALGORITHM.md` §9 flags as unread. Their *layout* recovered
anyway, because the layout is visible in the offsets and does not need the role;
what is still blocked on reading §9 is naming the fields.

**The gate covers this work.** The two functions §2.3 lists as still unreached
are `alt_model_p1_decode` and `unpredict_med`; neither is a model-block user. Phase 4's
largest object is entirely inside covered code. It is Phase 3, which touches
every global whether covered or not, that carries what exposure remains.

---

## 5. Order of work — status

Each phase ends green: build, every image round-trips, every stream
byte-identical to its reference. That held for every commit; the gate has never
been red at a commit boundary.

```
0  the gate, and one mode      done   §2
1  names you already know      done   39 identifiers
2  frames -> real locals       done   0 frames left
3  un-pin the globals          done   86 of 163 out; the other 77 cannot move
4  objects + pointers          done   73 structs; 2728 accesses name a field
5  casts and the vocabulary    done   2 of 3 items withdrawn on evidence
6  control flow                done   2 of 123; the rest is irreducible
```

Two things happened after all seven were closed, and neither is a phase:

**Naming carried on.** 42 more functions, each from evidence rather than from
shape — what `ALGORITHM.md` establishes, which `rc` method a body calls, where
it sits in a dispatch, or what its position in the call graph makes it. 71
named bodies now, and `tools/addrmap.txt` maps every one of them back to the
address in `BMF.exe` it was decompiled from, so the names cost no traceability.

**Five keep their addresses.** Two are pure SSE with no coder calls and no
distinguishing callers; three are helpers inside the alternate model families
whose role is not separable from the family's. A name for any of them would be
a guess, and §6 has the entry about what those cost.

**The corpus grew on purpose.** Four images written to reach specific code
rather than to add another photograph, plus an archive check; 89.97 % → 92.65 %,
and it turned up a capability an earlier commit had silently removed (§6). The
method is in §2.3: establish the entry condition, then write the input.

"Done" here means finished, not finished-as-written. Phases 3, 4, 5 and 6 each
met evidence that contradicted the plan, and each entry below says what the
evidence was and what was done instead. §7 collects the four corrections.

### Phase 0 — done

§2. Reference streams committed and the comparison made fatal; the mode made
constant and everything the other modes reached deleted; `--gc-sections` so what
is dead cannot ship. 25 462 → 17 968 lines, coverage 64.5 → 89.9 %. The gate has
grown twice since: `out_<name>.bmp` for inputs the program is not expected to
echo, and a two-member archive check.

### Phase 1 — done

39 identifiers from `ALGORITHM.md` §8 and §10, 1042 occurrences, nothing
speculative. `tools/rename.py` is scope-aware and refuses rather than guesses.

### Phase 2 — done

All 24 frames gone. **The plan said "expect 3–6 to resist" and 17 did.** Those
frames have bytes no alias names — `choose_plane_coding` names 29 220 of 41 456 — and the
code reaches that slack by running off the end of the alias next door. They keep
their layout as a struct with explicit padding and lose only the casts, each
carrying `static_assert(sizeof(__frame) == N)` so a layout that moves is a
compile error rather than something the corpus has to notice.

**16, then — all 24 now.** The seventeenth was `alt_model_p1_decode`, which took
the plain split because the gate said it was fine, and the gate said so because
nothing in the corpus ran that function. It was broken from this commit until
the day an image reached it (§6). Six more have since been converted for the
same reason without waiting for a symptom: they have the same unnamed slack and
only the gate's silence said they were safe. None is split now.

The number to take from this phase is not 16 or 17, it is that *the gate's
verdict on a body it never executes is not a verdict*.

### Phase 3 — 86 globals out, 78 left, and a map of why

Done: the four false bases from §4.1 retired, so no global's position matters to
another *there*; five globals that held addresses in `int32_t` words are real
pointers outside the blob (`coded_buf`, `out_cursor`, `packer_word`,
`hist_scratch`, `model_tables`); the error message table decoded and inlined,
which is what `__exit_402E40` used nine relocated pointers for.

**The all-at-once split does not work, and that is now measured rather than
assumed.** `tools/deblob.py` gives every global its own definition with a guard
gap; it segfaults on the first image. Three variants say what kind of failure:

| guard | contents | result |
| --- | --- | --- |
| 64 bytes | zero | SIGSEGV in `pixel_context` |
| 64 bytes | the bytes that followed in the data segment | SIGSEGV, same place |
| 4096 bytes | the bytes that followed in the data segment | SIGSEGV, same place |

A read running off a global's end would be fixed by the second and certainly by
the third. Neither is, which leaves **writes** crossing the boundaries: some of
what Hex-Rays presents as globals is a smaller number of larger tables split
into one name per access site, and a store through one name has to be visible
through another.

So the tool was run one global at a time instead — split it, build, run the ten
images, keep the answer — and the result is a map rather than a guess:

**86 of the 163 are independent and are now definitions of their own.** 77 are
not. `tools/blob-independence.txt` is the per-global answer, and it is the
worklist for the objects Phase 4 has to recover: a global marked SHARED is one
that something writes across.

The 39 absolute pointers `blob1_relocs` holds all turned out to live in globals
that moved, so `blob.inc`'s own rebasing no longer reaches them; the generated
block rebases them itself, resolving a target to a moved definition where there
is one and back into `blob1` where there is not. They pass through unread on
this corpus — the string tables belong to modes that are gone — but leaving raw
addresses in data nothing relocates is the kind of thing that works until it
does not.

`blob.inc` is down to 78 globals from 293 at the start. It cannot go to zero
until the 77 shared ones are understood as the tables they are.

### Phase 4 — 73 objects have structs; the blocker was the 64-bit goal, not the code

`tools/retype.py` converted every local and parameter used as a pointer base
first: 189 candidates, `char *` where the variable is only ever an address and
`uintptr_t` where the code also masks or tags it. That left one shape, the one
§4.2 warned about — the local is typed, the **field** is not:

```c
*(uint8_t *)(*(uint32_t *)(_this + 76) + 6)
```

`tools/structs.py` closes it. The alias analysis groups the names that denote
one allocation, the constant offsets become members, and every access is
rewritten to use them:

```c
*(uint32_t *)(_this + 76)   ->   _this->f76
this_3[19]                  ->   this_3->f76
*(uint16_t *)(v65 + v59 + 3800)
                            ->   *(uint16_t *)((char *)v59 + (intptr_t)(v65 + 3800))
```

**What made this possible was dropping the 64-bit target, not any new
understanding of the program.** At 32 bits a pointer is four bytes, which is
exactly the width the decompiled code assumes every field to be. So a struct
laid out from the observed offsets has the layout the code already had: nothing
moves, the variable-offset walks keep indexing what they indexed, and each
generated struct carries a `static_assert` on its size that says so and fails
loudly if it ever stops being true.

73 structs, gated one at a time — build, encode and decode every image, compare
every stream against its committed reference, revert the ones that change
anything. **523 raw-offset dereferences left, from 1646, and 2728 accesses now
name a field.** 30 objects are on the skip list; the reasons are
in `tools/struct-skip.txt` and the categories are below.

#### What is left, and why

| why an object was not converted | what it means |
| --- | --- |
| the class is not one object | the biggest refusal. `v109 v12 v130 v157 v172` are five cursors *into* one table, each pointing at a different element — assignments make them aliases, but an offset from one is not an offset from another. A struct would be wrong, not merely awkward, and the compiler said so: `8 * v12` is a multiplication, so `v12` was never an object pointer at all |
| the name is stepped as a pointer | `p += k` cannot be rewritten through a cast, because a cast is not an lvalue |
| the declaration cannot be found | the name has no declaration the tool recognises, and a name left at its old type takes `nm->f8` with it into a file that does not build |
| the gate rejected it | the rewrite compiled and changed a stream. Recorded, reverted, and not retried |

The same allocation can end up with more than one struct: six structs are all 278 772 bytes
to their last member, which is one object seen in six functions the alias
analysis could not connect to each other. That
is under-merging, and it is the safe direction — each struct describes offsets
actually observed under that name.

#### What a 64-bit build would still need

Not wider fields. Widening a field moves every field after it, which is
survivable only if an object is addressed at constant offsets alone; across all
objects there are **1683 constant-offset dereferences and 2184 variable-offset
ones, and not one object has only the first kind.** The structs recovered here
are 32-bit-shaped descriptions, not a step towards 64 bits.

What 64 bits needs is a heap the narrow fields can address. `bmf.cpp` has that —
a low arena under `malloc`, compiled for 64-bit targets only, so the 32-bit
build the streams are verified against allocates exactly as it did. With it and
`BmfArc` (the eight bytes `bmf c` and `bmf d` allocate for the open stream and
an image count, the one object with no variable-offset access), the 64-bit
binary compiles, runs, opens its files, and reaches `model_plane` before it
meets the model block. Getting past that is one fault at a time, and it will not
produce typed fields at the end.

### Phase 5 — done, and two thirds of it should not have been attempted

The 38 casts GCC calls useless went with the retyping. The 8-byte repeated-byte
stores into the model geometry table were runs of consecutive `uint64_t` writes
of one value; 12 of them are 4 `memset`s now, which says what they are.

The other two items are deliberately not done. Both were re-checked after Phase
4 recovered its structs, since the first one's reason depended on Phase 4
having recovered none:

* **`LOBYTE`/`HIWORD` → shift and mask.** The plan proposed this for the uses
  that turn out to be field access into a struct Phase 4 recovered. Phase 4 now
  recovers 44 structs, and **none of the 243 remaining uses applies to a field
  of one** — measured, not assumed. Narrow access at a member's offset never
  reached a macro in the first place: `structs.py` writes it as
  `*(uint8_t *)&p->f8` directly. What is left is all genuine bit-packing on
  locals, where `LOBYTE(x) = v` says more than `x = (x & ~0xFFu) | (v & 0xFF)`.
* **The `M128I`/`M128F` unions → intrinsics.** Those unions *are* how MSVC's
  member syntax is spelled portably; GCC's `__m128` has no named members. There
  is no lvalue intrinsic for `x.m128_f32[2] = v`. The plan conditioned this on
  the SIMD code being readable first, and it is not.

### Phase 6 — done to the extent the code allows: 2 of 123

`tools/degoto.py` rewrites the one shape with a structured equivalent — a
forward `goto` that is the whole of an `if`, over a region nothing else enters,
becomes the inverted `if` with the region inside it. It checks the conditions
rather than assuming them: the label reached by no other `goto`, no label inside
the region reached from outside, the region brace-balancing so it can become a
block.

Two qualify. The other 121 are 21 backward jumps — loops Hex-Rays could not name
— and 100 forward ones that are either not the whole of an `if` or jump over a
region with its own entries. The earlier measurement stands: **no label in this
file is followed by a `return`, a `break` or a `continue`**, none is unreachable,
and none has its `goto` on the line above. There is no fifth shape waiting to be
found; this is irreducible flow, and rewriting it blind would introduce bugs the
gate cannot tell from a layout change.

## 6. Things that will bite

- **The code reads memory it does not own.** It indexes past array bounds,
  aliases one type through another, and depends on adjacency. Every phase that
  changes layout — 2, 3, 4 — can change behaviour without changing meaning.
  This is why the gate is a *byte-identical stream*, not "it still compresses".
- **`bmf` opens its output `"a+b"` and reads it back**, and that read-back is
  not only walking an archive — it initialises writer state. A harness that
  reuses one output path, or a "cleanup" that removes the read, breaks things
  in ways that look like a refactoring bug. (Both were hit; see
  `git log 4c0a19c`.)

  The mode itself was changed to `"w+b"` at one point, on the reasoning that
  the command line writes one image per run so appending only meant that
  compressing twice to the same name grew the file. That is what appending
  *is*: two `bmf c` runs against one archive is how a multi-image archive is
  built, and `bmf d` reads every member back. It has been changed back. **A
  harness annoyance is not evidence about what a program is for** — the fix
  was to delete the output before each encode, which is what `test.sh` does.

  Nothing caught it for a long time because every check in the gate compressed
  one image to a fresh path, which is exactly the case where appending and
  replacing look the same. `test.sh` now builds a two-member archive and reads
  both back; reinstating `"w+b"` fails it with *SECOND MEMBER REPLACED THE
  FIRST*.
- **`operator new` is not `malloc`, and the difference is the whole
  out-of-memory path.** `__op_new` was inlined to `malloc` on the reasoning
  that a wrapper whose content is a call to the same-named library function is
  noise. It was not: MSVC's `operator new` calls the handler
  `set_new_handler` installed, BMF installs one, and with the call rewritten
  that store went nowhere and an allocation failure became a SIGSEGV instead of
  *Out of memory!*. `bmf_new` in `bmf.cpp` restores exactly that contract; the
  gate pins it with `ulimit -v`. **A wrapper is only noise once you have read
  what it wraps** — the same reasoning collapsed `fread` correctly and this
  one wrongly, in a single commit.
- **A body no test reaches is a body no gate protects, and one of them was
  broken for the whole project.** `alt_model_p1_decode` — 268 lines, the
  largest thing the corpus never touched — segfaulted the first time an image
  reached it. Bisected to Phase 2: its `__hexrays_frame` was 116 bytes and its
  aliases only named the first 84, and the plain split gave each local its own
  storage, so the code's writes into the trailing 32 bytes landed on unrelated
  stack. At `-O2` that is a null pointer handed to `sub_4248D0`; at `-O0` it is
  a plane that decodes to the wrong pixels. §Phase 2 says 16 of 24 frames could
  not take the plain split; this was the seventeenth, and the only reason it
  looked like one of the eight is that nothing ran it.

  It has the layout-preserving struct the other sixteen have, `testfiles/altp1.bmp`
  reaches it, and the reference stream is byte-identical to the one the
  *original decompilation* produces — so the encoder was right the whole time
  and only the decoder was broken. Reinstating the split fails the gate with
  *altp1: DECOMPRESS FAILED (rc=139)*.

  The lesson is not "write more tests". It is that **the gate's coverage is
  part of the gate**, and 10 % of a file that nothing exercises is 10 % where
  every phase was running unchecked.

  The audit that followed is the useful part. Of the 24 frames, 16 took the
  struct in Phase 2 and 8 took the plain split. **Every one of those 8 has the
  same ~32 bytes past its last alias** — the shape that broke the seventh — so
  rather than argue about which are safe, `tools/reframe.py` converts them: it
  reads each frame's offsets out of the pre-Phase-2 revision, keeps the types
  the later phases gave the locals, and emits the struct with padding and two
  `static_assert`s.

  **All 24 frames now carry `static_assert(sizeof(__frame) == N)`.** Seven of
  the eight went straightforwardly. `model_plane` was written off as the
  holdout — "later work spread its members across shared declaration lists" —
  and that was wrong on inspection: exactly one of its eighteen members,
  `v71`, has a trailing comment on its declaration, and a pattern anchored at
  the end of the line does not see past one. The other assumption in the way
  was that a frame's declarations sit together; they do not, because later
  phases moved some, so each one is now deleted where it is and the struct goes
  where the first was.

  Both of those were properties of the tool, described as properties of the
  code. That is worth more attention than the fix: *the reason a thing looks
  not worth doing is sometimes just the last error message you read.*
- **`read_bmp` validates less than it looks like it does.** It checks the `BM`
  signature, that the DIB header is 40 bytes and that the plane count is 1, and
  then trusts the rest. A **top-down BMP — a negative height, which is legal and
  common — segfaults it**, in `alloc_image` off a negative size. Found by
  feeding it one; the header checks that exist are BMF's own, so the donor is
  unlikely to have fared better. Left alone deliberately: this is a refactoring,
  and a crash that reproduces is behaviour the gate is there to preserve. Worth
  knowing before anyone points this at untrusted input.
- **A 16-bit BMP compresses and then cannot be expanded.** `read_bmp` accepts
  16 bits per pixel; the writer refuses depths 2, 15 and 16, because BMF sent
  those to a TGA writer and this build only writes BMP (`bmf_decompress` says
  so and exits 5). The stream is fine — nothing in this build can turn it back
  into a file. The asymmetry is in the original's shape, not introduced here,
  but the trap is: `bmf c` reports success on input whose output is
  unreachable.
- **Inherited names lie, and so do first readings.** `__n8`, `__n256`, `__n2`
  are the last value assigned, not the meaning. But the opposite error is just
  as easy: an earlier draft of §4.1 concluded `__Buffer` was not a buffer,
  because its address is used as a base for writes 32 bytes away. It is a
  buffer — `__Buffer = malloc(...)` — and it is *also* a strength-reduced base.
  Read every use before renaming, not the first few.
- **Coverage is not correctness.** 90 % line coverage of a codec still misses
  the branch that only a 5-plane 16-bit image takes. Add images when a phase
  touches something the corpus does not reach.
- **Do not trust a green gate on a change you did not understand.** Ten images
  is a signal, not a proof — and it is a thinner one than it sounds, because
  they are ten *similar* images. Arithmetic that runs identically on all ten can
  still differ on the eleventh.
- **Deleting beats testing, where the code is not wanted.** Phase 0 closed a
  35-point coverage gap by removing five modes this program does not offer,
  not by building a matrix to exercise them. Reach for that first: a mode with
  no interface is not a feature to keep working, and a test that keeps dead
  code alive is a cost with no return.

---

## 7. Where the plan was wrong

Every phase is finished. Four of them are not finished the way this document
first described, because the code said otherwise, and that is the more useful
half of the record.

| phase | the plan said | the code said |
| --- | --- | --- |
| 2 | 3–6 frames will resist splitting | 16 did — they carry bytes no alias names and the code runs into them |
| 3 | split every global; extents are the unknown | splitting all at once segfaults on *writes* crossing boundaries. One at a time: 86 move, 77 are parts of larger tables |
| 4 | recover the objects and widen the pointer fields | 1683 constant-offset dereferences against 2184 variable-offset ones, and **no object has only the first kind**. Widening is not available — but recovering the structs is, once the target is 32-bit, because there the layout does not move. 73 of them |
| 6 | the `goto`s are four rewritable shapes | none of the 123 is any of them. One other shape exists; it fits 2 |

Two of those were caught by measuring before starting, which cost nothing. One
was caught by the gate, which is what it is for. One — Phase 3's — was caught by
running the migration and having it crash, and that turned out to be the
cheapest way to get the map.

Phase 4 is the one worth reading twice, because the measurement was right and
the conclusion drawn from it was wrong. "No object's fields can be widened" is
true, and it says nothing about whether the objects can be *named*. Those were
one question only while the target was 64-bit. Dropping that made half of it
answerable immediately, and the code had not changed at all.

The tool that did it was wrong four times before it was right, and every one of
those was found by the gate rather than by reading:

- the alias analysis grouped objects by names that did not exist, because a
  cast-stripping pattern was eating the front of `_this` and calling it `s`;
- offsets were read as bytes for names declared as typed pointers, where the
  code meant elements;
- the rewrite ran line by line against a decompiler that wraps expressions, so
  a dereference split across two lines kept arithmetic that then stepped by the
  whole struct;
- a subscript whose index contained a subscript was not found at all.

Each of those produced a file that compiled. Three of the four segfaulted; the
first one did not, and would have been kept if the streams had not been checked
byte for byte.

### What is genuinely left

Two things, and neither is a phase.

**Naming.** 72 of the 73 structs are `ObjN` with `f76`-style members. The layout is
recovered; the meaning is not. `ModelBlock` is the exception, and it is named
only because §4.2 had already established what it is. `ALGORITHM.md` §9 still lists the
alternate model families as unread, and that is what naming those fields waits
on — not tooling.

**The 64-bit build**, if it is wanted. It compiles and runs and gets as far as
`model_plane`. Taking it further is the loop in §Phase 4 — fault, find the
object, extend the arena — repeated until the images round-trip. It is
mechanical and not blocked on anything, and it ends with a working 64-bit binary
whose fields are still `uint32_t`.

## Appendix A — how the numbers were measured

```sh
# sizes and vocabulary
wc -l subs1.hpp                                       # 23893
grep -o 'blob1 + 0x' subs1.hpp | wc -l                # 164 globals
grep -c 'static inline .*__fwd_' subs1.hpp            # 95 shims
grep -oE '\((const )?[A-Za-z_][A-Za-z0-9_]* *\*+\)' subs1.hpp | wc -l   # 5804
grep -c 'goto ' subs1.hpp                             # 123
grep -c '__hexrays_frame' subs1.hpp                   # 0

# structure recovery
grep -c 'struct \(Obj[0-9]*\|ModelBlock\) {' subs1.hpp  # 73
grep -oE '\->f[0-9]+' subs1.hpp | wc -l               # 2728 named accesses
grep -oE '\*\((const )?[A-Za-z_][A-Za-z0-9_]*( )?\*+\)\([A-Za-z_][A-Za-z0-9_]* \+ [0-9]+\)' \
     subs1.hpp | wc -l                                # 523 raw-offset, from 1646
wc -l tools/struct-skip.txt                           # 30 objects declined
python3 tools/structs.py subs1.hpp --list             # what is left, by traffic

# dead code, as the linker sees it
BMF_GC=list ./build.sh 2>&1 | grep 'removing unused section'

# coverage
#
# Run the gate itself rather than a hand-written imitation of it.  The two
# versions of this recipe that ran their own loop both understated the number,
# once by leaving out the archive and once by leaving out the refusals, and each
# time the missing lines looked like dead code.  `./test.sh ./bmfcov` cannot
# drift from what the gate does, because it is what the gate does.
g++ -m32 -march=k8 -msse2 -mfpmath=sse -std=c++17 -fno-strict-aliasing \
    -fpermissive -fno-rtti -fno-exceptions -O0 -DNDEBUG -U_FORTIFY_SOURCE \
    -D_FORTIFY_SOURCE=0 --coverage bmf.cpp -o bmfcov
rm -f bmfcov-bmf.gcda
BMF_TIMEOUT=600 ./test.sh ./bmfcov                    # 1m43 instrumented, 19 s not
gcov -n    -o . bmfcov-bmf.gcno                       # 95.91 % of 13346 lines
gcov -f -n -o . bmfcov-bmf.gcno                       # per function; nothing at 0.00 %
gcov      -o . bmfcov-bmf.gcno                        # writes subs1.hpp.gcov
awk -F: '$1 ~ /#####/' subs1.hpp.gcov | wc -l         # 546 unexecuted lines
rm -f bmfcov bmfcov-bmf.gc?? ./*.gcov                 # none of this is committed
```

Body sizes, global extents and overlaps, and the `base + constant` families in
§4.2 were all measured with throwaway brace-matching scripts, not with anything
in `tools/`.  What is in `tools/` is what got run repeatedly: `foldif.py`,
`rename.py`, `unframe.py`, `retype.py`, `extents.py`, `mkrefs.sh`. Turning the last two into real scripts is the first task of
Phase 3 and Phase 4 respectively; they are also what would keep the numbers in
this document honest as the work proceeds.

One caveat on the coverage figure: `gcov` counts *instrumented* lines (13 313
of the file's 18 609) and counts inlined copies separately, so its per-function
percentages do not sum the way source lines do. The line counts in §2 are source
lines, measured separately by matching braces over the function list.
