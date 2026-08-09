#!/usr/bin/env bash
# test.sh — the round-trip gate, against a native binary or a Windows one.
#
#   ./test.sh ./bmf                 native
#   ./test.sh --wine ./bmf.exe      under wine
#
# For every image in testfiles/:
#
#   bmf c <image>.bmp <image>.bmf
#   bmf d <image>.bmf <image>.bmp
#
# and the recovered file has to be byte-identical to the input, whole file.
# There is one compression mode now -- the binary always uses -S -Q9 -- so
# there is nothing to sweep: this is the mode.
#
# Next to each image is its reference stream — testfiles/ref_t24.bmf for
# testfiles/t24.bmp — and the compressed output has to match it byte for byte.
# That is the check that makes this a refactoring gate rather than a smoke test:
# a round-trip only says the code is self-consistent, while the reference says
# the code still encodes the way it did before you touched it.  A change that
# moves a stream is a change that altered behaviour, and has to be justified by
# regenerating the references deliberately (tools/mkrefs.sh) rather than by
# noticing later.
#
# BMF_TESTDIR and BMF_IMAGES override the corpus.  BMF_NOREF=1 skips the
# reference check, for the one case it is meant for: producing new references.
set -u
cd "$(dirname "$0")"

TESTDIR=${BMF_TESTDIR:-testfiles}
WORK=run

RUN=""
if [ "${1:-}" = "--wine" ]; then
  shift
  RUN=${WINE:-$(command -v wine || echo /usr/lib/wine/wine)}
  [ -x "$RUN" ] || { echo "no wine (Debian/Ubuntu: apt install wine wine32)"; exit 1; }
  export WINEDEBUG=${WINEDEBUG:--all}
  : "${WINEPREFIX:=$PWD/.wine}"
  export WINEPREFIX
fi
BIN=${1:-./bmf}
[ -x "$BIN" ] || { echo "no $BIN (run ./build.sh)"; exit 1; }

[ -d "$TESTDIR" ] || { echo "no test corpus at $TESTDIR"; exit 1; }
# Default corpus: every .bmp in TESTDIR, in a stable order.  `out_<name>.bmp`
# is not an input — it is what the decoder is expected to write for an input
# it does not reproduce byte for byte (see the comparison below).
IMAGES=${BMF_IMAGES:-$(cd "$TESTDIR" && ls *.bmp 2>/dev/null | grep -v '^out_' | sort)}
[ -n "$IMAGES" ] || { echo "no .bmp images in $TESTDIR"; exit 1; }

rm -rf "$WORK"; mkdir -p "$WORK"
cp "$BIN" "$WORK/" || exit 1
BIN="./$(basename "$BIN")"
for img in $IMAGES; do
  [ -f "$TESTDIR/$img" ] || { echo "missing test image: $TESTDIR/$img"; exit 1; }
  cp "$TESTDIR/$img" "$WORK/orig_$img"
done

fail=0
for img in $IMAGES; do
  st="${img%.bmp}"
  ref="$TESTDIR/ref_$st.bmf"
  (
    cd "$WORK"
    timeout "${BMF_TIMEOUT:-300}" $RUN "$BIN" c "orig_$img" "$st.bmf" >"$st.compress.log" 2>&1
    rc=$?; [ $rc -ne 0 ] && { echo "$st: COMPRESS FAILED (rc=$rc)"; cat "$st.compress.log"; exit 1; }
    [ -s "$st.bmf" ] || { echo "$st: NO STREAM PRODUCED"; exit 1; }
    if [ "${BMF_NOREF:-0}" != 1 ]; then
      if [ -f "../$ref" ]; then
        cmp -s "$st.bmf" "../$ref" || {
          echo "$st: STREAM CHANGED ($(stat -c%s "$st.bmf") bytes, reference is $(stat -c%s "../$ref"))"
          exit 1
        }
      else
        echo "$st: NO REFERENCE STREAM ($ref) — run tools/mkrefs.sh"
        exit 1
      fi
    fi
    timeout "${BMF_TIMEOUT:-300}" $RUN "$BIN" d "$st.bmf" "$img" >"$st.decompress.log" 2>&1
    rc=$?; [ $rc -ne 0 ] && { echo "$st: DECOMPRESS FAILED (rc=$rc)"; cat "$st.decompress.log"; exit 1; }
    # Most inputs come back byte for byte.  An RLE-compressed BMP does not:
    # BMF decodes the runs on the way in and re-encodes them on the way out,
    # with its own run splitting, so the pixels are identical and the file is
    # not.  For those, testfiles/out_<name>.bmp holds what the decoder is
    # expected to write, which still catches any change in behaviour.
    want="orig_$img"
    [ -f "../$TESTDIR/out_$img" ] && want="../$TESTDIR/out_$img"
    cmp -s "$want" "$img" || { echo "$st: NOT LOSSLESS"; exit 1; }
    printf '%-12s ok  %8s -> %8s\n' "$st" \
      "$(stat -c%s "orig_$img")" "$(stat -c%s "$st.bmf")"
    exit 0
  ) || fail=1
done
# The archive holds more than one member.  `bmf c` appends, `bmf d` walks every
# member and decodes each one, and the writing half of that was removed once by
# a change of the output mode that nothing caught, because no test built an
# archive with two images in it.  This does.
if [ "${BMF_ARCHIVE:-1}" = 1 ] && [ -f "$TESTDIR/t1.bmp" ] && [ -f "$TESTDIR/t8g.bmp" ]; then
  (
    cd "$WORK"
    rm -f arc.bmf arc.bmp
    $RUN "$BIN" c orig_t1.bmp arc.bmf >arc.log 2>&1 &&
    $RUN "$BIN" c orig_t8g.bmp arc.bmf >>arc.log 2>&1 ||
      { echo "archive: COMPRESS FAILED"; cat arc.log; exit 1; }
    one=$(stat -c%s t8g.bmf) both=$(stat -c%s arc.bmf)
    [ "$both" -gt "$one" ] || {
      echo "archive: SECOND MEMBER REPLACED THE FIRST ($both bytes, t8g alone is $one)"
      exit 1
    }
    $RUN "$BIN" d arc.bmf arc.bmp >arc.d.log 2>&1 ||
      { echo "archive: DECOMPRESS FAILED"; cat arc.d.log; exit 1; }
    grep -q 'number: 2' arc.d.log || {
      echo "archive: SECOND MEMBER NOT READ BACK"; cat arc.d.log; exit 1
    }
    cmp -s orig_t8g.bmp arc.bmp || { echo "archive: LAST MEMBER NOT LOSSLESS"; exit 1; }
    printf '%-12s ok  %8s -> %8s  (2 members)\n' archive \
      "$(stat -c%s orig_t1.bmp)" "$both"
    exit 0
  ) || fail=1
fi

# Input the program is expected to refuse.  Every check above hands it a file
# it can read, so the whole of the error half -- the "bad file!", "Read error!"
# and "Can't open file:" exits, and the frees that unwind to them -- ran only in
# whatever way a passing run happens to reach it, which is not at all.  These
# runs are the cheapest coverage left, and they check the thing worth checking
# about an error path: that it is an exit and not a crash.
#
# The exit status is pinned rather than merely required to be nonzero.  A status
# is behaviour the same way a stream is, and these are the runs that decide
# which one you get; leaving them as "not zero" would let 3 and 4 trade places
# unnoticed.  A status of 124 is the timeout and anything at or above 128 is a
# fatal signal, so those can never be an expectation here.
#
# The empty file is the one case that is not an error: an archive is a sequence
# of members and a file with none in it decodes to nothing, successfully.
if [ "${BMF_MALFORMED:-1}" = 1 ]; then
  (
    cd "$WORK"
    ref=$(ls -- *.bmf 2>/dev/null | grep -v '^arc\.' | head -1)
    img=$(ls -- orig_*.bmp 2>/dev/null | head -1)
    [ -n "$ref" ] && [ -n "$img" ] || { echo "malformed: no stream to truncate"; exit 1; }
    : >empty.bmf
    head -c 2000 /dev/zero >zeros.bmf
    head -c 2000 /dev/zero | tr '\0' '\377' >ones.bmf
    for n in 4 40 400 4000 40000; do head -c $n "$ref" >"cut$n.bmf"; done
    head -c 6000 "$img" >cut.bmp
    rm -f gone.bmp gone.bmf

    bad=0
    # mode  input        want  what it should hit
    while read -r mode file want _; do
      [ -n "${mode:-}" ] || continue
      rm -f mal.out
      timeout "${BMF_TIMEOUT:-300}" $RUN "$BIN" $mode "$file" mal.out >mal.log 2>&1
      rc=$?
      if [ "$rc" != "$want" ]; then
        echo "malformed: $BIN $mode $file exited $rc, expected $want"
        cat mal.log; bad=1; continue
      fi
      [ -f mal.out ] && { echo "malformed: $BIN $mode $file wrote an output anyway"; bad=1; }
    done <<'CASES'
d empty.bmf   0   an archive with no members in it
d cut4.bmf    3   the header read, short
d cut40.bmf   3   the header read, torn
d cut400.bmf  3   the member read, short
d cut4000.bmf 3   inside the first member
d cut40000.bmf 3  most of a member
d zeros.bmf   3   a stream that is not one
d ones.bmf    3   the other end of the same
d gone.bmf    6   an input that is not there
c gone.bmp    6   the same, compressing
c cut.bmp     4   a BMP whose pixels run out
CASES

    # The two files that are each other's wrong kind.  Named separately because
    # they come from the corpus rather than from anything written above.
    for pair in "d $img 3" "c $ref 4"; do
      set -- $pair
      rm -f mal.out
      timeout "${BMF_TIMEOUT:-300}" $RUN "$BIN" "$1" "$2" mal.out >mal.log 2>&1
      rc=$?
      [ "$rc" = "$3" ] || { echo "malformed: $BIN $1 $2 exited $rc, expected $3"; cat mal.log; bad=1; }
    done

    # No arguments at all, and a mode letter that is neither c nor d.
    for args in "" "x a b"; do
      timeout "${BMF_TIMEOUT:-300}" $RUN "$BIN" $args >mal.log 2>&1
      rc=$?
      [ "$rc" = 1 ] || { echo "malformed: $BIN $args exited $rc, expected 1"; cat mal.log; bad=1; }
    done

    [ $bad -eq 0 ] || exit 1
    printf '%-12s ok  refused 15 inputs, no crash\n' malformed
    exit 0
  ) || fail=1
fi

[ $fail -eq 0 ] || { echo "FAIL"; exit 1; }
echo "PASS"
