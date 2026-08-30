#!/bin/sh
#
# Every configuration t.sh should pass, in one go. t.sh checks one build; this
# checks that the -D space still holds together, which is where the breakage
# actually happens: a knob that skips work on one path and restores it on
# another can look fine at the defaults and segfault at a different RCNUM.
#
#   ./t_matrix.sh                  clang, -march=skylake
#   ARCH=native ./t_matrix.sh      the other target
#   CXX=g++ ./t_matrix.sh          the other compiler
#   TESTFILE=../book1 ./t_matrix.sh
#
# There are two coders and they are not stream-compatible, so there are two
# reference streams: every format-neutral row of a coder is compared against
# its own. The rows that matter most are the RC_VEC=0 ones -- the coding pass
# on rc_vec.inc's scalar coders instead of the generated kernel, from the same
# source file. For rANS that is the only test that sees the encoder's float32
# division: the scalar side divides with the integer `/`, so a run where the
# two produce the same 100 MB is a run where the float path is right.
#
# Each row is a roundtrip plus a byte-identical re-encode.
set -e
cd "$(dirname "$0")"
: "${NITER:=1}"; export NITER
: "${TESTFILE:=../enwik8}"; export TESTFILE

fail=0

# runsame: the row must reproduce its coder's reference stream as well as
#          roundtrip.
# run:     the row legitimately changes the format (geometry, or the coder's
#          own precision), so roundtrip only.
REF_RANS=$(pwd)/t_ref_rans.enc
REF_RC=$(pwd)/t_ref_rc.enc
rm -f "$REF_RANS" "$REF_RC"

_run() {
  want=$1; ref=$2; args=$3
  printf '  %-46s ' "${args:-<defaults>}"
  # shellcheck disable=SC2086
  if out=$(REFSTREAM="$ref" ./t.sh $args 2>&1); then
    ok=$(printf '%s' "$out" | grep -c 'ok: ')
    if [ "$ok" -ge "$want" ]; then echo "ok"; else echo "FAIL (checks missing)"; fail=1; fi
  else
    echo "FAIL"; fail=1
    printf '%s\n' "$out" | tail -4 | sed 's/^/      /'
  fi
}
runsame() { _run 2 "$REF_RANS" "$1"; }
run()     { _run 1 "" "$1"; }
rcsame()  { _run 2 "$REF_RC" "$1 -DRC_RANS=0"; }
rcrun()   { _run 1 "" "$1 -DRC_RANS=0"; }

echo "== ${CXX:-clang++}, -march=${ARCH:-skylake}, $TESTFILE"

#--- the rANS coder ----------------------------------------------------------
echo "-- rANS (rans.inc)"

# the reference row first, so every runsame below has something to match
runsame ""
# the generated kernel against rc_vec.inc's scalar coders, from one source.
# This is the row that checks RANS_DIV against an integer divide.
runsame "-DRC_VEC=0"
runsame "-DRC_SCATTER=0"
runsame "-DRC_SCATTER_W=0"
runsame "-DRC_SCATTER_SKIP=1"
runsame "-DRC_FUSE_PP_ENC=0"
runsame "-DRC_FUSE_PP_ENC=1"
run     "-DRC_RCNUM=32"
run     "-DRC_RCNUM=32 -DRC_VEC=0"
run     "-DCORO_FAKE=1"
# the state's range: a different stream, and the one knob that can push the
# float division out of its window (see misc/rans_div.cpp)
run     "-DRC_RANS_KLOG=1"
run     "-DRC_RANS_KLOG=4"
run     "-DRC_RANS_KLOG=8 -DRC_VEC=0"
run     "-DRC_BLKSIZE=4096"

#--- the range coder ---------------------------------------------------------
echo "-- range coder (rc.inc)"

rcsame ""
rcsame "-DRC_VEC=0"
rcrun   "-DRC_RCNUM=32"
rcrun   "-DCORO_FAKE=1"
rcsame "-DRC_SCATTER=0"
rcrun   "-DRC_CODBYTES=3 -DRC_LOWBYTES=7"
rcrun   "-DRC_CODBYTES=3"
rcsame "-DRC_FOLD_RPRE=1"
rcsame "-DRC_SCATTER_SKIP=1"
rcsame "-DRC_SCATTER_W=0"
rcsame "-DRC_ENC_NSEL=1"
rcsame "-DRC_ENC_NSEL=2"
rcsame "-DRC_ENC_RENORM=1"
rcsame "-DRC_ENC_RENORM=2"
rcsame "-DRC_ENC_RENORM=1 -DRC_ENC_NSEL=1"
rcsame "-DRC_ENC_RENORM=2 -DRC_ENC_NSEL=2"
rcsame "-DRC_FUSE_PP_ENC=0"
rcsame "-DRC_FUSE_PP_ENC=1"
# the two defs profiles themselves
rcsame "-DRC_FOLD_RPRE=1 -DRC_SCATTER_SKIP=0 -DRC_ENC_NSEL=0 -DRC_ENC_RENORM=0"
rcsame "-DRC_FOLD_RPRE=0 -DRC_SCATTER_SKIP=1 -DRC_ENC_NSEL=1 -DRC_ENC_RENORM=2"
# combinations that have bitten before: a knob that moves work between paths
rcrun   "-DRC_RCNUM=32 -DRC_ENC_NSEL=1"
rcrun   "-DRC_CODBYTES=3 -DRC_LOWBYTES=7 -DRC_FOLD_RPRE=1"

#--- the rANS encoder's division, proved rather than sampled ------------------
# One test that is not a coder run: for every frequency the model can produce,
# is the float32 quotient never above the true one and never more than one
# below? That is what rans.inc's single masked correction assumes, and it is
# 32767 cases, so it can be settled exhaustively instead of measured.
echo "-- misc/rans_div.cpp (exhaustive, all 32767 frequencies)"
for k in 1 4 8; do
  printf '  %-46s ' "-DRC_RANS_KLOG=$k"
  if ${CXX:-clang++} -O2 -std=c++17 -I../Lib3 -DRC_RANS_KLOG=$k misc/rans_div.cpp -o rans_div 2>/dev/null \
     && out=$(./rans_div); then
    printf '%s\n' "$out" | tail -1 | sed 's/^  *//'
  else
    echo "FAIL"; fail=1
    printf '%s\n' "$out" | tail -4 | sed 's/^/      /'
  fi
done
rm -f rans_div

rm -f "$REF_RANS" "$REF_RC"
[ "$fail" = 0 ] && echo "== all configurations pass" || { echo "== FAILURES above"; exit 1; }
