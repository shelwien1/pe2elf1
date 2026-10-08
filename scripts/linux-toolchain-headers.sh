#!/bin/sh
# Lists the headers of the C and C++ libraries, of the compiler and of the kernel in the given
# system include directories, for the relocatable Linux build, which ships copies of them: the
# files in those directories of the packages that own <stdio.h>, <vector>, <stddef.h>,
# <linux/types.h> and <crypt.h> (dpkg: Debian, Ubuntu).  Without dpkg, every file in the
# directories is listed.
#
# usage: scripts/linux-toolchain-headers.sh DIR...
set -eu
canon() { (cd "$1" && pwd -P); }
dirs=
for d in "$@"; do
  [ -d "$d" ] && dirs="$dirs $(canon "$d")"
done
under_dirs() {
  for d in $dirs; do
    case "$1" in "$d"/*) return 0 ;; esac
  done
  return 1
}
if command -v dpkg > /dev/null 2>&1; then
  pkgs=
  for h in stdio.h vector stddef.h linux/types.h crypt.h; do
    for d in $dirs; do
      if [ -f "$d/$h" ]; then
        p=$(dpkg -S "$(readlink -f "$d/$h")" 2>/dev/null | sed -n '1s/:.*//p')
        [ -n "$p" ] && pkgs="$pkgs $p"
        break
      fi
    done
  done
  for p in $(printf '%s\n' $pkgs | sort -u); do dpkg -L "$p"; done | while IFS= read -r f; do
    if [ -f "$f" ] && under_dirs "$f"; then echo "$f"; fi
  done
else
  echo "linux-toolchain-headers.sh: dpkg not found, listing every file of the directories" >&2
  for d in $dirs; do find -L "$d" -type f; done
fi | sort -u
