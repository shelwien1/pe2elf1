# mrpc -- lossless BMP compression, with the encoder's optimisation loop
# on OpenCL.
#
#   make            the library and the command line, with OpenCL
#   make mrpc-nocl  the same without: no OpenCL headers or loader needed
#   make windows    mrpc.exe, cross-built with MinGW-w64
#   make test       round-trip the library through its C API
#
# The library is mrpc_lib.cpp behind mrpc_lib.h; mrpc.cpp is a frontend
# that includes nothing else.

CXX      ?= g++
CC       ?= gcc
CXXFLAGS ?= -O2 -std=c++17 -march=native -DNDEBUG -w
CFLAGS   ?= -O2 -std=c99 -Wall
LDLIBS   ?= -lOpenCL

LIBSRC = mrpc_lib.cpp mrpc_lib.h mrpc_cl.inc mrpc_kernels.inc sh_common.inc sh_v2f.inc

all: mrpc

# --- shared ----------------------------------------------------------
# Built with -fvisibility=hidden on purpose: that is what a shared
# library should be built with, and it is what makes the attributes in
# mrpc_lib.h load-bearing.  `make exports` prints what comes out, which
# should be the ten functions in the header and nothing else.
libmrpc.so: $(LIBSRC)
	$(CXX) $(CXXFLAGS) -fPIC -fvisibility=hidden -fvisibility-inlines-hidden \
	  -DMRP_OPENCL -shared mrpc_lib.cpp -o $@ $(LDLIBS)

exports: libmrpc.so
	@nm -D --defined-only libmrpc.so | grep " T \| W " || true

# and the same for Windows.  -static-libgcc/-static-libstdc++ so the DLL
# does not drag the MinGW runtime along with it; the import library is
# what a caller links against, though LoadLibrary works too -- the
# exports are undecorated __cdecl.
mrpc.dll: $(LIBSRC) winsdk/CL/cl.h
	$(WINCXX) $(WINFLAGS_DLL) -DMRP_OPENCL -DMRPC_BUILD_DLL -isystem winsdk \
	  -shared mrpc_lib.cpp -o $@ -Wl,--out-implib,libmrpc.dll.a

dll: mrpc.dll

libmrpc.a: mrpc_lib.o
	$(AR) rcs $@ $^

mrpc_lib.o: $(LIBSRC)
	$(CXX) $(CXXFLAGS) -DMRP_OPENCL -c mrpc_lib.cpp -o $@

mrpc: mrpc.cpp mrpc_lib.h libmrpc.a
	$(CXX) $(CXXFLAGS) mrpc.cpp libmrpc.a $(LDLIBS) -o $@

# the reference build: no device, no OpenCL headers needed
mrpc-nocl: mrpc.cpp $(LIBSRC)
	$(CXX) $(CXXFLAGS) -c mrpc_lib.cpp -o mrpc_lib_nocl.o
	$(CXX) $(CXXFLAGS) mrpc.cpp mrpc_lib_nocl.o -o $@

# t_lib is C, not C++: if it compiles, the header is what it claims to be
t_lib: t_lib.c mrpc_lib.h libmrpc.a
	$(CC) $(CFLAGS) t_lib.c libmrpc.a $(LDLIBS) -lstdc++ -lm -o $@

test: t_lib
	./t_lib

.PHONY: all clean devices windows test exports dll
clean:
	rm -f mrpc mrpc-nocl mrpc.exe t_lib t_dll.exe *.o *.a *.so *.dll
	rm -rf winsdk

devices: mrpc
	./mrpc -l

# --- Windows ---------------------------------------------------------
# Cross-built with MinGW-w64.  There is no import library for the ICD
# loader on Windows and an executable that imports OpenCL.dll statically
# will not start on a machine without it, so mrpc_cl.inc opens it at run
# time: this .exe runs, and falls back to the host path, where there is no
# OpenCL at all.  Nothing but KERNEL32 and msvcrt is imported.
#
# CL_HEADERS is the CL directory itself -- from the opencl-headers
# package, or a checkout of KhronosGroup/OpenCL-Headers.  It is staged
# into winsdk/ rather than used where it lies, because handing the cross
# compiler the whole of /usr/include hands it glibc's headers too.
WINCXX     ?= x86_64-w64-mingw32-g++
WINARCH    ?= haswell
WINFLAGS   ?= -O2 -std=c++17 -march=$(WINARCH) -mtune=$(WINARCH) -DNDEBUG -w -static
WINFLAGS_DLL ?= -O2 -std=c++17 -march=$(WINARCH) -mtune=$(WINARCH) -DNDEBUG -w \
                -static-libgcc -static-libstdc++
CL_HEADERS ?= /usr/include/CL

windows: mrpc.exe

winsdk/CL/cl.h: $(CL_HEADERS)/cl.h
	mkdir -p winsdk && cp -r $(CL_HEADERS) winsdk/

mrpc.exe: mrpc.cpp $(LIBSRC) winsdk/CL/cl.h
	$(WINCXX) $(WINFLAGS) -DMRP_OPENCL -isystem winsdk -c mrpc_lib.cpp -o mrpc_lib_win.o
	$(WINCXX) $(WINFLAGS) mrpc.cpp mrpc_lib_win.o -o $@
