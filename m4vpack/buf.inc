// buf.inc -- cmbase.inc's Buf: a growing array of plain data.

// a growing array of plain data (the hint's counters of a family)
template <class T>
struct Buf {
  T* p;
  size_t n, cap;
  void Init() {
    p = nullptr;
    n = cap = 0;
  }
  void Quit() {
    free(p);
    Init();
  }
  bool empty() const { return !n; }
  size_t size() const { return n; }
  T& operator[](size_t i) { return p[i]; }
  void fill(size_t m, const T& v) {
    if (!grow(&p, &cap, m)) no_memory();
    for (size_t i = 0; i < m; i++) p[i] = v;
    n = m;
  }
};
