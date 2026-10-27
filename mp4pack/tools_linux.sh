#!/bin/sh
# The tools for the Linux mp4pack: tools_linux.sh PE2ELF_DIR TOOLS_DIR [OUT_DIR]
# Converts each <tool>.exe in TOOLS_DIR (all but mp4pack.exe) with pe2elf to
# OUT_DIR/<tool> (h264p_avx2.exe -> h264p_avx2; default OUT_DIR: here, next to
# the Linux mp4pack), and copies pe2elf's winapi_shim.so there: the tools load
# it from their own directory. PE2ELF_DIR is a built pe2elf (make in it).
pe=$1; in=$2; out=${3:-$(dirname "$0")}
if [ -z "$in" ]; then echo "usage: tools_linux.sh PE2ELF_DIR TOOLS_DIR [OUT_DIR]" >&2; exit 1; fi
if [ ! -x "$pe/pe2elf" ] || [ ! -f "$pe/winapi_shim.so" ]; then echo "no pe2elf or winapi_shim.so in $pe: build it there (make)" >&2; exit 1; fi
mkdir -p "$out" || exit 1
n=0
for f in "$in"/*.exe; do
  [ -f "$f" ] || continue
  b=$(basename "$f" .exe)
  [ "$b" = mp4pack ] && continue
  if ! "$pe/pe2elf" "$f" "$out/$b" > /dev/null; then echo "pe2elf failed on $f" >&2; exit 1; fi
  echo "$f -> $out/$b"
  n=$((n+1))
done
cp "$pe/winapi_shim.so" "$out/" || exit 1
echo "$n tools, and winapi_shim.so, in $out"
