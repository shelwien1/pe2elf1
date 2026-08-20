# ocltest - OpenCL smoke test for the Intel CPU runtime

CXX      ?= g++
CXXFLAGS ?= -O2 -std=c++17 -Wall -Wextra
LDLIBS   ?= -lOpenCL

ocltest: ocltest.cpp
	$(CXX) $(CXXFLAGS) $< $(LDLIBS) -o $@

.PHONY: run clean install-runtime
run: ocltest
	./ocltest

clean:
	rm -f ocltest

install-runtime:
	./install-intel-opencl-cpu.sh
