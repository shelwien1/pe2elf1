#!/usr/bin/env bash
# step.sh <sub_XXXXXXXX>: one iteration of incdec.md §3 for one function.
#   extract -> add to accepted.txt -> build -> gate
# Green: keep it, snapshot the sources (§3 step 7a) and show its probe count.
# Red: take it back out of accepted.txt and rebuild the last green state.
set -u
cd "$(dirname "$0")"
n=$1
python3 tools/extract.py "$n" || exit 1
grep -qx "$n" accepted.txt || echo "$n" >> accepted.txt
if ./build.sh && ./test.sh; then
  tar Jcf backup accepted.txt inc override fixups.txt head.cpp 2>/dev/null
  echo "GREEN $n: probe $(awk -v k="__$n" '$1==k{print $2}' probes.txt)"
  exit 0
fi
sed -i "/^$n\$/d" accepted.txt
./build.sh > /dev/null 2>&1
echo "RED $n (left out of accepted.txt; inc/$n.inc kept for inspection)"
exit 1
