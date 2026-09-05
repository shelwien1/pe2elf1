# pjpg — JPEG structure parser

`pjpg` walks a JPEG file byte by byte and prints the marker structure: APP0/APP1
(JFIF, JFXX, Exif including the embedded thumbnail IFDs), DQT, DHT, SOF0/1/2/9/10,
DRI and SOS. It is a *parser*, not a decoder — it reads and reports, it does not
produce an image.

The interesting part is *how* it parses. `pjpg0.inc` is written as if the input
were a stream it can pull bytes from at will, and two layers of machinery make
that work over a fixed-size input buffer:

* `coro_fsm.pl` rewrites every `coro2_get()` call site into a computed-goto
  resume point (`state=&&m17; return; m17: ...`), turning the per-marker parsers
  into resumable state machines.
* `Lib3/coro3b.inc` is a stackful coroutine built from `setjmp`/`longjmp` plus a
  hand-rolled `memcpy` of the live stack frame into a member array, so the byte
  loop in `pjpg1.inc` can suspend when the input buffer runs dry and resume when
  the frontend refills it.

Both are unusual enough that most of the porting effort went into confirming they
still behave on Linux/ELF. They do.

A third wrinkle: a JPEG can carry a JPEG thumbnail, so `pjpg.cpp` holds
`PjpgLevels` complete parser instances and hands an embedded thumbnail to the
next one. A thumbnail is not a special case — it is parsed by a plain `pjpg`,
which can recurse again in turn.

[**docs/pjpg-algorithm.md**](docs/pjpg-algorithm.md) describes the parsing
algorithm in detail and catalogues the ten bugs that were found in it — all now
fixed, including a remotely-triggerable SIGSEGV from a 69-byte crafted SOS
marker and a 3824-byte buffer overflow from a crafted DHT.
`docs/make-repros.py` regenerates the crafted JPEGs used as reproducers, and
`make test` runs them.

## Building

### Linux

```sh
cd 011_
make                    # -> ./pjpg
make test               # regression against the Windows-captured reference output
./pjpg ../testfiles/drazen1.jpg
```

`make` needs a C++17 compiler (gcc or clang) and `perl` for the codegen step.

Knobs:

| | |
|---|---|
| `make CXX=clang++` | build with clang instead of gcc |
| `make OPT=-Ofast` | optimisation level (default `-O2`) |
| `make CORO=libc` | use the portable `<setjmp.h>` coroutine backend |
| `make NATIVE=1` | add `-march=native` |
| `make LTO=1` / `STATIC=1` | link-time optimisation / static link |
| `make SAN=undefined` | build under UndefinedBehaviorSanitizer |

`011_/gc.sh` and `011_/t.sh` are one-shot script equivalents of `make` and
`make test`, mirroring the original `gc.bat` / `t.bat`.

### Windows

Unchanged. `011_/gc.bat` still drives the clang + MSVC-runtime build and
`011_/t.bat` still runs the two reference files. Neither was modified.

## Testing

| target | what it does |
|---|---|
| `make test` | Three assertions: no line that the Windows `011_/log1` printed has gone missing (pjpg now parses markers the original skipped, so its output is a superset); all 20 checked-in jpegs match `011_/tests/golden.log`; and all 15 crafted malformed inputs are *reported* rather than crashing. |
| `make corpus` | Runs all 118 bundled jpegs (`testfiles/` plus the 98 in `imagetestsuite-jpg-1.00.tar.gz`). Exit 1 (a reported parse error) is expected — imagetestsuite is a deliberately-damaged corpus — but a crash or hang fails the target. |
| `make crosscheck` | Builds with gcc *and* clang and confirms they produce byte-identical output on every corpus file. Disagreement between two correct compilers is the signature of undefined behaviour, which is worth watching for in code that hand-switches stacks and type-puns. |
| `make matrix` | Builds and runs 26 configurations: gcc and clang × `-O0`…`-Ofast`, both coroutine backends, LTO, static, `-march=native`, PIE and no-PIE, and the sanitizers. |
| `make golden` | Regenerates `tests/golden.log` after an intentional output change. |

Current status on Ubuntu 24.04 (gcc 13.3, clang 18.1.3, x86-64):

* Output is a **strict superset of the Windows reference**: 8 lines added by the
  marker handlers the original did not have, none lost.
* 118 corpus files: 0 crashes, 0 hangs. 57 parse clean; the 61 that report errors
  are the damaged imagetestsuite images, and the truncation verdict was
  cross-checked against an independent marker walker on all 118.
* 4000 mutated inputs and all 15 crafted malformed files: 0 crashes, 0 UBSan
  reports.
* gcc and clang agree byte-for-byte on all 118.
* All matrix cells pass, with the two exceptions noted below.
* Thumbnails are parsed recursively, up to 5 levels deep.
* Entropy-coded scans are decoded — Huffman and arithmetic, sequential and
  progressive — or skipped with `-s`. See [§10 of the algorithm doc](docs/pjpg-algorithm.md).
* Throughput: **61 MB/s** decoding entropy data, **2.9 GB/s** with `-s`
  (structure only) (was 378 MB/s before the byte loop
  was rewritten around `memchr` and bulk skipping).

## Exit status

```
0   parsed to the end of the file (warnings are allowed)
1   parsed, but at least one structural error was reported
2   could not open the file, or no argument was given
```

A malformed field is reported and its segment abandoned, but parsing continues
from the next marker — the declared length still says where that is — so one run
reports everything wrong with a file. Losing the stream position (input ending
inside a segment) is fatal. Either way the coroutine finishes with an error
yield rather than a quit.

## Porting notes

### Flags dropped from `gc.bat`

`gc.bat` is a Windows clang command line targeting the MSVC runtime. The Linux
build keeps only what is meaningful here:

* **Kept**: `-DNDEBUG`, `-Drestrict=__restrict` (`restrict` is not a C++ keyword),
  `-fomit-frame-pointer`, `-fno-stack-protector`, `-fno-stack-check`,
  `-fstrict-aliasing`, and optionally `-march=…`, `-flto`, `-static`.
* **Dropped as Windows-only**: `-DSTRICT`, `-DWIN32`, `-D_WIN32`,
  `-DWIN32_LEAN_AND_MEAN`, every `_CRT_*` / `_SECURE_SCL*` /
  `_HAS_ITERATOR_DEBUGGING` / `_ITERATOR_DEBUG_LEVEL` define, `-nostdlibinc`,
  `-nostdinc++`, `-fms-compatibility`, `-fms-extensions`, `-Wmsvc-not-found`,
  the `-isystem` paths into `C:\VC2019` and friends, and `-DCOMMON_SKIP_BSF`
  (it guards an MSVC-only `_BitScanForward` declaration in `common.inc`).
  Defining `_WIN32` on Linux would be actively wrong.
* **Dropped as unused**: `-D__DIRNAM__=%DIRNAM%` — no source file references it.

### The coroutine on Linux

`Lib3/coro3b.inc` picks a `setjmp` implementation at compile time:

| build | backend | header |
|---|---|---|
| gcc | `__builtin_setjmp` / `__builtin_longjmp` | `coro3_setjmp_x64.h` |
| clang | hand-written x86-64 inline asm | `coro3_setjmp_x64d.h` |
| `-DCORO_NOASM=1` | glibc `<setjmp.h>` | — |

The `CORO_NOASM` fallback is the only backend that is not x86-specific, so it
matters for any future non-x86 Linux port — but on glibc it needs fortification
turned off. glibc redirects `longjmp` to `__longjmp_chk` when `_FORTIFY_SOURCE`
is set (Ubuntu's gcc defaults it on at `-O1`+), and that check aborts with

```
*** longjmp causes uninitialized stack frame ***
```

precisely because this coroutine *does* jump into a restored stack. `make
CORO=libc` therefore adds `-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0`, after which
it produces identical output.

### AddressSanitizer does not work here

ASan instruments and poisons stack frames; `Coroutine::yield` `memcpy`s a live
stack frame out to `Coroutine::stk[]` and `coro_call0` copies it back. ASan
cannot follow that and aborts. This is inherent to the design, not a bug to fix,
so `make matrix` records it as an expected failure rather than hiding it. UBSan
works and passes.

### Line endings

Much of the tree, the reference `log1`, and the perl-generated `pjpg0j.inc` are
stored with CRLF. `.gitattributes` sets `* -text` so git never translates them,
because the byte-for-byte comparison against `log1` depends on those bytes. Test
comparisons strip CR before diffing, so the same golden file works on both
platforms.

### Generated files

`pjpg0i.inc` (preprocessed) and `pjpg0j.inc` (perl-rewritten) are build
artifacts — `gc.bat` deletes and regenerates them on every build — so they are
gitignored here too and `make` regenerates them. The codegen is deterministic:
Linux clang reproduces the Windows-generated `pjpg0j.inc` byte for byte, and
gcc's differs only by four blank lines from a preprocessor formatting difference.
