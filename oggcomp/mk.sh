#!/bin/sh
#  Regenerate MOD/ from IDX/ and build oggcomp -- every level, or one -- and
#  oggdet beside it.
#
#      ./mk.sh release         shipping build of every level -- every parameter
#                              folded -- as ./oggcomp
#      ./mk.sh [tuning [L]]    tuning build of level -L alone (default 9) --
#                              Debug 1, Const 0: every knob of IDX/lL live --
#                              as ./oggcomp
#      ./mk.sh check [L] [lst] the tuning build of -L (default: of each level in
#                              turn) and the shipping build code identically
#      ./mk.sh mod             regenerate MOD/ in the shipping form, build nothing
#      ./mk.sh pgo f.ogg       shipping build, laid out from a profile of f.ogg
#      ./mk.sh lib             oggcomp.so: every level as a library (oc_api.h)
#      ./mk.sh apitest         oc_api_test, the API driven from a program
#
#  The levels are the models: IDX/l9, l7, l3 and l1 each hold one, the six
#  families' context indices and parameters (levels_h.inc says which is
#  which), and IDX/idx2inc.pl turns each into MOD/l<L>/, under names of the
#  level's own (L9_dig_...), so that one program has them all and `c -L`
#  picks one.  IDX/ itself holds the tools: the generator, the optimizer,
#  the import script, and IDX-FORMAT.md, the format of the .idx files.
#
#  Only release, tuning and pgo write ./oggcomp and ./oggdet.  A shipping
#  binary has no !MAP! markers in it, so an optimizer driving one finds no
#  knobs and reports that nothing it tries changes anything -- which is why
#  `check` leaves ./oggcomp alone.
#
#  IDX-FORMAT.md sec.1: Debug and Const are orthogonal flags, not two modes.
#  `Const 0` leaves each threshold a live `mapping` object; `Debug 1` makes its
#  descriptor start with "!MAP!", which is the marker IDX/opt.pl scans the
#  executable for.  So the tuning build is the one an optimizer can drive, and
#  it holds the one level, so that its knobs are all opt.pl finds and `c`
#  codes at it:
#
#      ./mk.sh tuning 9
#      perl IDX/opt.pl opt.lst ./oggcomp        # hill-climbs the binary itself
#      cd IDX/l9 && for f in tsvcomp-*.idx; do \
#        perl ../import.pl $f ../../export.\!\!\! > t && mv t $f; done
#      cd ../.. && ./mk.sh check 9              # then ship what it found
#
#  Widening a context is free to a search whose objective is the size of one
#  file, which is how one arrives at a 38 GB model to save 3 kB.  Build the
#  tuning binary with a price on memory instead of a threshold on it:
#
#      CXXFLAGS='-O2 -DTC_MEMCOST' ./mk.sh tuning 9    # 10000 bytes per GB of tables
#      CXXFLAGS='-O2 -DTC_MEMCOST=25000' ./mk.sh tuning 9
#
#  The encoder then pads its output by that much, so opt.pl sees the memory in
#  the number it is minimizing and takes a widening only when it pays for
#  itself.  Off by default, including under `check`, so the two builds agree.
#
#  The shipping build is derived from the same source with one substitution --
#  "Const 0" becomes "Const 1" in a copy -- so the two cannot drift apart, and
#  `./mk.sh check` is the test that they have not: whatever the parameters are,
#  both builds must produce byte-identical streams, level by level.  That is
#  the format's stated contract and the only check that catches a parameter
#  which folds to something different from what it evaluated to.  It reads
#  one of the two streams back with the other build as well, since a
#  parameter that folds wrong is as wrong on the way out as it was on the way
#  in, and equal encodes say nothing about that.  With no list it uses
#  testfiles/; a list is one file per line, blanks and #comments skipped,
#  named relative to where you are and holding paths relative to the tree.
#
#  MOD/ is a build input, not a build artefact.  It ships generated -- in the
#  shipping form, so a plain compile works without perl -- and this script
#  rewrites it.  A stale MOD/ compiles fine and codes differently, so run
#  ./mk.sh mod after editing any IDX/l<L>/tsvcomp-<family>.idx or its .inc.
#
#  Which also means a tuning build leaves MOD/l<L> holding the tuning form and
#  `git status` showing twelve modified files.  That is not damage and there
#  is nothing to revert: `./mk.sh mod` writes the shipping form back, and so
#  does `./mk.sh check`, which is why it is the last thing that command does.

set -e

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
callerpwd=$PWD
cd "$here"

#  The levels, the strongest first, as levels.inc lists them.
LEVELS="9 7 3 1"

#  IDX/l<L>/ is a source directory and the generator writes into it: the
#  -const copies and the _h/_p output live there for as long as generate()
#  runs.  Anything that ends the script in the middle -- a failed compile, a
#  Ctrl-C -- leaves them behind looking like files someone checked in, so they
#  are swept on every exit.  The traps for the signals exit, which the bare
#  `trap ... EXIT INT TERM` they replace did not: that one deleted the scratch
#  directory and let the script carry on using it.
tmp=
cleanup() {
  for l in $LEVELS; do
    rm -f "$here/IDX/l$l"/tsvcomp-*-const.idx "$here/IDX/l$l"/tsvcomp-*-const.inc \
          "$here/IDX/l$l"/tsvcomp-*_h.inc "$here/IDX/l$l"/tsvcomp-*_p.inc
  done
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
REQ="-fwrapv -pthread"

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

#  Is $1 one of the levels?
is_level() {
  case " $LEVELS " in
    *" $1 "*) return 0 ;;
  esac
  return 1
}

#  Regenerate MOD/l$1 from all six of IDX/l$1.  $2 = UseNew (1 pointers, 0
#  fixed arrays), $3 = 1 to fold every parameter to a literal, which is
#  IDX-FORMAT.md sec.1's one substitution applied to each module in turn so
#  they cannot drift.
generate() {
  lv=$1
  d=IDX/l$lv
  mkdir -p "MOD/l$lv"
  for f in $FAMS; do
    s="tsvcomp-$f"
    [ -f "$d/tsvcomp-$f.idx" ] ||
      { echo "mk.sh: $d/tsvcomp-$f.idx is not there -- not a level's directory" >&2; exit 1; }
    if [ "$3" = 1 ]; then
      sed 's/^Const 0/Const 1/' "$d/tsvcomp-$f.idx" > "$d/tsvcomp-$f-const.idx"
      cp -f "$d/tsvcomp-$f.inc" "$d/tsvcomp-$f-const.inc"
      s="tsvcomp-$f-const"
    fi
    ( cd "$d" && IDX_NOCONST=0 perl "$here/IDX/idx2inc.pl" "$s.idx" "$2" >/dev/null )
    #  idx2inc.pl opens both its inputs without checking, has no `use strict`
    #  and no die anywhere in it, and exits 0 whatever happens: a misspelled
    #  family or a template that is not beside its .idx produces an empty _p
    #  and a stub _h, a clean compile, and a compressor with no model in it.
    #  `set -e` cannot see any of that, so the post-condition is checked here.
    for o in _h _p; do
      if [ ! -s "$d/${s}${o}.inc" ]; then
        #  The stubs and the -const copies go on the way out, in cleanup().
        echo "mk.sh: IDX/idx2inc.pl wrote no ${s}${o}.inc -- is $d/$s.inc there?" >&2
        echo "mk.sh: MOD/l$lv now holds part of one build and part of another;" >&2
        echo "       fix that and run this again, which regenerates all six." >&2
        exit 1
      fi
    done
    mv -f "$d/${s}_h.inc" "MOD/l$lv/tsvcomp-${f}_h.inc"
    mv -f "$d/${s}_p.inc" "MOD/l$lv/tsvcomp-${f}_p.inc"
    if [ "$3" = 1 ]; then rm -f "$d/$s.idx" "$d/$s.inc"; fi
  done
}

#  Every level in the shipping form: what MOD/ is checked in as, and what a
#  plain compile of oggcomp.cpp builds.
regenerate_mod() {
  for l in $LEVELS; do generate "$l" 0 1; done
}

#  $1 = output binary, the rest flags for the compiler (-DOC_ONLY=L)
build() {
  out=$1; shift
  $CXX $CXXFLAGS $WARN $REQ "$@" -o "$out" oggcomp.cpp -lm
}

#  oggdet, the stream carver, includes the compressor -- oc_coro.inc and
#  every layer under it -- so that `oggdet c -c` writes the .oc that
#  `oggcomp c` would.  It is built from the same MOD/, in whatever form the
#  target just generated, and a change to the model is a change to both.
#  A tree without oggdet.cpp -- this one as it came, which holds the
#  compressor and not the carver -- builds oggcomp and says so, rather
#  than stopping with the compiler's "no such file" after the build it was
#  asked for has worked: `./mk.sh` exited 1 here every time, `release` and
#  `pgo` before their checks ran, and `t.sh` skips oggdet's tests when it
#  is not there.
build_det() {
  if [ ! -f oggdet.cpp ]; then
    echo "mk.sh: no oggdet.cpp in this tree -- oggdet not built"
    return 0
  fi
  $CXX $CXXFLAGS $WARN $REQ "$@" -o oggdet oggdet.cpp -lm
}

case "${1:-tuning}" in
  tuning)
    lv=${2:-9}
    is_level "$lv" || { echo "mk.sh: no level -$lv (levels: $LEVELS)" >&2; exit 2; }
    generate "$lv" 1 0
    build oggcomp -DOC_ONLY="$lv"
    build_det -DOC_ONLY="$lv"
    #  The marker is put there by the pdesc macro, so it is the binary that
    #  has to be looked at -- which is also what opt.pl looks at.  A live
    #  descriptor reads !MAP!<name>!<offset>\0<pattern>\0 (sh_mapping.inc),
    #  and the pattern is one character per bit opt.pl may flip, so the sum
    #  of their lengths is the size of the search space this build exposes.
    #  Zero of either means the markers did not survive the build and an
    #  optimizer would report that nothing it tries changes anything.
    perl -e 'undef $/; $_ = <>;
             while( /!MAP!(.*?)!(.*?)\x00(.*?)\x00/gs ) { $n++; $b += length $3 }
             printf "mk.sh: tuning build of level -%s -- %d tunable bits in %d patterns, "
                  . "visible to IDX/opt.pl\n", $ARGV[0], $b, $n;
             exit($n ? 0 : 1)' oggcomp "$lv" ||
      { echo "mk.sh: the tuning build carries no !MAP! markers" >&2; exit 1; }
    ;;

  release)
    regenerate_mod
    build oggcomp
    build_det
    #  Look at the binary.  Grepping MOD/ for "!MAP!" -- which is what this
    #  used to do -- can never find anything: the marker is not in the
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
    echo "mk.sh: release build -- levels $LEVELS, every parameter folded"
    ;;

  check)
    #  check [L] [list]: a level given first is the only one checked.
    levels=$LEVELS
    if [ -n "${2:-}" ] && is_level "$2"; then levels=$2; shift; fi
    #  Both builds, over whatever the list names, compared byte for byte.
    #  The list is named from the caller's directory, so it has to be
    #  resolved before the cd above -- which has already happened, hence the
    #  saved callerpwd.  Paths inside the list stay relative to the tree,
    #  which is what an opt.lst checked in beside opt.pl wants.
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
    #  Normalise the list once rather than per line: strip the CR a list
    #  authored on Windows carries -- this tree ships gc.bat and t.bat from
    #  exactly such a sibling -- trim the surrounding blanks, and drop
    #  comments and empty lines.  opt.pl accepts all of those shapes, and
    #  mk.sh should read the same opt.lst that it does.
    sed 's/\r$//; s/^[[:space:]]*//; s/[[:space:]]*$//; /^#/d; /^$/d' \
        "$lst" > "$tmp/clean"

    #  The shipping build is the one that ships: every level in one binary.
    #  Each level's tuning build holds that level alone, as `tuning` builds it.
    regenerate_mod
    build "$tmp/rel"
    bad=0
    seen=0
    for lv in $levels; do
      generate "$lv" 1 0
      build "$tmp/tune" -DOC_ONLY="$lv"
      generate "$lv" 0 1
      echo "  level -$lv"
      while IFS= read -r f; do
        seen=$((seen + 1))
        #  A file neither build will code is a row, not the end of the run.
        #  `set -e` would otherwise stop here with the compressor's message
        #  and nothing from mk.sh, and every file after it would go
        #  uncompared while the exit status said only "1".  The encoder takes
        #  any bytes, so this is a file that is not there or cannot be read
        #  -- but a list is whatever someone wrote, and both builds have to
        #  fail on it alike.
        ta=0; "$tmp/tune" c -q -"$lv" "$f" "$tmp/a.oc" || ta=$?
        tb=0; "$tmp/rel"  c -q -"$lv" "$f" "$tmp/b.oc" || tb=$?
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
          if "$tmp/rel" d -q "$tmp/a.oc" "$tmp/back.ogg" >/dev/null 2>&1 &&
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
    done
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
      echo "mk.sh: tuning and shipping builds agree, level by level ($levels), over $seen codings"
    else
      echo "mk.sh: THE TWO BUILDS DISAGREE" >&2
      exit 1
    fi
    ;;

  mod)
    regenerate_mod
    echo "mk.sh: MOD/ regenerated in the shipping form, levels $LEVELS; no binary built"
    ;;

  lib)
    #  Every level as a library: oggcomp_dll.cpp compiled with the API
    #  exported (oc_api.h), for a program of someone else's to drive; the
    #  level goes in Init's flags.  The program does not load it: it has the
    #  levels itself.
    regenerate_mod
    $CXX $CXXFLAGS $WARN $REQ -shared -fPIC -fvisibility=hidden -DOC_BUILD_DLL \
         -o oggcomp.so oggcomp_dll.cpp -lm
    echo "mk.sh: oggcomp.so -- levels $LEVELS as a library, every parameter folded"
    ;;

  apitest)
    #  The API driven from a program: oc_api_test.cpp over the library's
    #  translation unit, in the shipping form.  ./t.sh runs it when it is
    #  there, and ./t.sh -B builds it here.
    regenerate_mod
    $CXX $CXXFLAGS $WARN $REQ -o oc_api_test oc_api_test.cpp -lm
    echo "mk.sh: oc_api_test -- run it on a corpus file"
    ;;

  pgo)
    #  The shipping build, laid out from a profile: the given .ogg is coded
    #  both ways at every level by an instrumented build first.  It was worth
    #  1.5 to 3% before OGGCOMP-SPEED.md section 8's changes took out the
    #  branches it was laying out, and measures as noise after them; it is
    #  here to be measured again when the code changes.  The stream is the
    #  same stream -- checked, on the file it trained on.  Both compiles go
    #  through an object of one name in one directory, which is how gcc
    #  finds the .gcda it wrote.
    #  Not `f`: generate() loops over the families in a variable of that name.
    pgo_in=${2:-}
    [ -f "$pgo_in" ] || { echo "usage: ./mk.sh pgo file.ogg" >&2; exit 2; }
    pgo_in=$(cd -- "$(dirname -- "$pgo_in")" && pwd)/$(basename -- "$pgo_in")
    tmp=$(mktemp -d "${TMPDIR:-/tmp}/mk-pgo.XXXXXX")
    regenerate_mod
    ( cd "$tmp" && $CXX $CXXFLAGS $WARN $REQ -fprofile-generate -c -o oggcomp.o "$here/oggcomp.cpp" \
        && $CXX -fprofile-generate -pthread -o gen oggcomp.o -lm ) || exit 1
    for lv in $LEVELS; do
      "$tmp/gen" c -q -"$lv" "$pgo_in" "$tmp/p$lv.oc" && "$tmp/gen" d -q "$tmp/p$lv.oc" "$tmp/p.ogg" || exit 1
      #  The training run decodes as well as encodes, so that the profile
      #  covers both halves; while it is there, check that what came back is
      #  what went in.  Free, and it is the only round trip this branch makes.
      cmp -s "$pgo_in" "$tmp/p.ogg" ||
        { echo "mk.sh pgo: the instrumented build did not give the file back at -$lv" >&2; exit 1; }
    done
    #  gcc treats a missing .gcda as a warning, so without this the profiled
    #  compile would quietly be an ordinary one and the line below would say
    #  it was laid out from a profile that was never read.
    [ -s "$tmp/oggcomp.gcda" ] ||
      { echo "mk.sh pgo: no profile was written -- $tmp/oggcomp.gcda is not there" >&2; exit 1; }
    ( cd "$tmp" && $CXX $CXXFLAGS $WARN $REQ -fprofile-use -fprofile-correction -c -o oggcomp.o "$here/oggcomp.cpp" \
        && $CXX -pthread -o "$here/oggcomp" oggcomp.o -lm ) || exit 1
    build_det
    for lv in $LEVELS; do
      ./oggcomp c -q -"$lv" "$pgo_in" "$tmp/q.oc"
      cmp -s "$tmp/p$lv.oc" "$tmp/q.oc" || { echo "mk.sh pgo: THE PROFILED BUILD CODES DIFFERENTLY at -$lv" >&2; exit 1; }
    done
    echo "mk.sh: shipping build, laid out from a profile of $(basename -- "$pgo_in") -- every parameter folded"
    ;;

  *)
    echo "usage: ./mk.sh [release | tuning [L] | check [L] [file-list] | mod | pgo file.ogg | lib | apitest]   (levels: $LEVELS)" >&2
    exit 2
    ;;
esac
