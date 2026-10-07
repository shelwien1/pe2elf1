// Classes, constructors, member functions, operators, references
#include <cstdio>

class Point {
 public:
  Point() : x_(0), y_(0) {}
  Point(int x, int y) : x_(x), y_(y) {}
  int x() const { return x_; }
  int y() const { return y_; }
  Point operator+(const Point& o) const { return Point(x_ + o.x_, y_ + o.y_); }
  Point& operator+=(const Point& o) {
    x_ += o.x_;
    y_ += o.y_;
    return *this;
  }
  static int count;

 private:
  int x_, y_;
};

int Point::count = 0;

struct Shape {
  virtual ~Shape() {}
  virtual int area() const = 0;
};

struct Rect : public Shape {
  Rect(int w, int h) : w(w), h(h) {}
  int area() const override { return w * h; }
  int w, h;
};

static int total(const Shape& s, int n) { return s.area() * n; }

int main() {
  Point a(1, 2), b(3, 4);
  Point c = a + b;
  c += a;
  Point::count++;
  Rect r(3, 5);
  Shape* s = &r;
  int& ref = Point::count;
  ref += 10;
  std::printf("%d %d %d %d %d\n", c.x(), c.y(), s->area(), total(r, 2), Point::count);
  return 0;
}
