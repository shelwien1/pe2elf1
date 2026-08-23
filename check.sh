#!/bin/sh
# check.sh -- the release-and-tuning contract, in one command.
#
# Both builds must code every image identically.  Run this rather than a bare
# ./mk.sh && verify: a build that fails to compile leaves the previous binary
# (or none), and a verify script pointed at a missing file reports an empty
# manifest, which reads like a mismatch or, worse, like a pass.
set -e
S=/tmp/claude-0/-home-user-pe2elf1/319c4029-94eb-5d39-9c49-e09a1ca282a4/scratchpad
for mode in release tuning; do
  rm -f bmf
  if [ "$mode" = release ]; then ./mk.sh release >/dev/null; else ./mk.sh >/dev/null; fi
  [ -x ./bmf ] || { echo "$mode: BUILD FAILED"; exit 1; }
  n=$(grep -ao '!MAP!' bmf | wc -l)
  printf '%-8s %4s live parameters  %s\n' "$mode" "$n" "$(sh $S/verify.sh ./bmf 2>&1 | tail -1)"
done
