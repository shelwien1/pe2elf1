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
# So this does two things the claim needs.  It runs them all, and it checks the
# file afterwards: a read-only sweep that changes the file it is measuring has
# measured nothing, and that is a failure, not a footnote.
#
# Tools that need more arguments print their usage line and are listed as such
# rather than counted -- they are not reporting zero, they are declining to
# guess, and the difference matters when the point of the exercise is to know
# which zeros are real.
set -u
cd "$(dirname "$0")/.."

file=${1:-subs1.hpp}
[ -f "$file" ] || { echo "no such file: $file" >&2; exit 2; }

before=$(cksum < "$file")
usage=0 quiet=0

for t in tools/*.py; do
  n=$(basename "$t")
  case $n in
    structs.py|outpath.py) continue ;;          # imported, not run
    mk*.py)                continue ;;          # generators: they write, not read
  esac
  line=$(timeout 300 python3 "$t" "$file" 2>&1 | tail -1)
  case $line in
    *'python3 '*)  usage=$((usage + 1)); line='(needs more arguments)' ;;
    '')            quiet=$((quiet + 1)); line='(said nothing)' ;;
  esac
  printf '%-22s %s\n' "$n" "$line"
done

after=$(cksum < "$file")
echo
if [ "$before" != "$after" ]; then
  echo "FAIL: $file changed during the sweep — a tool wrote to the file it was asked about"
  exit 1
fi
echo "$file unchanged by the sweep; $usage tools want more arguments, $quiet said nothing"
[ "$quiet" = 0 ] || {
  echo "FAIL: a tool that prints nothing cannot be told from one that crashed"
  exit 1
}
