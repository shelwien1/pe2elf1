# Compiler and linker options for programs that use ROSE, built with g++ (the ROSE SDK in this
# folder).  In a Makefile:
#
#   include /path/to/rose-2.18.0-linux64/rose.mk
#   tool: tool.o
#   	$(CXX) $(ROSE_LDFLAGS) -o $@ $< $(ROSE_LIBS)
#   tool.o: tool.C
#   	$(CXX) -O2 $(ROSE_CXXFLAGS) -c $< -o $@
#
# The compiler options are in include/rose/rose.rsp, with the include directories relative to
# include/rose (gcc -iprefix).  The programs find lib/librose.so through their run path, the
# absolute path of lib/ (move them with the folder: rebuild them, or set LD_LIBRARY_PATH).  The
# path of this folder must not contain spaces.
ROSE_SDK := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ROSE_CXXFLAGS := -iprefix $(ROSE_SDK)/include/rose/ @$(ROSE_SDK)/include/rose/rose.rsp
ROSE_LDFLAGS := -pthread -Wl,-rpath,$(ROSE_SDK)/lib
ROSE_LIBS := -L$(ROSE_SDK)/lib -lrose
