// Lambdas: captures by value and reference, explicit return types, conversion
// to function pointers, lambdas passed to templates
#include <cstdio>

int apply(int (*f)(int), int v) { return f(v); }
template <class F> int call(F f, int v) { return f(v); }

int main() {
  int base = 10, k = 3;
  auto add = [&base, k](int x) { base += x * k; return base; };
  add(1);
  int r = call([=](int y) -> int { return y + k; }, 4);
  r += apply([](int z) { return z * 2; }, 5);
  auto counter = [n = 0]() mutable { return ++n; };
  counter();
  std::printf("%d %d %d\n", base, r, counter());
  return 0;
}
