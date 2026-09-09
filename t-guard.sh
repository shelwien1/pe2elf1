#!/bin/sh
#  What an index may ask for, and what a failed run leaves behind.
#
#  IDX/opt.pl flips pattern bits and keeps whatever makes the corpus smaller.
#  The factor sizes in an index multiply, so the sets it reaches are wide --
#  tens of gigabytes of tables, of which a link touches a few hundred
#  megabytes -- and that has to work, because that is where the wins are.
#  What must not work is the set past even that, and what must never look
#  like a win is a run that died partway through: its output is a prefix,
#  a prefix is small, and small is exactly what the search is looking for.
#  These are the checks on both edges.
set -e
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$here"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/tc-guard.XXXXXX")
trap 'rm -rf "$tmp"' EXIT INT TERM
fail=0
say() { printf '  %-46s %s\n' "$1" "$2"; }

#  $1 = a sed program applied to the dig module, $2 = the binary to build.
#  The tuning form, because that is the one IDX/opt.pl patches and runs.  The
#  six modules are generated from a copy of IDX/, so a widened index never
#  lands in the tree and an interrupted run leaves nothing behind but MOD/,
#  which the tail of this script puts back.
build() {
  rm -rf "$tmp/IDX";  cp -R IDX "$tmp/IDX"
  sed -e "$1" IDX/tsvcomp-dig.idx > "$tmp/IDX/tsvcomp-dig.idx"
  for f in dig sgn flr cls aux hdr; do
    ( cd "$tmp/IDX" && IDX_NOCONST=0 perl idx2inc.pl "tsvcomp-$f.idx" 1 \
        >/dev/null 2>&1 ) || return 1
    mv -f "$tmp/IDX/tsvcomp-${f}_h.inc" "MOD/tsvcomp-${f}_h.inc" || return 1
    mv -f "$tmp/IDX/tsvcomp-${f}_p.inc" "MOD/tsvcomp-${f}_p.inc" || return 1
  done
  c++ ${CXXFLAGS:--O2} -fwrapv -o "$2" tsvcomp.cpp -lm
}

[ -n "$1" ] || { echo "usage: ./t-guard.sh some.tsv" >&2; exit 2; }
in=$1

#  A control: the shipped parameters code the file and leave it behind.
build '' "$tmp/ok"
if "$tmp/ok" c "$in" "$tmp/a.tc" >/dev/null 2>&1 && [ -s "$tmp/a.tc" ]
  then say "shipped parameters code the file" "ok"
  else say "shipped parameters code the file" "FAILED"; fail=1; fi

#  Wide, and legitimately so: two thresholds opened up put dig_a's counter at
#  5.4 billion entries -- 21.7 GB of tables, past what a 32-bit row base could
#  address and well past what this machine holds.  It has to code the file
#  anyway.  The pages a link actually reaches are a rounding error against what
#  it reserves, so a wide index costs address space and almost no memory, and
#  that is the whole reason an index is allowed to be this wide.
build 's/^\( *a_\(q1s\|ps\): *[a-z0-9]*, *-6!\)[01]*$/\11111111111111/' \
      "$tmp/wide"
"$tmp/wide" c "$in" "$tmp/c.tc" >/dev/null 2>&1 &
wp=$!
rss=0
while kill -0 "$wp" 2>/dev/null; do
  if [ -r "/proc/$wp/status" ]; then
    r=$(awk '/VmHWM/{print $2}' "/proc/$wp/status" 2>/dev/null || true)
    [ -n "$r" ] && rss=$r
  fi
  sleep 1
done
wrc=0;  wait "$wp" || wrc=$?
if [ "$wrc" != 0 ]; then
  say "a wide index codes the file" "FAILED (exit $wrc)"; fail=1
elif ! "$tmp/wide" d "$tmp/c.tc" "$tmp/c.tsv" >/dev/null 2>&1 || ! cmp -s "$in" "$tmp/c.tsv"; then
  say "a wide index codes the file" "FAILED (the stream does not decode)"; fail=1
elif [ "$rss" -gt 1048576 ]; then
  say "a wide index costs no memory" "FAILED ($((rss/1024)) MB resident)"; fail=1
elif [ "$rss" = 0 ]; then
  say "a wide index codes the file" "ok (no /proc, so no memory reading)"
else
  say "a wide index codes the file" "ok -- $((rss/1024)) MB resident of 21.7 GB"
fi

#  An index widened past what the machine has: refused by name, before a byte
#  of output exists, with a status the caller can see.
build 's/^\( *a_[a-z0-9]*: *[a-z0-9]*, *1!\)[01]*$/\1111111111111111/' \
      "$tmp/big" 2>/dev/null || true
rm -f "$tmp/b.tc"
if [ -x "$tmp/big" ]; then
  if "$tmp/big" c "$in" "$tmp/b.tc" >"$tmp/msg" 2>&1
    then say "an oversized index is refused" "FAILED (it ran)"; fail=1
    else if [ -e "$tmp/b.tc" ]
           then say "an oversized index is refused" "FAILED (left an output)"; fail=1
           else say "an oversized index is refused" "ok -- $(cut -c1-60 "$tmp/msg" | head -1)"; fi
  fi
else
  say "an oversized index is refused" "ok (the build itself refused it)"
fi
#  Leave MOD/ as it is checked in -- the shipping form, not the tuning form the
#  builds above were made in.  Not `./mk.sh release`, which would also replace
#  ./tsvcomp with a binary carrying no !MAP! markers, and so silently end
#  whatever tuning run was using it.
./mk.sh mod >/dev/null

[ "$fail" = 0 ] && echo "t-guard.sh: the guards hold" \
                || { echo "t-guard.sh: A GUARD DOES NOT HOLD" >&2; exit 1; }
