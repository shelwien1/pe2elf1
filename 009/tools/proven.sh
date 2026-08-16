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
# **And then this script fell into it.**  It replayed `subs1.hpp`, the single
# file the decompilation arrived as, and the tree became `bmf.cpp` and
# thirty-seven `.inc` files a long time ago.  `git log --follow -- subs1.hpp`
# has been empty ever since, so every run has ended on
#
#     no history for subs1.hpp
#
# and exit 2 -- the meta-gate for a hundred and ten zeros, saying nothing, for
# as many rounds as it took to notice.  It replays the whole directory now:
# `git archive` each revision into a temp tree and run each tool against that
# tree's `bmf.cpp`, which is what the tools splice.
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
# One of those shapes says so itself, and gets its own verdict.  A tool built on
# `tools/buildlog.py` refuses to answer for a file its log was not built from
# and prints the refusal; replayed, it refuses at every revision, and the three
# verdicts here are "answers differently at <rev>", "DECLINES", and "SAME ANSWER
# THROUGHOUT".  Only the last one is the list this script exists to produce.
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

# ~n points spread over the directory's history.  The unit is the directory
# and not a file: a tool splices `bmf.cpp`, so what it reads is every `.inc`
# beside it, and replaying `bmf.cpp` alone would replay a 195-line include list
# against today's sources.
mapfile -t hist < <(git log --format=%h -- .)
[ "${#hist[@]}" -gt 0 ] || { echo "no history for this directory" >&2; exit 2; }
st=$(( ${#hist[@]} / n + 1 ))
revs=()
for ((k = 0; k < ${#hist[@]}; k += st)); do revs+=("${hist[k]}"); done
echo "${#revs[@]} revisions of this directory, spanning ${#hist[@]} commits"
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
mkdir -p "$tmp/now" "$tmp/rev"
git archive HEAD -- . | tar -x -C "$tmp/now"
cp -r tools "$tmp/tools"
# Each revision's whole tree, materialised once and reused by every tool.  A
# revision from before the split has no `bmf.cpp` and is skipped; one from
# before a tool's subject existed simply answers whatever it answers, which is
# the point.
declare -A have
for r in "${revs[@]}"; do
  rm -rf "$tmp/rev/$r"
  mkdir -p "$tmp/rev/$r"
  git archive "$r" -- . 2>/dev/null | tar -x -C "$tmp/rev/$r" 2>/dev/null
  [ -f "$tmp/rev/$r/bmf.cpp" ] && have[$r]=1
done
usable=${#have[@]}
echo "$usable of them have a bmf.cpp to replay"
echo
# The path is normalised out of the answer.  Several tools name the file they
# were given in their summary -- `0 definitions in bmf.cpp that nothing uses`
# -- and against a temp copy that makes every revision's answer unique, so the
# tool reads as "answers differently" at the first revision tried.  That is a
# false pass, and the more dangerous direction: it takes a tool *off* the list
# this script exists to produce.
# And the provenance clauses come off with it.  Four tools now end their
# summary with a parenthesised note saying where their input came from --
# `(warn.log describes another file)`, `(resign-drive.sh: none of 28 reduces
# the count)` -- and those notes appear or vanish with the *working tree*, not
# with the file being replayed.  `explicitcmp.py`, `retype_locals.py` and
# `resign.py` all "answered differently" at the first revision tried for that
# reason alone, which took three tools off this list without a shred of
# evidence that their rules work.
#
# The phrases are listed rather than matched by shape: stripping every trailing
# `(...)` would also strip `unnamed.py`'s "(56 more kept on purpose)", which is
# part of its answer.  A tool that adds a provenance note adds it here, the
# same bargain as `unstale.py`'s HISTORY and `liftframe.py`'s PROVEN.
PROV='s| ([^()]*describes another file)||g; s| ([^()]*is not there)||g; s| (resign-drive\.sh:[^()]*)||g'
ans() { timeout 180 python3 "$1" "$2" 2>&1 | tail -1 \
        | sed "s|$tmp[^ ]*|<file>|g" | sed "$PROV"; }

flat_tools=()
seen=0
for t in "$tmp"/tools/*.py; do
  base=$(basename "$t")
  case $base in structs.py|outpath.py|mk*.py) continue ;; esac
  now=$(ans "$t" "$tmp/now/bmf.cpp")
  case $now in *'python3 '*) continue ;; esac      # needs more arguments
  seen=$((seen + 1))
  moved=''
  for r in "${revs[@]}"; do
    [ -n "${have[$r]:-}" ] || continue
    # And that revision's address map beside it.  `unnamed.py` -- which
    # `shape.py` and so `checktable.py` reach -- rebuilds the map from two `git
    # log -S` searches whenever the one it has does not fit the file it was
    # given, which costs two minutes a revision a tool and three tools do it.
    # The map is committed, so a revision has one; whether it *fits* is another
    # question, and often it does not, because `addrmap.txt` was regenerated by
    # hand and lags the renames.  When it lags, the search runs, which is the
    # right answer and the slow one.
    if [ "$(ans "$t" "$tmp/rev/$r/bmf.cpp")" != "$now" ]; then moved=$r; break; fi
  done
  # A tool that reads a *build log* cannot be proved this way, and it is not
  # flat.  `retype_locals.py` takes its worklist from `strict.log`, which is
  # stamped with the cksum of the source it was built from (tools/buildlog.py);
  # every old revision fails that stamp, so the tool declines and says so in a
  # parenthetical.  That parenthetical is one of the ones `PROV` strips -- for
  # the good reason above, that a note about the working tree must not read as
  # a different answer -- so after the strip the tool looks identical at every
  # revision and lands under SAME ANSWER THROUGHOUT, which reads as "a rule
  # written for a shape already gone".  It is the opposite: the rule was never
  # asked.  The first version of this checked `$now` for the phrase, which
  # cannot ever match: `$now` is the *current* file's answer and the log does
  # describe that one, so the note is not in it and the sed would have taken it
  # out if it were.  It is the replayed answer that carries the declination, so
  # that is what gets looked at -- unstripped, once, and only for a tool that
  # did not move.  Rebuilding each revision to make a log for it is the honest
  # fix and is a different script.
  # The *oldest* revision, not `revs[0]`: `pick` lists history newest first, so
  # `revs[0]` is the current file under another name -- the one revision whose
  # cksum the log does match, and so the one revision that would never make the
  # tool decline.  The check would have reported nothing and read as agreement.
  declines=''
  oldest=''
  for ((k = ${#revs[@]} - 1; k >= 0; k--)); do
    [ -n "${have[${revs[k]}]:-}" ] && { oldest=${revs[k]}; break; }
  done
  if [ -z "$moved" ] && [ -n "$oldest" ]; then
    case $(timeout 180 python3 "$t" "$tmp/rev/$oldest/bmf.cpp" 2>&1 | tail -1) in
      *'describes another file'*|*'is not there'*) declines=1 ;;
    esac
  fi
  if [ -n "$moved" ]; then
    printf '%-22s answers differently at %s\n' "$base" "$moved"
  elif [ -n "$declines" ]; then
    printf '%-22s DECLINES: its worklist is a build log for another revision\n' "$base"
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
  line=$(timeout 300 python3 "$tmp/tools/reads.py" "$t" "$tmp/now/bmf.cpp" 2>&1 | tail -1)
  printf '  %s\n' "$line"
  case $line in *'reads only what it is given') spoken=$((spoken + 1)) ;; esac
done
echo
echo "$spoken of ${#flat_tools[@]} read only the file they were replayed against;"
echo "for the rest the replay could not have moved them, and their zero here is"
echo "this script's reach rather than evidence about the file."
