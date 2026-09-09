#!/bin/sh
#  The guards on model size, and on what a failed run leaves behind.
#
#  IDX/opt.pl flips pattern bits and keeps whatever makes the corpus smaller.
#  The factor sizes in an index multiply, so a set that asks for a plausible
#  table is a few flips from one that asks for forty-seven gigabytes -- and a
#  run that dies partway through leaves a prefix of its output, which is small,
#  which is exactly what the search is looking for.  That is a search that
#  converges on a crash.  These are the checks that stop it.
set -e
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$here"
tmp=$(mktemp -d "${TMPDIR:-/tmp}/tc-guard.XXXXXX")
trap 'rm -rf "$tmp"' EXIT INT TERM
fail=0
say() { printf '  %-46s %s\n' "$1" "$2"; }

#  $1 = a .idx under IDX/, $2 = the binary to build from it.  The tuning form,
#  because that is the one IDX/opt.pl patches and runs.
build() {
  b=$(basename "$1" .idx)
  cp -f IDX/tsvcomp.inc "IDX/$b.inc"
  ( cd IDX && IDX_NOCONST=0 perl idx2inc.pl "$b.idx" 1 >/dev/null 2>&1 ) || return 1
  mv -f "IDX/${b}_h.inc" MOD/tsvcomp_h.inc || return 1
  mv -f "IDX/${b}_p.inc" MOD/tsvcomp_p.inc || return 1
  c++ ${CXXFLAGS:--O2} -fwrapv -o "$2" tsvcomp.cpp -lm
}

[ -n "$1" ] || { echo "usage: ./t-guard.sh some.tsv" >&2; exit 2; }
in=$1

#  A control: the shipped parameters code the file and leave it behind.
cp IDX/tsvcomp.idx "IDX/tc-guard.idx"
build IDX/tc-guard.idx "$tmp/ok"
if "$tmp/ok" c "$in" "$tmp/a.tc" >/dev/null 2>&1 && [ -s "$tmp/a.tc" ]
  then say "shipped parameters code the file" "ok"
  else say "shipped parameters code the file" "FAILED"; fail=1; fi

#  An index widened past what the machine has: refused by name, before a byte
#  of output exists, with a status the caller can see.
sed 's/^\( *dig_a_[a-z0-9]*: *[a-z0-9]*, *1!\)[01]*$/\1111111111111111/' \
    IDX/tsvcomp.idx > "IDX/tc-guard.idx"
build IDX/tc-guard.idx "$tmp/big" 2>/dev/null || true
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
rm -f IDX/tc-guard.idx IDX/tc-guard.inc IDX/tc-guard IDX/tc-guard_h.inc IDX/tc-guard_p.inc

#  Leave MOD/ as it is checked in -- the shipping form, which is ./mk.sh
#  release's, not the tuning form the two builds above were made in.
./mk.sh release >/dev/null

[ "$fail" = 0 ] && echo "t-guard.sh: the guards hold" \
                || { echo "t-guard.sh: A GUARD DOES NOT HOLD" >&2; exit 1; }
