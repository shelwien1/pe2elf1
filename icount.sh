#!/bin/bash
# icount.sh <inputfile> <level> <bin> ...
#
# Deterministic instruction counts via callgrind.  Wall clock in this container
# has a +-4% noise floor (verified with identical binaries in every slot), which
# is wider than most of the candidate effects, so anything whose mechanism is
# ALU/instruction-count gets measured here instead: callgrind's Ir is exact and
# reproducible to the instruction.
#
# NOT valid for the latency items (prefetch, wxpf, hugepages): valgrind neither
# models the hardware prefetchers nor charges for a cache miss, so a software
# prefetch shows up as pure added instructions with none of its benefit.
set -u
INP=${1:?}; LVL=${2:?}; shift 2
S=${S:-/tmp/claude-0/-home-user-pe2elf1/5e5d7486-4d33-5d0b-9606-ca56f9d55bc1/scratchpad}
ref=""
echo "== $(basename "$INP") L$LVL  callgrind Ir (exact)"
MODE=${MODE:-c}
for b in "$@"; do
  out=$S/cg.$$.$(basename "$b")
  if [ "$MODE" = d ]; then
    valgrind --tool=callgrind --callgrind-out-file=/dev/null --cache-sim=no --branch-sim=no \
             "$b" d "$INP" "$out" >/dev/null 2>$S/cg.$$.err
  else
  valgrind --tool=callgrind --callgrind-out-file=/dev/null --cache-sim=no --branch-sim=no \
           "$b" c "$INP" "$out" "$LVL" >/dev/null 2>$S/cg.$$.err
  fi
  ir=$(grep -oE 'refs:[ ]+[0-9,]+' $S/cg.$$.err | grep -oE '[0-9,]+' | tr -d ,)
  [ -z "$ref" ] && ref=$ir
  printf '   %-24s %16s  %+7.2f%%\n' "$(basename "$b")" "$ir" \
    "$(awk -v a="$ir" -v c="$ref" 'BEGIN{printf "%.2f",(a/c-1)*100}')"
  rm -f "$out" $S/cg.$$.err
done
