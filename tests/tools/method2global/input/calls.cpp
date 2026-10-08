#include <cstdio>
#include <vector>
struct Counter {
  int n = 0;
  void add(int k) { n += k; }
  virtual void reset() { n = 0; }
  static int twice(int x) { return 2 * x; }
  int value() const { return n; }
  void bump() { auto f = [this] { add(1); }; f(); }
  struct Inner { int z = 3; int get() const { return z; } };
  Inner in;
};
struct Holder { Counter c; Counter& ref() { return c; } };
int main() {
  std::vector<Counter> v(2);
  v[1].add(5);
  Holder h;
  h.c.add(2);
  h.ref().add(3);
  Counter* p = &v[0];
  (*p).add(4);
  p->bump();
  int (Counter::*pm)() const = &Counter::value;
  std::printf("%d %d %d %d %d\n", v[1].value(), h.c.value(), (p->*pm)(), v[0].in.get(), Counter::twice(2));
  return 0;
}
