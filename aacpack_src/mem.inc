// Memory and arrays, and the few other things aacpack would take from the
// C++ library (it takes none; h264p's mem.inc): mem_alloc() and its kin end the run when
// there is no memory (err_fatal(), err.inc), Buf<T> is a growable array of
// plain data and Map32<V> a map of 32-bit keys; Min() and Max() are
// std::min's and std::max's, and IntTag, Pick, NoRef, SameType and Extent
// what it uses of <type_traits>.
//
// Like aacpack's other structs, a Buf has no constructor or destructor:
// Init() sets it up empty, Quit() frees it (and leaves it empty, set up
// again), and whatever holds one calls them. A Buf is not copied (copy()
// copies its elements), so neither is a struct that holds one.

namespace aacpack {

// the smaller and the larger of two values of a type (b and a where they
// are equal, as std::min and std::max)
template <class T>
INLINE T Min(T a, T b) {
  return b < a ? b : a;
}
template <class T>
INLINE T Max(T a, T b) {
  return a < b ? b : a;
}
INLINE int64_t abs64(int64_t v) { return v < 0 ? -v : v; }

// a type for each int (a function's overload for each value of a template
// argument); T if B, else F; T without its reference; whether two types
// are one; the size of an array type's first dimension (0: not an array)
template <int N>
struct IntTag {};
template <bool B, class T, class F>
struct Pick {
  typedef T type;
};
template <class T, class F>
struct Pick<false, T, F> {
  typedef F type;
};
template <class T>
struct NoRef {
  typedef T type;
};
template <class T>
struct NoRef<T&> {
  typedef T type;
};
template <class A, class B>
struct SameType {
  static const bool value = false;
};
template <class A>
struct SameType<A, A> {
  static const bool value = true;
};
template <class T>
struct Extent {
  static const size_t value = 0;
};
template <class T, size_t N>
struct Extent<T[N]> {
  static const size_t value = N;
};

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
    T* q = p;  // (stores through p could change p, were they bytes: none fill as memset)
    for (size_t i = n; i < m; i++) q[i] = x;
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
    if (!k) return;
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

// A map from 32-bit keys to plain data V (a hash table, open addressing):
// get(k) is k's value, made zero where k is new
template <class V>
struct Map32 {
  struct E {
    uint32_t key, used;
    V v;
  };
  Buf<E> t;  // (a power of 2 of them, more than twice the keys)
  size_t n;  // the keys

  void Init() {
    t.Init();
    n = 0;
  }
  void Quit() { t.Quit(); }
  V& get(uint32_t k) {
    if (2 * (n + 1) > t.size()) grow();
    size_t m = t.size() - 1;
    for (size_t i = (size_t)(((uint64_t)k * 0x9E3779B97F4A7C15ull) >> 40) & m;; i = (i + 1) & m) {
      E& e = t[i];
      if (!e.used) {
        e.key = k, e.used = 1;
        memset((void*)&e.v, 0, sizeof e.v);
        n++;
        return e.v;
      }
      if (e.key == k) return e.v;
    }
  }
  // twice the room, the keys put in again
  void grow() {
    Buf<E> o;
    o.Init();
    o.swap(t);
    t.resize(o.size() ? 2 * o.size() : 16);
    n = 0;
    for (const E& e : o)
      if (e.used) get(e.key) = e.v;
    o.Quit();
  }
};

}  // namespace aacpack
