# Portable XP10 decoder build -- requires only g++ (no Verilator install).
# The C++ in generated/ was produced by `verilator --cc cr_cddip ...`
# upstream; verilator_include/ is a copy of /usr/share/verilator/include.
#
#   make            # build ./xp10
#   make clean

CXX      ?= g++
OPT      ?= -O2

CXXFLAGS  = $(OPT) -std=gnu++17 -pthread \
            -faligned-new -fcf-protection=none \
            -DVM_COVERAGE=0 -DVM_SC=0 \
            -DVM_TRACE=0 -DVM_TRACE_FST=0 -DVM_TRACE_VCD=0 \
            -I generated -I verilator_include -I verilator_include/vltstd \
            -Wno-bool-operation -Wno-overloaded-virtual -Wno-shadow \
            -Wno-sign-compare -Wno-uninitialized \
            -Wno-unused-but-set-parameter -Wno-unused-but-set-variable \
            -Wno-unused-parameter -Wno-unused-variable

LDLIBS    = -pthread -latomic

# Verilator-generated translation units
GEN_SRCS := $(wildcard generated/*.cpp)
# Verilator runtime (the only two files our generated code links against)
RT_SRCS  := verilator_include/verilated.cpp verilator_include/verilated_threads.cpp
# User driver
USR_SRCS := xp10.cpp

SRCS     := $(USR_SRCS) $(GEN_SRCS) $(RT_SRCS)
OBJS     := $(SRCS:.cpp=.o)

xp10: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f xp10 $(OBJS)

.PHONY: clean
