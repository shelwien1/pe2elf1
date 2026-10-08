#include "shapes.h"
#include <cstdio>
#include <vector>
#include <memory>
int geo::Shape::next = 0;
struct Square : geo::Shape {
  double side;
  Square(double s) : side(s) {}
  double area() const { return side * side; }
};
template <class T> double total(const std::vector<T*>& v) {
  double t = 0;
  for (auto* s : v) t += s->area();
  return t;
}
typedef geo::Shape ShapeBase;
int main() {
  geo::Circle c(1.0);
  Square sq(2.0);
  std::vector<geo::Shape*> all{&c, &sq};
  ShapeBase* first = all[0];
  int count = 0;
  for (int i = 0; i < 2; ++i) { int count = i; (void)count; }
  for (geo::Shape* s : all) { count += s->id; std::printf("%s %.2f\n", s->name().c_str(), s->area()); }
  std::printf("total %.2f %d %s\n", total(all), count, first->name().c_str());
  return 0;
}
