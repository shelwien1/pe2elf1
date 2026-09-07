#!/bin/sh
#  The round trip t.bat runs, over as many files as you name.
#
#      ./t.sh                     the bundled 00.ogg
#      ./t.sh testfiles/          every .ogg in the directory
#      ./t.sh a.ogg b.ogg         just those
#
#  For each input:
#
#      balrogg c  X.ogg  X.tsv          Ogg Vorbis  -> TSV record stream
#      tsv2wav c  X.tsv  X.wav X.meta   TSV         -> WAV + meta
#      tsv2wav d  X.wav  X.tsr X.meta   WAV + meta  -> TSV
#      balrogg d  X.tsr  X.ogr          TSV         -> Ogg Vorbis
#
#  and the file passes only if X.tsv == X.tsr *and* X.ogg == X.ogr, byte for
#  byte.  Exit status is 0 only when every file passed.
#
#  Work lands in a scratch directory, so the inputs are never written next to
#  themselves and two runs cannot collide:
#
#      TSV2WAV=./tsv2wav-vorbis ./t.sh testfiles/   pick the build to test
#      KEEP=1 ./t.sh testfiles/                     leave the scratch dir
#      OUT=/tmp/rt ./t.sh testfiles/                put it somewhere chosen

set -u

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BALROGG=${BALROGG:-$here/balrogg}
TSV2WAV=${TSV2WAV:-$here/tsv2wav}

for prog in "$BALROGG" "$TSV2WAV"; do
  [ -x "$prog" ] && continue
  echo "t.sh: $prog is missing -- run make first" >&2
  exit 2
done

out=${OUT:-$(mktemp -d "${TMPDIR:-/tmp}/t.sh.XXXXXX")} || exit 2
mkdir -p "$out" || exit 2
cleanup() { [ -n "${KEEP:-}" ] || [ -n "${OUT:-}" ] || rm -rf "$out"; }
trap cleanup EXIT INT TERM

#  Collect the inputs, one path per line: a directory contributes every .ogg in
#  it.  The list goes through a file read with `read -r`, so a path with spaces
#  stays one path -- iterating an unquoted $list would split it on every space.
[ $# -eq 0 ] && set -- "$here/00.ogg"
list=$out/.inputs
: > "$list"
for a in "$@"; do
  if [ -d "$a" ]; then find "$a" -maxdepth 1 -name '*.ogg' | sort >> "$list"
  else printf '%s\n' "$a" >> "$list"
  fi
done
[ -s "$list" ] || { echo "t.sh: no .ogg inputs" >&2; exit 2; }

printf '%-22s %-10s %10s %10s %10s %8s\n' file result ogg tsv meta wav
pass=0 fail=0

while IFS= read -r src; do
  [ -n "$src" ] || continue
  b=$(basename "$src" .ogg)
  w=$out/$b
  why=""

  cp -- "$src" "$w.ogg" || { why="copy"; }
  [ -z "$why" ] && { "$BALROGG" c "$w.ogg" "$w.tsv"          || why="balrogg c"; }
  [ -z "$why" ] && { "$TSV2WAV" c "$w.tsv" "$w.wav" "$w.meta" || why="tsv2wav c"; }
  [ -z "$why" ] && { "$TSV2WAV" d "$w.wav" "$w.tsr" "$w.meta" || why="tsv2wav d"; }
  [ -z "$why" ] && { "$BALROGG" d "$w.tsr" "$w.ogr"          || why="balrogg d"; }
  [ -z "$why" ] && { cmp -s "$w.tsv" "$w.tsr" || why="tsv differs"; }
  [ -z "$why" ] && { cmp -s "$w.ogg" "$w.ogr" || why="ogg differs"; }

  sz() { [ -f "$1" ] && wc -c < "$1" || echo -; }
  if [ -z "$why" ]; then
    pass=$((pass + 1))
    printf '%-22s %-10s %10s %10s %10s %8s\n' "$b" ok \
           "$(sz "$w.ogg")" "$(sz "$w.tsv")" "$(sz "$w.meta")" "$(sz "$w.wav")"
  else
    fail=$((fail + 1))
    printf '%-22s %-10s %s\n' "$b" FAILED "$why"
  fi

  [ -n "${KEEP:-}" ] || rm -f "$w.ogg" "$w.tsv" "$w.wav" "$w.meta" "$w.tsr" "$w.ogr"
done < "$list"

echo
echo "$pass passed, $fail failed"
[ -n "${KEEP:-}${OUT:-}" ] && echo "scratch: $out"
[ "$fail" -eq 0 ]
