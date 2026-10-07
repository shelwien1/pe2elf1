// C++11/14 declarations: noexcept, ref-qualifiers, trailing return types, decltype(auto),
// user-defined literals, thread_local, alignas, raw strings, attributes
#include <cstdio>
#include <cstddef>
struct S {
  void f() noexcept {}
  void g() const & {}
  void h() && {}
  [[nodiscard]] int k() const { return 1; }
};
auto add(int a, int b) -> int { return a + b; }
decltype(auto) ident(int& x) { return (x); }
constexpr unsigned long long operator"" _kb(unsigned long long v) { return v * 1024; }
thread_local int tl = 3;
alignas(16) static int aligned_var = 4;
[[noreturn]] void die();
void die() { while (true) {} }
int main() noexcept(false) {
  S s;
  s.f();
  const char* raw = R"(a\b"c)";
  int x = 5;
  ident(x) = 6;
  std::printf("%d %llu %d %zu %s %d %d\n", add(1, 2), 2_kb, x, alignof(S), raw, tl, aligned_var);
  static_assert(noexcept(s.f()), "noexcept");
  return 0;
}
