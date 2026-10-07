// Function and array types as template arguments (std::function), also in
// references and pointers to const instances
#include <cstdio>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace ns {
struct Foo { int v; };
typedef unsigned long size_type;
}
template <typename T> struct Box { T* p; };

int apply1(const std::function<void(const std::string&)>& f) { f("abc"); return 1; }
int apply2(const std::function<std::vector<int>(int)>& f) { return (int)f(3).size(); }
int apply3(const std::function<int(const ns::Foo&, ns::size_type)>& f) { ns::Foo x{4}; return f(x, 5); }
int apply4(const std::map<int, std::function<int()>>& m) { return m.at(1)(); }
int apply5(const std::function<int(int, ...)>* f) { return f ? 1 : 0; }
int boxes(const Box<int[3]>& a, const Box<Box<int(int)>>& b, Box<int(int)>* const& c) {
  return (a.p == nullptr) + (b.p == nullptr) + (c == nullptr);
}

int main() {
  int r = apply1([](const std::string& s) { std::printf("%s\n", s.c_str()); });
  r += apply2([](int n) { return std::vector<int>(n); });
  r += apply3([](const ns::Foo& f, ns::size_type n) { return f.v + (int)n; });
  std::map<int, std::function<int()>> m;
  m[1] = [] { return 9; };
  r += apply4(m) + apply5(nullptr);
  auto twice = [](const std::function<int(int)>& f, int z) { return f(z) * 2; };
  r += twice([](int y) { return y + 1; }, 20);
  Box<int[3]> a{nullptr};
  Box<Box<int(int)>> b{nullptr};
  std::printf("%d %d\n", r, boxes(a, b, nullptr));
  return 0;
}
