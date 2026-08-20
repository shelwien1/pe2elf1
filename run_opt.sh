#!/bin/sh
# run_opt.sh -- linux port of 022d's run.bat.
#
# run.bat made four copies of the 0/ template and handed each to cons.exe, which
# opened four console windows running 1.bat -- an endless `perl auto_opt2.pl`
# loop.  Here each instance is one detached `perl auto_opt2.pl`, run ONCE: the
# climber's own outer loop already repeats until a whole pass over all 133
# descriptors finds nothing, and then it prints "Done" and quits.  Restarting it
# from a wrapper is what 1.bat added, and it is not wanted here.
#
# Four instances, one corpus, no shared state: each owns a private xadpcm copy
# (the climber patches the binary in place) and a private wavs3, and they differ
# only by perl's per-process rand seed -- which is the whole search, since every
# map starts from a random 25%-flipped pattern and the bit order inside a pass is
# a random permutation.  They are gathered by best_exp.pl, not by talking.
set -e
ROOT=`pwd`
WORK=${1:-opt}
rm -rf "$WORK"
mkdir -p "$WORK/0"
cp xadpcm auto_opt2.pl wavs3 "$WORK/0"/
for d in a b c d; do
  cp -r "$WORK/0" "$WORK/$d"
done
for d in a b c d; do
  ( cd "$WORK/$d" && setsid nohup perl auto_opt2.pl > opt.log 2>&1 < /dev/null & )
done
sleep 2
echo "started:"
ps -eo pid,etime,args | grep '[a]uto_opt2.pl'
