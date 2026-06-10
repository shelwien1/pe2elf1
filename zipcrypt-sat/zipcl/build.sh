#!/bin/sh
# Build zipcl on Linux (needs g++). Extra args are passed through to g++.
cd "$(dirname "$0")" || exit 1
exec g++ -O2 -std=gnu++17 -DNDEBUG -DSTRICT -ILib3 -fpermissive zipcl.cpp -o zipcl "$@"
