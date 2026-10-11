#!/usr/bin/env bash
# xcheck.sh: `u` and `r` beyond the gate, vbn (the port) against vbn_ur
# (tools/mkvbn2.py's standalone build of the moved functions).
#
# test.sh --exe vbn2/vbn_ur is the gate; this runs sequences it does not: several
# files and masks, a mask in a subdirectory, names in another case, restoring
# over existing files (vbn asks; stdin is empty), an empty file, a damaged
# archive, restoring into another directory.  Each sequence runs with ../vbn
# and with vbn_ur in identical trees;
# stdout (the Time line masked), the exit codes and every file left behind
# (contents, and mtimes but the archive's) have to match.  `!cmd` is a shell
# command run in the tree between the two programs' commands.
set -u -f
cd "$(dirname "$0")/.."
export TZ=UTC
STAMP='1980-01-01 00:13'
[ -f corpus/mixed.bin ] || python3 tools/mkcorpus.py corpus
python3 tools/mkvbn2.py > /dev/null && g++ -m32 -O2 -o vbn2/vbn_ur vbn2/vbn_ur.cpp || exit 1
make -s -C .. vbn libvbnshim.so || exit 1

W=$(mktemp -d ./work.XXXXXX); W=$(cd "$W" && pwd)
trap 'rm -rf "$W"' EXIT

setup() {   # setup <dir> vbn|vbn_ur: a tree to work in, the program beside it in <dir>.bin
  mkdir -p "$1/sub/deep" "$1.bin"
  if [ "$2" = vbn ]; then cp ../vbn ../libvbnshim.so "$1.bin/"; cp ../vbn "$1.bin/X"
  else cp vbn2/vbn_ur "$1.bin/X"; fi
  cp ../input.txt "$1/a.txt"; cp corpus/rep.bin "$1/b.bin"
  head -c 30000 corpus/mixed.bin > "$1/sub/c.dat"; head -c 9000 ../vbn.exe > "$1/sub/deep/d.exe"
  find "$1" -type f -exec touch -d "$STAMP" {} +
}
run() {     # run <dir> <args...>
  (cd "$1" && VBN_PROBES=0 timeout 300 "$1.bin/X" "${@:2}" < /dev/null 2>&1 \
     | tr '\r' '\n' | sed -E 's/Time = [0-9]+ sec\./Time = N sec./'; echo "exit=${PIPESTATUS[0]}")
}
SEQS=(
  "single:u arc a.txt|!rm a.txt|r arc"
  "multi:u arc a.txt b.bin sub/c.dat|!rm -r a.txt b.bin sub|r arc"
  "recurse:u arc *.*|!rm -r a.txt b.bin sub|r arc"
  "masks:u arc *.txt *.bin|!rm a.txt b.bin|r arc"
  "subdir:u arc sub/*.*|!rm -r sub|r arc"
  "case:u ARC A.TXT|!rm a.txt|r arc"
  "overwrite:u arc a.txt b.bin|r arc"
  "again:u arc a.txt|u arc b.bin|!rm a.txt b.bin|r arc"
  "empty:!: > e.txt|u arc e.txt|!rm e.txt|r arc"
  "truncated:u arc a.txt|!truncate -s 5000 arc.vbn|!rm a.txt|r arc"
  "garbage:!head -c 3000 b.bin > arc.vbn|r arc"
  "target:u arc a.txt sub/c.dat|!rm -r a.txt sub|!mkdir out|r arc out"
  "newtarget:u arc a.txt|!rm a.txt|r arc new/dir"
)
fail=0
for s in "${SEQS[@]}"; do
  name=${s%%:*}; IFS='|' read -ra cmds <<< "${s#*:}"
  for e in vbn vbn_ur; do
    d="$W/$name.$e"; setup "$d" $e
    for c in "${cmds[@]}"; do
      echo "## $c"
      if [ "${c:0:1}" = '!' ]; then (cd "$d" && eval "${c:1}"); else run "$d" $c; fi
    done > "$d.out"
    (cd "$d" && find . -type f | sort | while read -r p; do   # an archive's mtime is when it was made
       case "${p,,}" in ./arc.*) t=-;; *) t=$(date -r "$p" '+%Y-%m-%d %H:%M');; esac
       echo "$p $(md5sum < "$p" | cut -c1-32) $t"; done) > "$d.files"
  done
  if cmp -s "$W/$name.vbn.out" "$W/$name.vbn_ur.out" && cmp -s "$W/$name.vbn.files" "$W/$name.vbn_ur.files"; then
    echo "same  $name ($(grep -c . "$W/$name.vbn.out") lines of output, $(wc -l < "$W/$name.vbn.files") files)"
  else
    echo "DIFF  $name"; fail=1
    diff "$W/$name.vbn.out" "$W/$name.vbn_ur.out" | head -10
    diff "$W/$name.vbn.files" "$W/$name.vbn_ur.files" | head -10
  fi
done
[ $fail = 0 ] && echo SAME || echo DIFFERENT
exit $fail
