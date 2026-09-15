# fpaq0mw

An experimental order-0 compressor whose bitwise model is turned into a
256-symbol distribution for every byte by running the compiled model step
speculatively and rolling its memory writes back through a journal. The
journaling calls are injected into the compiler's assembly output at build
time.

See [ALGORITHM.md](ALGORITHM.md) for a full description of the model, the
tree walk, the journal and the range coder.

## Building

Requirements: an x86-64 C++ compiler that emits GNU assembler syntax (GCC or
Clang; on Windows MinGW-w64 GCC or Clang, e.g. from MSYS2 or w64devkit) and
`perl` (on Windows: MSYS2, Strawberry Perl or the one shipped with Git for
Windows).

Linux (or MSYS2 bash on Windows):

```sh
./build.sh                    # g++, produces ./fpaq0mw
CXX=clang++ ./build.sh
CXXFLAGS="-O3 -ffast-math -march=x86-64-v3" ./build.sh
```

Windows, plain `cmd`:

```bat
build.bat                     REM g++ from PATH, produces fpaq0mw.exe
set CXX=clang++ & build.bat
```

Cross-building the Windows binary from Linux also works:

```sh
CXX=x86_64-w64-mingw32-g++ LDFLAGS="-static -s" EXE=fpaq0mw.exe ./build.sh
```

Both scripts run the same three steps as the original `g.bat`:

1. `CXX -S -masm=intel -DSIM_FUNC fpaq0mw.cpp -o coder.s` — the model step
   (`encode_sim`) as Intel-syntax assembly;
2. `perl track.pl coder.s coder1.s` — insert a journaling call in front of
   every store (the ABI, `rcx` vs `rdi`, is detected from the assembly);
3. `CXX fpaq0mw.cpp coder1.s -o fpaq0mw` — the program plus the instrumented
   step.

`track.pl` fails the build if the assembly contains a memory write it does
not know how to journal (see `perl track.pl` header, or `-v` for a listing).

## Usage

```
fpaq0mw c input output      compress
fpaq0mw d input output      decompress
```

## Testing

`test.sh` / `test.bat` (port of the original `t.bat`) compress and decompress
a file with `timetest` and append the timings and the compressed size to
`log.txt`, the experiment log:

```sh
./test.sh ../book1            # default input: ../book1 (Calgary corpus)
perl log.pl                   # tabulate log.txt into log1.txt
```

Expected result for `book1` (768 771 bytes) with this snapshot: 446 962 bytes,
the same number the author recorded for iteration `024` on Windows. GCC and
Clang on Linux and MinGW-w64 GCC produce byte-identical compressed output.

## Layout

| | |
| --- | --- |
| `fpaq0mw.cpp`, `*.inc` | sources (see ALGORITHM.md §2) |
| `track.pl` | assembly instrumentation (replaces `1.pl` / `1a.pl`) |
| `build.sh`, `build.bat` | build scripts |
| `test.sh`, `test.bat`, `timetest.cpp` | round-trip / timing test |
| `log.txt`, `log1.txt`, `log.pl` | experiment log |
| `legacy/` | the original Windows-only scripts, unchanged |

## Porting notes

Changes made to the original `024-track_inc` sources for the port:

* `track.inc`: the `track1/2/4` journaling stubs are now assembly (plus a new
  `track8` for 8-byte stores) that preserves all registers and EFLAGS, for
  both the Win64 and the SysV calling convention. The C versions relied on
  `__attribute__((no_caller_saved_registers))`, which does not preserve
  EFLAGS, so the fast `1a.pl` variant (no `pushf`/`popf`) was only correct by
  luck of instruction scheduling. `__stdcall` is gone (meaningless on x64).
  `trk` and `trkptr` have C linkage so the stubs can name them.
* `fpaq0mw.cpp`: `__max`/`__min` fallbacks for C libraries that do not define
  them (they are MSVC/MinGW CRT macros).
* No change to the model, the coder or the tree walk.
