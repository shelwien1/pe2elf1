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

.PHONY: all clean devices
clean:
	rm -f mrpc mrpc-nocl

devices: mrpc
	./mrpc -l
