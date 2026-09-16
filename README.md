# Bytewise coding of a bitwise model, by journaled speculative execution

Three experimental compressors that code a whole **byte** per arithmetic-coder
step, using a model that predicts one **bit** at a time. The 256-symbol
distribution is obtained by running the model step speculatively down the prefix
tree and rolling its memory writes back through a journal.

| Program | Model | Where the journaling comes from | `book1` (768 771 bytes) |
| --- | --- | --- | ---: |
| `fpaq0mw` | order-0: two counters and a mixer per partial-byte context | the assembly, rewritten by `track.pl` | 446 555 |
| `tangelo_w` | Tangelo: a paq/lpaq-class model, ~361 MB of state | the assembly, rewritten by `track.pl` | 197 078 |
| `tangelo_s` | the same Tangelo model | the model's own source | 197 078 |

All three share every line of the machinery - the walk, the journal, the range
coder and the driver. `ByteModel` (`bytemodel.inc`) is where a bitwise model
becomes a byte distribution, and it is the same interface an optimal parser
would want: `Predict()` then either code the byte or read the code length of
every byte that could have come next.

`tangelo_w` and `tangelo_s` are the same model journaled two different ways, and
they produce the same compressed file byte for byte. `track.pl` compiles the
model step to assembly and inserts a journaling call in front of every store it
finds; `tangelo_s` instead has every write to model state spelled `W(x) = ...`
in the model's own source (`write.inc`). The first cannot miss a store and needs
perl, a second compile of the step and a list of refusals; the second is one
ordinary compile of one translation unit, is 38 % faster, and can be wrong in a
way only `-DTRACK_VERIFY` will tell you about. Section 5.6 of
[ALGORITHM.md](ALGORITHM.md) is the comparison.

[ALGORITHM.md](ALGORITHM.md) describes how it works; [SPEED.md](SPEED.md) is
where the time goes and what has been done about it.

## Building

Requirements: a C++ compiler, plus - for the two `track.pl` programs only - an
x86-64 target whose compiler emits GNU assembler syntax (GCC or Clang; on Windows
MinGW-w64 GCC or Clang, e.g. from MSYS2 or w64devkit) and `perl` (on Windows:
MSYS2, Strawberry Perl or the one shipped with Git for Windows). `tangelo_s`
needs none of that.

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

`tangelo_s` is one ordinary compile:

```sh
$CXX $CXXFLAGS tangelo_s.cpp -o tangelo_s
```

The other two are built in the same three steps as the original `g.bat`:

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
tangelo_s c input output
tangelo_s d input output
```

Both Tangelo builds need about 380 MB of memory.

## Testing

`test.sh` / `test.bat` (ports of the original `t.bat`) compress and decompress a
file with `timetest` and append the timings and the compressed size to
`log.txt`, the experiment log:

```sh
./test.sh fpaq0mw   ../book1     # defaults: fpaq0mw, ../book1 (Calgary corpus)
./test.sh tangelo_w ../book1
./test.sh tangelo_s ../book1
perl log.pl                      # tabulate log.txt into log1.txt
```

Nothing is written to `log.txt` unless the round trip verified. `book1` is not
in the repository: it is the Calgary corpus file, 768 771 bytes, from
`https://corpus.canterbury.ac.nz/resources/calgary.tar.gz` (md5
`0a0fdbaf0589c9713bde9120cbb20199`). The scripts default to `../book1`, where
the author's log keeps it.

Expected for `book1`: 446 555 bytes from `fpaq0mw` and 197 078 from both
`tangelo_w` and `tangelo_s`. GCC and Clang on Linux and MinGW-w64 GCC produce
byte-identical compressed output, and so do the two Tangelo builds as against
each other - which is the main check that the hand-marked writes in `tangelo_s`
are complete.

`PRUNE_LOG` (§4.3) trades compression for time: `-DPRUNE_LOG=0x90000` gives up
0.3 % of `tangelo_w`'s compression for about a quarter of its time. `TRACKFLAGS`
passes options to the instrumenter, of which `--inline` is 5 % faster for 48 %
more assembly. SPEED.md measures both, and the four changes that were worth
taking by default.

`-DTRACK_VERIFY=n` builds a self-checking binary, which is what to reach for when
porting a new model. It checks the three things a round trip cannot, because
encoder and decoder make the same mistake and still agree:

* **that no write escapes the journal** - for `n` input bytes it snapshots every
  byte of model state, runs the walk, and compares. Thorough and expensive, so
  `-DTRACK_VERIFY_EVERY=k` samples every `k`-th byte after the first `n` rather
  than paying for all of them;
* **that no write lands outside the state the model declared** - a journal cell
  holds a 30-bit offset into one window of memory rather than a pointer
  (SPEED.md §3.5), so a store outside that window would be rolled back to the
  wrong address. Every cell's offset is checked against the declared state;
* **that the simulated step matches the real one** - for every byte of the file,
  the walk's predicted code length for the symbol actually coded must equal the
  sum of the eight real per-bit code lengths.

The window comes from `TRACK_STATE_RANGES`, where a program names any model state
outside the `Coder` object (`tangelo_w.cpp` names `y`, `bpos` and `rnd`); getting
that list wrong is what the second check is there to catch.

```sh
CXXFLAGS="-O3 -march=native -DTRACK_VERIFY=64 -DTRACK_VERIFY_EVERY=5000"   ./build.sh tangelo_w
./tangelo_w c book1 /tmp/out      # says nothing and exits 0 if the port is sound
```

Both programs pass, on GCC, Clang and MinGW-w64 alike. That command is the real
one: 218 comparisons of all 360.8 MB of model state, spread across `book1`, plus
the prediction check on every one of its 768 771 bytes.

Two knobs help reach paths a short run misses. `-DMEM=<bytes>` shrinks the model,
which is how the hash table's eviction branch gets exercised at all - at full
size nothing is evicted early. And deleting one of the 146 journaling calls from
`coder1-tangelo_w.s` by hand is worth doing once, to watch the check catch it.

## Layout

| | |
| --- | --- |
| `main.inc`, `bytemodel.inc`, `track.inc`, `log2lut.inc`, `sh_v1m.inc` | the shared machinery |
| `track.pl` | journaling by rewriting the assembly |
| `write.inc` | journaling from the model's own source (`W(x)`) |
| `fpaq0mw.cpp`, `model.inc`, `sh_mixer.inc`, `coder.inc` | the small model |
| `tangelo_w.cpp`, `tangelo_s.cpp`, `tangelo/*.inc`, `coder_tangelo.inc` | the Tangelo model, both ways |
| `tangelo_bm.inc` | the model packaged for a host program with its own coder |
| `tf003/` | a transformer-based coder with the Tangelo model in place of its PPMD |
| `build.sh`, `build.bat` | build scripts |
| `test.sh`, `test.bat`, `timetest.cpp` | round-trip / timing test |
| `log.txt`, `log1.txt`, `log.pl` | experiment log |
| `ALGORITHM.md`, `SPEED.md` | how it works, and where its time goes |
| `legacy/` | the original Windows-only scripts and `tangelo_orig.cpp`, unchanged |

## Using the model from another program

The three programs above are their own compressors. A host that already has an
arithmetic coder and just wants `P(next byte)` - which is what a PPM or PPMD
model hands a mixer - includes `tangelo_bm.inc` instead:

```cpp
tangelo::ByteSource bs;
bs.Init( cmap );          // cmap[256], nonzero where the byte can occur
bs.Predict( probs );      // P(next byte) into 256 floats, summing to 1
... the host codes a byte c ...
bs.Update( c );           // advance the model over c's eight bits
```

Everything is inside `namespace tangelo`, because such a host has its own
`SCALE` and its own `Rangecoder`. Only `tangelo_s`'s route can be embedded this
way: the model marks its own writes (`write.inc`), so there is no second compile
of the model step, no instrumented assembly and no perl in the host's build.
`-DTRACK_VERIFY` works there too, and is the thing to reach for first.

`tf003/` is a worked example - the Tangelo model replacing the PPMD one that
feeds a transformer's prior and a binary mixer. It is 7-11 % smaller than PPMD
as a context model on its own, and 1.6-4.6 % smaller once the transformer is in
the mix; `tf003/README.md` has the numbers and the caveats.

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
use; `track8/16/32/64` stubs; an 8-byte journal cell holding an offset rather
than a pointer; `-mno-red-zone -fno-builtin` on the instrumented compile; a
`PRUNE_LOG` knob; a journal-capacity check in `NEST()`; a runtime check that the
walk really is journaling; and the `-DTRACK_VERIFY` self-check.
