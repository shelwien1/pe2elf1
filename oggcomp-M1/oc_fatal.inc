static const char *ogc_prog = "oggcomp";
constexpr int OGC_EXIT_OK = 0;
constexpr int OGC_EXIT_REFUSED = 1;
constexpr int OGC_EXIT_USAGE = 2;
constexpr int OGC_EXIT_IO = 3;
constexpr int OGC_EXIT_INTERNAL = 4;
#define FATAL ogc_fatal
#define FATAL_CODE ogc_fatal_code
#define FATAL_UNLESS ogc_fatal_unless
NORETURN void ogc_fatal(const char *fmt, ...) OGC_PRINTF(1, 2);
NORETURN void ogc_fatal_code(int code, const char *fmt, ...) OGC_PRINTF(2, 3);
void ogc_fatal_unless(int cond, const char *fmt, ...) OGC_PRINTF(2, 3);
#define FATAL_IF_HOT(cond)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
  if(cond)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            \
  FATAL
#define Fi(n, ...)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
  for(i = 0; i < (n); i++) {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
    __VA_ARGS__;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
  }
#define Fj(n, ...)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
  for(j = 0; j < (n); j++) {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
    __VA_ARGS__;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
  }
#define Fk(n, ...)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    \
  for(k = 0; k < (n); k++) {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
    __VA_ARGS__;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      \
  }
template <class T> constexpr const T &ogc_min(const T &a, const T &b) { return a < b ? a : b; }
static void message(const char *fmt, va_list ap) OGC_PRINTF(1, 0);
static void ogc_set_prog(const char *argv0) {
  const char *s;
  if(!argv0 || !*argv0)
    return;
  for(s = argv0; *s; s++)
    if(*s == '/' || *s == '\\')
      argv0 = s + 1;
  if(*argv0)
    ogc_prog = argv0;
}
static void message(const char *fmt, va_list ap) {
  fputs(ogc_prog, stderr);
  fputs(": ", stderr);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
}
static const char *ogc_partial;
static void ogc_output(const char *path) { ogc_partial = path; }
static void ogc_output_kept(void) { ogc_partial = nullptr; }
static NORETURN void die(int code) {
  if(ogc_partial) {
    const char *p = ogc_partial;
    ogc_partial = nullptr;
    remove(p);
  }
  exit(code);
}
NORETURN void ogc_fatal(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  message(fmt, ap);
  va_end(ap);
  die(OGC_EXIT_REFUSED);
}
NORETURN void ogc_fatal_code(int code, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  message(fmt, ap);
  va_end(ap);
  die(code);
}
void ogc_fatal_unless(int cond, const char *fmt, ...) {
  va_list ap;
  if(cond)
    return;
  va_start(ap, fmt);
  message(fmt, ap);
  va_end(ap);
  die(OGC_EXIT_REFUSED);
}
static void ogc_paths_distinct(char **v, int n) {
  int i, j;
  for(i = 0; i < n; i++)
    for(j = i + 1; j < n; j++)
      if(!strcmp(v[i], v[j]))
        FATAL_CODE(OGC_EXIT_USAGE,
                   "%s is named twice: an output would be "
                   "written over an input",
                   v[i]);
}
