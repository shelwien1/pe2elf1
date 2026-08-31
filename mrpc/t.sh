#!/bin/sh
# Round-trip and reference check.  For each image: encode on the device,
# encode on the host, decode both, and compare against the input.  The two
# encodes are not expected to produce the same file -- see README -- so
# what is checked is that each decodes back to the original, and what is
# reported is what each cost.
#
# The two picture modes are checked too, since they run the same encoder:
# `p` has to produce a file of the input's own geometry and say what it
# cost -- of the widened geometry, at 8bpp, if the input was 1 or 4bpp
# packed -- and `q` an 8bpp class map of one plane, whatever went in.
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
  $M -C p "$f" "$T/p.bmp" 2>/dev/null >/dev/null || { echo "$f: plot failed"; fail=1; continue; }
  # the plot is the input's header and a raster of the same geometry, and
  # nothing else -- so it is the input's size less whatever trailed it.
  # A 1 or 4bpp file is the exception: a code length does not fit in its
  # depth, so its plot is an 8bpp file of the widened width and is bigger
  # by exactly the ratio of the depths
  bpp=$(od -An -tu2 -j28 -N2 "$f" | tr -d ' ')
  case "$bpp" in
    1|4) lim=$(( ($(wc -c <"$f") * 8 / bpp) + 1078 )) ;;
    *)   lim=$(wc -c <"$f") ;;
  esac
  if [ ! -s "$T/p.bmp" ] || [ "$(wc -c <"$T/p.bmp")" -gt "$lim" ]; then
    echo "$(basename "$f"): the plot is not the shape of the image"; fail=1
  fi
  # the class map: 8bpp of one plane, so 54 + 1024 + a padded row per row,
  # and every class it names has to be one the encode could have used
  $M -C q "$f" "$T/q.bmp" 2>"$T/qerr" >/dev/null || { echo "$f: class map failed"; fail=1; continue; }
  qw=$(od -An -tu4 -j18 -N4 "$T/q.bmp" | tr -d ' ')
  qh=$(od -An -tu4 -j22 -N4 "$T/q.bmp" | tr -d ' ')
  qb=$(od -An -tu2 -j28 -N2 "$T/q.bmp" | tr -d ' ')
  # biHeight is signed and the map keeps the input's row order, so a
  # top-down image gives it back as a negative
  [ "$qh" -gt 2147483647 ] && qh=$((4294967296 - qh))
  want=$(( 1078 + ((qw + 3) / 4 * 4) * qh ))
  if [ "$qb" != 8 ] || [ "$(wc -c <"$T/q.bmp")" -ne "$want" ]; then
    echo "$(basename "$f"): the class map is not an 8bpp plane of its geometry"; fail=1
  fi
  if ! grep -q "pixels over" "$T/qerr"; then
    echo "$(basename "$f"): the class map said nothing about the classes"; fail=1
  fi
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
rm -f "$T/err" "$T/qerr" "$T/a.mrp" "$T/b.mrp" "$T/a.bmp" "$T/b.bmp" "$T/p.bmp" "$T/q.bmp"
exit $fail
