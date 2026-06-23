#!/usr/bin/env bash
#
# build.sh - fetch & build Zydis, then compile the mine32 frontend.
#
# Usage: ./build.sh
#
set -euo pipefail
cd "$(dirname "$0")"

ZYDIS_DIR="zydis"
ZYDIS_URL="https://github.com/zyantific/zydis.git"

if [ ! -d "$ZYDIS_DIR" ]; then
    echo ">> cloning Zydis"
    git clone --recursive "$ZYDIS_URL" "$ZYDIS_DIR"
fi

if [ ! -f "$ZYDIS_DIR/build/libZydis.a" ]; then
    echo ">> building Zydis (static)"
    cmake -S "$ZYDIS_DIR" -B "$ZYDIS_DIR/build" \
        -DCMAKE_BUILD_TYPE=Release \
        -DZYDIS_BUILD_EXAMPLES=OFF \
        -DZYDIS_BUILD_TOOLS=OFF \
        -DZYDIS_BUILD_SHARED_LIB=OFF
    cmake --build "$ZYDIS_DIR/build" -j"$(nproc)"
fi

INCS="-I $ZYDIS_DIR/include -I $ZYDIS_DIR/dependencies/zycore/include \
      -I $ZYDIS_DIR/build -I $ZYDIS_DIR/build/zycore"
LIBS="$ZYDIS_DIR/build/libZydis.a $ZYDIS_DIR/build/zycore/libZycore.a"

echo ">> compiling mine32"
cc -O3 -pthread $INCS mine32.c $LIBS -o mine32

echo ">> compiling apxdis"
cc -O2 $INCS apxdis.c $LIBS -o apxdis

echo ">> done: ./mine32 --help"
