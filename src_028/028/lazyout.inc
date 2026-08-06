/* lazyout.inc -- deferred output-file handle.
 *
 * Global scope on purpose: coro_fp.inc needs it and is included before the d5c
 * namespace opens, while d5c's own drivers need the same type.  Guarded so both
 * includes are safe. */
#ifndef D5C_LAZYOUT_INC
#define D5C_LAZYOUT_INC

/* Output file opened on the FIRST WRITE, not up front.
 *
 * Opening at start-up leaves a zero-length file behind whenever the run dies
 * for any reason -- OOM, a bad header, a kill -- and a zero-length archive is
 * indistinguishable from a valid empty one to whatever consumes it next.
 * Deferring means the file only exists once there is something in it. */
struct LazyOut {
  const char* path = nullptr;
  FILE* f = nullptr;
  bool bad = false;
  FILE* w() {                                   // call immediately before writing
    if(!f && !bad && path) {
      f = fopen(path, "wb");
      if(!f) { bad = true; fprintf(stderr, "error: cannot create '%s'\n", path); }
    }
    return f;
  }
  bool ok() const { return !bad; }
  bool close() {                                // true if nothing went wrong
    if(!f) return !bad;
    bool good = (fclose(f) == 0); f = nullptr; return good && !bad;
  }
};


#endif
