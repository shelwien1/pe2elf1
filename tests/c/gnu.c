/* GNU C extensions */
#include <stdio.h>

#define max(a, b) ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a > _b ? _a : _b; })

struct __attribute__((packed)) packed_s {
  char c;
  int i;
};

static int classify(int v) {
  switch (v) {
    case 0 ... 9:
      return 1;
    case 10 ... 99:
      return 2;
    default:
      return 3;
  }
}

static int dispatch(int op) {
  static void *table[] = {&&do_add, &&do_sub, &&done};
  int acc = 10;
  goto *table[op];
do_add:
  acc += 5;
  goto done;
do_sub:
  acc -= 5;
done:
  return acc;
}

static inline int __attribute__((always_inline)) square(int x) { return x * x; }

int main(void) {
  int a = 3, b = 7;
  long m = max(a, b);
  __typeof__(m) copy = m;
  int z = a ?: b;
  printf("%ld %ld %d\n", m, copy, z);
  printf("%d %d %d\n", classify(5), classify(42), classify(1000));
  printf("%d %d %d\n", dispatch(0), dispatch(1), dispatch(2));
  printf("%zu %d\n", sizeof(struct packed_s), square(9));
  if (__builtin_expect(a > b, 0)) printf("unlikely\n");
  int r;
  __asm__("movl %1, %0" : "=r"(r) : "r"(a));
  printf("%d\n", r);
  int x = ({ int t = a * 2; t + 1; });
  printf("%d %d\n", x, __builtin_popcount(255));
  return 0;
}
