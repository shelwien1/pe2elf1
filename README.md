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

1. `CXX -S -masm=intel -mno-red-zone -DSIM_FUNC <prog>.cpp -o coder-<prog>.s` -
   the model step (`encode_sim`) as Intel-syntax assembly;
2. `perl track.pl coder-<prog>.s coder1-<prog>.s` - insert a journaling call in
   front of every memory write (the ABI, `rcx` vs `rdi`, is detected from the
   assembly);
3. `CXX <prog>.cpp coder1-<prog>.s -o <prog>` - the program plus the
   instrumented step.

`track.pl` fails the build if the assembly contains a memory write it cannot
journal, a call whose target is not in the same file (a library call's writes
never reach the journal), a weak (COMDAT) symbol that would let the linker
discard the instrumentation, a static initialiser, or use of the red zone. Run `perl track.pl -v` to see
every write it instruments or skips; its header comment documents the options.

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

Expected for `book1`: `fpaq0mw` gives 446 962 bytes, the number the author
recorded for iteration `024` on Windows, and `tangelo_w` gives 197 483.
GCC and Clang on Linux and MinGW-w64 GCC produce byte-identical compressed
output.

The speed/size knob is `PRUNE_LOG` (§4.3): build with `-DPRUNE_LOG=0x90000` to
trade about 0.3 % of `tangelo_w`'s compression for 25 % of its time.

`-DTRACK_VERIFY=n` builds a self-checking binary, which is what to reach for
when porting a new model. It proves the two halves of the contract that a round
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

Both programs pass: `fpaq0mw` over all of `book1`, and `tangelo_w` with all
360.8 MB of its model state compared byte for byte.

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

**The Tangelo model** (`legacy/tangelo_orig.cpp` is the untouched original)
needed five, each commented where it was made and listed at the top of
`tangelo/tangelo.inc`:

* `common.inc`: the mutable globals are declared, not defined, so that the
  instrumented and the real copy of the step share one set (§5.4); and the
  `<ctype.h>` calls became open-coded C-locale tests, which removes a call the
  journal could not see and makes the output locale-independent.
* `CM.inc`: the `memset()` in the ContextMap's eviction path is spelled out as a
  loop, for the same reason.
* `CM.inc`, `CM_main.inc`, `coder_tangelo.inc`: `INLINE` on `ContextMap::mix`,
  `Model::predictNext` and `Coder::encode_sim`, so the instrumented translation
  unit defines no weak symbol the linker could discard.
* `mixer.inc`: `using Base::` declarations for the dependent base members, which
  GCC and Clang require and MSVC does not.
* `CM_small.inc`: a template parameter renamed so it stops shadowing `m`.

The model's arithmetic is untouched; so are the walk, the journal and the coder.

**The build machinery** gained: `track.pl` handling vector stores up to 64 bytes
and read-modify-write instructions, skipping stack writes from any instruction,
and refusing out-of-file calls, weak symbols, static initialisers or red-zone
use; `track8/16/32/64` stubs; `-mno-red-zone -fno-builtin` on the instrumented
compile; a `PRUNE_LOG` knob; a journal-capacity check in `NEST()`; a runtime
check that the walk really is journaling; and the `-DTRACK_VERIFY` self-check.
