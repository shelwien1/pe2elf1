#!/bin/sh
#  The tsvcomp round trip, over as many .ogg files as you name.
#
#      ./t-comp.sh                    the bundled 00.ogg
#      ./t-comp.sh testfiles/         every .ogg in the directory
#
#      balrogg c  X.ogg  X.tsv        Ogg Vorbis -> TSV
#      tsvcomp c  X.tsv  X.tc         TSV        -> coded
#      tsvcomp d  X.tc   X.tsr        coded      -> TSV
#
#  and the file passes only if X.tsv == X.tsr, byte for byte.  The `xz -9e`
#  column is what the same TSV costs a general-purpose compressor, and the
#  `net` column is the coded stream against the .ogg it came from: under 100%
#  is a smaller file than the one that went in.
#
#      XZ=0 ./t-comp.sh testfiles/    skip the xz column, which dominates the
#                                     running time on a large corpus

set -u

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BALROGG=${BALROGG:-$here/balrogg}
TSVCOMP=${TSVCOMP:-$here/tsvcomp}
OPTS=${OPTS:-}
XZ=${XZ:-1}

for prog in "$BALROGG" "$TSVCOMP"; do
  [ -x "$prog" ] && continue
  echo "t-comp.sh: $prog is missing -- run make first" >&2
  exit 2
done

out=${OUT:-$(mktemp -d "${TMPDIR:-/tmp}/t-comp.XXXXXX")} || exit 2
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
[ -s "$list" ] || { echo "t-comp.sh: no .ogg inputs" >&2; exit 2; }

printf '%-18s %-8s %10s %10s %10s %10s %7s\n' \
       file result ogg tsv xz coded net
pass=0 fail=0
totogg=0 totcod=0 totxz=0 tottsv=0

while IFS= read -r src; do
  [ -n "$src" ] || continue
  b=$(basename "$src" .ogg)
  w=$out/$b
  why=""

  "$BALROGG" c "$src" "$w.tsv" || why="balrogg c"
  [ -z "$why" ] && { $TSVCOMP c $OPTS "$w.tsv" "$w.tc"  || why="tsvcomp c"; }
  [ -z "$why" ] && { $TSVCOMP d $OPTS "$w.tc" "$w.tsr"  || why="tsvcomp d"; }
  [ -z "$why" ] && { cmp -s "$w.tsv" "$w.tsr" || why="tsv differs"; }

  sz() { [ -f "$1" ] && wc -c < "$1" || echo 0; }
  if [ -z "$why" ]; then
    pass=$((pass + 1))
    o=$(sz "$src") t=$(sz "$w.tsv") c=$(sz "$w.tc") x=0
    [ "$XZ" = 1 ] && x=$(xz -9e -c "$w.tsv" | wc -c)
    totogg=$((totogg + o)) totcod=$((totcod + c))
    totxz=$((totxz + x)) tottsv=$((tottsv + t))
    printf '%-18s %-8s %10s %10s %10s %10s %6s%%\n' \
           "$b" ok "$o" "$t" "$x" "$c" \
           "$(awk "BEGIN{printf \"%.1f\", 100*$c/$o}")"
  else
    fail=$((fail + 1))
    printf '%-18s %-8s %s\n' "$b" FAILED "$why"
  fi
  [ -n "${KEEP:-}" ] || rm -f "$w.tsv" "$w.tc" "$w.tsr"
done < "$list"

echo
if [ "$totogg" -gt 0 ]; then
  printf '%-18s %-8s %10s %10s %10s %10s %6s%%\n' total "" \
         "$totogg" "$tottsv" "$totxz" "$totcod" \
         "$(awk "BEGIN{printf \"%.1f\", 100*$totcod/$totogg}")"
  [ "$totxz" -gt 0 ] && awk "BEGIN{printf \"tsvcomp is %.2f%% of xz -9e on the same TSVs\\n\", 100*$totcod/$totxz}"
  echo
fi
echo "$pass passed, $fail failed"
[ -n "${KEEP:-}${OUT:-}" ] && echo "scratch: $out"
[ "$fail" -eq 0 ]
