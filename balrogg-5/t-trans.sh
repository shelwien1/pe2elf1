#!/bin/sh
#  The tsvtrans round trip, over as many .ogg files as you name.
#
#      ./t-trans.sh                    the bundled 00.ogg
#      ./t-trans.sh testfiles/         every .ogg in the directory
#
#      balrogg  c  X.ogg  X.tsv        Ogg Vorbis -> TSV
#      tsvtrans c  X.tsv  X.tt         TSV        -> transformed TSV
#      tsvtrans d  X.tt   X.tsr        transformed -> TSV
#
#  and the file passes only if X.tsv == X.tsr, byte for byte.  The `xz`
#  column is there to be looked at rather than asserted on: see TSVTRANS.md
#  for why it goes the way it does.
#
#      OPTS=-R ./t-trans.sh testfiles/    pass options to both directions

set -u

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BALROGG=${BALROGG:-$here/balrogg}
TSVTRANS=${TSVTRANS:-$here/tsvtrans}
OPTS=${OPTS:-}

for prog in "$BALROGG" "$TSVTRANS"; do
  [ -x "$prog" ] && continue
  echo "t-trans.sh: $prog is missing -- run make first" >&2
  exit 2
done

out=${OUT:-$(mktemp -d "${TMPDIR:-/tmp}/t-trans.XXXXXX")} || exit 2
mkdir -p "$out" || exit 2
cleanup() { [ -n "${KEEP:-}" ] || [ -n "${OUT:-}" ] || rm -rf "$out"; }
trap cleanup EXIT INT TERM

[ $# -eq 0 ] && set -- "$here/00.ogg"
list=$out/.inputs
: > "$list"
for a in "$@"; do
  if [ -d "$a" ]; then find "$a" -maxdepth 1 -name '*.ogg' | sort >> "$list"
  else printf '%s\n' "$a" >> "$list"
  fi
done
[ -s "$list" ] || { echo "t-trans.sh: no .ogg inputs" >&2; exit 2; }

printf '%-22s %-10s %10s %10s\n' file result tsv transformed
pass=0 fail=0

while IFS= read -r src; do
  [ -n "$src" ] || continue
  b=$(basename "$src" .ogg)
  w=$out/$b
  why=""

  "$BALROGG" c "$src" "$w.tsv" || why="balrogg c"
  [ -z "$why" ] && { $TSVTRANS c $OPTS "$w.tsv" "$w.tt"  || why="tsvtrans c"; }
  [ -z "$why" ] && { $TSVTRANS d $OPTS "$w.tt" "$w.tsr"  || why="tsvtrans d"; }
  [ -z "$why" ] && { cmp -s "$w.tsv" "$w.tsr" || why="tsv differs"; }

  sz() { [ -f "$1" ] && wc -c < "$1" || echo -; }
  if [ -z "$why" ]; then
    pass=$((pass + 1))
    printf '%-22s %-10s %10s %10s\n' "$b" ok "$(sz "$w.tsv")" "$(sz "$w.tt")"
  else
    fail=$((fail + 1))
    printf '%-22s %-10s %s\n' "$b" FAILED "$why"
  fi
  [ -n "${KEEP:-}" ] || rm -f "$w.tsv" "$w.tt" "$w.tsr"
done < "$list"

echo
echo "$pass passed, $fail failed"
[ -n "${KEEP:-}${OUT:-}" ] && echo "scratch: $out"
[ "$fail" -eq 0 ]
