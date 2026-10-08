// Classes are renamed with their constructors and destructors, also in base class initializers
// written with qualified names, explicit destructor calls and new-expressions; a class template
// with its constructor
#include <cstdio>
#include <new>
namespace ns {
class Res {
 public:
  Res() { std::printf("open\n"); }
  ~Res();
  using Self = Res;
};
Res::~Res() { std::printf("close\n"); }
template <class T> struct Box {
  Box() { std::printf("box\n"); }
  T value{};
};
}  // namespace ns
struct Holder : ns::Res, ::ns::Box<int> {
  ns::Res r;
  Holder() : ns::Res(), ::ns::Box<int>(), r() {}
  ~Holder() {
    r.~Res();
    new (&r) ns::Res;
  }
};
int main() {
  ns::Res x;
  x.ns::Res::~Res();
  new (&x) ns::Res;
  Holder h;
  return 0;
}
