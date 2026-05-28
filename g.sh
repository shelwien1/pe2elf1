#!/bin/sh
# Build pngtool in one shot from list.txt (no make, no libpreflate.a).
cd "$(dirname "$0")"
g++ @list.txt -o pngtool
