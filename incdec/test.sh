#!/usr/bin/env bash
# test.sh [--baseline]: the incdec gate (incdec.md §9) for vbn.
#
# For each corpus file: `vbn u archive <file>` in a fresh directory, then
# `vbn r archive` in another one that holds only the archive.  Pass means
#   - archive.vbn is byte-identical to the reference,
#   - the restored file is identical to the input, with the same mtime,
#   - stdout of both runs matches the reference, the elapsed-time line aside.
# --baseline writes the references with the un-injected port (../vbn).
#
# The input mtime is pinned: VBN stores it in the archive.
set -u
cd "$(dirname "$0")"
export TZ=UTC
STAMP='1980-01-01 00:13'
CORPUS="../input.txt corpus/mixed.bin corpus/rep.bin ../vbn.exe"
BASELINE=0; [ "${1:-}" = --baseline ] && BASELINE=1

[ -f corpus/mixed.bin ] || python3 tools/mkcorpus.py corpus
if [ $BASELINE = 0 ] && [ ! -d ref ]; then "$0" --baseline || exit 1; fi
if [ $BASELINE = 1 ]; then
  make -s -C .. vbn libvbnshim.so || exit 1
  BIN="../vbn ../libvbnshim.so"; EXE=./vbn; mkdir -p ref
else
  BIN="vbn.elf libvbnshim.so dummy.so"; EXE=./vbn.elf
fi
for f in $BIN; do [ -f "$f" ] || { echo "missing $f (run build.sh)"; exit 1; }; done

work=$(mktemp -d ./work.XXXXXX)
trap 'rm -rf "$work"' EXIT
norm() { tr '\r' '\n' < "$1" | sed -E 's/Time = [0-9]+ sec\./Time = N sec./'; }

one() {   # one <file>: prints one result line, exit status = pass/fail
  local src=$1 f; f=$(basename "$1")
  local d="$work/$f"; mkdir -p "$d/u" "$d/r"
  cp $BIN "$d/u/"; cp $BIN "$d/r/"; cp "$src" "$d/u/$f"; touch -d "$STAMP" "$d/u/$f"
  (cd "$d/u" && timeout 300 $EXE u archive "$f" > ../u.out 2> ../u.err); local urc=$?
  cp "$d/u/archive.vbn" "$d/r/" 2>/dev/null
  (cd "$d/r" && timeout 300 $EXE r archive > ../r.out 2> ../r.err); local rrc=$?
  if [ $BASELINE = 1 ]; then
    cp "$d/u/archive.vbn" "ref/$f.vbn"; norm "$d/u.out" > "ref/$f.u.out"; norm "$d/r.out" > "ref/$f.r.out"
    cmp -s "$d/r/$f" "$src" || { echo "FAIL $f: the baseline itself does not round-trip"; return 1; }
    echo "ref  $f: archive $(stat -c%s "ref/$f.vbn") bytes"; return 0
  fi
  local why=""
  [ $urc -ne 0 ] && why+=" u-exit=$urc"
  [ $rrc -ne 0 ] && why+=" r-exit=$rrc"
  cmp -s "$d/u/archive.vbn" "ref/$f.vbn" || why+=" archive-differs"
  cmp -s "$d/r/$f" "$src" || why+=" restore-differs"
  [ "$(date -r "$d/r/$f" '+%Y-%m-%d %H:%M' 2>/dev/null)" = "$STAMP" ] || why+=" mtime"
  norm "$d/u.out" | cmp -s - "ref/$f.u.out" || why+=" u-stdout"
  norm "$d/r.out" | cmp -s - "ref/$f.r.out" || why+=" r-stdout"
  cat "$d/u.err" "$d/r.err" | grep -v '^\[probe\]' | head -3 | sed "s/^/  $f: /"
  # probe counts, u and r summed
  cat "$d/u.err" "$d/r.err" | awk '/^\[probe\]   /{c[$2]+=$3} END{for(k in c) print k, c[k]}' > "$work/$f.probes"
  if [ -z "$why" ]; then echo "ok   $f"; return 0; fi
  echo "FAIL $f:$why"; return 1
}

pids=(); for src in $CORPUS; do one "$src" > "$work/$(basename "$src").res" & pids+=($!); done
fail=0; for p in "${pids[@]}"; do wait "$p" || fail=1; done
for src in $CORPUS; do cat "$work/$(basename "$src").res"; done
if [ $BASELINE = 0 ]; then
  cat "$work"/*.probes 2>/dev/null | awk '{c[$1]+=$2} END{for(k in c) printf "%s %d\n", k, c[k]}' | sort > probes.txt
  echo "probes: $(awk '$2>0' probes.txt | wc -l) of $(wc -l < probes.txt) moved functions were called (probes.txt)"
  zero=$(awk '$2==0{print $1}' probes.txt | tr '\n' ' ')
  [ -n "$zero" ] && echo "never called: $zero"
fi
[ $fail = 0 ] && echo PASS || echo FAIL
exit $fail
