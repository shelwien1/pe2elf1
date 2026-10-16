# Linux build of h264dec (the equivalent of g.bat).
#
#   make                         optimized build -> ./h264dec
#   make CXXFLAGS="-O0 -g"       debug build (asserts enabled)
#   make test                    decode test/*.h264 and check output MD5s
#
# -fno-strict-aliasing is required, not optional: the decoder type-puns its
# motion vector / reference index arrays (ST32/LD32 etc.). Upstream OpenH264
# always builds with it.

CXXFLAGS ?= -O3 -DNDEBUG

OBJDIR := _obj_linux
SRCS   := $(wildcard codec/*.cpp)
OBJS   := $(SRCS:codec/%.cpp=$(OBJDIR)/%.o)

h264dec: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -pthread -o $@ $^ $(LDLIBS)

$(OBJDIR)/%.o: codec/%.cpp | $(OBJDIR)
	$(CXX) $(CPPFLAGS) -Icodec $(CXXFLAGS) -fno-strict-aliasing -pthread -MMD -MP -c $< -o $@

$(OBJDIR):
	mkdir -p $@

test: h264dec
	./test/run.sh ./h264dec

clean:
	rm -rf $(OBJDIR) h264dec

.PHONY: test clean

-include $(OBJS:.o=.d)
