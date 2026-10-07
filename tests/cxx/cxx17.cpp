// C++17 statements, GNU vector types, and name qualification corner cases
#include <cstdio>

int counter = 0;
int next() { return ++counter; }

// if and switch with initialization statements
int selections(int x) {
  int r = 0;
  if (int c = x * 2; c > 4) r += c; else r -= c;
  if (int c = next(); int d = c - 1) r += d;
  if (x++; x > 3) r += 100;
  switch (int s = x % 3; s) {
    case 0: r += 1000; break;
    case 1: r += 2000; break;
    default: r += 3000; break;
  }
  return r;
}

// decltype of variables and unnamed types in function bodies
struct S { static enum { No, Yes } state; };
decltype(S::state) S::state = S::Yes;
int decltypes() {
  decltype(S::state) a = S::No;
  decltype(a) b = S::Yes;
  return a + 2 * b;
}

// GNU vector types (operations with scalars are implicitly widened)
typedef int v4si __attribute__((vector_size(16)));
int vectors() {
  v4si a = {1, 2, 3, 4};
  v4si b = a * 2 + 1;
  int __attribute__((vector_size(8))) c = {5, 6};
  return b[0] + b[3] + c[1];
}

// member typedefs of the same name and type in different classes
namespace lib {
typedef unsigned long size_t;
template <typename T> struct tree { typedef size_t size_type; size_type count(const T&) const; };
template <typename T> typename tree<T>::size_type tree<T>::count(const T&) const { size_type n = 1; return n; }
template <typename T> struct vec { typedef size_t size_type; };
}
int typedefs() {
  const lib::vec<char*>::size_type n = 3;
  return (int)n + (int)lib::tree<int>().count(0);
}

// new-expressions whose type has a declarator
float half() { return 0.5f; }
float news() {
  typedef float (*fp)();
  fp* p = new fp(half);
  float (**q)() = new (float (*)());
  *q = half;
  float r = (*p)() + (*q)();
  delete p;
  delete q;
  return r;
}

// qualified calls of members of different bases with the same name
struct A { int f() { return 1; } int i = 10; };
struct B { int f() { return 2; } int i = 20; };
struct C : A, B {
  int g() { return A::f() * 10 + B::f() + A::i + B::i; }
};

// names in nontype template arguments of member templates
class T {
  static int twice(int x) { return 2 * x; }
 public:
  template <int (*F)(int)> struct apply { int operator()(int x) const { return F(x); } };
  apply<twice> make();
};
T::apply<&T::twice> T::make() { return apply<twice>(); }
struct M { int get() const { return 3; } };
template <int (M::*G)() const> struct getter { int operator()(const M& m) const { return (m.*G)(); } };

// protected member typedefs in template arguments of shared instances
template <typename X> struct holder { typedef X type; X value; };
template <typename X> class wrapper {
 protected:
  typedef X hidden;
 public:
  typedef typename holder<hidden>::type visible;
};
holder<wrapper<int>::visible> held = {42};

int main() {
  std::printf("%d %d %d\n", selections(1), selections(5), counter);
  std::printf("%d %d %d\n", decltypes(), vectors(), typedefs());
  std::printf("%g %d %d %d\n", news(), C().g(), T().make()(21) + getter<&M::get>()(M()), held.value);
  return 0;
}
