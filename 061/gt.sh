#!/bin/sh
# gt.sh -- build and run gradtest.cpp: numerical check of the gradient
# derivation in Counter::C_Update, Mix2::Update and the end-to-end chain.
cd "$(dirname "$0")"
g++ -std=c++23 -DNDEBUG -O1 -march=haswell -ffp-contract=off -fno-builtin-logf -fno-builtin-expf -w -I. gradtest.cpp -o gradtest && ./gradtest
