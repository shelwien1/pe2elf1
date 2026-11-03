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
static inline void ogc_set_prog(const char *argv0) {
  const char *s;
  if(!argv0 || !*argv0)
    return;
  for(s = argv0; *s; s++)
    if(*s == '/' || *s == '\\')
      argv0 = s + 1;
  if(*argv0)
    ogc_prog = argv0;
}
//  Under the library API (oc_api.inc) a message is kept for the caller to
//  read back rather than printed, and a fatal error returns through the
//  API instead of ending the process: the two hooks below, set for the
//  length of an API call and clear otherwise, so that a program's own
//  FATAL -- a file it cannot open -- still exits as it always did.
static OC_TLS char *ogc_msg_dst;
static OC_TLS sz ogc_msg_cap;
static OC_TLS void (*ogc_die_hook)(int code);
//  Called before a message goes to stderr: whatever is on the line it
//  would land on -- a progress line -- ends first (oc_progress.inc).
static void (*ogc_msg_hook)(void);
static void message(const char *fmt, va_list ap) {
  if(ogc_msg_dst) {
    vsnprintf(ogc_msg_dst, ogc_msg_cap, fmt, ap);
    return;
  }
  if(ogc_msg_hook)
    ogc_msg_hook();
  fputs(ogc_prog, stderr);
  fputs(": ", stderr);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
}
//  The output of a run that fails is removed, so that a half-written .oc
//  is never left looking like a whole one.  Its handle is closed first:
//  the failure usually comes from inside the coroutine, with main() still
//  holding the file open, and Windows will not remove a file anyone has
//  open -- the remove would fail silently and the half-written file stay.
//  "-" names stdin or stdout to the file layer, never a file: an output
//  of "-" that fails is not a file to remove -- remove("-") took away
//  whatever file of that name was in the current directory.
static inline int ogc_is_stdio(const char *p) { return p[0] == '-' && !p[1]; }
struct ogc_partial_t {
  const char *path, *path2;
  filehandle *fh, *fh2;
  void set(const char *p, filehandle *f) {
    path = p;
    fh = f;
  }
  //  A second output of the same run -- oggdet writes two -- dropped with
  //  the first.
  void also(const char *p, filehandle *f) {
    path2 = p;
    fh2 = f;
  }
  void kept() {
    path = path2 = nullptr;
    fh = fh2 = nullptr;
  }
  void drop() {
    const char *p = path, *p2 = path2;
    filehandle *f = fh, *f2 = fh2;
    if(!p && !p2)
      return;
    kept();
    if(f)
      f->close();
    if(p && !ogc_is_stdio(p))
      remove(p);
    if(f2)
      f2->close();
    if(p2 && !ogc_is_stdio(p2))
      remove(p2);
  }
};
static ogc_partial_t ogc_partial;
static NORETURN void die(int code) {
  if(ogc_die_hook)
    ogc_die_hook(code);
  ogc_partial.drop();
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
//  A refusal the Vorbis parser can take back.  Every check in vb_setup.inc,
//  in io_t and in the stream walker's packet joining goes through these
//  rather than FATAL, because the encoder runs each page through the parser
//  twice: once against a sink that codes nothing, to learn whether the page
//  parses at all, and only then against the model.  During the first run a
//  refusal is not the end of the program, it is the answer -- the page is
//  coded as bytes instead -- so it unwinds to page_fits() and stops there.
//  Outside a dry run these are FATAL, as they were.
struct vb_dry_refusal {};
static OC_TLS int vb_dry;

NORETURN void vb_refuse(const char *fmt, ...) OGC_PRINTF(1, 2);
void vb_refuse_unless(int cond, const char *fmt, ...) OGC_PRINTF(2, 3);
#define VB_REFUSE vb_refuse
#define VB_UNLESS vb_refuse_unless
#define VB_IF(cond) if(cond) VB_REFUSE
NORETURN void vb_refuse(const char *fmt, ...) {
  va_list ap;
  if(vb_dry)
    throw vb_dry_refusal();
  va_start(ap, fmt);
  message(fmt, ap);
  va_end(ap);
  die(OGC_EXIT_REFUSED);
}
void vb_refuse_unless(int cond, const char *fmt, ...) {
  va_list ap;
  if(cond)
    return;
  if(vb_dry)
    throw vb_dry_refusal();
  va_start(ap, fmt);
  message(fmt, ap);
  va_end(ap);
  die(OGC_EXIT_REFUSED);
}
//  The same name twice on a command line is an output over an input.  "-"
//  is the exception: it is stdin as an input and stdout as an output, two
//  streams, so `oggcomp c - - < a.ogg > a.oc` is a pipe, not a mistake.
//  (A name that is the same file in another spelling -- ./a.ogg, a link --
//  is caught by ogc_same_file, oggcomp.cpp, which has to look at the files.)
static inline void ogc_paths_distinct(const char *const *v, int n) {
  int i, j;
  for(i = 0; i < n; i++)
    for(j = i + 1; j < n; j++)
      if(!strcmp(v[i], v[j]) && !ogc_is_stdio(v[i]))
        FATAL_CODE(OGC_EXIT_USAGE,
                   "%s is named twice: an output would be "
                   "written over an input",
                   v[i]);
}
