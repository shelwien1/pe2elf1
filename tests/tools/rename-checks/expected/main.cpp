#define GETTER(name) int name() const { return v; }
struct X { int v = 1; int size() const { return 1; } GETTER(value) };
struct Y { int size() const { return 2; } int len() const { return 3; } };
template <class T> int get(const T& t) { return t.size(); }
int main() { X x; return get(x) + get(Y()) + x.value() + Y().len(); }
#define CALL_VALUE(o) (o).value()
int other() { X x; return CALL_VALUE(x); }
