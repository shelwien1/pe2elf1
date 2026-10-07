// POSIX functions that ROSE uses and MinGW-w64 lacks (declared in win32/rose_mingw.h and the
// headers in win32/include), implemented with the C run-time library and the Windows API.
#include <err.h>
#include <dlfcn.h>
#include <sys/resource.h>

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <psapi.h>

// ---------------------------------------------------------------------------------------
// <err.h>
// ---------------------------------------------------------------------------------------

static void report(const char* format, va_list args, bool withErrno) {
  int code = errno;
  char program[MAX_PATH];
  DWORD n = GetModuleFileNameA(NULL, program, sizeof program);
  const char* name = program;
  for (DWORD i = 0; i < n; ++i)
    if (program[i] == '\\' || program[i] == '/') name = program + i + 1;
  fprintf(stderr, "%s: ", n > 0 ? name : "rose");
  if (format != NULL) vfprintf(stderr, format, args);
  if (withErrno) fprintf(stderr, "%s%s", format != NULL ? ": " : "", strerror(code));
  fputc('\n', stderr);
}

extern "C" void vwarn(const char* format, va_list args) { report(format, args, true); }
extern "C" void vwarnx(const char* format, va_list args) { report(format, args, false); }
extern "C" void verr(int status, const char* format, va_list args) {
  report(format, args, true);
  exit(status);
}
extern "C" void verrx(int status, const char* format, va_list args) {
  report(format, args, false);
  exit(status);
}
extern "C" void warn(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vwarn(format, args);
  va_end(args);
}
extern "C" void warnx(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vwarnx(format, args);
  va_end(args);
}
extern "C" void err(int status, const char* format, ...) {
  va_list args;
  va_start(args, format);
  verr(status, format, args);
}
extern "C" void errx(int status, const char* format, ...) {
  va_list args;
  va_start(args, format);
  verrx(status, format, args);
}

// ---------------------------------------------------------------------------------------
// <dlfcn.h>
// ---------------------------------------------------------------------------------------

static std::string dlError;

static void setDlError(const char* what, const char* name) {
  dlError = std::string(what) + (name ? std::string(" ") + name : std::string()) + ": Windows error " +
            std::to_string((unsigned long)GetLastError());
}

extern "C" void* dlopen(const char* file, int /*mode*/) {
  HMODULE module = file != NULL ? LoadLibraryA(file) : GetModuleHandleA(NULL);
  if (module == NULL) setDlError("cannot load", file);
  return (void*)module;
}

extern "C" void* dlsym(void* handle, const char* name) {
  FARPROC address = GetProcAddress((HMODULE)handle, name);
  if (address == NULL) setDlError("cannot find", name);
  return (void*)address;
}

extern "C" int dlclose(void* handle) {
  return FreeLibrary((HMODULE)handle) ? 0 : -1;
}

extern "C" char* dlerror(void) {
  static std::string last;
  if (dlError.empty()) return NULL;
  last = dlError;
  dlError.clear();
  return &last[0];
}

// ---------------------------------------------------------------------------------------
// <sys/resource.h>
// ---------------------------------------------------------------------------------------

static void toTimeval(const FILETIME& t, struct timeval* tv) {
  unsigned long long hundredNs = ((unsigned long long)t.dwHighDateTime << 32) | t.dwLowDateTime;
  tv->tv_sec = (long)(hundredNs / 10000000ULL);
  tv->tv_usec = (long)((hundredNs % 10000000ULL) / 10);
}

extern "C" int getrusage(int who, struct rusage* usage) {
  if (who != RUSAGE_SELF || usage == NULL) {
    errno = EINVAL;
    return -1;
  }
  memset(usage, 0, sizeof *usage);
  FILETIME creation, exit, kernel, user;
  if (GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user)) {
    toTimeval(user, &usage->ru_utime);
    toTimeval(kernel, &usage->ru_stime);
  }
  PROCESS_MEMORY_COUNTERS counters;
  if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof counters)) {
    usage->ru_maxrss = (long)(counters.PeakWorkingSetSize / 1024);
    usage->ru_majflt = (long)counters.PageFaultCount;
  }
  return 0;
}

// ---------------------------------------------------------------------------------------
// Signals (win32/rose_mingw.h): handlers are installed with signal(); masks and flags are
// ignored.
// ---------------------------------------------------------------------------------------

extern "C" int sigaction(int signum, const struct sigaction* action, struct sigaction* old) {
  void (*previous)(int) = SIG_DFL;
  if (action != NULL) {
    previous = signal(signum, action->sa_handler);
    if (previous == SIG_ERR) {
      errno = EINVAL;
      return -1;
    }
  } else if (old != NULL) {
    previous = signal(signum, SIG_DFL);  // query the current handler and restore it
    signal(signum, previous);
  }
  if (old != NULL) {
    memset(old, 0, sizeof *old);
    old->sa_handler = previous;
  }
  return 0;
}

extern "C" int sigemptyset(sigset_t* set) {
  *set = 0;
  return 0;
}

extern "C" char* strsignal(int signum) {
  static char buffer[32];
  switch (signum) {
    case SIGINT: return (char*)"Interrupt";
    case SIGILL: return (char*)"Illegal instruction";
    case SIGFPE: return (char*)"Floating point exception";
    case SIGSEGV: return (char*)"Segmentation fault";
    case SIGTERM: return (char*)"Terminated";
    case SIGABRT: return (char*)"Aborted";
    default:
      snprintf(buffer, sizeof buffer, "Signal %d", signum);
      return buffer;
  }
}

extern "C" int getpagesize(void) {
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return (int)info.dwPageSize;
}
