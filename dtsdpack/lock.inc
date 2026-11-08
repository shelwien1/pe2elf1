// lock.inc -- Lock: a lock between threads (the progress line's, progress.inc).

// A lock between threads: a critical section (Windows), else a mutex
struct Lock {
#ifdef _WIN32
  CRITICAL_SECTION cs;
  void Init() { InitializeCriticalSection(&cs); }
  void Quit() { DeleteCriticalSection(&cs); }
  void lock() { EnterCriticalSection(&cs); }
  void unlock() { LeaveCriticalSection(&cs); }
#else
  pthread_mutex_t m;
  void Init() { pthread_mutex_init(&m, nullptr); }
  void Quit() { pthread_mutex_destroy(&m); }
  void lock() { pthread_mutex_lock(&m); }
  void unlock() { pthread_mutex_unlock(&m); }
#endif
};
