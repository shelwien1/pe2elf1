/* In C, the names of structures are apart from the other names, those of typedefs are not */
#include <stdio.h>

struct point { int x, y; };
typedef struct point point_t;
int point = 0;

/* A parameter of a function in K&R C is declared twice */
int kr_sum(one, second)
int one;
int second;
{
  return one + second;
}

int main(void) {
  struct point p = { 1, 2 };
  point_t q = p;
  printf("%d %d\n", p.x + q.y + point, kr_sum(3, 4));
  return 0;
}
