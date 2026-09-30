#!/bin/sh
# builds the tfwz codec (any C++17 compiler)
cd "$(dirname "$0")" && ${CXX:-g++} -std=c++17 -O2 tfwz.cpp -o tfwz
