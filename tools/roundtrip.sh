#!/bin/sh
# roundtrip.sh -- verify the corpus64.p (dis)assembler round-trips byte-exactly.
#
# For each (DESC, ASM, BIN) triple it checks both directions:
#   parsergen.py DESC BIN  == ASM      (disassembly)
#   asm.py       DESC ASM  == BIN      (assembly)
# Exits non-zero on the first mismatch.
set -e
cd "$(dirname "$0")/.."
fail=0

check() {  # desc asm bin label
  desc=$1; asm=$2; bin=$3; label=$4
  if python3 parsergen.py "$desc" "$bin" 2>/dev/null | diff -q - "$asm" >/dev/null; then
    d=OK
  else d=DIFF; fail=1; fi
  if python3 asm.py "$desc" "$asm" 2>/dev/null | cmp -s - "$bin"; then
    a=OK
  else a=DIFF; fail=1; fi
  printf "%-28s  disasm=%-4s  asm=%-4s\n" "$label" "$d" "$a"
}

# corpus.asm has comments/blank lines; compare against a stripped copy.
grep -vE '^;|^$' corpus.asm > /tmp/_corpus_strip.asm 2>/dev/null || cp corpus.asm /tmp/_corpus_strip.asm
check corpus.p   /tmp/_corpus_strip.asm corpus.bin "corpus.p (x86-32)"
check corpus64.p corpus64.asm          corpus64.bin "corpus64.p (x86-64)"
check corpus64.p tests/prog.asm        tests/prog.bin "corpus64.p (compiled C)"
check corpus64.p evex64.asm            evex64.bin   "corpus64.p (AVX-512 EVEX)"
check corpus64.p xop64.asm             xop64.bin    "corpus64.p (XOP)"

if [ "$fail" = 0 ]; then echo "ALL ROUND-TRIPS BYTE-EXACT"; else echo "FAILURES PRESENT"; fi
exit $fail
