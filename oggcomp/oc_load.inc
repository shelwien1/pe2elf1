//  The model as a table of pointers, so that a program runs its own model
//  and a loaded one through the same code.  Model 0 is the program's own:
//  the static addresses of oc_api.inc's functions.  Models 1 to 9 are
//  oggcompN.dll on Windows and oggcompN.so elsewhere, looked for beside
//  the program first and on the system's library path second, each built
//  by `./mk.sh dll N` from an IDX model of its own, with the tables and the
//  instance of its own that a library of statics has.
#include "oc_api.h"
struct oc_api {
  int model;
  void *lib;
  void *(OC_CALL *Alloc)(void);
  void(OC_CALL *Free)(void *);
  int(OC_CALL *Init)(void *, int, int, const char *);
  int(OC_CALL *Loop)(void *);
  int(OC_CALL *getoutlen)(void *);
  void(OC_CALL *addinp)(void *, void *, int);
  void(OC_CALL *addout)(void *, void *, int);
  const char *(OC_CALL *Error)(void *);
  int(OC_CALL *StreamVersion)(void);
  unsigned long long(OC_CALL *TableBytes)(void);
  const char *(OC_CALL *Describe)(void);
  void(OC_CALL *Report)(void *);
};
static void oc_api_own(oc_api *a) {
  a->model = 0;
  a->lib = nullptr;
  a->Alloc = oggcomp_Alloc;
  a->Free = oggcomp_Free;
  a->Init = oggcomp_Init;
  a->Loop = oggcomp_Loop;
  a->getoutlen = oggcomp_getoutlen;
  a->addinp = oggcomp_addinp;
  a->addout = oggcomp_addout;
  a->Error = oggcomp_Error;
  a->StreamVersion = oggcomp_StreamVersion;
  a->TableBytes = oggcomp_TableBytes;
  a->Describe = oggcomp_Describe;
  a->Report = oggcomp_Report;
}
#ifdef _WIN32
#define OC_LIB_EXT ".dll"
static void *oc_dl_open(const char *path) { return (void *)LoadLibraryA(path); }
static void *oc_dl_sym(void *h, const char *n) { return (void *)GetProcAddress((HMODULE)h, n); }
static const char *oc_dl_error(void) {
  static char e[64];
  snprintf(e, sizeof e, "error %lu", (unsigned long)GetLastError());
  return e;
}
static void oc_exe_dir(char *buf, sz cap) {
  DWORD n = GetModuleFileNameA(nullptr, buf, (DWORD)cap);
  char *s;
  if(!n || n >= cap) {
    buf[0] = 0;
    return;
  }
  for(s = buf + n; s > buf && s[-1] != '\\' && s[-1] != '/'; s--)
    ;
  *s = 0;
}
#else
#include <dlfcn.h>
#include <unistd.h>
#define OC_LIB_EXT ".so"
static void *oc_dl_open(const char *path) { return dlopen(path, RTLD_NOW | RTLD_LOCAL); }
static void *oc_dl_sym(void *h, const char *n) { return dlsym(h, n); }
static const char *oc_dl_error(void) {
  const char *e = dlerror();
  return e ? e : "?";
}
static void oc_exe_dir(char *buf, sz cap) {
  ssize_t n = readlink("/proc/self/exe", buf, cap - 1);
  char *s;
  if(n <= 0) {
    buf[0] = 0;
    return;
  }
  buf[n] = 0;
  for(s = buf + n; s > buf && s[-1] != '/'; s--)
    ;
  *s = 0;
}
#endif
//  Model `model` into `a`: the program's own for 0, else the library.
static void oc_api_load(oc_api *a, int model) {
  static char name[32], path[4096 + 64], dir[4096];
  void *h = nullptr;
  if(model == 0) {
    oc_api_own(a);
    return;
  }
  snprintf(name, sizeof name, "oggcomp%d" OC_LIB_EXT, model);
  oc_exe_dir(dir, sizeof dir);
  if(dir[0]) {
    snprintf(path, sizeof path, "%s%s", dir, name);
    h = oc_dl_open(path);
  }
  if(!h)
    h = oc_dl_open(name);
  if(!h)
    FATAL_CODE(OGC_EXIT_IO, "model %d: %s is not beside %s and not on the library path (%s)", model, name, ogc_prog, oc_dl_error());
#define OC_SYM(f)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
  do {                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
    a->f = (decltype(a->f))oc_dl_sym(h, "oggcomp_" #f);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                \
    FATAL_UNLESS(a->f != nullptr, "%s has no oggcomp_%s: not an oggcomp model", name, #f);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             \
  } while(0)
  OC_SYM(Alloc);
  OC_SYM(Free);
  OC_SYM(Init);
  OC_SYM(Loop);
  OC_SYM(getoutlen);
  OC_SYM(addinp);
  OC_SYM(addout);
  OC_SYM(Error);
  OC_SYM(StreamVersion);
  OC_SYM(TableBytes);
  OC_SYM(Describe);
  OC_SYM(Report);
#undef OC_SYM
  a->model = model;
  a->lib = h;
}
