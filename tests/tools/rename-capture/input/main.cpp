// Renamings after which a name would refer to another entity, and renamings after which none does
#include <cstdio>

#define LIMIT 100

int counter = 1;
int total = 0;

// A local variable would capture the global
int bump() { int step = 2; counter += step; return counter; }

// Variables declared in the same scope: two locals, a parameter and a local of the function body
int pair() { int first = 3, second = 4; return first * 10 + second; }
int plusOne(int in) { int out = in + 1; return out; }

// A variable of an inner block would capture the outer one, unless declared after the use
int nested() { int outer = 1; { int inner = 2; outer += inner; } return outer; }
int ordered() { int early = 1; { early += 1; int late = 2; (void)late; } return early; }

// A loop variable, a lambda parameter
int loop() { int sum = 0; for (int i = 0; i < 3; ++i) sum += i; return sum; }
int lambda() { int base = 1; auto add = [&](int delta) { return base + delta; }; return add(2); }

// The global would be hidden by the renamed local where it is used
int reverse() { int local = 5; total = local; return total; }

// Members: of a base class (they are found before the global), hidden by a member of a derived
// class
struct Base { int m = 5; int h() const { return 6; } };
struct Derived : Base {
  int n = 7;
  int k() const { return 8; }
  int sum() const { return m + n + h() + k() + counter; }
};

// The member that a constructor initializes is looked up in the class (the parameters are not
// seen there)
struct Point {
  int px, py;
  Point(int x, int y) : px(x), py(y) { py += x; }
  int sum() const { return px * 10 + py; }
};

// A namespace member would capture the global
namespace ns { int value = 9; int get() { return total + value; } }

// A template parameter would capture the global
template <class T> T scaled(T t) { return t * (T)counter; }

// A qualified name is not looked up in the scopes around it
int q = 10;
int qualified() { int r = 11; return r + ::q; }

// Labels are apart from the other names
int labels() { int n = 0; again: if (++n < 3) goto again; goto done; done: return n; }

// A macro
int limited() { return counter < LIMIT ? counter : LIMIT; }

int main() {
  std::printf("%d %d %d %d %d %d %d %d %d\n", bump(), pair(), plusOne(1), nested(), ordered(), loop(),
              lambda(), reverse(), Derived().sum());
  std::printf("%d %d %d %d %d %d\n", Point(1, 2).sum(), ns::get(), scaled(3), qualified(), labels(), limited());
  return 0;
}
