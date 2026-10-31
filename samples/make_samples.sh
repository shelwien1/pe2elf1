#!/bin/sh
#  Encode the ffmpeg-made part of the corpus into DIR: E-AC-3 as ffmpeg's eac3 encoder writes it
#  (bsid 16, one independent substream, AC-3's tools in E-AC-3's syntax, 1, 2, 3 or 6 blocks a
#  frame by the bit rate, 32/44.1/48 kHz) and AC-3 in each channel mode, with and without the LFE,
#  coupling, and the alternate bit stream syntax (bsid 6: xbsi1, xbsi2).  The sources are 20 s of
#  ss_sp3.ac3 (one of the AC-3 test files: stereo) and of matrix2_english_5.1_640.eac3 (5.1, from
#  get_samples.sh), decoded by ffmpeg.  The MD5s in samples.md5 are ffmpeg 6.1.1's.
#
#      ./make_samples.sh AC3DIR [DIR]      AC3DIR: the AC-3 test files; DIR: default ./corpus
set -e
ac3dir=${1:?usage: ./make_samples.sh AC3DIR [DIR]}
dir=${2:-corpus}
[ -s "$ac3dir/ss_sp3.ac3" ] || { echo "make_samples.sh: no $ac3dir/ss_sp3.ac3" >&2; exit 1; }
[ -s "$dir/matrix2_english_5.1_640.eac3" ] || { echo "make_samples.sh: run get_samples.sh $dir first" >&2; exit 1; }
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
ff() { ffmpeg -hide_banner -loglevel error -y -bitexact "$@"; }
ff -i "$ac3dir/ss_sp3.ac3" -t 20 "$tmp/st.wav"
ff -i "$dir/matrix2_english_5.1_640.eac3" -t 20 "$tmp/s51.wav"
e() {  # name source options...
  n=$1
  s=$2
  shift 2
  ff -i "$tmp/$s.wav" "$@" -f "${n##*.}" "$dir/$n"
}
#  E-AC-3: frames of 6 blocks up to 1024 kbps at 48 kHz, 3 to 2048, 2 to 3072, 1 to 6144
e ff_eac3_st_96k.eac3      st  -c:a eac3 -b:a 96k
e ff_eac3_st_1536k.eac3    st  -c:a eac3 -b:a 1536k
e ff_eac3_mono_64k.eac3    st  -c:a eac3 -b:a 64k -ac 1
e ff_eac3_441_st_192k.eac3 st  -c:a eac3 -b:a 192k -ar 44100
e ff_eac3_32_st_128k.eac3  st  -c:a eac3 -b:a 128k -ar 32000
e ff_eac3_51_448k.eac3     s51 -c:a eac3 -b:a 448k
e ff_eac3_51_3072k.eac3    s51 -c:a eac3 -b:a 3072k
e ff_eac3_51_6144k.eac3    s51 -c:a eac3 -b:a 6144k
#  AC-3: the channel modes (acmod 1-7; 0, dual mono, ffmpeg does not write), the LFE, 32 and 44.1
#  kHz (whose frames take a word of padding by turns), coupling, the alternate bsi
e ff_ac3_mono_96k.ac3      st  -c:a ac3 -b:a 96k -ac 1
e ff_ac3_441_st_192k.ac3   st  -c:a ac3 -b:a 192k -ar 44100
e ff_ac3_32_st_96k.ac3     st  -c:a ac3 -b:a 96k -ar 32000
e ff_ac3_st_cpl_96k.ac3    st  -c:a ac3 -b:a 96k -channel_coupling 1
e ff_ac3_st_xbsi_256k.ac3  st  -c:a ac3 -b:a 256k -dmix_mode ltrt -ltrt_cmixlev 0.707 -ad_conv_type hdcd -dsurex_mode on
e ff_ac3_21_192k.ac3       s51 -c:a ac3 -b:a 192k -af "pan=2.1|FL=FL|FR=FR|LFE=LFE"
e ff_ac3_30_256k.ac3       s51 -c:a ac3 -b:a 256k -af "pan=3.0|FL=FL|FR=FR|FC=FC"
e ff_ac3_21b_256k.ac3      s51 -c:a ac3 -b:a 256k -af "pan=3.0(back)|FL=FL|FR=FR|BC=0.5*SL+0.5*SR"
e ff_ac3_31_320k.ac3       s51 -c:a ac3 -b:a 320k -af "pan=4.0|FL=FL|FR=FR|FC=FC|BC=0.5*SL+0.5*SR"
e ff_ac3_22_320k.ac3       s51 -c:a ac3 -b:a 320k -af "pan=quad(side)|FL=FL|FR=FR|SL=SL|SR=SR"
e ff_ac3_50_384k.ac3       s51 -c:a ac3 -b:a 384k -af "pan=5.0(side)|FL=FL|FR=FR|FC=FC|SL=SL|SR=SR"
e ff_ac3_51_448k.ac3       s51 -c:a ac3 -b:a 448k
e ff_ac3_51_640k.ac3       s51 -c:a ac3 -b:a 640k -dialnorm -27 -room_type large -mixing_level 105
echo "make_samples.sh: $(ls "$dir"/ff_* | wc -l) files in $dir"
