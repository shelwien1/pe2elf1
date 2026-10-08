#include <cstdio>
#include <vector>
struct Counter {
  int n = 0;
  friend void add(Counter* This, int k);
  virtual void reset() { n = 0; }
  static int twice(int x) { return 2 * x; }
  int value() const { return n; }
  void bump() { auto f = [this] { add(this, 1); }; f(); }
  struct Inner { int z = 3; friend int get(const Counter::Inner* This); };
  Inner in;
};

inline int get(const Counter::Inner* This) { return This->z; }

inline void add(Counter* This, int k) { This->n += k; }
struct Holder { Counter c; Counter& ref() { return c; } };
int main() {
  std::vector<Counter> v(2);
  add(&v[1], 5);
  Holder h;
  add(&h.c, 2);
  add(&h.ref(), 3);
  Counter* p = &v[0];
  add(&(*p), 4);
  p->bump();
  int (Counter::*pm)() const = &Counter::value;
  std::printf("%d %d %d %d %d\n", v[1].value(), h.c.value(), (p->*pm)(), get(&v[0].in), Counter::twice(2));
  return 0;
}
