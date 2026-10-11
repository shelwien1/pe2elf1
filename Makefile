# vbn.exe -> vbn (i386 ELF, via pe2elf32) + libvbnshim.so (DOS-service shim),
# and vbn2, VBN's compressor as standalone C++ (vbn2 c|d input output).
#
# The shim is built -m32, which needs the 32-bit dev headers and libraries
# (Debian/Ubuntu: gcc-multilib).  Running vbn needs only the i386 glibc
# runtime (/lib/ld-linux.so.2).  vbn2 is built for the host; it builds with
# -m32 as well.

PE2ELF32 = pe2elf32/pe2elf32

all: vbn libvbnshim.so vbn2

# Host tool: it only emits ELF32 bytes, so it is built natively.
$(PE2ELF32): pe2elf32/pe2elf32.cpp $(wildcard pe2elf32/*.hpp)
	$(CXX) -O2 -std=c++17 -Wall -Wextra -Wno-unused-parameter -static -o $@ pe2elf32/pe2elf32.cpp

# vbn.exe imports vbnshim.dll; the ELF gets DT_NEEDED libvbnshim.so and
# DT_RUNPATH $ORIGIN, so the shim is looked up next to vbn.
vbn: vbn.exe $(PE2ELF32)
	$(PE2ELF32) vbn.exe $@ --shim-soname libvbnshim.so

libvbnshim.so: shim.c
	$(CC) -m32 -O2 -Wall -Wextra -shared -fPIC -o $@ shim.c

# vbn2.cpp is maintained by hand (it began as incdec/'s decompiled functions);
# it needs neither vbn.exe nor the shim, at build or at run time.
vbn2: vbn2.cpp
	$(CXX) -O2 -o $@ vbn2.cpp

test: all
	./test.sh

clean:
	rm -f $(PE2ELF32) vbn libvbnshim.so vbn2

.PHONY: all test clean
