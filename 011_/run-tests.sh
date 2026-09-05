#!/usr/bin/env bash
# Linux test driver for pjpg.  Invoked by the Makefile; also usable directly:
#
#   ./run-tests.sh check   ./pjpg ../testfiles   regression vs tests/golden.log
#   ./run-tests.sh golden  ./pjpg ../testfiles   regenerate tests/golden.log
#   ./run-tests.sh corpus  ./pjpg ../testfiles   run every jpeg, incl. imagetestsuite
#   ./run-tests.sh carve   ./pjpg ../testfiles   jpegdet round trip + decodability
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

  # 4) Huffman and arithmetic must agree on losslessly-transcoded pairs
  if [ -d "$TESTDIR/coders" ]; then
    if out=$("$0" coders "$BIN" "$TESTDIR" 2>&1) && [ -n "$out" ]; then
      echo "PASS  coders    ${out#coders: }"
    else
      echo "FAIL  coders"; echo "$out" | sed 's/^/  /'; fail=1
    fi
  fi

  # 5) the carver: extract, rebuild, compare, and open what was extracted
  if [ -x "$HERE/jpegdet" ]; then
    if out=$("$0" carve "$BIN" "$TESTDIR" 2>&1) && [ -n "$out" ]; then
      echo "PASS  carve     ${out##*carve: }"
    else
      echo "FAIL  carve"; echo "$out" | sed 's/^/  /'; fail=1
    fi
  else
    echo "SKIP  carve     jpegdet not built"
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

coders)
  # Cross-coder invariant: jpegtran transcodes losslessly in the DCT domain, so
  # the Huffman and arithmetic variants of an image hold identical coefficients
  # and must produce identical MCU and block counts.  Any disagreement is an
  # entropy decoder bug.  No scan may need a resync (which would mean the
  # decoder did not land on the terminating marker) or end up INCOMPLETE.
  cd "$HERE" || exit 1
  dir="$TESTDIR/coders"
  if [ ! -d "$dir" ]; then echo "coders: $dir missing"; exit 0; fi
  blocks() { timeout 120 "$BIN" "$1" 2>&1 | grep -a '^  scan ' \
             | sed 's/.*MCUs, \([0-9]*\) blocks.*/\1/' | paste -sd+ | bc; }
  n=0; bad=0
  for base in "$dir"/*.base.jpg; do
    [ -e "$base" ] || continue
    b=${base%.base.jpg}
    n=$((n+1))
    for v in base prog arith arithprog; do
      [ -f "$b.$v.jpg" ] || { echo "  MISSING $(basename "$b").$v.jpg"; bad=$((bad+1)); continue; }
      o=$(timeout 120 "$BIN" "$b.$v.jpg" 2>&1)
      echo "$o" | grep -qa resync     && { echo "  RESYNC in $(basename "$b").$v"; bad=$((bad+1)); }
      echo "$o" | grep -qa INCOMPLETE && { echo "  INCOMPLETE in $(basename "$b").$v"; bad=$((bad+1)); }
    done
    hs=$(blocks "$b.base.jpg");  as=$(blocks "$b.arith.jpg")
    hp=$(blocks "$b.prog.jpg");  ap=$(blocks "$b.arithprog.jpg")
    [ "$hs" = "$as" ] || { echo "  $(basename "$b") sequential: Huffman $hs blocks vs arithmetic $as"; bad=$((bad+1)); }
    [ "$hp" = "$ap" ] || { echo "  $(basename "$b") progressive: Huffman $hp blocks vs arithmetic $ap"; bad=$((bad+1)); }
  done
  echo "coders: $n images x 4 variants, $bad problems"
  [ $bad -eq 0 ]
  ;;

carve)
  # jpegdet, the carver built on the same parser.  Two invariants, and they pull
  # in opposite directions, which is the point of testing them together:
  #
  #   1. LOSSLESS.  "c" then "d" must reproduce the input byte for byte.  This
  #      has to hold for every input, including ones with no JPEG in them at
  #      all, because it is a property of the partition (every byte goes to
  #      exactly one of the metainfo file and one .jpg), not of the detection.
  #   2. DECODABLE.  Every file "c" writes must actually open.  Nothing stops a
  #      carver from being trivially lossless by never carving anything, so the
  #      streams below are built from real images and the count is checked too.
  #
  # Decodability is judged by whether djpeg produces an image, not by its exit
  # status: libjpeg exits 2 for a warning on a file it decoded perfectly well.
  cd "$HERE" || exit 1
  DET="$HERE/jpegdet"
  if [ ! -x "$DET" ]; then echo "carve: $DET missing (run make)"; exit 1; fi
  have_djpeg=1; command -v djpeg >/dev/null 2>&1 || have_djpeg=0

  # Three real images, small enough to keep the streams quick.
  mapfile -d '' -t src < <(list_golden_files)
  pick=(); for f in "${src[@]}"; do
    [ "$(stat -c %s "$f")" -lt 60000 ] && pick+=("$f")
    [ "${#pick[@]}" -ge 3 ] && break
  done
  if [ "${#pick[@]}" -lt 2 ]; then echo "carve: not enough small test images"; exit 0; fi

  mk() { # mk <name> -- builds $WORK/<name>.bin on stdin
    cat > "$WORK/$1.bin"
  }
  head -c 3000 /dev/urandom > "$WORK/noise"
  cat "${pick[0]}"                                       | mk single
  { printf 'HEAD'; cat "${pick[0]}"; printf 'TAIL'; }     | mk sandwiched
  { cat "${pick[0]}"; cat "${pick[1]}"; }                 | mk adjacent
  { cat "$WORK/noise"; cat "${pick[0]}"; cat "$WORK/noise"; cat "${pick[1]}"; cat "$WORK/noise"; } | mk noise_between
  cat "$WORK/noise"                                       | mk nojpeg
  : | mk empty
  # a JPEG whose EOI was cut off, immediately followed by another: the carver
  # has to cut the first one at the second one's SOI to get two openable files
  { head -c $(( $(stat -c %s "${pick[0]}") - 2 )) "${pick[0]}"; cat "${pick[1]}"; } | mk no_eoi_then_jpg

  # How many images each stream must yield.  Checking the count per stream, and
  # not just a total, is what stops the carver from passing by carving nothing:
  # losslessness alone is trivially satisfied by treating the whole input as
  # literal data.
  want_single=1 want_sandwiched=1 want_adjacent=2 want_noise_between=2
  want_nojpeg=0 want_empty=0 want_no_eoi_then_jpg=2

  bad=0; n=0; imgs=0
  for c in "$WORK"/*.bin; do
    name=$(basename "$c" .bin); n=$((n+1))
    rm -rf "$WORK/o"; mkdir -p "$WORK/o"
    if ! timeout 300 "$DET" c "$c" "$WORK/o/i" >"$WORK/c.log" 2>&1; then
      echo "  $name: c failed -- $(tail -1 "$WORK/c.log")"; bad=$((bad+1)); continue
    fi
    if ! timeout 300 "$DET" d "$WORK/o/i" "$WORK/r.bin" >"$WORK/d.log" 2>&1; then
      echo "  $name: d failed -- $(tail -1 "$WORK/d.log")"; bad=$((bad+1)); continue
    fi
    cmp -s "$c" "$WORK/r.bin" || { echo "  $name: round trip is not byte-exact"; bad=$((bad+1)); continue; }
    got=0
    for j in "$WORK"/o/i????????.jpg; do
      [ -e "$j" ] || continue
      got=$((got+1)); imgs=$((imgs+1))
      [ $have_djpeg = 1 ] || continue
      rm -f "$WORK/d.ppm"
      djpeg -outfile "$WORK/d.ppm" "$j" >/dev/null 2>&1
      [ -s "$WORK/d.ppm" ] || { echo "  $name: $(basename "$j") does not decode"; bad=$((bad+1)); }
    done
    eval "want=\${want_$name:-}"
    [ -z "$want" ] || [ "$got" = "$want" ] || { echo "  $name: carved $got images, expected $want"; bad=$((bad+1)); }
  done

  [ $have_djpeg = 1 ] || echo "  (djpeg not installed: decodability not checked)"
  echo "carve: $n streams, $imgs images, $bad problems"
  [ $bad -eq 0 ]
  ;;

crosscheck)
  # Build with every available compiler at -O2 and confirm all of them produce
  # byte-identical output over the whole corpus.  A disagreement between two
  # correct compilers is the signature of undefined behaviour, which matters here
  # because the coroutine hand-rolls stack switching and the parsers type-pun.
  cd "$HERE" || exit 1
  ext="$WORK/its"; mkdir -p "$ext"
  for t in "$TESTDIR"/*.tar.gz; do [ -e "$t" ] && tar -xzf "$t" -C "$ext"; done
  base="-std=c++17 -O2 -I../Lib3 -DNDEBUG -Drestrict=__restrict"
  cg="-fomit-frame-pointer -fno-stack-protector -fno-stack-check -fstrict-aliasing"
  built=()
  for cc in g++ clang++; do
    command -v "$cc" >/dev/null 2>&1 || continue
    if $cc $base $cg pjpg.cpp -o "$WORK/x_${cc%%+*}" >/dev/null 2>&1; then built+=("${cc%%+*}"); fi
  done
  if [ ${#built[@]} -lt 2 ]; then echo "crosscheck: need two compilers, have ${#built[@]}"; exit 0; fi
  ref=${built[0]}; n=0; bad=0
  while IFS= read -r -d '' f; do
    n=$((n+1))
    timeout 60 "$WORK/x_$ref" "$f" > "$WORK/a" 2>&1 || true
    for o in "${built[@]:1}"; do
      timeout 60 "$WORK/x_$o" "$f" > "$WORK/b" 2>&1 || true
      if ! diff -a -q "$WORK/a" "$WORK/b" >/dev/null; then
        echo "DISAGREE ($ref vs $o)  $f"; bad=$((bad+1))
      fi
    done
  done < <(find "$TESTDIR" "$ext" -type f \( -iname '*.jpg' -o -iname '*.jpeg' \) -print0 | LC_ALL=C sort -z)
  echo "crosscheck: ${built[*]} agree on $((n-bad))/$n files"
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
    # a frame-pointer build needs the alternative setjmp asm; both halves of
    # that switch are covered, including that the default still works without it
    cell "$tag-framepointer" $cc -O2 $cg -fno-omit-frame-pointer -DCORO_FRAME_POINTER
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
  echo "usage: $0 {check|golden|corpus|coders|crosscheck|matrix} [binary] [testdir]" >&2; exit 2 ;;
esac
