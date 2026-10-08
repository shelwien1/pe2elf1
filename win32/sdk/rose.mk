# Compiler and linker options for programs that use ROSE, built with MinGW-w64 GCC (the ROSE SDK
# in this folder).  In a Makefile:
#
#   include C:/path/to/rose-2.18.0-win64/rose.mk
#   tool.exe: tool.o
#   	$(CXX) $(ROSE_LDFLAGS) -o $@ $< $(ROSE_LIBS)
#   tool.o: tool.C
#   	$(CXX) -O2 $(ROSE_CXXFLAGS) -c $< -o $@
#
# The compiler options are in include\rose\rose.rsp, with the include directories relative to
# include\rose (gcc -iprefix).  The programs use bin\rose.dll: put them in bin\, or bin\ in the
# PATH.  The path of this folder must not contain spaces.
ROSE_SDK := $(patsubst %/,%,$(subst \,/,$(dir $(lastword $(MAKEFILE_LIST)))))
ROSE_CXXFLAGS := -iprefix $(ROSE_SDK)/include/rose/ @$(ROSE_SDK)/include/rose/rose.rsp
ROSE_LDFLAGS := -static -pthread -Wl,--stack,67108864
ROSE_LIBS := $(ROSE_SDK)/lib/rose.lib -lshlwapi -lws2_32 -lbcrypt -lpsapi
