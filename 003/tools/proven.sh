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
# "Never looks" has more than one shape, and this script cannot see any of them
# on its own: a tool that ignores the path, or joins it against `warn.log` or a
# fresh compile of the working tree, answers identically for every revision and
# lands in the flat list looking like a rule written for a shape already gone.
# So each flat row is handed to `tools/reads.py`, which runs the tool with
# `open` and `subprocess` spied and says what its input actually was.  A row
# that says anything but "reads only what it is given" is this script's reach,
# not a finding about the file.
#
# The counts this used to quote -- "43 of 51 tools", "six revisions", "329
# commits" -- were true when they were written and are printed by the run now,
# because a claim in a comment is a measurement that has to be re-taken.
#
# Nothing in the working tree is touched -- which used to be a claim and is now
# a copy: see the note above `ans`.
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

# Every run goes through a copy, and every copy is named `subs1.hpp`.
#
#   * the working tree is what this script promises not to touch, and `now` was
#     being taken by running each tool against the real file -- a tool that
#     rewrites what it finds would have rewritten the decompilation to measure
#     it;
#   * the name matters as much as the contents.  `decast.py` and `uncast.py`
#     keep the compiler warnings whose file matches the path's basename, so a
#     replay through `$tmp/s.hpp` matched nothing and answered "no useless
#     casts" for every revision including the current one.  Same basename, same
#     question.
#
# The snapshot is taken once, at the start.  A run takes ten minutes or more,
# and re-reading the working file per tool means an edit made while it runs
# lands in the middle of the answers -- half the tools compared against one
# file and half against another, with nothing in the output to say so.
#
# `tools/` is snapshotted with it.  The file was pinned and the *tools* were
# not, so a run that took ten minutes read each tool off disk as it reached it:
# the answers at the top came from one version of the directory and the ones at
# the bottom from another.  It showed: `explicitcmp.py` was reported as reading
# `warn.log, cksum` in a run that started before `cksum` was in it.
mkdir -p "$tmp/now" "$tmp/rev" "$tmp/base"
cp subs1.hpp "$tmp/base/subs1.hpp"
cp -r tools "$tmp/tools"
ans() { timeout 180 python3 "$1" "$2" 2>&1 | tail -1; }

flat_tools=()
seen=0
for t in "$tmp"/tools/*.py; do
  base=$(basename "$t")
  case $base in structs.py|outpath.py|mk*.py) continue ;; esac
  cp "$tmp/base/subs1.hpp" "$tmp/now/subs1.hpp"
  now=$(ans "$t" "$tmp/now/subs1.hpp")
  case $now in *'python3 '*) continue ;; esac      # needs more arguments
  seen=$((seen + 1))
  moved=''
  for r in "${revs[@]}"; do
    git show "$r:./subs1.hpp" > "$tmp/rev/subs1.hpp" 2>/dev/null || continue
    if [ "$(ans "$t" "$tmp/rev/subs1.hpp")" != "$now" ]; then moved=$r; break; fi
  done
  if [ -n "$moved" ]; then
    printf '%-22s answers differently at %s\n' "$base" "$moved"
  else
    printf '%-22s SAME ANSWER THROUGHOUT  %s\n' "$base" "${now:0:44}"
    flat_tools+=("$t")
  fi
done

echo
echo "${#flat_tools[@]} of $seen tools give the same answer for every revision checked."
echo "What each of those actually reads, from tools/reads.py:"
echo
spoken=0
for t in "${flat_tools[@]}"; do
  line=$(timeout 300 python3 "$tmp/tools/reads.py" "$t" "$tmp/base/subs1.hpp" 2>&1 | tail -1)
  printf '  %s\n' "$line"
  case $line in *'reads only what it is given') spoken=$((spoken + 1)) ;; esac
done
echo
echo "$spoken of ${#flat_tools[@]} read only the file they were replayed against;"
echo "for the rest the replay could not have moved them, and their zero here is"
echo "this script's reach rather than evidence about the file."
