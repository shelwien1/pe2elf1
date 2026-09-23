#!/bin/sh
# gtm.sh -- build and run gradtest_mix.cpp: finite-difference check of the
# end-to-end gradients of p = SSE(mix2(C0, C1)) (the chain factors main() hands
# back through the stages, and what the mixer's and the counters' ParamUpdaters get).
#
# The check runs on a copy of the tree in a temporary directory whose
# schrau.inc keeps the names but does exact libm math: the Schraudolph
# approximations are piecewise linear, so their finite differences would not
# match the analytic chain to better than a few percent.  Uses the shipping
# MOD/ (run ./gc.sh first); touches the C0 table, so it needs its memory.
cd "$(dirname "$0")"
T=$(mktemp -d "${TMPDIR:-/tmp}/gtm.XXXXXX") || exit 1
trap 'rm -rf "$T"' EXIT
cp -a coder0.cpp config.hpp config_mix2.hpp sh_*.inc gradtest_mix.cpp MOD book1 "$T"/
cat > "$T/schrau.inc" <<'INC'
inline float logf_schraudolph(float x) { return ::logf(x); }
inline float expf_schraudolph(float x) { return ::expf(x); }
inline float sq_schraudolph(float x)   { return 1.0f/(1.0f+::expf(-x)); }
inline float st_schraudolph(float p)   { return ::logf(p/(1.0f-p)); }
inline float tanhf_schraudolph(float x){ return ::tanhf(x); }
#define SCHRAUDOLPH 1
#define logf(x) logf_schraudolph(x)
#define expf(x) expf_schraudolph(x)
#define sq(x)   sq_schraudolph(x)
#define st(x)   st_schraudolph(x)
#define tanhf(x) tanhf_schraudolph(x)
INC
${CXX:-g++} -std=c++23 -O2 -march=${TARG:-haswell} -DGRAD_TEST -D_FILE_OFFSET_BITS=64 -w -I"$T" \
  "$T/gradtest_mix.cpp" -o "$T/gradtest_mix" && ( cd "$T" && ./gradtest_mix )
