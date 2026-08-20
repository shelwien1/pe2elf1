#!/bin/sh
# Round-trip and reference check.  For each image: encode on the device,
# encode on the host, decode both, and compare against the input.  The two
# encodes are not expected to produce the same file -- see README -- so
# what is checked is that each decodes back to the original, and what is
# reported is what each cost.
#
#   ./t.sh img.bmp [img.bmp ...]

M=${MRPC:-./mrpc}
T=${TMPDIR:-/tmp}
fail=0
printf '%-28s %10s %10s %8s\n' image device host delta
for f in "$@"; do
  $M    c "$f" "$T/a.mrp" 2>"$T/err" >/dev/null || { echo "$f: encode (device) failed"; fail=1; continue; }
  if grep -q "running on the host\|falling back" "$T/err"; then
    echo "$(basename "$f"): the device was not used --"; sed 's/^/    /' "$T/err"; fail=1
  fi
  $M -C c "$f" "$T/b.mrp" >/dev/null 2>&1 || { echo "$f: encode (host) failed";   fail=1; continue; }
  $M    d "$T/a.mrp" "$T/a.bmp" >/dev/null 2>&1
  $M    d "$T/b.mrp" "$T/b.bmp" >/dev/null 2>&1
  a=$(wc -c <"$T/a.mrp"); b=$(wc -c <"$T/b.mrp")
  d=$(awk "BEGIN{printf \"%+.3f%%\", 100*($a-$b)/$b}")
  printf '%-28s %10s %10s %8s' "$(basename "$f")" "$a" "$b" "$d"
  if cmp -s "$f" "$T/a.bmp" && cmp -s "$f" "$T/b.bmp"; then
    echo "  ok"
  else
    echo "  ROUND TRIP FAILED"
    fail=1
  fi
done
rm -f "$T/err" "$T/a.mrp" "$T/b.mrp" "$T/a.bmp" "$T/b.bmp"
exit $fail
