/* In C, the names of structures are apart from the other names, those of typedefs are not */
#include <stdio.h>

struct point { int x, y; };
typedef struct point point_t;
int origin = 0;

int main(void) {
  struct point p = { 1, 2 };
  point_t q = p;
  printf("%d\n", p.x + q.y + origin);
  return 0;
}
