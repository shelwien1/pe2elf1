#!/usr/bin/env bash
# Resume the encoder g++ compile at -O0 -j2 with progress.
# Verilator elaboration already produced obj_xp10enc/*.cpp (1803 files);
# this script just drives g++.

set -uo pipefail
cd "$(dirname "$0")/obj_xp10enc"

LOG=/tmp/enc-build.log
PROG=/tmp/enc-progress.log
TOTAL=$(ls *.cpp | wc -l)

# Strip the existing -O2 in Vcr_cceip_64.mk has been done.
# Force OPT_FAST/SLOW/GLOBAL to -O0 too; -j 2 to stay under ~6-8 GB peak.

# Progress watcher: every 15s log "compiled X/Y, RAM=..., latest=..."
(
  while sleep 15; do
    done_n=$(ls *.o 2>/dev/null | wc -l)
    mem=$(free -m | awk '/^Mem:/ {printf "used=%dMi free=%dMi", $3, $7}')
    latest=$(ls -t *.o 2>/dev/null | head -1)
    echo "[$(date +%T)] $done_n/$TOTAL  $mem  latest=$latest" >> "$PROG"
  done
) &
WATCHER_PID=$!
trap "kill $WATCHER_PID 2>/dev/null" EXIT

echo "[$(date +%T)] BUILD START: $TOTAL .cpp files at -O0 -j2" | tee -a "$PROG"

make -f Vcr_cceip_64.mk -j 2 \
     OPT_FAST=-O0 OPT_SLOW=-O0 OPT_GLOBAL=-O0 \
     xp10-enc < /dev/null > "$LOG" 2>&1
RC=$?

done_n=$(ls *.o 2>/dev/null | wc -l)
if [[ $RC -eq 0 && -x xp10-enc ]]; then
  echo "[$(date +%T)] BUILD OK: compiled $done_n .o files; xp10-enc=$(stat -c%s xp10-enc) bytes" | tee -a "$PROG"
  cp xp10-enc ../xp10-enc
  exit 0
else
  echo "[$(date +%T)] BUILD FAIL: rc=$RC, compiled $done_n/$TOTAL .o; last lines of $LOG:" | tee -a "$PROG"
  tail -20 "$LOG" | tee -a "$PROG"
  exit "$RC"
fi
