// like remap_test.c, but another mapping is created right after the heap (as
// cmix's other model tables are), so the freed range has no slack around it
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
int main(void) {
  size_t t = 14000ULL << 20;
  int fd = open("remap_test.tmp", O_RDWR | O_CREAT | O_TRUNC, 0664);
  if (lseek(fd, t, SEEK_SET) < 0 || write(fd, "", 1) != 1) return 1;
  char *a = mmap(NULL, t, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);
  char *filler = mmap(NULL, 1ULL << 30, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  printf("heap %p..%p filler %p..%p (align of heap: %s)\n", a, a + t, filler, filler + (1ULL << 30),
         ((unsigned long)a & ((2UL << 20) - 1)) ? "not 2MiB" : "2MiB");
  a[123] = 42;
  int moved = 0;
  for (int i = 0; i < 5; i++) {
    munmap(a, t);
    fd = open("remap_test.tmp", O_RDWR);
    char *b = mmap(NULL, t, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (b != a) { moved++; printf("  remap %d moved %p -> %p\n", i, a, b); }
    a = b;
  }
  printf("remaps that moved: %d of 5, byte check %d\n", moved, a[123]);
  unlink("remap_test.tmp");
  return 0;
}
