#!/bin/bash
# How the streams in this directory were made (documentation; the streams and
# md5.txt are committed, so this does not need to be run).
#
# x264_*.264: ffmpeg 6.1 with libx264, from the first 120 frames of
#             test/002/00.h264 scaled to 640x352.
# jm_*.264:   JM 19 reference encoder (lencod, path in $LENCOD), from frames
#             810-839 of test/002/01.h264 scaled to 176x144 (JM_CFG = JM's cfg
#             directory), with features x264
#             never uses (explicit weighted bi-prediction, temporal direct with
#             direct_8x8_inference_flag 0, POC types 1 and 2, long-term refs, ...).
#
# md5.txt holds the MD5 of each stream's decoded yuv420p output; ffmpeg and the
# JM decoder (ldecod) both produce exactly that output for every stream.
set -e
cd "$(dirname "$0")"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

ffmpeg -v error -i ../002/00.h264 -fps_mode passthrough -frames:v 120 -vf scale=640:352 \
  -f rawvideo -pix_fmt yuv420p "$tmp/src.yuv"
x264() {
  name=$1; shift
  ffmpeg -v error -y -f rawvideo -pix_fmt yuv420p -s 640x352 -r 25 -i "$tmp/src.yuv" "$@" -an -f h264 x264_$name.264
}
B="-c:v libx264 -preset medium -crf 24"
x264 base             $B
x264 cavlc_b          $B -x264-params cabac=0:bframes=3:b-pyramid=normal:weightp=2
x264 temporal         $B -x264-params direct=temporal:bframes=3:b-pyramid=normal
x264 temporal_cavlc   $B -x264-params direct=temporal:cabac=0:bframes=3
x264 temporal_nopyr   $B -x264-params direct=temporal:bframes=5:b-pyramid=none
x264 slices4          $B -x264-params slices=4:bframes=3
x264 slicemax         $B -x264-params slice-max-size=1500:bframes=3
x264 cqm_jvt          $B -profile:v high -x264-params cqm=jvt:8x8dct=1
x264 noweight         $B -x264-params weightb=0:weightp=0:bframes=3
x264 weightp1         $B -x264-params weightp=1:bframes=3
x264 weightp2_nob     $B -x264-params weightp=2:bframes=0:ref=6
x264 bf16_ref16       $B -x264-params bframes=16:ref=16:b-adapt=2:b-pyramid=normal
x264 pyr_strict       $B -x264-params bframes=4:b-pyramid=strict
x264 pyr_none         $B -x264-params bframes=4:b-pyramid=none
x264 open_gop         $B -x264-params open-gop=1:keyint=30:bframes=3
x264 keyint12         $B -x264-params keyint=12:min-keyint=1:bframes=3:scenecut=80
x264 parts_all        -c:v libx264 -preset placebo -crf 22 -x264-params partitions=all:8x8dct=1:ref=8:bframes=8
x264 constr_intra     $B -x264-params constrained-intra=1:bframes=3
x264 intra_refresh    $B -x264-params intra-refresh=1:bframes=0:keyint=40
x264 deblock_m3p3     $B -x264-params deblock=-3,3:bframes=3
x264 deblock_p6m6     $B -x264-params deblock=6,-6:bframes=3
x264 nodeblock        $B -x264-params no-deblock=1:bframes=3
x264 chromaqp         $B -x264-params chroma-qp-offset=-5:bframes=3
x264 qp45             -c:v libx264 -preset medium -qp 45 -x264-params bframes=3
x264 qp4              -c:v libx264 -preset medium -qp 4 -x264-params bframes=3
x264 baseline         -c:v libx264 -preset medium -crf 24 -profile:v baseline
x264 baseline_slices  -c:v libx264 -preset medium -crf 24 -profile:v baseline -x264-params slices=3:ref=4
x264 main_nodct       -c:v libx264 -preset medium -crf 24 -profile:v main -x264-params bframes=3
x264 aud_sps          $B -x264-params aud=1:repeat-headers=1:bframes=3
x264 nombtree_aq      $B -x264-params mbtree=0:aq-mode=2:bframes=3
x264 fastdecode       -c:v libx264 -preset medium -tune fastdecode -crf 24
x264 zerolatency      -c:v libx264 -preset medium -tune zerolatency -crf 24
x264 ultrafast        -c:v libx264 -preset ultrafast -crf 24

[ -x "$LENCOD" ] && [ -d "$JM_CFG" ] || { echo "set LENCOD and JM_CFG to make the jm_* streams"; exit 0; }
ffmpeg -v error -i ../002/01.h264 -fps_mode passthrough -vf "select='between(n,810,839)',scale=176:144" \
  -fps_mode passthrough -f rawvideo -pix_fmt yuv420p "$tmp/qcif.yuv"
jm() {
  name=$1; shift
  "$LENCOD" -d "$JM_CFG/encoder_main.cfg" -p InputFile="$tmp/qcif.yuv" \
    -p FramesToBeEncoded=30 -p SourceWidth=176 -p SourceHeight=144 -p OutputFile=jm_$name.264 \
    -p ReconFile=/dev/null -p LevelIDC=30 -p QPISlice=30 -p QPPSlice=31 -p QPBSlice=32 "$@" > /dev/null
}
B="-p NumberBFrames=2 -p NumberReferenceFrames=4"
jm wbp_explicit           $B -p WeightedPrediction=1 -p WeightedBiprediction=1
jm wbp_explicit_cavlc     $B -p WeightedPrediction=1 -p WeightedBiprediction=1 -p SymbolMode=0
jm wbp_implicit           $B -p WeightedBiprediction=2
jm direct_t_noinfer       $B -p DirectModeType=0 -p DirectInferenceFlag=0
jm direct_s_noinfer       $B -p DirectModeType=1 -p DirectInferenceFlag=0
jm direct_t_noinfer_cavlc $B -p DirectModeType=0 -p DirectInferenceFlag=0 -p SymbolMode=0
jm poc1                   $B -p PicOrderCntType=1
jm poc2                   -p NumberBFrames=0 -p NumberReferenceFrames=4 -p PicOrderCntType=2
jm hier_b                 -p NumberBFrames=3 -p NumberReferenceFrames=4 -p HierarchicalCoding=2 -p BReferencePictures=1
jm longterm               $B -p SetFirstAsLongTerm=1
jm slices                 $B -p SliceMode=1 -p SliceArgument=20
jm high8x8_cabac          $B -p ProfileIDC=100 -p Transform8x8Mode=1
jm high8x8_cavlc          $B -p ProfileIDC=100 -p Transform8x8Mode=1 -p SymbolMode=0
jm high_qmatrix           $B -p ProfileIDC=100 -p Transform8x8Mode=1 -p ScalingMatrixPresentFlag=3 \
                             -p QmatrixFile="$JM_CFG/q_matrix.cfg"
jm constrained_intra      $B -p UseConstrainedIntraPred=1
