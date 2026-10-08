// References in templates are renamed, also in templates that are not instantiated; the uses that
// the front end does not resolve (in code that the preprocessor skips) are not
#include <cstdio>
int counter;
void bump() { ++counter; }
template <class T> void unused(T) { ++counter; bump(); }
template <class T> struct S {
  void f() { ++counter; }
  int g() { return counter; }
};
template <class T> struct U { void h() { counter += 2; } };
#if 0
int old() { return counter; }
#endif
int main() {
  S<int> s;
  s.f();
  bump();
  std::printf("%d\n", counter);
  return 0;
}
