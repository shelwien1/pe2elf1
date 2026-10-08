#include <cstdio>
#include <string>
namespace ns {
class Account {
public:
  enum Kind { Checking, Savings };
  typedef long Cents;
  Account(const std::string& n, Kind k) : name(n), kind(k) {}
  Cents balance() const { return total; }
  void deposit(Cents amount, int times = 1) {
    for (int i = 0; i < times; ++i) total += amount;
    if (kind == Savings && this->total > limit) total = limit;
    log("deposit");
  }
  void transfer(Account& to, Cents amount);
  static Cents limit;
private:
  void log(const char* what) { std::printf("%s %s %ld\n", name.c_str(), what, total); }
  std::string name;
  Kind kind;
  Cents total = 0;
};
Account::Cents Account::limit = 1000;
void Account::transfer(Account& to, Cents amount) {
  if (amount > total) amount = total;
  total -= amount;
  to.deposit(amount);
  deposit(0);
}
}
int main() {
  ns::Account a("alice", ns::Account::Checking), b("bob", ns::Account::Savings);
  a.deposit(500, 2);
  ns::Account* pb = &b;
  pb->deposit(300);
  a.transfer(b, 900);
  std::printf("%ld %ld\n", a.balance(), pb->balance());
  return 0;
}
