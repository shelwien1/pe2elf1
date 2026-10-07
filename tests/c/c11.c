/* C99/C11 features */
#include <complex.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define type_name(x) _Generic((x), int: "int", double: "double", default: "other")

_Static_assert(sizeof(int) == 4, "int must be 4 bytes");

struct with_anon {
  int kind;
  union {
    int i;
    double d;
  };
  struct {
    int x, y;
  };
};

static int sum_vla(int n, int m, int a[n][m]) {
  int s = 0;
  for (int i = 0; i < n; i++)
    for (int j = 0; j < m; j++) s += a[i][j];
  return s;
}

static int flex_total(int n) {
  int vla[n];
  for (int i = 0; i < n; i++) vla[i] = i;
  int s = 0;
  for (int i = 0; i < n; i++) s += vla[i];
  return s + (int)(sizeof vla / sizeof vla[0]);
}

int main(void) {
  bool flag = true;
  int64_t big = INT64_C(1) << 40;
  uint8_t small = (uint8_t)300;
  double complex z = 1.0 + 2.0 * I;
  struct with_anon w = {.kind = 1, .d = 2.5, .x = 3, .y = 4};
  int grid[2][3] = {{1, 2, 3}, {4, 5, 6}};
  alignas(16) char buffer[32];
  printf("%d %lld %u\n", flag, (long long)big, small);
  printf("%.1f %.1f\n", creal(z), cimag(z));
  printf("%d %.1f %d %d\n", w.kind, w.d, w.x, w.y);
  printf("%d %d\n", sum_vla(2, 3, grid), flex_total(5));
  printf("%s %s %s\n", type_name(1), type_name(1.0), type_name("s"));
  printf("%zu %zu\n", alignof(double), sizeof buffer);
  for (int i = 0, j = 10; i < j; i += 3, j--) printf("%d:%d ", i, j);
  printf("\n");
  return 0;
}
