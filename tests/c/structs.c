/* Structs, unions, bit-fields, designated initializers, compound literals */
#include <stdio.h>

struct inner {
  int a;
  char tag;
};

struct outer {
  struct inner in;
  int values[3];
  union {
    int i;
    float f;
  } u;
  unsigned flag : 1;
  unsigned mode : 3;
};

struct list {
  int value;
  struct list *next;
};

static struct outer global = {.in = {.a = 1, .tag = 'g'}, .values = {[1] = 20, 30}, .flag = 1};

static int sum_list(const struct list *l) {
  int s = 0;
  for (; l; l = l->next) s += l->value;
  return s;
}

int main(void) {
  struct outer o = {{2, 'o'}, {1, 2, 3}, {7}, 0, 5};
  struct list c = {3, 0}, b = {2, &c}, a = {1, &b};
  struct inner *ip = &(struct inner){42, 'c'};
  o.u.f = 1.5f;
  o.mode = 6;
  printf("%d %c %d %d %d\n", global.in.a, global.in.tag, global.values[0], global.values[1], global.values[2]);
  printf("%d %c %d %u %u\n", o.in.a, o.in.tag, o.values[2], o.flag, o.mode);
  printf("%g %d %d %c\n", o.u.f, sum_list(&a), ip->a, ip->tag);
  int arr[5] = {[2] = 1, 3};
  printf("%d %d %d %d %d\n", arr[0], arr[1], arr[2], arr[3], arr[4]);
  return 0;
}
