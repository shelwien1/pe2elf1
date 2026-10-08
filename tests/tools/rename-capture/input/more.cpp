// More renamings after which a name would refer to another entity
#include <cstdio>
#include <initializer_list>

// The calls of a function template are references to its instances
template <class T> T maximum(T a, T b) { return a > b ? a : b; }

// A template parameter may not be declared again in its template
template <class T> T twice(T t) { T sum = t + t; return sum; }

// The members of an anonymous union are members of the enclosing class
struct Value {
  int kind;
  union { int i; double d; };
  int get() const { return kind == 0 ? i : (int)d; }
};

// The outermost block of the body of a for statement is in the scope of its init-statement (or
// range declaration), that of an if statement in the scope of its condition
int loops() {
  int s = 0;
  for (int i = 0; i < 3; ++i) { int step = 1; s += step; }
  for (int j : {1, 2}) { int add = 2; s += add + j; }
  if (int c = s) { int d = 1; s += d + c; }
  return s;
}

// A member may not have the name of its class
struct Counter { int value = 4; int count() const { return value; } };

// The name of a class is declared in its scope, so in those of the classes derived from it too
namespace shapes { struct Unit { int size = 1; }; }
namespace lib { template <class T> struct Box { T value = 2; }; }
struct Crate : lib::Box<int> { shapes::Unit unit; int weight() const { return value + unit.size; } };

// A non-type template parameter
template <int N> struct Grid { int cells(int n) const { return N * n; } };

// A constructor template is renamed with its class
template <class T> struct Holder { template <class U> Holder(U u) : held(T(u)) {} T held; };

int main() {
  int total = maximum(1, 2);
  Value v{0, {5}};
  std::printf("%d %d %d %d %d\n", total, twice(3), v.get(), loops(), Counter().count());
  std::printf("%d %d %d\n", Crate().weight(), Grid<3>().cells(2), Holder<int>(2.5).held);
  return 0;
}
