#!/usr/bin/env bash
# fuzz.sh — run mutated inputs through an AddressSanitizer build.
#
#     tools/fuzz.sh                 # 400 mutants, seed 1
#     tools/fuzz.sh 2000 7          # 2000 mutants, seed 7
#     BMF_FUZZ_PLAIN=1 tools/fuzz.sh   # the -O2 binary instead
#
# The round before this one fuzzed the plain build and asked only "did it
# crash".  It ran 1122 compress and 900 expand mutants and found nothing, which
# was worth exactly as much as `test.sh` saying the fifteen streams match:
# both are silent about a body that writes past the end of something and gets
# away with it.  `tools/asan.sh` exists because that class had a live member in
# it -- `choose_plane_coding` clearing 192 bytes of a 120-byte run -- that nine
# rounds of byte-identical streams never noticed.  So this runs the same idea
# as asan.sh over inputs nobody wrote by hand.
#
# What counts as a finding:
#
#   * any AddressSanitizer report about memory safety, which is the point;
#   * any exit outside the documented table.  `__exit_402E40` in subs1.hpp
#     defines 1..8 and 0 is success; 139 and 134 are the shell reporting a
#     signal, and those are the seven that section 46 fixed.
#
# What does not: a mutant that is refused.  Every one of these files is broken
# on purpose and "bad file!" is the right answer to almost all of them.
#
# And ASan's *allocator* messages -- "out of memory", "requested allocation
# size ... exceeds maximum" -- are counted apart from the rest, because they
# are as often this script's `ulimit` as they are the program's fault.  The one
# that made the distinction: `x_ai.bmp` with `biHeight` at 0xFFFF is a legal
# BMP header describing a 2820x65535 image, and `x_ai.bmp` is RLE8, whose
# decoder memsets the whole buffer before it starts -- so the 185 MB is asked
# for honestly and filled honestly, and only the 1 GB limit below made it look
# like a finding.  A `member:data_len=0x40000000` in a 53 kB .bmf is the same
# message and a real defect.  Neither is decided by the message, so they are
# reported and not failed on.
set -u
cd "$(dirname "$0")/.."

count=${1:-400}
seed=${2:-1}

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

bin=$tmp/bmf
if [ "${BMF_FUZZ_PLAIN:-0}" = 1 ]; then
  BMF_OUT="$bin" ./build.sh >"$tmp/build" 2>&1 || { tail -20 "$tmp/build" >&2; exit 2; }
  label="the -O2 build"
else
  # Same three switches asan.sh needs and for the same reasons: ASan's runtime
  # does not survive -static here, and --gc-sections hides the frames.
  BMF_OUT="$bin" BMF_STATIC=0 BMF_GC=0 ./build.sh \
      -fsanitize=address -fno-omit-frame-pointer -g -O1 >"$tmp/build" 2>&1 \
      || { echo "the ASan build failed:" >&2; tail -20 "$tmp/build" >&2; exit 2; }
  label="an ASan build"
fi

# `BMF_FUZZ_EXTRA` used to carry one seed the corpus does not have: a
# two-member archive, because the walk in `expand_image` -- decode a member, go
# round, stop when one does not parse -- was entered once by everything here.
# There is no walk now; a `.bmf` holds one image.  The directory stays because
# `fuzz.py` takes it, and it is empty.
mkdir -p "$tmp/extra"
export ASAN_OPTIONS=detect_leaks=0

BMF_FUZZ_EXTRA="$tmp/extra" tools/fuzz.py "$tmp/in" "$count" "$seed" \
    >"$tmp/manifest" || exit 2

# One directory per run so a mutant that writes a huge output cannot fill the
# disk for the next one; `ulimit -f` caps that at 64 MB, well above the largest
# legitimate output (DLRAW is 490 kB) and well below the session's allowance.
mkdir -p "$tmp/out"

# `ulimit -v` and a 64-bit AddressSanitizer do not mix: ASan reserves a
# sixteen-terabyte shadow region on x86-64, so any address-space cap at all
# makes every single run abort before `main` -- which reads as 400 mutants
# reporting rather than as the instrument declining to start.  On -m64 the
# file-size cap and the timeout are the whole bound; ASan's own
# `allocator_may_return_null` keeps a runaway request from being fatal.
vlimit=(-v 1048576); [ "${BMF_BITS:-64}" = 32 ] || vlimit=()

bad=0 ran=0 refused=0 alloc=0
while IFS=$'\t' read -r file from what; do
  case $file in
    *.bmp) mode=c; dst=$tmp/out/$file.bmf ;;
    *)     mode=d; dst=$tmp/out/$file.bmp ;;
  esac
  log=$tmp/out/$file.log
  ( ulimit -f 65536 "${vlimit[@]}"
    ASAN_OPTIONS=detect_leaks=0:abort_on_error=0:allocator_may_return_null=1 \
      "$bin" "$mode" "$tmp/in/$file" "$dst" ) >"$log" 2>&1
  rc=$?
  ran=$((ran + 1))
  rm -f "$dst"
  if grep -q 'ERROR: AddressSanitizer' "$log" 2>/dev/null; then
    msg=$(grep -m1 'ERROR: AddressSanitizer' "$log" | sed 's/^==[0-9]*==ERROR: //')
    case $msg in
      *'out of memory'*|*'exceeds maximum supported size'*|*'allocation size'*)
        alloc=$((alloc + 1)); tag='(allocator)' ;;
      *)
        bad=$((bad + 1)); tag='' ;;
    esac
    printf '%-28s %-14s %-22s %s%s\n' "$file" "$from" "$what" "$tag" "$msg"
    [ -n "$tag" ] || grep -m1 -A1 'is located in stack of' "$log" | sed 's/^/    /'
    cp "$tmp/in/$file" "${BMF_FUZZ_KEEP:-$tmp}/" 2>/dev/null
  elif [ "$rc" -gt 8 ]; then
    bad=$((bad + 1))
    printf '%-28s %-14s %-22s exited %d\n' "$file" "$from" "$what" "$rc"
    cp "$tmp/in/$file" "${BMF_FUZZ_KEEP:-$tmp}/" 2>/dev/null
  elif [ "$rc" != 0 ]; then
    refused=$((refused + 1))
  fi
done <"$tmp/manifest"

echo
seeds=$(( $(ls testfiles | grep -cE '\.bm[pf]$') + $(ls "$tmp/extra" | wc -l) ))
echo "$ran mutants of $seeds seed files through $label, seed $seed"
echo "$refused refused, $((ran - refused - bad - alloc)) accepted, $bad reported, $alloc allocator"
# The corpus and this script both feed the same generator, so "seed $seed" only
# names a set of inputs together with the version of `fuzz.py` that made them:
# adding the `member` mutator renumbered every mutant after the first .bmf.
[ $((bad + alloc)) = 0 ] ||
  echo "re-make any of these with: tools/fuzz.py DIR $count $seed, at this revision"
exit $((bad ? 1 : 0))
