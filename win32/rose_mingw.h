/*
 * MinGW-w64 portability for the Windows build of ROSE: force-included (-include) in every
 * compilation of librose, edg2sage and the tools by the Makefile when CXX targets Windows.
 * The sources in rose/ stay unmodified; win32/include holds replacements for POSIX headers
 * that MinGW-w64 lacks.
 */
#ifndef ROSE_MINGW_H
#define ROSE_MINGW_H

/* librose is a static library on Windows (a DLL cannot export its more than 65535 symbols),
   so ROSE's DLL import/export attributes are disabled.  (Sawyer's are disabled by the copy of
   Sawyer.h that the Makefile generates.) */
#include "rosedll.h"
#undef ROSE_DLL_HELPER_DLL_IMPORT
#undef ROSE_DLL_HELPER_DLL_EXPORT
#undef ROSE_DLL_HELPER_DLL_LOCAL
#undef ROSE_DLL_API
#undef ROSE_UTIL_API
#define ROSE_DLL_HELPER_DLL_IMPORT
#define ROSE_DLL_HELPER_DLL_EXPORT
#define ROSE_DLL_HELPER_DLL_LOCAL
#define ROSE_DLL_API
#define ROSE_UTIL_API

/* GCC for Windows also predefines __WIN32__, which sage3basic.h rejects (a check meant for an
   old Sun compiler); _WIN32 remains defined. */
#undef __WIN32__

/* POSIX functions such as isatty that ROSE uses without including <unistd.h> (with glibc, the C++
   library headers include it) */
#include <unistd.h>

/* POSIX signal and setjmp functions used by ROSE's -rose:keep_going support, and getpagesize,
   implemented in win32/posix.C.  Signal masks are not supported. */
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#ifndef _POSIX
typedef _sigset_t sigset_t;
#endif
typedef jmp_buf sigjmp_buf;
#define sigsetjmp(env, savemask) setjmp(env)
#define siglongjmp(env, value) longjmp(env, value)
struct sigaction {
  void (*sa_handler)(int);
  sigset_t sa_mask;
  int sa_flags;
};
extern "C" {
int sigaction(int signum, const struct sigaction* action, struct sigaction* old);
int sigemptyset(sigset_t* set);
char* strsignal(int signum);
int getpagesize(void);
}

#endif
