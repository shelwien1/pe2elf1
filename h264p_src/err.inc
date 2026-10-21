// Errors, without C++ exceptions (h264p builds with -fno-exceptions).
//
// err_raise() says what went wrong; where that goes, the code that called
// decides, by the traps it set (ErrTrap: a jmp_buf, Lib3's setjmp): the
// innermost one takes it. So the dry run sets one around a NAL unit's
// parse and write-back (cmstream.inc: the NAL unit is then stored
// verbatim), the checked model pass one around a slice (compressed again),
// update_params() one around a verbatim parameter set (its errors are of
// no matter), and a run, the coroutine of c or d, one around all of it
// (coro.inc: the run then says why and ends, yield 3). With no trap set,
// the error is said and the program ends.
//
// A trap is set with setjmp() in the function that holds it (err_push()
// after it, err_pop() before it is left), and the jump to it skips what
// the frames between would destroy: they hold nothing that must be
// destroyed. The traps are those of the coroutine that set them: as it
// yields, they are put aside (err_hold(), rcio.inc), so that an error
// outside it never jumps into it.
//
// Running out of memory (mem.inc) is an error of the run, not of what it
// was parsing: err_fatal() jumps to the outermost trap.

namespace h264p {

// a run's status when it is to be done again (code_one, run_part): the
// model pass's check found a slice that does not come out as it was
const int kRestart = -1;

struct ErrTrap {
  jmp_buf jb;
  ErrTrap* up;    // the trap set before it
  int code;       // what reached it: 1 an error (why), or err_jump()'s code
  char why[256];
};

thread_local ErrTrap* err_trap_ = nullptr;  // the innermost

INLINE void err_push(ErrTrap& t) {
  t.up = err_trap_;
  t.code = 0;
  t.why[0] = 0;
  err_trap_ = &t;
}
INLINE void err_pop(ErrTrap& t) { err_trap_ = t.up; }

// to trap t (set before), with code: the traps set since it are gone
[[noreturn]] NOINLINE void err_jump(ErrTrap& t, int code) {
  t.code = code;
  err_trap_ = t.up;
  longjmp(t.jb, 1);
  __builtin_unreachable();
}
// an error, why, to the innermost trap, else said: the program ends
[[noreturn]] NOINLINE void err_raise_s(const char* why) {
  ErrTrap* t = err_trap_;
  if (!t) {
    fprintf(stderr, "h264p: %s\n", why);
    exit(1);
  }
  snprintf(t->why, sizeof t->why, "%s", why);
  err_jump(*t, 1);
}
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
  if (!t) {
    fprintf(stderr, "h264p: %s\n", why);
    exit(1);
  }
  snprintf(t->why, sizeof t->why, "%s", why);
  err_jump(*t, 1);
}

// the traps of a coroutine about to yield, put aside; and back as it goes on
INLINE ErrTrap* err_hold() {
  ErrTrap* t = err_trap_;
  err_trap_ = nullptr;
  return t;
}
INLINE void err_resume(ErrTrap* t) { err_trap_ = t; }

}  // namespace h264p
