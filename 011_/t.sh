#!/usr/bin/env bash
# Linux equivalent of t.bat: run the two reference files, then diff against the
# log1 captured on Windows.  t.bat only wrote log1; this also checks it.
set -u
cd "$(dirname "$0")"
rm -f log1.new
{ ./pjpg ../testfiles/drazen1.jpg
  ./pjpg ../testfiles/000107_Exif_MM.jpg; } > log1.new 2>&1
if diff -u <(tr -d '\r' < log1) log1.new; then
  echo "OK: output matches the Windows reference log1"
else
  echo "MISMATCH vs log1"; exit 1
fi
