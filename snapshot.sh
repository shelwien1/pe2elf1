#!/bin/sh
# snapshot.sh -- take the best export the running climbers have produced, fold it
# into the parameter sources, rebuild, and measure.
#
#   ./snapshot.sh [workdir] [outdir]
#   ./snapshot.sh --export=FILE [outdir]
#
# workdir defaults to ./opt (the a/ b/ c/ d/ instance dirs live there), outdir to
# ./snap.  With --export the gather step is skipped and that profile is measured
# directly -- which is how a merge_opt.pl hybrid gets checked, since it is not
# any one instance's export and best_exp.pl would never pick it.
#
# Nothing here touches the instances: they own their own xadpcm copy and
# keep climbing while this runs.  The tree's own IDX/ is left alone too -- the
# import happens on a COPY, so a snapshot is a measurement, not a commit.  Fold a
# result into the tree only when it is the one being kept.
set -e
ROOT=`pwd`
EXPORT=""
case "$1" in
  --export=*) EXPORT=${1#--export=}; shift;;
esac
if [ -n "$EXPORT" ]; then
  WORK=""
  OUT=${1:-snap}
  [ -f "$EXPORT" ] || { echo "snapshot: $EXPORT not found" >&2; exit 1; }
  case "$EXPORT" in /*) ;; *) EXPORT="$ROOT/$EXPORT";; esac
else
  WORK=${1:-opt}
  OUT=${2:-snap}
  cd "$WORK"
  # best_exp.pl dies when no instance has an opttimes.!!! yet -- that is the
  # normal state of the first minutes, not an error worth a stack trace.
  if ! perl "$ROOT/best_exp.pl" a b c d; then
    echo "snapshot: no instance has improved on the baseline yet"
    exit 2
  fi
  cd "$ROOT"
  EXPORT="$ROOT/$WORK/export.!!!"
fi

rm -rf "$OUT"
mkdir -p "$OUT"
# the sources, minus the things a build does not read (and minus the 4 MB corpus,
# which is symlinked instead of copied)
cp -r IDX Lib3 MOD *.inc *.cpp mk.sh "$OUT"/
ln -sf "$ROOT/wavs3" "$OUT"/wavs3
cp "$EXPORT" "$OUT"/export.!!!

cd "$OUT"
sh "$ROOT/import.sh" "export.!!!" IDX > import.log 2>&1
CXX=${CXX:-clang++} ARCH=${ARCH:--march=native} sh mk.sh
./xadpcm c wavs3 wavs3.xac
echo "coded: `stat -c%s wavs3.xac` bytes"
# and it must still decode to the input it came from
./xadpcm d wavs3.xac wavs3.out >/dev/null 2>&1
if cmp -s wavs3 wavs3.out; then echo "roundtrip: OK"; else echo "roundtrip: FAILED"; fi
rm -f wavs3.out
