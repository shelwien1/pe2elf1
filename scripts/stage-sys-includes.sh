#!/bin/sh
# Maps the system include directories of the backend compilers to directories under a
# staging directory, for a relocatable ROSE (the Windows build) that ships the headers
# it parses with.
#
# usage: scripts/stage-sys-includes.sh [--copy] DEST CXX-DIR... [-- C-DIR...]
#
# Each directory that is not inside another of the given directories becomes DEST/sysN
# (N counting from 0 in order of appearance); a directory inside another one keeps its
# place in that directory's copy.  The names relative to DEST are printed in the order
# of the arguments as a C initializer list ("sys0/c++", "sys0", ...), one line for the
# C++ and one for the C directories.  With --copy, the directory trees are also copied
# into DEST (following symbolic links).
set -eu
copy=false
if [ "$1" = "--copy" ]; then copy=true; shift; fi
dest=$1
shift

canon() { (cd "$1" && pwd -P); }
IFS='
'

# All canonical directories, one per line
dirs=
for d in "$@"; do
  [ "$d" = "--" ] || dirs="$dirs$(canon "$d")
"
done

# The outermost given directory containing $1 (or $1 itself)
root_of() {
  r=$1
  for o in $dirs; do
    case "$1" in "$o"/*) [ ${#o} -lt ${#r} ] && r=$o ;; esac
  done
  echo "$r"
}

roots=
list=
for arg in "$@"; do
  if [ "$arg" = "--" ]; then
    printf '%s\n' "${list%, }"
    list=
    continue
  fi
  d=$(canon "$arg")
  r=$(root_of "$d")
  n=0
  found=
  for o in $roots; do
    [ "$o" = "$r" ] && found=$n
    n=$((n + 1))
  done
  if [ -z "$found" ]; then
    found=$n
    roots="$roots$r
"
    if $copy; then
      rm -rf "$dest/sys$found"
      mkdir -p "$dest/sys$found"
      cp -RL "$r/." "$dest/sys$found/"
    fi
  fi
  list="$list\"sys$found${d#"$r"}\", "
done
printf '%s\n' "${list%, }"
