#include "shapes.h"
#include <cstdio>
#include <vector>
#include <memory>
int geometry::Figure::nextId = 0;
struct Square : geometry::Figure {
  double side;
  Square(double s) : side(s) {}
  double surface() const { return side * side; }
};
template <class T> double total(const std::vector<T*>& v) {
  double t = 0;
  for (auto* s : v) t += s->surface();
  return t;
}
typedef geometry::Figure ShapeBase;
int main() {
  geometry::Circle c(1.0);
  Square sq(2.0);
  std::vector<geometry::Figure*> all{&c, &sq};
  ShapeBase* first = all[0];
  int count = 0;
  for (int i = 0; i < 2; ++i) { int inner = i; (void)inner; }
  for (geometry::Figure* s : all) { count += s->id; std::printf("%s %.2f\n", s->name().c_str(), s->surface()); }
  std::printf("total %.2f %d %s\n", total(all), count, first->name().c_str());
  return 0;
}
