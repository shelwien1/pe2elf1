#!/bin/sh
# rc.inc -> rc_kernel0.c -> rc_kernel1.c -> rc_vecD.inc
#
#   $CPP -E      resolves the #if forest against the build's configuration
#                (RC_VECOUT=1 picks the vector-shape coder bodies)
#   rc_soa.pl    lane state to [RCNUM] arrays, rcidx threaded through
#   rc_macro.pl  functions into #define/#enddef blocks
#   defines.pl   ... into real multi-line macros
#
# The output depends on the RC_* configuration and on NOTHING ELSE -- not on
# the target, not on which compiler preprocesses it. Anything the compile has
# to decide for itself (whether the store is staged, and what commits it) is
# left as a macro for the compile to define; see model.inc. So build.sh and
# gc.bat can run this with whatever preprocessor is at hand, as long as the -D
# set matches. Nothing here is committed.
#   mk_kernel.sh [-Dname=value ...]
set -e
cd "$(dirname "$0")"
CPP="${CPP:-cc -E}"

# A Windows compiler path arrives with backslashes -- gc.bat's %gcc% is
# C:\clangN10x\bin\clang++.exe -- and to sh a word with no forward slash in it
# is a command NAME, looked up in PATH, not a path. It never resolves:
#
#   mk_kernel.sh: line 21: C:\clangN10x\bin\clang++.exe: command not found
#
# Backslashes are a legal separator for the Windows exec either way, so swap
# them and the same string becomes a path sh will run. Harmless everywhere
# else: a backslash in a compiler path is not a thing on POSIX.
case "$CPP" in
  *\\*) CPP=`printf '%s' "$CPP" | tr '\\' '/'` ;;
esac

# $CPP is deliberately unquoted below, so it can carry flags -- which means its
# words are split on whitespace and cannot be shell-quoted. A path with spaces
# in it has to reach us already escaped, or as a short name.
$CPP -P -x c++ -DRC_VECOUT=1 -DRC_CARRYLESS=1 "$@"   -imacros rc_config.inc rc.inc > rc_kernel0.c

perl ../Lib3/rc_soa.pl rc_kernel0.c
perl ../Lib3/rc_macro.pl rc_kernel1.c
perl ../Lib3/defines.pl  rc_kernel1_macro.c
mv rc_kernel1_macro_D.c rc_vecD.inc
rm -f rc_kernel1_macro.c rc_kernel1_macro_U.c

echo "generated rc_vecD.inc from rc.inc"
