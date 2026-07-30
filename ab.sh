#!/bin/bash
# ab.sh <inputfile> <level> <reps> <refbin> <candbin> [candbin...]
#
# Wall-clock A/B hardened against this container's noise, which is worse than
# the doc's section 1 describes: a single binary spans 27.9-37.8 KB/s run to
# run, and five *identical* binaries in five slots reported spreads of +-4% with
# win rates up to 7/9.  Two changes fix that:
#
#   1. an in-block CONTROL - a byte-identical copy of the reference occupying a
#      slot of its own.  Whatever it reports is this block's noise, measured
#      under exactly the conditions the candidates saw.  A candidate counts only
#      if it beats the control.
#   2. the slot order is shuffled every round, so no binary keeps a systematic
#      position (the first slot in a fixed order was measurably penalised).
#
# Reports median of per-round ratios (drift inside a round cancels) and
# best-of-reps (robust to the downward interference tail), plus win counts.
set -u
INP=${1:?}; LVL=${2:?}; REPS=${3:?}; REF=${4:?}; shift 4
S=${S:-/tmp/claude-0/-home-user-pe2elf1/5e5d7486-4d33-5d0b-9606-ca56f9d55bc1/scratchpad}
CTL=$S/p_CONTROL.$$
cp "$REF" "$CTL"
BINS=("$REF" "$@" "$CTL")
O=$S/ab.$$.out

# MODE=d benchmarks decoding instead: <inputfile> is then an archive and <level>
# is ignored (it comes from the archive header).
MODE=${MODE:-c}
if [ "$MODE" = d ]; then
  run() { nice -n -5 "$1" d "$INP" "$O" 2>/dev/null | grep -oE '[0-9.]+ KB/s' | grep -oE '^[0-9.]+'; }
else
  run() { nice -n -5 "$1" c "$INP" "$O" "$LVL" 2>/dev/null | grep -oE '[0-9.]+ KB/s' | grep -oE '^[0-9.]+'; }
fi

for b in "${BINS[@]}"; do run "$b" >/dev/null; done          # warm, discarded

declare -A best ratios wins
for b in "${BINS[@]}"; do best[$b]=0; ratios[$b]=""; wins[$b]=0; done
for ((r=0;r<REPS;r++)); do
  declare -A cur
  for b in $(printf '%s\n' "${BINS[@]}" | shuf); do
    v=$(run "$b"); cur[$b]=$v
    awk -v a="$v" -v c="${best[$b]}" 'BEGIN{exit !(a>c)}' && best[$b]=$v
  done
  for b in "${BINS[@]}"; do
    ratios[$b]="${ratios[$b]} $(awk -v a="${cur[$b]}" -v c="${cur[$REF]}" 'BEGIN{printf "%.5f",a/c}')"
    awk -v a="${cur[$b]}" -v c="${cur[$REF]}" 'BEGIN{exit !(a>c)}' && wins[$b]=$((wins[$b]+1))
  done
done

echo "== $(basename "$INP") L$LVL  n=$REPS shuffled interleave, vs $(basename "$REF")"
printf '   %-24s %9s %9s %8s\n' binary median best wins
for b in "${BINS[@]}"; do
  med=$(printf '%s\n' ${ratios[$b]} | sort -n | awk '{a[NR]=$1} END{print (NR%2)?a[(NR+1)/2]:(a[NR/2]+a[NR/2+1])/2}')
  nm=$(basename "$b"); [ "$b" = "$CTL" ] && nm="[control=ref copy]"
  printf '   %-24s %+8.1f%% %9s %5d/%d\n' "$nm" \
    "$(awk -v m="$med" 'BEGIN{printf "%.1f",(m-1)*100}')" "${best[$b]}" "${wins[$b]}" "$REPS"
done
rm -f "$O" "$CTL"
