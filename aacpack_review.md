# aacpack: a code review

A review of all of aacpack: its sources (`aacpack_src/`), the IDX
generator and tuning tools (`aacpack_src/IDX/`, `mk.sh`), the Makefile
and the tests (`test/`). It looked for bugs, duplicate code and unused
code. The review is of the tree as of commit 82eb6a44 (format version 7).
Each finding below says what was done about it.

## How

- **Five independent reads**, one per area:
  - A: the syntax walk and the containers;
  - B: the model and codebook selection;
  - C: the coder core;
  - D: the compressed stream, drivers and command line;
  - E: the tooling and tests.
- **Every finding was checked again against the code.** Where one claims a
  failure, it was reproduced (the reproductions are described with each).
  Two reads reporting the same thing are merged under one id.
- **Compiler warnings:** gcc 13.3 with `-Wall -Wextra
  -Wshadow=compatible-local`, clang 18.1 with `-Wall -Wextra -Wshadow
  -Wcomma`.
- **clang-tidy:** its `bugprone-*` and `clang-analyzer-*` checks.
- **Unused code:** a scan for identifiers that occur only once.
- **Duplicate code:** a scan for repeated blocks of at least four
  normalized lines.
- **Float-built tables:** a 60-digit recomputation of each, to see how
  close any entry comes to a rounding boundary.
- **Runs:**
  - ASan and UBSan builds;
  - `test/fuzz.py` and fuzzers of compressed streams;
  - a generator of random valid streams (zero-length sections, every
    window sequence, CPE with and without a common window, intensity,
    noise, escapes, pulses, TNS, CCE, DSE, FIL);
  - gcc and clang builds at `-O0` and `-O2`, with and without frame
    pointers;
  - streams over 4 GB.
- **Severity:**
  - high: a wrong result or a crash on valid input, in a build the README
    supports;
  - medium: data loss, a security hole, a check that cannot fail, or a
    wrong result in a reachable edge case;
  - low: the rest.

The ids are the area's letter (M: the mechanical checks).

## Bugs

| id | where | sev | what | done |
|---|---|---|---|---|
| D1 | `cmstream.inc` `Scanner::unit` | high | `c` failed on valid input, see below | fixed |
| C1 | `Lib3/coro3b.inc`, `coro3_setjmp_x64d.h` | high | clang builds crashed at `-O0` and with frame pointers, see below | fixed |
| B1 = A1 | `model.inc` `cs_sections` | high | `c` failed on a legal stream with more than 4096 zero-length sections in a group, see below | fixed |
| M1 | `Lib3/coro3b.inc` `STKPAD` | high | the documented debug build failed on some streams, see below | fixed |
| D2 | `cli.inc` `Output::open` | medium | output `-` was not checked against the input | fixed |
| D3 | `cli.inc` `Output::open` | medium | where the temporary file could not be made, the destination was written in place | fixed |
| D5 | `cli.inc` `Output::open` | medium | the temporary file was opened through links | fixed |
| D4 | `cli.inc` `Output::close` | medium | a failed `d` replaced an existing file with what it had restored | fixed |
| D8 | `Lib3/coro3b.inc`, Makefile | medium | non-x86-64 targets failed in obscure ways; the Makefile said any target builds | fixed |
| C2 | `fam.inc` `code`, `codes_in` | medium | on damaged input, a decoded value could exceed its range | fixed |
| E1 | `mk.sh check` | medium | with no streams given, failed when not run from `aacpack_src` | fixed |
| E2 | `IDX/opt.pl` | medium | unquoted paths; the first measurement was not checked | fixed |
| E3 | `IDX/import.pl` | medium | never failed | fixed |
| E4 | `test/roundtrip.sh` | medium | passed with a missing file, or with none | fixed |
| E5 | Makefile, `test/gen.sh` | medium | after a failed `gen.sh`, `make test` passed on the partial corpus | fixed |
| E6 | `test/gen.sh` | medium | encoders that failed were hidden | fixed |
| E7 | `test/fuzz.py` | medium | its exit status ignored its own checks | fixed |
| E8 | `mk.sh` `generate`, `idx2inc.pl` | medium | the shipping form could leave a parameter live | fixed |
| E15 | Makefile, `mk.sh check` | medium | nothing tested `-b` | fixed |
| D6 | `cli.inc` `hook_signals` | low | most fatal signals left the temporary file behind | fixed |
| D7 | `cli.inc` `Output::close` | low | `tmp` was freed before it was nulled | fixed |
| D9 | `cmstream.inc` `tell_unit` | low | ADIF was shown as raw access units | fixed |
| D10 | `cli.inc` `Output::open` | low | an output link to nothing was replaced by a file | fixed |
| B2 | `model.inc` `cs_kinds`, `cs_sections` | low | `-v`'s section rows were wrong in the default order | fixed |
| C3 | `mem.inc` `Buf::reserve` | low | the byte count could overflow | fixed |
| C4 | `mapping.inc` `mapping::Init` | low | signed overflow in the tuning build | fixed |
| C6 | `cmbase.inc` `tc_map` | low | a failed map went to the wrong trap | fixed |
| R1 | `cmbase.inc` `tc_clamp` | low | `int` only, called with 64-bit arguments | fixed |
| R2 | `model.inc` `coef` | low | read a value not yet coded | fixed |
| E9 | `idx2inc.pl` | low | an expression base was pasted unparenthesized | fixed |
| E10 | `idx2inc.pl`, `mk.sh` | low | a family's own template was used by the tuning form only | fixed |
| E11 | `test/check.sh` | low | the strict-mode test passed on any failure | fixed |
| E12 | `IDX/ftune.py` | low | reported success when it had tuned nothing | fixed |
| E13 | `IDX/ftune.py` | low | ignored ^C | fixed |
| E14 | `IDX/opt.pl`, `mk.sh` | low | interrupted, left the binary out of step with its export | fixed |
| E16 | `mk.sh check`, `test/fuzz.py` | low | coded `NAME.asc-HEX.raw` without `-asc` | fixed |
| E17 | `idx2inc.pl`, `mapping.inc` | low | a wide `&` mask made a 2^n table in the tuning form | fixed |
| E18 | `test/adts2adif.py`, `adts2loas.py` | low | hung on an ADTS frame length of 0 | fixed |
| E19 | README | low | its one-line build lacked `-ffp-contract=off` | fixed |
| E20 | `famtune.cpp`, `ftune.py`, `idx2inc.pl` | low | stale references | fixed |
| B3 | `model.inc` `hdr_val` | low | coupled SBR channel 1 read a stale grid | changes the format: open |
| B4 | `model.inc` `sbr_val` | low | the mean of frequency-delta envelopes was off by one band | changes the format: open |
| B5 | `IDX/l9/spc.idx`, `sgn.idx` | low | codebooks 10 and 11 shared every context with `-b` | changes the format: open |
| B6 | `scf.idx`, `sec.idx`, `hdr.idx` | low | some window-sequence and hx contexts are lumped | changes the format: open |

### D1: `c` failed on valid input

`Scanner::unit()` took a pointer into the input window (`p`). It then
coded the raw bytes before the unit (`flush_raw()`), and that can move the
window (`InWindow::release()` erases what was coded). Finally it compared
the model pass's output with the bytes at the old pointer.

The window moves only once the coded part is at least 1 MB and at least
half of what is buffered. So the stale bytes were usually the old copy,
which survives the move, and still compared equal. Whenever the window's
16 bytes of zero padding landed on the unit, `c` stopped with "internal:
the model pass did not walk the unit as it was parsed".

- **Repro:** an ID3 tag of T bytes, then 2000 copies of one 1420-byte
  frame of `ff_lc256.aac`. T = 1441791, 1441792, 1441796 and 1441807
  failed; 1441808 worked. An ID3 tag before 18 copies of
  `ff_mono64.asc-1208.raw`, coded with `-asc`, fails the same way.
- **Fix:** the check reads the unit's bytes at `win().at(pos)` after
  `flush_raw()`. The check is now one function, `walked_as()`, shared with
  `writes_back()` (D11).
- All the reproductions round-trip now.

### C1: clang builds crashed at `-O0` and with frame pointers

`coro3b.inc` chose an inline-asm setjmp for clang (`coro3_setjmp_x64d.h`).
It saved and restored only rip and rsp; it listed rbp as clobbered, which a
function that keeps its frame pointer cannot honour, and did not restore
rbx either.

- **Repro:** a clang `-O0` build crashed (SIGSEGV) on any raised error,
  e.g. `d` of a compressed stream cut short. A clang `-O2
  -fno-omit-frame-pointer` build crashed on plain `c`.
- **Fix:** clang uses the `__builtin_setjmp` header gcc uses, and the asm
  version is gone.
- clang `-O0`, clang with frame pointers and gcc `-O0` now code the test
  corpus to the same bytes as the `-O2` builds, and fail cleanly on the
  error cases (exit status 1).

### B1 = A1: zero-length sections

The default order decodes sections of no bands with a guard against a
corrupt stream that never ends them:

    if (++n > 4096) err_raise_s("corrupt stream: sections of no bands without end");

It ran when compressing too. The syntax allows any number of zero-length
sections: about 9000 fit in one ADTS frame. The model pass has no trap of
its own, so a frame with 4097 of them in a group ended the whole
compression. That frame parses, writes back, and round-trips with `-b`.

- **Fix:** the guard is the decoder's alone, and its bound is what the
  unit could hold: 7 bits a section at least, so `fw.cap / 7`.
- The 4097-section stream round-trips in both orders now.

### M1: the debug build failed on some streams

A yield copies the coroutine's stack out into `Coroutine::stk`, which had
64 KB. Without optimization the frames are larger: the Makefile's own debug
build, `make CXXFLAGS="-O0 -g"`, needed up to 103 KB. 2 of the corpus's 22
streams failed with "internal: the coroutine's stack (102913 bytes) is too
deep to yield" (`d` of `ff_mixed.bin`; `c` of `ff_lc256.aac`).

- **Fix:** 256 KB. A yield copies only the depth in use, and pages that are
  never touched cost no memory.
- Both `-O0` builds round-trip all 22 streams, byte-identical to `-O2`.

### The output file (D2-D7, D10)

`cli.inc`'s `Output` writes a regular file under a temporary name beside
it and renames it as the call ends well.

- **D2:** `-` skipped the "not the input" check. So `c in - >> in` grew the
  input without end, and `d x.apk - >> x.apk` damaged `x.apk`. The
  descriptor's file is now compared with the input's (`same_fd()`).
- **D3:** where `name.PID.tmp` could not be made, `name` was written in
  place. That happens with a name too long for `NAME_MAX`, or a directory
  that cannot be written. A failed call then deleted the original, and a
  signal left half a file.
  - Now a name too long gets `.aacpack.PID.tmp` in the same directory, and
    a number is added where a name is taken.
  - Where no temporary file can be made, the call fails without touching
    the destination.
- **D5:** the temporary file was opened with `fopen("wb")`, which follows a
  link and truncates. A link planted at the predictable name got the
  stream written through it, and its target's mode changed. Now it is
  `open(O_CREAT | O_EXCL | O_NOFOLLOW)`.
- **D4:** `d` keeps what it restored before an error. It also did so over
  an existing file, which then held a partial, partly wrong stream. Now it
  keeps that only where there was no file, and says so where there was.
- **D6:** only INT, TERM, HUP and PIPE removed the temporary file. QUIT,
  ALRM, USR1, USR2, XCPU, XFSZ, VTALRM and PROF do now too. Tested: each of
  QUIT, USR1, ALRM and XCPU left a file before and leaves none now.
- **D7:** `close()` freed `tmp` before it nulled it, and the signal handler
  reads `tmp`. Now it is nulled first, as `Quit()` already did. Also, the
  candidate names are kept apart until the file is made, so that a signal
  never removes a name that is not this call's.
- **D10:** a dangling link was taken for a new file, and the rename replaced
  the link. Now that is refused, as GNU cp does.

### The rest of the bugs

- **D8:** every target but x86-64 got i386 asm (an aarch64 build failed in
  the asm). The `CORO_NOASM` fallback aborts under glibc's fortified
  longjmp. And the Makefile said `PARCH=` builds for any target. Now any
  target but x86-64 and i386 stops with `#error`, and the Makefile and
  README say x86-64.
- **C2:** `tc_fam::code()` bounds the unary length by `xmax`, not the top
  length's mantissa. So a damaged stream could decode a value above `xmax`
  (and `codes_in()` one outside `[lo, hi]`). The walk rejected it later,
  but `hdr_val()`'s hooks saw it first. Now the decoder stops there:
  "corrupt stream: a value of ...". The compressor never codes such values.
- **B2:** in the default order, `-v` booked the bits of the section starts,
  the zero-length sections and the codebooks under `sect.len`. `sect.cb`
  counted kind runs, not sections. Now each part goes to its element, and
  each section is one value of both, as with `-b`. On `ff_mono64.aac`:
  `sect.cb` 58 bytes and `sect.len` 704, 3120 sections each; before, 17 and
  744, over 2507 kind runs. Coding is unchanged.
- **C3:** `Buf::reserve()` did not check `m * sizeof(T)` for overflow. Now it
  is an out-of-memory error.
- **C4:** `mapping::Init()` shifted a signed `value` left for every bit of a
  pattern, up to 69 of them. That is defined only with `-fwrapv`, and
  `value` is read for `Number`s alone. It is computed unsigned now.
- **C6:** `tc_map()` raised to the innermost trap. Running out of memory is
  an error of the run (`err.inc`), so it is `err_fatal()` now.
- **R1:** `tc_clamp(int, int, int)` was called with `i64` arguments, which
  were narrowed. There is an `i64` overload now. The values were in range:
  no change in coding.
- **R2:** for short windows, `D.pn` read `pp[k + 1]` of the window before.
  At a band's last place in the same group, that value is not coded yet.
  It read 0 only because the coefficients are cleared first. It is guarded
  now; coding is the same.
- **E1:** `mk.sh check` with no streams globbed `../test/corpus/*` relative
  to where it ran. Now it is relative to `mk.sh`'s directory, and paths go
  through `abspath`.
- **E2:** `opt.pl` ran the binary through a shell with unquoted paths, so a
  space in a path failed every run. A first measurement that failed was
  then taken as the best, and the run ended with "Done: 2147483647", exit
  status 0. Now the binary runs without a shell, and a first measurement
  that fails stops `opt.pl`.
- **E3:** `import.pl` opened its files without `or die`. A missing export
  changed nothing, and a missing `.idx` gave empty output, both with exit
  status 0. Now both fail, and so does an export with no `mdesc` line.
- **E4:** `roundtrip.sh` skipped a missing file and passed with no files at
  all. Both fail now.
- **E5:** `gen.sh` stops at the first error (`set -e`). It had made
  `test/corpus` first and left its temporary WAVs there, and the Makefile
  tested only that the directory exists. Now a trap removes the temporary
  files, and `gen.sh` writes `test/corpus/.done` last, which is what the
  Makefile tests.
- **E6:** `gen.sh` hid each failed encode (`2> /dev/null || rm`). With an
  fdkaac whose library has no SBR, as here, the corpus had no HE-AAC or PS
  stream, and nothing said so. Now each failed encode is reported, and so
  is the missing SBR and PS.
- **E7:** `fuzz.py`'s exit status ignored units that would not come out as
  they were, and its round-trip half never looked for sanitizer reports.
  Both count now, in both halves.
- **E8:** the shipping form came from a `sed` of `Const 0` to `Const 1`.
  A line's `-` toggles `Const`, so it became live again. Lines before the
  first `Const` and an indented `Const` were missed too. `mk.sh mod` then
  wrote `pdesc` into MOD/ and exited 0. No `.idx` has such a line today.
  - Now `mk.sh` runs `idx2inc.pl` on the `.idx` itself with
    `IDX_NOCONST=1`, which folds every line.
  - `mk.sh` fails if a `pdesc`, `pmask`, `mdesc` or `mmask` is left in the
    shipping form.
- **E9:** a base was pasted into the folded code unparenthesized, so
  `1<<2` gave 9 in the tuning build and 24 in the shipping build. Nothing
  uses such a base today. Bases that are not plain numbers are
  parenthesized now; MOD/ is unchanged.
- **E10:** the shipping copy was named `<f>-const.idx`, so a family's own
  template `<f>.inc` was never found and `family.inc` was used instead.
  E8's fix removes the copy.
- **E11:** `check.sh`'s strict-mode test took any non-zero status, a crash
  included. It takes 1 now.
- **E12:** `ftune.py` printed "all: 0.0 -> 0.0" and exited 0 when it had
  tuned nothing: no dumps, an unknown family, or every trial failing. Now
  each of these is an error.
- **E13:** `ftune.py` ignored ^C, because its executor waits for every task
  submitted, and its family copies stayed behind. Now ^C stops it at the
  next trial (about 3 s here, against 20 s and more before). It exports the
  best so far, and removes the copies.
- **E14:** `opt.pl` interrupted left the binary with the pattern it was
  trying, which is in no export. Now on INT, TERM and HUP it puts the best
  patterns back first. Its temporary file is removed at the end, and
  `mk.sh`'s recipe points `opt.pl` at a copy of the binary.
- **E16:** `mk.sh check` and `fuzz.py` coded `NAME.asc-HEX.raw` without
  `-asc`, i.e. as bytes, testing nothing of the model. Both pass `-asc` now,
  as `roundtrip.sh` does.
- **E17:** the tuning form maps an `&` mask through a table of 2^length
  ints, so at 32 bits it overruns. The masks are 8 bits today.
  `idx2inc.pl` now refuses a mask over 20 bits in the tuning form.
- **E18:** both ADTS converters looped forever on a frame length of 0. They
  share `test/adts.py` now (E25), which steps over such a header.
- **E19:** README's one-line build lacked `-ffp-contract=off`. The binary
  had 8 FMA instructions, though the outputs of 115 streams were the same.
  It has the flag now.
- **E20:** fixed `famtune.cpp`'s example family (`sig`, which aacpack has
  not), `ftune.py`'s H.264 "pcm without I_PCM", and `idx2inc.pl`'s
  references to other projects (bmf, OGGCOMP-SPEED.md).
- **E15:** `-b` is a model of its own, which neither `make test` nor
  `mk.sh check` ran. `make test` round-trips the corpus in both orders now,
  and `mk.sh check` compares the three builds in both orders.

### Context inaccuracies: changes of the format

These are symmetric, so no stream decodes wrongly. Each fix changes what
the compressor writes, so it needs a new format version.

- **B3:** in coupled SBR the grid is coded for channel 0 only. So channel
  1's grid tracker (`sbrt_`) was never updated, and its `E_SDF_ENV` /
  `E_SDF_NOISE` context read the last uncoupled frame's grid.
- **B4:** `D.rm` (the mean so far) divided the sum of the frequency-delta
  envelope's deltas by `band`, not `band - 1`. So band 1 read "mean 0", not
  the first delta.
- **B5:** `cb, 1!1111111111` has 10 thresholds for codebooks 1-11. So
  codebooks 10 and 11 shared every spc and sgn context with `-b` (the
  default order codes the values without their codebook).
- **B6:**
  - scf and sec pass the window sequence (0-3) under `1!1`, which puts
    LONG_START and LONG_STOP with EIGHT_SHORT (spc, esc and sgn pass a
    short-window flag);
  - hdr's `hx` reaches 20 for `E_WIN_SHAPE`, under 16 thresholds.

## Duplicate code

| id | where | what | done |
|---|---|---|---|
| M2 = B-D1, B-D2 | `model.inc` | the codebook tree, the section bit, and the first codebook's coding with the digest, each written twice (`sect_cb`/`sect_len` and the default order's) | one `sec_tree()`, `sec_bit()`, `sec_first()` |
| B-D4 | `model.inc` `sf_val` | `cb == 13 ? 2 : cb >= 14 ? 1 : 0` 5 times | `sf_kind()` |
| B-D5 | `model.inc` | `kScale.v[tc_clamp(d, -60, 60) + 60]` 8 times | `kScale.at()` |
| B-D8 | `model.inc` | four `*_make()` builders of tables | `table_make<T>()` |
| B-D9 | `model.inc` `coef` | window to group, twice | `group_of()` (the history's walk over the windows stays) |
| B-D10 | `cbsel.inc` `fdk` | the cheapest codebook of merged bands, twice | `best_of` |
| D11 | `cmstream.inc` | the walk-back comparison, in `writes_back()` and the model pass | `walked_as()` |
| D12 | `drivers.inc` | `CodeLv::run` and `DecodeLv::run` | `run_lv<LV, DEC>()` |
| D13 | `drivers.inc`, `coro.inc` | the header parsed twice; the coroutine's level and order mismatch could not be reached | `decode()` parses it, the coroutine skips it |
| C7 | `cm.inc` | `cm_arec[]` is `cm_rec[0..256]` | `cm_rec` |
| C9 | `fam.inc` | the counters' count lanes computed in `cnt_inputs()` and again in `cnt_update()` | kept in `CntRows` |
| C10 | `rc.inc` | the held byte and its 0xFFs emitted in `ShiftLow()` and `rc_Quit()` | `emit_held()` |
| C11 | IDX/l9/family.inc, `famcfg.inc` | every table's size written twice, only W's checked | each checked (`static_assert`) |
| A6 | `syntax.inc`, `units.inc` | `(8 - pos % 8) % 8` 4 times; the alignment-padding test twice | `BitWriter::to_byte()`, `BitReader::not_zero_pad()` |
| A7 | `syntax.inc` | the writing of a counted extension, in `ps_data()` and `sbr_extension()` | `ext_write()` |
| E24 | `mk.sh`, README | the compile flags; `famtune`'s build its own | `build()` takes the source; README's has the flags |
| E25 | `test/adts2adif.py`, `adts2loas.py` | the ADTS frame loop and the bit writer | `test/adts.py` |

Kept, and why:

- **B-D3:** the `fn` formula appears three times, but the three formulas
  differ.
- **B-D6:** the long and short `av` updates differ in decay and layout.
- **B-D7:** small values computed twice (a rounded division, an index):
  each is one line.
- **B-D11:** the compressor walks the channel's sections twice, once for
  kinds and once for starts; two representations of one list.
- **B-D10:** codeword and sign bits are counted by index in `SpecPrior`
  and by values in `CbSel::tuple_bits`.
- **D14:** `name_kind()` and `file_size()` stat by name and by descriptor.
- **C10:** the hold, yield and resume bracket of `tell()` and `wait()`.
  These are `NOINLINE` frames that the stack copy depends on.
- **C11:** `__min`/`__max` beside `Min`/`Max`: the generated code's.
- **E23:** the `!MAP!` scanners are in three languages (perl, python, sh).
  The format is IDX-FORMAT.md's.

## Unused code

Each confirmed by grep over the sources, MOD/ and the tools.

| id | what | done |
|---|---|---|
| C12 | the hint of `tc_fam` (h264p's): `hint` is always -1 in aacpack. That leaves the members `hint`, `hs`, `Z`, `ZM`, `bz`, `bn` and the mixer `mxm` without use. So are the hint's mixer input (always 0) and the hint states in `code()`, `bitm()` and the dump records. Then famcfg's `lrm`, `mbm`, `hN`, `kN`, IDX/l9/family.inc's tables WM and WMC, and the 5-fold rows of WF and WFC. Last, every family's index `n` and its knobs `lrm`, `mbm`, `hN`. | removed, with famtune's replay of it. Coding unchanged: the input was 0, so its weight never moved, and the final mixer read row `node * 5 + 0` |
| C13 | `cm_cnt2s` (h264p's -1 counters) and the overloads that chose between it and `cm_cnt2`: `tc_sp`, `tc_sq`, `tc_st4`, `tc_upper`, `tc_put_sq`, `tc_x20`; `NoRef` | removed |
| C14 | the shapes no family has: 2-8 counters, one mixer, no APMs | removed; famcfg checks the shape. Kept: APM 2's `bw == 16` path, since `bw` is tunable |
| C14 | `ErrTrap::code` and `err_jump()`'s code; `tc_sq(i32)`; `CM_PREFETCHW`; `Buf::back`, `Buf::same`; `masking::map` | removed |
| C14 | `rc_Byte()`'s second renormalization never runs: range is at least 2^12 before any | kept: it guards the coder's invariant for one predicted branch |
| M3 = C14, E21 | famcfg's `Uses` and `FC_USE`, read by nothing; `idx2inc.pl`'s comment said the model computes only those | removed; the comment corrected. The generated `_USES` lists stay: the format has them (IDX-FORMAT.md §14) |
| M4 | `progress_poll()` (= D17), `dump_discard()` and the dump's `discard()` with its `start` (= C14), `Huff::bits_of()` (= A4), `abs64()` (= C14), `Buf::pop_back()` | removed |
| M5 | `tables.inc`: `#define swb_offset_768_32 swb_offset_768_48`, of a table that does not exist, and an orphaned comment | removed |
| D15 | Lib3 files nothing includes: `coro2pair.inc`, `coro_fhp2.inc`, `win_shim.inc`; `file_api.inc`'s branch to a `file_api_win.inc` that is not there | removed; aacpack includes `file_api_std.inc` |
| D16 | Lib3's unused parts. common.inc: `bzero`, global `Min`/`Max`, `macro_Min`/`Max`, `AlignUp`, `wc`, `for_e*`/`while_e*`, `flen`, `log2i`, `X64flag`, `sqword`, `__assume_aligned`, `restrict`. coro3b and coro3_pin: `CAPI`, `f_init`, `f_quit`, and the byte get/put and length methods. file_api_std: everything but `open`/`read`/`sread`/`writ` (its `operator int` cut a pointer to an int) | removed; `coro3_setjmp_x64d.h` too (C1) |
| D17 | `AacArgs::level`, `order`, `meter`: set, never read | removed |
| D18 | `c.f_quit = 1` in `run_file()`: read by nothing | removed |
| A2 = B-U8 | `Walk::Pos::win`, `k`, `ss` and `ChanData::pulse`: written, never read (`at.k`, `at.win` per coefficient) | removed |
| A3 | `Chan::end()` | removed |
| B-U1 | `ChanHist::group_len` | removed |
| B-U2 | the spc family's variable `kq`, read by no spc.idx line (sgn's `kq` is computed directly) | removed |
| B-U3, B-U4, B-U5 | a dead local in `val()`; `sbr_val()`'s parameter `n`; a test that is always true (`bd - 1 < 64`) | removed |
| A5 | the element `E_AU_RAW` | changes the format (the digest's tokens carry the element's number): open |
| E22 | `idx2inc.pl`'s `IDX_NOCONST`, only ever set to its default | used now (E8); `IDX_USENEW` stays, documented |
| B-U6 | variables constant in one order (spc's `cb`, `tp`, `sc` in the default order; sec's `pr`, `pt`, `ch`, `bq` with `-b`) | kept: each order uses them; tune each order |
| B-U7 | thresholds past a variable's largest value (sec `fn`, unt `fld` and `ctx`, sbr `en`) | kept: they cost no coding; trimming them is for a tuning run |

The cleanup of C12-C14, D15-D16 and the rest leaves the optimized binary
at 804752 bytes, from 896400 (gcc 13.3, the Makefile's flags). Inlining
changed (M2) and some code went, and the output did not change.

## Mechanical checks

- **Compiler warnings, before:**
  - gcc: 17. Shadowed locals in `model.inc` (an `lb`, and an `i64 v` that
    hid `coef()`'s parameter `v`, the value coded). Two in Lib3.
  - clang: 461. Mostly `-Wcomma`. Also `mx` (a mixer) shadowed by a
    parameter in `fam.inc`, null-pointer subtraction in Lib3's file API,
    and MSVC pragmas clang does not know.
- **Warnings, after:** none from either compiler (`-Wcomma` aside, a style
  choice).
- **clang-tidy:** 14 reports, all false positives on reading:
  - `strcpy` after its length check;
  - `(int)(v + 0.5)` of a positive `v`;
  - a `switch` with its default after it;
  - `memcmp` used as a truth value;
  - tables of small signed chars;
  - padding;
  - a signal handler's linkage.
- **Float-built tables.** The squash table (`exp`), `kScale` (`pow`) and
  `kGeoPrior` (`sqrt`) are built when the program starts, on both sides.
  Every entry is at least 6.1e-9 (relative) from a rounding boundary, so
  any libm within millions of ulp builds the same tables. The plain-bytes
  decision (`random()`, `log2`) is the compressor's alone, and coded.
- **The default order's spectra go 4 values at a time.** Every band offset
  of every table (1024, 960, 512, 480, 128 and 120 samples) is a multiple
  of 4.

## Checked and found sound

- **Desync:** every `ENC` branch only supplies the value coded, never a
  later context. Every `ChanData` field the model reads is set earlier in
  the same pass; the compressor's walk holds the unit's arrays from the
  write-back, the decompressor's the last unit's.
- **History reads:** the last frame's are guarded by window kind,
  `num_swb`, `num_groups` and `max_sfb`. The fixed array indices of the
  walk are in range.
- **Counts the decoder decodes:** each one, written by the walk, is
  `cnt()`-bounded or checked against the writer's cap.
- **cbsel's simulations:**
  - fdk: no miss on 13 fdk-aac streams;
  - faac: no miss on 4 faac streams;
  - ffmpeg: up to 0.9% of bands on ffmpeg's;
  - checked line by line against fdk's `dyn_bits`, ffmpeg's
    `codebook_trellis_rate` and faac's `BOOKMIN`.
- **The default order's value log:**
  - the spectra in 4-value steps stay in step with the parse's 2- and
    4-value tuples;
  - `cs_read()` takes exactly the section values;
  - `cs_write()` checks a decoded channel's groups, codebooks, lengths and
    kinds.
- **Mixers:** the AVX2 sums are the plain C++ sums exactly (6M trials at
  the extremes). The analytic bounds hold for up to 32 inputs and |x| <=
  2048.
- **Counters:** the vector path is the plain path, bytes and tables alike.
- **Range coder:** carries are right, and the decoder reads exactly the
  bytes written (60 sequences of up to 2M decisions). Frequencies stay in
  [8, 32760].
- **Indexing:** every Volume of `-9` is under 2^24, so no table is hashed
  in the shipping build.
- **Tuning tools:**
  - `mk.sh tuning 9`: 4981 tunable bits in 550 patterns;
  - famtune's replay gives exactly `-v`'s bytes per family;
  - `ftune.py` end to end: unt 20.5 -> 17.5 bytes, as `-v` says after the
    import.
- **Containers:**
  - the window and the CRC across the probe and the main scan;
  - frame sync at the window's edge;
  - streams past 4 GB: 4.4 GB of random bytes and then ADTS frames
    round-trip through `c - - | d - -`, at about 50 MB of memory for `c`
    and 42 MB for `d`.
- **Syntax:** reading and writing are symmetric for sections, fill
  elements, SBR and PS extensions, DSE and PCE alignment, ADTS's fixed
  header and CRC flags, LATM's other data, and ER's rest and rollback.

## Verification of the fixes

The fixes of the format-neutral findings (all but B3-B6 and A5) keep
format version 7, and were checked against 82eb6a44:

- **The same output.** The measurement corpus has 115 streams: the 22 of
  the dev set, 25 encodes of the music excerpts and FATE's 39. The other 29
  are variants in other containers: ADIF, AUS1, LOAS/LATM v0 and v1 with
  subframes and other data, ER AAC LD and ELD, and raw access units. On
  each, in both orders, the fixed binary writes the bytes 82eb6a44's
  writes, and its `d` gives back the input: 230 of 230.
- **Other builds:**
  - clang `-O2`, clang `-O2 -fno-omit-frame-pointer`, and gcc and clang
    at `-O0` write the same bytes as gcc `-O2` on the corpus, in both
    orders.
  - Neither compiler warns, with the flags of the mechanical checks.
- **`make test`** passes:
  - the 22 streams round-trip in both orders, to 77.06% of their size
    and to 77.35% with `-b`;
  - `check.sh`'s 13 cases pass;
  - `gen.sh` makes the same corpus as before.
- **`mk.sh check 9`:** the tuning, shipping and plain C++ builds code the
  22 streams to the same bytes in both orders (44 of 44).
- **ASan and UBSan** (gcc `-O1`):
  - the 22 streams round-trip in both orders with no report;
  - `fuzz.py`, 300 cases on them (seed 11): none failed, and no unit
    was stored as its bytes for not coming out as it was.
- **Tuning tools:** famtune's replay of `ff_mono64.aac`'s dumps gives
  `-v`'s bytes for every family: spc 32889, sgn 11782, scf 6161, sec 762,
  esc 1211, hdr 502 and unt 10.
- **Speed and memory**, on a 2 MB stream (ffmpeg, 256 kbit/s), the best
  of two runs each:
  - `c` takes 5.93 s, against 5.97 s before;
  - `d` takes 5.71 s, against 6.02 s;
  - at most 40 MB of memory is used, as before.
