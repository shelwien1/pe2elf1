#!/usr/bin/env bash
# xcheck.sh: the commands the gate does not run, original port vs moved build.
#
# The gate (test.sh) covers plain `u` and `r`.  main (sub_10002160) is moved
# too, and it dispatches every command, so its other branches -- the usage
# text, `l`, `b`/`B`, several files, a `*.*` mask recursing into
# subdirectories -- now run decompiled code.  For each sequence below, run it
# with ../vbn and with vbn.elf+dummy.so in identical directory trees and
# compare stdout (Time line masked), exit codes and every file left behind.
# Not a gate: these paths also run original code that was never moved.
set -u -f
cd "$(dirname "$0")"
export TZ=UTC
STAMP='1980-01-01 00:13'
[ -f corpus/mixed.bin ] || python3 tools/mkcorpus.py corpus
for f in vbn.elf libvbnshim.so dummy.so; do [ -f "$f" ] || { echo "missing $f (run build.sh)"; exit 1; }; done
make -s -C .. vbn libvbnshim.so || exit 1

W=$(mktemp -d ./work.XXXXXX); W=$(cd "$W" && pwd)
trap 'rm -rf "$W"' EXIT

setup() {   # setup <dir> orig|moved: a tree to work in, binaries beside it in <dir>.bin
  mkdir -p "$1/sub/deep" "$1.bin"
  if [ "$2" = orig ]; then cp ../vbn ../libvbnshim.so "$1.bin/"; cp ../vbn "$1.bin/X"
  else cp vbn.elf libvbnshim.so dummy.so "$1.bin/"; cp vbn.elf "$1.bin/X"; fi
  cp ../input.txt "$1/a.txt"; cp corpus/rep.bin "$1/b.bin"
  head -c 30000 corpus/mixed.bin > "$1/sub/c.dat"; head -c 9000 ../vbn.exe > "$1/sub/deep/d.exe"
  find "$1" -type f -exec touch -d "$STAMP" {} +
}
run() {     # run <dir> <args...>
  (cd "$1" && VBN_PROBES=0 timeout 300 "$1.bin/X" "${@:2}" < /dev/null 2>&1 \
     | tr '\r' '\n' | sed -E 's/Time = [0-9]+ sec\./Time = N sec./'; echo "exit=${PIPESTATUS[0]}")
}
SEQS=(
  "usage:"
  "list:u arc a.txt|l arc"
  "normal:b arc a.txt|r arc"
  "fast:B arc a.txt|r arc"
  "multi:u arc a.txt b.bin|l arc|r arc"
  "recurse:b arc *.*|l arc"
  "files-only:bf arc *.*|l arc"
)
fail=0
for s in "${SEQS[@]}"; do
  name=${s%%:*}; IFS='|' read -ra cmds <<< "${s#*:}"
  for e in orig moved; do
    d="$W/$name.$e"; setup "$d" $e
    { [ ${#cmds[@]} = 0 ] && run "$d"; for c in "${cmds[@]}"; do echo "## $c"; run "$d" $c; done; } > "$d.out"
    (cd "$d" && find . -type f -exec md5sum {} + | sort -k2) > "$d.files"
  done
  if cmp -s "$W/$name.orig.out" "$W/$name.moved.out" && cmp -s "$W/$name.orig.files" "$W/$name.moved.files"; then
    echo "same  $name ($(grep -c . "$W/$name.orig.out") lines of output, $(wc -l < "$W/$name.orig.files") files)"
  else
    echo "DIFF  $name"; fail=1
    diff "$W/$name.orig.out" "$W/$name.moved.out" | head -10
    diff "$W/$name.orig.files" "$W/$name.moved.files" | head -10
  fi
done
[ $fail = 0 ] && echo SAME || echo DIFFERENT
exit $fail
