# Linux build of h264dec (the equivalent of g.bat) and h264p.
#
#   make                         optimized build -> ./h264dec ./h264p
#   make CXXFLAGS="-O0 -g"       debug build (asserts enabled)
#   make test                    decode test/*.h264 and check output MD5s,
#                                and round-trip them through h264p
#
# -fno-strict-aliasing is required, not optional: the decoder type-puns its
# motion vector / reference index arrays (ST32/LD32 etc.). Upstream OpenH264
# always builds with it.

CXXFLAGS ?= -O3 -DNDEBUG
# h264p's target: x86-64-v3 (AVX2). PARCH= builds the plain C++ instead, for
# any target, which codes identically
PARCH ?= -march=x86-64-v3 -ffp-contract=off

OBJDIR := _obj_linux
SRCS   := $(wildcard codec/*.cpp)
OBJS   := $(SRCS:codec/%.cpp=$(OBJDIR)/%.o)
PSRCS  := $(wildcard h264p_src/*.cpp)
POBJS  := $(PSRCS:h264p_src/%.cpp=$(OBJDIR)/h264p/%.o)

all: h264dec h264p

h264dec: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -pthread -o $@ $^ $(LDLIBS)

h264p: $(POBJS)
	$(CXX) $(CXXFLAGS) $(PARCH) $(LDFLAGS) -pthread -o $@ $^ $(LDLIBS)

$(OBJDIR)/%.o: codec/%.cpp | $(OBJDIR)
	$(CXX) $(CPPFLAGS) -Icodec $(CXXFLAGS) -fno-strict-aliasing -pthread -MMD -MP -c $< -o $@

$(OBJDIR)/h264p/%.o: h264p_src/%.cpp | $(OBJDIR)/h264p
	$(CXX) $(CPPFLAGS) -Ih264p_src/Lib3 $(CXXFLAGS) $(PARCH) -std=c++11 -fwrapv -fno-exceptions -fno-rtti -pthread -MMD -MP -c $< -o $@

$(OBJDIR) $(OBJDIR)/h264p:
	mkdir -p $@

test: h264dec h264p
	./test/run.sh ./h264dec
	./test/h264p.sh ./h264p

clean:
	rm -rf $(OBJDIR) h264dec h264p

.PHONY: all test clean

-include $(OBJS:.o=.d) $(POBJS:.o=.d)
