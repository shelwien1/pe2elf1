#pragma once
#include <string>
namespace geometry {
class Figure {
public:
  Figure() : id(nextId++) {}
  virtual ~Figure() {}
  virtual double surface() const = 0;
  virtual std::string name() const { return "shape"; }
  int id;
  static int nextId;
};
class Circle : public Figure {
public:
  explicit Circle(double r) : r(r) {}
  double surface() const override { return 3.14159 * r * r; }
  std::string name() const override { return "circle"; }
private:
  double r;
};
}
