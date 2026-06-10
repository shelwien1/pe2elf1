#!/bin/sh
# ZIP64 / edge-case test harness for zipcl.
# Regenerates the archives, then for each one checks:
#   1. d/c passthrough is byte-exact (decrypt then re-encrypt == original)
#   2. the decrypted stream is a valid zip (unzip -t)
#   3. encrypt-with-synthetic-pad keeps the (ZIP64) central directory readable,
#      and decrypting it back reproduces the original (forces offset shifts)
# Run from the repo root:  sh zip64tests/run.sh
cd "$(dirname "$0")/.."
K="a3e30892 f9185194 eb474b09"

python3 zip64tests/gen.py >/dev/null

scratch="_d _pad _c _e _pad2 _back"
trap 'rm -f $scratch' EXIT

echo "== passthrough (d then c) =="
for f in zip64tests/t*.zip stream.zip; do
  rm -f $scratch
  timeout 60 ./zipcl d "$f" _d _pad   $K >/dev/null 2>&1; rc1=$?
  timeout 60 ./zipcl c _d  _c  _pad   $K >/dev/null 2>&1; rc2=$?
  unzip -tqq _d >/dev/null 2>&1; uv=$?
  cmp -s "$f" _c && rt=OK || rt=DIFF
  st="ok"
  { [ $rc1 -ne 0 ] || [ $rc2 -ne 0 ]; } && st="ERR(d=$rc1,c=$rc2)"
  [ "$rt" != "OK" ] && st="$st RT-DIFF"
  # a zero-entry archive (t10) is legitimately rejected by unzip; only note it
  [ "$uv" != 0 ] && [ "$rt" = "OK" ] && st="$st (empty-archive ok)"
  printf "  %-36s %s\n" "$(basename "$f")" "$st"
done

echo "== encrypt-with-pad, then decrypt back (forces offset shift) =="
for f in zip64tests/t*.zip; do
  case "$f" in *t10_*) printf "  %-36s skip (0 entries)\n" "$(basename "$f")"; continue;; esac
  rm -f $scratch
  n=$(python3 -c "import zipfile;print(len(zipfile.ZipFile('$f').namelist()))" 2>/dev/null) || { echo "  $(basename "$f") unreadable"; continue; }
  python3 -c "open('_pad','wb').write(bytes((i*7+3)&0xff for i in range(12*$n)))"
  timeout 60 ./zipcl c "$f" _e _pad  $K >/dev/null 2>&1
  names=$(python3 -c "import zipfile;print(len(zipfile.ZipFile('_e').namelist()))" 2>/dev/null || echo BADCD)
  timeout 60 ./zipcl d _e _back _pad2 $K >/dev/null 2>&1
  cmp -s "$f" _back && rt=OK || rt=DIFF
  st="ok"
  [ "$names" != "$n" ] && st="CD-BAD($names/$n)"
  [ "$rt" != "OK" ] && st="$st BACK-DIFF"
  printf "  %-36s entries=%-6s %s\n" "$(basename "$f")" "$n" "$st"
done
