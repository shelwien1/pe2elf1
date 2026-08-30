#!/bin/sh
#
# Every configuration t.sh should pass, in one go. t.sh checks one build; this
# checks that the -D space still holds together, which is where the breakage
# actually happens: a knob that skips work on one path and restores it on
# another can look fine at the defaults and segfault at RC_VEC=0.
#
#   ./t_matrix.sh                  clang, -march=skylake
#   ARCH=native ./t_matrix.sh      the other target
#   CXX=g++ ./t_matrix.sh          the other compiler
#   TESTFILE=../book1 ./t_matrix.sh
#
# Each row is roundtrip + byte-identical against the -C scalar reference.
set -e
cd "$(dirname "$0")"
: "${NITER:=1}"; export NITER
: "${TESTFILE:=../enwik8}"; export TESTFILE

fail=0
run() {
  printf '  %-42s ' "${1:-<defaults>}"
  # shellcheck disable=SC2086
  if out=$(./t.sh $1 2>&1); then
    ok=$(printf '%s' "$out" | grep -c 'ok: ')
    if [ "$ok" -ge 2 ]; then echo "ok"; else echo "FAIL (checks missing)"; fail=1; fi
  else
    echo "FAIL"; fail=1
    printf '%s\n' "$out" | tail -4 | sed 's/^/      /'
  fi
}

echo "== ${CXX:-clang++}, -march=${ARCH:-skylake}, $TESTFILE"

# geometry and the paths that compile the vector coder out entirely
run ""
run "-DRC_VEC=0"
run "-DRC_FORCE_CARRY=1"
run "-DRC_RCNUM=8"
run "-DRC_RCNUM=32"
run "-DCORO_FAKE=1"
run "-DRC_LOWSPLIT=0"
run "-DRC_SCATTER=0"
run "-DRC_LOWBYTES=4"

# the knobs, at their non-default settings
run "-DRC_CHUNK=0"
run "-DRC_CHUNK=512"
run "-DRC_FUSE_PP_ENC=0 -DRC_FUSE_PP_DEC=0"
run "-DRC_FUSE_PP_ENC=1 -DRC_FUSE_PP_DEC=1"
run "-DRC_DEC_WAVE=1"
run "-DRC_DEC_WAVE=2"
run "-DRC_DEC_PUTW=1"
run "-DRC_DEC_ALIGN=0"
run "-DRC_DEC_COLD=1"
run "-DRC_LOAD32=1"
run "-DRC_LOAD32=2"
run "-DRC_FF_LANES=8"
run "-DRC_SWEEP_NEGIDX=1"
run "-DRC_FOLD_RPRE=1"
run "-DRC_SHIFT_SAT=1"
run "-DRC_DEC_RENORM=0"
run "-DRC_DEC_RENORM=1"
run "-DRC_DEC_RENORM=2"
run "-DRC_DEC_RENORM=3"
run "-DRC_DEC_RENORM=4"
run "-DRC_DEC_RENORM=5"
run "-DRC_DEC_RENORM=6"
run "-DRC_DEC_RENORM=7"
run "-DRC_DEC_RENORM=8"
run "-DRC_DEC_RENORM=9"
run "-DRC_ENC_NSEL=1"
run "-DRC_ENC_NSEL=2"
run "-DRC_SCATTER_SKIP=1"
run "-DRC_SCATTER_W=0"

# combinations that have bitten before: a knob that moves work between paths
run "-DRC_CHUNK=0 -DRC_VEC=0"
run "-DRC_CHUNK=512 -DRC_FORCE_CARRY=1"
run "-DRC_FF_LANES=8 -DRC_RCNUM=32"

[ "$fail" = 0 ] && echo "== all configurations pass" || { echo "== FAILURES above"; exit 1; }
