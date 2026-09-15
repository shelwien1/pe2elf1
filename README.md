# Bytewise coding of a bitwise model, by journaled speculative execution

Two experimental compressors that code a whole **byte** per arithmetic-coder
step, using a model that predicts one **bit** at a time. The 256-symbol
distribution is obtained by running the compiled model step speculatively down
the prefix tree and rolling its memory writes back through a journal; the
journaling calls are injected into the compiler's assembly output at build time.

| Program | Model | `book1` (768 771 bytes) |
| --- | --- | ---: |
| `fpaq0mw` | order-0: two counters and a mixer per partial-byte context | 446 962 |
| `tangelo_w` | Tangelo: a paq/lpaq-class model, ~361 MB of state | 197 483 |

Both share every line of the machinery - the walk, the journal, the range coder
and the driver - and differ only in the model.
[ALGORITHM.md](ALGORITHM.md) describes all of it.

## Building

Requirements: an x86-64 C++ compiler that emits GNU assembler syntax (GCC or
Clang; on Windows MinGW-w64 GCC or Clang, e.g. from MSYS2 or w64devkit) and
`perl` (on Windows: MSYS2, Strawberry Perl or the one shipped with Git for
Windows).

Linux (or MSYS2 bash on Windows):

```sh
./build.sh                    # both programs
./build.sh tangelo_w          # just one
CXX=clang++ ./build.sh
CXXFLAGS="-O3 -ffast-math -march=x86-64-v3" ./build.sh
```

Windows, plain `cmd`:

```bat
build.bat                     REM g++ from PATH
set CXX=clang++ & build.bat
```

Cross-building the Windows binaries from Linux also works:

```sh
CXX=x86_64-w64-mingw32-g++ EXT=.exe LDFLAGS="-static -s" ./build.sh
```

Each program is built in the same three steps as the original `g.bat`:

1. `CXX $CXXFLAGS -mno-red-zone -fno-builtin -S -masm=intel -DSIM_FUNC <prog>.cpp
   -o coder-<prog>.s` - the model step (`encode_sim`) as Intel-syntax assembly.
   The two extra flags are why it works: the journaling call inserted in step 2
   writes just below `rsp`, and `-fno-builtin` stops the compiler turning a loop
   into a library call whose writes the journal could never see;
2. `perl track.pl coder-<prog>.s coder1-<prog>.s` - insert a journaling call in
   front of every memory write (the ABI, `rcx` vs `rdi`, is detected from the
   assembly);
3. `CXX <prog>.cpp coder1-<prog>.s -o <prog>` - the program plus the
   instrumented step.

`track.pl` fails the build rather than guess, if the assembly contains a memory
write it cannot journal, a call whose target is not in the same file (a library
call's writes never reach the journal), an exported symbol besides the one entry
point (the linker would be free to discard the instrumented copy), a static
initialiser, or use of the red zone. Run
`perl track.pl -v coder-tangelo_w.s /dev/null` after a build to see every write
it instruments or skips; its header comment documents the options and every
refusal.

## Usage

```
fpaq0mw   c input output      compress
fpaq0mw   d input output      decompress
tangelo_w c input output
tangelo_w d input output
```

`tangelo_w` needs about 380 MB of memory.

## Testing

`test.sh` / `test.bat` (ports of the original `t.bat`) compress and decompress a
file with `timetest` and append the timings and the compressed size to
`log.txt`, the experiment log:

```sh
./test.sh fpaq0mw   ../book1     # defaults: fpaq0mw, ../book1 (Calgary corpus)
./test.sh tangelo_w ../book1
perl log.pl                      # tabulate log.txt into log1.txt
```

Nothing is written to `log.txt` unless the round trip verified. `book1` is not
in the repository: it is the Calgary corpus file, 768 771 bytes, from
`https://corpus.canterbury.ac.nz/resources/calgary.tar.gz` (md5
`0a0fdbaf0589c9713bde9120cbb20199`). The scripts default to `../book1`, where
the author's log keeps it.

Expected for `book1`: `fpaq0mw` gives 446 962 bytes, the number the author
recorded for iteration `024` on Windows, and `tangelo_w` gives 197 483.
GCC and Clang on Linux and MinGW-w64 GCC produce byte-identical compressed
output.

The speed/size knob is `PRUNE_LOG` (§4.3): building with `-DPRUNE_LOG=0x90000`
trades 0.3 % of `tangelo_w`'s compression for 27 % of its time (20 804 bytes in
7.5 s against 20 739 in 10.2 s, on `book1`'s first 64 KB).

`-DTRACK_VERIFY=n` (optionally with `-DTRACK_VERIFY_EVERY=k`) builds a
self-checking binary, which is what to reach for when porting a new model. It proves the two halves of the contract that a round
trip cannot: that the journal restores **everything** (for the first `n` input
bytes it snapshots every byte of model state, runs the walk, and compares), and
that the simulated step reproduces the real one (for every byte, the walk's
predicted code length for the symbol actually coded must equal the sum of the
eight real per-bit code lengths). Both failures are otherwise silent - they cost
compression without ever breaking a round trip.

```sh
CXXFLAGS="-O3 -march=native -DTRACK_VERIFY=48" ./build.sh tangelo_w
./tangelo_w c book1 /tmp/out      # says nothing and exits 0 if the port is sound
```

Both programs pass on GCC, Clang and MinGW-w64 alike. `-DMEM=<bytes>` shrinks
the Tangelo model, which is how its hash-table eviction path gets exercised:
at full size nothing is evicted in a short run.

## Layout

| | |
| --- | --- |
| `main.inc`, `track.inc`, `track.pl`, `log2lut.inc`, `sh_v1m.inc` | the shared machinery |
| `fpaq0mw.cpp`, `model.inc`, `sh_mixer.inc`, `coder.inc` | the small model |
| `tangelo_w.cpp`, `tangelo/*.inc`, `coder_tangelo.inc` | the Tangelo model |
| `build.sh`, `build.bat` | build scripts |
| `test.sh`, `test.bat`, `timetest.cpp` | round-trip / timing test |
| `log.txt`, `log1.txt`, `log.pl` | experiment log |
| `legacy/` | the original Windows-only scripts and `tangelo_orig.cpp`, unchanged |

## Porting notes

**The original `024-track_inc` sources** needed two changes:

* `track.inc`: the `track1/2/4` journaling stubs are now assembly that preserves
  all registers and EFLAGS, for both the Win64 and the SysV calling convention.
  The C versions relied on `__attribute__((no_caller_saved_registers))`, which
  does not preserve EFLAGS, so the fast `1a.pl` variant (no `pushf`/`popf`) was
  only correct by luck of instruction scheduling. `__stdcall` is gone
  (meaningless on x64). `trk` and `trkptr` have C linkage so the stubs can name
  them.
* `fpaq0mw.cpp`: `__max`/`__min` fallbacks for C libraries that do not define
  them (they are MSVC/MinGW CRT macros).

**The Tangelo model** (`legacy/tangelo_orig.cpp` is the original, unchanged
except for line endings) needed the edits listed at the top of
`tangelo/tangelo.inc`, which is a mechanical diff rather than a recollection:
the shared globals declared instead of defined (§5.4); `memset()` and the
`<ctype.h>` calls spelled out, since the journal cannot see a library call's
writes, which also makes the output locale-independent; `INLINE` on the five
functions a compiler might leave out of line, so the instrumented translation
unit exports nothing the linker could substitute; and two portability fixes the
original needs merely to compile with GCC or Clang.

The model's arithmetic is untouched; so are the walk, the journal and the coder.

**The build machinery** gained: `track.pl` handling vector stores up to 64 bytes
and read-modify-write instructions, skipping stack writes from any instruction,
and refusing out-of-file calls, weak symbols, static initialisers or red-zone
use; `track8/16/32/64` stubs; `-mno-red-zone -fno-builtin` on the instrumented
compile; a `PRUNE_LOG` knob; a journal-capacity check in `NEST()`; a runtime
check that the walk really is journaling; and the `-DTRACK_VERIFY` self-check.
