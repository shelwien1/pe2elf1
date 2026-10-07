/* Replacement for <sys/resource.h> (MinGW-w64): getrusage for the calling process (times and
   peak memory use); the function is in win32/posix.C. */
#ifndef ROSE_MINGW_SYS_RESOURCE_H
#define ROSE_MINGW_SYS_RESOURCE_H

#include <sys/time.h>

#define RUSAGE_SELF 0
#define RUSAGE_CHILDREN (-1)

struct rusage {
  struct timeval ru_utime;  /* user time */
  struct timeval ru_stime;  /* system time */
  long ru_maxrss;           /* peak working set (kilobytes) */
  long ru_ixrss, ru_idrss, ru_isrss, ru_minflt, ru_majflt, ru_nswap, ru_inblock, ru_oublock,
       ru_msgsnd, ru_msgrcv, ru_nsignals, ru_nvcsw, ru_nivcsw;
};

#ifdef __cplusplus
extern "C" {
#endif
int getrusage(int who, struct rusage* usage);
#ifdef __cplusplus
}
#endif

#endif
