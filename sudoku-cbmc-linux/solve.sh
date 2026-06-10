#!/bin/sh
# Linux port of solve.bat
#
# Original (Windows) pipeline:
#   .\cbmc64.exe sudoku.cpp --error-label match --trace \
#       | .\grep .stream | .\tail --lines=1 | .\grep -oP "={ \d[^}]*}" >solution.inc
#   del sudoku_chk.exe
#   call .\cl.bat sudoku_chk.cpp
#   sudoku_chk.exe
#
# Linux equivalents:
#   cbmc64.exe -> cbmc   (apt install cbmc)
#   grep/tail  -> system grep/tail
#   cl.bat     -> g++
#
# Trace-format note:
#   CBMC 5.7 (the bundled cbmc64.exe) printed the whole nondeterministic
#   `stream` array as a single `stream={ 55, 51, ... }` line, which the old
#   grep/tail/grep pipeline scraped. CBMC 5.95 no longer does this for a
#   read-only `extern` array (only the `stream[81]=1` flag write shows up).
#   The 81 solved cell values are instead recovered from the row-major reads
#   `c = x.tbl[i*N+j]` (sudoku.cpp line 51) which CBMC emits once per cell, in
#   order. They are reassembled into the same `={ ..., 1 }` initializer the
#   original produced. If sudoku.cpp is edited, update the line number below.

set -e

cbmc sudoku.cpp --error-label match --trace \
  | grep -A2 'line 51 thread' \
  | grep -oP 'c=\K[0-9]+' \
  | paste -sd, \
  | sed 's/,/, /g; s/.*/={ &, 1 }/' > solution.inc

rm -f sudoku_chk
g++ sudoku_chk.cpp -o sudoku_chk

./sudoku_chk
