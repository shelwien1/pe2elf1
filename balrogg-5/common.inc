/*  Copyright (C) 2026 Kamila Szewczyk

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.  */

/*  <inttypes.h> rather than <stdint.h>: the same fixed-width types, plus the
    PRI* macros the diagnostics below need.  Nothing in this tree spells a
    width as `long`, because that is 64 bits on LP64 and 32 on Windows -- a
    codebook table that fitted one and not the other is what made the point.
    The exceptions are the casts where libc or libvorbis asks for a `long` by
    name; each of those says so.  */
#include <inttypes.h>

typedef uint8_t  u8;   typedef int8_t  i8;
typedef uint16_t u16;  typedef int16_t i16;
typedef uint32_t u32;  typedef int32_t i32;
typedef uint64_t u64;  typedef int64_t i64;
typedef size_t   sz;

/*  Hot-path hints.  */
#if defined(__GNUC__) && !defined(BLR_NO_ATTRS)
#define INLINE      inline __attribute__((__always_inline__))
#define HOT         __attribute__((__hot__))
#define FLATTEN     __attribute__((__flatten__))
#define NORETURN    __attribute__((__noreturn__))
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define BLR_PRINTF(f, a) __attribute__((__format__(__printf__, f, a)))
#else
#define INLINE      inline
#define HOT
#define FLATTEN
#define NORETURN
#define LIKELY(x)   (x)
#define UNLIKELY(x) (x)
#define BLR_PRINTF(f, a)
#endif

/*  Bit width with ilog(0) = 0.  */
static INLINE u32 blr_ilog(u32 v) {
#if defined(__GNUC__) && !defined(BLR_NO_ATTRS)
  return v ? 32 - (u32) __builtin_clz(v) : 0;
#else
  u32 n = 0;
  while (v) { n++;  v >>= 1; }
  return n;
#endif
}

constexpr int BLR_EXIT_OK       = 0;  /*  Success.  */
constexpr int BLR_EXIT_REFUSED  = 1;  /*  Malformed, unsupported, foreign.  */
constexpr int BLR_EXIT_USAGE    = 2;  /*  Usage error.  */
constexpr int BLR_EXIT_IO       = 3;  /*  A file could not be opened, read
                                          or written.  */
constexpr int BLR_EXIT_INTERNAL = 4;  /*  Internal error.  */

#define FATAL         blr_fatal
#define FATAL_CODE    blr_fatal_code
#define FATAL_UNLESS  blr_fatal_unless

/*  FATAL uses BLR_EXIT_REFUSED.  */
NORETURN void blr_fatal(const char * fmt, ...) BLR_PRINTF(1, 2);
NORETURN void blr_fatal_code(int code, const char * fmt, ...) BLR_PRINTF(2, 3);
void blr_fatal_unless(int cond, const char * fmt, ...) BLR_PRINTF(2, 3);

/*  Avoid FATAL_UNLESS call overhead on per-bit hot paths.  */
#define FATAL_IF_HOT(cond)  if (cond) FATAL

/*  Loop macros require the caller to declare each induction variable.  */
#define Fi(n, ...) for (i = 0; i < (n); i++) {  __VA_ARGS__; }
#define Fj(n, ...) for (j = 0; j < (n); j++) {  __VA_ARGS__; }
#define Fk(n, ...) for (k = 0; k < (n); k++) {  __VA_ARGS__; }

template <class T> constexpr const T & blr_min(const T & a, const T & b) {
  return a < b ? a : b;
}
template <class T> constexpr const T & blr_max(const T & a, const T & b) {
  return a > b ? a : b;
}

/*  Every buffer in this program is a fixed-size table sized from the format's
    own limits, so there is no allocator and nothing to run out of but the
    tables themselves.  Each one states the bound it is derived from where it
    is defined.  */

static void message(const char * fmt, va_list ap) BLR_PRINTF(1, 0);

/*  Set from argv[0] at entry.  Both programs share this file, so a fixed
    "balrogg: " labelled tsv2wav's diagnostics with the wrong program.  */
static const char * blr_prog = "balrogg";

static void blr_set_prog(const char * argv0) {
  const char * s;
  if (!argv0 || !*argv0) return;
  for (s = argv0; *s; s++) if (*s == '/' || *s == '\\') argv0 = s + 1;
  if (*argv0) blr_prog = argv0;
}

static void message(const char * fmt, va_list ap) {
  fputs(blr_prog, stderr);  fputs(": ", stderr);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
}

static NORETURN void die(int code) { exit(code); }

NORETURN void blr_fatal(const char * fmt, ...) {
  va_list ap;
  va_start(ap, fmt);  message(fmt, ap);  va_end(ap);
  die(BLR_EXIT_REFUSED);
}

NORETURN void blr_fatal_code(int code, const char * fmt, ...) {
  va_list ap;
  va_start(ap, fmt);  message(fmt, ap);  va_end(ap);
  die(code);
}

void blr_fatal_unless(int cond, const char * fmt, ...) {
  va_list ap;
  if (cond) return;
  va_start(ap, fmt);  message(fmt, ap);  va_end(ap);
  die(BLR_EXIT_REFUSED);
}

/*  Both tools create their outputs with "wb", which truncates before the input
    has been read, so naming one file twice destroyed it and still exited 0.
    Every path either tool is given names a distinct file in every legitimate
    invocation, so requiring exactly that is the whole check.  It compares the
    paths as written, which catches the repeated argument that actually
    happens; it does not see one file reached by two different spellings.  */
static void blr_paths_distinct(char ** v, int n) {
  int i, j;
  for (i = 0; i < n; i++)
    for (j = i + 1; j < n; j++)
      if (!strcmp(v[i], v[j]))
        FATAL_CODE(BLR_EXIT_USAGE, "%s is named twice: an output would be "
                   "written over an input", v[i]);
}
