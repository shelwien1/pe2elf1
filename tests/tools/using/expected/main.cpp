#include <vector>
#include <cstdio>
template <class T> class Container {
public:
  typedef T value_type;
  std::vector<T> items;
  int size() const { return (int)items.size(); }
  void add(const T& t) { items.push_back(t); }
protected:
  int version = 0;
  template <class U> U convert(const T& t) const { return U(t); }
};
template <class T>
class Stack : public Container<T>
{
public:
  using Container<T>::add;
  using Container<T>::items;
  using Container<T>::size;
  using typename Container<T>::value_type;
protected:
  using Container<T>::version;
public:
  void push(const T& t) { add(t); ++version; }
  T top() const { return items.back(); }
  value_type second() const;
  int depth() const { return size(); }
};
template <class T> typename Stack<T>::value_type Stack<T>::second() const {
  return items[items.size() - 2] + Container<T>::template convert<int>(items[0]) * 0;
}
template <class K, class V> struct Pair { K key; V val; };
template <class V> struct Named : Pair<const char*, V> {
  using Pair<const char*, V>::key;
  void print() const { std::printf("%s\n", key); }
};
template <class T> struct Unused : Container<T> { void f() { add(T()); } };
int main() {
  Stack<int> s; s.push(1); s.push(2);
  Named<int> n; n.key = "name"; n.print();
  std::printf("%d %d %d\n", s.top(), s.second(), s.depth());
  return 0;
}
