#!/bin/sh
#  Regenerate MOD/ from IDX/ and build aacpack, level by level (as h264p's
#  mk.sh).
#
#      ./mk.sh release        shipping build of every level -- every parameter folded
#      ./mk.sh mod            regenerate MOD/ in the shipping form, build nothing
#      ./mk.sh tuning L       tuning build of level -L alone -- Debug 1, Const 0:
#                             every knob of IDX/lL live (MOD/lL in the tuning form)
#      ./mk.sh check L        the tuning and the shipping build of -L, and the
#                             shipping build without AVX2 (the plain C++
#                             paths), compress the given streams (or
#                             ../test/corpus/*) to the same bytes
#      ./mk.sh dump L DIR STREAM...
#                             what each family of -L codes compressing the
#                             streams, a directory of dumps per stream in DIR
#                             (aacpack built with -DAACPACK_DUMP; fam.inc)
#      ./mk.sh famtune L      ../famtune: the families of -L coding their
#                             dumps again, the tuning build (IDX/famtune.cpp)
#
#  Each level has its own IDX sources, IDX/l<L>/<family>.idx and the
#  template of the level's families IDX/l<L>/family.inc (format:
#  IDX/IDX-FORMAT.md), which IDX/idx2inc.pl turns into
#  MOD/l<L>/<family>_h.inc and _p.inc: a family's constants, tables, index
#  builder and the class the model's instances for the level take
#  (famcfg.inc), which lv<L>.inc includes.  MOD/ is a build input kept in
#  the repository, in the shipping form, so that a plain `make` needs no
#  perl.  Run ./mk.sh mod after editing IDX/.
#
#  The tuning build embeds every pattern of its level in the binary behind a
#  "!MAP!" marker, where IDX/opt.pl finds it and hill-climbs it in place;
#  the binary holds that level alone, so that its patterns are all opt.pl
#  finds and `c` runs it:
#
#      ./mk.sh tuning 9                         # ../aacpack, the tuning build
#      perl IDX/opt.pl opt.lst ../aacpack       # one stream path per line
#      cd IDX/l9 && for f in *.idx; do perl ../import.pl $f ../../export.\!\!\! > t && mv t $f; done
#      cd ../.. && ./mk.sh mod && ./mk.sh check 9
#
#  Faster, a family at a time against its dumps (IDX/ftune.py), since a
#  trial then codes only that family's decisions:
#
#      ./mk.sh dump 9 dump stream...
#      ./mk.sh famtune 9
#      python3 IDX/ftune.py ../famtune dump        # every family, four at a time
#      (then import the export as above)
#
#  Both builds must code identically.  A stream coded by a tuning binary that
#  opt.pl has patched decodes only with that binary: the parameters are part
#  of the format.

set -e
orig=$(pwd)
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$here"

CXX=${CXX:-g++}
CXXFLAGS=${CXXFLAGS:--O2}
PARCH=${PARCH--march=x86-64-v3 -ffp-contract=off}  # as the Makefile's
FAMS=$(sed -n 's/^#define AAC_FAMS(X) //p' famvars.inc | sed 's/X(\([a-z0-9]*\))/\1/g')  # (the model's list)
[ -n "$FAMS" ] || { echo "mk.sh: no AAC_FAMS in famvars.inc" >&2; exit 1; }
LEVELS="9"

cleanup() {
  for l in $LEVELS; do
    for f in $FAMS; do
      rm -f "IDX/l$l/$f-const.idx" "IDX/l$l/${f}_h.inc" "IDX/l$l/${f}_p.inc" \
            "IDX/l$l/$f-const_h.inc" "IDX/l$l/$f-const_p.inc"
    done
  done
}
trap cleanup EXIT
# (a signal ends the script through exit, so that the EXIT trap runs)
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 129' HUP

# the tunable patterns in the binary $1 (said, as what $2 is): 1 if none
count_maps() {
  perl -e 'undef $/; $_ = <>;
           while( /!MAP!(.*?)!(.*?)\x00(.*?)\x00/gs ) { $n++; $b += length $3 }
           printf "mk.sh: %s -- %d tunable bits in %d patterns\n", $ARGV[0], $b, $n;
           exit($n ? 0 : 1)' "$1" "$2"
}

#  $1 = the level, $2 = UseNew (1 pointers mapped at init, 0 fixed arrays),
#  $3 = 1 to fold every parameter to a literal (IDX-FORMAT.md sec.1)
generate() {
  mkdir -p "MOD/l$1"
  for f in $FAMS; do
    s=$f
    if [ "$3" = 1 ]; then
      sed 's/^Const 0/Const 1/' "IDX/l$1/$f.idx" > "IDX/l$1/$f-const.idx"
      s=$f-const
    fi
    ( cd "IDX/l$1" && IDX_NOCONST=0 perl ../idx2inc.pl "$s.idx" "$2" >/dev/null )
    for o in _h _p; do
      if [ ! -s "IDX/l$1/${s}${o}.inc" ]; then
        echo "mk.sh: IDX/idx2inc.pl wrote no l$1/${s}${o}.inc -- is IDX/l$1/family.inc there?" >&2
        exit 1
      fi
    done
    mv -f "IDX/l$1/${s}_h.inc" "MOD/l$1/${f}_h.inc"
    mv -f "IDX/l$1/${s}_p.inc" "MOD/l$1/${f}_p.inc"
  done
}

#  $1 = the binary, $2... more flags
build() {
  out=$1
  shift
  $CXX $CXXFLAGS $PARCH -std=c++11 -fwrapv -fno-exceptions -fno-rtti -ILib3 -pthread "$@" -o "$out" aacpack.cpp -lm
}

#  $1 = a path as given, relative to where mk.sh was run
abspath() {
  case $1 in
    /*) echo "$1" ;;
    *) echo "$orig/$1" ;;
  esac
}

level() {
  case " $LEVELS " in
    *" $1 "*) ;;
    *) echo "mk.sh: no level -$1 (levels: $LEVELS)" >&2; exit 2 ;;
  esac
}

case "${1:-}" in
  tuning)
    level "${2:?usage: ./mk.sh tuning L}"
    lv=$2
    # (MOD/lL back in the shipping form once the build has it)
    trap 'generate "$lv" 0 1; cleanup' EXIT
    generate "$2" 1 0
    build ../aacpack -DAACPACK_ONLY="$2"
    count_maps ../aacpack "tuning build of -$2"
    ;;
  release)
    for l in $LEVELS; do generate "$l" 0 1; done
    build ../aacpack
    if grep -aq '!MAP!' ../aacpack; then
      echo "mk.sh: the release build still carries !MAP! markers" >&2
      exit 1
    fi
    echo "mk.sh: release build -- every parameter folded"
    ;;
  mod)
    for l in $LEVELS; do generate "$l" 0 1; done
    ;;
  check)
    level "${2:?usage: ./mk.sh check L [STREAM...]}"
    lv=$2
    shift 2
    tmp=$(mktemp -d)
    trap 'rm -rf "$tmp"; generate "$lv" 0 1; cleanup' EXIT
    generate "$lv" 1 0
    build "$tmp/tuning" -DAACPACK_ONLY="$lv"
    generate "$lv" 0 1
    build "$tmp/release" -DAACPACK_ONLY="$lv"
    p=$PARCH
    PARCH=
    build "$tmp/plain" -DAACPACK_ONLY="$lv"
    PARCH=$p
    bad=0
    if [ $# -eq 0 ]; then set -- ../test/corpus/*; fi
    for s in "$@"; do
      case $s in /*) ;; *) s=$orig/$s ;; esac
      "$tmp/tuning" c "$s" "$tmp/a"
      "$tmp/release" c "$s" "$tmp/b"
      "$tmp/plain" c "$s" "$tmp/p"
      if cmp -s "$tmp/a" "$tmp/b" && cmp -s "$tmp/b" "$tmp/p"; then
        echo "mk.sh: $s: the same"
      else
        echo "mk.sh: $s: the builds differ" >&2
        bad=1
      fi
    done
    exit $bad
    ;;
  dump)
    level "${2:?usage: ./mk.sh dump L DIR STREAM...}"
    lv=$2
    dir=$(abspath "${3:?usage: ./mk.sh dump L DIR STREAM...}")
    shift 3
    tmp=$(mktemp -d)
    trap 'rm -rf "$tmp"; cleanup' EXIT
    build "$tmp/aacpack" -DAACPACK_ONLY="$lv" -DAACPACK_DUMP
    mkdir -p "$dir"
    n=0
    for s in "$@"; do
      d=$dir/$(printf 's%03d' $n)
      rm -rf "$d"
      mkdir -p "$d"
      AACPACK_DUMP=$d "$tmp/aacpack" c "$(abspath "$s")" "$tmp/c"
      n=$((n + 1))
    done
    echo "mk.sh: dumps of $n streams in $dir"
    ;;
  famtune)
    level "${2:?usage: ./mk.sh famtune L}"
    lv=$2
    tmp=$(mktemp -d)
    # (MOD/lL back in the shipping form however the build ends)
    trap 'rm -rf "$tmp"; generate "$lv" 0 1; cleanup' EXIT
    generate "$lv" 1 0
    $CXX $CXXFLAGS $PARCH -std=c++11 -fwrapv -fno-exceptions -fno-rtti -DAACPACK_ONLY="$lv" -o "$tmp/famtune" IDX/famtune.cpp -lm
    cp "$tmp/famtune" ../famtune
    count_maps ../famtune "../famtune for -$2"
    ;;
  *)
    echo "usage: ./mk.sh release | mod | tuning L | check L [STREAM...] | dump L DIR STREAM... | famtune L   (levels: $LEVELS)" >&2
    exit 2
    ;;
esac
