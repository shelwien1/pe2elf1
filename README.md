# h264dec, h264p

- `h264dec`: H.264 decoder: the OpenH264 decoder and its console front end
  (upstream commit 0819f288 from 2022, C code only, flattened into `codec/`),
  with fixes that make it decode correctly.
- `h264p`: a lossless compressor of H.264 streams: it removes the entropy
  coding and codes the syntax again with a context mixing model. The same
  syntax can be dumped to a TSV file, from which the identical stream is
  restored (`h264p_src/`).

## Build

Linux (gcc or clang):

    make                        # -> ./h264dec ./h264p
    make CXXFLAGS="-O0 -g"      # debug build

Windows (MinGW): `g.bat`, as before, for h264dec, `gp.bat` for h264p.

h264p is a single module: `h264p_src/h264p.cpp` includes the rest of its
sources (`*.inc`, the coroutine library in `h264p_src/Lib3` and MTwrap in
`h264p_src/MTwrap_v3`, which includes `Lib3/win_shim.inc` by its name
alone, as mtwrap builds it), so
`g++ -O2 -march=x86-64-v3 -std=c++11 -fwrapv -Ih264p_src/Lib3 -pthread h264p_src/h264p.cpp -o h264p`
builds it too. Its target is x86-64-v3 (AVX2:
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
    ./h264p t [options] input.264 output.tsv     # dump the syntax
    ./h264p r [options] input.tsv output.264     # restore from the dump
    ./h264p x [options] input.264 output.264     # c and d at once: the stream restored

`-v` prints statistics, `-s` makes `c`, `t` and `x` fail when a NAL unit of
a supported type has to be stored verbatim, and `-9`, `-7`, `-5`, `-3`, `-1`
pick the level of `c`'s (and `x`'s) model, from the strongest (the default)
to the fastest; `d` reads the level from the stream. `-t N` makes `c` (and
`x`) compress on N threads, up to 64, the stream in parts of at least `-c N`
MB (`-c Nk`: KB; 4 MB by default), and `d` decompresses that on as many
threads as it says (see Threads, below); without `-t` (or with `-t 1`), `c`
writes the stream of one model in one thread, as it always did. Input and
output can be `-` (stdin, stdout).

Where stderr is a terminal (or with `-p`), `c`, `d` and `x` show their
progress there, on a line put over itself every half second:

    [enc] 1441792 > 983051   37.3%   0.47 MB/s  180.01M RAM
    [enc] 3866640 > 2736446  100.0%  done in 11.89s  193.72M RAM

the bytes of the stream and of the compressed stream done so far (`dec`:
the same way round, the stream written from the compressed stream read),
how far through its input the call is (none for a pipe, whose size is not
known), the speed (MB of the stream a second) and the most memory the
process has had; and at the end, how long it took. With `-t`, the workers
put the line as they code (the counts are theirs), since the main thread
mostly waits on them. A message (an error, `-v`'s) starts on a line of its
own, and after an error the line is not put again.

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
out the same is stored verbatim, as in the TSV file (below); a CRC-32 at
the end checks the whole stream, and `d` takes data after the end of the
compressed stream for corruption.

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
| 00 | 815,140 | 559,790 | 68.7% |
| 01 | 1,095,103 | 793,364 | 72.4% |
| 02 | 28,143 | 26,047 | 92.6% |
| 03 | 1,181,369 | 768,001 | 65.0% |
| 04 | 484,238 | 378,938 | 78.3% |

The macroblock syntax takes 71.3% of what its CABAC coding takes (from
3,536,129 to 2,521,031 bytes over these streams; whole streams go from
3,603,993 to 2,526,140 bytes; `h264p_cm.md` section 9 has the figures by
syntax element). That is level `-9`. In one thread, on a 2.1 GHz Xeon with
the x86-64-v3 build, the levels do on the 7 streams of `test/002` (3.87 MB
of H.264):

| level | compressed | | `c` KB/s | `d` KB/s | memory |
|---|---:|---:|---:|---:|---:|
| `-9` | 2,718,200 | 70.3% | 106 | 108 | 63 MB |
| `-7` | 2,762,018 | 71.4% | 131 | 141 | 61 MB |
| `-5` | 3,028,487 | 78.3% | 222 | 226 | 58 MB |
| `-3` | 3,128,074 | 80.9% | 323 | 349 | 37 MB |
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
full; `yield 0` ends a run, `yield 3` an error. So the library's drivers run
them: `CoroFileProc` on files (`c`, `d`), and `CoroutinePair` chains two
(`x`: the compressor's output is the decompressor's input, in one pass, and
what comes out must be the input).

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
again, as before; through pipes, or a `CoroutinePair` stage, the dry run
writes back each NAL unit, as h264p always did through pipes. The library's
`setjmp`/`longjmp` are GCC's builtins or, with clang, x86-64 assembly: h264p
builds for x86 now (the plain C++ build there too); elsewhere, the
library's `CORO_NOASM`, untested.

### Threads

`c -t N` compresses on N threads, 2 to 64 (`h264p_src/mt.inc`). A splitter
(`h264p_src/h264split.inc`, rzwrap_h264's, on the main thread) cuts the
stream into parts: a part ends at the first IDR access unit at least `-c`
(4 MB by default) after its start, and the parameter sets it needs are
copied in at its start (after its access unit delimiter, if it has one), so
that each part is a stream by itself. MTwrap (`h264p_src/MTwrap_v3`,
unchanged, its parts mode: `MTwrap.md` section 11 in mtwrap) deals the
parts in turn to N workers; a worker reads a part whole into memory and
compresses it with a model of its own, an `H264pCoro` run as a coroutine of
the worker's, then the next part dealt to it. Decompressing, a worker
decompresses its parts so, removes the inserted parameter sets, and MTwrap
takes the parts from the workers in turn, as they were dealt.

The stream starts with `h264m`, the version, the count of threads and the
part size (KB, 32 bits), then MTwrap's container of the workers' streams
(a block of up to 64 KB at a time, tagged with its worker), each of them a
record for each of its parts: its size, the count of inserted parameter
sets, the size of the part as compressed (32, 32 and 64 bits,
little-endian), and the part compressed, a stream of h264p's with its own
header. `d` knows it by its first bytes and runs as many threads as it
says; `x -t N` runs both at once, MTwrap's coder into its decoder. Without
`-t`, or with `-t 1`, `c` writes h264p's stream of one model, as before
(`h264c`).

On the 7 corpus streams concatenated (3.87 MB, 82 IDR pictures), on 4
cores of the 2.1 GHz Xeon VM:

| | size | `c` | `d` |
|---|---|---|---|
| `-1` | 3,212,415 | 6.3 s | 5.2 s |
| `-1 -t 2 -c 1` | 3,217,271 (+0.15%) | 4.0 s | 3.7 s |
| `-1 -t 4 -c 1` | 3,217,276 (+0.15%) | 2.1 s | 2.0 s |
| `-1 -t 4 -c 256k` | 3,228,356 (+0.50%) | 2.1 s | 1.9 s |
| `-1 -t 4 -c 64k` | 3,254,952 (+1.33%) | 2.5 s | 2.3 s |
| `-9` | 2,719,928 | 33.8 s | 33.1 s |
| `-9 -t 4 -c 1` | 2,736,446 (+0.61%) | 11.3 s | 11.2 s |
| `-9 -t 4 -c 256k` | 2,767,700 (+1.76%) | 10.0 s | 10.0 s |

A part starts its model from nothing, which costs compression (more at
`-9`, whose model learns more) and the set-up of the model (about 50 ms of
CPU time at `-1`); hence parts of 4 MB by default, for long streams. A
stream shorter than a part is one part, on one thread. Each thread has a
model: at `-9`, 64 MB of memory in one thread, 194 MB on 4.

### TSV

The TSV file holds the values of the syntax elements, with the CAVLC, CABAC
and Exp-Golomb coding undone: a macroblock's coefficients are numbers, not
coeff_token, level_prefix etc. or CABAC bins. `r` codes the values again,
with CAVLC or CABAC as the picture parameter set says, and the stream comes
out byte for byte as it was. To guarantee that, `t` restores each NAL unit
from its rows and compares; a NAL unit it cannot reproduce is stored in hex,
verbatim. That happens to

- NAL unit types other than slices (1, 5), SEI (6), SPS (7), PPS (8), access
  unit delimiters (9), end of sequence and stream (10, 11) and filler data
  (12): data partitions (2-4), SVC and MVC (14, 15, 20) and the rest
- NAL units with syntax errors or referring to a missing parameter set
- emulation prevention bytes the standard would not insert

Everything else of the H.264 standard (2D, single layer) is supported: CAVLC
and CABAC; I, P, B, SP and SI slices; frames, field pictures and MBAFF;
monochrome, 4:2:0, 4:2:2 and 4:4:4, also with separate colour planes; bit
depths up to 14; lossless coding, I_PCM; FMO and arbitrary slice order;
redundant pictures; scaling matrices.

Speed is about 1 MB of H.264 per second for `t` and 2 MB/s for `r`; the TSV
file is 10-20 times as large as the stream. `h264p_cm.md` is about
compressing it with context mixing: where CABAC spends its bits, and
contexts for each field (`tools/ctxexp.py` tries them out).

#### TSV format

One row per line: a name and tab-separated values. Absent values are `-`.
Rows of the stream, in stream order:

    h264p  1               first row, format version
    junk   HEX             bytes before the first start code
    nal    Z REF TYPE      a NAL unit: Z 0x00 bytes before its 00 00 01 start
                           code (zero_byte and the previous NAL unit's
                           trailing_zero_8bits), nal_ref_idc, nal_unit_type;
                           its rows follow
    nalx   Z HEX           a NAL unit with forbidden_zero_bit set, or empty
    hex    HEX             the NAL unit after its header byte, verbatim
    trail  N BITS          the trailing bits are not the expected ones: the
                           last N bits written, up to the rbsp_stop_one_bit,
                           are replaced by BITS
    czw    N               N cabac_zero_words
    x264_padding N         before a nal row: the slices from there on end
                           their CABAC data with x264's padding bit (below),
                           that slice being in frame N; - for no padding bit
    eof    N               N 0x00 bytes after the last NAL unit

x264 does not end its CABAC data quite like the standard: it sets the last
bit of the byte with the rbsp_stop_one_bit, after the alignment zero bits, to
bit (frame & 31) of 0x35a4e4f5, frame being the number of the picture in
coding order from the start of the encoding. It does the same with the
pcm_alignment_zero_bits before I_PCM samples. `t` recognizes the pattern in
the bits it sees and writes an x264_padding row where it starts or its phase
changes (in a stream cut from a longer one, or a concatenation); the frame
number then goes up with every slice that has first_mb_in_slice 0. These bits
are expected from then on, so trail rows and nonzero pcm alignment values are
only needed for bits that differ: an x264 stream from the start of the
encoding gets one x264_padding row and none of those.

The rows of SPS, PPS, SEI, AUD, filler data and slice headers are named after
their syntax elements in the standard (7.3) and come in bitstream order, one
row per syntax element, except for these groups:

    constraint_set_flags              constraint_set0..5_flag, reserved_zero_2bits: 8 bits
    seq_scaling_list, pic_scaling_list   present flag, the delta_scale values
    offset_for_ref_frame              all of them
    cpb                               bit_rate_value_minus1 cpb_size_value_minus1 cbr_flag
    run_length_minus1, slice_group_id all of them
    slice_group_rect                  top_left bottom_right
    sei_message                       payloadType payloadSize payload (hex)
    ff_bytes                          the number of ff_byte of filler data
    ref_pic_list_modification_l0/l1   flag, then each modification_of_pic_nums_idc
                                      with its argument, ending with 3
    pred_weight_l0/l1                 one row per reference index: luma_weight_flag
                                      [weight offset] chroma_weight_flag [Cb weight
                                      offset Cr weight offset]
    mmco                              one row per memory_management_control_operation,
                                      with its arguments; the last one is mmco 0

The slice data follows the slice header, a group of rows per macroblock in
decoding order:

    skip  N                 N skipped macroblocks (mb_skip_run, mb_skip_flag)
    mb    T X C P Q         a coded macroblock: mb_type T, numbered as in the
                            standard for the slice type (I 0-25, SI 0-26, P and
                            SP 0-30, B 0-48), transform_size_8x8_flag X,
                            intra_chroma_pred_mode C, coded_block_pattern P,
                            mb_qp_delta Q
    fld   F                 mb_field_decoding_flag (MBAFF), where coded
    pcm   A S...            I_PCM: the pcm_alignment_zero_bits as a number, XOR
                            x264's padding bit in the last one if expected,
                            and the samples: 256 Y, then Cb, Cr
    ipm   M...              Intra 4x4 or 8x8 prediction modes: -1 for
                            prev_intra_pred_mode_flag, else rem_intra_pred_mode
    sub   S S S S           sub_mb_type
    ref0  R...              ref_idx_l0 of each partition that has one
    ref1  R...              ref_idx_l1
    mvd0  X Y...            mvd_l0 of each partition or sub-partition
    mvd1  X Y...            mvd_l1
    res   CAT IDX C...      a block of coefficients that are not all zero, in
                            scan order up to the last nonzero one; blocks
                            without a row are zero

    CAT   block                    IDX                       coefficients
    0     Intra16x16 DC            0                         16
    1     Intra16x16 AC            luma4x4BlkIdx             15 (scan positions 1-15)
    2     luma 4x4                 luma4x4BlkIdx             16
    3     chroma DC                0 Cb, 1 Cr                4 (4:2:0), 8 (4:2:2)
    4     chroma AC                iCbCr * 4 (8 for 4:2:2)   15
                                   + chroma block index
    5     luma 8x8                 luma8x8BlkIdx             64, also with CAVLC
    6-9   Cb (4:4:4) as 0, 1, 2, 5
    10-13 Cr (4:4:4) as 0, 1, 2, 5

CAT is the ctxBlockCat of the standard. Two macroblocks of
`test/002/01.h264`: an I_16x16_0_1_0 one with luma and chroma DC
coefficients, and a B_Bi_Bi_16x8 one (two references in list 0, one in
list 1, so no ref_idx_l1):

    mb    5   -   0   -   13
    res   0   0   5   0   -1  -1  0   0   0   0   0   -1
    res   3   0   2   0   -1
    res   3   1   -1  0   1

    mb    20  -   -   0   -
    ref0  0   0
    mvd0  0   0   0   0
    mvd1  0   6   0   1

### Verification

`test/h264p.sh` round-trips every test stream both ways, `c` and `d`, `t` and
`r`, with `-s`, so all their NAL units are converted (except the data
partitions of `test/h264p/jm_dp.264`), and two concatenations: one in which
x264's padding bits change phase, and one that changes the picture size,
chroma format, bit depth, slice groups and field coding from stream to
stream.
The streams in `test/h264p` cover what the others don't (see `gen.sh` there).
The dumped values of all test streams were also compared, macroblock by
macroblock, with the syntax elements traced by the JM reference decoder;
`t` and `r` were fuzzed with corrupted streams and TSV files, and `c` and
`d` (built with ASan and UBSan, which report nothing on the test streams)
with corrupted streams, compressed files and random concatenations of the
test streams. The tuning and release builds of `h264p_src/mk.sh`, and gcc
and clang builds at -O2 and -O3, code the test streams identically.
