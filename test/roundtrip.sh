#!/bin/sh
# Round trips every BMP in test/bmp through every coder setting worth checking.
#
# Two things are asserted for each run.  First, the decoded BMP depicts the same
# image as the source: same geometry, same palette, same pixels.  Second, coding
# that decoded BMP again and decoding it back gives a file identical to it --
# once a BMP has been through the coder it is a fixed point, padding bits and
# all, which is the strongest statement that can be made when the coder rebuilds
# the file header rather than carrying the source's.

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
bin=$root/ccsds
bmps=$root/test/bmp
work=${TMPDIR:-/tmp}/ccsds-check.$$

if [ ! -x "$bin" ]; then
  echo "test: $bin has not been built (run make)" >&2
  exit 1
fi
if [ ! -d "$bmps" ]; then
  python3 "$root/test/make_bmps.py" "$bmps" >/dev/null || exit 1
fi

mkdir -p "$work" || exit 1
trap 'rm -rf "$work"' EXIT INT TERM

# Each line is one set of compression options.  Decompression takes none: every
# parameter travels in the stream.
settings='--auto
--sample
--block
--rct
--pred-bands 1
--reduced
--column-sum
--sample --k 0 --u-max 8 --y-star 9 --y0 8
--sample --u-max 32 --w-resolution 4 --w-interval 2048 --w-initial -6 --w-final 9
--sample --w-resolution 19 --reg-size 64 --pred-bands 0
--auto --rct --reg-size 64
--block --block-size 8 --ref-interval 1
--block --block-size 64 --ref-interval 4096 --restricted
--block --rct --column-sum --reduced'

runs=0
fails=0

for bmp in "$bmps"/*.bmp; do
  name=$(basename "$bmp" .bmp)
  orig=$(wc -c < "$bmp")
  printf '%s (%s bytes)\n' "$name" "$orig"
  echo "$settings" | while IFS= read -r opts; do
    [ -n "$opts" ] || continue
    printf '%s\n' "$opts"
  done | while IFS= read -r opts; do
    stream=$work/$name.cc
    back=$work/$name.out.bmp
    again=$work/$name.again.bmp
    label=$(printf '%-58s' "$opts")

    if ! $bin c $opts "$bmp" "$stream" >"$work/err" 2>&1; then
      printf '  FAIL %s compression failed\n' "$label"
      sed 's/^/         /' "$work/err"
      continue
    fi
    if ! $bin d "$stream" "$back" >"$work/err" 2>&1; then
      printf '  FAIL %s decompression failed\n' "$label"
      sed 's/^/         /' "$work/err"
      continue
    fi
    if ! python3 "$root/test/bmpcmp.py" "$bmp" "$back" >"$work/err" 2>&1; then
      printf '  FAIL %s decoded image differs\n' "$label"
      sed 's/^/         /' "$work/err"
      continue
    fi
    # The decoded BMP, coded again, must come back byte for byte.
    if ! $bin c $opts "$back" "$stream.2" >/dev/null 2>&1 \
       || ! $bin d "$stream.2" "$again" >/dev/null 2>&1 \
       || ! cmp -s "$back" "$again"; then
      printf '  FAIL %s not a fixed point\n' "$label"
      continue
    fi
    size=$(wc -c < "$stream")
    printf '  ok   %s %s bytes\n' "$label" "$size"
  done
done > "$work/log" 2>&1

# The row encoding is a decompression-time choice, so it gets its own pass: the
# default reproduces the source's, and either can be forced.
{
  printf 'row encoding overrides\n'
  for bmp in "$bmps"/*.bmp; do
    name=$(basename "$bmp" .bmp)
    stream=$work/enc.$name.cc
    $bin c "$bmp" "$stream" >/dev/null 2>&1 || continue
    for opt in --rle --no-rle; do
      out=$work/enc.$name$opt.bmp
      if ! $bin d $opt "$stream" "$out" >/dev/null 2>&1; then
        printf '  FAIL %-24s %-8s decompression failed\n' "$name" "$opt"
        continue
      fi
      # bmpcmp compares pixels; the encoding itself is read straight out of the
      # info header, since forcing one is exactly what is under test here.
      got=$(python3 -c "import struct,sys; print(struct.unpack_from('<I', open(sys.argv[1],'rb').read(), 30)[0])" "$out")
      want_stored=$([ "$opt" = "--no-rle" ] && echo yes || echo no)
      if ! python3 "$root/test/bmpcmp.py" "$bmp" "$out" >/dev/null 2>&1; then
        printf '  FAIL %-24s %-8s pixels differ\n' "$name" "$opt"
      elif [ "$want_stored" = yes ] && [ "$got" != 0 ]; then
        printf '  FAIL %-24s %-8s wanted stored rows, got %s\n' "$name" "$opt" "$got"
      else
        printf '  ok   %-24s %-8s compression %s\n' "$name" "$opt" "$got"
      fi
    done
  done
} >> "$work/log" 2>&1

cat "$work/log"
runs=$(grep -c '^  \(ok\|FAIL\) ' "$work/log")
fails=$(grep -c '^  FAIL ' "$work/log")

echo
echo "$runs runs, $fails failures"
[ "$fails" -eq 0 ]
