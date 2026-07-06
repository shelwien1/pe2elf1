# Makefile for stegdict — Linux port of g.bat
#
#   make        build ./stegdict (statically linked, mirrors g.bat)
#   make test   build, then run the round-trip test (t.sh)
#   make clean  remove the built binary
#
# g.bat builds with MinGW g++ and -D_WIN32; that define is a Windows-target
# artifact and is dropped here so the glibc headers see a real Linux target.
# -static is kept: the binary is used as its own test payload and must be
# larger than the ~96 KB dictionary capacity (see t.sh).

CXX      ?= g++

INCS     := -DNDEBUG -DSTRICT -ILib3
OPTS     := -fstrict-aliasing -fomit-frame-pointer -ffast-math \
            -fno-rtti -fno-stack-protector -fno-stack-check \
            -fno-check-new -fno-exceptions -fpermissive
ARCH     := -march=k8 -masm=att
CXXFLAGS := -s -Ofast -std=gnu++20 $(ARCH) $(INCS) $(OPTS)
LDFLAGS  := -static

TARGET   := stegdict
SRC      := stegdict.cpp

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(SRC) -o $(TARGET)

.PHONY: test clean
test: $(TARGET)
	./t.sh

clean:
	rm -f $(TARGET)
