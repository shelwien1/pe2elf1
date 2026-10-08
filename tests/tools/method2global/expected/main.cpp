#include <cstdio>
#include <string>
namespace ns {
class Account {
public:
  enum Kind { Checking, Savings };
  typedef long Cents;
  Account(const std::string& n, Kind k) : name(n), kind(k) {}
  friend Account::Cents balance(const Account* This);
  friend void deposit(Account* This, Account::Cents amount, int times);
  friend void transfer(Account* This, Account& to, Account::Cents amount);
  static Cents limit;
private:
  void log(const char* what) { std::printf("%s %s %ld\n", name.c_str(), what, total); }
  std::string name;
  Kind kind;
  Cents total = 0;
};

inline void deposit(Account* This, Account::Cents amount, int times = 1) {
  for (int i = 0; i < times; ++i) This->total += amount;
  if (This->kind == Account::Savings && This->total > Account::limit) This->total = Account::limit;
  This->log("deposit");
}

inline Account::Cents balance(const Account* This) { return This->total; }

void transfer(Account* This, Account& to, Account::Cents amount);
Account::Cents Account::limit = 1000;
void transfer(Account* This, Account& to, Account::Cents amount) {
  if (amount > This->total) amount = This->total;
  This->total -= amount;
  deposit(&to, amount);
  deposit(This, 0);
}
}
int main() {
  ns::Account a("alice", ns::Account::Checking), b("bob", ns::Account::Savings);
  deposit(&a, 500, 2);
  ns::Account* pb = &b;
  deposit(pb, 300);
  transfer(&a, b, 900);
  std::printf("%ld %ld\n", balance(&a), balance(pb));
  return 0;
}
