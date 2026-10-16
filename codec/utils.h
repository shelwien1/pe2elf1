
#ifndef WELS_UTILS_H__
#define WELS_UTILS_H__

#include <stdarg.h>
#include "typedefs.h"

#define MAX_LOG_SIZE    1024
#define MAX_MBS_PER_FRAME 36864 //in accordance with max level support in Rec
/*
 *  Function pointer declaration for various tool sets
 */
// wels log output
typedef void (*PWelsLogCallbackFunc) (void* pCtx, const int32_t iLevel, const char* kpFmt, va_list argv);

typedef struct TagLogContext {
  PWelsLogCallbackFunc pfLog;
  void* pLogCtx;
  void* pCodecInstance;
} SLogContext;


#ifdef __GNUC__
extern void WelsLog (SLogContext* pCtx, int32_t iLevel, const char* kpFmt, ...) __attribute__ ((__format__ (__printf__,
    3,
    4)));
#else
extern void WelsLog (SLogContext* pCtx, int32_t iLevel, const char* kpFmt, ...);
#endif

/*
 *  PSNR calculation routines
 */
/*!
 *************************************************************************************
 * \brief   PSNR calculation utilization in Wels
 *
 * \param   kpTarPic    target picture to be calculated in Picture pData format
 * \param   kiTarStride stride of target picture pData pBuffer
 * \param   kpRefPic    base referencing picture samples
 * \param   kiRefStride stride of reference picture pData pBuffer
 * \param   kiWidth     picture iWidth in pixel
 * \param   kiHeight    picture iHeight in pixel
 *
 * \return  actual PSNR result;
 *
 * \note    N/A
 *************************************************************************************
 */
float WelsCalcPsnr (const void* kpTarPic,
                    const int32_t kiTarStride,
                    const void* kpRefPic,
                    const int32_t kiRefStride,
                    const int32_t kiWidth,
                    const int32_t kiHeight);


#endif//WELS_UTILS_H__
