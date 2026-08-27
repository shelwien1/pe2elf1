#!/bin/sh
# rc_kernel.c -> rc_kernel.ispc
#
# The perl scripts name their output after the input's extension, so the source
# being .c is what keeps the names apart:
#
#   rc_kernel.c -> rc_kernel_macro.c -> rc_kernel_macro_D.c -> rc_kernel.ispc
#
#   rc_macro.pl  turns each function into a #define / #enddef block
#   defines.pl   turns those into real multi-line macros (and emits the #undefs,
#                which a single-use kernel has no need of)
#
# The generated file is committed, so perl is only wanted by whoever edits the
# kernel; ispc compiles it at build time (build.sh, gc.bat). build.sh calls
# this when rc_kernel.c is newer.
set -e
cd "$(dirname "$0")"
perl rc_macro.pl rc_kernel.c
perl defines.pl  rc_kernel_macro.c
mv  rc_kernel_macro_D.c rc_kernel.ispc
rm -f rc_kernel_macro.c rc_kernel_macro_U.c

echo "regenerated rc_kernel.ispc from rc_kernel.c"
