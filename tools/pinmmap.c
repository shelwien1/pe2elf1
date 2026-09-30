// pinmmap.c - LD_PRELOAD shim for fx2-cmix-family Hutter Prize binaries.
//
// cmix's PPMd model (src/models/ppmd.cpp, mmap_to_disk = true) keeps its
// 14000 MiB heap in a MAP_SHARED mapping of ./ppm.temp and, every 20000 bytes,
// munmap()s it and mmap()s it again with addr = NULL. The suballocator keeps
// absolute pointers into that heap, so this only works when the kernel happens
// to return the same address. On Linux 6.18 it does not, and the process
// segfaults at exactly byte 20000. Affected: fx2-cmix (compressor and archive9)
// and the 24 Jul 2026 fx2-cmix-transformer build. cmix-lex (MADV_DONTNEED) and
// the 21 Aug 2026 fx2-cmix-transformer build (MAP_FIXED) are already fixed.
//
// The shim remembers the address of the first large (>= 4 GiB) MAP_SHARED
// mapping requested with addr = NULL and puts every later mapping of the same
// length back at that address. The environment is inherited by system(), so
// the "./archive9 -d ..." child that archive9 starts is covered too.
//
// build: gcc -O2 -Wall -shared -fPIC -o pinmmap.so pinmmap.c -ldl
// use:   LD_PRELOAD=$PWD/pinmmap.so ./archive9
//        LD_PRELOAD=$PWD/pinmmap.so ./cmix -d dict.comp english.dic
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif

static void *(*real_mmap)(void *, size_t, int, int, int, off_t);
static void *pinned_addr;
static size_t pinned_len;

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off) {
  if (!real_mmap) real_mmap = (void *(*)(void *, size_t, int, int, int, off_t))dlsym(RTLD_NEXT, "mmap");
  if (addr != NULL || !(flags & MAP_SHARED) || len < (1ULL << 32))
    return real_mmap(addr, len, prot, flags, fd, off);
  if (pinned_addr == NULL || len != pinned_len) {
    void *p = real_mmap(addr, len, prot, flags, fd, off);
    if (p != MAP_FAILED) {
      pinned_addr = p;
      pinned_len = len;
    }
    return p;
  }
  // The old mapping was just unmapped; ask for exactly the same range.
  void *p = real_mmap(pinned_addr, len, prot, flags | MAP_FIXED_NOREPLACE, fd, off);
  if (p == pinned_addr) return p;
  if (p != MAP_FAILED) munmap(p, len);  // pre-4.17 kernels treat the flag as a hint
  fprintf(stderr, "pinmmap: cannot re-map %zu bytes at %p\n", len, pinned_addr);
  abort();
}

void *mmap64(void *addr, size_t len, int prot, int flags, int fd, off_t off) {
  return mmap(addr, len, prot, flags, fd, off);
}
