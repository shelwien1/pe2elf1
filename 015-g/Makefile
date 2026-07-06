# Makefile for stegdict — Linux port of g.bat
#
#   make        build ./stegdict (statically linked, mirrors g.bat)
#   make test   build, then run the round-trip test (t.sh)
#   make clean  remove the built binary and generated usage.inc
#
# g.bat builds with MinGW g++ and -D_WIN32; that define is a Windows-target
# artifact and is dropped here so the glibc headers see a real Linux target.
# -static is kept: the binary is used as its own test payload and must be
# larger than the ~96 KB dictionary capacity (see t.sh).
#
# usage.inc holds the help/error strings; it is generated from usage.txt by
# txt2inc.pl (which also stamps the build date) and #included by stegdict.cpp.

CXX      ?= g++

INCS     := -DNDEBUG -DSTRICT -ILib3
OPTS     := -fstrict-aliasing -fomit-frame-pointer -ffast-math \
            -fno-rtti -fno-stack-protector -fno-stack-check \
            -fno-check-new -fno-exceptions -fpermissive
ARCH     := -march=k8 -masm=att
# the help/error strings from usage.inc are printf() format strings (txt2inc
# escapes % -> %% and maps $DATA -> %s), so printing a no-argument one is safe;
# silence Ubuntu g++'s default -Wformat-security on those calls.
CXXFLAGS := -s -Ofast -std=gnu++20 -Wno-format-security $(ARCH) $(INCS) $(OPTS)
LDFLAGS  := -static

TARGET   := stegdict
SRC      := stegdict.cpp
GEN      := usage.inc

$(TARGET): $(SRC) $(GEN)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(SRC) -o $(TARGET)

$(GEN): usage.txt txt2inc.pl
	perl txt2inc.pl usage.txt $(GEN)

.PHONY: test clean
test: $(TARGET)
	./t.sh

clean:
	rm -f $(TARGET) $(GEN)
