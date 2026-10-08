#include <vector>
template <class T> struct Base {
  int x;
  void f(int) {}
  typedef T value_type;
  static int s;
protected:
  int y;
};
template <class T> int Base<T>::s = 0;
template <class T> class Derived : public Base<T> {
  int own;
public:
  void g() { x = 1; f(2); y = x + s; own = 0; value_type v = T(); (void)v; }
  int h() const { return this->x + Base<T>::y; }
};
struct Plain { int z; };
template <class T> struct D2 : Plain, Base<T> { int k() { return z + x; } };
int main() { Derived<int> d; d.g(); D2<long> e; return d.h() + e.k(); }
