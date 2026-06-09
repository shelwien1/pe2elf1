# Portable XP10 encoder build (no Verilator install needed)
# Parallels portable_decoder/, but the encoder's Verilator-generated C++
# is ~3.5 GB raw (two files are ~890 MB each), so it's shipped as a single
# generated.tar.xz (25 MB) that's unpacked into generated/ before g++ runs.
#
#   make unpack     # generated.tar.xz -> generated/   (~3.5 GB on disk)
#   make            # build ./xp10-enc                 (very slow; see WARN)
#   make clean      # remove .o, ./xp10-enc, and the unpacked tree
#
# WARNING: this build is much larger than the decoder. Two ~890 MB .cpp
# files come from Verilator unrolling the LZ77 / Huffman state machines;
# g++ needs many GBs of RAM and (even at -O0) substantial time to compile
# them. Build only on a host with >=16 GB RAM. The project README marks
# the encoder as "code-complete, not yet validated" -- this tree exists
# so a beefier host can pick it up.

CXX      ?= g++
OPT      ?= -O0

CXXFLAGS  = $(OPT) -std=gnu++17 -pthread \
            -faligned-new -fcf-protection=none \
            -DVM_COVERAGE=0 -DVM_SC=0 \
            -DVM_TRACE=0 -DVM_TRACE_FST=0 -DVM_TRACE_VCD=0 \
            -DENCODE_BUILD \
            -I generated -I verilator_include -I verilator_include/vltstd \
            -Wno-bool-operation -Wno-overloaded-virtual -Wno-shadow \
            -Wno-sign-compare -Wno-uninitialized \
            -Wno-unused-but-set-parameter -Wno-unused-but-set-variable \
            -Wno-unused-parameter -Wno-unused-variable

LDLIBS    = -pthread -latomic

GEN_SRCS  := $(addprefix generated/,$(shell cat generated.cpp.list))
RT_SRCS   := verilator_include/verilated.cpp verilator_include/verilated_threads.cpp
USR_SRCS  := xp10.cpp

SRCS      := $(USR_SRCS) $(GEN_SRCS) $(RT_SRCS)
OBJS      := $(SRCS:.cpp=.o)

xp10-enc: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDLIBS)

unpack: generated/.unpacked
generated/.unpacked: generated.tar.xz
	mkdir -p generated
	xz -dc generated.tar.xz | tar -xf - -C generated
	touch $@

# All generated .cpp must wait for unpack -- this is a single order-only
# prereq so make doesn't re-extract on every compile.
$(GEN_SRCS): | generated/.unpacked

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f xp10-enc xp10.o verilator_include/*.o
	rm -rf generated

.PHONY: clean unpack
