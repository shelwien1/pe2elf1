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
# list; three of these print a census or a map and have no count to be zero, so
# a reader scanning for the word could be forgiven either way.  Those three are
# named below and exempted; every other tool's last line must contain a zero,
# and the sweep exits non-zero if one does not.
set -u
cd "$(dirname "$0")/.."

file=${1:-subs1.hpp}
[ -f "$file" ] || { echo "no such file: $file" >&2; exit 2; }

# Beside the real file, not in /tmp: some tools resolve the repository from the
# path they are given, and a copy under /tmp made `addrmap.py` fail on
# `git rev-parse` rather than report.
work=$(mktemp "$(dirname "$file")/.sweep.XXXXXX")
trap 'rm -f "$work"' EXIT
cp "$file" "$work"
before=$(cksum < "$work")
usage=0 quiet=0 killed=0 nonzero= reported=

for t in tools/*.py; do
  n=$(basename "$t")
  case $n in
    structs.py|outpath.py) continue ;;          # imported, not run
    mk*.py|fuzz.py)        continue ;;          # generators: they write, not read
    sweep_check.py)        continue ;;          # reads the report, not the file
  esac
  # These five have no count that can be zero.  Three answer with a table, a
  # census and an address map.  The fourth, `resign.py`, answers with proposals:
  # whether a type change reduces the conversions or moves them is settled by
  # the compiler and not by anything the tool can see, which is what the
  # ratchet in `build.sh` is for.  Its list is meant to be read and tried, and
  # a non-empty one is not a defect.
  case $n in
    addrmap.py|shape.py|unify_types.py|resign.py|resign_group.py)
        report=1; reported="$reported ${n%.py}" ;;
    *)  report=0 ;;
  esac
  line=$(timeout 300 python3 "$t" "$work" 2>&1 | tail -1)
  rc=$?
  case $line in
    *'python3 '*)  usage=$((usage + 1)); line='(needs more arguments)' ;;
    '')            quiet=$((quiet + 1)); line='(said nothing)' ;;
  esac
  # A tool the timeout killed is not a tool reporting zero, and it can have
  # been killed part-way through a write.  Say which it was.
  [ "$rc" = 124 ] && { killed=$((killed + 1)); line='(timed out)'; }
  if [ "$report" = 0 ] && [ "$rc" != 124 ]; then
    case $line in
      *[!0-9]0[!0-9]*|0[!0-9]*|*[!0-9]0|0) ;;
      'no '*|*'nothing'*|*'(needs more arguments)'*) ;;
      *) nonzero="$nonzero $n" ;;
    esac
  fi
  printf '%-22s %s\n' "$n" "$line"
done

after=$(cksum < "$work")
echo
if [ "$before" != "$after" ]; then
  echo "FAIL: the copy changed during the sweep — a tool wrote to the file it was asked about"
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
[ "$quiet" = 0 ] || {
  echo "FAIL: a tool that prints nothing cannot be told from one that crashed"
  exit 1
}
