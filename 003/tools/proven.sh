#!/usr/bin/env bash
# proven.sh — which tools' answers depend on the file at all?
#
#   tools/proven.sh            check against ~12 points in the file's history
#   tools/proven.sh 30         check against ~30
#
# `sweep.sh` says every tool reports zero on the file as it stands.  That is the
# claim each round ends on, and on its own it is worth nothing: a broken tool
# reports zero too, and a rule written from a case that has already been fixed
# reports zero whether or not it works.  `undup.py` was written from a case in
# `colour_transform`, reported zero on the file, and reported zero on the
# version from before the fix as well — because it matched only single-line
# `if`s and MSVC's aliasing proofs run to six lines.  It was wrong in exactly
# the way that reads as "nothing left to do".
#
# `shape.py` was worse: it ignored the path it was given and read `subs1.hpp`
# from the working directory, so it answered identically for every revision it
# was asked about.  That is the failure this script is shaped to catch.
#
# So it runs each tool — today's tool — against old revisions of the file and
# asks whether its answer ever changes.  A tool whose last line is the same
# across the whole history either always finds nothing or never looks, and the
# two are indistinguishable from the outside.  That is not proof of a defect;
# some rules were written for a shape already gone.  It is the list of zeros
# that have never been demonstrated to mean anything.
#
# Run against six revisions spanning the project's 329 commits, 43 of 51 tools
# answer differently somewhere.  The eight that do not were each checked by
# hand and every one of their zeros is correct -- REFACTORING9.md section 10
# has the table and the evidence.  That is the outcome this is for: the list is
# short enough to audit, which a list of fifty-one zeros is not.
#
# Nothing in the working tree is touched.
set -u
cd "$(dirname "$0")/.."

n=${1:-12}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

mapfile -t all < <(git log --format=%h --follow -- subs1.hpp)
[ "${#all[@]}" -gt 0 ] || { echo "no history for subs1.hpp" >&2; exit 2; }
step=$(( ${#all[@]} / n + 1 ))
revs=()
for ((k = 0; k < ${#all[@]}; k += step)); do revs+=("${all[k]}"); done
echo "${#revs[@]} revisions of subs1.hpp, spanning ${#all[@]} commits"
echo

ans() { timeout 180 python3 "$1" "$2" 2>&1 | tail -1; }

flat=0
for t in tools/*.py; do
  base=$(basename "$t")
  case $base in structs.py|outpath.py|mk*.py) continue ;; esac
  now=$(ans "$t" subs1.hpp)
  case $now in *'python3 '*) continue ;; esac      # needs more arguments
  moved=''
  for r in "${revs[@]}"; do
    git show "$r:./subs1.hpp" > "$tmp/s.hpp" 2>/dev/null || continue
    if [ "$(ans "$t" "$tmp/s.hpp")" != "$now" ]; then moved=$r; break; fi
  done
  if [ -n "$moved" ]; then
    printf '%-22s answers differently at %s\n' "$base" "$moved"
  else
    printf '%-22s SAME ANSWER THROUGHOUT  %s\n' "$base" "${now:0:44}"
    flat=$((flat + 1))
  fi
done

echo
echo "$flat tools give the same answer for every revision checked"
