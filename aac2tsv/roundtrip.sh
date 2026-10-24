#!/bin/sh
# roundtrip.sh BIN files...  - convert each file to TSV and back, compare with
# the original; prints the parse statistics of each file.
BIN=$1
shift
tmp=${TMPDIR:-/tmp}/aac2tsv.$$
fail=0
for f in "$@"; do
    stats=$("$BIN" c -v "$f" "$tmp.tsv" 2>&1 | tail -n 1)
    if "$BIN" d "$tmp.tsv" "$tmp.out" && cmp -s "$f" "$tmp.out"; then
        echo "ok    $f: $stats"
    else
        echo "FAIL  $f"
        fail=1
    fi
done
rm -f "$tmp.tsv" "$tmp.out"
exit $fail
