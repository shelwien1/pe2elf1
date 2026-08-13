#!/usr/bin/env bash
# x64diff.sh — do the two pointer widths answer the same on inputs nobody chose?
#
#     tools/x64diff.sh [count] [seed]
#     BMF_HIGH=1 tools/x64diff.sh   # and put the 64-bit side above 4 GB
#
# `tools/x64.sh` asks the corpus.  The corpus is fifteen images and every one of
# them has been looked at for eleven rounds, which is exactly what makes it a
# weak differential: a path no test image takes can differ between the targets
# for as long as it likes.  One did.  `choose_plane_coding` read a union slot
# as an `int64_t` where the low half was wanted, and the top half is a plane
# index -- eight gigabytes of offset with a 64-bit pointer, and nothing at all
# with a 32-bit one, because the index converts to `ptrdiff_t` first.  It cost
# three of the fifteen streams; a path the corpus does not enter would have
# cost nothing visible.
#
# So this runs `tools/fuzz.py`'s mutants through *both* builds and compares
# what comes out: the exit code, and the bytes when there are any.  Neither
# build is trusted -- the claim is only that they agree, which is the claim
# that matters for a file whose whole subject is one target's assumptions.
#
# No AddressSanitizer here.  `tools/fuzz.sh` already asks that question of each
# build on its own; this one is about the answers, so it runs the ordinary -O2
# builds and can afford many more mutants.
set -u
# A job killed by a signal is reported by the shell that waits on it, and that
# shell is this one -- so the redirect is on the loop, not on the command.
set +m
cd "$(dirname "$0")/.."

count=${1:-400}
seed=${2:-1}

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# `BMF_HIGH=1` builds the 64-bit side with `-DBMF_HIGH_ARENA`, so every
# allocation sits where a four-byte pointer cannot name it.  The two questions
# compose: the mutants ask whether the answers agree, and the arena asks
# whether any of those paths still truncates.  Off by default because
# `MAP_FIXED_NOREPLACE` is Linux-only and the fallback here would be silent.
high=; [ "${BMF_HIGH:-0}" = 1 ] && high=-DBMF_HIGH_ARENA
for bits in 32 64; do
  opt=; [ $bits = 64 ] && opt=$high
  BMF_OUT="$tmp/bmf$bits" BMF_BITS=$bits ./build.sh $opt >"$tmp/build$bits" 2>&1 || {
    echo "the -m$bits build failed:" >&2; grep -m5 'error:' "$tmp/build$bits" >&2; exit 2; }
done
if [ -n "$high" ] && ! "$tmp/bmf64" c testfiles/t1.bmp "$tmp/probe.bmf" >/dev/null 2>&1; then
  echo "the high arena could not be placed here" >&2; exit 2
fi
rm -f "$tmp/probe.bmf"

# The seeds `fuzz.sh` uses, and for the same reasons: the corpus, the reference
# streams, and a two-member archive, which no single image is.
# Concatenated for the reason `fuzz.sh` gives: `bmf c` creates its output, so
# building this by running it twice against one path leaves one member.
mkdir -p "$tmp/extra"
"$tmp/bmf32" c testfiles/t1.bmp  "$tmp/extra/a1.bmf" >/dev/null 2>&1 &&
"$tmp/bmf32" c testfiles/t8g.bmp "$tmp/extra/a2.bmf" >/dev/null 2>&1 &&
cat "$tmp/extra/a1.bmf" "$tmp/extra/a2.bmf" > "$tmp/extra/arc2.bmf" &&
[ "$(stat -c%s "$tmp/extra/arc2.bmf")" = "$(( $(stat -c%s "$tmp/extra/a1.bmf") +
                                              $(stat -c%s "$tmp/extra/a2.bmf") ))" ] &&
rm -f "$tmp/extra/a1.bmf" "$tmp/extra/a2.bmf" || {
  echo "warning: could not build the two-member archive" >&2
  rm -f "$tmp/extra/arc2.bmf" "$tmp/extra/a1.bmf" "$tmp/extra/a2.bmf"; }

BMF_FUZZ_EXTRA="$tmp/extra" tools/fuzz.py "$tmp/in" "$count" "$seed" \
    >"$tmp/manifest" || exit 2
mkdir -p "$tmp/o32" "$tmp/o64"

same=0 rc_differ=0 out_differ=0
while IFS=$'\t' read -r file from what; do
  case $file in
    *.bmp) mode=c; dst=$file.bmf ;;
    *)     mode=d; dst=$file.bmp ;;
  esac
  rm -f "$tmp/o32/$dst" "$tmp/o64/$dst"
  # `ulimit -f`, as in fuzz.sh: a mutant that claims a huge image writes it.
  ( ulimit -f 65536 -v 1048576
    timeout 60 "$tmp/bmf32" "$mode" "$tmp/in/$file" "$tmp/o32/$dst" ) >/dev/null 2>&1
  a=$?
  # No `ulimit -v` on the 64-bit side under `BMF_HIGH`: the arena reserves 768
  # MB up front and any address-space cap refuses it, which would read as every
  # mutant disagreeing.
  ( ulimit -f 65536
    timeout 60 "$tmp/bmf64" "$mode" "$tmp/in/$file" "$tmp/o64/$dst" ) >/dev/null 2>&1
  b=$?
  if [ "$a" != "$b" ]; then
    printf '%-28s %-14s %-22s -m32 exits %s, -m64 exits %s\n' "$file" "$from" "$what" "$a" "$b"
    rc_differ=$((rc_differ + 1))
    continue
  fi
  # Both refused, and refused the same way: there is nothing to compare.
  if [ ! -f "$tmp/o32/$dst" ] && [ ! -f "$tmp/o64/$dst" ]; then
    same=$((same + 1)); continue
  fi
  if cmp -s "$tmp/o32/$dst" "$tmp/o64/$dst"; then
    same=$((same + 1))
  else
    printf '%-28s %-14s %-22s output differs (%s vs %s bytes)\n' "$file" "$from" "$what" \
      "$(stat -c%s "$tmp/o32/$dst" 2>/dev/null || echo none)" \
      "$(stat -c%s "$tmp/o64/$dst" 2>/dev/null || echo none)"
    out_differ=$((out_differ + 1))
  fi
done <"$tmp/manifest" 2>/dev/null

echo
echo "$count mutants of $(cut -f2 "$tmp/manifest" | sort -u | wc -l) seed files through both builds, seed $seed${high:+, the 64-bit side above 4 GB}"
echo "$same agree, $rc_differ differ in exit code, $out_differ in output"
echo "re-make any of these with: tools/fuzz.py DIR $count $seed, at this revision"
exit $(( (rc_differ + out_differ) ? 1 : 0 ))
