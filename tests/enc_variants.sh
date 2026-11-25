#!/bin/bash
# M4 check: qpdf-encrypted variants (each R, empty user password) must extract the same payload
# files as a plain qpdf rewrite of the same input, with no options, and round-trip byte-exact.
#
#   tests/enc_variants.sh [-b pdfdump] [-o workdir] files...
#
# Needs qpdf on PATH.

PDFDUMP=./pdfdump
WORK=${TMPDIR:-/tmp}/pdfdump-enc.$$
while getopts "b:o:" o; do
  case $o in
    b) PDFDUMP=$OPTARG ;;
    o) WORK=$OPTARG ;;
    *) exit 2 ;;
  esac
done
shift $((OPTIND - 1))
mkdir -p "$WORK"

# name: qpdf encryption arguments (user password empty, owner "owner")
VARIANTS=(
  "r2:--allow-weak-crypto --encrypt --user-password= --owner-password=owner --bits=40 --"
  "r3:--allow-weak-crypto --encrypt --user-password= --owner-password=owner --bits=128 --use-aes=n --"
  "r4:--encrypt --user-password= --owner-password=owner --bits=128 --use-aes=y --"
  "r6:--encrypt --user-password= --owner-password=owner --bits=256 --"
)

# hash list of extracted payload files (own files + class files), names stripped
payload_hashes() {
  local p=$1
  { for f in "$p"[0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F][0-9A-F].*; do
      [ -f "$f" ] && echo "${f##*.} $(sha1sum < "$f" | cut -c1-40)"
    done
    for c in cs cmap; do [ -f "$p.$c" ] && echo "$c $(sha1sum < "$p.$c" | cut -c1-40)"; done
  } | sort
}

fail=0
n=0
for f in "$@"; do
  n=$((n + 1))
  b="$WORK/f$n"
  rm -f "$b-plain.pdf"
  qpdf --static-id --decrypt "$f" "$b-plain.pdf" 2>/dev/null
  [ -s "$b-plain.pdf" ] || { echo "qpdf failed on $f"; continue; }
  $PDFDUMP c -q "$b-plain.pdf" "$b-plain" || { echo "FAIL c plain $f"; fail=$((fail + 1)); continue; }
  payload_hashes "$b-plain" > "$b-plain.h"
  for v in "${VARIANTS[@]}"; do
    name=${v%%:*}
    args=${v#*:}
    rm -f "$b-$name.pdf"
    qpdf --static-id --decrypt $args "$f" "$b-$name.pdf" 2>/dev/null
    if [ ! -s "$b-$name.pdf" ]; then echo "skip $name ($f): qpdf could not write it"; continue; fi
    $PDFDUMP c -q "$b-$name.pdf" "$b-$name" || { echo "FAIL c $name $f"; fail=$((fail + 1)); continue; }
    $PDFDUMP d "$b-$name" "$b-$name.out" && cmp -s "$b-$name.pdf" "$b-$name.out" || {
      echo "FAIL round trip $name $f"; fail=$((fail + 1)); continue; }
    payload_hashes "$b-$name" > "$b-$name.h"
    if cmp -s "$b-plain.h" "$b-$name.h"; then
      echo "ok   $name $(wc -l < "$b-$name.h") payload files identical to plain: $f"
    else
      echo "DIFF $name: $(diff "$b-plain.h" "$b-$name.h" | grep -c '^[<>]') differing payload files: $f"
      fail=$((fail + 1))
    fi
  done
done
[ $fail -eq 0 ] && echo "all variants ok" || echo "$fail failures"
[ $fail -eq 0 ]
