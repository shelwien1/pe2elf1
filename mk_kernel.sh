#!/bin/sh
# rc_kernel0.cl -> rc_kernel.cl -> rc_kernel.inc
#
#   rc_macro.pl  turns each function into a #define / #enddef block
#   defines.pl   turns those into real multi-line macros (and emits the #undefs,
#                which a single-use kernel has no need of)
#   txt2inc.pl   wraps the result up as C string literals
#
# Both generated files are committed, so perl is only wanted by whoever edits
# the kernel. build.sh calls this when rc_kernel0.cl is newer.
set -e
cd "$(dirname "$0")"
perl rc_macro.pl rc_kernel0.cl
perl defines.pl  rc_kernel0_macro.cl
mv  rc_kernel0_macro_D.cl rc_kernel.cl
rm -f rc_kernel0_macro.cl rc_kernel0_macro_U.cl
perl txt2inc.pl -raw rc_kernel.cl rc_kernel.inc
echo "regenerated rc_kernel.cl and rc_kernel.inc from rc_kernel0.cl"
