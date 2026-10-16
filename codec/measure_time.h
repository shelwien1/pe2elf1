
#ifndef WELS_TIME_COST_MEASURE_UTIL_H__
#define WELS_TIME_COST_MEASURE_UTIL_H__

#include <stdlib.h>

#include "typedefs.h"
#ifndef _WIN32
#include <sys/time.h>
#else
#include <windows.h>
#endif
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/*!
 * \brief   time cost measure utilization
 * \param   void
 * \return  time elapsed since run (unit: microsecond)
 */

static inline int64_t WelsTime (void) {
#ifndef _WIN32
  struct timeval tv_date;

  gettimeofday (&tv_date, NULL);
  return ((int64_t) tv_date.tv_sec * 1000000 + (int64_t) tv_date.tv_usec);
#else
  static int64_t iMtimeFreq = 0;
  int64_t iMtimeCur = 0;
  int64_t iResult = 0;
  if (!iMtimeFreq) {
    QueryPerformanceFrequency ((LARGE_INTEGER*)&iMtimeFreq);
    if (!iMtimeFreq)
      iMtimeFreq = 1;
  }
  QueryPerformanceCounter ((LARGE_INTEGER*)&iMtimeCur);
  iResult = (int64_t) ((double)iMtimeCur * 1e6 / (double)iMtimeFreq + 0.5);
  return iResult;
#endif//_WIN32
}

#ifdef __cplusplus
}
#endif

#endif//WELS_TIME_COST_MEASURE_UTIL_H__
