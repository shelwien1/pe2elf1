// Templates in namespaces, member templates, alias templates, template template
// parameters, default template arguments, inheriting and delegating constructors
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace geo {
template <typename T>
struct Vec2 {
  T x, y;
  Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
  template <typename U> Vec2<U> cast() const { return {static_cast<U>(x), static_cast<U>(y)}; }
};
template <typename T>
std::ostream& operator<<(std::ostream& os, const Vec2<T>& v) { return os << "(" << v.x << "," << v.y << ")"; }
using Vec2d = Vec2<double>;
}  // namespace geo

template <class T> using Table = std::map<std::string, std::vector<T>>;

template <template <class...> class Container, class T>
Container<T> fill(int n, T v) { return Container<T>(n, v); }

template <class T, int N = 3>
struct Fixed { T v[N]; int size() const { return N; } };

struct Base { int b; explicit Base(int v) : b(v) {} virtual ~Base() = default; };
struct Derived : Base {
  using Base::Base;
  Derived() : Derived(9) {}
};

int main() {
  geo::Vec2<int> a{1, 2}, b{3, 4};
  geo::Vec2d d = (a + b).cast<double>();
  std::cout << a + b << " " << d << std::endl;
  Table<int> t;
  t["k"].push_back(5);
  std::vector<int> f = fill<std::vector>(3, 7);
  Fixed<char> fx;
  Derived dd, de(4);
  std::cout << t["k"][0] << " " << f.size() << " " << fx.size() << " " << dd.b << " " << de.b << std::endl;
  return 0;
}
