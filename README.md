# RC016 -- a SIMD binary rangecoder, and a binary rANS in its place

An RCNUM-lane binary entropy coder driven by an order-0 FSM counter model.
The lanes are not intrinsics and not a library: `rc.inc` is one scalar
rangecoder, `mk_kernel.sh` rewrites it into a lane-array shape whose state is
`[RCNUM]` arrays and whose methods are macros, and the encoder's sweep over
those arrays is what clang auto-vectorizes. The scalar coder stays compiled
beside it as the reference and as the carry-propagating fallback, so both
sides of the comparison come out of the same source file.

`rans.inc` is a second coder in exactly that shape -- a binary rANS, through
the same three perl passes, driven by the same model through the same
`rc_Process(lane, p, bit)`. `RC_RANS` picks between them and defaults to rANS.
Both are built and tested by `t_matrix.sh`; see **The rANS coder** below for
what changes and what it costs.

Bit `i` of a block goes to lane `i%RCNUM`, so the lanes are independent; a
block is `RCNUM` substreams behind a table of 2-byte lengths.

    016-no_clsize/    the coder
      coder.cpp         CLI: encode, decode, and the encode+decode test mode
      rc.inc            the scalar rangecoder, included twice (carryless twin)
      rans.inc          the binary rANS coder, in the same shape
      rc_kernel_src.inc which of the two mk_kernel.sh preprocesses
      rc_config.inc     every RC_* knob, each one documented where it is defined
      rc_io.inc         substream buffers and the block format
      rc_vec.inc        RCNUM scalar coders behind the lane interface
      model0.inc        encoder: model pass, then the vector coding pass
      model1.inc        decoder
      mk_kernel.sh      rc.inc / rans.inc -> rc_vecD.inc, the lane-array coder
      build.sh, t.sh    Linux build and test
      t_matrix.sh       both coders across the whole -D space
      gc.bat, t.bat     the Windows equivalents
      misc/rans_div.cpp the rANS encoder's float division, proved exhaustively
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

`t.sh` builds, then encodes `../enwik8` ten times and decodes it (the coder's
own test mode, which appends the best timings to `log.txt`), and checks the
decoded file against the input. Set `REFSTREAM` and it also compares the
encoded stream against that file, recording it the first time -- a roundtrip
alone would not catch a coder that emits a *different* valid stream, and that
is the check `t_matrix.sh` builds its rows out of.

    == ../enwik8 (100000000 bytes), 6 encode / 4 decode passes, FSM ../FSM0.txt
    enc  63.45MB/s 1.503s   #5
    dec  41.77MB/s 2.283s   #2
    == roundtrip
       ok: decoded output matches the input
    == 62588528 bytes, 5.0071 bpc

## Configuration

Every knob is a `-D` in `rc_config.inc`. The same `-D` set has to reach both
the kernel generator and the compile -- `rc_vecD.inc` is `rc.inc` or `rans.inc`
preprocessed against it -- so pass them to `build.sh`, or to `t.sh`, which
forwards them:

    ./build.sh -DRC_RANS=0                      # the range coder
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

`misc/speed_plan.md` is that plan and what came of it -- all seven items
tested, every variant byte-identical to the reference. Two knobs came out of
it, both stream-neutral:

    RC_FUSE_PP_ENC / RC_FUSE_PP_DEC   the counter carries {p,state} and the
                                      FSM entry carries the next state's
                                      probability, so the model reads one
                                      table per bit instead of two, and the
                                      probability stops being a load that
                                      depends on a load.  Decode +18%.
    RC_CHUNK=2048                     the model pass and the coding sweep
                                      interleaved a chunk at a time, both
                                      working the low end of pbit[], so the
                                      buffer between them is 32 KB rather
                                      than 1 MB.  Encode +3%.

On `-march=skylake`, against the same source one round earlier: encode
72.77 -> 78.62 MB/s, decode 39.79 -> 46.86. On `-march=native`: 81.28 -> 83.02
and 40.43 -> 47.55. Compressed size unchanged everywhere -- 62,513,092 bytes
for enwik8, as before.

`t.sh` checks one build; `t_matrix.sh` checks that the `-D` space still holds
together -- both coders, every knob at its non-default setting, the geometries
(`RC_RCNUM=32`, `RC_BLKSIZE`, `CORO_FAKE=1`), and `RC_VEC=0`, which puts the
coding pass on `rc_vec.inc`'s scalar coders instead of the generated kernel.
The format-neutral rows are compared byte for byte against their own coder's
reference stream, so a knob that quietly changed the format fails there rather
than passing a roundtrip. That is where breakage actually happens: a knob that
moves work between two paths can look fine at the defaults and segfault at
`RC_VEC=0`.

The last rows are not coder runs at all: `misc/rans_div.cpp` settles the rANS
encoder's float32 division by enumeration rather than by sampling -- see below.

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

## The rANS coder

`rans.inc` codes the same bits with a binary rANS instead of a rangecoder. It
is the same file shape as `rc.inc` -- a scalar coder whose `RC_VECOUT` twin
`mk_kernel.sh` pushes through `rc_soa.pl`, `rc_macro.pl` and `defines.pl` into
the same `[RCNUM]` lane arrays and macros -- and the model drives it through
the same `rc_Process(lane, p, bit)`. `RC_RANS=0` gets the rangecoder back.

    M = SCALE = 2^15           the model's probability denominator
    L = M << RC_RANS_KLOG      the state's lower bound, 2^23 by default
    x in [L, 256L)             one uint per lane, and the only thing carried

    encode   x' = (x/f)*M + (x%f) + start
    decode   s = x&(M-1); bit = (s>=p); x' = f*(x>>15) + s - start

with `f = bit ? M-p : p` and `start = bit ? p : 0`.

Three things follow from that, and they are the whole story.

**There is no carry.** The carryless/carry-twin pair, `FFNum`, `Cache`, the
lost-carry re-code, `RC_FF_TRIM` and the 0xFF padding all go, and the lane
state drops from `lowl, lowh, rpre, FFNum, range` to one word.

**It is LIFO**, so `model0.inc`'s coding pass walks the block backwards --
tail bits first, then the groups from the top down, then the 16 block-length
bits, then the flag. The decoder still runs forwards. The reversal is free in
the file layout: the encoder writes its row upwards and `rc_Write` copies it
ascending, which is already the order a LIFO decoder wants, so the decoder
descends from the top of the chunk exactly as the rangecoder's does. What is
not free is that the encoder cannot start until the model pass has produced
the whole block -- `RC_CHUNK`, the interleave that was worth 3% here, is
structurally incompatible with a reverse sweep.

**The encoder divides.** That is rANS's one real cost and it is paid in
float32: after renormalisation the quotient is below `1<<(KLOG+8)`, so a
reciprocal multiply carries it exactly, and one masked add corrects the single
case where the truncation lands one low. `RANS_DIV` in `model.inc` is the
reciprocal, biased by `1-2^-19` so that the truncation is one-sided -- without
that bias it rounds *up* about once in 800 symbols, the remainder wraps
negative, and the correction makes it worse. `misc/rans_div.cpp` settles that
by enumeration: the whole question reduces to where `f*rcp` falls in a window
that depends only on `f`, and there are 32767 frequencies, so it is checked
for every one of them rather than sampled. It reports the margins too -- 15.5x
inside one edge, 7.7x inside the other at the default `KLOG`.

`RANS_DIV` is left to the compile, like `RC_STAGE_CL`, because any correct
division gives the same stream. Writing it as a reciprocal rather than a
divide is worth 11% -- the divide only depends on `f`, which comes out of
`pbit[]`, so this way the divider stays off the loop-carried chain and a
23-cycle latency stops mattering. Once it is off the chain, though, the sweep
is bound by instruction throughput, and every other way of computing the same
quotient loses: the hardware reciprocal approximation plus a Newton step and
an ALU-only reciprocal are both ~11% slower, `double` division 22%, and a
gathered table of inverses 34%. `model.inc` has all six measured against each
other, and why a fast `exp`/`log` cannot reach the precision the single
correction assumes.

### What it costs and what it buys

enwik8, clang 18.1.3, against the rangecoder built from the same tree. The two
binaries are run alternately -- four rounds, each taking the best of 6 encode
and 8 decode passes -- because this box drifts several percent over a minute
and sequential runs of the two are not comparable:

| `-march=native` (AVX-512) | encode | decode | round trip | bytes |
|---|---|---|---|---|
| range coder | 68.22 MB/s | 39.04 MB/s | 40.27 ms/MB | 62,513,092 |
| rANS        | 63.98 MB/s | **42.96 MB/s** | **38.91 ms/MB** | 62,588,528 |
|             | -6.2% | **+10.0%** | **-3.4%** | +0.121% |

| `-march=skylake` (AVX2) | encode | decode | round trip | bytes |
|---|---|---|---|---|
| range coder | 62.02 MB/s | 36.19 MB/s | 43.76 ms/MB | 62,513,092 |
| rANS        | 53.23 MB/s | **41.03 MB/s** | **43.16 ms/MB** | 62,588,528 |
|             | -14.2% | **+13.4%** | **-1.4%** | +0.121% |

**rANS wins the decode and loses the encode, and neither is an accident.** It
comes out ahead over an encode plus a decode on both targets, and it is ahead
by more where it matters most: decode was the slower half to begin with, and
it is the half a decompressor runs.

Decode is where the coder's own dependency chain competes with the model's.
The rangecoder's bit comes out of `code >= (range>>15)*freq`, so the model's
`p` has to go through a multiply before the bit is known and the context can
advance. rANS's comes out of `s >= p`, a compare on state the previous step
already produced -- and the one multiply it does need, `p*(x>>15)`, does not
wait for the bit at all, because both arms of the step reduce to it:
`(M-p)*(x>>15) + s - p` is `x - p - p*(x>>15)`. Three cycles off the loop the
decoder actually waits on, and a decode loop 25% smaller in instructions.

Encode is the mirror image. The rangecoder's step is a multiply and two
compares; rANS's is a divide, two float conversions, the same multiply and the
same two compares. The generated sweep is *shorter* than the rangecoder's --
82 instructions per 16-lane group against 92 -- and still slower, because the
extra work lands on ports the rangecoder was not using and the store the two
share is already most of the group. Nothing recovered it, and the failures
agree on why. `RCNUM=32` (twice the independent chains) is flat, and so is
moving the divide ahead of the renormalisation to shorten the chain: the sweep
is not chain-bound. Swapping `vdivps` for the hardware reciprocal
approximation plus a Newton step is 11% slower, and an ALU-only reciprocal
10% slower: the divider is not the limit either -- its port is otherwise idle,
and anything that moves that work onto the ALUs loses. Nor is it the store:
staging plus one `vpscatterdd` per sub-group beats a direct store (63.83
against 56.75) and beats scalar commit stores (58.57), so the shape in place
is already the best of the three. What the sweep is short of is registers --
21 of its 82 instructions are moves and spills.

The 0.121% of ratio is almost all one thing: rANS ends every substream by
flushing its whole 31-bit state, four bytes per lane per block, where the
rangecoder's minimal flush writes one or two. `RC_RANS_FLUSH` takes back the
part of that which is not information -- see `misc/rans_flush.md`, which is
the distribution of 24,416 flushed states and what can be cut from it. It scales with `RCNUM` and with
`1/BLKSIZE`, not with the arithmetic -- the rounding loss the `L/M = 2^8` state
range actually costs is about 300 bytes of the 75,436 -- that is the whole
`KLOG=7 -> 8` step, and the loss halves with every one -- which is why
`RC_RANS_KLOG` barely moves the total.

`misc/rans_flush.md` answers whether the rangecoder's minimal flush has an
rANS analogue. The flushed state is log-uniform to within a fifth of a percent
per octave -- exactly what rANS theory requires -- so the only redundancy in
it is its **4.50 leading zero bits** on average, and conditioned on the octave
the rest measures 7.93-7.95 bits of 8. `RC_RANS_FLUSH=1` (the default) writes
three bytes instead of four when the state fits, told apart by the state's own
`x >= RANSL` invariant with no flag: 2,975 bytes, and free. The rangecoder's
trick itself does not carry over -- it works because a range of final `low`
values decode identically, and rANS's decoder is injective. The freedom rANS
does have is at the encoder's *initial* state, and renormalisation destroys it
within about a dozen symbols; the file has that measured.

Its §4 checks two further proposals against the geometry. A big-endian flush
with zero padding is already what the format does -- the decoder reads the
chunk downward, so the state's top byte is at the top of the substream -- and
an explicit byte count in the length header would be equivalent in yield, not
better, because `x >= L` puts the floor at three bytes for every
`RC_RANS_KLOG` from 1 to 8. The length field does have three free high bits:
lane lengths top out at 4,166 on enwik8 against a 4,096-byte raw equivalent.
A raw-storage escape is worth under 300 bytes on enwik8 -- but 20 MB of
urandom expands **3.021%** today, and a per-block escape would cap that near
zero. Per-lane would not: a raw lane's bits still walk the shared context
tree, so it would cost a test inside the 22-instruction decode body on every
file. Not built; the file says why.

`misc/iftree_rans.md` is Shelwien's generated if-tree -- 255 real branch sites,
one per context node -- tested against rANS, where `dec_vectorize.md` §9
measured 1.63x for the range coder (1.38x reproduced on this box). It loses,
40-45%, in every shape and at every depth tried. Four controls rule out the
obvious explanations, one of which was mine: it is not the lost wavefront ILP
(a byte-sequential walk costs 3.3%), not register pressure (the cheap tree
spills 1.4%), not where the multiply sits (forcing the bit to wait for it, as
the range coder's does, moves the tree/walk ratio from 0.602 to 0.585 -- the
wrong way), and not code size (the range coder's *winning* tree is 12,516
instructions against this one's losing 7,675). What is left is that the tree
costs what it costs -- 285 branches against the walk's 18 -- and only wins
where the walk is more expensive than that. The file is explicit that the
cross-bench comparison does not establish the coder as the reason, since the
two harnesses differ in model and structure too.

`misc/rans_decode.md` is the decoder read off its own disassembly: 130 vector
instructions in 1099 against the encoder's 385 in 843, 22 instructions per bit
at ~9.4 cycles, so IPC ~2.5 -- about 38% of a 4-wide machine's issue width
idle. It has `rc.inc`'s `RC_DEC_SPLIT` ported and measured (-33%), the decode
step's three algebraic forms (the default wins by a quarter, because its
multiply does not wait for the bit), a one-byte refill that is +1.3% on one
machine and -9.2% here, and Shelwien's `rans_shapes` shapes reproduced -- the
ordering transfers, the magnitude does not.

Its §5a is the one number the whole wide-SIMD question turns on. Every
proposal to gather the counters for 8 lanes, step them as a vector and scatter
back needs a defined adaptation lag in the FORMAT, because at depth 0 every
in-flight byte reads and writes `cty[1]`. `RC_DEFER_UPD=G` prices that in
`model_pass` alone: **+0.309% of enwik8 at G=2 and +1.90% at G=8**, against a
total rANS-vs-rangecoder ratio gap of 0.121%. No measured speed result is
worth 16x the coder's whole ratio cost, so the space is closed.

`misc/rans_comparison.md` reads this coder against three other vectorised rANS
implementations -- `rz`, `lolz` and Oodle's LZNA -- from their source. The
short version is that they vectorise the *alphabet* of one stream (a SIMD
search over 16 or 32 cumulative frequencies, two interleaved states) and leave
the coder scalar, where we vectorise the *coder* across sixteen streams and
leave the alphabet at two. Two of them also code single bits, and all three
binary steps turn out to be the same algebra; the differences are that they
branch on the bit where we select, and that their encoders divide with the
integer `/` where sixteen lanes left us no such option.

### Knobs

| | |
|---|---|
| `RC_RANS=0` | build the rangecoder instead (default 1) |
| `RC_RANS_KLOG=4` | `log2(L/M)`, 1..8. Above 8 the state passes 2^31 and `rans.inc`'s signed float conversion misreads it, which is an `#error` |
| `RC_VEC=0` | put the coding pass on `rc_vec.inc`'s scalar coders. The rANS one divides with the integer `/` where the generated kernel divides in float32, so a run where the two agree byte for byte is a run where the float path is right |
| `RANS_DIV`, `RANS_BIAS` | the division and its bias, defined in `model.inc` because they are the compile's choice |

## Running the coder directly

    coder [-C] c|d input output FSM_file [n_iter] [test_output]

    coder c ../enwik8 1 ../FSM0.txt            encode
    coder d 1 2 ../FSM0.txt                    decode
    coder c ../enwik8 1 ../FSM0.txt 10 2       encode x10, decode, log.txt
    coder -C c ../enwik8 1 ../FSM0.txt         the scalar reference coder

`log.txt` collects one line per test-mode run: encode speed and time, decode
speed and time, output size, then the directory, the coder (`rans` or `rc`),
`RCNUM` and `RC_LOWBYTES`. `t.sh` appends the compiler and the ISA the build
actually got, which is the `avx512` / `avx2` column the earlier lines carry.
Lines from before the rANS coder existed have no coder column; they are all
range coder runs.
