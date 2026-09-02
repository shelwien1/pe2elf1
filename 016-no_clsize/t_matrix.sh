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
# Each row is a roundtrip plus a byte-identical re-encode.
set -e
cd "$(dirname "$0")"
: "${NITER:=1}"; export NITER
: "${TESTFILE:=../enwik8}"; export TESTFILE

fail=0

# runsame: the row must reproduce the reference stream as well as roundtrip.
# run:     the row legitimately changes the format (geometry), so roundtrip only.
REFSTREAM_FILE=$(pwd)/t_ref.enc; rm -f "$REFSTREAM_FILE"

_run() {
  want=$1; ref=$2; args=$3
  printf '  %-42s ' "${args:-<defaults>}"
  # shellcheck disable=SC2086
  if out=$(REFSTREAM="$ref" ./t.sh $args 2>&1); then
    ok=$(printf '%s' "$out" | grep -c 'ok: ')
    if [ "$ok" -ge "$want" ]; then echo "ok"; else echo "FAIL (checks missing)"; fail=1; fi
  else
    echo "FAIL"; fail=1
    printf '%s\n' "$out" | tail -4 | sed 's/^/      /'
  fi
}
runsame() { _run 2 "$REFSTREAM_FILE" "$1"; }
run()     { _run 1 "" "$1"; }

echo "== ${CXX:-clang++}, -march=${ARCH:-skylake}, $TESTFILE"

# geometry and the paths that move work between the vector and scalar sides
runsame ""
run "-DRC_RCNUM=32"
run "-DCORO_FAKE=1"
runsame "-DRC_SCATTER=0"
run "-DRC_CODBYTES=3 -DRC_LOWBYTES=7"
run "-DRC_CODBYTES=3"

# the knobs, at their non-default settings
runsame "-DRC_FOLD_RPRE=1"
runsame "-DRC_SCATTER_SKIP=1"
runsame "-DRC_SCATTER_W=0"
runsame "-DRC_ENC_NSEL=1"
runsame "-DRC_ENC_NSEL=2"
runsame "-DRC_ENC_RENORM=1"
runsame "-DRC_ENC_RENORM=2"
runsame "-DRC_ENC_RENORM=1 -DRC_ENC_NSEL=1"
runsame "-DRC_ENC_RENORM=2 -DRC_ENC_NSEL=2"
runsame "-DRC_FUSE_PP_ENC=0"
runsame "-DRC_FUSE_PP_ENC=1"

# the two defs profiles themselves
runsame "-DRC_FOLD_RPRE=1 -DRC_SCATTER_SKIP=0 -DRC_ENC_NSEL=0 -DRC_ENC_RENORM=0"
runsame "-DRC_FOLD_RPRE=0 -DRC_SCATTER_SKIP=1 -DRC_ENC_NSEL=1 -DRC_ENC_RENORM=2"

# the two defs profiles themselves

# combinations that have bitten before: a knob that moves work between paths
run "-DRC_RCNUM=32 -DRC_ENC_NSEL=1"
run "-DRC_CODBYTES=3 -DRC_LOWBYTES=7 -DRC_FOLD_RPRE=1"

# The decoder's vector renorm pass: decode side only, so the stream is the
# reference's. Every pass width, the scalar and 16-wide loop shapes, the
# gather variant with and without the zero-slot select, and the width that
# is one step at RCNUM=32.
runsame "-DRC_DEC_VRENORM=16"
runsame "-DRC_DEC_VRENORM=8"
runsame "-DRC_DEC_VRENORM=4"
runsame "-DRC_DEC_VRENORM=2"
runsame "-DRC_DEC_VRENORM=16 -DRC_DEC_VRENORM_VF=1"
runsame "-DRC_DEC_VRENORM=16 -DRC_DEC_VRENORM_VF=16"
runsame "-DRC_DEC_VRENORM=16 -DRC_DEC_VRENORM_GATHER=1"
runsame "-DRC_DEC_VRENORM=8 -DRC_DEC_VRENORM_GATHER=1 -DRC_DEC_ZSLOT=1"
run "-DRC_DEC_VRENORM=16 -DRC_RCNUM=32"
run "-DRC_DEC_VRENORM=4 -DRC_RCNUM=32"

# The header model's context depth. Changes the stream, so `run`, not
# `runsame` -- and the shallow end is what exercises the p=1/2 tail.
run "-DRC_HDR_CTXBITS=1"
run "-DRC_HDR_CTXBITS=16"
run "-DRC_HDR_CTXBITS=8 -DRC_RCNUM=32"

rm -f "$REFSTREAM_FILE"
[ "$fail" = 0 ] && echo "== all configurations pass" || { echo "== FAILURES above"; exit 1; }
