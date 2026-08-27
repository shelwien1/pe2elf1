#!/bin/sh
# rc_kernel.c -> rc_kernel.cl -> rc_kernel.inc
#
# The perl scripts name their output after the input's extension, so the source
# being .c is what keeps the three names apart:
#
#   rc_kernel.c -> rc_kernel_macro.c -> rc_kernel_macro_D.c -> rc_kernel.cl
#
#   rc_macro.pl  turns each function into a #define / #enddef block
#   defines.pl   turns those into real multi-line macros (and emits the #undefs,
#                which a single-use kernel has no need of)
#   txt2inc.pl   wraps the result up as C string literals
#
# Both generated files are committed, so perl is only wanted by whoever edits
# the kernel. build.sh calls this when rc_kernel.c is newer.
set -e
cd "$(dirname "$0")"
perl rc_macro.pl rc_kernel.c
perl defines.pl  rc_kernel_macro.c
mv  rc_kernel_macro_D.c rc_kernel.cl
rm -f rc_kernel_macro.c rc_kernel_macro_U.c
perl txt2inc.pl -raw rc_kernel.cl rc_kernel.inc
echo "regenerated rc_kernel.cl and rc_kernel.inc from rc_kernel.c"
