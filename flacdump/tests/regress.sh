#!/bin/bash
# Regression tests for robustness bugs (crafted inputs in tests/regress):
#   valid/    valid but unusual FLAC streams (truncated or overflowing garbage frames, extreme
#             frame numbers, rarely used model paths): c (with verify), then d must restore them
#   invalid/  files that are not valid FLAC: c must refuse them cleanly (exit 1)
#   corrupt/  .meta + .wav pairs that do not decode: d must fail cleanly (exit 1, no crash/hang)
#   tests/regress.sh [-b path/to/flacdump]
dir="$(cd "$(dirname "$0")" && pwd)"
fd="$dir/../flacdump"
if [ "$1" = "-b" ]; then fd="$2"; shift 2; fi
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
fail=0
report() { printf "%-8s %s\n" "$1" "$2"; [ "$1" = OK ] || fail=1; }
for f in "$dir"/regress/valid/*.flac; do
    n=$(basename "$f" .flac)
    if ! timeout 60 "$fd" c "$f" "$tmp/$n" > /dev/null 2> "$tmp/err"; then report FAIL "valid/$n: c: $(head -c 200 "$tmp/err")"; continue; fi
    if ! timeout 60 "$fd" d "$tmp/$n" "$tmp/$n.flac" 2> "$tmp/err"; then report FAIL "valid/$n: d: $(head -c 200 "$tmp/err")"; continue; fi
    cmp -s "$f" "$tmp/$n.flac" && report OK "valid/$n" || report MISMATCH "valid/$n"
done
for f in "$dir"/regress/invalid/*.flac; do
    n=$(basename "$f" .flac)
    timeout 60 "$fd" c "$f" "$tmp/$n" > /dev/null 2> "$tmp/err"
    rc=$?
    [ $rc = 1 ] && report OK "invalid/$n" || report FAIL "invalid/$n: exit $rc $(head -c 200 "$tmp/err")"
done
for m in "$dir"/regress/corrupt/*.meta; do
    n=$(basename "$m" .meta)
    cp "$m" "$tmp/$n.meta"
    cp "${m%.meta}.wav" "$tmp/$n.wav"
    timeout 60 "$fd" d "$tmp/$n" "$tmp/$n.out.flac" > /dev/null 2> "$tmp/err"
    rc=$?
    if [ $rc != 1 ]; then report FAIL "corrupt/$n: exit $rc $(head -c 200 "$tmp/err")"
    elif [ -e "$tmp/$n.out.flac" ]; then report FAIL "corrupt/$n: output file left behind"
    else report OK "corrupt/$n"; fi
done
exit $fail
