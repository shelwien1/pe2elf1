
#ifndef WELS_CRT_UTIL_SAFE_CROSS_PLATFORMS_H__
#define WELS_CRT_UTIL_SAFE_CROSS_PLATFORMS_H__

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#if defined(_WIN32)
#include <windows.h>
#include <sys/types.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#include "typedefs.h"
#endif//_WIN32

#include "typedefs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define     WELS_FILE_SEEK_SET           SEEK_SET
#define     WELS_FILE_SEEK_CUR           SEEK_CUR
#define     WESL_FILE_SEEK_END           SEEK_END

typedef      FILE  WelsFileHandle;

#ifdef _WIN32
typedef      struct _timeb     SWelsTime;
#else
typedef struct TagWelsTime {
  time_t time;
  unsigned short millitm;
} SWelsTime;
#endif

int32_t   WelsSnprintf (char* buffer,  int32_t sizeOfBuffer,  const char* format, ...);
char*   WelsStrncpy (char* dest, int32_t sizeInBytes, const char* src);
char*   WelsStrcat (char* dest, uint32_t sizeInBytes, const char* src);
int32_t   WelsVsnprintf (char* buffer, int32_t sizeOfBuffer, const char* format, va_list argptr);

WelsFileHandle*        WelsFopen (const char* filename,  const char* mode);
int32_t                WelsFclose (WelsFileHandle*   fp);
int32_t                WelsFread (void* buffer, int32_t size, int32_t count, WelsFileHandle* fp);
int32_t                WelsFwrite (const void* buffer, int32_t size, int32_t count, WelsFileHandle* fp);
int32_t                WelsFseek (WelsFileHandle* fp, int32_t offset, int32_t origin);
int32_t                WelsFflush (WelsFileHandle* fp);

int32_t                WelsGetTimeOfDay (SWelsTime* tp);
int32_t                WelsStrftime (char* buffer, int32_t size, const char* format, const SWelsTime* tp);
uint16_t               WelsGetMillisecond (const SWelsTime* tp);


#ifdef __cplusplus
}
#endif

#endif//WELS_CRT_UTIL_SAFE_CROSS_PLATFORMS_H__
