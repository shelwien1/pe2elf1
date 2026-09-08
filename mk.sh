#!/bin/sh
#  Regenerate MOD/ from IDX/ and build tsvcomp.
#
#      ./mk.sh              tuning build   -- Debug 1, Const 0
#      ./mk.sh release      shipping build -- every parameter folded
#      ./mk.sh check        build both and prove they code identically
#
#  IDX-FORMAT.md sec.1: Debug and Const are orthogonal flags, not two modes.
#  `Const 0` leaves each threshold a live `mapping` object; `Debug 1` makes its
#  descriptor start with "!MAP!", which is the marker IDX/opt.pl scans the
#  executable for.  So the tuning build is the one an optimizer can drive:
#
#      ./mk.sh
#      perl IDX/opt.pl opt.lst ./tsvcomp        # hill-climbs the binary itself
#      cd IDX && perl import.pl tsvcomp.idx ../export.\!\!\! > t && mv t tsvcomp.idx
#      ./mk.sh check                            # then ship what it found
#
#  The shipping build is derived from the same source with one substitution --
#  "Const 0" becomes "Const 1" in a copy -- so the two cannot drift apart, and
#  `./mk.sh check` is the test that they have not: whatever the parameters are,
#  both builds must produce byte-identical streams.  That is the format's
#  stated contract and the only check that catches a parameter which folds to
#  something different from what it evaluated to.
#
#  MOD/ is a build input, not a build artefact.  It ships generated -- in the
#  shipping form, so `make` works without perl -- and this script rewrites it.
#  A stale MOD/ compiles fine and codes differently, so run ./mk.sh after
#  editing IDX/tsvcomp.idx or IDX/tsvcomp.inc.

set -e

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$here"

CXX=${CXX:-c++}
CXXFLAGS=${CXXFLAGS:--O2}
WARN=${WARN:--Wall -Wextra}
REQ=-fwrapv

#  $1 = idx stem, $2 = UseNew (1 pointers, 0 fixed arrays), $3 = output binary
generate_and_build() {
  ( cd IDX && IDX_NOCONST=0 perl idx2inc.pl "$1.idx" "$2" >/dev/null )
  mkdir -p MOD
  mv -f "IDX/$1_h.inc" MOD/tsvcomp_h.inc
  mv -f "IDX/$1_p.inc" MOD/tsvcomp_p.inc
  $CXX $CXXFLAGS $WARN $REQ -o "$3" tsvcomp.cpp -lm
}

release_source() {
  #  IDX-FORMAT.md sec.1's one substitution, and the template alongside it.
  sed 's/^Const 0/Const 1/' IDX/tsvcomp.idx > IDX/tsvcomp-const.idx
  cp -f IDX/tsvcomp.inc IDX/tsvcomp-const.inc
}

case "${1:-tuning}" in
  tuning)
    generate_and_build tsvcomp 1 tsvcomp
    #  The marker is put there by the pdesc macro, so it is the binary that
    #  has to be looked at -- which is also what opt.pl looks at.
    n=$(grep -ac '!MAP!' tsvcomp || true)
    b=$(perl -ne 'BEGIN{$/=undef} $n+=length($3) while /!MAP!(.*?)!(.*?)\x00(.*?)\x00/gs; END{print $n+0}' tsvcomp)
    echo "mk.sh: tuning build -- $b tunable bits in $(perl -ne 'BEGIN{$/=undef} $n++ while /!MAP!/g; END{print $n+0}' tsvcomp) patterns, visible to IDX/opt.pl"
    ;;

  release)
    release_source
    generate_and_build tsvcomp-const 0 tsvcomp
    rm -f IDX/tsvcomp-const.idx IDX/tsvcomp-const.inc
    if grep -q '!MAP!' MOD/tsvcomp_h.inc; then
      echo "mk.sh: release build still carries !MAP! markers" >&2
      exit 1
    fi
    echo "mk.sh: release build -- every parameter folded"
    ;;

  check)
    #  Both builds, over whatever the list names, compared byte for byte.
    tmp=$(mktemp -d "${TMPDIR:-/tmp}/mk-check.XXXXXX")
    trap 'rm -rf "$tmp"' EXIT INT TERM
    lst=${2:-}
    if [ -z "$lst" ]; then
      #  No list given: make one from the bundled .ogg, so the contract can be
      #  checked in a fresh clone with nothing else to hand.  A real tuning
      #  corpus belongs in opt.lst; see IDX/opt.pl on what it should cover.
      [ -x ./balrogg ] || make balrogg
      ./balrogg c 00.ogg "$tmp/00.tsv"
      lst="$tmp/list"
      echo "$tmp/00.tsv" > "$lst"
    fi
    [ -f "$lst" ] || { echo "mk.sh check: no $lst -- one .tsv per line" >&2; exit 2; }
    generate_and_build tsvcomp 1 "$tmp/tune"
    release_source
    generate_and_build tsvcomp-const 0 "$tmp/rel"
    rm -f IDX/tsvcomp-const.idx IDX/tsvcomp-const.inc
    bad=0
    while IFS= read -r f; do
      case "$f" in ''|\#*) continue;; esac
      "$tmp/tune" c "$f" "$tmp/a.tc"
      "$tmp/rel"  c "$f" "$tmp/b.tc"
      if cmp -s "$tmp/a.tc" "$tmp/b.tc"; then
        printf '  %-40s %10s  identical\n' "$(basename "$f")" "$(wc -c < "$tmp/a.tc")"
      else
        printf '  %-40s DIFFERS\n' "$(basename "$f")"
        bad=1
      fi
    done < "$lst"
    #  Leave the tree in the shipping shape, which is what MOD/ is checked in as.
    cp -f "$tmp/rel" tsvcomp
    [ "$bad" = 0 ] && echo "mk.sh: tuning and shipping builds agree" \
                   || { echo "mk.sh: THE TWO BUILDS DISAGREE" >&2; exit 1; }
    ;;

  *)
    echo "usage: ./mk.sh [tuning|release|check [file-list]]" >&2
    exit 2
    ;;
esac
