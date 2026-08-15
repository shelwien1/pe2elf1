#!/usr/bin/env bash
# sweep.sh — ask every tool in this directory what it still finds.
#
#   tools/sweep.sh              report on subs1.hpp
#   tools/sweep.sh other.hpp    report on another file
#
# Each round of this project ends with the claim "every tool reports zero", and
# for a long time that claim was worth very little.  Some tools printed nothing
# at all when they found nothing, which reads exactly like a tool that died;
# five printed a traceback; one rewrote the file just for being run; and three
# were generators that took the argument as the file to *write*, so asking the
# whole directory a question left the decompilation as a 96x96 bitmap.
#
# So this does two things the claim needs.  It runs them all against a *copy*,
# and it checks the copy afterwards: a read-only sweep that changes the file it
# is measuring has measured nothing, and that is a failure, not a footnote.
#
# The copy is why the real file is never at risk.  An earlier version pointed
# every tool at `subs1.hpp` itself and reported the change afterwards, which
# leaves the window between "a tool wrote" and "the sweep says so" -- and a
# tool killed by the timeout below can leave a truncated file inside it.
# Nothing here can now touch the decompilation.
#
# Tools that need more arguments print their usage line and are listed as such
# rather than counted -- they are not reporting zero, they are declining to
# guess, and the difference matters when the point of the exercise is to know
# which zeros are real.
#
# And the claim itself is checked rather than read.  Every round ends "all of
# them report zero", which for a long time meant someone had looked down the
# list; six of these print a census, a map or a list of proposals and have no
# count to be zero, so a reader scanning for the word could be forgiven either
# way.  Those six are named below and exempted; every other tool's last line
# must contain a zero, and the sweep exits non-zero if one does not.
#
# A third answer joined the two above when the decompilation moved to its own
# repository: three of these check something that is not here at all -- the
# previous round's document, the blob globals a round removed, the first commit
# of the Hex-Rays output.  Two of them died on it and one said so in prose, and
# the sweep read all three as findings.  A question whose subject is absent is
# neither a finding nor a zero, so a tool says `not applicable: <what is
# missing>` and is counted apart, the same way "needs more arguments" already
# is.  What must not happen is the tempting fix: answering zero.
set -u
cd "$(dirname "$0")/.."

file=${1:-subs1.hpp}
[ -f "$file" ] || { echo "no such file: $file" >&2; exit 2; }

# Beside the real file, not in /tmp: some tools resolve the repository from the
# path they are given, and a copy under /tmp made `addrmap.py` fail on
# `git rev-parse` rather than report.
work=$(mktemp "$(dirname "$file")/.sweep.XXXXXX")
trap 'rm -f "$work"' EXIT

# The copy is the unit, not the file.  `bmf.cpp` is 195 lines of `#include`
# with two bodies in it, and 77 of the 88 tools below read the path they are
# given and nothing else -- so for those 77 the sentence at the end of this
# script, "every counting tool reports zero", was a statement about 195 lines
# of include list.  Eleven tools splice for themselves, and each of those was
# taught to only after it reported something impossible.
#
# Splicing here fixes the other 77 in one place instead of 77.  A tool that
# splices for itself is unaffected: `structs.splice` looks for `#include "x"`
# lines and this copy has none left, so it hands the file back unchanged.
# Line numbers in a report are the spliced file's, which is the cost; the
# question the sweep asks is whether a tool still finds something, and a
# finding whose line number needs translating is still a finding.
python3 - "$file" > "$work" <<'PY' || { echo "could not splice $file" >&2; exit 2; }
import sys
sys.path.insert(0, 'tools')
import structs
sys.stdout.write('\n'.join(structs.splice(sys.argv[1])[0]))
PY
before=$(cksum < "$work")
usage=0 quiet=0 killed=0 absent=0 nonzero= reported= inapplicable= wrote=

for t in tools/*.py; do
  n=$(basename "$t")
  case $n in
    structs.py|outpath.py) continue ;;          # imported, not run
    mk*.py|fuzz.py)        continue ;;          # generators: they write, not read
    split.py)              continue ;;          # a migration: it writes the tree
    sweep_check.py)        continue ;;          # reads the report, not the file
  esac
  # These six have no count that can be zero.  Three answer with a table, a
  # census and an address map.  The fourth, `resign.py`, answers with proposals:
  # whether a type change reduces the conversions or moves them is settled by
  # the compiler and not by anything the tool can see, which is what the
  # ratchet in `build.sh` is for.  Its list is meant to be read and tried, and
  # a non-empty one is not a defect.
  case $n in
    addrmap.py|shape.py|unify_types.py|resign.py|resign_group.py|uncastwidth.py)
        report=1; reported="$reported ${n%.py}" ;;
    *)  report=0 ;;
  esac
  # `rc=$?` after a pipeline is `tail`'s status, not the tool's, and `tail`
  # succeeds on a truncated stream as readily as on a complete one.  So the
  # timeout check below could not fire: `addrmap.py` was killed at 300 seconds
  # mid-run and the sweep read its last surviving line as an answer.  A tool
  # this kills has to be seen to have been killed, or the timeout is a way of
  # turning a slow tool into a quiet one.
  line=$(timeout 300 python3 "$t" "$work" 2>&1 | tail -1; exit "${PIPESTATUS[0]}")
  rc=$?
  case $line in
    *'python3 '*)      usage=$((usage + 1)); line='(needs more arguments)' ;;
    'not applicable:'*) absent=$((absent + 1))
                       inapplicable="$inapplicable ${n%.py}" ;;
    '')                quiet=$((quiet + 1)); line='(said nothing)' ;;
  esac
  # A tool the timeout killed is not a tool reporting zero, and it can have
  # been killed part-way through a write.  Say which it was.
  [ "$rc" = 124 ] && { killed=$((killed + 1)); line='(timed out)'; }
  if [ "$report" = 0 ] && [ "$rc" != 124 ]; then
    case $line in
      *[!0-9]0[!0-9]*|0[!0-9]*|*[!0-9]0|0) ;;
      'no '*|*'nothing'*|*'(needs more arguments)'*|'not applicable:'*) ;;
      *) nonzero="$nonzero $n" ;;
    esac
  fi
  printf '%-22s %s\n' "$n" "$line"
  # Which tool wrote, not just that one did.  The check at the end has always
  # been able to say the copy changed and never which of the eighty-eight did
  # it, so the answer was a bisection by hand every time.  It costs one cksum
  # per tool.
  now=$(cksum < "$work")
  if [ "$now" != "$before" ]; then
    wrote="$wrote $n"
    before=$now
  fi
done

echo
if [ -n "$wrote" ]; then
  echo "FAIL: these tools wrote to the file they were asked about:$wrote"
  exit 1
fi
[ "$killed" = 0 ] || { echo "FAIL: $killed tools were killed by the timeout"; exit 1; }
[ -z "$nonzero" ] || {
  echo "FAIL: these tools still find something:$nonzero"
  exit 1
}
echo "$file unchanged by the sweep; $usage tools want more arguments, $quiet said nothing"
# Named from the `case` above rather than restated here.  A sentence that
# lists the exemptions in prose is a claim that ages the moment the list
# changes -- which is the defect this round found in seventeen frame comments,
# and there is no reason for the sweep's own summary to have it.
echo "every counting tool reports zero; these report rather than count:$reported"
[ "$absent" = 0 ] ||
  echo "$absent asked about something this repository does not have:$inapplicable"
[ "$quiet" = 0 ] || {
  echo "FAIL: a tool that prints nothing cannot be told from one that crashed"
  exit 1
}
