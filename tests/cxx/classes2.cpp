// Operators, conversions, RTTI, member pointers, nested classes, static members
#include <cstdio>
#include <new>
#include <typeinfo>

class Counter {
 public:
  class Step {
   public:
    explicit Step(int s) : size(s) {}
    int size;
  };
  Counter() = default;
  explicit Counter(int start) : value_(start) { ++instances; }
  Counter& operator++() { ++value_; return *this; }
  Counter operator++(int) { Counter old = *this; ++value_; return old; }
  Counter& operator+=(const Step& s) { value_ += s.size; return *this; }
  bool operator<(const Counter& o) const { return value_ < o.value_; }
  bool operator==(const Counter& o) const { return value_ == o.value_; }
  int operator()(int x) const { return value_ * x; }
  explicit operator bool() const { return value_ != 0; }
  operator int() const { return value_; }
  friend Counter operator+(Counter a, const Counter& b) { a.value_ += b.value_; return a; }
  static int instances;
  int get() const { return value_; }
 private:
  int value_ = 0;
};
int Counter::instances = 0;

struct Animal { virtual ~Animal() {} virtual const char* sound() const = 0; };
struct Dog : Animal { const char* sound() const override { return "woof"; } };
struct Cat : Animal { const char* sound() const override { return "meow"; } };

struct V { int v = 1; };
struct L : virtual V {};
struct R : virtual V {};
struct D : L, R { int sum() { return v + 1; } };

union Value { int i; float f; };
struct Flags { unsigned a : 3; unsigned b : 5; };

int apply(int (Counter::*m)() const, const Counter& c) { return (c.*m)(); }

int main() {
  Counter c(5);
  ++c;
  Counter old = c++;
  c += Counter::Step(10);
  Counter d = c + old;
  std::printf("%d %d %d %d %d %d\n", c.get(), old.get(), d.get(), (int)(c < d), c(2), Counter::instances);
  if (c) std::printf("truthy %d\n", static_cast<int>(c));
  Animal* zoo[] = {new Dog, new Cat};
  for (Animal* a : zoo) {
    std::printf("%s %d\n", a->sound(), dynamic_cast<Dog*>(a) != nullptr);
    delete a;
  }
  D dd;
  std::printf("%d %d\n", dd.sum(), typeid(dd) == typeid(D));
  Value val;
  val.i = 0x3f800000;
  Flags fl{5, 17};
  std::printf("%g %u %u\n", val.f, fl.a, fl.b);
  int (Counter::*getter)() const = &Counter::get;
  std::printf("%d\n", apply(getter, c));
  alignas(Counter) unsigned char storage[sizeof(Counter)];
  Counter* placed = new (storage) Counter(42);
  std::printf("%d\n", placed->get());
  placed->~Counter();
  return 0;
}
