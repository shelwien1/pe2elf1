// Templates: class and function templates, and code using the standard library
#include <cstdio>
#include <string>
#include <vector>

template <typename T>
T maximum(T a, T b) {
  return a > b ? a : b;
}

template <class T, int N>
struct Buffer {
  T data[N];
  int size() const { return N; }
  T& operator[](int i) { return data[i]; }
};

int main() {
  std::vector<int> v;
  for (int i = 0; i < 5; i++) v.push_back(i * i);
  Buffer<double, 3> b;
  b[0] = 1.5;
  std::string s = "abc";
  s += "def";
  int total = 0;
  for (int x : v) total += x;
  std::printf("%d %d %g %d %s %zu\n", maximum(3, 7), total, b[0], b.size(), s.c_str(), v.size());
  return 0;
}
