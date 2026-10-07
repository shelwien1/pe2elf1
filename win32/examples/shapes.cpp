// Example for test.bat: a C++17 program with classes, templates, lambdas and the standard library
#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

class Shape {
 public:
  virtual ~Shape() = default;
  virtual double area() const = 0;
  virtual std::string name() const = 0;
};

class Rectangle : public Shape {
 public:
  Rectangle(double w, double h) : w_(w), h_(h) {}
  double area() const override { return w_ * h_; }
  std::string name() const override { return "rectangle"; }

 private:
  double w_, h_;
};

class Circle : public Shape {
 public:
  explicit Circle(double r) : r_(r) {}
  double area() const override { return 3.14159265358979 * r_ * r_; }
  std::string name() const override { return "circle"; }

 private:
  double r_;
};

template <typename Container, typename Function>
double total(const Container& items, Function f) {
  return std::accumulate(items.begin(), items.end(), 0.0,
                         [&](double sum, const auto& item) { return sum + f(*item); });
}

int main() {
  std::vector<std::unique_ptr<Shape>> shapes;
  shapes.push_back(std::make_unique<Rectangle>(3, 4));
  shapes.push_back(std::make_unique<Circle>(1));
  shapes.push_back(std::make_unique<Rectangle>(1, 2));
  shapes.push_back(std::make_unique<Circle>(2));
  std::sort(shapes.begin(), shapes.end(), [](const auto& a, const auto& b) { return a->area() < b->area(); });

  std::map<std::string, int> counts;
  for (const auto& shape : shapes) {
    std::cout << shape->name() << " " << shape->area() << "\n";
    ++counts[shape->name()];
  }
  for (const auto& [name, count] : counts) std::cout << count << " " << name << "s\n";
  std::cout << "total area " << total(shapes, [](const Shape& s) { return s.area(); }) << "\n";
  return 0;
}
