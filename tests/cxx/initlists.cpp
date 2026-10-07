// Initializer lists, aggregates of classes, and auto variables initialized
// with class temporaries
#include <cstdio>
#include <initializer_list>
#include <map>
#include <string>
#include <utility>
#include <vector>

struct Pr {
  Pr() : x(7), y(0.5) {}
  Pr(int a, double b) : x(a), y(b) {}
  int x;
  double y;
};
struct V {
  V(std::initializer_list<Pr> l) : n(0) { for (const Pr& p : l) n += p.x; }
  int n;
};
struct S { S(int v) : v(v) {} int v; };
struct P { int x, y; };
struct R { int v; R() { v = 3; } R(int a, int b) : v(a + b) {} };

V abc = {{42, 42.0}, {1, 2.0}};
V def{Pr(1, 2.0)};
V ghi = {{}, Pr()};
Pr arr[4] = {{1, 1.0}, Pr(2, 2.0)};
S conv[2] = {1, 2};
std::vector<std::pair<int, double>> pairs = {{42, 42.0}, {3, 1.5}};
std::vector<std::string> strings = {"a", "bc", std::string("def")};
std::map<std::string, int> m = {{"x", 1}, {"yy", 2}};

int sum(std::initializer_list<double> l) {
  double s = 0;
  for (double d : l) s += d;
  return (int)s;
}

int main() {
  std::printf("%d %d %d %d %d %d %d\n", abc.n, def.n, ghi.n, arr[0].x, arr[1].x, arr[2].x, arr[3].x);
  std::printf("%d %zu %d %zu %zu %zu %d\n", conv[1].v, pairs.size(), pairs[1].first, strings.size(),
              strings[2].size(), m.size(), m["yy"]);
  auto a = R();
  auto b = R(1, 2);
  auto c = P{1, 2};
  auto d{R(4, 5)};
  auto e = R{};
  auto f = P();
  const auto g = R(6, 7);
  auto* h = new R(1, 1);
  std::printf("%d %d %d %d %d %d %d %d %d\n", a.v, b.v, c.x + c.y, d.v, e.v, f.x + f.y, g.v, h->v, sum({1.5, 2.5}));
  delete h;
  return 0;
}
