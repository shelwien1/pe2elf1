#!/bin/sh
# Cross-compiles the Boost libraries that ROSE uses for Windows (static libraries, for the
# MinGW-w64 build of ROSE; see "Windows" in README.md).
#
# usage: scripts/build-boost-mingw.sh <boost source dir> <install prefix> [compiler]
#
# <boost source dir> is an unpacked Boost release (tested with 1.83.0, e.g. from
# https://archives.boost.io/release/1.83.0/source/boost_1_83_0.tar.bz2); the compiler
# defaults to x86_64-w64-mingw32-g++-posix.  zlib for MinGW-w64 is needed by iostreams
# (Debian/Ubuntu: libz-mingw-w64-dev).  Then build ROSE with BOOST_ROOT=<install prefix>.
set -eu
src=$(cd "$1" && pwd)
prefix=$2
cxx=${3:-x86_64-w64-mingw32-g++-posix}
mkdir -p "$prefix"
prefix=$(cd "$prefix" && pwd)
target=$($cxx -dumpmachine)
sysroot=/usr/$target

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT INT TERM
cat > "$work/user-config.jam" <<EOF
using gcc : mingw : $cxx : <archiver>$target-ar <ranlib>$target-ranlib <rc>$target-windres ;
EOF

cd "$src"
[ -x ./b2 ] || ./bootstrap.sh --with-toolset=gcc
./b2 --user-config="$work/user-config.jam" --build-dir="$work/build" --prefix="$prefix" \
  --layout=system -j"$(nproc 2>/dev/null || echo 2)" \
  toolset=gcc-mingw target-os=windows address-model=64 architecture=x86 threadapi=win32 \
  variant=release link=static runtime-link=static threading=multi \
  --with-date_time --with-filesystem --with-iostreams --with-program_options --with-random \
  --with-regex --with-system --with-wave --with-chrono --with-thread --with-atomic \
  --with-serialization \
  -sZLIB_INCLUDE="$sysroot/include" -sZLIB_LIBPATH="$sysroot/lib" -sNO_BZIP2=1 -sNO_LZMA=1 \
  -sNO_ZSTD=1 install
