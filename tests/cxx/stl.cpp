// Standard library: streams, containers, algorithms, lambdas, smart pointers, exceptions
#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct Shape {
  virtual ~Shape() {}
  virtual double area() const = 0;
  virtual std::string name() const { return "shape"; }
};

struct Rect : Shape {
  double w, h;
  Rect(double w, double h) : w(w), h(h) {}
  double area() const override { return w * h; }
  std::string name() const override { return "rect"; }
};

struct Circle : Shape {
  double r;
  explicit Circle(double r) : r(r) {}
  double area() const override { return 3.0 * r * r; }
};

int parse(const std::string& s) {
  if (s.empty()) throw std::invalid_argument("empty");
  return std::stoi(s);
}

int main() {
  std::vector<std::unique_ptr<Shape>> shapes;
  shapes.push_back(std::make_unique<Rect>(2, 3));
  shapes.push_back(std::unique_ptr<Shape>(new Circle(1)));
  for (const auto& s : shapes) std::cout << s->name() << " " << s->area() << "\n";

  std::map<std::string, int> counts;
  std::istringstream in("a b a c b a");
  std::string word;
  while (in >> word) counts[word]++;
  for (auto& kv : counts) std::cout << kv.first << "=" << kv.second << " ";
  std::cout << std::endl;

  std::vector<int> v = {5, 3, 9, 1};
  std::sort(v.begin(), v.end(), [](int a, int b) { return a > b; });
  int base = 10;
  std::for_each(v.begin(), v.end(), [&base](int x) { base += x; });
  std::cout << v[0] << " " << base << "\n";

  try {
    parse("");
  } catch (const std::invalid_argument& e) {
    std::cout << "caught " << e.what() << "\n";
  }
  std::cout << parse("42") << "\n";
  return 0;
}
