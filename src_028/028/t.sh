#!/bin/sh
# roundtrip + size check over every .dds given (or over *.dds)
set -e
[ $# -gt 0 ] && set -- "$@" || set -- *.dds
for f in "$@"; do
  ./dxt5comp c "$f" _t.d5c >/dev/null
  ./dxt5comp d _t.d5c _t.dds >/dev/null
  if cmp -s "$f" _t.dds; then
    printf "OK   %-24s %9d -> %9d\n" "$f" `stat -c%s "$f"` `stat -c%s _t.d5c`
  else
    printf "FAIL %s\n" "$f"; exit 1
  fi
done
rm -f _t.d5c _t.dds
