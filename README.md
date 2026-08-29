# RC016 -- a SIMD binary rangecoder

An RCNUM-lane binary rangecoder driven by an order-0 FSM counter model.
The lanes are not intrinsics and not a library: `rc.inc` is one scalar
rangecoder, `mk_kernel.sh` rewrites it into a lane-array shape whose state is
`[RCNUM]` arrays and whose methods are macros, and the encoder's sweep over
those arrays is what clang auto-vectorizes. The scalar coder stays compiled
beside it as the reference and as the carry-propagating fallback, so both
sides of the comparison come out of the same source file.

Bit `i` of a block goes to lane `i%RCNUM`, so the lanes are independent; a
block is `RCNUM` substreams behind a table of 2-byte lengths.

    016-no_clsize/    the coder
      coder.cpp         CLI: encode, decode, and the encode+decode test mode
      rc.inc            the scalar rangecoder, included twice (carryless twin)
      rc_config.inc     every RC_* knob, each one documented where it is defined
      rc_io.inc         substream buffers and the block format
      rc_vec.inc        RCNUM scalar coders behind the lane interface
      model0.inc        encoder: model pass, then the vector coding pass
      model1.inc        decoder
      mk_kernel.sh      rc.inc -> rc_vecD.inc, the generated lane-array coder
      build.sh, t.sh    Linux build and test
      gc.bat, t.bat     the Windows equivalents
      misc/bench/       shape benchmarks and prototype patches, see its README
    Lib3/             shared headers: the coroutine, the file API, the perl
                      passes mk_kernel.sh runs
    FSM0.txt          the counter state machine, loaded by both ends
    get_enwik8.sh     fetch the benchmark file

## Build and test on Linux

Needs clang, perl, and a POSIX shell.

    ./get_enwik8.sh          # ~36 MB down, 100 MB unpacked, into ./enwik8
    cd 016-no_clsize
    ./build.sh               # -> ./coder
    ./t.sh                   # roundtrip + stream check + timings

`t.sh` builds, then encodes `../enwik8` ten times and decodes it once (the
coder's own test mode, which appends the best timings to `log.txt`), checks
the decoded file against the input, and re-encodes with `-C` -- the scalar
reference coder -- to confirm the vector coder produced the same stream byte
for byte. A roundtrip alone would not catch a vector coder that emits a
*different* valid stream, which is why the second check is there.

    == ../enwik8 (100000000 bytes), 10 encode passes, FSM ../FSM0.txt
    enc  76.02MB/s 1.254s   #10
    dec  36.05MB/s 2.644s   #1
    == roundtrip
       ok: decoded output matches the input
    == vector stream vs the -C scalar reference
       ok: byte-identical
    == 62513092 bytes, 5.0010 bpc

## Configuration

Every knob is a `-D` in `rc_config.inc`. The same `-D` set has to reach both
the kernel generator and the compile -- `rc_vecD.inc` is `rc.inc` preprocessed
against it -- so pass them to `build.sh`, or to `t.sh`, which forwards them:

    ./build.sh -DRC_RCNUM=32
    ./t.sh -DRC_RCNUM=32 -DRC_LOWBYTES=4
    ./t.sh "-DRC_RCNUM=32 -DRC_LOWBYTES=4"      # one argument works too

Nothing generated is committed, and `build.sh` regenerates it every time, so
switching configurations needs no clean. The generated kernel depends on the
`-D` set and on nothing else -- not on the target, not on which compiler
preprocesses it -- so any build script can generate it with whatever is at
hand; `gc.bat` uses its own `%defs%` and compiler. A `-D` set that does not
match between the generation and the compile is a build error, not a silently
wrong coder: `rc_config.inc`'s `RC_KERNEL_CONF` is baked into the kernel and
checked against the build.

`RC_SCATTER` is the one knob that keys off the target rather than the `-D` set.
The coder's one hot store goes to a per-lane address, and the sweep is straight-
line code, so SLP -- which is what vectorizes it -- emits an address extract
plus a scalar store per lane instead of a scatter, which it never forms. With
`RC_SCATTER` on, `ShiftLow` stages the store in `stcl[]`/`stad[]` (contiguous
array stores, which SLP does vectorize) and the model commits a whole group
with one `vpscatterdd`. It turns on by itself for clang + AVX-512 at
`RCNUM%16==0`, and is off everywhere else, where staging would only cost.
The stream does not change either way -- `t.sh` checks that.

`misc/speed_plan.md` is what to try next, ranked: it starts from the fact that
decode costs 1.86x what encode does and has never been profiled, and from the
fact that `FSM0.txt` has 256 live states -- so the model's whole working set is
2 KB, which retires the cache-footprint explanation two of the older negative
results were written against.

`build.sh` takes `CXX`, so a different toolchain is one variable away. Newer is
not faster here: clang 23.1.0 builds a byte-identical stream 7.4% slower than
clang 18.1.3 on AVX2 and 6.1% slower on AVX-512, from a sweep loop that is two
instructions *shorter*. `misc/avx2_profile.md` section 9d has the numbers and
the localization.

`misc/avx2_profile.md` is the measured breakdown of where encode time goes on
the AVX2 target -- the model pass against the rename ceiling, the sweep against
rename and the ALU ports at once, what the store costs and why staging it wins
there too, and the things that did not work (prefetch, chunking, skip branches,
fusing the tables). `misc/model0_process_skl.asm` is that target's
disassembly of `Model<0>::do_process`, annotated block by block: what each one
does, what the hot loop costs instruction by instruction, and what is left to
try. `misc/clk.c` measures the core clock the figures are against, which is
not the one `/proc/cpuinfo` reports.

`RC_FOLD_RPRE` came out of reading that listing: `rc_Process` leaves `rpre`
pending for the next `ShiftLow` to add to `low`, and applying it where it is
produced instead would take `rpre` out of the sweep's loop-carried set -- two
ymm out of a working set that does not fit. It is stream-neutral, it shortens
the loop by six instructions and two stores, and it is 2.55% slower. Default
0, kept for the measurement; `rc_config.inc` has it.

`RC_SCATTER_SKIP` sits on top of it: a lane whose renorm emitted nothing owes
no store, and only about 1.3 of 16 lanes owe one, so the scatter can be masked
down to them or branched past entirely. Both are correct and both lose on
cascadelake -- the comment in `rc_config.inc` has the measurements and says
which machines they should win on.

| `build.sh`                | |
|---|---|
| `ARCH=skylake-avx512`     | the `-march`/`-mtune` target (default `native`, probed -- an unsupported one drops out with a warning instead of failing the build) |
| `CXX=g++`                 | gcc builds correctly and produces byte-identical streams, but does not vectorize the lane sweep: about half the encode speed |
| `LTO=0`                   | skip `-flto`/`lld` (used when both are available; a failed LTO link falls back on its own) |
| `OPT='-O3 -mprefer-vector-width=512'` plus `-DRC_SCATTER_W=16` | the wide scatter, if you want to measure it -- slower here |
| `STATIC=1`, `OUT=`, `STD=`, `OPT=` | link static, name the binary, pick the standard and the optimization level |

| `t.sh`                    | |
|---|---|
| `TESTFILE=../book1`       | what to code (default `../enwik8`) |
| `NITER=3`                 | encode passes to time (default 10) |
| `FSM=../FSM1.txt`         | the counter state machine (default `../FSM0.txt`) |
| `KEEP=1`                  | leave `t.enc` / `t.dec` / `t.ref` behind |

`RC_VEC=0` and `RC_RANGE64=1` both leave the models on the scalar coder;
`build.sh` notices and skips the kernel generation rather than tripping
`rc_config.inc`'s `#error`.

## Running the coder directly

    coder [-C] c|d input output FSM_file [n_iter] [test_output]

    coder c ../enwik8 1 ../FSM0.txt            encode
    coder d 1 2 ../FSM0.txt                    decode
    coder c ../enwik8 1 ../FSM0.txt 10 2       encode x10, decode, log.txt
    coder -C c ../enwik8 1 ../FSM0.txt         the scalar reference coder

`log.txt` collects one line per test-mode run: encode speed and time, decode
speed and time, output size, then the directory, `RCNUM` and `RC_LOWBYTES`.
`t.sh` appends the compiler and the ISA the build actually got, which is the
`avx512` / `avx2` column the earlier lines carry.
