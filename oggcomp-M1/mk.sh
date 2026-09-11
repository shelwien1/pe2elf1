#!/bin/sh
#  Regenerate MOD/ from IDX/ and build oggcomp, and oggdet beside it.
#
#      ./mk.sh              tuning build   -- Debug 1, Const 0
#      ./mk.sh release      shipping build -- every parameter folded
#      ./mk.sh check [lst]  build both and prove they code identically
#      ./mk.sh mod          regenerate MOD/ in the shipping form, build nothing
#      ./mk.sh pgo f.ogg    shipping build, laid out from a profile of f.ogg
#
#  Only the first two (and pgo) write ./oggcomp and ./oggdet.  A shipping
#  binary has no !MAP! markers in it, so an optimizer driving one finds no
#  knobs and reports that nothing it tries changes anything -- which is why
#  `check` leaves ./oggcomp alone.
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
#  something different from what it evaluated to.  It reads one of the two
#  streams back with the other build as well, since a parameter that folds
#  wrong is as wrong on the way out as it was on the way in, and equal
#  encodes say nothing about that.  With no list it uses testfiles/; a list
#  is one file per line, blanks and #comments skipped, named relative to
#  where you are and holding paths relative to the tree.
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
callerpwd=$PWD
cd "$here"

#  IDX/ is a source directory and the generator writes into it: the -const
#  copies and the _h/_p output live there for as long as generate() runs.
#  Anything that ends the script in the middle -- a failed compile, a Ctrl-C
#  -- leaves them behind looking like files someone checked in, so they are
#  swept on every exit.  The traps for the signals exit, which the bare
#  `trap ... EXIT INT TERM` they replace did not: that one deleted the
#  scratch directory and let the script carry on using it.
tmp=
cleanup() {
  rm -f "$here"/IDX/tsvcomp-*-const.idx "$here"/IDX/tsvcomp-*-const.inc \
        "$here"/IDX/tsvcomp-*_h.inc "$here"/IDX/tsvcomp-*_p.inc
  [ -n "$tmp" ] && rm -rf "$tmp"
  return 0
}
trap cleanup EXIT
trap 'cleanup; exit 130' INT
trap 'cleanup; exit 143' TERM

CXX=${CXX:-c++}
CXXFLAGS=${CXXFLAGS:--O2}
WARN=${WARN:--Wall -Wextra}

#  -fwrapv for sh_mapping.inc's `value <<= 1` on a negative int, which a
#  tuning build really does execute.  It is defined behaviour from C++20 on,
#  so pinning -std=c++20 would fix it at the language level instead; no -std
#  is passed here because every g++ from 11 on defaults to gnu++17 or later
#  and the code needs nothing newer.  -lm is redundant on glibc 2.34 and
#  after, and required before it.  -O3, -march=native and -flto were all
#  measured and are noise: the program is bound by a gigabyte of model
#  tables, not by code.
REQ=-fwrapv

#  Lib3/coro3b.inc picks between an x86-64 and an i386 setjmp written in
#  inline assembly, and there is no third branch: on aarch64 the i386 one is
#  what gets included, and the build stops with an assembler error out of a
#  header nobody was reading.  Say so here instead.  The CORO_NOASM fallback
#  in that file is not a way out on glibc -- it restores a stack that longjmp
#  then refuses to jump into -- so porting means giving coro3b.inc a branch,
#  not passing a flag.  Set OGGCOMP_ANY_ARCH=1 to try anyway.
arch=$(uname -m)
case "${1:-tuning}:$arch" in
  #  `mod` runs perl and nothing else, so the architecture it runs on does
  #  not come into it -- and putting MOD/ back the way it ships is exactly
  #  what someone on a machine that cannot build would want to do.
  mod:*) ;;
  *:x86_64 | *:amd64 | *:i[3456]86) ;;
  *)
    [ "${OGGCOMP_ANY_ARCH:-0}" = 1 ] || {
      echo "mk.sh: this is x86-64 only -- Lib3/coro3b.inc has no branch for $arch" >&2
      echo "       (OGGCOMP_ANY_ARCH=1 ./mk.sh to try it regardless)" >&2
      exit 2; }
    ;;
esac

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
        #  The stubs and the -const copies go on the way out, in cleanup().
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

#  oggdet, the stream carver, includes the compressor -- oc_coro.inc and
#  every layer under it -- so that `oggdet c -c` writes the .oc that
#  `oggcomp c` would.  It is built from the same MOD/, in whatever form the
#  target just generated, and a change to the model is a change to both.
build_det() {
  $CXX $CXXFLAGS $WARN $REQ -o oggdet oggdet.cpp -lm
}

#  Regenerate MOD/ in the shipping form and build nothing.  What a script wants
#  when it has finished borrowing MOD/ and has to put it back the way it ships:
#  building would also replace ./oggcomp, and ./oggcomp may be the tuning build
#  someone is in the middle of optimizing.
regenerate_mod() { generate 0 1; }

case "${1:-tuning}" in
  tuning)
    generate_and_build 1 0 oggcomp
    build_det
    #  The marker is put there by the pdesc macro, so it is the binary that
    #  has to be looked at -- which is also what opt.pl looks at.  A live
    #  descriptor reads !MAP!<name>!<offset>\0<pattern>\0 (sh_mapping.inc),
    #  and the pattern is one character per bit opt.pl may flip, so the sum
    #  of their lengths is the size of the search space this build exposes.
    #  Zero of either means the markers did not survive the build and an
    #  optimizer would report that nothing it tries changes anything.
    perl -e 'undef $/; $_ = <>;
             while( /!MAP!(.*?)!(.*?)\x00(.*?)\x00/gs ) { $n++; $b += length $3 }
             printf "mk.sh: tuning build -- %d tunable bits in %d patterns, "
                  . "visible to IDX/opt.pl\n", $b, $n;
             exit($n ? 0 : 1)' oggcomp ||
      { echo "mk.sh: the tuning build carries no !MAP! markers" >&2; exit 1; }
    ;;

  release)
    generate_and_build 0 1 oggcomp
    build_det
    #  Look at the binary.  Grepping MOD/*_h.inc for "!MAP!" -- which is what
    #  this used to do -- can never find anything: the marker is not in the
    #  generated header, it is in the pdesc_live macro in sh_mapping.inc that
    #  the generated header expands.  So the guard passed on a MOD/ that was
    #  entirely unfolded, and `mk.sh release` would ship a binary with live
    #  knobs in it while printing that every parameter had been folded.
    #  grep answers 0 for found, 1 for not found and 2 or more for "I could
    #  not look" -- an unreadable or missing binary among them.  A bare `if
    #  grep` reads that last case as "not found", i.e. as the good news.
    #  `grep ...; g=$?` would not do: under set -e the shell stops at a
    #  grep that exits 1, which is the case this is here to call good.
    g=0; grep -aq '!MAP!' oggcomp || g=$?
    if [ $g = 0 ]; then
      echo "mk.sh: the release build still carries !MAP! markers" >&2
      exit 1
    elif [ $g != 1 ]; then
      echo "mk.sh: could not read ./oggcomp to check it for !MAP! markers" >&2
      exit 1
    fi
    echo "mk.sh: release build -- every parameter folded"
    ;;

  check)
    #  Both builds, over whatever the list names, compared byte for byte.
    #  The list is named from the caller's directory, so it has to be
    #  resolved before the cd above -- which has already happened, hence the
    #  saved OLDPWD.  Paths inside the list stay relative to the tree, which
    #  is what an opt.lst checked in beside opt.pl wants.
    lst=${2:-}
    case "$lst" in
      ''|/*) ;;
      #  The caller's directory wins.  Testing the tree first would let a
      #  list of the same name sitting in the tree shadow the one that was
      #  meant, silently, and the header above promises the opposite.
      *) [ ! -f "$callerpwd/$lst" ] || lst="$callerpwd/$lst";;
    esac
    tmp=$(mktemp -d "${TMPDIR:-/tmp}/mk-check.XXXXXX")
    if [ -z "$lst" ]; then
      #  No list given: the bundled corpus, so the contract can be checked in
      #  a fresh clone with nothing else to hand.  testfiles/gen.sh says what
      #  is in it and why.  A real tuning corpus belongs in opt.lst; see
      #  IDX/opt.pl on what it should cover.
      lst="$tmp/list"
      : > "$lst"
      #  printf, not echo: dash's echo expands backslash escapes, so a
      #  corpus file with a backslash in its name would go into the list
      #  as some other name and the run would die trying to open it.
      for g in testfiles/*.ogg testfiles/*.bin; do
        [ -f "$g" ] && printf '%s\n' "$g" >> "$lst"
      done
      [ -s "$lst" ] || {
        echo "mk.sh check: no testfiles/*.ogg -- ./testfiles/gen.sh makes them," >&2
        echo "             or name a file holding one input per line" >&2
        exit 2; }
    fi
    [ -f "$lst" ] || { echo "mk.sh check: no $lst -- one input per line" >&2; exit 2; }
    generate_and_build 1 0 "$tmp/tune"
    generate_and_build 0 1 "$tmp/rel"
    #  Normalise the list once rather than per line: strip the CR a list
    #  authored on Windows carries -- this tree ships gc.bat and t.bat from
    #  exactly such a sibling -- trim the surrounding blanks, and drop
    #  comments and empty lines.  opt.pl accepts all of those shapes, and
    #  mk.sh should read the same opt.lst that it does.
    sed 's/\r$//; s/^[[:space:]]*//; s/[[:space:]]*$//; /^#/d; /^$/d' \
        "$lst" > "$tmp/clean"

    bad=0
    seen=0
    while IFS= read -r f; do
      seen=$((seen + 1))
      #  A file neither build will code is a row, not the end of the run.
      #  `set -e` would otherwise stop here with the compressor's message
      #  and nothing from mk.sh, and every file after it would go
      #  uncompared while the exit status said only "1".  The encoder takes
      #  any bytes, so this is a file that is not there or cannot be read
      #  -- but a list is whatever someone wrote, and both builds have to
      #  fail on it alike.
      ta=0; "$tmp/tune" c "$f" "$tmp/a.oc" || ta=$?
      tb=0; "$tmp/rel"  c "$f" "$tmp/b.oc" || tb=$?
      if [ "$ta" != 0 ] || [ "$tb" != 0 ]; then
        if [ "$ta" = "$tb" ]; then
          printf '  %-40s %10s  refused alike\n' "$(basename "$f")" "exit $ta"
        else
          printf '  %-40s REFUSED BY ONE BUILD ONLY (%s vs %s)\n' \
                 "$(basename "$f")" "$ta" "$tb"
          bad=1
        fi
      elif cmp -s "$tmp/a.oc" "$tmp/b.oc"; then
        #  And read one of them back with the other build.  Equal encodes
        #  say nothing about the decoders, and a parameter that folds to
        #  something different would be just as wrong on the way out.
        if "$tmp/rel" d "$tmp/a.oc" "$tmp/back.ogg" >/dev/null 2>&1 &&
           cmp -s "$f" "$tmp/back.ogg"; then
          printf '  %-40s %10s  identical\n' "$(basename "$f")" "$(wc -c < "$tmp/a.oc")"
        else
          printf '  %-40s DOES NOT READ BACK\n' "$(basename "$f")"
          bad=1
        fi
      else
        printf '  %-40s DIFFERS\n' "$(basename "$f")"
        bad=1
      fi
    done < "$tmp/clean"
    #  An empty corpus is not agreement.  A glob that matched nothing or an
    #  opt.lst that moved would otherwise print the same line as a clean run.
    [ "$seen" -gt 0 ] || {
      echo "mk.sh check: $lst named no files -- nothing was compared" >&2; exit 2; }
    #  Leave MOD/ in the shipping shape, which is what it is checked in as.
    #  Both binaries stay in $tmp: ./oggcomp is not this command's to replace,
    #  and replacing it with the shipping build is the quiet way to end a
    #  tuning session -- that build carries no !MAP! markers, so IDX/opt.pl
    #  finds nothing to patch and every measurement comes back the same.
    regenerate_mod
    #  Not `A && B || C`: a failed echo -- a closed pipe, a full disk -- would
    #  then take the C branch and report the opposite of what was measured.
    if [ "$bad" = 0 ]; then
      [ "$seen" = 1 ] && many="1 file" || many="$seen files"
      echo "mk.sh: tuning and shipping builds agree over $many"
    else
      echo "mk.sh: THE TWO BUILDS DISAGREE" >&2
      exit 1
    fi
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
    generate 0 1
    ( cd "$tmp" && $CXX $CXXFLAGS $WARN $REQ -fprofile-generate -c -o oggcomp.o "$here/oggcomp.cpp" \
        && $CXX -fprofile-generate -o gen oggcomp.o -lm ) || exit 1
    "$tmp/gen" c "$pgo_in" "$tmp/p.oc" && "$tmp/gen" d "$tmp/p.oc" "$tmp/p.ogg" || exit 1
    #  The training run decodes as well as encodes, so that the profile
    #  covers both halves; while it is there, check that what came back is
    #  what went in.  Free, and it is the only round trip this branch makes.
    cmp -s "$pgo_in" "$tmp/p.ogg" ||
      { echo "mk.sh pgo: the instrumented build did not give the file back" >&2; exit 1; }
    #  gcc treats a missing .gcda as a warning, so without this the profiled
    #  compile would quietly be an ordinary one and the line below would say
    #  it was laid out from a profile that was never read.
    [ -s "$tmp/oggcomp.gcda" ] ||
      { echo "mk.sh pgo: no profile was written -- $tmp/oggcomp.gcda is not there" >&2; exit 1; }
    ( cd "$tmp" && $CXX $CXXFLAGS $WARN $REQ -fprofile-use -fprofile-correction -c -o oggcomp.o "$here/oggcomp.cpp" \
        && $CXX -o "$here/oggcomp" oggcomp.o -lm ) || exit 1
    build_det
    ./oggcomp c "$pgo_in" "$tmp/q.oc"
    cmp -s "$tmp/p.oc" "$tmp/q.oc" || { echo "mk.sh pgo: THE PROFILED BUILD CODES DIFFERENTLY" >&2; exit 1; }
    echo "mk.sh: shipping build, laid out from a profile of $(basename -- "$pgo_in") -- every parameter folded"
    ;;

  *)
    echo "usage: ./mk.sh [tuning|release|check [file-list]|pgo file.ogg|mod]" >&2
    exit 2
    ;;
esac
