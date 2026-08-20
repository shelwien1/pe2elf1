# mrpc -- lossless BMP compressor, with the encoder's optimisation loop
# on OpenCL.  `make` builds with OpenCL, `make mrpc-nocl` without.

CXX      ?= g++
CXXFLAGS ?= -O2 -std=c++17 -march=native -DNDEBUG -w
LDLIBS   ?= -lOpenCL

all: mrpc

mrpc: mrpc.cpp mrpc_cl.inc mrpc_kernels.inc sh_common.inc sh_v2f.inc
	$(CXX) $(CXXFLAGS) -DMRP_OPENCL mrpc.cpp $(LDLIBS) -o $@

# the reference build: no device, no OpenCL headers needed
mrpc-nocl: mrpc.cpp sh_common.inc sh_v2f.inc
	$(CXX) $(CXXFLAGS) mrpc.cpp -o $@

.PHONY: all clean devices windows
clean:
	rm -f mrpc mrpc-nocl mrpc.exe
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
CL_HEADERS ?= /usr/include/CL

windows: mrpc.exe

winsdk/CL/cl.h: $(CL_HEADERS)/cl.h
	mkdir -p winsdk && cp -r $(CL_HEADERS) winsdk/

mrpc.exe: mrpc.cpp mrpc_cl.inc mrpc_kernels.inc sh_common.inc sh_v2f.inc winsdk/CL/cl.h
	$(WINCXX) $(WINFLAGS) -DMRP_OPENCL -isystem winsdk mrpc.cpp -o $@
