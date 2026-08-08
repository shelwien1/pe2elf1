#!/usr/bin/env bash
# test.sh — the round-trip gate, against a native binary or a Windows one.
#
#   ./test.sh ./bmf                 native
#   ./test.sh --wine ./bmf.exe      under wine
#
# For every image in testfiles/, compress it with -S, decompress the result,
# and require the recovered file to be byte-identical to the input, whole
# file.  -S is BMF's "slow, but efficient" mode: it exercises the second,
# heavier model on top of everything the default path already runs, so it
# reaches more of the decompiled code than a plain round-trip does.
#
# If a reference stream is sitting next to an image — testfiles/ref_t24.bmf
# for testfiles/t24.bmp, as produced by the original BMF.exe — the compressed
# output is compared against it as well.  That is the stronger check, since it
# says the bits are the donor's and not merely self-consistent; it is reported
# but does not fail the run, because the corpus ships without those files.
#
# BMF_TESTDIR, BMF_IMAGES and BMF_FLAGS override the corpus and the switches.
set -u
cd "$(dirname "$0")"

TESTDIR=${BMF_TESTDIR:-testfiles}
FLAGS=${BMF_FLAGS:--S}
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
# Default corpus: every .bmp in TESTDIR, in a stable order.
IMAGES=${BMF_IMAGES:-$(cd "$TESTDIR" && ls *.bmp 2>/dev/null | sort)}
[ -n "$IMAGES" ] || { echo "no .bmp images in $TESTDIR"; exit 1; }

rm -rf "$WORK"; mkdir -p "$WORK"
cp "$BIN" "$WORK/" || exit 1
BIN="./$(basename "$BIN")"
for img in $IMAGES; do
  [ -f "$TESTDIR/$img" ] || { echo "missing test image: $TESTDIR/$img"; exit 1; }
  cp "$TESTDIR/$img" "$WORK/"
done

fail=0
for img in $IMAGES; do
  st="${img%.bmp}"
  ref="$TESTDIR/ref_$st.bmf"
  (
    cd "$WORK"
    timeout 300 $RUN "$BIN" $FLAGS "$img" >"$st.compress.log" 2>&1
    rc=$?; [ $rc -ne 0 ] && { echo "$st: COMPRESS FAILED (rc=$rc)"; cat "$st.compress.log"; exit 1; }
    [ -s "$st.bmf" ] || { echo "$st: NO .bmf PRODUCED"; exit 1; }
    if [ -f "../$ref" ]; then
      cmp -s "$st.bmf" "../$ref" \
        || echo "$st: stream differs from the original's ($(stat -c%s "$st.bmf") vs $(stat -c%s "../$ref"))" >&2
    fi
    mv "$img" "orig_$st.bmp"
    timeout 300 $RUN "$BIN" "$st.bmf" >"$st.decompress.log" 2>&1
    rc=$?; [ $rc -ne 0 ] && { echo "$st: DECOMPRESS FAILED (rc=$rc)"; cat "$st.decompress.log"; exit 1; }
    cmp -s "orig_$st.bmp" "$img" || { echo "$st: NOT LOSSLESS"; exit 1; }
    printf '%-12s ok  %8s -> %8s\n' "$st" \
      "$(stat -c%s "orig_$st.bmp")" "$(stat -c%s "$st.bmf")"
    exit 0
  ) || fail=1
done
[ $fail -eq 0 ] || { echo "FAIL"; exit 1; }
echo "PASS"
