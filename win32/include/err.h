/* Replacement for the BSD <err.h> (MinGW-w64); the functions are in win32/posix.C. */
#ifndef ROSE_MINGW_ERR_H
#define ROSE_MINGW_ERR_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif
void err(int status, const char* format, ...) __attribute__((noreturn, format(gnu_printf, 2, 3)));
void errx(int status, const char* format, ...) __attribute__((noreturn, format(gnu_printf, 2, 3)));
void warn(const char* format, ...) __attribute__((format(gnu_printf, 1, 2)));
void warnx(const char* format, ...) __attribute__((format(gnu_printf, 1, 2)));
void verr(int status, const char* format, va_list args) __attribute__((noreturn));
void verrx(int status, const char* format, va_list args) __attribute__((noreturn));
void vwarn(const char* format, va_list args);
void vwarnx(const char* format, va_list args);
#ifdef __cplusplus
}
#endif

#endif
