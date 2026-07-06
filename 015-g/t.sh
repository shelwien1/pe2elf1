#!/bin/sh
# t.sh — Linux port of t.bat
#
# Round-trip test for stegdict: hide the stegdict binary inside the fp_log
# dictionaries by permuting them (the "c" step), then recover the hidden
# payload back out (the "d" step) and check it against the original binary.
#
#   c @list1 stegdict : range-decode stegdict into dictionary permutations,
#                       writing the permuted dictionaries to fp_mut/
#   d @list2 payload  : range-encode the fp_mut/ permutations back into the
#                       payload, writing the recovered bytes to ./payload
#                       (and the restored dictionaries to fp_unp/)
#
# The dictionaries hold a fixed amount of entropy, so exactly 96294 payload
# bytes fit. The stegdict binary must therefore be larger than that — the
# Makefile links it with -static so it is (~800 KB).

set -e

exe=./stegdict

if [ ! -x "$exe" ]; then
  echo "error: $exe not found — run 'make' first." >&2
  exit 1
fi

# fresh output directories (rmdir /S /Q + md)
rm -rf fp_mut fp_unp
mkdir  fp_mut fp_unp

# list1/list2 use Windows '\' path separators; translate to '/' for Linux.
# (Originals are left untouched; these temporaries are removed at the end.)
sed 's,\\,/,g' list1 > list1.unix
sed 's,\\,/,g' list2 > list2.unix

# c: embed the stegdict binary into the permuted dictionaries -> fp_mut/
"$exe" c @list1.unix stegdict

# d: recover the embedded payload from fp_mut/ -> ./payload (+ fp_unp/)
"$exe" d @list2.unix payload

rm -f list1.unix list2.unix

# Compare (fc /b payload stegdict). The payload is the leading slice of the
# binary that fit into the dictionaries. Its final byte is a partial
# range-coder byte that carries fewer than 8 meaningful bits, so it cannot be
# compared exactly — every byte before it must match the start of the binary.
size=$(wc -c < payload)
echo "payload size = $size bytes (expected 96294)"

if cmp -s -n $((size - 1)) payload stegdict; then
  echo "PASS: first $((size - 1)) bytes of payload match the start of stegdict"
  echo "      (final byte is a partial range-coder byte and is not compared)"
else
  echo "FAIL: payload does not match the start of stegdict" >&2
  cmp payload stegdict || true
  exit 1
fi
