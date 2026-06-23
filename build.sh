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

# ---- Capstone variant (mine32cs) ----------------------------------------
CS_DIR="capstone"
if [ ! -d "$CS_DIR" ]; then
    echo ">> cloning Capstone"
    git clone --depth 1 https://github.com/capstone-engine/capstone.git "$CS_DIR"
fi
if [ ! -f "$CS_DIR/build/libcapstone.a" ]; then
    echo ">> building Capstone (x86, static)"
    cmake -S "$CS_DIR" -B "$CS_DIR/build" -DCMAKE_BUILD_TYPE=Release \
        -DCAPSTONE_ARCHITECTURE_DEFAULT=OFF -DCAPSTONE_X86_SUPPORT=ON \
        -DCAPSTONE_BUILD_SHARED_LIBS=OFF -DCAPSTONE_BUILD_CSTOOL=OFF \
        -DCAPSTONE_BUILD_TESTS=OFF -DCAPSTONE_BUILD_CSTEST=OFF
    cmake --build "$CS_DIR/build" -j"$(nproc)"
fi
echo ">> compiling mine32cs"
cc -O3 -pthread -I "$CS_DIR/include" mine32cs.c "$CS_DIR/build/libcapstone.a" -o mine32cs

echo ">> done: ./mine32 --help  /  ./mine32cs --help"
