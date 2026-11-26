// base.inc -- the small part of a C++ standard library that pdfdump uses, built on the C library
// and the OS only (no C++ library headers). Names and semantics follow std:: where they exist, so
// lib::x reads as std::x; only what the program calls is here.
//
// Allocation: operator new is replaced below; an allocation that cannot be met prints a message
// and exits (the build has no exceptions).

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX  // (no min/max macros: lib::min, lib::max)
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#endif

#if defined(_MSC_VER)
#define LIB_INLINE __forceinline
#define LIB_NOINLINE __declspec(noinline)
#else
#define LIB_INLINE inline __attribute__((always_inline))
#define LIB_NOINLINE __attribute__((noinline))
#endif

namespace lib {

[[noreturn]] inline void outOfMemory() {
  fputs("pdfdump: out of memory\n", stderr);
  _Exit(1);
}

// ---- moves and small algorithms ---------------------------------------------------------------
template <class T> struct remove_ref { typedef T type; };
template <class T> struct remove_ref<T&> { typedef T type; };
template <class T> struct remove_ref<T&&> { typedef T type; };

template <class T>
constexpr typename remove_ref<T>::type&& move(T&& t) noexcept {
  return static_cast<typename remove_ref<T>::type&&>(t);
}
template <class T>
constexpr T&& forward(typename remove_ref<T>::type& t) noexcept {
  return static_cast<T&&>(t);
}
template <class T>
constexpr T&& forward(typename remove_ref<T>::type&& t) noexcept {
  return static_cast<T&&>(t);
}
template <class T>
inline void swap(T& a, T& b) {
  T t(lib::move(a));
  a = lib::move(b);
  b = lib::move(t);
}
template <class T>
constexpr const T& min(const T& a, const T& b) {
  return b < a ? b : a;
}
template <class T>
constexpr const T& max(const T& a, const T& b) {
  return a < b ? b : a;
}

template <class A, class B>
struct pair {
  A first;
  B second;
  pair() : first(), second() {}
  pair(const A& a, const B& b) : first(a), second(b) {}
  template <class U, class V>
  pair(U&& a, V&& b) : first(lib::forward<U>(a)), second(lib::forward<V>(b)) {}
  bool operator==(const pair& o) const { return first == o.first && second == o.second; }
  bool operator!=(const pair& o) const { return !(*this == o); }
  bool operator<(const pair& o) const { return first < o.first || (!(o.first < first) && second < o.second); }
};

template <class T, size_t N>
struct array {
  T v[N];
  T& operator[](size_t i) { return v[i]; }
  const T& operator[](size_t i) const { return v[i]; }
  T* begin() { return v; }
  T* end() { return v + N; }
  const T* begin() const { return v; }
  const T* end() const { return v + N; }
  static constexpr size_t size() { return N; }
};

struct less {
  template <class T>
  bool operator()(const T& a, const T& b) const { return a < b; }
};

// Stable merge sort (the order of equal elements is kept, on every platform): sort and
// stable_sort are both this. Scratch space as large as the range; short runs by insertion.
template <class T, class C>
void mergeSortRec(T* a, T* tmp, size_t n, C& lt) {
  if (n <= 16) {
    for (size_t i = 1; i < n; i++) {
      if (!lt(a[i], a[i - 1])) continue;
      T x(lib::move(a[i]));
      size_t j = i;
      do { a[j] = lib::move(a[j - 1]); } while (--j > 0 && lt(x, a[j - 1]));
      a[j] = lib::move(x);
    }
    return;
  }
  size_t h = n / 2;
  mergeSortRec(a, tmp, h, lt);
  mergeSortRec(a + h, tmp, n - h, lt);
  if (!lt(a[h], a[h - 1])) return;  // already in order
  for (size_t i = 0; i < h; i++) tmp[i] = lib::move(a[i]);
  size_t i = 0, j = h, k = 0;
  while (i < h && j < n) a[k++] = lt(a[j], tmp[i]) ? lib::move(a[j++]) : lib::move(tmp[i++]);
  while (i < h) a[k++] = lib::move(tmp[i++]);
}
template <class T, class C>
void stable_sort(T* first, T* last, C lt) {
  size_t n = (size_t)(last - first);
  if (n < 2) return;
  T* tmp = new T[(n + 1) / 2];
  mergeSortRec(first, tmp, n, lt);
  delete[] tmp;
}
template <class T>
void stable_sort(T* first, T* last) { stable_sort(first, last, less()); }
template <class T, class C>
void sort(T* first, T* last, C lt) { stable_sort(first, last, lt); }
template <class T>
void sort(T* first, T* last) { stable_sort(first, last, less()); }

template <class T, class V>
T* lower_bound(T* first, T* last, const V& v) {
  size_t n = (size_t)(last - first);
  while (n > 0) {
    size_t h = n / 2;
    if (first[h] < v) { first += h + 1; n -= h + 1; }
    else n = h;
  }
  return first;
}
template <class T, class V>
T* upper_bound(T* first, T* last, const V& v) {
  size_t n = (size_t)(last - first);
  while (n > 0) {
    size_t h = n / 2;
    if (!(v < first[h])) { first += h + 1; n -= h + 1; }
    else n = h;
  }
  return first;
}
template <class T, class V>
T* find(T* first, T* last, const V& v) {
  for (; first != last; ++first)
    if (*first == v) return first;
  return last;
}
// first occurrence of [b, be) in [a, ae), or ae
template <class T, class U>
T* search(T* a, T* ae, const U* b, const U* be) {
  size_t m = (size_t)(be - b);
  if (m == 0) return a;
  for (; (size_t)(ae - a) >= m; ++a) {
    size_t k = 0;
    while (k < m && a[k] == b[k]) k++;
    if (k == m) return a;
  }
  return ae;
}
template <class T, class V>
void fill(T* first, T* last, const V& v) {
  for (; first != last; ++first) *first = v;
}

// ---- vector ---------------------------------------------------------------------------------
template <class A, class B> struct is_same { enum { v = 0 }; };
template <class A> struct is_same<A, A> { enum { v = 1 }; };
// element types compared with memcmp
template <class T> struct is_bytewise { enum { v = 0 }; };
template <> struct is_bytewise<char> { enum { v = 1 }; };
template <> struct is_bytewise<signed char> { enum { v = 1 }; };
template <> struct is_bytewise<unsigned char> { enum { v = 1 }; };
template <> struct is_bytewise<short> { enum { v = 1 }; };
template <> struct is_bytewise<unsigned short> { enum { v = 1 }; };
template <> struct is_bytewise<int> { enum { v = 1 }; };
template <> struct is_bytewise<unsigned> { enum { v = 1 }; };
template <> struct is_bytewise<long> { enum { v = 1 }; };
template <> struct is_bytewise<unsigned long> { enum { v = 1 }; };
template <> struct is_bytewise<long long> { enum { v = 1 }; };
template <> struct is_bytewise<unsigned long long> { enum { v = 1 }; };

}  // namespace lib
namespace lib {
struct PlaceTag {};
}
inline void* operator new(size_t, void* p, lib::PlaceTag) noexcept { return p; }
inline void operator delete(void*, void*, lib::PlaceTag) noexcept {}
namespace lib {

// A growable array: std::vector's interface as far as pdfdump uses it. Iterators are pointers.
// Trivially copyable elements move with memcpy/realloc; others are moved one by one.
template <class T>
class vector {
  static constexpr bool kTrivial = __is_trivially_copyable(T);
  static constexpr bool kZeroInit = __is_trivially_constructible(T) && __is_trivially_copyable(T);

 public:
  typedef T value_type;
  typedef T* iterator;
  typedef const T* const_iterator;

  vector() {}
  // (explicit, all of them: a braced list like {a, b} must not quietly mean "a copies of b" or a
  // range; there is no initializer-list constructor, so such lists do not compile)
  explicit vector(size_t n) { resize(n); }
  explicit vector(size_t n, const T& v) { assign(n, v); }
  template <class U>
  explicit vector(const U* a, const U* b) { assign(a, b); }
  template <class U>
  explicit vector(U* a, U* b) { assign((const U*)a, (const U*)b); }
  vector(const vector& o) { assign(o.p_, o.p_ + o.n_); }
  vector(vector&& o) noexcept : p_(o.p_), n_(o.n_), cap_(o.cap_) {
    o.p_ = nullptr;
    o.n_ = o.cap_ = 0;
  }
  ~vector() {
    destroy(p_, p_ + n_);
    free(p_);
  }
  vector& operator=(const vector& o) {
    if (this != &o) assign(o.p_, o.p_ + o.n_);
    return *this;
  }
  vector& operator=(vector&& o) noexcept {
    if (this != &o) {
      destroy(p_, p_ + n_);
      free(p_);
      p_ = o.p_;
      n_ = o.n_;
      cap_ = o.cap_;
      o.p_ = nullptr;
      o.n_ = o.cap_ = 0;
    }
    return *this;
  }

  size_t size() const { return n_; }
  size_t capacity() const { return cap_; }
  bool empty() const { return n_ == 0; }
  T* data() { return p_; }
  const T* data() const { return p_; }
  T* begin() { return p_; }
  T* end() { return p_ + n_; }
  const T* begin() const { return p_; }
  const T* end() const { return p_ + n_; }
  T& operator[](size_t i) { return p_[i]; }
  const T& operator[](size_t i) const { return p_[i]; }
  T& front() { return p_[0]; }
  const T& front() const { return p_[0]; }
  T& back() { return p_[n_ - 1]; }
  const T& back() const { return p_[n_ - 1]; }

  void reserve(size_t m) {
    if (m > cap_) setCap(m);
  }
  void shrink_to_fit() {
    if (cap_ > n_) setCap(n_);
  }
  void clear() {
    destroy(p_, p_ + n_);
    n_ = 0;
  }
  LIB_INLINE void push_back(const T& v) {
    if (n_ < cap_) {
      new (p_ + n_, PlaceTag()) T(v);
      n_++;
    } else {
      emplaceSlow(v);
    }
  }
  LIB_INLINE void push_back(T&& v) {
    if (n_ < cap_) {
      new (p_ + n_, PlaceTag()) T(lib::move(v));
      n_++;
    } else {
      emplaceSlow(lib::move(v));
    }
  }
  template <class... A>
  T& emplace_back(A&&... a) {
    if (n_ < cap_) {
      new (p_ + n_, PlaceTag()) T(lib::forward<A>(a)...);
      n_++;
    } else {
      emplaceSlow(lib::forward<A>(a)...);
    }
    return p_[n_ - 1];
  }
  void pop_back() {
    n_--;
    p_[n_].~T();
  }
  void resize(size_t m) {
    if (m <= n_) {
      destroy(p_ + m, p_ + n_);
    } else {
      if (m > cap_) setCap(newCap(m));
      if (kZeroInit) memset((void*)(p_ + n_), 0, (m - n_) * sizeof(T));
      else
        for (size_t i = n_; i < m; i++) new (p_ + i, PlaceTag()) T();
    }
    n_ = m;
  }
  void resize(size_t m, const T& v) {
    if (m <= n_) {
      resize(m);
      return;
    }
    T t(v);  // (v may be an element)
    if (m > cap_) setCap(newCap(m));
    for (size_t i = n_; i < m; i++) new (p_ + i, PlaceTag()) T(t);
    n_ = m;
  }
  void assign(size_t m, const T& v) {
    T t(v);
    clear();
    if (m > cap_) setCap(m);
    for (size_t i = 0; i < m; i++) new (p_ + i, PlaceTag()) T(t);
    n_ = m;
  }
  template <class U>
  void assign(const U* a, const U* b) {
    if (aliases(a)) {
      vector t(a, b);
      swap(t);
      return;
    }
    clear();
    size_t k = (size_t)(b - a);
    if (k > cap_) setCap(k);
    construct(p_, a, k);
    n_ = k;
  }
  template <class U>
  void assign(U* a, U* b) { assign((const U*)a, (const U*)b); }

  // insert the range [a, b) at pos; returns the first inserted element
  template <class U>
  T* insert(const T* pos, const U* a, const U* b) {
    size_t off = (size_t)(pos - p_), k = (size_t)(b - a);
    if (k == 0) return p_ + off;
    if (aliases(a)) {
      if (off == n_ && is_same<T, U>::v) {
        // an append of our own elements (an LZ77 copy): the source keeps its offset across a
        // reallocation, and lies below the new elements
        size_t src = (size_t)((const T*)(const void*)a - p_);
        if (n_ + k > cap_) setCap(newCap(n_ + k));
        construct(p_ + n_, (const U*)(const void*)(p_ + src), k);
        n_ += k;
        return p_ + off;
      }
      vector t(a, b);
      return insert(p_ + off, t.p_, t.p_ + k);
    }
    openGap(off, k);
    construct(p_ + off, a, k);
    n_ += k;
    return p_ + off;
  }
  template <class U>
  T* insert(const T* pos, U* a, U* b) { return insert(pos, (const U*)a, (const U*)b); }
  T* insert(const T* pos, size_t k, const T& v) {
    size_t off = (size_t)(pos - p_);
    if (k == 0) return p_ + off;
    T t(v);
    openGap(off, k);
    for (size_t i = 0; i < k; i++) new (p_ + off + i, PlaceTag()) T(t);
    n_ += k;
    return p_ + off;
  }
  T* insert(const T* pos, const T& v) {
    T t(v);
    return insert(pos, &t, &t + 1);
  }
  T* erase(const T* a, const T* b) {
    size_t off = (size_t)(a - p_), k = (size_t)(b - a);
    if (k == 0) return p_ + off;
    if (kTrivial) {
      memmove((void*)(p_ + off), (const void*)(p_ + off + k), (n_ - off - k) * sizeof(T));
    } else {
      for (size_t i = off; i + k < n_; i++) p_[i] = lib::move(p_[i + k]);
      destroy(p_ + n_ - k, p_ + n_);
    }
    n_ -= k;
    return p_ + off;
  }
  T* erase(const T* pos) { return erase(pos, pos + 1); }
  void swap(vector& o) {
    lib::swap(p_, o.p_);
    lib::swap(n_, o.n_);
    lib::swap(cap_, o.cap_);
  }
  bool operator==(const vector& o) const {
    if (n_ != o.n_) return false;
    if (is_bytewise<T>::v) return n_ == 0 || memcmp(p_, o.p_, n_ * sizeof(T)) == 0;
    for (size_t i = 0; i < n_; i++)
      if (!(p_[i] == o.p_[i])) return false;
    return true;
  }
  bool operator!=(const vector& o) const { return !(*this == o); }

 private:
  T* p_ = nullptr;
  size_t n_ = 0, cap_ = 0;

  template <class U>
  bool aliases(const U* a) const {  // a points at one of our elements
    const void* q = (const void*)a;
    return p_ && q >= (const void*)p_ && q < (const void*)(p_ + n_);
  }
  // doubling, from at least 64 bytes' worth (large elements: from one)
  static constexpr size_t kMinCap = sizeof(T) >= 64 ? 1 : 64 / sizeof(T);
  size_t newCap(size_t need) const { return lib::max(need, lib::max(cap_ * 2, kMinCap)); }
  static T* allocate(size_t c) {
    if (c > (size_t)-1 / sizeof(T)) outOfMemory();
    T* p = (T*)malloc(c * sizeof(T));
    if (!p && c) outOfMemory();
    return p;
  }
  // capacity c >= n_
  void setCap(size_t c) {
    if (kTrivial) {
      if (c == 0) {
        free(p_);
        p_ = nullptr;
      } else {
        if (c > (size_t)-1 / sizeof(T)) outOfMemory();
        T* p = (T*)realloc((void*)p_, c * sizeof(T));
        if (!p) outOfMemory();
        p_ = p;
      }
    } else {
      T* p = allocate(c);
      for (size_t i = 0; i < n_; i++) {
        new (p + i, PlaceTag()) T(lib::move(p_[i]));
        p_[i].~T();
      }
      free(p_);
      p_ = p;
    }
    cap_ = c;
  }
  template <class... A>
  LIB_NOINLINE void emplaceSlow(A&&... a) {
    // the new element is made before the old storage goes: the arguments may refer into it
    size_t c = newCap(n_ + 1);
    T* p = allocate(c);
    new (p + n_, PlaceTag()) T(lib::forward<A>(a)...);
    if (kTrivial) {
      if (n_) memcpy((void*)p, (const void*)p_, n_ * sizeof(T));
    } else {
      for (size_t i = 0; i < n_; i++) {
        new (p + i, PlaceTag()) T(lib::move(p_[i]));
        p_[i].~T();
      }
    }
    free(p_);
    p_ = p;
    cap_ = c;
    n_++;
  }
  // k uninitialized slots at off (elements after off moved up; n_ unchanged)
  void openGap(size_t off, size_t k) {
    if (n_ + k > cap_) setCap(newCap(n_ + k));
    if (off == n_) return;
    if (kTrivial) {
      memmove((void*)(p_ + off + k), (const void*)(p_ + off), (n_ - off) * sizeof(T));
    } else {
      for (size_t i = n_; i-- > off;) {
        new (p_ + i + k, PlaceTag()) T(lib::move(p_[i]));
        p_[i].~T();
      }
    }
  }
  template <class U>
  static void construct(T* d, const U* a, size_t k) {
    if (is_same<T, U>::v && kTrivial) {
      if (k) memcpy((void*)d, (const void*)a, k * sizeof(T));
    } else {
      for (size_t i = 0; i < k; i++) new (d + i, PlaceTag()) T(a[i]);
    }
  }
  static void destroy(T* a, T* b) {
    if (!kTrivial)
      for (; a != b; ++a) a->~T();
  }
};

// ---- string ---------------------------------------------------------------------------------
// Bytes with a NUL after them; up to 15 kept in place (no allocation), as libstdc++ does.
class string {
 public:
  static constexpr size_t npos = (size_t)-1;

  string() : p_(buf_) { buf_[0] = 0; }
  string(const char* s) { init(s, strlen(s)); }
  string(const char* s, size_t n) { init(s, n); }
  explicit string(size_t n, char c) {
    init(nullptr, 0);
    resize(n, c);
  }
  template <class U>
  string(const U* a, const U* b) { init((const char*)a, (size_t)(b - a)); }
  template <class U>
  string(U* a, U* b) { init((const char*)a, (size_t)(b - a)); }
  string(const string& o) { init(o.p_, o.n_); }
  string(string&& o) noexcept { take(o); }
  ~string() {
    if (!small()) free(p_);
  }
  string& operator=(const string& o) {
    if (this != &o) assign(o.p_, o.n_);
    return *this;
  }
  string& operator=(string&& o) noexcept {
    if (this != &o) {
      if (!small()) free(p_);
      take(o);
    }
    return *this;
  }
  string& operator=(const char* s) { return assign(s, strlen(s)); }

  size_t size() const { return n_; }
  size_t length() const { return n_; }
  bool empty() const { return n_ == 0; }
  const char* data() const { return p_; }
  char* data() { return p_; }
  const char* c_str() const { return p_; }
  char* begin() { return p_; }
  char* end() { return p_ + n_; }
  const char* begin() const { return p_; }
  const char* end() const { return p_ + n_; }
  char& operator[](size_t i) { return p_[i]; }
  const char& operator[](size_t i) const { return p_[i]; }
  char& back() { return p_[n_ - 1]; }
  const char& back() const { return p_[n_ - 1]; }
  char& front() { return p_[0]; }

  void reserve(size_t m) {
    if (m > cap()) setCap(m);
  }
  void clear() {
    n_ = 0;
    p_[0] = 0;
  }
  void resize(size_t m, char c = 0) {
    if (m > n_) {
      if (m > cap()) setCap(lib::max(m, cap() * 2));
      memset(p_ + n_, c, m - n_);
    }
    n_ = m;
    p_[n_] = 0;
  }
  LIB_INLINE void push_back(char c) {
    if (n_ == cap()) setCap(lib::max((size_t)31, cap() * 2));
    p_[n_++] = c;
    p_[n_] = 0;
  }
  void pop_back() { p_[--n_] = 0; }
  string& assign(const char* s, size_t n) {
    if (s >= p_ && s < p_ + n_ + 1) {  // (part of this string)
      string t(s, n);
      return *this = lib::move(t);
    }
    if (n > cap()) setCap(n);
    if (n) memmove(p_, s, n);
    n_ = n;
    p_[n_] = 0;
    return *this;
  }
  template <class U>
  string& assign(const U* a, const U* b) { return assign((const char*)a, (size_t)(b - a)); }
  string& assign(size_t k, char c) {
    clear();
    resize(k, c);
    return *this;
  }
  string& erase(size_t pos, size_t len = npos) {
    if (pos > n_) pos = n_;
    len = lib::min(len, n_ - pos);
    memmove(p_ + pos, p_ + pos + len, n_ - pos - len);
    n_ -= len;
    p_[n_] = 0;
    return *this;
  }
  string& append(const char* s, size_t n) {
    if (n == 0) return *this;
    if (n_ + n > cap()) {
      if (s >= p_ && s < p_ + n_ + 1) {  // (part of this string)
        string t(s, n);
        return append(t.p_, n);
      }
      setCap(lib::max(n_ + n, cap() * 2));
    }
    memmove(p_ + n_, s, n);
    n_ += n;
    p_[n_] = 0;
    return *this;
  }
  string& append(const char* s) { return append(s, strlen(s)); }
  string& append(const string& o) { return append(o.p_, o.n_); }
  string& append(size_t k, char c) {
    resize(n_ + k, c);
    return *this;
  }
  string& operator+=(const string& o) { return append(o.p_, o.n_); }
  string& operator+=(const char* s) { return append(s, strlen(s)); }
  string& operator+=(char c) {
    push_back(c);
    return *this;
  }
  string substr(size_t pos, size_t len = npos) const {
    if (pos > n_) pos = n_;
    return string(p_ + pos, lib::min(len, n_ - pos));
  }
  size_t find(char c, size_t pos = 0) const {
    if (pos >= n_) return npos;
    const void* q = memchr(p_ + pos, (unsigned char)c, n_ - pos);
    return q ? (size_t)((const char*)q - p_) : npos;
  }
  size_t find(const char* s, size_t pos, size_t m) const {
    if (m == 0) return pos <= n_ ? pos : npos;
    for (size_t i = pos; i + m <= n_; i++)
      if (p_[i] == s[0] && memcmp(p_ + i, s, m) == 0) return i;
    return npos;
  }
  size_t find(const char* s, size_t pos = 0) const { return find(s, pos, strlen(s)); }
  size_t find(const string& o, size_t pos = 0) const { return find(o.p_, pos, o.n_); }
  size_t rfind(char c, size_t pos = npos) const {
    if (n_ == 0) return npos;
    for (size_t i = lib::min(pos, n_ - 1) + 1; i-- > 0;)
      if (p_[i] == c) return i;
    return npos;
  }
  int compare(size_t pos, size_t len, const char* s) const {
    if (pos > n_) pos = n_;
    len = lib::min(len, n_ - pos);
    size_t m = strlen(s);
    int r = memcmp(p_ + pos, s, lib::min(len, m));
    return r ? r : len < m ? -1 : len > m ? 1 : 0;
  }
  int compare(const string& o) const {
    int r = memcmp(p_, o.p_, lib::min(n_, o.n_));
    return r ? r : n_ < o.n_ ? -1 : n_ > o.n_ ? 1 : 0;
  }
  void swap(string& o) {
    string t(lib::move(o));
    o = lib::move(*this);
    *this = lib::move(t);
  }

  bool operator==(const string& o) const { return n_ == o.n_ && memcmp(p_, o.p_, n_) == 0; }
  bool operator!=(const string& o) const { return !(*this == o); }
  bool operator==(const char* s) const { return strlen(s) == n_ && memcmp(p_, s, n_) == 0; }
  bool operator!=(const char* s) const { return !(*this == s); }
  bool operator<(const string& o) const {
    int r = memcmp(p_, o.p_, lib::min(n_, o.n_));
    return r < 0 || (r == 0 && n_ < o.n_);
  }

 private:
  static constexpr size_t kSmall = 15;
  char* p_;
  size_t n_ = 0;
  union {
    size_t cap_;
    char buf_[kSmall + 1];
  };
  bool small() const { return p_ == buf_; }
  size_t cap() const { return small() ? kSmall : cap_; }
  void init(const char* s, size_t n) {
    if (n <= kSmall) {
      p_ = buf_;
    } else {
      p_ = (char*)malloc(n + 1);
      if (!p_) outOfMemory();
      cap_ = n;
    }
    if (n) memcpy(p_, s, n);
    n_ = n;
    p_[n] = 0;
  }
  void take(string& o) {
    n_ = o.n_;
    if (o.small()) {
      p_ = buf_;
      memcpy(buf_, o.buf_, o.n_ + 1);
    } else {
      p_ = o.p_;
      cap_ = o.cap_;
      o.p_ = o.buf_;
    }
    o.n_ = 0;
    o.buf_[0] = 0;
  }
  void setCap(size_t c) {  // c >= n_, c > kSmall
    char* q = (char*)malloc(c + 1);
    if (!q) outOfMemory();
    memcpy(q, p_, n_ + 1);
    if (!small()) free(p_);
    p_ = q;
    cap_ = c;
  }
};
inline string operator+(const string& a, const string& b) {
  string r;
  r.reserve(a.size() + b.size());
  r.append(a);
  r.append(b);
  return r;
}
inline string operator+(const string& a, const char* b) {
  string r(a);
  r.append(b);
  return r;
}
inline string operator+(const char* a, const string& b) {
  string r(a);
  r.append(b);
  return r;
}
inline string operator+(const string& a, char c) {
  string r(a);
  r.push_back(c);
  return r;
}
inline string operator+(string&& a, const string& b) {
  a.append(b);
  return lib::move(a);
}
inline string operator+(string&& a, const char* b) {
  a.append(b);
  return lib::move(a);
}
inline string operator+(string&& a, char c) {
  a.push_back(c);
  return lib::move(a);
}

// ---- hashing and the unordered containers ---------------------------------------------------
// Integers: folded, not scrambled: keys that come in sequence (object numbers, item indices)
// fall in neighbouring buckets, as with libstdc++'s identity hash (that locality is most of the
// cost of a large map). The high bits are folded in for the power-of-two bucket mask.
template <class K> struct hash;
inline size_t hashFold(uint64_t x) { return (size_t)(x ^ (x >> 24) ^ (x >> 48)); }
inline size_t hashMix(uint64_t x) {
  x ^= x >> 33;
  x *= 0xff51afd7ed558ccdull;
  x ^= x >> 33;
  return (size_t)x;
}
template <> struct hash<int> { size_t operator()(int v) const { return hashFold((uint64_t)(int64_t)v); } };
template <> struct hash<unsigned> { size_t operator()(unsigned v) const { return hashFold(v); } };
template <> struct hash<unsigned long> { size_t operator()(unsigned long v) const { return hashFold(v); } };
template <> struct hash<unsigned long long> { size_t operator()(unsigned long long v) const { return hashFold(v); } };
template <> struct hash<string> {
  size_t operator()(const string& s) const {
    uint64_t h = 1469598103934665603ull;
    for (size_t i = 0; i < s.size(); i++) h = (h ^ (unsigned char)s[i]) * 1099511628211ull;
    return hashMix(h);
  }
};

// Separate chaining, one allocation per element: elements keep their address for their lifetime
// (code holds references into the map across inserts). Buckets: a power of two, load <= 1.
template <class K, class V, class H = hash<K>>
class unordered_map {
 public:
  typedef pair<const K, V> value_type;

 private:
  struct Node {
    value_type kv;
    Node* next;
    size_t h;
    template <class KK, class VV>
    Node(KK&& k, VV&& v, size_t hh) : kv(lib::forward<KK>(k), lib::forward<VV>(v)), next(nullptr), h(hh) {}
  };

 public:
  class iterator {
   public:
    iterator() {}
    iterator(const unordered_map* m, size_t b, Node* n) : m_(m), b_(b), n_(n) {}
    value_type& operator*() const { return n_->kv; }
    value_type* operator->() const { return &n_->kv; }
    bool operator==(const iterator& o) const { return n_ == o.n_; }
    bool operator!=(const iterator& o) const { return n_ != o.n_; }
    iterator& operator++() {
      n_ = n_->next;
      while (!n_ && ++b_ < m_->nb_) n_ = m_->b_[b_];
      return *this;
    }

   private:
    const unordered_map* m_ = nullptr;
    size_t b_ = 0;
    Node* n_ = nullptr;
  };
  typedef iterator const_iterator;

  unordered_map() {}
  unordered_map(const unordered_map& o) {
    for (auto& kv : o) emplace(kv.first, kv.second);
  }
  unordered_map(unordered_map&& o) noexcept : b_(o.b_), nb_(o.nb_), n_(o.n_) {
    o.b_ = nullptr;
    o.nb_ = o.n_ = 0;
  }
  unordered_map& operator=(const unordered_map& o) {
    if (this != &o) {
      clear();
      for (auto& kv : o) emplace(kv.first, kv.second);
    }
    return *this;
  }
  unordered_map& operator=(unordered_map&& o) noexcept {
    if (this != &o) {
      clear();
      free(b_);
      b_ = o.b_;
      nb_ = o.nb_;
      n_ = o.n_;
      o.b_ = nullptr;
      o.nb_ = o.n_ = 0;
    }
    return *this;
  }
  ~unordered_map() {
    clear();
    free(b_);
  }

  size_t size() const { return n_; }
  bool empty() const { return n_ == 0; }
  iterator begin() const {
    for (size_t b = 0; b < nb_; b++)
      if (b_[b]) return iterator(this, b, b_[b]);
    return end();
  }
  iterator end() const { return iterator(this, nb_, nullptr); }
  iterator find(const K& k) const {
    if (!n_) return end();
    size_t h = H()(k), b = h & (nb_ - 1);
    for (Node* n = b_[b]; n; n = n->next)
      if (n->h == h && n->kv.first == k) return iterator(this, b, n);
    return end();
  }
  size_t count(const K& k) const { return find(k) != end() ? 1 : 0; }
  V& operator[](const K& k) {
    iterator it = find(k);
    if (it != end()) return it->second;
    return insertNew(k, V())->kv.second;
  }
  template <class KK, class VV>
  pair<iterator, bool> emplace(KK&& k, VV&& v) {
    iterator it = find(k);
    if (it != end()) return pair<iterator, bool>(it, false);
    Node* n = insertNew(lib::forward<KK>(k), lib::forward<VV>(v));
    size_t b = n->h & (nb_ - 1);
    return pair<iterator, bool>(iterator(this, b, n), true);
  }
  size_t erase(const K& k) {
    if (!n_) return 0;
    size_t h = H()(k), b = h & (nb_ - 1);
    for (Node** pp = &b_[b]; *pp; pp = &(*pp)->next)
      if ((*pp)->h == h && (*pp)->kv.first == k) {
        Node* n = *pp;
        *pp = n->next;
        delete n;
        n_--;
        return 1;
      }
    return 0;
  }
  void clear() {
    for (size_t b = 0; b < nb_; b++) {
      for (Node* n = b_[b]; n;) {
        Node* x = n->next;
        delete n;
        n = x;
      }
      b_[b] = nullptr;
    }
    n_ = 0;
  }
  void reserve(size_t m) {
    size_t want = 16;
    while (want < m) want *= 2;
    if (want > nb_) rehash(want);
  }

 private:
  Node** b_ = nullptr;
  size_t nb_ = 0, n_ = 0;

  template <class KK, class VV>
  Node* insertNew(KK&& k, VV&& v) {
    size_t h = H()(k);
    if (n_ + 1 > nb_) rehash(nb_ ? nb_ * 2 : 16);
    Node* n = new Node(lib::forward<KK>(k), lib::forward<VV>(v), h);
    size_t b = h & (nb_ - 1);
    n->next = b_[b];
    b_[b] = n;
    n_++;
    return n;
  }
  void rehash(size_t nb) {
    Node** nbk = (Node**)calloc(nb, sizeof(Node*));
    if (!nbk) outOfMemory();
    for (size_t b = 0; b < nb_; b++)
      for (Node* n = b_[b]; n;) {
        Node* x = n->next;
        size_t d = n->h & (nb - 1);
        n->next = nbk[d];
        nbk[d] = n;
        n = x;
      }
    free(b_);
    b_ = nbk;
    nb_ = nb;
  }
};

template <class K, class H = hash<K>>
class unordered_set {
  struct Unit {};
  unordered_map<K, Unit, H> m_;

 public:
  size_t size() const { return m_.size(); }
  bool empty() const { return m_.empty(); }
  size_t count(const K& k) const { return m_.count(k); }
  bool insert(const K& k) { return m_.emplace(k, Unit()).second; }  // (true if new)
  size_t erase(const K& k) { return m_.erase(k); }
  void clear() { m_.clear(); }
  void reserve(size_t m) { m_.reserve(m); }
};

// ---- owning pointers --------------------------------------------------------------------------
template <class T>
class unique_ptr {
 public:
  unique_ptr() {}
  explicit unique_ptr(T* p) : p_(p) {}
  unique_ptr(unique_ptr&& o) noexcept : p_(o.p_) { o.p_ = nullptr; }
  unique_ptr& operator=(unique_ptr&& o) noexcept {
    if (this != &o) reset(o.release());
    return *this;
  }
  unique_ptr(const unique_ptr&) = delete;
  unique_ptr& operator=(const unique_ptr&) = delete;
  ~unique_ptr() { delete p_; }
  T* get() const { return p_; }
  T* operator->() const { return p_; }
  T& operator*() const { return *p_; }
  explicit operator bool() const { return p_ != nullptr; }
  T* release() {
    T* p = p_;
    p_ = nullptr;
    return p;
  }
  void reset(T* p = nullptr) {
    T* o = p_;
    p_ = p;
    delete o;
  }

 private:
  T* p_ = nullptr;
};
template <class T>
class unique_ptr<T[]> {
 public:
  unique_ptr() {}
  explicit unique_ptr(T* p) : p_(p) {}
  unique_ptr(unique_ptr&& o) noexcept : p_(o.p_) { o.p_ = nullptr; }
  unique_ptr& operator=(unique_ptr&& o) noexcept {
    if (this != &o) reset(o.release());
    return *this;
  }
  unique_ptr(const unique_ptr&) = delete;
  unique_ptr& operator=(const unique_ptr&) = delete;
  ~unique_ptr() { delete[] p_; }
  T* get() const { return p_; }
  T& operator[](size_t i) const { return p_[i]; }
  explicit operator bool() const { return p_ != nullptr; }
  T* release() {
    T* p = p_;
    p_ = nullptr;
    return p;
  }
  void reset(T* p = nullptr) {
    T* o = p_;
    p_ = p;
    delete[] o;
  }

 private:
  T* p_ = nullptr;
};

// ---- function: a copyable callable, type-erased (virtual calls need no RTTI) ------------------
template <class S>
class function;
template <class R, class... A>
class function<R(A...)> {
  struct Base {
    virtual R call(A... a) = 0;
    virtual Base* clone() const = 0;
    virtual ~Base() {}
  };
  template <class F>
  struct Impl : Base {
    F f;
    explicit Impl(const F& x) : f(x) {}
    explicit Impl(F&& x) : f(lib::move(x)) {}
    R call(A... a) override { return f(lib::forward<A>(a)...); }
    Base* clone() const override { return new Impl(f); }
  };
  Base* b_ = nullptr;

 public:
  function() {}
  function(decltype(nullptr)) {}
  function(const function& o) : b_(o.b_ ? o.b_->clone() : nullptr) {}
  function(function& o) : function(static_cast<const function&>(o)) {}
  function(function&& o) noexcept : b_(o.b_) { o.b_ = nullptr; }
  template <class F>
  function(F f) : b_(new Impl<F>(lib::move(f))) {}
  ~function() { delete b_; }
  function& operator=(const function& o) {
    if (this != &o) {
      Base* n = o.b_ ? o.b_->clone() : nullptr;
      delete b_;
      b_ = n;
    }
    return *this;
  }
  function& operator=(function& o) { return *this = static_cast<const function&>(o); }
  function& operator=(function&& o) noexcept {
    if (this != &o) {
      delete b_;
      b_ = o.b_;
      o.b_ = nullptr;
    }
    return *this;
  }
  template <class F>
  function& operator=(F f) {
    function t(lib::move(f));
    return *this = lib::move(t);
  }
  explicit operator bool() const { return b_ != nullptr; }
  R operator()(A... a) const { return b_->call(lib::forward<A>(a)...); }
};

// ---- threads, mutex, atomic counter, clock ----------------------------------------------------
class thread {
 public:
  thread() {}
  template <class F>
  explicit thread(F f) {
    function<void()>* fn = new function<void()>(lib::move(f));
#ifdef _WIN32
    h_ = CreateThread(nullptr, 0, &thread::run, fn, 0, nullptr);
    if (!h_) outOfMemory();
#else
    if (pthread_create(&t_, nullptr, &thread::run, fn) != 0) outOfMemory();
#endif
    joinable_ = true;
  }
  thread(thread&& o) noexcept { take(o); }
  thread& operator=(thread&& o) noexcept {
    if (this != &o) {
      if (joinable_) join();
      take(o);
    }
    return *this;
  }
  thread(const thread&) = delete;
  thread& operator=(const thread&) = delete;
  ~thread() {
    if (joinable_) join();
  }
  void join() {
    if (!joinable_) return;
#ifdef _WIN32
    WaitForSingleObject(h_, INFINITE);
    CloseHandle(h_);
#else
    pthread_join(t_, nullptr);
#endif
    joinable_ = false;
  }
  static unsigned hardware_concurrency() {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwNumberOfProcessors > 0 ? (unsigned)si.dwNumberOfProcessors : 1;
#else
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return n > 0 ? (unsigned)n : 1;
#endif
  }

 private:
#ifdef _WIN32
  static DWORD WINAPI run(LPVOID p) {
#else
  static void* run(void* p) {
#endif
    function<void()>* fn = (function<void()>*)p;
    (*fn)();
    delete fn;
    return 0;
  }
  void take(thread& o) {
#ifdef _WIN32
    h_ = o.h_;
#else
    t_ = o.t_;
#endif
    joinable_ = o.joinable_;
    o.joinable_ = false;
  }
#ifdef _WIN32
  HANDLE h_ = nullptr;
#else
  pthread_t t_;
#endif
  bool joinable_ = false;
};

class mutex {
 public:
#ifdef _WIN32
  mutex() { InitializeSRWLock(&m_); }
  void lock() { AcquireSRWLockExclusive(&m_); }
  void unlock() { ReleaseSRWLockExclusive(&m_); }
#else
  mutex() { pthread_mutex_init(&m_, nullptr); }
  ~mutex() { pthread_mutex_destroy(&m_); }
  void lock() { pthread_mutex_lock(&m_); }
  void unlock() { pthread_mutex_unlock(&m_); }
#endif
  mutex(const mutex&) = delete;
  mutex& operator=(const mutex&) = delete;

 private:
#ifdef _WIN32
  SRWLOCK m_;
#else
  pthread_mutex_t m_;
#endif
};

template <class M>
class lock_guard {
 public:
  explicit lock_guard(M& m) : m_(m) { m_.lock(); }
  ~lock_guard() { m_.unlock(); }
  lock_guard(const lock_guard&) = delete;
  lock_guard& operator=(const lock_guard&) = delete;

 private:
  M& m_;
};

// a counter shared between threads: fetch-and-add
class atomic_size {
 public:
  explicit atomic_size(size_t v = 0) : v_(v) {}
  size_t operator++(int) {
#ifdef _MSC_VER
    return (size_t)InterlockedExchangeAdd64((volatile LONG64*)&v_, 1);
#else
    return __atomic_fetch_add(&v_, (size_t)1, __ATOMIC_SEQ_CST);
#endif
  }

 private:
  volatile size_t v_;
};

// seconds on a monotonic clock
inline double monoSec() {
#ifdef _WIN32
  LARGE_INTEGER f, c;
  QueryPerformanceFrequency(&f);
  QueryPerformanceCounter(&c);
  return (double)c.QuadPart / (double)f.QuadPart;
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec + ts.tv_nsec * 1e-9;
#endif
}

}  // namespace lib

// ---- allocation: a global new that never returns null ------------------------------------------

// (a sanitizer build keeps the sanitizer's own allocator, which checks new/delete pairing and
// reports an allocation that cannot be met by itself)
#if defined(__SANITIZE_ADDRESS__)
#define LIB_SANITIZER_ALLOC 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define LIB_SANITIZER_ALLOC 1
#endif
#endif
#ifndef LIB_SANITIZER_ALLOC
void* operator new(size_t n) {
  void* p = malloc(n ? n : 1);
  if (!p) lib::outOfMemory();
  return p;
}
void* operator new[](size_t n) {
  void* p = malloc(n ? n : 1);
  if (!p) lib::outOfMemory();
  return p;
}
void operator delete(void* p) noexcept { free(p); }
void operator delete[](void* p) noexcept { free(p); }
void operator delete(void* p, size_t) noexcept { free(p); }
void operator delete[](void* p, size_t) noexcept { free(p); }
#endif
