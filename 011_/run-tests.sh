#!/usr/bin/env bash
# Linux test driver for pjpg.  Invoked by the Makefile; also usable directly:
#
#   ./run-tests.sh check   ./pjpg ../testfiles   regression vs tests/golden.log
#   ./run-tests.sh golden  ./pjpg ../testfiles   regenerate tests/golden.log
#   ./run-tests.sh corpus  ./pjpg ../testfiles   run every jpeg, incl. imagetestsuite
#   ./run-tests.sh matrix  ''    ../testfiles    build+run the compiler/flag matrix
#
# Note on line endings: log1 and tests/golden.log are stored with CRLF (they came
# from, and must stay diffable against, the Windows build).  Every comparison here
# strips CR first, so the same golden file works on both platforms.

set -u
mode=${1:-check}
BIN=${2:-./pjpg}
TESTDIR=${3:-../testfiles}
HERE=$(cd "$(dirname "$0")" && pwd)
GOLDEN="$HERE/tests/golden.log"
WORK=$(mktemp -d); trap 'rm -rf "$WORK"' EXIT

# The golden set: every jpeg checked into testfiles/, in a fixed C-locale order so
# the log is reproducible.  Files are listed by basename in the log so the golden
# does not embed absolute paths.
list_golden_files() {
  find "$TESTDIR" -maxdepth 1 -type f \( -iname '*.jpg' -o -iname '*.jpeg' \) -print0 \
    | LC_ALL=C sort -z
}

emit_golden() {   # $1 = binary to run
  local bin=${1:-$BIN}
  while IFS= read -r -d '' f; do
    printf '===== %s =====\n' "$(basename "$f")"
    timeout 60 "$bin" "$f" 2>&1
    printf '===== exit=%d =====\n' $?
  done < <(list_golden_files)
}

case "$mode" in

golden)
  mkdir -p "$HERE/tests"
  emit_golden > "$WORK/g"
  # store with CRLF, to match log1 and stay stable across platforms
  sed 's/$/\r/' "$WORK/g" > "$GOLDEN"
  echo "wrote $GOLDEN ($(wc -l < "$GOLDEN") lines, $(list_golden_files | tr -cd '\0' | wc -c) files)"
  ;;

check)
  fail=0

  # 1) No line the Windows build printed may have disappeared.  pjpg now parses
  #    markers the original skipped, so its output is a SUPERSET of log1; what
  #    would be a regression is a line going missing.
  {
    timeout 60 "$BIN" "$TESTDIR/drazen1.jpg"
    timeout 60 "$BIN" "$TESTDIR/000107_Exif_MM.jpg"
  } > "$WORK/log1.new" 2>&1
  tr -d '\r' < "$HERE/log1" > "$WORK/log1.ref"
  diff -a "$WORK/log1.ref" "$WORK/log1.new" > "$WORK/log1.diff"
  gone=$(grep -ac '^<' "$WORK/log1.diff" 2>/dev/null || true); gone=${gone:-0}
  add=$(grep -ac '^>' "$WORK/log1.diff" 2>/dev/null || true); add=${add:-0}
  if [ "$gone" -eq 0 ]; then
    echo "PASS  log1      no line lost vs the Windows reference ($add added)"
  else
    echo "FAIL  log1      $gone line(s) the Windows build printed are missing:"
    grep -a '^<' "$WORK/log1.diff" | head -20; fail=1
  fi

  # 2) the full golden log over every checked-in jpeg
  if [ -f "$GOLDEN" ]; then
    emit_golden "$BIN" > "$WORK/g.new"
    tr -d '\r' < "$GOLDEN" > "$WORK/g.ref"
    if diff -a -u "$WORK/g.ref" "$WORK/g.new" > "$WORK/g.diff"; then
      echo "PASS  golden    $(list_golden_files | tr -cd '\0' | wc -c) files match tests/golden.log"
    else
      echo "FAIL  golden    differs from tests/golden.log:"; head -60 "$WORK/g.diff"; fail=1
    fi
  else
    echo "SKIP  golden    tests/golden.log missing (run 'make golden')"
  fi

  # 3) the crafted malformed inputs must be reported, never crash
  if [ -f "$HERE/../docs/make-repros.py" ]; then
    rm -rf "$WORK/repro"; python3 "$HERE/../docs/make-repros.py" "$WORK/repro" >/dev/null
    bad=0
    for f in "$WORK/repro"/*.jpg; do
      timeout 60 "$BIN" "$f" >/dev/null 2>&1; e=$?
      if [ $e -gt 1 ]; then echo "  CRASH exit=$e on $(basename "$f")"; bad=1; fi
    done
    if [ $bad -eq 0 ]; then
      echo "PASS  repros    $(ls "$WORK"/repro/*.jpg | wc -l) crafted malformed files reported, none crashed"
    else
      echo "FAIL  repros    a crafted input crashed the parser"; fail=1
    fi
  fi

  [ $fail -eq 0 ] && echo "all tests passed" || echo "TESTS FAILED"
  exit $fail
  ;;

corpus)
  # every bundled jpeg plus the 98 in imagetestsuite-jpg-1.00.tar.gz
  ext="$WORK/its"; mkdir -p "$ext"
  for t in "$TESTDIR"/*.tar.gz; do
    [ -e "$t" ] && tar -xzf "$t" -C "$ext"
  done
  # exit 0 = parsed clean, 1 = parsed but reported errors (expected: imagetestsuite
  # is a deliberately-damaged corpus), anything else is a crash or a hang.
  n=0; errs=0; bad=0
  while IFS= read -r -d '' f; do
    n=$((n+1))
    timeout 60 "$BIN" "$f" > /dev/null 2>&1; e=$?
    if [ $e -eq 1 ]; then errs=$((errs+1))
    elif [ $e -ne 0 ]; then
      bad=$((bad+1))
      if [ $e -eq 124 ]; then echo "HANG      $f"; else echo "CRASH exit=$e   $f"; fi
    fi
  done < <(find "$TESTDIR" "$ext" -type f \( -iname '*.jpg' -o -iname '*.jpeg' \) -print0 | LC_ALL=C sort -z)
  echo "corpus: $n files, $((n-errs-bad)) clean, $errs reported parse errors, $bad crashes/hangs"
  [ $bad -eq 0 ]
  ;;

matrix)
  # Every configuration we claim to support.  Each cell builds, runs the two t.bat
  # files and compares against log1.
  cd "$HERE" || exit 1
  if [ ! -f "$GOLDEN" ]; then echo "matrix: tests/golden.log missing (run 'make golden')"; exit 1; fi
  ref="$WORK/g.ref"; tr -d '\r' < "$GOLDEN" > "$ref"
  base="-std=c++17 -I../Lib3 -DNDEBUG -Drestrict=__restrict"
  cg="-fomit-frame-pointer -fno-stack-protector -fno-stack-check -fstrict-aliasing"
  nofort="-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0"
  rc=0
  # A missing sanitizer runtime (libclang_rt / libasan) is a packaging gap in the
  # container, not a defect in this code, so those cells report SKIP not FAIL.
  runtime_missing() { grep -qE 'cannot find .*(libclang_rt|libasan|libubsan)|libasan\.so|libubsan\.so' "$1"; }

  cell() { # [xfail=<reason>] name compiler flags...
    local xfail=""
    case "$1" in xfail=*) xfail=${1#xfail=}; shift ;; esac
    local name=$1 cc=$2; shift 2
    if ! command -v "$cc" >/dev/null 2>&1; then printf '  %-28s SKIP (no %s)\n' "$name" "$cc"; return; fi
    if ! $cc $base "$@" pjpg.cpp -o "$WORK/$name" > "$WORK/$name.err" 2>&1; then
      if runtime_missing "$WORK/$name.err"; then
        printf '  %-28s SKIP (sanitizer runtime not installed)\n' "$name"; return
      fi
      if [ -n "$xfail" ]; then printf '  %-28s XFAIL (%s)\n' "$name" "$xfail"; return; fi
      printf '  %-28s BUILD FAIL\n' "$name"; sed 's/^/      /' "$WORK/$name.err" | head -5; rc=1; return
    fi
    emit_golden "$WORK/$name" > "$WORK/$name.out" 2>&1 || true
    if diff -a -q "$ref" "$WORK/$name.out" > /dev/null 2>&1; then
      if [ -n "$xfail" ]; then printf '  %-28s XPASS -- expected to fail (%s)\n' "$name" "$xfail"; fi
      printf '  %-28s PASS\n' "$name"
    elif [ -n "$xfail" ]; then
      printf '  %-28s XFAIL (%s)\n' "$name" "$xfail"
    else
      printf '  %-28s OUTPUT MISMATCH\n' "$name"; rc=1
    fi
  }
  echo "build/run matrix:"
  for cc in g++ clang++; do
    tag=${cc%%+*}
    for o in -O0 -O1 -O2 -O3 -Ofast; do cell "$tag$o" $cc $o $cg; done
    cell "$tag-libc-coro"  $cc -O2 $cg -DCORO_NOASM=1 $nofort
    cell "$tag-lto"        $cc -O2 $cg -flto
    cell "$tag-static"     $cc -O2 $cg -static
    cell "$tag-native"     $cc -Ofast $cg -march=native
    cell "$tag-no-pie"     $cc -O2 $cg -no-pie
    cell "$tag-pie"        $cc -O2 $cg -fPIE -pie
    cell "$tag-ubsan"      $cc -O1 -g -fsanitize=undefined
    # AddressSanitizer instruments and poisons stack frames, which is exactly what
    # the coroutine hand-copies in and out of Coroutine::stk[].  Known incompatible;
    # recorded here so the limitation stays visible instead of silently untested.
    cell xfail="ASan cannot follow the hand-copied coroutine stack" \
         "$tag-asan"       $cc -O1 -g -fsanitize=address
  done
  [ $rc -eq 0 ] && echo "matrix: all cells pass" || echo "matrix: FAILURES above"
  exit $rc
  ;;

*)
  echo "usage: $0 {check|golden|corpus|crosscheck|matrix} [binary] [testdir]" >&2; exit 2 ;;
esac
