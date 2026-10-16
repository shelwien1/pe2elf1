# h264dec

H.264 decoder: the OpenH264 decoder and its console front end (upstream
commit 0819f288 from 2022, C code only, flattened into `codec/`), with fixes
that make it decode correctly.

## Build

Linux (gcc or clang):

    make                        # -> ./h264dec
    make CXXFLAGS="-O0 -g"      # debug build

Windows (MinGW): `g.bat`, as before.

## Use

    ./h264dec input.264 output.yuv

writes the decoded frames as raw yuv420p in display order, cropped as the
stream specifies. Further options: `-trace N`, `-ec N`, `-length FILE`,
`-options FILE`, `-legacy`.

## Test

    make test

decodes 55 streams and compares the MD5 of the output with references that
both ffmpeg and the JM reference decoder agree on:

- `test/002`: the 7 provided streams (x264, Main/High, B-frames, CABAC)
- `test/gen`: 48 small streams made from them with x264 and the JM encoder to
  cover more coding tools (see `test/gen/gen.sh`)

`test/run.sh ./h264dec file.264...` checks any other stream against ffmpeg.

## Fixes

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

## Limitations (inherited from OpenH264)

- progressive 8-bit 4:2:0 only; interlaced streams (PAFF/MBAFF) are rejected
- FMO / arbitrary slice order: slices are deblocked one at a time instead of
  the whole picture in MB order, which is wrong when slices interleave
- the multi-threaded decoding mode of the library is not used by h264dec and
  was not tested
