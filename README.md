# h264dec, h264p

- `h264dec`: H.264 decoder: the OpenH264 decoder and its console front end
  (upstream commit 0819f288 from 2022, C code only, flattened into `codec/`),
  with fixes that make it decode correctly.
- `h264p`: dumps the syntax of an H.264 stream to a TSV file with the entropy
  coding removed, and restores the identical stream from it (`h264p_src/`).

## Build

Linux (gcc or clang):

    make                        # -> ./h264dec ./h264p
    make CXXFLAGS="-O0 -g"      # debug build

Windows (MinGW): `g.bat`, as before, for h264dec, `gp.bat` for h264p.

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

    ./h264p c [options] input.264 output.tsv    # dump
    ./h264p d [options] input.tsv output.264    # restore

`-v` prints statistics, `-s` makes `c` fail when a NAL unit of a supported
type has to be stored verbatim. Input and output can be `-` (stdin, stdout).

The TSV file holds the values of the syntax elements, with the CAVLC, CABAC
and Exp-Golomb coding undone: a macroblock's coefficients are numbers, not
coeff_token, level_prefix etc. or CABAC bins. `d` codes the values again,
with CAVLC or CABAC as the picture parameter set says, and the stream comes
out byte for byte as it was. To guarantee that, `c` restores each NAL unit
from its rows and compares; a NAL unit it cannot reproduce is stored in hex,
verbatim. That happens to

- NAL unit types other than slices (1, 5), SEI (6), SPS (7), PPS (8), access
  unit delimiters (9), end of sequence and stream (10, 11): for example data
  partitions (2-4), SVC and MVC (14, 15, 20) and filler data (12)
- NAL units with syntax errors or referring to a missing parameter set
- emulation prevention bytes the standard would not insert

Everything else of the H.264 standard (2D, single layer) is supported: CAVLC
and CABAC; I, P, B, SP and SI slices; frames, field pictures and MBAFF;
monochrome, 4:2:0, 4:2:2 and 4:4:4, also with separate colour planes; bit
depths up to 14; lossless coding, I_PCM; FMO and arbitrary slice order;
redundant pictures; scaling matrices.

Speed is about 1 MB of H.264 per second for `c` and 2 MB/s for `d`; the TSV
file is 10-20 times as large as the stream.

### TSV format

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
    trail  N BITS          the trailing bits are not the standard ones: the
                           last N bits written, up to the rbsp_stop_one_bit,
                           are replaced by BITS (x264 sets a pseudo-random bit
                           after the CABAC stop bit, for example)
    czw    N               N cabac_zero_words
    eof    N               N 0x00 bytes after the last NAL unit

The rows of SPS, PPS, SEI, AUD and slice headers are named after their syntax
elements in the standard (7.3) and come in bitstream order, one row per
syntax element, except for these groups:

    constraint_set_flags              constraint_set0..5_flag, reserved_zero_2bits: 8 bits
    seq_scaling_list, pic_scaling_list   present flag, the delta_scale values
    offset_for_ref_frame              all of them
    cpb                               bit_rate_value_minus1 cpb_size_value_minus1 cbr_flag
    run_length_minus1, slice_group_id all of them
    slice_group_rect                  top_left bottom_right
    sei_message                       payloadType payloadSize payload (hex)
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
    pcm   A S...            I_PCM: value of the pcm_alignment_zero_bits (not 0 in
                            some x264 streams), samples: 256 Y, then Cb, Cr
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

`test/h264p.sh` round-trips every test stream with `-s`, so all their NAL
units are converted (except the data partitions of `test/h264p/jm_dp.264`).
The streams in `test/h264p` cover what the others don't (see `gen.sh` there).
The dumped values of all test streams were also compared, macroblock by
macroblock, with the syntax elements traced by the JM reference decoder, and
`c` and `d` were fuzzed with corrupted streams and TSV files.
