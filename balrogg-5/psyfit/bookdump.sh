#!/bin/sh
#  Build psyfit/bookgen.c once per libvorbis release and merge the dumps into
#  ../vbooks_gen.inc.  Run it from a directory holding the unpacked releases:
#
#      for v in 1.0beta4 1.0rc1 1.0rc2 1.0 1.0.1 1.1.0 1.1.1 1.1.2 \
#               1.2.0 1.2.2 1.2.3 1.3.1 1.3.2 1.3.3 1.3.4 1.3.5 1.3.6 1.3.7; do
#        curl -sSLO https://downloads.xiph.org/releases/vorbis/libvorbis-$v.tar.gz
#        tar xzf libvorbis-$v.tar.gz
#      done
#      git clone https://github.com/AO-Yumi/vorbis_aotuv.git aotuv
#      #  and the aoTuV betas from https://ao-yumi.github.io/aotuv_web/,
#      #  unpacked as aotuv-b1a, aotuv-b2, ... aotuv-b6.03_2015
#      git clone --depth 1 https://github.com/FFmpeg/FFmpeg.git ffmpeg
#      curl -sSLO https://downloads.xiph.org/releases/ogg/libogg-1.3.5.tar.gz
#      tar xzf libogg-1.3.5.tar.gz
#      (cd libogg-1.3.5 && ./configure -q --disable-shared --prefix=$PWD/../prefix \
#                       && make -s install)
#      sh /path/to/psyfit/bookdump.sh
#
#  A release is skipped, loudly, if it will not build -- the merge is a union,
#  so a missing one costs only the sets it alone could make.  vorbis-1.0b1
#  through b3 are skipped for having no encoder API at all; they also predate
#  the frozen Vorbis I bitstream, so nothing balrogg can read was written by
#  them.  1.0beta4 and 1.0rc1 are in the same position but build anyway, and
#  cost nothing to keep.
#
#  Encoders that are not libvorbis need a dumper of their own, and get one:
#  ffbookgen.c does ffmpeg's, and is run here too when an ffmpeg checkout is
#  beside the releases.  The aoTuV betas do not need one -- they replaced the
#  setup templates, not the tables, and add not one book.
here=`cd \`dirname "$0"\` && pwd`
work=${WORK:-./bookdump}
prefix=${PREFIX:-$PWD/prefix}
mkdir -p "$work"
built=0

for d in libvorbis-* aotuv aotuv-* vorbis-1.*; do
  [ -d "$d" ] || continue
  [ -f "$d/lib/vorbisenc.c" ] || { echo "skip $d: no encoder"; continue; }
  name=`echo "$d" | sed 's/^libvorbis-//;s/^vorbis-//'`

  set -- -O1 -w -std=gnu89 "-DBG_LIB=\"$name\""
  #  lengthlist went from long* to char* in 1.1; the header says which
  grep -q 'char *\* *lengthlist' "$d/lib/codebook.h" || set -- "$@" -DBG_LL_LONG
  #  the VBR entry point arrived in 1.0rc2; beta4 and rc1 have no declaration
  #  to call at all, which is what this flag is about -- rc2 declares it and
  #  returns OV_EIMPL, and needs no flag
  grep -q vorbis_encode_init_vbr "$d/include/vorbis/vorbisenc.h" \
    || set -- "$@" -DBG_NO_VBR
  set -- "$@" -I "$d/include" -I "$d/lib" -I "$prefix/include" \
              -o "$work/bookgen-$name" "$here/bookgen.c"

  #  The library's own sources, less the ones that are not part of it: the
  #  standalone tools, and the sources an older tree #includes into another
  #  (iir.c into envelope.c), which would otherwise be linked in twice.
  inc=`grep -ho '#include *"[a-z0-9_]*\.c"' "$d"/lib/*.c 2>/dev/null |
       sed 's/.*"\(.*\)"/\1/' | sort -u`
  for c in "$d"/lib/*.c; do
    b=`basename "$c"`
    case $b in vorbisfile.c|psytune.c|barkmel.c|tone.c) continue;; esac
    echo "$inc" | grep -qx "$b" && continue
    set -- "$@" "$c"
  done
  set -- "$@" "$prefix/lib/libogg.a" -lm

  if ! cc "$@" 2> "$work/build-$name.log"; then
    echo "skip $name: will not build (see $work/build-$name.log)"; continue
  fi
  if ! "$work/bookgen-$name" > "$work/$name.dump" 2> "$work/run-$name.log"; then
    echo "skip $name: dump failed (see $work/run-$name.log)"
    rm -f "$work/$name.dump";  continue
  fi
  echo "  `cat "$work/run-$name.log"`"
  built=`expr $built + 1`
done

#  ffmpeg's own encoder, if its source is here.  It chooses from nothing --
#  one fixed set of 29 books for every file it writes -- so there is no sweep,
#  just the one dump.
if [ -f ffmpeg/libavcodec/vorbis_enc_data.h ]; then
  if cc -O1 -w -I ffmpeg -o "$work/ffbookgen" "$here/ffbookgen.c" -lm \
       2> "$work/build-ffmpeg.log" &&
     "$work/ffbookgen" > "$work/ffmpeg.dump" 2> "$work/run-ffmpeg.log"; then
    echo "  `cat "$work/run-ffmpeg.log"`"
    built=`expr $built + 1`
  else
    echo "skip ffmpeg: see $work/build-ffmpeg.log and $work/run-ffmpeg.log"
    rm -f "$work/ffmpeg.dump"
  fi
else
  echo "skip ffmpeg: no checkout beside the releases"
fi

[ "$built" -gt 0 ] || { echo "nothing built"; exit 1; }
python3 "$here/bookmerge.py" "$work"/*.dump > "$here/../vbooks_gen.inc"
echo "wrote vbooks_gen.inc from $built releases"
