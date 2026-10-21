// Memory and arrays, without the C++ library's containers: mem_alloc()
// and its kin end the run when there is no memory (err_fatal(), err.inc),
// and Buf<T> is a growable array of plain data.
//
// Like h264p's other structs, a Buf has no constructor or destructor:
// Init() sets it up empty, Quit() frees it (and leaves it empty, set up
// again), and whatever holds one calls them. A Buf is not copied (copy()
// copies its elements), so neither is a struct that holds one.

namespace h264p {

void* mem_alloc(size_t n) {
  void* p = malloc(n ? n : 1);
  if (!p) err_fatal("out of memory");
  return p;
}
void* mem_realloc(void* p, size_t n) {
  void* q = realloc(p, n ? n : 1);
  if (!q) err_fatal("out of memory");
  return q;
}
void mem_free(void* p) { free(p); }

template <class T>
struct Buf {
  static_assert(__is_trivially_copyable(T), "a Buf holds plain data");
  T* p;
  size_t n, cap;  // the elements, and the room for them

  Buf() = default;
  Buf(const Buf&) = delete;
  void operator=(const Buf&) = delete;

  void Init() {
    p = nullptr;
    n = cap = 0;
  }
  void Quit() {
    mem_free(p);
    Init();
  }

  size_t size() const { return n; }
  bool empty() const { return !n; }
  T* data() { return p; }
  const T* data() const { return p; }
  T* begin() { return p; }
  T* end() { return p + n; }
  const T* begin() const { return p; }
  const T* end() const { return p + n; }
  T& operator[](size_t i) { return p[i]; }
  const T& operator[](size_t i) const { return p[i]; }
  T& back() { return p[n - 1]; }
  const T& back() const { return p[n - 1]; }

  void clear() { n = 0; }
  // room for m elements (what is there stays)
  void reserve(size_t m) {
    if (m <= cap) return;
    p = (T*)mem_realloc(p, m * sizeof(T));
    cap = m;
  }
  // and room as the elements grow: twice as much each time
  void room(size_t m) {
    if (m > cap) reserve(m > 2 * cap ? m : 2 * cap);
  }
  // m elements, the new ones zero (or v)
  void resize(size_t m) {
    room(m);
    if (m > n) memset((void*)(p + n), 0, (m - n) * sizeof(T));
    n = m;
  }
  void resize(size_t m, const T& v) {
    T x = v;  // (v may be one of the elements)
    room(m);
    for (size_t i = n; i < m; i++) p[i] = x;
    n = m;
  }
  void push_back(const T& v) {
    T x = v;
    room(n + 1);
    p[n++] = x;
  }
  // k elements at the end: q's (not the Buf's own), or k times v
  void append(const T* q, size_t k) {
    room(n + k);
    if (k) memcpy((void*)(p + n), q, k * sizeof(T));
    n += k;
  }
  void append(size_t k, const T& v) { resize(n + k, v); }
  // m elements, all v
  void fill(size_t m, const T& v) {
    n = 0;
    resize(m, v);
  }
  void pop_back() { n--; }
  // q's k elements in place of these
  void assign(const T* q, size_t k) {
    n = 0;
    append(q, k);
  }
  void copy(const Buf& o) { assign(o.p, o.n); }
  // the first k elements gone, the rest moved to the front
  void erase_front(size_t k) {
    if (k < n) memmove((void*)p, p + k, (n - k) * sizeof(T));
    n = k < n ? n - k : 0;
  }
  void swap(Buf& o) {
    T* q = p;
    size_t m = n, c = cap;
    p = o.p, n = o.n, cap = o.cap;
    o.p = q, o.n = m, o.cap = c;
  }
  // the same elements as q's k (byte for byte: integers)
  bool same(const T* q, size_t k) const { return n == k && (!k || !memcmp(p, q, k * sizeof(T))); }
  bool same(const Buf& o) const { return same(o.p, o.n); }
};

}  // namespace h264p
