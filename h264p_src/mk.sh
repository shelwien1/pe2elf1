#!/bin/sh
#  Regenerate MOD/ from IDX/ and build h264p, level by level.
#
#      ./mk.sh release        shipping build of every level -- every parameter folded
#      ./mk.sh mod            regenerate MOD/ in the shipping form, build nothing
#      ./mk.sh tuning L       tuning build of level -L alone -- Debug 1, Const 0:
#                             every knob of IDX/lL live (MOD/lL in the tuning form)
#      ./mk.sh check L        the tuning and the shipping build of -L compress
#                             the streams of test/002 to the same bytes
#
#  Each level has its own IDX sources, IDX/l<L>/<family>.idx with its
#  template IDX/l<L>/<family>.inc (format: IDX/IDX-FORMAT.md), which
#  IDX/idx2inc.pl turns into MOD/l<L>/<family>_h.inc and _p.inc; lv<L>.inc
#  wraps each family's constants into the class the model's instances for
#  the level take (famcfg.inc; h264p_levels.md section 7).  MOD/ is a build
#  input kept in the repository, in the shipping form, so that a plain
#  `make` needs no perl.  Run ./mk.sh mod after editing IDX/.
#
#  The tuning build embeds every pattern of its level in the binary behind a
#  "!MAP!" marker, where IDX/opt.pl finds it and hill-climbs it in place;
#  the binary holds that level alone, so that its patterns are all opt.pl
#  finds and `c` runs it:
#
#      ./mk.sh tuning 1
#      perl IDX/opt.pl opt.lst ./h264p          # one stream path per line
#      cd IDX/l1 && for f in *.idx; do perl ../import.pl $f ../../export.\!\!\! > t && mv t $f; done
#      cd ../.. && ./mk.sh mod && ./mk.sh check 1
#
#  Both builds must code identically.  A stream coded by a tuning binary that
#  opt.pl has patched decodes only with that binary: the parameters are part
#  of the format.

set -e
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$here"

CXX=${CXX:-g++}
CXXFLAGS=${CXXFLAGS:--O2}
PARCH=${PARCH--march=x86-64-v3 -ffp-contract=off}  # as the Makefile's
FAMS="nal hdr skp mbt cbp ipm mvs qpd cbf sig lvl sgn mvg qpg pcm"
LEVELS="9 7 5 3 1"

cleanup() {
  for l in $LEVELS; do
    for f in $FAMS; do
      rm -f "IDX/l$l/$f-const.idx" "IDX/l$l/$f-const.inc" "IDX/l$l/${f}_h.inc" "IDX/l$l/${f}_p.inc" \
            "IDX/l$l/$f-const_h.inc" "IDX/l$l/$f-const_p.inc"
    done
  done
}
trap cleanup EXIT

#  $1 = the level, $2 = UseNew (1 pointers mapped at init, 0 fixed arrays),
#  $3 = 1 to fold every parameter to a literal (IDX-FORMAT.md sec.1)
generate() {
  mkdir -p "MOD/l$1"
  for f in $FAMS; do
    s=$f
    if [ "$3" = 1 ]; then
      sed 's/^Const 0/Const 1/' "IDX/l$1/$f.idx" > "IDX/l$1/$f-const.idx"
      cp -f "IDX/l$1/$f.inc" "IDX/l$1/$f-const.inc"
      s=$f-const
    fi
    ( cd "IDX/l$1" && IDX_NOCONST=0 perl ../idx2inc.pl "$s.idx" "$2" >/dev/null )
    for o in _h _p; do
      if [ ! -s "IDX/l$1/${s}${o}.inc" ]; then
        echo "mk.sh: IDX/idx2inc.pl wrote no l$1/${s}${o}.inc -- is IDX/l$1/$f.inc there?" >&2
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
  $CXX $CXXFLAGS $PARCH -std=c++11 -fwrapv "$@" -o "$out" h264p.cpp
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
    generate "$2" 1 0
    build ../h264p -DH264P_ONLY="$2"
    perl -e 'undef $/; $_ = <>;
             while( /!MAP!(.*?)!(.*?)\x00(.*?)\x00/gs ) { $n++; $b += length $3 }
             printf "mk.sh: tuning build of -%d -- %d tunable bits in %d patterns\n", $ARGV[0], $b, $n;
             exit($n ? 0 : 1)' ../h264p "$2"
    ;;
  release)
    for l in $LEVELS; do generate "$l" 0 1; done
    build ../h264p
    if grep -aq '!MAP!' ../h264p; then
      echo "mk.sh: the release build still carries !MAP! markers" >&2
      exit 1
    fi
    echo "mk.sh: release build -- every parameter folded"
    ;;
  mod)
    for l in $LEVELS; do generate "$l" 0 1; done
    ;;
  check)
    level "${2:?usage: ./mk.sh check L}"
    tmp=$(mktemp -d)
    trap 'rm -rf "$tmp"; cleanup' EXIT
    generate "$2" 1 0
    build "$tmp/tuning" -DH264P_ONLY="$2"
    generate "$2" 0 1
    build "$tmp/release" -DH264P_ONLY="$2"
    bad=0
    for s in ../test/002/*.h264; do
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
    echo "usage: ./mk.sh release | mod | tuning L | check L   (levels: $LEVELS)" >&2
    exit 2
    ;;
esac
