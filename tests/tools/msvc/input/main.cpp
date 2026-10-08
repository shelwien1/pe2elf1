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
  Stack<int> stack;
  stack.push(1);
  stack.push(2);
  Account account(100);
  account.deposit(stack.top());
  std::printf("depth %d, top %d, balance %d\n", stack.depth(), stack.top(), account.balance());
  return 0;
}
