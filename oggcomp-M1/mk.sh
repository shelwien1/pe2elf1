#!/bin/sh
#  Regenerate MOD/ from IDX/ and build oggcomp.
#
#      ./mk.sh              tuning build   -- Debug 1, Const 0
#      ./mk.sh release      shipping build -- every parameter folded
#      ./mk.sh check        build both and prove they code identically
#      ./mk.sh mod          regenerate MOD/ in the shipping form, build nothing
#      ./mk.sh pgo f.ogg    shipping build, laid out from a profile of f.ogg
#
#  Only the first two write ./oggcomp.  A shipping binary has no !MAP! markers
#  in it, so an optimizer driving one finds no knobs and reports that nothing
#  it tries changes anything -- which is why `check` leaves ./oggcomp alone.
#
#  IDX-FORMAT.md sec.1: Debug and Const are orthogonal flags, not two modes.
#  `Const 0` leaves each threshold a live `mapping` object; `Debug 1` makes its
#  descriptor start with "!MAP!", which is the marker IDX/opt.pl scans the
#  executable for.  So the tuning build is the one an optimizer can drive:
#
#      ./mk.sh
#      perl IDX/opt.pl opt.lst ./oggcomp        # hill-climbs the binary itself
#      cd IDX && for f in tsvcomp-*.idx; do \
#        perl import.pl $f ../export.\!\!\! > t && mv t $f; done
#      ./mk.sh check                            # then ship what it found
#
#  Widening a context is free to a search whose objective is the size of one
#  file, which is how one arrives at a 38 GB model to save 3 kB.  Build the
#  tuning binary with a price on memory instead of a threshold on it:
#
#      CXXFLAGS='-O2 -DTC_MEMCOST' ./mk.sh     # 10000 bytes per GB of tables
#      CXXFLAGS='-O2 -DTC_MEMCOST=25000' ./mk.sh
#
#  The encoder then pads its output by that much, so opt.pl sees the memory in
#  the number it is minimizing and takes a widening only when it pays for
#  itself.  Off by default, including under `check`, so the two builds agree.
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
#
#  Which also means a tuning build leaves MOD/ holding the tuning form and
#  `git status` showing twelve modified files.  That is not damage and there
#  is nothing to revert: `./mk.sh mod` writes the shipping form back, and so
#  does `./mk.sh check`, which is why it is the last thing that command does.

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
    #  idx2inc.pl opens both its inputs without checking, has no `use strict`
    #  and no die anywhere in it, and exits 0 whatever happens: a misspelled
    #  family or a template that is not beside its .idx produces an empty _p
    #  and a stub _h, a clean compile, and a compressor with no model in it.
    #  `set -e` cannot see any of that, so the post-condition is checked here.
    for o in _h _p; do
      if [ ! -s "IDX/${s}${o}.inc" ]; then
        rm -f "IDX/${s}_h.inc" "IDX/${s}_p.inc"
        [ "$2" = 1 ] && rm -f "IDX/$s.idx" "IDX/$s.inc"
        echo "mk.sh: IDX/idx2inc.pl wrote no ${s}${o}.inc -- is IDX/$s.inc there?" >&2
        echo "mk.sh: MOD/ now holds part of one build and part of another;" >&2
        echo "       fix that and run this again, which regenerates all six." >&2
        exit 1
      fi
    done
    mv -f "IDX/${s}_h.inc" "MOD/tsvcomp-${f}_h.inc"
    mv -f "IDX/${s}_p.inc" "MOD/tsvcomp-${f}_p.inc"
    if [ "$2" = 1 ]; then rm -f "IDX/$s.idx" "IDX/$s.inc"; fi
  done
}

#  $1 = UseNew, $2 = fold, $3 = output binary
generate_and_build() {
  generate "$1" "$2"
  $CXX $CXXFLAGS $WARN $REQ -o "$3" oggcomp.cpp -lm
}

#  Regenerate MOD/ in the shipping form and build nothing.  What a script wants
#  when it has finished borrowing MOD/ and has to put it back the way it ships:
#  building would also replace ./oggcomp, and ./oggcomp may be the tuning build
#  someone is in the middle of optimizing.
regenerate_mod() { generate 0 1; }

case "${1:-tuning}" in
  tuning)
    generate_and_build 1 0 oggcomp
    #  The marker is put there by the pdesc macro, so it is the binary that
    #  has to be looked at -- which is also what opt.pl looks at.
    n=$(grep -ac '!MAP!' oggcomp || true)
    b=$(perl -ne 'BEGIN{$/=undef} $n+=length($3) while /!MAP!(.*?)!(.*?)\x00(.*?)\x00/gs; END{print $n+0}' oggcomp)
    echo "mk.sh: tuning build -- $b tunable bits in $(perl -ne 'BEGIN{$/=undef} $n++ while /!MAP!/g; END{print $n+0}' oggcomp) patterns, visible to IDX/opt.pl"
    ;;

  release)
    generate_and_build 0 1 oggcomp
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
      #  No list given: the bundled corpus, so the contract can be checked in
      #  a fresh clone with nothing else to hand.  testfiles/gen.sh says what
      #  is in it and why.  A real tuning corpus belongs in opt.lst; see
      #  IDX/opt.pl on what it should cover.
      lst="$tmp/list"
      ls testfiles/*.ogg > "$lst" 2>/dev/null || true
      [ -s "$lst" ] || {
        echo "mk.sh check: no testfiles/*.ogg -- ./testfiles/gen.sh makes them," >&2
        echo "             or name a file holding one .ogg per line" >&2
        exit 2; }
    fi
    [ -f "$lst" ] || { echo "mk.sh check: no $lst -- one .ogg per line" >&2; exit 2; }
    generate_and_build 1 0 "$tmp/tune"
    generate_and_build 0 1 "$tmp/rel"
    bad=0
    while IFS= read -r f; do
      case "$f" in ''|\#*) continue;; esac
      "$tmp/tune" c "$f" "$tmp/a.oc"
      "$tmp/rel"  c "$f" "$tmp/b.oc"
      if cmp -s "$tmp/a.oc" "$tmp/b.oc"; then
        printf '  %-40s %10s  identical\n' "$(basename "$f")" "$(wc -c < "$tmp/a.oc")"
      else
        printf '  %-40s DIFFERS\n' "$(basename "$f")"
        bad=1
      fi
    done < "$lst"
    #  Leave MOD/ in the shipping shape, which is what it is checked in as.
    #  Both binaries stay in $tmp: ./oggcomp is not this command's to replace,
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

  pgo)
    #  The shipping build, laid out from a profile: the given .ogg is coded
    #  both ways by an instrumented build first.  It was worth 1.5 to 3%
    #  before OGGCOMP-SPEED.md section 8's changes took out the branches it
    #  was laying out, and measures as noise after them; it is here to be
    #  measured again when the code changes.  The stream is the same stream
    #  -- checked, on the file it trained on.  Both compiles go through an
    #  object of one name in one directory, which is how gcc finds the
    #  .gcda it wrote.
    #  Not `f`: generate() loops over the families in a variable of that name.
    pgo_in=${2:-}
    [ -f "$pgo_in" ] || { echo "usage: ./mk.sh pgo file.ogg" >&2; exit 2; }
    pgo_in=$(cd -- "$(dirname -- "$pgo_in")" && pwd)/$(basename -- "$pgo_in")
    tmp=$(mktemp -d "${TMPDIR:-/tmp}/mk-pgo.XXXXXX")
    trap 'rm -rf "$tmp"' EXIT INT TERM
    generate 0 1
    ( cd "$tmp" && $CXX $CXXFLAGS $WARN $REQ -fprofile-generate -c -o oggcomp.o "$here/oggcomp.cpp" \
        && $CXX -fprofile-generate -o gen oggcomp.o -lm ) || exit 1
    "$tmp/gen" c "$pgo_in" "$tmp/p.oc" && "$tmp/gen" d "$tmp/p.oc" "$tmp/p.ogg" || exit 1
    ( cd "$tmp" && $CXX $CXXFLAGS $WARN $REQ -fprofile-use -fprofile-correction -c -o oggcomp.o "$here/oggcomp.cpp" \
        && $CXX -o "$here/oggcomp" oggcomp.o -lm ) || exit 1
    ./oggcomp c "$pgo_in" "$tmp/q.oc"
    cmp -s "$tmp/p.oc" "$tmp/q.oc" || { echo "mk.sh pgo: THE PROFILED BUILD CODES DIFFERENTLY" >&2; exit 1; }
    echo "mk.sh: shipping build, laid out from a profile of $(basename -- "$pgo_in") -- every parameter folded"
    ;;

  *)
    echo "usage: ./mk.sh [tuning|release|check [file-list]|pgo file.ogg|mod]" >&2
    exit 2
    ;;
esac
