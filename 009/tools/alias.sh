#!/bin/sh
# alias.sh -- is anything still read through a pointer of another type?
#
#     tools/alias.sh
#
# `build.sh` sets `-fno-strict-aliasing` because it had to: with gcc's default,
# three of the seventeen reference streams -- t24, t32 and x_ep -- came out
# different.  That was a true measurement of a tree that read objects through
# pointers of another type in about twenty places, which is what MSVC emitted
# for 1997 x86 and what the decompilation kept.
#
# Two checks, because one of them turned out not to be a check at all.
#
# ## the count, which is the real one
#
# `-Wstrict-aliasing=2` on an optimising build names every site.  It reported
# 22, and each was a real one: `(uint16_t*)&counters[...]` where the callee
# wanted a `CounterNode`, `(int32_t*)&grid[i]` cast straight back to
# `FreqRec*` on the next use, `*(const uint16_t*)&img->stride` for a
# truncation, `(uint16_t*)&kids[node]` for a pair of children in a `uint64_t`.
# All 22 are gone.  Zero is the gate.
#
# The warning needs `-O2`: under `-fsyntax-only` the analysis does not run and
# it reports the same number whatever the code says, which is how it first
# looked like it had no opinion.
#
# ## the streams, which are weaker than they look
#
# Building with `-fstrict-aliasing` and comparing the streams is the obvious
# check and it is nearly useless on its own.  Measured: nine `*(uint32_t*)` on
# a `PixRec` were put back and all seventeen streams still matched.  gcc is
# entitled to miscompile that and simply does not, at this optimisation level,
# on this code.  So a pass here means "the compiler is not currently exploiting
# anything", not "there is nothing to exploit" -- which is why the count above
# is the gate and this is the corroboration.
#
# ## what neither claims
#
# That `-fno-strict-aliasing` can be removed.  It is part of the recipe the
# reference streams were taken under, and so is `-O2`.  The claim is only that
# the program no longer depends on it.
set -u
cd "$(dirname "$0")/.."

CXX=${CXX:-g++}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# The optimiser has to run for the analysis to happen, so this is a real
# compile to /dev/null rather than a syntax check.
n=$($CXX -std=gnu++17 -O2 -msse3 -fstrict-aliasing -Wstrict-aliasing=2 \
        -fpermissive -c bmf.cpp -o /dev/null 2>&1 | grep -c 'strict-aliasing')
echo "$n type-punned dereferences reported by -Wstrict-aliasing=2"
$CXX -std=gnu++17 -O2 -msse3 -fstrict-aliasing -Wstrict-aliasing=2 \
     -fpermissive -c bmf.cpp -o /dev/null 2>&1 | grep 'strict-aliasing' \
     | sed 's/^/  /' | sed 's/: warning.*//'

BMF_ALIAS=-fstrict-aliasing ./build.sh || { echo "FAILED: it does not build"; exit 2; }
ok=0 total=0 bad=''
for f in testfiles/*.bmp; do
  b=$(basename "$f" .bmp)
  case $b in out_*) continue ;; esac
  ref=testfiles/ref_$b.bmf
  [ -f "$ref" ] || continue
  total=$((total + 1))
  ./bmf c "$f" "$tmp/$b.bmf" >/dev/null 2>&1
  if cmp -s "$tmp/$b.bmf" "$ref"; then ok=$((ok + 1)); else bad="$bad $b"; fi
done

# Leave the tree with the build everything else expects.
./build.sh >/dev/null 2>&1

echo "$ok/$total streams match when built with -fstrict-aliasing"
[ -n "$bad" ] && echo "  differ:$bad"
if [ "$n" = 0 ] && [ "$ok" = "$total" ] && [ "$total" -gt 0 ]; then
  echo 'PASS: nothing is read through a pointer of another type'
else
  echo 'FAILED: the punning is back'
  exit 1
fi
