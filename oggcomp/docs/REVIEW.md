# A review of oggcomp

All of `oggcomp/` was read for bugs: the compressor (about 10,000 lines,
not counting the generated `vbooks_gen.inc` and `MOD/`), `Lib3/`,
`MTwrap_v3/`, `oggart.inc`, and the build and test scripts.  Five readers
took the source between them:

- the container and the coder;
- the Vorbis parser;
- the model;
- the program, the library and the threads;
- `Lib3` and MTwrap.

The rest was read separately.  Each reader's findings were checked against
the code, and reproduced where this machine could.

Alongside the reading the program was fuzzed, under AddressSanitizer and
UBSan:

- mutated `.ogg` files given to `c`, which has to give each back;
- damaged `.oc` files given to `d`, which has to refuse them or give back
  the original;
- damaged streams coded on threads.

That was 2,100 encoder runs, 1,100 decoder runs and 850 runs on threads,
before and after the fixes.  On threads the program was also run under
ThreadSanitizer.  It was built with g++, with clang (`-O0`, `-O2`, and
`-O2 -fno-omit-frame-pointer`), and with mingw-w64 for both file backends,
and the mingw builds were run under wine.

**35 defects were found and 33 fixed**, in the commit that adds this file
(2.1 is two of them).  The other two are latent and listed at the end.
Every fix leaves the stream format as it was.  The 54 reference inputs code
to the same `.oc` files, byte for byte, as the program as it was uploaded.
The tuning and shipping builds agree on all 40 files of the corpus
(`./mk.sh check`).  `./t.sh` passes on both builds.

The worst of it:

- **Data loss.** `c` could write a `.oc` that `d` cannot restore, given one
  of two legal setup headers (2.1).  It could also destroy its input when
  the output path named the same file another way (2.2).
- **Crashes in `d` and on threads.** `d` crashed on a valid stream coded on
  threads (1.3).  The shipping build crashed on every `-t` run (1.1), and
  `c -v -t` always crashed (1.2).
- **Build-dependent coding.** An out-of-bounds read made what the model
  coded depend on how the compiler laid out its tables (1.5).

The first two of these, and four smaller ones, came in with this branch's
own work on threads, in commits `c5c23c7` and `426512f`.  Each section says
so where it applies.

| # | where | what | kind |
|---|---|---|---|
| 1.1 | `tc_tables.inc` | the shipping build crashes on `-t N` | crash (from `c5c23c7`) |
| 1.2 | `oggcomp.cpp` | `c -v -t N` reads freed memory | crash (from `426512f`) |
| 1.3 | `MTwrap_v3/loop_dec.inc` | one byte written past a 64 KB block | memory corruption in `d` |
| 1.4 | `Lib3/file_api_*.inc` | a close that fails is closed twice | crash, partial output left |
| 1.5 | `tc_ptab.inc` | `cm_st[4096]` read, one past the table | undefined; coding depends on memory layout |
| 1.6 | `Lib3/coro3_setjmp_x64d.h` | clang with frame pointers crashes at the first resume | crash |
| 1.7 | `oggart.inc` | a count from the metainfo overflows an allocation | heap overflow (not built here) |
| 1.8 | `oggart.inc` | images 8 and on overwrite the extension of images 0 and on | wrong data (not built here) |
| 2.1 | `vb_packet.inc` | two legal setup forms code to a `.oc` that does not restore | silent data loss |
| 2.2 | `oggcomp.cpp` | `c a.ogg ./a.ogg` empties a.ogg and exits 0 | data loss |
| 2.3 | `oc_fatal.inc` | a failed run to stdout deletes a file named `-` | data loss |
| 3.1 | `vb_packet.inc` | `c` exits 1 on an audio packet naming a mode past the last | contract |
| 3.2 | `oc_residue.inc` | `c` exits 1 past 2^20 classification slots | contract |
| 3.3 | `oc_fatal.inc` | `c - -` and `d - -` refused | contract |
| 3.4 | `oc_mt.inc` | on threads, a later failure reported instead of the first | wrong exit status (from `426512f`) |
| 3.5 | `oggcomp.cpp` | a read error in `d` exits 1, not 3 | wrong exit status |
| 3.6 | `oggcomp.cpp` | a header naming model 10 or more exits 3, not 1 | wrong exit status |
| 3.7 | `oc_mt.inc` | MTwrap out of memory: SIGILL | wrong exit status |
| 3.8 | `MTwrap_v3/loop_enc.inc` | a worker failing after the input's end is not noticed | wrong result (MTwrap's) |
| 3.9 | `Lib3/file_api_win.inc` | every pipe into the WinAPI build ends in a read error | contract |
| 4.1 | `vb_packet.inc` | books wider than their partitions: 244 bytes take 2.6 s | denial of service |
| 4.2 | `ogg_stream.inc` | junk dense in `O` is rescanned per 64 KB: quadratic | denial of service |
| 4.3 | `ogg_stream.inc` | a long unterminated packet is walked again at every page | denial of service |
| 4.4 | `ogg_stream.inc` | `d` clears up to 16.6 MB per continued packet | denial of service |
| 5.1 | `oc_api.inc` | `Loop` after `OC_NEED_OUTPUT` with no new buffer writes past it | library misuse, unguarded |
| 5.2 | `oc_api.inc` | `oggcomp_Free` leaks the history's spill | leak (from `c5c23c7`) |
| 5.3 | `tc_tables.inc`, `tc_base.inc` | a failed table init leaves its mappings | leak |
| 5.4 | `oc_progress.inc` | the progress line can call its counter with no context | race (from `426512f`) |
| 5.5 | `oc_load.inc` | Windows loads `oggcompN.dll` from the current directory | DLL planting |
| 6.1 | `mk.sh` | `./mk.sh` exits 1 in this tree | build |
| 6.2 | `t.sh` | a path given to `t.sh` is read against its own directory | test |
| 6.3 | `docs/BUGS.md` | says its four bugs are not fixed; they are | docs |
| 7 | `win_shim.inc`, `tc_fam.inc` | two latent ones, left | -- |

Two bugs that the work on threads found are fixed in their own commits,
not here: `c40ae18` (a `FILE*` was tested by its low 32 bits) and
`426512f` (`d` from stdin read the header twice).

---

## 1. Crashes and memory errors

### 1.1 The shipping build crashes on `-t N`

`./mk.sh release`, then `oggcomp c -t 2 a.ogg a.oc`, dies with SIGSEGV
(exit 139), and so does `d` of any stream coded on threads.  Every
threaded run of the shipping binary did this.  `-t 1` and every tuning
build work.

**Why.** The shipping build kept every model table under 256 MB as a
static object.  `c5c23c7` made the model's state the thread's, which
turned those tables into 68 MB of static thread-local storage.  glibc
carves a thread's static TLS out of its stack.  MTwrap starts its workers
with the default stack, which left them a few kilobytes, and they died at
their first call.

The same 68 MB was also zero-filled by glibc at the start of every thread,
so even one thread had all of it resident.  A 2.7 kB file cost 74 MB of
memory.

It went unseen because the `MOD/` checked into the tree is in the tuning
form.  `g++ oggcomp.cpp` therefore builds the tuning build, and that is
the build every test of the threads had used.

**Fix.** In the shipping build every table is now mapped per thread
(`TC_STATIC_MAX` is 0), as the digit family's 893 MB always was.  The
tables are reached through `tabs`' pointers either way, so no code that
touches them changes and no byte that is coded changes.  The tuning
build's structs of pointers stay static, because `reset()` zeroes every
mapping.

`.tbss` went from 68 MB to 1 MB, and the tiny file now runs in 12 MB.
`./t.sh` on a shipping build runs its threads section, and passes.

### 1.2 `c -v -t N` reads freed memory

`oggcomp c -v -t 2 a.ogg a.oc` died with SIGSEGV after it had written the
output.  `run_mt_c` called `mt_report` after `M2.Quit()` had freed the
workers it reads.  The report also asked the main thread for the tables'
size, and the main thread runs no model, so it would have said 0 MB.

**Fix.** The report runs before `Quit()`, and each worker records the
tables' size on its own thread.  `t.sh` checks `-v` on threads.

### 1.3 MTwrap's decoder writes a byte past a 64 KB block

A worker's last block can carry one byte.  MTwrap writes it as a "tail"
block of length L = 2.  The decoder's tail fix-up wrote two bytes,
`buf[0]` and `buf[1]`, whatever L was.  Usually `buf[1]` lands in the
slack of a ring slot.  But when a worker is starved, blocks are parked in
`dyn_buff` and appended to a 64 KB node, and a node filled to 65,535 bytes
takes the one byte at its end.  `buf[1]` then lands one byte into the next
allocation, which is the size header `win_shim`'s `VirtualAlloc` keeps.
`d` crashed later, in `dyn_buff::Quit`.

The container that does this is **valid**: one worker's blocks first,
then the others'.  It is an interleaving the format allows.  Since `c`
writes such a block whenever a part's last buffer holds one byte, a real
stream could trip it.

**Fix.** `buf[1]` is written only when it is data (L > 2).  The re-blocked
container now decodes to its input.

### 1.4 A close that fails is closed twice

Suppose the last flush fails at `fclose`: a full disk, a quota, `ulimit -f`.
`run()` reports the write error, and the report's way out
(`ogc_partial.drop`) closes the output again.  That is `fclose` twice on
one `FILE`: glibc aborts with "double free" (exit 134), and the
half-written file stays on disk, because the abort comes before the
`remove`.  The WinAPI backend called `CloseHandle` on a stale handle,
which by then may belong to another file.

**Fix.** Both backends take the handle before closing it, so a second
close finds none.  `t.sh` checks that a write failing this way is exit 3
with nothing left behind.

### 1.5 `cm_st[4096]`: an out-of-bounds read that decides what is coded

`tcp_p` answers `TCP_SURE0` (`CM_PONE`, 4096) for a bit that is surely 0.
`tc_pcur::axis()` passed that to `tcp_bucket`, which looked it up in the
4,096-entry stretch table.  So the bucket, which is part of a context, was
whatever followed the table in memory:

- in GCC's builds, `cm_mrec[0]`, read as -1;
- in clang's, 0, read from past the end of `.bss`.

Both give bucket 16, so the builds tested agree by luck.  A layout that
put `cm_sq[0]` there would give 17 and code those files differently, and
streams from one build would not decode in the other.  It is not rare:
`testfiles/rebooked-8k.ogg` hits it 27 times, and one real music file in
17 hit it.  UBSan stopped at it within 20 runs of the fuzzer.

**Fix.** `TCP_SURE0` is bucket 16, explicitly: what every stream so far
holds, without the read.  The reference streams are unchanged.

### 1.6 clang with frame pointers crashes at the first resume

`coro3_setjmp_x64d.h`, the coroutine switch that clang builds use, saved
rip and rsp, and listed rbp as clobbered.  Where rbp is the frame pointer
clang cannot honour that.  This covers `-fno-omit-frame-pointer`, `-O0`,
and the default flags of several distributions.  So after the jump, the
code reloaded its locals through the jumper's rbp.  Every run crashed at
the first resume.

**Fix.** rbp is saved and restored with the other two.  The jump takes
its three values in named registers, so no operand can be allocated to
rbp itself.  clang `-O2`, `-O2 -fno-omit-frame-pointer` and `-O0` all
round-trip, on threads too.

### 1.7 and 1.8 `oggart.inc`: allocations sized from the metainfo

`art_get` reads an image count and a page count from the metainfo, which
is untrusted, and sized allocations from them.  A count of 2^59 images,
times `sizeof(artimg)`, wraps to 0 bytes, and the first image was written
past it.  The counts are now held to what the remaining bytes can encode
before anything is sized from them.

Image extensions were kept in a static buffer of eight, so images 0 and 8
shared one, and images 0 and 1 ended up with the extensions of 8 and 9.
Each image now keeps its own.

`oggart.inc` is included by `oggdet.cpp` and `oggart.cpp`, neither of
which is in this tree.  Both bugs were reproduced, and their fixes
checked, with a small harness under ASan.

## 2. Data loss

### 2.1 Two legal setup forms code to a `.oc` that does not restore

`c` exited 0, and `d` then said "the restored file does not check".  The
original is recoverable only if it was kept.  There were two ways in.

- **A residue cascade in the long form with no high bits.** The cascade
  is three bits, a flag, and five more bits if the flag is set.  The flag
  may be set with the five bits all zero.  libvorbis reads that, but the
  decoder writes the short form whenever it can, so the packet came back
  five bits shorter.
- **A floor post of 32,768 or more.** A post is an entry of a subclass
  book, up to 2^20, but the model keeps floor values in 16 signed bits
  (`fl_cur`).  From 65,536 up the value came back as another value (65,541
  as 5), and from 32,768 to 65,535 it was negative and `c` exited 1.

**Fix.** The parser refuses both.  That happens in the dry run, so the
page is coded as bytes.  `testfiles/cascade-long.ogg` and
`testfiles/floor-post-65541.ogg` are the two cases.

### 2.2 `c a.ogg ./a.ogg` empties a.ogg and exits 0

`ogc_paths_distinct` compared the two paths as strings.  The same file
under another name got past it: `./a.ogg`, a full path, a link, another
case on Windows.  The output's creation then truncated the input before a
byte of it had been read.  The run exited 0, and a.ogg was now the 12-byte
`.oc` of nothing.  `d a.oc ./a.oc` lost a.oc the same way, unless the file
fitted in stdio's first buffer.

**Fix.** `ogc_same_file` compares identity: device and inode, or on
Windows volume and file index.  The run is refused with exit 2 before
anything is opened for writing.  `t.sh` checks it with a path through
`./`.

### 2.3 A failed run to stdout deletes a file named `-`

The failure path removes the output path, and for `-` that was
`remove("-")`.  It took away whatever file of that name sat in the current
directory.  `-` is now never removed.  `t.sh` checks it.

## 3. The contract: `c` takes anything; exit statuses mean what they say

### 3.1 and 3.2 The dry run passes pages the model then refuses

The encoder runs each page through the parser against `null_sink` first,
and codes it as bytes if the parser refuses.  The promise is that a page
that passes is one the model can code.  Two checks were in the model and
not in the parser, so `c` exited 1 on structurally valid files.

- **An audio packet naming a mode past the last.** `ilog(nmd - 1)` bits
  can name one: with three modes, mode 3.  The parser now refuses it
  (`testfiles/mode-past-last.ogg`).
- **More than 2^20 classification slots.** The parser allows
  `VB_CSMAX`, which is 17 times more, and the model refused anything past
  2^20.  Every read and write of the two buffers concerned was already
  bounds-checked, on both sides alike, so the model's refusal is gone.  It
  now codes such residues and remembers what fits
  (`testfiles/classwords-255ch.ogg`).

### 3.3 `c - -` and `d - -` were refused

`-` named twice read as an output over an input.  It is stdin as an input
and stdout as an output: two streams.  `t.sh` checks a run from stdin to
stdout in both directions.

### 3.4 On threads, a later failure was reported instead of the first

When one worker fails, MTwrap ends every worker's input.  A worker in the
middle of a part then fails too ("a part cut short"), and `mt_failed`
reported the lowest-numbered failed worker, not the first.  Under
`ulimit -v 3200000`, coding a 5.7 MB file on four threads exited 4,
"internal error: a part cut short".  The first failure was a mapping
refused, which is exit 3.  A stream decoded short of memory could
likewise be called damaged.  Failures now carry a sequence number and the
first is reported: exit 3, "cannot map 136 MB for the model's state".

### 3.5 to 3.9 Exit statuses

- **3.5** `run()` checked for a read error only after the loop.  A read
  error in `d` therefore looked like the end of the input, and the
  decoder refused the stream as cut short: exit 1.  `main`'s read of the
  header was not checked at all.  Both are checked as they happen now:
  `d` of a directory is exit 3.
- **3.6** `-N` is a digit, so a header naming model 10 or more cannot
  have come from `c`: it is damage, exit 1.  It used to send `d` looking
  for `oggcomp128.so`, and exit 3.
- **3.7** `dyn_buff`, MTwrap's store of blocks for a worker that cannot
  take them yet, called `__debugbreak()` when out of memory, because
  oggcomp did not define `MTWRAP_NOMEM`.  That was SIGILL.  It now
  defines it, and runs out with exit 3.
- **3.8** MTwrap's encoder loop tested worker failures only while
  `f_quit` was 0.  The input's end sets `f_quit`, so a worker that failed
  after it went unnoticed and the job yielded 0.  oggcomp's `mt_failed`
  caught it anyway.  The loop now tests `f_err`, as its decoder does.
- **3.9** A pipe whose writer has closed answers `ReadFile` with FALSE and
  `ERROR_BROKEN_PIPE`.  That is its end of file, as the CRT reads it.  The
  WinAPI backend recorded it as an error, so `type a.ogg | oggcomp c - a.oc`
  ended in "read error on -".  Wine reports EOF there, which is why the
  earlier test under wine passed.  `ERROR_BROKEN_PIPE` and
  `ERROR_HANDLE_EOF` are the end now.  This was fixed from the
  documentation; there is no Windows machine here to run it on.

## 4. Denial of service on crafted input

All four kept their output: the `.oc` before and after each fix is the
same, byte for byte.

| case | before | after |
|---|---|---|
| a residue book 65535 wide for partitions of 1, 244 bytes (`wide-book.ogg`) | 2.6 s each way | 0.02 s |
| 16 MB of `O` bytes | 30.5 s | 1.9 s |
| 10 MB of one unterminated packet on 283-byte pages | 20.5 s | 1.2 s |
| a 3.7 kB `.oc` declaring the largest spill on every page | 2.4 s to decode | 0.03 s |

- **4.1** Every codeword is coded as all its digits, including the ones
  past the partition's end, because they are the entry.  A book 65,535
  wide for partitions of one value made each value cost 65,535 digits.
  Classwords are the same.  The parser now refuses:
  - a residue book wider than its partition, which leaves no partition
    more than twice the digits it has values;
  - a classbook wider than both its partition count and 64.

  A short block with more classes per word than partitions is
  libvorbis's ordinary case, so that is let be up to 64.  No encoder
  writes a book over 8 wide.
- **4.2** `consume_junk` codes junk 64 KB at a time.  Before each chunk it
  called `page_start` from the cursor, which scanned the rest of the run
  again, up to the window's 32 MB.  `page_start` only looks where a whole
  page has room after the spot, so what it passed over can never become a
  page.  The scan now resumes where it stopped, and the answer is the one
  a scan from the cursor would give.
- **4.3** `join_len` walked a packet's continuation pages once for every
  page of them, when the join was refused: N²/2 page parses.  A refusal
  for an end of stream, a missing page or a page that does not continue
  is reached the same way from any later page of the chain.  It is
  remembered, with the window it was found in, and repeated without the
  walk.
- **4.4** The decoder clears the buffer a continued packet is written
  into.  It cleared the packet's length plus its spill, up to 16.6 MB,
  which the stream names.  It now clears only what an earlier packet may
  have written there (`jb_hw`).

## 5. The library

- **5.1** `Loop` after `OC_NEED_OUTPUT`, with no new buffer, resumed the
  coroutine into the write it had stopped before.  The pin writes and then
  checks, and does not check again on resume.  So the next byte went past
  the end of the caller's buffer.  This happens with `addout` skipped, or
  given a size of 0, which it ignores.  It is the class of `BUGS.md` #2,
  which its guard on `pin[1].beg` does not cover.  `Loop` now returns 8
  until a buffer has been given, and the run goes on from there.
  `oc_api_test` checks both, with a guard band behind a 16-byte buffer.
  It fails on the old code with "WROTE PAST THE BUFFER".
- **5.2** `oggcomp_Free` unmapped the history pool without freeing its
  spill, the tables `calloc`'d for a setup that asks for more than the
  pool.  It leaked them once per instance, and a run on threads frees an
  instance per worker.  `hist->reset()` comes first now.  (From `c5c23c7`,
  which made `Free` unmap.)
- **5.3** A table init that failed part way, for example on a mapping
  refused, left its mappings in `tc_maps`.  The next `Init` mapped a second
  set beside them, until `tc_maps` was full and said so as an internal
  error.  `tc_map` also checked that limit after the `mmap`, leaking the
  last mapping outright.  `init()` now starts from nothing, and the check
  comes first.
- **5.4** `counts(nullptr, nullptr)` cleared the context before the
  function.  A worker polling the progress line between the two stores
  could call the counter with no context.  Clearing now goes in the other
  order, and `poll()` checks both.  (From `426512f`.)
- **5.5** On Windows the library's fallback was `LoadLibraryA` by bare
  name, which searches the current directory.  The model's number comes
  from the header of the `.oc` that `d` is given, so a `.oc` naming model 7
  beside a planted `oggcomp7.dll` would have run it.
  `SetDllDirectoryA("")` takes the current directory out of the search and
  leaves PATH in.  On Linux, a library beside the program that would not
  load, for another architecture or for want of a dependency, was
  reported by the fallback's "not found".  It is now reported as itself.
  (Since then the models have become levels of the one program, and
  nothing is loaded: `oc_load.inc` is a table of the program's own
  functions, and a header naming a level the build has not is refused
  with exit 1.)

## 6. Build, tests, documentation

- **6.1** `./mk.sh`, `release` and `pgo` built oggcomp and then stopped
  with the compiler's "no such file": this tree has no `oggdet.cpp`.  It
  exited 1 every time, and `release` and `pgo` exited before their checks
  ran.  `build_det` now says so and goes on.
- **6.2** `t.sh` changes into its own directory before reading its
  arguments.  So `oggcomp/t.sh oggcomp/testfiles/a.ogg`, from the
  directory above, found nothing and failed.  `-x build/oggcomp` from
  anywhere else said the binary was not there.  A path given is now the
  caller's, where it exists there.
- **6.3** `docs/BUGS.md` said none of its four bugs was fixed in this
  tree.  All four are.  It says so now.

New tests: the five setups of sections 2 to 4 are in `testfiles/`, built
bit by bit by the new last section of `testfiles/gen.sh`.  `t.sh` has five
new checks:

- one file named twice;
- a write that fails;
- `c - -` and `d - -`;
- a failed run to stdout beside a file named `-`;
- `-v` on threads.

The corpus round trip covers the setups; `oc_api_test` covers 5.1.  Run
against the program as it was, the new `t.sh` fails all five checks, and
four of the five setups do not round-trip.  The fifth, `wide-book.ogg`,
only took its 2.6 s each way.

## 7. Found and left

- **`MTwrap_v3/win_shim.inc`: `CloseHandle` on a thread not yet joined.**
  It detaches the thread and frees the block the new thread may not have
  read its function and argument from yet.  This is a use-after-free for
  the usual `CloseHandle(CreateThread(...))`.  MTwrap always joins first,
  so it never happens here.  The fix is to let whichever of the two
  finishes last free the block.
- **`tc_fam.inc`: the Volume limit for hashed tables.**
  `tc_tables.inc` says a Volume must stay under 2^31, because indices are
  built in an `int`.  `wire()` checks the hashed row count, not the
  Volume.  The largest today is 313,528,320.  It would matter only if
  `IDX/opt.pl` widened a pattern past 2^31, and under the mandatory
  `-fwrapv` it would wrap into range rather than fault.

## 8. Not bugs, but worth knowing

- **MTwrap's ring buffer and done flags are `volatile` with fences, not
  atomics.**
  - That is formally a data race, and ThreadSanitizer reports it.
  - It works with GCC, clang and MSVC on x86 and ARM, and it is
    m4vpack's design.
  - With those made atomic in a test copy, ThreadSanitizer reports
    nothing in oggcomp's own code.
  - MTwrap here is m4vpack's apart from two fixes, 1.3 and 3.8, which
    m4vpack's copy wants as well.
- **A stream coded on threads is not the same bytes from run to run.**
  MTwrap writes the workers' blocks as they come.  `d` gives the same
  file from all of them.
- **Memory on threads.** A part ends at eight times `-c` where no cut
  comes, so `-c 1024` on a large file of junk holds parts of up to 8 GB,
  in the splitter and in each worker.
- **A hostile setup can ask for gigabytes of history.**
  Up to about 4.3 GB is `calloc`'d.  The limit is set in `oc_hist.inc`.
- **The squash table is computed with `exp()` at start-up.**
  - A libm that rounds differently could, in principle, build another
    table and so another stream.
  - Computed exactly, the entry nearest a rounding boundary is 1.7e-4
    from it, which is no libm's error.
  - Every entry this machine computes matches the exact value.

## 9. What was checked and found sound

- **The range coder.**
  - Renormalisation keeps the range at 2^24 or more.
  - No frequency gives an empty sub-interval.
  - The carry and the run of 0xFF bytes are handled.
  - The flush picks a value inside the interval.
  - The decoder's read past the end is capped, so a truncated stream
    cannot hang it.
- **Pages.**
  - Lacing pack and unpack are inverses.
  - Tail packets are multiples of 255.
  - The page emitter stays inside `PAGE_MAX`.
- **The encoder and the decoder agree throughout.**
  - Raw runs, page flags, groups, BOS handling, continuation spill.
  - Granule deltas modulo 2^64.
  - The header-or-audio decision.
- **Inside a dry run** nothing can FATAL or yield, so `vb_dry` cannot
  leak.
- **The parser** range-checks every field the decoder takes from the
  model.
- **The codebook table** is consistent: every row's lengths and quantlists
  are long enough, and pack as libvorbis packs them.
- **Per-thread state.** Every mutable static is `OC_TLS`, except the
  coder's tables, which are filled once under a thread-safe guard.
- **Solid runs.** `reset()` clears what a fresh process would have, and
  `oc_next` keeps what a solid run keeps.
- **Large inputs.** Window offsets, part sizes and counts are 64-bit, so
  inputs over 4 GB and parts over 2 GB are fine.
- **MTwrap's block format and its decoder's bounds checks** are right.
  Its wakeups recheck the ring after every event, so none is lost, and a
  failed worker ends the run.
- **Fuzzing found nothing more.**
  - 250 damaged containers from the reviewer of `Lib3` and MTwrap, and
    850 more, gave no crash and no hang.
  - Every damaged `.oc` was refused or decoded to its input.
  - No damaged `.oc` decoded to something else with exit 0: every `.oc`
    ends in a CRC of the file it holds.
