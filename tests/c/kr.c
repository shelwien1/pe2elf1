/* Old-style (K&R) definitions, linkage, tentative definitions, function pointers */
#include <stdio.h>

int counter;
int counter;
extern int shared;
int shared = 42;

int kr_add(a, b)
int a;
int b;
{
  return a + b;
}

double kr_scale(x, factor)
double x;
int factor;
{
  return x * factor;
}

struct ops {
  int (*apply)(int, int);
  const char *name;
};

static int mul(int a, int b) { return a * b; }

static struct ops table[] = {{kr_add, "add"}, {mul, "mul"}};

typedef int handler_t(int);
static int twice(int v) { return 2 * v; }
static handler_t *pick(void) { return twice; }

int main(void) {
  counter++;
  for (unsigned i = 0; i < sizeof table / sizeof table[0]; i++)
    printf("%s=%d ", table[i].name, table[i].apply(6, 7));
  printf("\n%d %.1f %d %d\n", counter, kr_scale(1.5, 4), shared, pick()(21));
  return 0;
}
