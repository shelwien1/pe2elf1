# h264dec, h264p

- `h264dec`: H.264 decoder: the OpenH264 decoder and its console front end
  (upstream commit 0819f288 from 2022, C code only, flattened into `codec/`),
  with fixes that make it decode correctly.
- `h264p`: a lossless compressor of H.264 streams: it removes the entropy
  coding and codes the syntax again with a context mixing model
  (`h264p_src/`).

## Build

Linux (gcc or clang):

    make                        # -> ./h264dec ./h264p
    make CXXFLAGS="-O0 -g"      # debug build

Windows (MinGW): `g.bat`, as before, for h264dec, `gp.bat` for h264p.

h264p is one translation unit: `h264p_src/h264p.cpp`, its command line,
includes `h264p_src/h264p.inc`, which includes the rest of its sources
(`*.inc`, the coroutine library in `h264p_src/Lib3` and MTwrap in
`h264p_src/MTwrap_v3`, which includes `Lib3/win_shim.inc` by its name
alone, as mtwrap builds it; a program of its own includes `h264p.inc` for
h264p's coroutines), so
`g++ -O2 -march=x86-64-v3 -std=c++11 -fwrapv -fno-exceptions -fno-rtti -Ih264p_src/Lib3 -pthread h264p_src/h264p.cpp -o h264p`
builds it too: it uses neither C++ exceptions nor RTTI (its errors jump
to traps the callers set, `h264p_src/err.inc`), nor anything of the C++
library: no containers, strings or algorithms (`h264p_src/mem.inc` has the
few it needs), so that it links without it (`gcc` in place of `g++`, with
`-lm`); and its structs have no constructors or destructors: `Init()`
sets one up and `Quit()` frees what it holds, so that its coroutines are
plain data on pages mapped for them. Its target is x86-64-v3 (AVX2:
Haswell, Zen and later), for which its pixel kernels and mixers have SIMD
code; built for any other target (`make PARCH=`, or without `-march`) it
uses the plain C++ beside them, which is slower and codes identically
(as do builds by GCC and clang).
h264p runs in one thread, or with `-t N` on N (below). `-DH264P_ONLY=L`
builds one level of its model alone.

## Test

    make test

decodes 55 streams with h264dec and compares the MD5 of the output with
references that both ffmpeg and the JM reference decoder agree on:

- `test/002`: the 7 provided streams (x264, Main/High, B-frames, CABAC)
- `test/gen`: 48 small streams made from them with x264 and the JM encoder to
  cover more coding tools (see `test/gen/gen.sh`)

`test/run.sh ./h264dec file.264...` checks any other stream against ffmpeg.

It then round-trips these streams and the ones in `test/h264p` through
h264p, compressing at each level of its model (`test/h264p.sh`, which
takes stream names too, and `LEVELS="9 1"` for some of the levels).

## h264dec

    ./h264dec input.264 output.yuv

writes the decoded frames as raw yuv420p in display order, cropped as the
stream specifies. Further options: `-trace N`, `-ec N`, `-length FILE`,
`-options FILE`, `-legacy`.

### Fixes

Decoding was wrong for every stream with B-frames: in the 4 such provided
streams the errors accumulated to visible corruption (worst frames at 16-36 dB
PSNR). The fixes (details in the commit messages):

- bi-predicted 16x8 / 8x16 partitions (as upstream #4004)
- deblocking of internal edges of B_Skip macroblocks (still wrong upstream)
- pictures overwritten before they were output, e.g. at scene cuts
  (as upstream #3707)
- MV prediction next to temporal direct sub-blocks, CAVLC and CABAC
- reference list modification with duplicate references (x264 weightp)
- initial B-slice lists with long-term references
- explicit weighted bi-prediction of 8x4 / 4x8 sub-blocks
- POC for pic_order_cnt_type 1 and 2, frame POC with a bottom field offset
- max_long_term_frame_idx range check (as upstream f70c905b)

The Makefile builds with `-fno-strict-aliasing`, like upstream: the code
type-puns its motion vector arrays.

### Limitations (inherited from OpenH264)

- progressive 8-bit 4:2:0 only; interlaced streams (PAFF/MBAFF) are rejected
- FMO / arbitrary slice order: slices are deblocked one at a time instead of
  the whole picture in MB order, which is wrong when slices interleave
- the multi-threaded decoding mode of the library is not used by h264dec and
  was not tested

## h264p

    ./h264p c [options] input.264 output.h264c   # compress
    ./h264p d [options] input.h264c output.264   # decompress

`-v` prints statistics, `-s` makes `c` fail when a NAL unit of a supported
type has to be stored verbatim, and `-9`, `-7`, `-5`, `-3`, `-1` pick the
level of `c`'s model, from the strongest (the default) to the fastest; `d`
reads the level from the stream. `-t N` makes `c` compress on N threads, up
to 64, the stream in parts of at least `-c N` MB, 1 to 1024 (`-c Nk`: KB;
4 MB by default), and `d` decompresses that on as many threads as it says
(see Threads, below); without `-t` (or with `-t 1`), `c` writes the stream
of one model in one thread, as it always did. Input and output can be `-`
(stdin, stdout), and not the same file; an output named `/dev/stdout`,
`/dev/fd/N` or `/proc/self/fd/N` (or a link to one) is written through that
descriptor, as `-` is, where the shell put it. A file is written under a temporary
name beside it (`name.PID.tmp`, with the mode of the file it replaces) and
renamed when the call is done: a call that fails leaves what was there (or
nothing), except that `d` keeps what it restored before an error in the
stream (not after one writing it). Interrupted, or ended by a pipe it
writes to closing (SIGINT, SIGTERM, SIGHUP, SIGPIPE; not on Windows), it
removes the temporary file.

Where stderr is a terminal (or with `-p`), `c` and `d` say what the video
is, as its first slice has it, and show their progress on a line put over
itself every half second:

    [enc] video: 960x540, High@L4.0, 4:2:0 8-bit, CABAC, progressive, 8x8 transform, weighted P, implicit weighted B, 4 reference frames, 60 fps
    [enc] 1441792 > 983051   37.3%  1910 pictures   0.47 MB/s  180.01M RAM
    [enc] 3866640 > 2736446  100.0%  4840 pictures  done in 11.89s  193.72M RAM at most

The video: its size as shown (cropped), profile and level, chroma format
and bit depth, entropy coding, frames or fields (MBAFF, field pictures),
the tools its parameter sets turn on (8x8 transform, weighted prediction,
slice groups, constrained intra, lossless, scaling matrices), the
reference frames and the frame rate where the stream gives one. With `-v`
it is said where there is no progress line too (and with `-t`), and again
where it changes ("the video now: ..."): another sequence (SPS), or a tool
its slices had not used since. A stream that goes back and forth between
frame and field pictures (PAFF), or between PPSs of other tools, says
each of them once. The line: the bytes of the stream and of
the compressed stream done so far (`dec`: the same way round, the stream
written from the compressed stream read), how far through its input the
call is (none for a pipe, whose size is not known), the pictures, the
speed (MB of the stream a second) and the memory the process has (its
resident set); and at the end, how long it took and the most memory it
had. (On systems other than Linux and Windows, the line shows the most
throughout.) A message (an error, `-v`'s) starts on a line of its own,
and after an error the line is not put again.

The codec prints nothing: its coroutine tells the front end what there is
to show, by what it yields with (`rcio.inc`): the video, a message, its
statistics at the end, a point to show progress at (each picture), its
error. The front end (`drivers.inc`, and with `-t` the workers'
`run_part()` in `mt.inc`, which put the line as they code, since the main
thread mostly waits on them) shows it, under one lock.

### Compression

`c` parses each NAL unit and codes the values it parsed with the model: the
macroblock syntax (skip flags, mb_type, intra modes, motion,
coded_block_pattern, mb_qp_delta, the coefficients) and the header syntax.
`d` decodes the values and codes them with CAVLC or CABAC as the stream did.
`c` sees to it that this gives back the NAL unit as it was: it writes the
NAL unit back from the values before coding them, or, a slice, checks as it
codes it that the walk writes the bits and CABAC bins it read (should a
slice fail that, `c` compresses the stream again, writing back every NAL
unit first, as it always does through pipes). A NAL unit that does not come
out the same is stored verbatim, and so is a slice that would take the
model's records of the last picture of each kind (slice type, reference or
not, frame or field) past 512 MB: a stream has a few kinds, 4.2 MB each at
1080p, while all 30 at the largest pictures would take 1.4 GB. A CRC-32 at
the end checks the whole stream, and `d` takes data after the end of the
compressed stream for corruption.

The whole stream is one run of the range coder, flushed once at its end;
only h264p's header (the magic, the version, the level) is outside it. The
bytes stored verbatim, and those before the first start code, go through
it in chunks of up to 1 MB. A chunk of 64 KB or more that would cost the
model 8 bits a byte or more is coded as it is, each byte one step of the
coder that takes 1/256 of its range: bytes that look random to the model
(their entropy of order 0, of their differences and of order 1 near 8
bits, and next to nothing its match would find). That costs nothing over
the bytes themselves, where the model spends 0.1-0.2% more on random
bytes, and is about ten times faster: 16 MB of random bytes, `c` 3.3 s
and `d` 3.4 s before, 0.34 s and 0.22 s now (one decision a chunk says
which it is).

Memory does not grow with the stream, so one call can take a stream of any
length (petabytes, through a pipe). `c` and `d` hold at most 64 MB of a NAL
unit: a longer one, and the bytes before the first start code, are stored
verbatim as they are read, in chunks of 1 MB, and runs of zero bytes are
counted, not held. The model's tables and records are bounded by the
level and the largest pictures, and the bytes stored verbatim are matched
against the last 16 MB of them. Counters are 64-bit or start again where
they would wrap, and POCs are kept modulo 2^32 and compared by their
differences, so a stream that never sends another IDR picture is decoded
as a decoder would. With `-t`, the splitter keeps one version of each
parameter set and those of the part under way. `d` holds no more of a
compressed stream than `c` makes of one, whatever it reads: a NAL unit it
would parse past 64 MB (an SEI's messages), a part of `-t` that comes out
longer than its record says, or a record of a part larger than `-c` lets
`c` make, is broken as it gets there. It reads a part's compressed bytes
as the part's model asks for them, whatever size its record says. What it
reads past to find the next part in turn (MTwrap's container has the
workers' blocks in the order they came: `c`'s within about a round of
parts of each other) it holds packed: a stream made to make it hold much
can make it hold no more than the bytes it has read.

The model is binary context mixing, after oggcomp (Ogg Vorbis): each value
is a sequence of binary decisions, each predicted by ten adaptive counters
under different contexts (each with a slow and a fast probability), an
APM, three mixers (and a fourth that mixes them) and a second APM, all
selected by the decision's place in the value; where the stream is CABAC,
the probability CABAC's own context state gives a bin is one more input.
Where the model has a guess at a number (a level, a count, an mvd, a QP
delta), each decision also sees what the guess would decide there. Header
values are coded against the last of their element (frame_num and the POC
modulo their range), and the bytes nothing else models, x264's SEI among
them, with a match against a built-in dictionary of x264's options. The
contexts and the parameters of each group of syntax elements (a family)
are declared in `h264p_src/IDX/l<L>/*.idx`, a set for each level, and turned
into code in `h264p_src/MOD/l<L>/` (see `h264p_src/mk.sh`), where they can
also be tuned, a level at a time. `h264p_cm.md` is the design; this is
level `-9`, and the lighter levels leave parts of it out
(`h264p_levels.md`).

Most of the contexts come from the pictures: the model decodes them as it
goes (`recon.inc`, for 8-bit 4:2:0 frame streams without scaling matrices,
which covers x264's defaults; it matched ffmpeg's output bit for bit on the
parts of the test streams compared), and looks at the samples before each
decision:

- mb_qp_delta: x264's adaptive quantization sets a macroblock's QP from
  log2 of its energy, so a line of QP against energy fitted to the
  picture's macroblocks so far predicts the QP from the energy of the
  macroblock's motion-compensated prediction (for an intra macroblock, of
  its proxy, below; a picture's first macroblocks take the last picture's
  slope); the delta's magnitude and sign follow the line's.  Where the
  picture stays the same from one picture to the next, x264's macroblock
  tree lowers the QP.  For other encoders: the slice's QP (macroblock QPs
  keep near it), the class of the energy (whose QP is no line of it), and
  the QP of the picture's last macroblock of the same kind and energy
  class (the rate control's QP plus what the class adds), and of the
  neighbours and col where they are of the same kind.
- skip and mb_type: how well the prediction the macroblock would have if
  skipped continues the decoded samples above and to the left of it, how
  well that motion predicts those samples themselves (template matching),
  how flat it is against the quantizer's step, in B slices how far apart
  its two predictions are and what the same macroblock did in list 1's
  reference.
- motion: the vector a template matching search finds for the partition,
  as a guess at the mvd (and its sign, once the magnitude is known), and
  how flat the reference is at the predicted vector across each component
  (a search gains nothing where it is), and how far the neighbours' vectors
  were from their template matches'; the search starts from the
  neighbours' vectors (those on other references scaled by POC distance)
  and the temporal one: in B slices the vector temporal direct would give,
  in P col's, scaled by the distance it spanned; for ref_idx, the
  reference it matches best.
- intra modes: the gradients around the block (DIMD), and the modes that
  predict the block best in the reference, by x264's measure (SATD): how
  much worse the most probable mode does there, in x264's lambda, tells
  whether an encoder would have taken it, and the best two others are
  likelier than the rest; likewise for the Intra16x16 and chroma modes.
- coefficients: the texture of the prediction and how far it is from
  continuing its surroundings, against the quantizer's step, and which way
  the texture runs (a motion error leaves a residual of those
  frequencies), and for the significance of inter blocks the prediction's
  own transform; what x264's decimation still requires of an 8x8 block;
  for a chroma block, the coefficients of the luma under it, and for a
  chroma or Intra16x16 AC block its DC (from the DC levels);
  the signs of the first coefficients are
  predicted as the ones that make the block continue the decoded samples
  around it best, and for inter blocks the sign of each as that of a small
  shift of the prediction, the shift fitted to the signs known so far; for
  intra blocks, the block in the reference that matches the macroblocks
  around (searched from the motion around too, and again as the macroblock
  decodes) stands in for the source, and its difference to the prediction,
  transformed and quantized as an encoder would, estimates the count and
  the levels (trusted as far as the reference matched the samples around
  and the blocks decoded); in P pictures, the coefficients of the block
  the vector points at in its reference.  Besides the decoder's view, the
  significance of a position sees that of the eight before it and of the
  same position in the blocks to the left and above, the levels and signs
  the ones coded before, and the top mantissa bits of a number the bits
  above them.

On the test streams (`-v` shows where the bits go):

| stream | H.264 | compressed | |
|---|---:|---:|---:|
| 00 | 815,140 | 559,490 | 68.6% |
| 01 | 1,095,103 | 793,286 | 72.4% |
| 02 | 28,143 | 26,044 | 92.5% |
| 03 | 1,181,369 | 767,960 | 65.0% |
| 04 | 484,238 | 378,937 | 78.3% |

The macroblock syntax takes 71.3% of what its CABAC coding takes (from
3,536,129 to 2,520,607 bytes over these streams; whole streams go from
3,603,993 to 2,525,717 bytes; `h264p_cm.md` section 9 has the figures by
syntax element). That is level `-9`. In one thread, on a 2.1 GHz Xeon with
the x86-64-v3 build, the levels do on the 7 streams of `test/002` (3.87 MB
of H.264):

| level | compressed | | `c` KB/s | `d` KB/s | memory |
|---|---:|---:|---:|---:|---:|
| `-9` | 2,717,777 | 70.3% | 106 | 108 | 63 MB |
| `-7` | 2,761,576 | 71.4% | 131 | 141 | 61 MB |
| `-5` | 3,028,386 | 78.3% | 222 | 226 | 58 MB |
| `-3` | 3,127,949 | 80.9% | 323 | 349 | 37 MB |
| `-1` | 3,210,963 | 83.0% | 660 | 742 | 28 MB |

(memory: the peak resident size compressing 03; since, `-1`'s counters
keep their probabilities stretched, so it decompresses 3-5% faster and
peaks at 30 MB: `h264p_l1_speed.md` section 6.4). `h264p_levels.md` has
what each level leaves out of the model, where the time goes, and how each
level has its own IDX profile, tuned against dumps of its families'
decisions; `h264p_speed.md` how `-9` got here, `h264p_l1_speed.md` how
`-1` got from 422 and 528 KB/s to this.

### Streaming: coroutines

`c` and `d` run as coroutines of Lib3, the stack-copying coroutines of the
library mtwrap is built on (`h264p_src/Lib3`, its files unchanged), in its
buffer protocol: the input comes in the buffers `addinp()` gives, a
0-length one with `f_quit` set at its end, and `yield 1` asks for the next;
the output goes into the buffer `addout()` gave, `yield 2` handing it over
full; `yield 0` ends a run, `yield 3` an error. So `run_file()`
(`drivers.inc`) runs one on files (`c`, `d`), and the library's
`CoroutinePair` chains `c -t`'s splitter to MTwrap's coder (Threads,
below).

The coroutine is the codec, as in the library's own codecs:
`H264pCoro<f_DEC, L>` (`h264p_src/coro.inc`; `f_DEC` 0 compresses, 1
decompresses, `L` the level) derives from the model, `Model<f_DEC, L>`; the
model from the range coder with the statistics of `-v` (`tc_rc<f_DEC>`,
`fam.inc`), which its families code through, each by a pointer to it; and
the coder from the coroutine's I/O (`rcio.inc`: its bytes go into the
coroutine's output buffer and come from its input buffer, through
`Coroutine`). A run is an object of its own, constructed for it, and runs
share nothing: any number of them can go at once, on as many threads. What
the program has besides are constants (the counters' and mixers' tables,
computed as it starts). The front end constructs a run's coroutine for the
level (`c`'s `-L`, or the byte `d` reads ahead in the stream) on pages of
its own, and its model maps its tables as it starts and unmaps them as it
ends.

Each yield copies the coroutine's stack out, into a buffer of 64 KB
(`Coroutine::STKPAD`). Both yield deep in the model, where the range coder
writes its output or reads its input, with up to 11 KB of stack (`-9`;
`-v` prints it); a yield that would need more than the buffer fails the run
instead of overrunning it. The output is byte for byte the file-based
h264p's (every level's output of every test stream has the MD5 it had), as
fast.

The model pass checks the slices only where the input can be read again
(regular files, and the parts on threads, which are read whole first): a
slice that does not come out as it is makes the front end compress the file
again, as before; through pipes, the dry run writes back each NAL unit, as
h264p always did. The library's
`setjmp`/`longjmp` are GCC's builtins or, with clang, x86-64 assembly: h264p
builds for x86 now (the plain C++ build there too); elsewhere, the
library's `CORO_NOASM`, untested.

### Threads

`c -t N` compresses on N threads, 2 to 64 (`h264p_src/mt.inc`). A splitter
(`h264p_src/h264split.inc`, rzwrap_h264's, on the main thread) cuts the
stream into parts: a part ends at the first IDR or I access unit at least
`-c` (4 MB by default) after its start, or, in a stream with too few of
those, at the first picture at least 4 times `-c` (the cap, at most 2 GB)
after it; where no picture comes within half again of the cap, at a NAL
unit, and where none comes within twice the cap (no start code, or one
NAL unit as large), there, inside it. The parameter sets a part needs are
copied in at its start (after its access unit delimiter, if it has one),
so that each part is a stream by itself (one that starts at a P picture
loses the reference pictures before it, and some compression; never
losslessness: the bytes of a part are coded whatever they are). MTwrap (`h264p_src/MTwrap_v3`,
mtwrap's but that it only ever waits on events, never polls with `Sleep()`;
its parts mode: `MTwrap.md` section 11 in mtwrap) deals the
parts in turn to N workers; a worker reads a part whole into memory and
compresses it with a model of its own, an `H264pCoro` run as a coroutine of
the worker's, then the next part dealt to it. Decompressing, a worker
decompresses its parts so, removes the inserted parameter sets, and MTwrap
takes the parts from the workers in turn, as they were dealt.

The stream starts with `h264m`, the version, the count of threads and the
part size (KB, 32 bits: `d` takes a part's record that says more than `c`
makes at that size for broken; MTwrap's rings have a size of their own),
then MTwrap's container of the workers' streams (a block of up to 64 KB at
a time, tagged with its worker; a worker's record goes out as its part is
done, its last block as far as it is filled, so that `d`, which takes the
parts in turn, finds each within about a round of parts of the others),
each of them a record for each of its parts: its size, the size of the
part as compressed (with the inserted parameter sets) and where in it
those sets are (`d` removes those bytes, whatever the part holds), the
part's place in the stream (worker w's k-th of N: w + kN), 64 bits each,
little-endian, and the CRC-32 of these, then the part compressed, a
stream of h264p's with its own header; and at its end a record that says
it ends. So `d` finds a part lost, added or moved, a stream cut where a
part ends, and a record changed, which the parts' own CRCs do not see.
`d` knows it by its first bytes and runs as many threads as it says. Without `-t`, or with `-t 1`, `c` writes h264p's stream of one model,
as before (`h264c`).

On the 7 corpus streams concatenated (3.87 MB, 82 IDR pictures), on 4
cores of the 2.1 GHz Xeon VM:

| | size | `c` | `d` |
|---|---|---|---|
| `-1` | 3,212,413 | 6.3 s | 5.2 s |
| `-1 -t 2 -c 1` | 3,217,270 (+0.15%) | 4.0 s | 3.7 s |
| `-1 -t 4 -c 1` | 3,217,275 (+0.15%) | 2.1 s | 2.0 s |
| `-1 -t 4 -c 256k` | 3,228,357 (+0.50%) | 2.1 s | 1.9 s |
| `-1 -t 4 -c 64k` | 3,254,953 (+1.32%) | 2.5 s | 2.3 s |
| `-9` | 2,719,518 | 33.8 s | 33.1 s |
| `-9 -t 4 -c 1` | 2,736,026 (+0.61%) | 11.3 s | 11.2 s |
| `-9 -t 4 -c 256k` | 2,767,263 (+1.76%) | 10.0 s | 10.0 s |

A part starts its model from nothing, which costs compression (more at
`-9`, whose model learns more) and the set-up of the model (about 50 ms of
CPU time at `-1`); hence parts of 4 MB by default, for long streams. A
stream shorter than a part is one part, on one thread. Each thread has a
model: at `-9`, 64 MB of memory in one thread, 194 MB on 4.

### Verification

`test/h264p.sh` round-trips every test stream through `c` and `d`, with
`-s`, so all their NAL units are converted (except the data partitions of
`test/h264p/jm_dp.264`), and two concatenations: one in which
x264's padding bits change phase, and one that changes the picture size,
chroma format, bit depth, slice groups and field coding from stream to
stream. Then the cases that must fail, and fail well (a corrupt slice with
`-s`, on threads too, where `c` must not hang; the input as the output; a
cut compressed stream; a full output device), and the crafted streams of
`test/crafted`, each one h264p once crashed on, took minutes or gigabytes
for, or failed as a whole (`h264p_review.md`).
The streams in `test/h264p` cover what the others don't (see `gen.sh` there).
The values h264p parses from all test streams were also compared,
macroblock by macroblock, with the syntax elements traced by the JM
reference decoder (through the TSV dump h264p had then), and `c` and `d`
(built with ASan and UBSan, which report nothing on the test streams) were
fuzzed with corrupted streams, compressed files and random concatenations
of the test streams. The tuning and release builds of `h264p_src/mk.sh`, and gcc
and clang builds at -O2 and -O3, code the test streams identically.
