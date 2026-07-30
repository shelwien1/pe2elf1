#!/bin/bash
# bench.sh <inputfile> <level> <reps> <binA> <binB> ...
#
# Measurement protocol from paq8hpc_speed.md section 1, which this container
# still needs - it drifts enough that a non-interleaved A/B is meaningless.
#
#   - one discard run per binary first (the first run after a build is never
#     comparable: cold binary/page cache)
#   - then <reps> rounds, each round running every binary once, in order
#   - per round, take each binary's speed as a RATIO to the first binary's speed
#     in that same round, so drift shared by the round cancels
#   - report the median ratio (the headline), the best-of-reps absolute, and the
#     pairwise win count
#
# Median-of-ratios is the number to trust: best-of-N picks up whichever binary
# happened to run during the quietest moment, which is exactly the artifact this
# container produces.
set -u
INP=${1:?}; LVL=${2:?}; REPS=${3:?}; shift 3
BINS=("$@")
S=${S:-/tmp/claude-0/-home-user-pe2elf1/5e5d7486-4d33-5d0b-9606-ca56f9d55bc1/scratchpad}
O=$S/bench.$$.out

run() { "$1" c "$INP" "$O" "$LVL" 2>/dev/null | grep -oE '[0-9.]+ KB/s' | grep -oE '^[0-9.]+'; }

for b in "${BINS[@]}"; do run "$b" >/dev/null; done          # warm, discarded

declare -A best ratios wins
for b in "${BINS[@]}"; do best[$b]=0; ratios[$b]=""; wins[$b]=0; done
for ((r=0;r<REPS;r++)); do
  declare -A cur
  for b in "${BINS[@]}"; do
    v=$(run "$b"); cur[$b]=$v
    awk -v a="$v" -v c="${best[$b]}" 'BEGIN{exit !(a>c)}' && best[$b]=$v
  done
  for b in "${BINS[@]}"; do
    ratios[$b]="${ratios[$b]} $(awk -v a="${cur[$b]}" -v c="${cur[${BINS[0]}]}" 'BEGIN{printf "%.5f",a/c}')"
    awk -v a="${cur[$b]}" -v c="${cur[${BINS[0]}]}" 'BEGIN{exit !(a>c)}' && wins[$b]=$((wins[$b]+1))
  done
done

echo "== $(basename "$INP") L$LVL  n=$REPS interleaved   median-ratio vs $(basename "${BINS[0]}")"
printf '   %-24s %9s %9s %6s\n' binary median best wins
for b in "${BINS[@]}"; do
  med=$(printf '%s\n' ${ratios[$b]} | sort -n | awk '{a[NR]=$1} END{print (NR%2)?a[(NR+1)/2]:(a[NR/2]+a[NR/2+1])/2}')
  printf '   %-24s %+8.1f%% %9s %4d/%d\n' "$(basename "$b")" \
    "$(awk -v m="$med" 'BEGIN{printf "%.1f",(m-1)*100}')" "${best[$b]}" "${wins[$b]}" "$REPS"
done
rm -f "$O"
