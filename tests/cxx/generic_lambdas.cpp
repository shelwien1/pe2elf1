// Generic lambdas (translated from their source text): sorting, captures, trailing return types
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

struct Holder {
  int base = 10;
  int apply(int x) const {
    auto f = [this](auto v) { return v + base; };  // in a member function, capturing this
    return f(x);
  }
};

int main() {
  std::vector<int> v = {5, 3, 9, 1};
  std::sort(v.begin(), v.end(), [](auto a, auto b) { return a > b; });
  auto twice = [](auto x) { return x + x; };
  int k = 2;
  auto addk = [&k](const auto& x) {
    // a comment with braces { and a quote "
    return x + k + '}' - '}';
  };
  auto multi = [=](auto a,
                   auto b) -> decltype(a * b) {
    return a * b * k;
  };
  std::string s = twice(std::string("ab"));
  std::printf("%d %d %d %d %d %s %d %d %d\n", v[0], v[1], v[2], v[3], twice(21), s.c_str(), addk(1),
              multi(2, 3), Holder().apply(5));
  std::printf("%g\n", twice(1.25));
  return 0;
}
