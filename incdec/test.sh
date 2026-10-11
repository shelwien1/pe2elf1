#!/usr/bin/env bash
# test.sh [--baseline | --exe PATH]: the incdec gate (incdec.md §9) for vbn.
#
# For each corpus entry: `vbn u archive <file>` in a fresh directory holding
# only that file -- for a directory, its tree and `vbn u archive *.*` -- then
# `vbn r archive` in another one that holds only the archive.  Pass means
#   - archive.vbn is byte-identical to the reference,
#   - the restore recreates the input tree exactly: names, contents, mtimes,
#   - stdout of both runs matches the reference, the elapsed-time line aside.
# `r` also restores the entry's `b` (NORMAL) and `B` (FAST) archives, which the
# baseline makes with ../vbn: same tree, same stdout.
# Then each command in errors.txt runs in an empty directory; its stdout and
# exit code have to match the reference.
# --baseline writes the references with the un-injected port (../vbn).
# --exe PATH runs the gate on another build of vbn, such as ../vbn2.
#
# The binaries run from their own directory, so that a `*.*` scan sees only
# the input.  The inputs' mtimes are pinned: VBN stores them in the archive.
set -u
BASELINE=0; EXE=; [ "${1:-}" = --baseline ] && BASELINE=1
if [ "${1:-}" = --exe ]; then
  [ -n "${2:-}" ] && [ -f "$2" ] && [ -x "$2" ] || { echo "usage: $0 [--baseline | --exe PATH]"; exit 1; }
  EXE=$(cd "$(dirname "$2")" && pwd)/$(basename "$2")
fi
cd "$(dirname "$0")"
export TZ=UTC
STAMP='1980-01-01 00:13'
CORPUS="../input.txt corpus/mixed.bin corpus/rep.bin ../vbn.exe corpus/big.bin corpus/tree"

[ -f corpus/tree/sub/deep/d.exe ] || python3 tools/mkcorpus.py corpus
if [ $BASELINE = 0 ] && [ ! -d ref ]; then "$0" --baseline || exit 1; fi
if [ $BASELINE = 1 ]; then
  make -s -C .. vbn libvbnshim.so || exit 1
  RUN=$(cd .. && pwd)/vbn; NEED="../vbn ../libvbnshim.so"; mkdir -p ref
elif [ -n "$EXE" ]; then
  RUN=$EXE; NEED=$EXE
else
  RUN=$PWD/vbn.elf; NEED="vbn.elf libvbnshim.so dummy.so"
fi
for f in $NEED; do [ -f "$f" ] || { echo "missing $f (run build.sh)"; exit 1; }; done

work=$(mktemp -d ./work.XXXXXX)
trap 'rm -rf "$work"' EXIT
norm() { tr '\r' '\n' < "$1" | sed -E 's/Time = [0-9]+ sec\./Time = N sec./'; }
listing() {   # every file under $1 but the archive: path, md5, mtime
  (cd "$1" && find . -type f ! -name archive.vbn | sort | while read -r p; do
     echo "$p $(md5sum < "$p" | cut -c1-32) $(date -r "$p" '+%Y-%m-%d %H:%M')"; done)
}

one() {   # one <file|dir>: prints one result line, exit status = pass/fail
  local src=$1 f arg; f=$(basename "$1")
  local d="$work/$f"; mkdir -p "$d/u" "$d/r"
  if [ -d "$src" ]; then cp -r "$src/." "$d/u/"; arg='*.*'; else cp "$src" "$d/u/$f"; arg=$f; fi
  find "$d/u" -type f -exec touch -d "$STAMP" {} +
  (cd "$d/u" && timeout 300 "$RUN" u archive "$arg" > ../u.out 2> ../u.err); local urc=$?
  cp "$d/u/archive.vbn" "$d/r/" 2>/dev/null
  (cd "$d/r" && timeout 300 "$RUN" r archive > ../r.out 2> ../r.err); local rrc=$?
  listing "$d/u" > "$d/u.files"; listing "$d/r" > "$d/r.files"
  # `r` also has to restore archives that `b` (NORMAL) and `B` (FAST) wrote;
  # the baseline makes them with ../vbn and the gate restores them.
  local m mrc=""
  for m in b B; do
    if [ $BASELINE = 1 ]; then
      mkdir -p "$d/$m"; cp -a "$d/u/." "$d/$m/"; rm -f "$d/$m/archive.vbn"
      (cd "$d/$m" && timeout 300 "$RUN" $m archive "$arg" > /dev/null 2>&1) && cp "$d/$m/archive.vbn" "ref/$f.$m.vbn"
    fi
    mkdir -p "$d/r$m"; cp "ref/$f.$m.vbn" "$d/r$m/archive.vbn"
    (cd "$d/r$m" && timeout 300 "$RUN" r archive > ../r$m.out 2> ../r$m.err) || mrc+=" r$m-exit=$?"
    listing "$d/r$m" > "$d/r$m.files"
  done
  if [ $BASELINE = 1 ]; then
    cp "$d/u/archive.vbn" "ref/$f.vbn"; norm "$d/u.out" > "ref/$f.u.out"; norm "$d/r.out" > "ref/$f.r.out"
    norm "$d/rb.out" > "ref/$f.rb.out"; norm "$d/rB.out" > "ref/$f.rB.out"
    for m in r rb rB; do
      cmp -s "$d/u.files" "$d/$m.files" || { echo "FAIL $f: the baseline itself does not round-trip ($m)"; return 1; }
    done
    echo "ref  $f: archive $(stat -c%s "ref/$f.vbn") bytes (b $(stat -c%s "ref/$f.b.vbn"), B $(stat -c%s "ref/$f.B.vbn")), $(wc -l < "$d/u.files") file(s)"
    return 0
  fi
  local why="$mrc"
  [ $urc -ne 0 ] && why+=" u-exit=$urc"
  [ $rrc -ne 0 ] && why+=" r-exit=$rrc"
  cmp -s "$d/u/archive.vbn" "ref/$f.vbn" || why+=" archive-differs"
  for m in r rb rB; do
    cmp -s "$d/u.files" "$d/$m.files" || why+=" $m-restore-differs"
    norm "$d/$m.out" | cmp -s - "ref/$f.$m.out" || why+=" $m-stdout"
  done
  norm "$d/u.out" | cmp -s - "ref/$f.u.out" || why+=" u-stdout"
  cat "$d/u.err" "$d/r.err" "$d/rb.err" "$d/rB.err" | grep -v '^\[probe\]' | head -3 | sed "s/^/  $f: /"
  # probe counts, all runs summed
  cat "$d/u.err" "$d/r.err" "$d/rb.err" "$d/rB.err" \
    | awk '/^\[probe\]   /{c[$2]+=$3} END{for(k in c) print k, c[k]}' > "$work/$f.probes"
  if [ -z "$why" ]; then echo "ok   $f"; return 0; fi
  echo "FAIL $f:$why"; return 1
}

err() {   # err <n> <args...>: a user error (errors.txt), run in an empty directory
  local n=$1; shift
  local d="$work/err$n"; mkdir -p "$d/x"
  (cd "$d/x" && timeout 300 "$RUN" "$@" > ../out 2> ../err); local rc=$?
  { norm "$d/out"; echo "exit=$rc"; } > "$d/res"
  cat "$d/err" | awk '/^\[probe\]   /{c[$2]+=$3} END{for(k in c) print k, c[k]}' > "$work/err$n.probes"
  if [ $BASELINE = 1 ]; then cp "$d/res" "ref/err$n.out"; echo "ref  error $n: $*"; return 0; fi
  if cmp -s "$d/res" "ref/err$n.out"; then echo "ok   error $n: $*"; return 0; fi
  echo "FAIL error $n: $*: stdout or exit code differs"; return 1
}

pids=(); for src in $CORPUS; do one "$src" > "$work/$(basename "$src").res" & pids+=($!); done
n=0; while read -r line; do
  case "$line" in ''|'#'*) continue;; esac
  n=$((n + 1)); (set -f; err $n $line) > "$work/err$n.res" & pids+=($!)
done < errors.txt
fail=0; for p in "${pids[@]}"; do wait "$p" || fail=1; done
for src in $CORPUS; do cat "$work/$(basename "$src").res"; done
cat "$work"/err*.res
if [ $BASELINE = 0 ] && [ -z "$EXE" ]; then
  cat "$work"/*.probes 2>/dev/null | awk '{c[$1]+=$2} END{for(k in c) printf "%s %d\n", k, c[k]}' | sort > probes.txt
  echo "probes: $(awk '$2>0' probes.txt | wc -l) of $(wc -l < probes.txt) moved functions were called (probes.txt)"
  zero=$(awk '$2==0{print $1}' probes.txt | tr '\n' ' ')
  [ -n "$zero" ] && echo "never called: $zero"
fi
[ $fail = 0 ] && echo PASS || echo FAIL
exit $fail
