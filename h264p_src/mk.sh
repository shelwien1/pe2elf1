#!/bin/sh
#  Regenerate MOD/ from IDX/ and build h264p.
#
#      ./mk.sh              tuning build   -- Debug 1, Const 0: every knob live
#      ./mk.sh release      shipping build -- every parameter folded
#      ./mk.sh mod          regenerate MOD/ in the shipping form, build nothing
#
#  The model's context indices and parameters are declared in IDX/<family>.idx
#  (format: IDX/IDX-FORMAT.md); IDX/idx2inc.pl turns each, with its template
#  IDX/<family>.inc, into MOD/<family>_h.inc and MOD/<family>_p.inc.  MOD/ is
#  a build input kept in the repository, in the shipping form, so that a
#  plain `make` needs no perl.  Run ./mk.sh mod after editing IDX/.
#
#  The tuning build embeds every pattern in the binary behind a "!MAP!"
#  marker, where IDX/opt.pl finds it and hill-climbs it in place:
#
#      ./mk.sh
#      perl IDX/opt.pl opt.lst ./h264p          # one stream path per line
#      cd IDX && for f in *.idx; do perl import.pl $f ../export.\!\!\! > t && mv t $f; done
#      cd .. && ./mk.sh mod
#
#  Both builds must code identically.  A stream coded by a tuning binary that
#  opt.pl has patched decodes only with that binary: the parameters are part
#  of the format.

set -e
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$here"

CXX=${CXX:-g++}
CXXFLAGS=${CXXFLAGS:--O2}
FAMS="nal hdr skp mbt cbp ipm mvs qpd cbf sig lvl sgn mvg pcm"

cleanup() {
  for f in $FAMS; do
    rm -f "IDX/$f-const.idx" "IDX/$f-const.inc" "IDX/${f}_h.inc" "IDX/${f}_p.inc" \
          "IDX/$f-const_h.inc" "IDX/$f-const_p.inc"
  done
}
trap cleanup EXIT

#  $1 = UseNew (1 pointers mapped at init, 0 fixed arrays), $2 = 1 to fold
#  every parameter to a literal (IDX-FORMAT.md sec.1)
generate() {
  mkdir -p MOD
  for f in $FAMS; do
    s=$f
    if [ "$2" = 1 ]; then
      sed 's/^Const 0/Const 1/' "IDX/$f.idx" > "IDX/$f-const.idx"
      cp -f "IDX/$f.inc" "IDX/$f-const.inc"
      s=$f-const
    fi
    ( cd IDX && IDX_NOCONST=0 perl idx2inc.pl "$s.idx" "$1" >/dev/null )
    for o in _h _p; do
      if [ ! -s "IDX/${s}${o}.inc" ]; then
        echo "mk.sh: IDX/idx2inc.pl wrote no ${s}${o}.inc -- is IDX/$f.inc there?" >&2
        exit 1
      fi
    done
    mv -f "IDX/${s}_h.inc" "MOD/${f}_h.inc"
    mv -f "IDX/${s}_p.inc" "MOD/${f}_p.inc"
  done
}

build() {
  $CXX $CXXFLAGS -std=c++11 -fwrapv -o ../h264p h264p.cpp
}

case "${1:-tuning}" in
  tuning)
    generate 1 0
    build
    perl -e 'undef $/; $_ = <>;
             while( /!MAP!(.*?)!(.*?)\x00(.*?)\x00/gs ) { $n++; $b += length $3 }
             printf "mk.sh: tuning build -- %d tunable bits in %d patterns\n", $b, $n;
             exit($n ? 0 : 1)' ../h264p
    ;;
  release)
    generate 0 1
    build
    if grep -aq '!MAP!' ../h264p; then
      echo "mk.sh: the release build still carries !MAP! markers" >&2
      exit 1
    fi
    echo "mk.sh: release build -- every parameter folded"
    ;;
  mod)
    generate 0 1
    ;;
  *)
    echo "usage: ./mk.sh [tuning|release|mod]" >&2
    exit 2
    ;;
esac
