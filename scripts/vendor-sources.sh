#!/bin/sh
# Regenerates rose/ and edg/ -- the subsets of the ROSE and EDG source trees
# that the Makefile uses -- from full checkouts, using the compiler dependency
# files of a complete build made from those checkouts:
#
#   make ROSE_SRC=/path/to/rose EDG_SRC=/path/to/edg B=/tmp/build-full -j8
#   scripts/vendor-sources.sh /path/to/rose /path/to/edg /tmp/build-full
#
# Besides the sources and headers seen by the compiler, the subsets contain the
# inputs of the generators (ROSETTA grammar, flex/bison sources, EDG error
# message tables, predefined-macro script), version files and licenses.
set -eu
[ $# -eq 3 ] || { echo "usage: $0 <rose-checkout> <edg-checkout> <build-dir>" >&2; exit 2; }
rose=$(cd "$1" && pwd)
edg=$(cd "$2" && pwd)
build=$3
top=$(cd "$(dirname "$0")/.." && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# Every file the compiler read
find "$build/obj" -name '*.d' -exec cat {} + | tr ' \\' '\n\n' | grep -v ':$' | grep -v '^$' |
  while read -r f; do readlink -f "$f"; done | sort -u > "$tmp/deps"

# mk_errinfo is built without dependency tracking
g++ -x c++ -std=c++14 -w -I"$edg/src" -MM "$edg/util/mk_errinfo.c" | tr ' \\' '\n\n' |
  grep -v ':$' | grep -v '^$' | while read -r f; do readlink -f "$edg/$f" 2>/dev/null || readlink -f "$f"; done >> "$tmp/deps"

{
  sed -n "s|^$rose/||p" "$tmp/deps"
  printf '%s\n' ROSE_VERSION config/SCM_DATE LICENSE NOTICE LicenseInformation/ROSE_BSD_License.txt \
    src/ROSETTA/astNodeList src/Rose/CommandLine/LicenseString.pre \
    src/frontend/SageIII/preproc-c.ll src/frontend/SageIII/omplexer.ll src/frontend/SageIII/ompparser.yy
  (cd "$rose" && find src/ROSETTA/Grammar -type f)
} | sort -u > "$tmp/rose.txt"
{
  sed -n "s|^$edg/||p" "$tmp/deps"
  printf '%s\n' LICENSE.txt README.md util/mk_errinfo.c util/make_predef_macro_table \
    src/error_msg.txt src/error_tag.txt src/defines_linux.h
} | sort -u > "$tmp/edg.txt"

rm -rf "$top/rose" "$top/edg"
mkdir -p "$top/rose" "$top/edg"
(cd "$rose" && tar cf - -T "$tmp/rose.txt") | (cd "$top/rose" && tar xf -)
(cd "$edg" && tar cf - -T "$tmp/edg.txt") | (cd "$top/edg" && tar xf -)
echo "rose/: $(wc -l < "$tmp/rose.txt") files from $rose ($(git -C "$rose" rev-parse HEAD 2>/dev/null || echo unknown))"
echo "edg/:  $(wc -l < "$tmp/edg.txt") files from $edg ($(git -C "$edg" rev-parse HEAD 2>/dev/null || echo unknown))"
