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

clean:
	rm -f parser test asm32 fuzz insns.dat out.bin corpus.head nc.dat nc_out.bin

.PHONY: all corpus regen check run runtest roundtrip noncanon fuzz clean

x8632all: asm32 x8632all.bin
	@./asm32 d x8632all.bin /tmp/x8.dat >/dev/null 2>&1 && ./asm32 c /tmp/x8.dat /tmp/x8.out >/dev/null 2>&1 && \
	  cmp -s x8632all.bin /tmp/x8.out && echo "x8632all: 110072/110072 byte-identical (18836 insns)" || echo "x8632all: MISMATCH"
