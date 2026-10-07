/* Replacement for <sys/wait.h> (MinGW-w64), which ROSE includes: the Windows build runs
   processes with the Windows API (win32/processSupport.C), whose statuses are exit codes. */
#ifndef ROSE_MINGW_SYS_WAIT_H
#define ROSE_MINGW_SYS_WAIT_H

#define WIFEXITED(status) 1
#define WEXITSTATUS(status) (status)
#define WIFSIGNALED(status) 0
#define WTERMSIG(status) 0

#endif
