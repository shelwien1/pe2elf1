#!/bin/sh
# rc.inc -> rc_kernel0.c -> rc_kernel1.c -> rc_vecD.inc
#
#   $CPP -E      resolves the #if forest against the build's configuration
#                (RC_VECOUT=1 picks the vector-shape coder bodies)
#   rc_soa.pl    lane state to [RCNUM] arrays, rcidx threaded through
#   rc_macro.pl  functions into #define/#enddef blocks
#   defines.pl   ... into real multi-line macros
#
# The output depends on the RC_* configuration -- and on the target, since
# RC_SCATTER keys off __AVX512F__ -- so build.sh runs this every build with the
# current -D set and CPP="$CXX $archflags -E"; nothing here is committed.
# Running it by hand without those flags produces a kernel the compile will
# reject (model0.inc static_asserts on RC_SCATTER).
#   mk_kernel.sh [-Dname=value ...]
set -e
cd "$(dirname "$0")"
CPP="${CPP:-cc -E}"

$CPP -P -x c++ -DRC_VECOUT=1 -DRC_CARRYLESS=1 "$@"   -imacros rc_config.inc rc.inc > rc_kernel0.c

perl ../Lib3/rc_soa.pl rc_kernel0.c
perl ../Lib3/rc_macro.pl rc_kernel1.c
perl ../Lib3/defines.pl  rc_kernel1_macro.c
mv rc_kernel1_macro_D.c rc_vecD.inc
rm -f rc_kernel1_macro.c rc_kernel1_macro_U.c

echo "generated rc_vecD.inc from rc.inc"
