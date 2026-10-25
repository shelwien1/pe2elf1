// Errors, without C++ exceptions (aacpack builds with -fno-exceptions, as
// h264p does; its err.inc).
//
// err_raise() says what went wrong; where that goes, the code that called
// decides, by the traps it set (ErrTrap: a jmp_buf, Lib3's setjmp): the
// innermost one takes it. So the compressor's parse of a unit sets one
// around it (the unit is then stored as its bytes), the walk one around
// each syntax element of a raw_data_block and each extension payload (the
// rest of the frame, or the payload, is then kept as bits), the check that
// a unit comes out as it was one around writing it back, a run, the
// coroutine of c or d, one around all of it (coro.inc: the run then says
// why and ends, yield 3). With no trap set, the error is said and the
// program ends.
//
// A trap is set with setjmp() in the function that holds it (err_push()
// after it, err_pop() before it is left), and the jump to it skips what
// the frames between would destroy: they hold nothing that must be
// destroyed, and what they changed the trap's holder puts back. The traps
// are those of the coroutine that set them: as it yields, they are put
// aside (err_hold(), rcio.inc), so that an error outside it never jumps
// into it, and while it runs, its caller's are (drive(), coro.inc), so
// that its own are not set on them.
//
// Running out of memory (mem.inc) is an error of the run, not of what it
// was parsing: err_fatal() jumps to the outermost trap.

namespace aacpack {

struct ErrTrap {
  jmp_buf jb;
  ErrTrap* up;    // the trap set before it
  char why[256];  // what reached it
};

thread_local ErrTrap* err_trap_ = nullptr;  // the innermost

INLINE void err_push(ErrTrap& t) {
  t.up = err_trap_;
  t.why[0] = 0;
  err_trap_ = &t;
}
INLINE void err_pop(ErrTrap& t) { err_trap_ = t.up; }

// to trap t (set before): the traps set since it are gone
[[noreturn]] NOINLINE void err_jump(ErrTrap& t) {
  err_trap_ = t.up;
  longjmp(t.jb, 1);
  __builtin_unreachable();
}
// an error, why, to trap t (none: said, and the program ends); to the
// innermost
[[noreturn]] NOINLINE void err_to(ErrTrap* t, const char* why) {
  if (!t) {
    fprintf(stderr, "aacpack: %s\n", why);
    exit(1);
  }
  snprintf(t->why, sizeof t->why, "%s", why);
  err_jump(*t);
}
[[noreturn]] NOINLINE void err_raise_s(const char* why) { err_to(err_trap_, why); }
[[noreturn]] NOINLINE void err_raise(const char* fmt, ...) {
  char why[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(why, sizeof why, fmt, ap);
  va_end(ap);
  err_raise_s(why);
}
// an error to the outermost trap (the run's), past those set since
[[noreturn]] NOINLINE void err_fatal(const char* why) {
  ErrTrap* t = err_trap_;
  while (t && t->up) t = t->up;
  err_to(t, why);
}

// the traps of a coroutine about to yield, put aside; and back as it goes on
INLINE ErrTrap* err_hold() {
  ErrTrap* t = err_trap_;
  err_trap_ = nullptr;
  return t;
}
INLINE void err_resume(ErrTrap* t) { err_trap_ = t; }

}  // namespace aacpack
