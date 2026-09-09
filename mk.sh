#!/bin/sh
#  Regenerate MOD/ from IDX/ and build tsvcomp.
#
#      ./mk.sh              tuning build   -- Debug 1, Const 0
#      ./mk.sh release      shipping build -- every parameter folded
#      ./mk.sh check        build both and prove they code identically
#      ./mk.sh mod          regenerate MOD/ in the shipping form, build nothing
#
#  Only the first two write ./tsvcomp.  A shipping binary has no !MAP! markers
#  in it, so an optimizer driving one finds no knobs and reports that nothing
#  it tries changes anything -- which is why `check` leaves ./tsvcomp alone.
#
#  IDX-FORMAT.md sec.1: Debug and Const are orthogonal flags, not two modes.
#  `Const 0` leaves each threshold a live `mapping` object; `Debug 1` makes its
#  descriptor start with "!MAP!", which is the marker IDX/opt.pl scans the
#  executable for.  So the tuning build is the one an optimizer can drive:
#
#      ./mk.sh
#      perl IDX/opt.pl opt.lst ./tsvcomp        # hill-climbs the binary itself
#      cd IDX && for f in tsvcomp-*.idx; do \
#        perl import.pl $f ../export.\!\!\! > t && mv t $f; done
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
#  editing any IDX/tsvcomp-<family>.idx or its .inc.

set -e

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$here"

CXX=${CXX:-c++}
CXXFLAGS=${CXXFLAGS:--O2}
WARN=${WARN:--Wall -Wextra}
REQ=-fwrapv

#  One IDX module per family -- IDX-FORMAT.md sec.13.
FAMS="dig sgn flr cls aux hdr"

#  Regenerate MOD/ from all six.  $1 = UseNew (1 pointers, 0 fixed arrays),
#  $2 = 1 to fold every parameter to a literal, which is IDX-FORMAT.md sec.1's
#  one substitution applied to each module in turn so they cannot drift.
generate() {
  mkdir -p MOD
  for f in $FAMS; do
    s="tsvcomp-$f"
    if [ "$2" = 1 ]; then
      sed 's/^Const 0/Const 1/' "IDX/tsvcomp-$f.idx" > "IDX/tsvcomp-$f-const.idx"
      cp -f "IDX/tsvcomp-$f.inc" "IDX/tsvcomp-$f-const.inc"
      s="tsvcomp-$f-const"
    fi
    ( cd IDX && IDX_NOCONST=0 perl idx2inc.pl "$s.idx" "$1" >/dev/null )
    mv -f "IDX/${s}_h.inc" "MOD/tsvcomp-${f}_h.inc"
    mv -f "IDX/${s}_p.inc" "MOD/tsvcomp-${f}_p.inc"
    if [ "$2" = 1 ]; then rm -f "IDX/$s.idx" "IDX/$s.inc"; fi
  done
}

#  $1 = UseNew, $2 = fold, $3 = output binary
generate_and_build() {
  generate "$1" "$2"
  $CXX $CXXFLAGS $WARN $REQ -o "$3" tsvcomp.cpp -lm
}

#  Regenerate MOD/ in the shipping form and build nothing.  What a script wants
#  when it has finished borrowing MOD/ and has to put it back the way it ships:
#  building would also replace ./tsvcomp, and ./tsvcomp may be the tuning build
#  someone is in the middle of optimizing.
regenerate_mod() { generate 0 1; }

case "${1:-tuning}" in
  tuning)
    generate_and_build 1 0 tsvcomp
    #  The marker is put there by the pdesc macro, so it is the binary that
    #  has to be looked at -- which is also what opt.pl looks at.
    n=$(grep -ac '!MAP!' tsvcomp || true)
    b=$(perl -ne 'BEGIN{$/=undef} $n+=length($3) while /!MAP!(.*?)!(.*?)\x00(.*?)\x00/gs; END{print $n+0}' tsvcomp)
    echo "mk.sh: tuning build -- $b tunable bits in $(perl -ne 'BEGIN{$/=undef} $n++ while /!MAP!/g; END{print $n+0}' tsvcomp) patterns, visible to IDX/opt.pl"
    ;;

  release)
    generate_and_build 0 1 tsvcomp
    if grep -q '!MAP!' MOD/tsvcomp-*_h.inc; then
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
    generate_and_build 1 0 "$tmp/tune"
    generate_and_build 0 1 "$tmp/rel"
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
    #  Leave MOD/ in the shipping shape, which is what it is checked in as.
    #  Both binaries stay in $tmp: ./tsvcomp is not this command's to replace,
    #  and replacing it with the shipping build is the quiet way to end a
    #  tuning session -- that build carries no !MAP! markers, so IDX/opt.pl
    #  finds nothing to patch and every measurement comes back the same.
    regenerate_mod
    [ "$bad" = 0 ] && echo "mk.sh: tuning and shipping builds agree" \
                   || { echo "mk.sh: THE TWO BUILDS DISAGREE" >&2; exit 1; }
    ;;

  mod)
    regenerate_mod
    echo "mk.sh: MOD/ regenerated in the shipping form; no binary built"
    ;;

  *)
    echo "usage: ./mk.sh [tuning|release|check [file-list]|mod]" >&2
    exit 2
    ;;
esac
