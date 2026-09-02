#!/bin/sh
# rc.inc -> rc_kernel0.c -> rc_kernel1.c -> rc_vecD.inc
#
#   $CPP -E      resolves the #if forest against the build's configuration
#                (RC_VECOUT=1 picks the vector-shape coder bodies)
#   rc_soa.pl    lane state to [RCNUM] arrays, rcidx threaded through
#   rc_macro.pl  functions into #define/#enddef blocks
#   defines.pl   ... into real multi-line macros
#
# The output depends on the RC_* configuration and on the TARGET: rc.inc
# chooses the scatter that commits the staged store and the decoder's vector
# pass on __AVX512F__ and friends, so $CPP has to be the compile's compiler
# with the compile's -march -- build.sh and gc.bat pass it -- and
# RC_KERNEL_CONF (rc_config.inc) fingerprints the result, so a kernel
# generated for one target and compiled for another is a build error.
# Nothing here is committed.
#   CPP="clang++ -E -march=native" mk_kernel.sh [-Dname=value ...]
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

# Lib3 is shared and syncs separately from this directory, and a stale
# rc_soa.pl produces a kernel that COMPILES -- it just hardcodes
# ALIGN(VECSIZE) and ignores SOA_FOLD, so the alignment and fold settings
# silently do nothing. Fail here instead, where the cause is obvious.
for feat in RC_KALIGN RC_FOLD; do
  grep -q "$feat" rc_vecD.inc || {
    echo "mk_kernel.sh: ../Lib3/rc_soa.pl is out of date -- the generated" >&2
    echo "  kernel has no $feat. Sync Lib3 with this directory; an old" >&2
    echo "  generator builds fine but ignores the alignment and fold settings." >&2
    exit 1
  }
done

# Same for rc_macro.pl: a stale one emits macro bodies that do not parenthesise
# their parameters, so the body's precedence applies to the caller's
# expression. ShiftLow's `n*8` turns `ShiftLow( sh>>3 )` into `sh>>3*8`. That
# compiles, encodes, and produces a stream that does not decode.
grep -q '\[(rcidx)\]' rc_vecD.inc || {
  echo "mk_kernel.sh: ../Lib3/rc_macro.pl is out of date -- the generated" >&2
  echo "  kernel does not parenthesise macro parameters. Sync Lib3 with this" >&2
  echo "  directory; an old generator builds fine and miscodes any call that" >&2
  echo "  passes an expression (RC_ENC_NSEL=1, say)." >&2
  exit 1
}

# A #pragma line inside a macro body is a compile error, and it is what a
# stale rc_macro.pl leaves of the loop pragmas rc.inc writes as _Pragma: the
# preprocessor prints those as #pragma lines, and the generator has to turn
# them back.
grep -q '^[[:space:]]*#[[:space:]]*pragma' rc_vecD.inc && {
  echo "mk_kernel.sh: ../Lib3/rc_macro.pl is out of date -- the generated" >&2
  echo "  kernel has a #pragma line inside a macro body. Sync Lib3 with this" >&2
  echo "  directory; a current generator emits it as a _Pragma." >&2
  exit 1
}

# And the third way a stale rc_soa.pl produces a kernel that compiles wrongly
# or not at all: threading rcidx into a call that already names its lane
# (`rc_Refill( rcidx+i )` in rc.inc's vector pass), which a lookahead that
# came after the \s* let through. Only present when the pass is generated.
grep -q '( *(rcidx), *(rcidx)' rc_vecD.inc && {
  echo "mk_kernel.sh: ../Lib3/rc_soa.pl is out of date -- the generated" >&2
  echo "  kernel threads rcidx into a call that already passes it. Sync Lib3" >&2
  echo "  with this directory." >&2
  exit 1
}

echo "generated rc_vecD.inc from rc.inc"
