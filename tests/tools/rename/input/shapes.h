#pragma once
#include <string>
namespace geo {
class Shape {
public:
  Shape() : id(next++) {}
  virtual ~Shape() {}
  virtual double area() const = 0;
  virtual std::string name() const { return "shape"; }
  int id;
  static int next;
};
class Circle : public Shape {
public:
  explicit Circle(double r) : r(r) {}
  double area() const override { return 3.14159 * r * r; }
  std::string name() const override { return "circle"; }
private:
  double r;
};
}
