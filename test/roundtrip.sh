#!/bin/sh
# roundtrip.sh BIN files... -- compress each file with BIN c, decompress it
# with BIN d and compare; prints the sizes. Exit status 1 if any fails, is
# not there, or there is none. OPTS are more options for c; a file named
# NAME.asc-HEX.raw is raw access units, compressed with -asc HEX.
BIN=$1
shift
tmp=${TMPDIR:-/tmp}/aacpack.$$
fail=0
n=0
tin=0
tout=0
for f in "$@"; do
  if [ ! -f "$f" ]; then
    echo "FAIL  $f: no such file"
    fail=1
    continue
  fi
  n=$((n + 1))
  asc=
  case "$f" in
  *.asc-*.raw) asc="-asc $(echo "$f" | sed 's/.*\.asc-\([0-9a-fA-F]*\)\.raw$/\1/')" ;;
  esac
  if "$BIN" c $OPTS $asc "$f" "$tmp.apk" 2> "$tmp.err" && "$BIN" d "$tmp.apk" "$tmp.out" 2>> "$tmp.err" && cmp -s "$f" "$tmp.out"; then
    a=$(wc -c < "$f")
    b=$(wc -c < "$tmp.apk")
    tin=$((tin + a))
    tout=$((tout + b))
    printf 'ok    %10d %10d %7s  %s\n' "$a" "$b" "$(awk "BEGIN { if ($a) printf \"%.2f%%\", 100 * $b / $a; else print \"-\" }")" "$f"
  else
    echo "FAIL  $f"
    sed 's/^/      /' "$tmp.err"
    fail=1
  fi
done
[ "$n" -gt 0 ] || { echo "FAIL  no files to try"; fail=1; }
[ "$tin" -gt 0 ] && printf 'total %10d %10d %6.2f%%\n' "$tin" "$tout" "$(awk "BEGIN { print 100 * $tout / $tin }")"
rm -f "$tmp.apk" "$tmp.out" "$tmp.err"
exit $fail
