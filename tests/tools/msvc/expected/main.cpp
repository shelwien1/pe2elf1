// Example for test.bat: the refactoring tools change this program, which then prints the same.
// As written, Microsoft C++ compiles it and g++ does not: Stack uses the members of its base class
// Container<T> without qualification, which rose-using fixes with using-declarations.
#include <cstdio>
#include <vector>

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
  using Container<T>::add;
  using Container<T>::size;
protected:
  using Container<T>::items;
 public:
  void push(const T& t) { add(t); }
  T top() const { return items.back(); }
  int depth() const { return size(); }
};

class Account {
 public:
  explicit Account(int balance) : balance_(balance) {}
  friend void deposit(Account* This, int amount);
  int getBalance() const { return balance_; }

 private:
  int balance_;
};

inline void deposit(Account* This, int amount) { This->balance_ += amount; }

int main() {
  Stack<int> stack;
  stack.push(1);
  stack.push(2);
  Account account(100);
  deposit(&account, stack.top());
  std::printf("depth %d, top %d, balance %d\n", stack.depth(), stack.top(), account.getBalance());
  return 0;
}
