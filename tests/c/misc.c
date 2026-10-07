/* Assorted C features: goto, labels, varargs, static locals, comma, ?:, strings */
#include <stdarg.h>
#include <stdio.h>

static int sum(int count, ...) {
  va_list ap;
  int s = 0;
  va_start(ap, count);
  for (int i = 0; i < count; i++) s += va_arg(ap, int);
  va_end(ap);
  return s;
}

static int counter(void) {
  static int calls = 0;
  return ++calls;
}

static int find(const int *v, int n, int x) {
  int i;
  for (i = 0; i < n; i++) {
    if (v[i] == x) goto found;
  }
  return -1;
found:
  return i;
}

static int hidden = 11;
extern int hidden; /* same (internal) linkage */
static int read_hidden(void) {
  extern int hidden; /* the file-scope variable, not a new local one */
  return hidden;
}

int main(void) {
  int v[] = {4, 8, 15, 16, 23, 42};
  int a = 1, b = 2, c;
  c = (a++, b += a, a * b);
  printf("%d %d %d\n", a, b, c);
  printf("%d %d\n", find(v, 6, 15), find(v, 6, 7));
  counter();
  counter();
  printf("%d\n", counter());
  printf("%d\n", sum(4, 1, 2, 3, 4));
  const char *s = a > b ? "greater" : "not greater";
  printf("%s %c %c\n", s, "xyz"[1], '\t' == 9 ? 'y' : 'n');
  unsigned char bits = 0xF0;
  bits = (unsigned char)(bits >> 2 | 1);
  printf("%d %d %d\n", bits, ~bits & 0xFF, !bits);
  double d = 1e-3 + .5 + 2.;
  float f = 3.25f;
  printf("%.4f %.2f\n", d, f);
  long long big = 1LL << 40;
  printf("%lld %llx\n", big, (unsigned long long)big);
  hidden = 12;
  printf("%d\n", read_hidden());
  return 0;
}
