#!/bin/bash
# Round-trip test: for each FLAC given, run "flacdump c" and "flacdump d" and
# compare the restored file with the original byte by byte.
#   tests/roundtrip.sh [-b path/to/flacdump] file1.flac [file2.flac ...]
fd="$(dirname "$0")/../flacdump"
if [ "$1" = "-b" ]; then fd="$2"; shift 2; fi
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
fail=0
for f in "$@"; do
    if ! "$fd" c -n "$f" "$tmp/x" > /dev/null 2> "$tmp/err"; then
        echo "FAIL(c)  $f: $(cat "$tmp/err")"; fail=1; continue
    fi
    if ! "$fd" d "$tmp/x" "$tmp/r.flac" 2> "$tmp/err"; then
        echo "FAIL(d)  $f: $(cat "$tmp/err")"; fail=1; continue
    fi
    if cmp -s "$f" "$tmp/r.flac"; then
        printf "OK       %-40s flac=%10d meta=%7d\n" "$f" "$(stat -c %s "$f")" "$(stat -c %s "$tmp/x.meta")"
    else
        echo "MISMATCH $f"; fail=1
    fi
done
exit $fail
