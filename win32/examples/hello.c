/* Example for test.bat: a C program using the C library and the Windows API */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

struct point {
  int x, y;
};

static int distance2(const struct point* p) { return p->x * p->x + p->y * p->y; }

static int compare_points(const void* a, const void* b) {
  return distance2((const struct point*)a) - distance2((const struct point*)b);
}

int main(void) {
  struct point points[] = {{3, 4}, {1, 1}, {-2, 0}, {5, -5}};
  size_t n = sizeof points / sizeof points[0];
  char buffer[64];

  qsort(points, n, sizeof points[0], compare_points);
  for (size_t i = 0; i < n; i++) printf("(%d, %d) ", points[i].x, points[i].y);
  printf("\n");

  snprintf(buffer, sizeof buffer, "%s, %s!", "Hello", "Windows");
  printf("%s (%u characters)\n", buffer, (unsigned)strlen(buffer));
  printf("sizeof(long) = %u, sizeof(void*) = %u, MAX_PATH = %d\n", (unsigned)sizeof(long),
         (unsigned)sizeof(void*), MAX_PATH);
  printf("process id: %s\n", GetCurrentProcessId() != 0 ? "yes" : "no");
  return 0;
}
