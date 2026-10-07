// Modern C++: variadic templates, CRTP, std::function, structured bindings,
// enum class, move semantics, std::optional, constexpr, static_assert
#include <array>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

template <typename T>
T sum(T v) { return v; }
template <typename T, typename... Rest>
T sum(T first, Rest... rest) { return first + sum(rest...); }

template <class Derived>
struct Counter {
  int count() const { return static_cast<const Derived*>(this)->n * 2; }
};
struct Widget : Counter<Widget> { int n = 21; };

enum class Color : unsigned char { Red = 1, Green = 2, Blue = 4 };

constexpr int square(int x) { return x * x; }
static_assert(square(4) == 16, "constexpr");

struct Buffer {
  std::unique_ptr<int[]> data;
  size_t size;
  explicit Buffer(size_t n) : data(new int[n]), size(n) {
    for (size_t i = 0; i < n; ++i) data[i] = static_cast<int>(i);
  }
  Buffer(Buffer&& other) noexcept : data(std::move(other.data)), size(other.size) { other.size = 0; }
  Buffer& operator=(Buffer&&) = default;
  Buffer(const Buffer&) = delete;
};

template <typename K, typename V>
class Registry {
 public:
  void add(const K& k, V v) { items_.emplace(k, std::move(v)); }
  std::optional<V> find(const K& k) const;
 private:
  std::map<K, V> items_;
};

template <typename K, typename V>
std::optional<V> Registry<K, V>::find(const K& k) const {
  auto it = items_.find(k);
  if (it == items_.end()) return std::nullopt;
  return it->second;
}

std::pair<int, std::string> make() { return {7, "seven"}; }

int main() {
  std::printf("%d %g\n", sum(1, 2, 3, 4), sum(1.5, 2.5));
  Widget w;
  std::printf("%d\n", w.count());
  Color c = Color::Green;
  std::printf("%d\n", static_cast<int>(c) | static_cast<int>(Color::Blue));
  Buffer b(5);
  Buffer moved(std::move(b));
  std::printf("%zu %zu %d\n", b.size, moved.size, moved.data[4]);
  Registry<std::string, int> reg;
  reg.add("one", 1);
  reg.add("two", 2);
  auto r = reg.find("two");
  std::printf("%d %d\n", r.has_value() ? *r : -1, reg.find("three").value_or(-1));
  auto [num, name] = make();
  std::printf("%d %s\n", num, name.c_str());
  std::function<int(int)> f = [](int x) { return x * 3; };
  std::array<int, 3> arr{{1, 2, 3}};
  int acc = 0;
  for (auto v : arr) acc += f(v);
  std::unordered_map<std::string, std::vector<int>> groups;
  groups["a"].push_back(1);
  groups["a"].push_back(2);
  std::tuple<int, double, char> t{1, 2.0, 'c'};
  std::printf("%d %zu %d %c\n", acc, groups["a"].size(), std::get<0>(t), std::get<2>(t));
  int* p = nullptr;
  std::printf("%s\n", p == nullptr ? "null" : "set");
  return 0;
}
