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
#include <windows.h>
#else
#include <pthread.h>
#include <time.h>
#include <unistd.h>
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

// ---- allocation: placement new for the containers, and a global new that never returns null ---
namespace lib {
struct PlaceTag {};
}
inline void* operator new(size_t, void* p, lib::PlaceTag) noexcept { return p; }
inline void operator delete(void*, void*, lib::PlaceTag) noexcept {}

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
