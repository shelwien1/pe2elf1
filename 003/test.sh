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
[ $fail -eq 0 ] || { echo "FAIL"; exit 1; }
echo "PASS"
