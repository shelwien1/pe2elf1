#!/bin/bash
# How the streams in this directory were made (documentation; the streams are
# committed, so this does not need to be run). They cover coding tools that
# matter to h264p but not to h264dec, which decodes none of them: interlace
# (MBAFF, field pictures), 4:2:2, 4:4:4, monochrome, high bit depth, lossless,
# I_PCM, cabac_zero_words, filler data, FMO, SP slices and data partitioning.
#
# x264_*.264: ffmpeg 6.1 with libx264, from frames 810-841 of
#             test/002/01.h264 scaled to 320x192; the interlaced source has
#             its bottom fields taken from 8 frames later, so that many
#             macroblock pairs are field coded.
# jm_*.264:   JM 19 reference encoder (lencod, path in $LENCOD; JM_CFG = JM's
#             cfg directory), the same frames at CIF.
#
# Every stream round-trips through h264p with all NAL units converted except
# for the data partitions of jm_dp.264, and h264p's dump of each one matches
# the syntax element values traced by the JM decoder.
set -e
cd "$(dirname "$0")"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

ffmpeg -v error -i ../002/01.h264 -fps_mode passthrough -vf "select='between(n,810,841)',scale=320:192" \
  -fps_mode passthrough -f rawvideo -pix_fmt yuv420p "$tmp/prog.yuv"
python3 - "$tmp" <<'EOF'
import os, sys
tmp = sys.argv[1]
W, H = 320, 192
fs = W * H * 3 // 2
src = open(os.path.join(tmp, 'prog.yuv'), 'rb').read()
out = bytearray()
for i in range(len(src) // fs - 8):
    f = bytearray(src[i * fs:(i + 1) * fs])
    b = src[(i + 8) * fs:(i + 9) * fs]
    for w, h, base in ((W, H, 0), (W // 2, H // 2, W * H), (W // 2, H // 2, W * H * 5 // 4)):
        for y in range(1, h, 2):
            o = base + y * w
            f[o:o + w] = b[o:o + w]
    out += f
open(os.path.join(tmp, 'ilace.yuv'), 'wb').write(out)
# incompressible content: x264 codes it with I_PCM at qp 0
open(os.path.join(tmp, 'noise.yuv'), 'wb').write(os.urandom(64 * 64 * 3 // 2 * 2))
EOF

x264() {
  name=$1 src=$2; shift 2
  ffmpeg -v error -y -f rawvideo -pix_fmt yuv420p -s 320x192 -r 25 -i "$tmp/$src.yuv" -frames:v 12 \
    -c:v libx264 "$@" -an -f h264 x264_$name.264
}
B="-preset medium -crf 24"
x264 mbaff          ilace $B -x264-params interlaced=1:tff=1:bframes=3
x264 mbaff_cavlc    ilace $B -x264-params interlaced=1:tff=1:bframes=3:cabac=0
x264 mbaff_8x8      ilace -preset slow -crf 22 -x264-params interlaced=1:bff=1:bframes=2:8x8dct=1:ref=4:weightp=2:partitions=all
x264 mbaff_422_10   ilace $B -pix_fmt yuv422p10le -x264-params interlaced=1:bframes=2
x264 mbaff_444_cavlc ilace $B -pix_fmt yuv444p -x264-params interlaced=1:bframes=2:cabac=0
x264 422            prog $B -pix_fmt yuv422p -x264-params bframes=3
x264 422_cavlc      prog $B -pix_fmt yuv422p -x264-params bframes=3:cabac=0
x264 420_10         prog $B -pix_fmt yuv420p10le -x264-params bframes=3
x264 422_10_cavlc   prog $B -pix_fmt yuv422p10le -x264-params bframes=2:cabac=0
x264 444            prog $B -pix_fmt yuv444p -x264-params bframes=3
x264 444_cavlc      prog $B -pix_fmt yuv444p -x264-params bframes=3:cabac=0
x264 444_8x8        prog -preset slow -crf 20 -pix_fmt yuv444p -x264-params bframes=2:8x8dct=1:partitions=all
x264 444_10_cavlc   prog $B -pix_fmt yuv444p10le -x264-params cabac=0
x264 gray           prog $B -pix_fmt gray -x264-params bframes=3
x264 gray_cavlc     prog $B -pix_fmt gray -x264-params cabac=0
x264 filler         prog -preset fast -b:v 600k -frames:v 4 -x264-params nal-hrd=cbr:vbv-maxrate=600:vbv-bufsize=300:filler=1
x264 lossless       prog -preset fast -qp 0 -frames:v 3 -x264-params bframes=1
x264 lossless_444_cavlc prog -preset fast -qp 0 -frames:v 3 -pix_fmt yuv444p -x264-params cabac=0
noise() {
  name=$1; shift
  ffmpeg -v error -y -f rawvideo -pix_fmt yuv420p -s 64x64 -i "$tmp/noise.yuv" -c:v libx264 -qp 0 "$@" \
    -an -f h264 x264_$name.264
}
noise pcm
noise pcm_cavlc -x264-params cabac=0
noise pcm_10 -pix_fmt yuv420p10le

# cabac_zero_words (0x0000, 00 00 03 when escaped) appended to the slices
python3 - x264_444.264 x264_czw.264 <<'EOF'
import sys
data = open(sys.argv[1], 'rb').read()
parts = data.split(b'\x00\x00\x01')
out = parts[0]
for p in parts[1:]:
    if p and p[0] & 0x1f in (1, 5):
        z = len(p) - len(p.rstrip(b'\x00'))  # zero_byte of the next start code
        p = p[:len(p) - z] + b'\x00\x00\x03' * 3 + b'\x00' * z
    out += b'\x00\x00\x01' + p
open(sys.argv[2], 'wb').write(out)
EOF

[ -x "$LENCOD" ] && [ -d "$JM_CFG" ] || { echo "set LENCOD and JM_CFG to make the jm_* streams"; exit 0; }
ffmpeg -v error -i ../002/01.h264 -fps_mode passthrough -vf "select='between(n,810,841)',scale=352:288" \
  -fps_mode passthrough -f rawvideo -pix_fmt yuv420p "$tmp/cif.yuv"
ffmpeg -v error -f rawvideo -pix_fmt yuv420p -s 320x192 -i "$tmp/ilace.yuv" -vf scale=352:288:interl=1 \
  -f rawvideo -pix_fmt yuv420p "$tmp/cif_ilace.yuv"
ffmpeg -v error -f rawvideo -pix_fmt yuv420p -s 352x288 -i "$tmp/cif.yuv" -f rawvideo -pix_fmt yuv444p "$tmp/cif444.yuv"
jm() {
  name=$1 src=$2; shift 2
  (cd "$tmp" && "$LENCOD" -d "$JM_CFG/encoder_main.cfg" -p InputFile="$tmp/$src.yuv" -p FramesToBeEncoded=6 \
    -p SourceWidth=352 -p SourceHeight=288 -p OutputFile="$OLDPWD/jm_$name.264" -p ReconFile=/dev/null \
    -p LevelIDC=30 -p QPISlice=30 -p QPPSlice=31 -p QPBSlice=32 "$@" > /dev/null)
}
B="-p NumberBFrames=2 -p NumberReferenceFrames=3"
jm paff        cif_ilace $B -p PicInterlace=1
jm paff_cavlc  cif_ilace $B -p PicInterlace=1 -p SymbolMode=0
jm afrf        cif_ilace $B -p PicInterlace=2
for t in 0 1 2 3 4 5 6; do
  jm fmo$t cif -p ProfileIDC=66 -p SymbolMode=0 -p NumberBFrames=0 -p num_slice_groups_minus1=2 \
    -p slice_group_map_type=$t -p slice_group_change_direction_flag=1 -p slice_group_change_rate_minus1=20 \
    -p SliceGroupConfigFileName="$JM_CFG/sg${t}conf.cfg"
done
jm sp          cif -p ProfileIDC=88 -p SymbolMode=0 -p NumberBFrames=0 -p SPPicturePeriodicity=2 \
                   -p QPSPSlice=28 -p QPSP2Slice=27
jm dp          cif -p ProfileIDC=88 -p SymbolMode=0 -p NumberBFrames=0 -p PartitionMode=1
jm mono        cif -p ProfileIDC=100 -p YUVFormat=0 $B
jm mono_cavlc  cif -p ProfileIDC=100 -p YUVFormat=0 -p SymbolMode=0 $B
jm 444_planes  cif444 -p ProfileIDC=244 -p YUVFormat=3 -p SeparateColourPlane=1 $B
jm 444_planes_cavlc cif444 -p ProfileIDC=244 -p YUVFormat=3 -p SeparateColourPlane=1 -p SymbolMode=0 $B
