#!/bin/sh
#  Regenerate mp3c's MOD/ from IDX/ and build mp3c (aacpack's mk.sh, for
#  mp3c's model).
#
#      ./mk.sh release          shipping build -- every parameter folded: ./mp3c
#      ./mk.sh mod              regenerate MOD/ in the shipping form, build nothing
#      ./mk.sh tuning           tuning build: ./mp3c.tune, the knobs the .idx
#                               sources state live (Const 0) behind "!MAP!"
#      ./mk.sh check DUMP...    the tuning and the shipping build compress the
#                               dumps (mp3rw c's .dmp files) to the same bytes
#      ./mk.sh mp3pack          ./mp3pack: mp3rw and mp3c's model in aacpack's
#                               front end (mp3pack.cpp, mp3pack_src/), MOD/ in the
#                               shipping form
#
#  The model's IDX sources are mp3rw/mp3_model/IDX/<module>.idx, each with
#  its template <module>.inc (format: mp3rw/mp3_model/IDX/IDX-FORMAT.md),
#  which IDX/idx2inc.pl turns into mp3rw/mp3_model/MOD/<module>_h.inc and
#  _p.inc.  MOD/ is a build input kept in the repository, in the shipping
#  form, so that g.bat and gc.bat need no perl.  Run ./mk.sh mod after
#  editing IDX/.
#
#  The tuning build embeds every live pattern in the binary behind a "!MAP!"
#  marker, where IDX/opt.pl finds it and hill-climbs it in place:
#
#      ./mk.sh tuning
#      cp mp3c.tune mp3c.opt                             # (opt.pl patches the binary it is given)
#      perl mp3rw/mp3_model/IDX/opt.pl opt.lst mp3c.opt  # one .dmp path per line
#      cd mp3rw/mp3_model/IDX && for f in *.idx; do perl import.pl $f ../../../export.\!\!\! > t && mv t $f; done
#      cd ../../.. && ./mk.sh mod && ./mk.sh check dump...
#
#  A knob is live when its line is under Const 0 (or toggled with "-"), and
#  frozen out of opt.pl's search with "!" (IDX-FORMAT.md sec.10).  Both
#  builds must code identically.  A file coded by a tuning binary that
#  opt.pl has patched decodes only with that binary: the parameters are part
#  of the format.

set -e
orig=$(pwd)
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$here"

IDX=mp3rw/mp3_model/IDX
MOD=mp3rw/mp3_model/MOD
MODS="sh_model sh_model_counts sh_model_freq sh_modelu sh_modelw"
CXX=${CXX:-g++}
CXXFLAGS=${CXXFLAGS:--O2}

cleanup() {
  for f in $MODS; do rm -f "$IDX/${f}_h.inc" "$IDX/${f}_p.inc"; done
}
trap cleanup EXIT
# (a signal ends the script through exit, so that the EXIT trap runs)
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 129' HUP

# the tunable patterns in the binary $1 (said, as what $2 is)
count_maps() {
  perl -e 'undef $/; $_ = <>;
           while( /!MAP!(.*?)!(.*?)\x00(.*?)\x00/gs ) { $n++; $b += length $3 }
           printf "mk.sh: %s -- %d tunable bits in %d patterns\n", $ARGV[0], $b, $n;' "$1" "$2"
}

#  $1 = UseNew (1 tables allocated as the model starts, 0 fixed arrays),
#  $2 = 1 to fold every parameter to a literal (IDX-FORMAT.md sec.1:
#  idx2inc.pl's IDX_NOCONST=1, which folds every line of the .idx as it is)
generate() {
  for f in $MODS; do
    ( cd "$IDX" && IDX_NOCONST=$2 perl idx2inc.pl "$f.idx" "$1" >/dev/null )
    for o in _h _p; do
      if [ ! -s "$IDX/${f}${o}.inc" ]; then
        echo "mk.sh: IDX/idx2inc.pl wrote no ${f}${o}.inc -- is $IDX/$f.inc there?" >&2
        exit 1
      fi
    done
    if [ "$2" = 1 ] && grep -qE '^[pm](desc|mask2?)\(' "$IDX/${f}_h.inc"; then
      echo "mk.sh: $f.idx: a parameter not folded in the shipping form" >&2
      exit 1
    fi
    mv -f "$IDX/${f}_h.inc" "$MOD/${f}_h.inc"
    mv -f "$IDX/${f}_p.inc" "$MOD/${f}_p.inc"
  done
}

#  $1 = the binary; Linux has no __max and __min (MSVC's and MinGW's
#  headers define them)
build() {
  $CXX $CXXFLAGS -std=gnu++11 -w -D'__max(a,b)=(((a)>(b))?(a):(b))' -D'__min(a,b)=(((a)<(b))?(a):(b))' \
    -DNDEBUG -ILib3 -Imp3rw -DINC_LOG2I mp3c.cpp -o "$1"
}

#  $1 = the binary: mp3pack, one translation unit (mp3pack.cpp) that takes
#  neither C++ exceptions nor RTTI, as aacpack; -fwrapv as aacpack builds
#  (mp3c's model counts on signed integers that wrap)
build_mp3pack() {
  $CXX $CXXFLAGS -std=gnu++11 -fwrapv -fno-exceptions -fno-rtti -Wno-invalid-offsetof -DNDEBUG \
    -I. -ILib3 -Imp3rw -pthread mp3pack.cpp -o "$1"
}

#  $1 = a path as given, relative to where mk.sh was run
abspath() {
  case $1 in
    /*) echo "$1" ;;
    *) echo "$orig/$1" ;;
  esac
}

case "${1:-}" in
  tuning)
    # (MOD/ back in the shipping form once the build has it)
    trap 'generate 0 1; cleanup' EXIT
    generate 1 0
    build mp3c.tune
    count_maps mp3c.tune "the tuning build"
    ;;
  release)
    generate 0 1
    build mp3c
    if grep -aq '!MAP!' mp3c; then
      echo "mk.sh: the release build still carries !MAP! markers" >&2
      exit 1
    fi
    echo "mk.sh: release build -- every parameter folded"
    ;;
  mod)
    generate 0 1
    ;;
  mp3pack)
    generate 0 1
    build_mp3pack mp3pack
    echo "mk.sh: ./mp3pack"
    ;;
  check)
    shift
    [ $# -gt 0 ] || { echo "usage: ./mk.sh check DUMP..." >&2; exit 2; }
    tmp=$(mktemp -d)
    trap 'rm -rf "$tmp"; generate 0 1; cleanup' EXIT
    generate 1 0
    build "$tmp/tuning"
    generate 0 1
    build "$tmp/release"
    bad=0
    for s in "$@"; do
      s=$(abspath "$s")
      "$tmp/tuning" c "$s" "$tmp/a"
      "$tmp/release" c "$s" "$tmp/b"
      if cmp -s "$tmp/a" "$tmp/b"; then
        echo "mk.sh: $s: the same"
      else
        echo "mk.sh: $s: the builds differ" >&2
        bad=1
      fi
    done
    exit $bad
    ;;
  *)
    echo "usage: ./mk.sh release | mod | tuning | check DUMP... | mp3pack" >&2
    exit 2
    ;;
esac
