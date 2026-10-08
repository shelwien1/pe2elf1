// References in templates are renamed, also in templates that are not instantiated; the uses that
// the front end does not resolve (in code that the preprocessor skips) are not
#include <cstdio>
int total;
void bump() { ++total; }
template <class T> void unused(T) { ++total; bump(); }
template <class T> struct S {
  void f() { ++total; }
  int g() { return total; }
};
template <class T> struct U { void h() { total += 2; } };
#if 0
int old() { return counter; }
#endif
int main() {
  S<int> s;
  s.f();
  bump();
  std::printf("%d\n", total);
  return 0;
}
