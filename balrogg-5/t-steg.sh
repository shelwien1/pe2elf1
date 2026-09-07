#!/bin/sh
#  t.sh for tsvsteg: the same round trip, with a payload riding in the WAV.
#
#      ./t-steg.sh                     the bundled 00.ogg
#      ./t-steg.sh testfiles/          every .ogg in the directory
#
#  For each input:
#
#      balrogg c  X.ogg  X.tsv                 Ogg Vorbis -> TSV
#      tsvsteg c  X.tsv  X.wav X.meta  P       TSV        -> WAV + meta, P inside
#      tsvsteg d  X.wav  X.tsr X.meta  P.out   WAV + meta -> TSV, P back out
#      balrogg d  X.tsr  X.ogr                 TSV        -> Ogg Vorbis
#
#  and the file passes only if X.tsv == X.tsr, X.ogg == X.ogr *and*
#  P == P.out, byte for byte.
#
#      TSVSTEG=./tsvsteg-vorbis ./t-steg.sh testfiles/   pick the build
#      PAYLOAD=8192 ./t-steg.sh testfiles/               payload bytes
#
#  The default payload is small enough for any stream of a second or two; a
#  stream too short for it fails with the size that would fit, which is not a
#  bug in the tool.  See STEG.md.

set -u

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BALROGG=${BALROGG:-$here/balrogg}
TSVSTEG=${TSVSTEG:-$here/tsvsteg}
PAYLOAD=${PAYLOAD:-1024}

for prog in "$BALROGG" "$TSVSTEG"; do
  [ -x "$prog" ] && continue
  echo "t-steg.sh: $prog is missing -- run make first" >&2
  exit 2
done

out=${OUT:-$(mktemp -d "${TMPDIR:-/tmp}/t-steg.XXXXXX")} || exit 2
mkdir -p "$out" || exit 2
cleanup() { [ -n "${KEEP:-}" ] || [ -n "${OUT:-}" ] || rm -rf "$out"; }
trap cleanup EXIT INT TERM

dd if=/dev/urandom of="$out/payload" bs=1 count="$PAYLOAD" 2>/dev/null || {
  echo "t-steg.sh: cannot make a payload" >&2; exit 2; }

[ $# -eq 0 ] && set -- "$here/00.ogg"
list=$out/.inputs
: > "$list"
for a in "$@"; do
  if [ -d "$a" ]; then find "$a" -maxdepth 1 -name '*.ogg' | sort >> "$list"
  else printf '%s\n' "$a" >> "$list"
  fi
done
[ -s "$list" ] || { echo "t-steg.sh: no .ogg inputs" >&2; exit 2; }

printf '%-22s %-10s %10s %10s %10s %8s\n' file result ogg tsv meta payload
pass=0 fail=0

while IFS= read -r src; do
  [ -n "$src" ] || continue
  b=$(basename "$src" .ogg)
  w=$out/$b
  why=""

  cp -- "$src" "$w.ogg" || why="copy"
  [ -z "$why" ] && { "$BALROGG" c "$w.ogg" "$w.tsv" || why="balrogg c"; }
  [ -z "$why" ] && { "$TSVSTEG" c "$w.tsv" "$w.wav" "$w.meta" "$out/payload" \
                       || why="tsvsteg c"; }
  [ -z "$why" ] && { "$TSVSTEG" d "$w.wav" "$w.tsr" "$w.meta" "$w.out" \
                       || why="tsvsteg d"; }
  [ -z "$why" ] && { "$BALROGG" d "$w.tsr" "$w.ogr" || why="balrogg d"; }
  [ -z "$why" ] && { cmp -s "$w.tsv" "$w.tsr" || why="tsv differs"; }
  [ -z "$why" ] && { cmp -s "$w.ogg" "$w.ogr" || why="ogg differs"; }
  [ -z "$why" ] && { cmp -s "$out/payload" "$w.out" || why="payload differs"; }

  sz() { [ -f "$1" ] && wc -c < "$1" || echo -; }
  if [ -z "$why" ]; then
    pass=$((pass + 1))
    printf '%-22s %-10s %10s %10s %10s %8s\n' "$b" ok \
           "$(sz "$w.ogg")" "$(sz "$w.tsv")" "$(sz "$w.meta")" "$(sz "$w.out")"
  else
    fail=$((fail + 1))
    printf '%-22s %-10s %s\n' "$b" FAILED "$why"
  fi

  [ -n "${KEEP:-}" ] || rm -f "$w.ogg" "$w.tsv" "$w.wav" "$w.meta" "$w.tsr" \
                              "$w.ogr" "$w.out"
done < "$list"

echo
echo "$pass passed, $fail failed"
[ -n "${KEEP:-}${OUT:-}" ] && echo "scratch: $out"
[ "$fail" -eq 0 ]
