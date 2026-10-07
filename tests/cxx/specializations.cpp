// Explicit specializations of static data members, value-initialization with
// explicit default constructors, members of template instances
#include <cstdio>

template <typename T> struct X { static T value; static int count; enum { one = 1, two }; };
template <typename T> T X<T>::value = T(1);
template <typename T> int X<T>::count = 7;
template <> double X<double>::value = 2.5;
template <> int X<char>::count = 9;
template <> long X<long>::value;  // declared, not defined

struct tag_t { explicit tag_t() = default; };
constexpr tag_t tag = tag_t();
int use(tag_t) { return 3; }

template <typename T, long a, long b> struct range { };
template <typename T> struct holder { template <typename R> holder(R) : n(1) {} int n; };
typedef range<int, 1, 10> small_range;

int main() {
  holder<double> h((small_range()));
  std::printf("%g %d %d %d %d %d\n", X<double>::value, X<int>::value, X<char>::count, X<int>::count,
              X<int>::two + use(tag), h.n);
  return 0;
}
