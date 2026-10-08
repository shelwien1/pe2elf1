#!/bin/sh
# Builds the Boost libraries that ROSE uses as static libraries with position-independent code,
# for the relocatable Linux build of ROSE, which links them into librose.so (see "Linux release"
# in README.md).
#
# usage: scripts/build-boost-linux.sh <boost source dir> <install prefix> [compiler]
#
# <boost source dir> is an unpacked Boost release (tested with 1.83.0, e.g. from
# https://archives.boost.io/release/1.83.0/source/boost_1_83_0.tar.bz2); the compiler defaults
# to g++.  iostreams uses the system's zlib (Debian/Ubuntu: zlib1g-dev).  Then build ROSE with
# BOOST_ROOT=<install prefix>.
set -eu
src=$(cd "$1" && pwd)
prefix=$2
cxx=${3:-g++}
mkdir -p "$prefix"
prefix=$(cd "$prefix" && pwd)

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT INT TERM
cat > "$work/user-config.jam" <<END
using gcc : pic : $cxx ;
END

cd "$src"
[ -x ./b2 ] || ./bootstrap.sh --with-toolset=gcc
./b2 --user-config="$work/user-config.jam" --build-dir="$work/build" --prefix="$prefix" \
  --layout=system -j"$(nproc 2>/dev/null || echo 2)" \
  toolset=gcc-pic variant=release link=static runtime-link=shared threading=multi \
  cflags=-fPIC cxxflags=-fPIC \
  --with-date_time --with-filesystem --with-iostreams --with-program_options --with-random \
  --with-regex --with-system --with-wave --with-chrono --with-thread --with-atomic \
  --with-serialization \
  -sNO_BZIP2=1 -sNO_LZMA=1 -sNO_ZSTD=1 install
