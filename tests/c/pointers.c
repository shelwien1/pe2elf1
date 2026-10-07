/* Pointers, arrays, function pointers, casts */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmp(const void *a, const void *b) {
  return *(const int *)a - *(const int *)b;
}

typedef int (*binop)(int, int);

static int mul(int a, int b) { return a * b; }
static int sub(int a, int b) { return a - b; }

static binop ops[] = {mul, sub};

int apply(binop f, int x, int y) { return f(x, y); }

int main(void) {
  int data[] = {5, 3, 9, 1, 7};
  size_t n = sizeof data / sizeof data[0];
  qsort(data, n, sizeof(int), cmp);
  for (size_t i = 0; i < n; i++) printf("%d ", data[i]);
  printf("\n");
  int *p = data + 2;
  printf("%d %d %d\n", *p, p[1], *(p - 1));
  char buf[32];
  strcpy(buf, "abc");
  strcat(buf, "def");
  printf("%s %zu\n", buf, strlen(buf));
  int (*fp)(int, int) = &mul;
  printf("%d %d %d\n", apply(ops[0], 6, 7), apply(ops[1], 6, 7), (*fp)(2, 3));
  char *heap = malloc(16);
  memset(heap, 'x', 15);
  heap[15] = '\0';
  printf("%s\n", heap);
  free(heap);
  void *vp = (void *)data;
  printf("%d\n", ((int *)vp)[4]);
  int matrix[2][3] = {{1, 2, 3}, {4, 5, 6}};
  int total = 0;
  for (int r = 0; r < 2; r++)
    for (int c = 0; c < 3; c++) total += matrix[r][c];
  printf("%d\n", total);
  return 0;
}
