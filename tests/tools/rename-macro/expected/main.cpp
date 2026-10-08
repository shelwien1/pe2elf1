#define GETTER(name) int name() const { return v; }
struct X { int v = 1; int size() const { return 1; } GETTER(val) };
struct Y { int size() const { return 2; } int length() const { return 3; } };
template <class T> int get(const T& t) { return t.size(); }
int main() { X x; return get(x) + get(Y()) + x.val() + Y().length(); }
