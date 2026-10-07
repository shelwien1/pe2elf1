/* Replacement for <dlfcn.h> (MinGW-w64): dlopen and friends on top of LoadLibrary, for ROSE's
   plugins (DLLs on Windows); the functions are in win32/posix.C. */
#ifndef ROSE_MINGW_DLFCN_H
#define ROSE_MINGW_DLFCN_H

#define RTLD_LAZY 1
#define RTLD_NOW 2
#define RTLD_GLOBAL 0x100
#define RTLD_LOCAL 0

#ifdef __cplusplus
extern "C" {
#endif
void* dlopen(const char* file, int mode);
void* dlsym(void* handle, const char* name);
int dlclose(void* handle);
char* dlerror(void);
#ifdef __cplusplus
}
#endif

#endif
