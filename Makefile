CXX ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra

# liblzma (optional) enables -a, the per-file option search
LZMA ?= $(shell pkg-config --exists liblzma 2>/dev/null && echo 1)
ifeq ($(LZMA),1)
LZMA_CFLAGS = -DX64PP_LZMA $(shell pkg-config --cflags liblzma)
LZMA_LIBS = $(shell pkg-config --libs liblzma)
endif

x64pp: src/x64pp.cpp src/x64dec.h src/tables.h src/analyze.h
	$(CXX) $(CXXFLAGS) $(LZMA_CFLAGS) -o $@ src/x64pp.cpp $(LZMA_LIBS)

# Windows binary with mingw-w64.  For -a, set LZMA_WIN to a directory with
# include/lzma.h and lib/liblzma.a built for mingw-w64 (for example the
# mingw64 tree of the MSYS2 package mingw-w64-x86_64-xz).
ifneq ($(LZMA_WIN),)
WIN_LZMA_CFLAGS = -DX64PP_LZMA -DLZMA_API_STATIC -I$(LZMA_WIN)/include
WIN_LZMA_LIBS = $(LZMA_WIN)/lib/liblzma.a
endif

x64pp.exe: src/x64pp.cpp src/x64dec.h src/tables.h src/analyze.h
	x86_64-w64-mingw32-g++ -O2 -Wall -Wextra -static -s $(WIN_LZMA_CFLAGS) -o $@ src/x64pp.cpp $(WIN_LZMA_LIBS)

clean:
	rm -f x64pp x64pp.exe

.PHONY: clean
