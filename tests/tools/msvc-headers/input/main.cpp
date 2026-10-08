// Parsed as Visual C++ parses it, with its headers (the test runs when MSVC_DIR is set): code that
// only Visual C++ compiles is renamed and converted, then made standard C++
#include <cstdio>
#include <map>
#include <string>
#include <vector>

#ifdef _MSC_VER
// The data model of Visual C++ for x64, its language and its library
static_assert(sizeof(long double) == 8 && sizeof(long) == 4 && sizeof(wchar_t) == 2, "Visual C++ x64");
static_assert(_MSC_VER >= 1900 && _WIN64 && _MSVC_LANG >= 201402L, "Visual C++");
static_assert(_MSVC_STL_VERSION >= 140, "the Visual C++ library");
#endif

template <class T>
class Container {
 public:
  int size() const { return (int)items.size(); }
  void add(const T& t) { items.push_back(t); }

 protected:
  std::vector<T> items;
};

template <class T>
class Stack : public Container<T> {
 public:
  void push(const T& t) { add(t); }
  T top() const { return items.back(); }
  int depth() const { return size(); }
};

class Account {
 public:
  explicit Account(int balance) : balance_(balance) {}
  void deposit(int amount) { balance_ += amount; }
  int balance() const { return balance_; }

 private:
  int balance_;
};

int main() {
  Stack<std::string> names;
  names.push("first");
  names.push("second");
  std::map<std::string, Account> accounts;
  accounts.emplace(names.top(), Account(100));
  accounts.at("second").deposit(names.depth());
  std::printf("%s: %d\n", names.top().c_str(), accounts.at("second").balance());
  return 0;
}
