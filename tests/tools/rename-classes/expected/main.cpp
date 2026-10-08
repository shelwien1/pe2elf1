// Classes are renamed with their constructors and destructors, also in base class initializers
// written with qualified names, explicit destructor calls and new-expressions; a class template
// with its constructor
#include <cstdio>
#include <new>
namespace ns {
class Resource {
 public:
  Resource() { std::printf("open\n"); }
  ~Resource();
  using Self = Resource;
};
Resource::~Resource() { std::printf("close\n"); }
template <class T> struct Container {
  Container() { std::printf("box\n"); }
  T value{};
};
}  // namespace ns
struct Holder : ns::Resource, ::ns::Container<int> {
  ns::Resource r;
  Holder() : ns::Resource(), ::ns::Container<int>(), r() {}
  ~Holder() {
    r.~Resource();
    new (&r) ns::Resource;
  }
};
int main() {
  ns::Resource x;
  x.ns::Resource::~Resource();
  new (&x) ns::Resource;
  Holder h;
  return 0;
}
