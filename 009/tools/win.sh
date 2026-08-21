#!/usr/bin/env bash
# win.sh — build this tree for Windows and run it, on a Linux host.
#
#     tools/win.sh              # everything below
#     tools/win.sh --build      # cross-compile only; no wine, no runs
#
# **Why this exists.**  The program grew two in-memory entry points, and they
# are the one place it reached for something Windows does not have:
# `open_memstream` and `fmemopen`.  Neither is in MSVC's CRT and neither is in
# mingw-w64.  That was reported from outside rather than found here, because
# nothing in this tree had ever pointed a Windows compiler at it — the gate was
# `g++` and `g++` is happy.
#
# `platform.inc` is the shim, and a shim nobody compiles is a guess.  So this
# does four things, in order of how much they prove:
#
#   1. **it compiles and links** as a PE.  This alone would have caught the
#      original report: the failure was two undeclared names, nothing subtler.
#   2. **the same seventeen streams, byte for byte.**  `bmf c` on Windows
#      against `testfiles/ref_*.bmf`, which is `test.sh`'s own gate run one
#      platform over, and `bmf d` against what the native build decodes.
#   3. **the shims themselves.**  Neither `bmf c` nor `bmf d` touches them —
#      both open a *path* — so steps 1 and 2 can pass with the memory layer
#      completely broken.  `tools/parallel.cpp` is what goes through
#      `compress_to_memory` and `expand_from_memory`, so it is the only thing
#      here that executes a line of the Windows half of `platform.inc`.
#   4. **nothing is left behind.**  The Windows shim is a temporary file, and a
#      temporary file that outlives the process is a bug that no byte
#      comparison can see.  The count before and after has to match.
#
# **`-static` on the test binaries and not on the program.**  `parallel.cpp`
# pulls in libstdc++ for `std::vector` and `std::thread`; under wine that meant
# `libstdc++-6.dll not found` and an exit code with no output, which reads
# exactly like a crash in the code under test.  It was neither.  The program
# itself links clean either way.
#
# Absent tools are "not applicable" and not a failure: this is a Linux-host
# convenience for a Windows target, and a machine without the cross-compiler
# should say so rather than fail a sweep.
set -u
cd "$(dirname "$0")/.."

CXXWIN=${CXXWIN:-x86_64-w64-mingw32-g++}
WINE=${WINE:-wine}
build_only=0
[ "${1:-}" = --build ] && build_only=1

if ! command -v "$CXXWIN" >/dev/null 2>&1; then
  echo "not applicable: no $CXXWIN here.  apt-get install mingw-w64"
  exit 0
fi

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
fails=0
say() { printf '%-26s %s\n' "$1" "$2"; }
bad() { say "$1" "$2"; fails=$((fails + 1)); }

# 1 -- it compiles and links.
if ! "$CXXWIN" -std=c++17 -O2 -I. -o "$tmp/bmf.exe" bmf.cpp >"$tmp/cc.log" 2>&1; then
  bad 'cross-compile' "$(grep -m1 'error:' "$tmp/cc.log" || echo 'see the log')"
  grep 'error:' "$tmp/cc.log" | head -10 >&2
  exit 1
fi
say 'cross-compile' "$(basename "$tmp/bmf.exe") built, $(stat -c%s "$tmp/bmf.exe") bytes"

# The memory entry points live here and nowhere else, so this binary is the
# only one that proves the shims.  `-static` for the reason in the header.
if ! "$CXXWIN" -std=c++17 -w -O2 -I. -static -o "$tmp/par.exe" \
        tools/parallel.cpp >"$tmp/par.log" 2>&1; then
  bad 'cross-compile parallel' "$(grep -m1 'error:' "$tmp/par.log" || echo 'see the log')"
fi

if [ "$build_only" = 1 ]; then
  say 'skipped' 'the runs; drop --build for those'
  [ "$fails" = 0 ] || exit 1
  echo 'the tree cross-compiles for Windows'
  exit 0
fi

if ! command -v "$WINE" >/dev/null 2>&1; then
  say 'not applicable' "no $WINE here, so nothing was run.  apt-get install wine64"
  [ "$fails" = 0 ] || exit 1
  echo 'the tree cross-compiles for Windows; it was not run'
  exit 0
fi

export WINEPREFIX="$tmp/wp" WINEDEBUG=-all
"$WINE" wineboot -i >/dev/null 2>&1

# 2 -- the same streams, byte for byte, both directions.
n=0 differ=0
for f in testfiles/*.bmp; do
  b=$(basename "$f" .bmp)
  ref=testfiles/ref_$b.bmf
  [ -f "$ref" ] || continue
  n=$((n + 1))
  "$WINE" "$tmp/bmf.exe" c "$f" "$tmp/$b.bmf" >/dev/null 2>&1
  cmp -s "$tmp/$b.bmf" "$ref" || { echo "  compress differs: $b" >&2; differ=$((differ + 1)); }
done
[ "$differ" = 0 ] && say 'compress' "$n streams byte-identical to testfiles/ref_*.bmf" \
                  || bad 'compress' "$differ of $n streams differ"

# Against the native build's output rather than against a checked-in file,
# because what is being asked is whether the two platforms agree.
n=0 differ=0
for ref in testfiles/ref_*.bmf; do
  b=$(basename "$ref" .bmf)
  b=${b#ref_}
  [ -f "testfiles/$b.bmp" ] || continue
  n=$((n + 1))
  "$WINE" "$tmp/bmf.exe" d "$ref" "$tmp/$b.win.bmp" >/dev/null 2>&1
  ./bmf d "$ref" "$tmp/$b.native.bmp" >/dev/null 2>&1
  cmp -s "$tmp/$b.win.bmp" "$tmp/$b.native.bmp" \
    || { echo "  expand differs: $b" >&2; differ=$((differ + 1)); }
done
[ "$differ" = 0 ] && say 'expand' "$n images match the native build" \
                  || bad 'expand' "$differ of $n images differ"

# 3 and 4 -- the shims, and what they leave behind.
wtemp="$WINEPREFIX/drive_c/users/$(whoami)/Temp"
before=$(ls -1 "$wtemp" 2>/dev/null | wc -l)
if [ -x "$tmp/par.exe" ]; then
  out=$(timeout 900 "$WINE" "$tmp/par.exe" 2>&1)
  rc=$?
  case $out in
    PASS*) say 'in-memory entry points' "${out%% (*}" ;;
    *)     bad 'in-memory entry points' "exit $rc -- $(printf '%s\n' "$out" | tail -1)" ;;
  esac
  after=$(ls -1 "$wtemp" 2>/dev/null | wc -l)
  [ "$before" = "$after" ] && say 'temporary files' "none left behind ($after in Temp, as before)" \
                           || bad 'temporary files' "$before before, $after after -- the shim leaked one"
fi

[ "$fails" = 0 ] || { echo "FAILED: $fails of the Windows checks did not answer what they should" >&2; exit 1; }
echo 'the tree builds and runs for Windows, and answers what the native build answers'
