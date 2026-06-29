# x86-32 corpus-driven (dis)assembler -- build
#
#   gen_rules.py --mkcorpus.py--> corpus.p   (instruction rule body)
#   corpus.p --gen.py--> x86_tables.h        (decode FSM:  bytes -> x86insn_t)
#   corpus.p --gasm.py--> x86_tables_enc.h    (encode:      x86insn_t -> bytes)
#   asm32.cpp is the file-to-file tool; parser.cpp is the demo + self-test.
#
# make            build the demo, the test, and asm32
# make check      run gen.py's self-check (decode tables + FSM sim)
# make corpus     re-assemble corpus.p from gen_rules.py
# make regen      regenerate corpus.p and both headers
# make roundtrip  decode+re-encode corpus.bin, assert byte-identical (858/858)
# make noncanon   round-trip the curated non-canonical encodings

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2

all: parser test asm32

# corpus.p's instruction body is generated; the prelude/main frame are hand-kept
corpus: gen_rules.py mkcorpus.py
	python3 mkcorpus.py

x86_tables.h: corpus.p gen.py
	python3 gen.py

x86_tables_enc.h: corpus.p gasm.py gen.py x86_tables.h
	python3 gasm.py

regen:
	python3 mkcorpus.py
	python3 gen.py
	python3 gasm.py

check: corpus.p gen.py
	python3 gen.py --check

DECHDRS = x86_tables.h x86_tables_enc.h x86insn.hpp x86enc.hpp x86dec.hpp

parser: parser.cpp $(DECHDRS)
	$(CXX) $(CXXFLAGS) -I. -o $@ parser.cpp

test: test.cpp parser.cpp $(DECHDRS)
	$(CXX) $(CXXFLAGS) -I. -o $@ test.cpp

asm32: asm32.cpp $(DECHDRS)
	$(CXX) $(CXXFLAGS) -I. -o $@ asm32.cpp

run: parser
	./parser

runtest: test
	./test

# decode corpus.bin to records, re-encode, and assert the whole file is identical
roundtrip: asm32 corpus.bin
	./asm32 d corpus.bin insns.dat
	./asm32 c insns.dat out.bin
	@cmp out.bin corpus.bin && echo "roundtrip: 858/858 byte-identical"

# build and round-trip the curated non-canonical encodings
noncanon: asm32
	python3 make_noncanon.py
	./asm32 d noncanon.bin nc.dat
	./asm32 c nc.dat nc_out.bin
	@cmp noncanon.bin nc_out.bin && echo "noncanon: byte-identical round-trip"


# bijection fuzzer: every accepted decode must re-encode to the same bytes
fuzz: fuzz.cpp $(DECHDRS)
	$(CXX) $(CXXFLAGS) -I. -o fuzz fuzz.cpp
	./fuzz 5000000

# ---- x86-64 (long mode) -----------------------------------------------------
# Same generators + same C++ (mode-gated via ARCH_MODE), driven by corpus64.p.
# The C++ #includes "x86_tables.h" (resolved in its own directory first), so the
# 64-bit build generates the 64-bit tables into ./x86_tables.h, compiles
# asm32_64 against them, then restores the 32-bit tables so plain `make` is
# unaffected. asm32_64 carries its tables compiled-in, so it stays 64-bit.
asm64 asm32_64: corpus64.p gen.py gasm.py asm32.cpp x86insn.hpp x86dec.hpp x86enc.hpp
	python3 gen.py corpus64.p x86_tables.h
	python3 gasm.py corpus64.p x86_tables_enc.h
	$(CXX) $(CXXFLAGS) -I. -o asm32_64 asm32.cpp
	python3 gen.py corpus.p x86_tables.h         # restore 32-bit tables (default)
	python3 gasm.py corpus.p x86_tables_enc.h

check64: corpus64.p gen.py
	python3 gen.py corpus64.p /dev/null

# decode + re-encode the x86-64 corpus, assert byte-identical
roundtrip64: asm32_64 corpus64.bin
	./asm32_64 d corpus64.bin insns64.dat
	./asm32_64 c insns64.dat out64.bin
	@cmp out64.bin corpus64.bin && echo "roundtrip64: byte-identical"

# bijection fuzz against the x86-64 tables: every accepted decode (legacy and
# VEX/EVEX/XOP alike) must re-encode to the same bytes -- both failure counts must
# be 0. Builds fuzz64 against 64-bit tables, then restores the 32-bit default; the
# fuzz64 binary carries its tables compiled-in.
fuzz64: fuzz.cpp $(DECHDRS) corpus64.p gen.py gasm.py
	python3 gen.py corpus64.p x86_tables.h
	python3 gasm.py corpus64.p x86_tables_enc.h
	$(CXX) $(CXXFLAGS) -I. -o fuzz64 fuzz.cpp
	python3 gen.py corpus.p x86_tables.h         # restore 32-bit tables (default)
	python3 gasm.py corpus.p x86_tables_enc.h
	./fuzz64 5000000

# round-trip the .text of a gcc -O2 object (integer C: full coverage)
prog64: asm32_64 prog64.c
	gcc -O2 -fno-stack-protector -fcf-protection=none -c prog64.c -o prog64.o
	objcopy -O binary --only-section=.text prog64.o prog64.bin
	./asm32_64 d prog64.bin prog64.dat
	./asm32_64 c prog64.dat prog64.out
	@cmp prog64.bin prog64.out && echo "prog64: byte-identical"

# integer-only program exercising the extended ISA (cmpxchg/xadd atomics, bsf/bsr/
# tzcnt/lzcnt/popcnt, bswap, endbr64 via CET). -mgeneral-regs-only keeps SSE/AVX
# out of .text so every byte is in the integer subset the 64-bit corpus covers.
prog64b: asm32_64 prog64b.c
	gcc -O2 -mgeneral-regs-only -mpopcnt -mbmi -mlzcnt -fcf-protection=full -fno-stack-protector -c prog64b.c -o prog64b.o
	objcopy -O binary --only-section=.text prog64b.o prog64b.bin
	./asm32_64 d prog64b.bin prog64b.dat
	./asm32_64 c prog64b.dat prog64b.out
	@cmp prog64b.bin prog64b.out && echo "prog64b: byte-identical"

# floating-point program: gcc -O2 lowers it to SSE2 (movsd/addsd/mulsd/divsd/
# sqrtsd/comisd/cvtsi2sd/cvttsd2si/pxor), validating the SSE subset on real code.
prog64c: asm32_64 prog64c.c
	gcc -O2 -fcf-protection=full -fno-stack-protector -c prog64c.c -o prog64c.o
	objcopy -O binary --only-section=.text prog64c.o prog64c.bin
	./asm32_64 d prog64c.bin prog64c.dat
	./asm32_64 c prog64c.dat prog64c.out
	@cmp prog64c.bin prog64c.out && echo "prog64c: byte-identical"

# integer array kernels auto-vectorized by gcc -O3 to baseline SSE2 packed ops
# (movdqa/u, paddd/psubd, pmullw, pand/pxor, punpck, shifts, pshufd) -- exercises
# the SSE2 packed-integer set on real vectorized output.
prog64d: asm32_64 prog64d.c
	gcc -O3 -fno-stack-protector -fcf-protection=full -c prog64d.c -o prog64d.o
	objcopy -O binary --only-section=.text prog64d.o prog64d.bin
	./asm32_64 d prog64d.bin prog64d.dat
	./asm32_64 c prog64d.dat prog64d.out
	@cmp prog64d.bin prog64d.out && echo "prog64d: byte-identical"

# AVX-512 build: gcc emits VEX + EVEX (vmovup[sd]/vfmadd*/vpmulld/vbroadcast*/
# masked forms/vzeroupper). Validates the VEX/EVEX path on real AVX compiler
# output, not just random VEX bytes.
prog64e: asm32_64 prog64e.c
	gcc -O3 -mavx512f -mavx512vl -mavx512bw -mavx512dq -fno-stack-protector -fcf-protection=full -c prog64e.c -o prog64e.o
	objcopy -O binary --only-section=.text prog64e.o prog64e.bin
	./asm32_64 d prog64e.bin prog64e.dat
	./asm32_64 c prog64e.dat prog64e.out
	@cmp prog64e.bin prog64e.out && echo "prog64e: byte-identical"

# long double math -> x87 (D8-DF): fldt/fstpt, fmul/fadd/fmulp, fxch, fucomi,
# fildll/fistpll. Validates the x87 escape opcodes on real compiler output.
prog64f: asm32_64 prog64f.c
	gcc -O2 -fno-stack-protector -fcf-protection=full -c prog64f.c -o prog64f.o
	objcopy -O binary --only-section=.text prog64f.o prog64f.bin
	./asm32_64 d prog64f.bin prog64f.dat
	./asm32_64 c prog64f.dat prog64f.out
	@cmp prog64f.bin prog64f.out && echo "prog64f: byte-identical"

clean:
	rm -f parser test asm32 asm32_64 fuzz fuzz64 insns.dat out.bin corpus.head nc.dat nc_out.bin
	rm -f insns64.dat out64.bin prog64.o prog64.bin prog64.out prog64.dat
	rm -f prog64b.o prog64b.bin prog64b.out prog64b.dat
	rm -f prog64c.o prog64c.bin prog64c.out prog64c.dat
	rm -f prog64d.o prog64d.bin prog64d.out prog64d.dat
	rm -f prog64e.o prog64e.bin prog64e.out prog64e.dat
	rm -f prog64f.o prog64f.bin prog64f.out prog64f.dat
	rm -rf gen64

.PHONY: all corpus regen check run runtest roundtrip noncanon fuzz clean \
        asm64 check64 roundtrip64 prog64 prog64b prog64c prog64d prog64e prog64f fuzz64

x8632all: asm32 x8632all.bin
	@./asm32 d x8632all.bin /tmp/x8.dat >/dev/null 2>&1 && ./asm32 c /tmp/x8.dat /tmp/x8.out >/dev/null 2>&1 && \
	  cmp -s x8632all.bin /tmp/x8.out && echo "x8632all: 110072/110072 byte-identical (18836 insns)" || echo "x8632all: MISMATCH"
