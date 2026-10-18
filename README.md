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
sources (`*.inc`), so `g++ -O2 -march=x86-64-v3 -std=c++11 -fwrapv
h264p_src/h264p.cpp -o h264p` builds it too. Its target is x86-64-v3 (AVX2:
Haswell, Zen and later), for which its pixel kernels and mixers have SIMD
code; built for any other target (`make PARCH=`, or without `-march`) it
uses the plain C++ beside them, which is slower and codes identically.

## Test

    make test

decodes 55 streams with h264dec and compares the MD5 of the output with
references that both ffmpeg and the JM reference decoder agree on:

- `test/002`: the 7 provided streams (x264, Main/High, B-frames, CABAC)
- `test/gen`: 48 small streams made from them with x264 and the JM encoder to
  cover more coding tools (see `test/gen/gen.sh`)

`test/run.sh ./h264dec file.264...` checks any other stream against ffmpeg.

It then round-trips these streams and the ones in `test/h264p` through
h264p (`test/h264p.sh`, which takes stream names too).

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

`-v` prints statistics, `-s` makes `c` and `t` fail when a NAL unit of a
supported type has to be stored verbatim. Input and output can be `-`
(stdin, stdout).

### Compression

`c` parses each NAL unit, writes it back from the values it parsed to see
that the NAL unit comes out the same, and then codes the values with the
model: the macroblock syntax (skip flags, mb_type, intra modes, motion,
coded_block_pattern, mb_qp_delta, the coefficients) and the header syntax.
`d` decodes the values and codes them with CAVLC or CABAC as the stream did.
A NAL unit that does not come out the same is stored verbatim, as in the TSV
file (below); a CRC-32 at the end checks the whole stream, and `d` takes
data after the end of the compressed stream for corruption.

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
are declared in `h264p_src/IDX/*.idx` and turned into code in
`h264p_src/MOD/` (see `h264p_src/mk.sh`), where they can also be tuned.
`h264p_cm.md` is the design.

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
| 00 | 815,140 | 567,469 | 69.6% |
| 01 | 1,095,103 | 804,089 | 73.4% |
| 02 | 28,143 | 26,254 | 93.3% |
| 03 | 1,181,369 | 774,624 | 65.6% |
| 04 | 484,238 | 383,693 | 79.2% |

The macroblock syntax takes 72.1% of what its CABAC coding takes (from
3,536,129 to 2,550,876 bytes over these streams; whole streams go from
3,603,993 to 2,556,129 bytes; `h264p_cm.md` section 9 has the figures by
syntax element). Speed is 0.03-0.13 MB of H.264 per second both ways, depending
on the stream, and the model tables take about 226 MB.

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
