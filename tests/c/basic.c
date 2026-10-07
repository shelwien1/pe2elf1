/* Basic declarations, expressions and control flow */
#include <stdio.h>

enum color { RED, GREEN = 5, BLUE };
#define N 10

struct point {
  int x, y;
};

typedef struct point point_t;

static int table[N + 1];
const char *message = "hello, \"world\"\n";
static double scale = 2.5;

int add(int a, int b) { return a + b; }

static int sum_table(void) {
  int s = 0;
  for (int i = 0; i <= N; i++) s += table[i];
  return s;
}

int classify(int v) {
  switch (v) {
    case 0:
      return 100;
    case 1:
    case 2:
      return 200;
    default:
      break;
  }
  return -1;
}

int main(void) {
  point_t p = {3, 4};
  struct point *pp = &p;
  enum color c = GREEN;
  unsigned u = 0x10u;
  long l = 'a' + 1L;
  int i = 0;
  for (i = 0; i < N + 1; ++i) table[i] = i * i;
  while (i > 5) i--;
  do {
    i += 2;
  } while (i < 20);
  if (c == GREEN && pp->x == 3)
    printf("green %d %d\n", pp->y, (int)(scale * 2));
  else
    printf("not green\n");
  printf("%s", message);
  printf("%d %u %ld %d\n", add(p.x, p.y), u, l, i);
  printf("%d %d %d\n", sum_table(), classify(1), classify(7));
  printf("%d\n", (int)sizeof(struct point) + BLUE);
  return i > 0 ? 0 : 1;
}
