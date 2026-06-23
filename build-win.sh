#!/usr/bin/env bash
#
# build-win.sh - cross-compile a standalone Windows .exe of the miner with MinGW.
#
# Produces mine32.exe (statically linked: depends only on KERNEL32.dll and
# msvcrt.dll). Requires the mingw-w64 cross toolchain, e.g. on Debian/Ubuntu:
#     sudo apt-get install mingw-w64
#
# Usage:
#     ./build-win.sh            # 64-bit (x86_64-w64-mingw32)
#     ARCH=i686 ./build-win.sh  # 32-bit (i686-w64-mingw32)
#
set -euo pipefail
cd "$(dirname "$0")"

ARCH="${ARCH:-x86_64}"
TRIPLE="${ARCH}-w64-mingw32"

# Prefer the POSIX-threads variant (needed for pthreads / <stdatomic.h>).
if command -v "${TRIPLE}-gcc-posix" >/dev/null 2>&1; then
    CC="${TRIPLE}-gcc-posix"
elif command -v "${TRIPLE}-gcc" >/dev/null 2>&1; then
    CC="${TRIPLE}-gcc"
else
    echo "error: ${TRIPLE}-gcc not found (install mingw-w64)" >&2
    exit 1
fi
WINDRES="${TRIPLE}-windres"

ZYDIS_DIR="zydis"
BUILD="build-win-${ARCH}"

if [ ! -d "$ZYDIS_DIR" ]; then
    echo ">> cloning Zydis"
    git clone --recursive https://github.com/zyantific/zydis.git "$ZYDIS_DIR"
fi

if [ ! -f "$ZYDIS_DIR/$BUILD/libZydis.a" ]; then
    echo ">> cross-building Zydis for Windows ($TRIPLE)"
    cmake -S "$ZYDIS_DIR" -B "$ZYDIS_DIR/$BUILD" \
        -DCMAKE_SYSTEM_NAME=Windows \
        -DCMAKE_C_COMPILER="$CC" \
        -DCMAKE_RC_COMPILER="$WINDRES" \
        -DCMAKE_BUILD_TYPE=Release \
        -DZYDIS_BUILD_EXAMPLES=OFF \
        -DZYDIS_BUILD_TOOLS=OFF \
        -DZYDIS_BUILD_SHARED_LIB=OFF
    cmake --build "$ZYDIS_DIR/$BUILD" -j"$(nproc)"
fi

echo ">> compiling mine32.exe with $CC"
# ZYDIS_STATIC_BUILD / ZYCORE_STATIC_BUILD stop the headers from marking the
# API as __declspec(dllimport); -static bundles libgcc and winpthreads.
"$CC" -O3 -pthread -static \
    -DZYDIS_STATIC_BUILD -DZYCORE_STATIC_BUILD \
    -I "$ZYDIS_DIR/include" \
    -I "$ZYDIS_DIR/dependencies/zycore/include" \
    -I "$ZYDIS_DIR/$BUILD" \
    -I "$ZYDIS_DIR/$BUILD/zycore" \
    mine32.c \
    "$ZYDIS_DIR/$BUILD/libZydis.a" \
    "$ZYDIS_DIR/$BUILD/zycore/libZycore.a" \
    -o mine32.exe

echo ">> done: mine32.exe"
file mine32.exe || true
