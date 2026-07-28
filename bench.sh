#!/bin/bash
# bench.sh [binary] [reps] -- min-of-N user time for the four hot paths.
#
# perf is not available in this container, so this reports user CPU seconds from
# bash's own `time`, taking the MINIMUM of N runs.  Minimum rather than mean
# because the noise here is all additive (other tenants on the box), so the
# fastest run is the closest estimate of the true cost; a mean would mostly
# measure the neighbours.  N=5 gives a spread well under 2% on this machine.
#
# bash, not sh: TIMEFORMAT is a bashism and dash's `time` writes a format this
# cannot parse.
#
# Encode and decode are both measured: they share the model but not the RC path,
# and several optimizations touch one more than the other.
X=${1:-./xadpcm}
N=${2:-5}
[ -x "$X" ] || { echo "no such binary: $X" >&2; exit 1; }

$X c wavs2 _b.ms.xac >/dev/null 2>&1
$X c Player_Death_Music_ima.wav _b.ima.xac >/dev/null 2>&1

total=0
run() {
  local lab=$1; shift
  local best= t i
  for ((i=0;i<N;i++)); do
    t=$( { TIMEFORMAT='%3U'; time "$@" >/dev/null 2>/dev/null; } 2>&1 )
    if [ -z "$best" ] || (( $(echo "$t < $best" | bc -l) )); then best=$t; fi
  done
  printf "  %-22s %7s s\n" "$lab" "$best"
  total=$(echo "$total + $best" | bc -l)
}

echo "$X  (min of $N)"
run "encode MS  (wavs2)"   $X c wavs2 /dev/null
run "encode IMA (pdm)"     $X c Player_Death_Music_ima.wav /dev/null
run "decode MS"            $X d _b.ms.xac /dev/null
run "decode IMA"           $X d _b.ima.xac /dev/null
printf "  %-22s %7.3f s\n" "TOTAL" "$total"
rm -f _b.ms.xac _b.ima.xac
