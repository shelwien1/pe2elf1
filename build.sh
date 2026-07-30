#!/bin/bash
# build.sh <tag> [defines/flags...]   -> $S/p_<tag>
S=${S:-/tmp/claude-0/-home-user-pe2elf1/5e5d7486-4d33-5d0b-9606-ca56f9d55bc1/scratchpad}
tag=${1:?usage: build.sh <tag> [flags...]}; shift
g++ -O3 -march=native "$@" -o "$S/p_$tag" paq8hpc_speed.cpp 2>&1 | grep -Ev 'warning: too many arguments|^ *[0-9]+ \||^ *\^|^ *\|'
[ -x "$S/p_$tag" ] || { echo "BUILD FAILED: $tag"; exit 1; }
