#!/usr/bin/env bash
# Linux equivalent of t.bat: run the two reference files, then compare against
# the log1 captured on Windows.  t.bat only wrote log1; this also checks it.
#
# pjpg now parses markers the original skipped (APP2/ICC, APP13, APP14/Adobe,
# the frame-type line, DNL, EXP, DAC, COM), so its output is a SUPERSET of log1.
# What would be a regression is a line going missing, so that is what is checked.
# For the full regression suite use `make test`.
set -u
cd "$(dirname "$0")"
rm -f log1.new
{ ./pjpg ../testfiles/drazen1.jpg
  ./pjpg ../testfiles/000107_Exif_MM.jpg; } > log1.new 2>&1

diff -a <(tr -d '\r' < log1) log1.new > log1.diff
gone=$(grep -ac '^<' log1.diff || true); gone=${gone:-0}
add=$(grep -ac  '^>' log1.diff || true); add=${add:-0}

if [ "$gone" -eq 0 ]; then
  echo "OK: no line lost vs the Windows reference log1 ($add line(s) added by newer marker handlers)"
  [ "$add" -gt 0 ] && { echo "added:"; grep -a '^>' log1.diff | sed 's/^> /  /'; }
  exit 0
else
  echo "REGRESSION: $gone line(s) the Windows build printed are missing:"
  grep -a '^<' log1.diff | sed 's/^< /  /'
  exit 1
fi
