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

echo ">> compiling mine32"
cc -O3 -pthread \
    -I "$ZYDIS_DIR/include" \
    -I "$ZYDIS_DIR/dependencies/zycore/include" \
    -I "$ZYDIS_DIR/build" \
    -I "$ZYDIS_DIR/build/zycore" \
    mine32.c \
    "$ZYDIS_DIR/build/libZydis.a" \
    "$ZYDIS_DIR/build/zycore/libZycore.a" \
    -o mine32

echo ">> done: ./mine32 --help"
