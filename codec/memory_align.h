
#if !defined(WELS_COMMON_MEMORY_ALIGN_H__)
#define WELS_COMMON_MEMORY_ALIGN_H__

#include "typedefs.h"

// NOTE: please do not clean below lines even comment, turn on for potential memory leak verify and memory usage monitor etc.
//#define MEMORY_CHECK
#define MEMORY_MONITOR
#ifdef MEMORY_CHECK
#ifndef MEMORY_MONITOR
#define MEMORY_MONITOR
#endif//MEMORY_MONITOR
#endif//MEMORY_CHECK


#ifdef MEMORY_CHECK
#include <stdio.h>
#endif//MEMORY_CHECK

namespace WelsCommon {

class CMemoryAlign {
 public:
CMemoryAlign (const uint32_t kuiCacheLineSize);
virtual ~CMemoryAlign();

void* WelsMallocz (const uint32_t kuiSize, const char* kpTag);
void* WelsMalloc (const uint32_t kuiSize, const char* kpTag);
void WelsFree (void* pPointer, const char* kpTag);
const uint32_t WelsGetCacheLineSize() const;
const uint32_t WelsGetMemoryUsage() const;

 private:
// private copy & assign constructors adding to fix klocwork scan issues
CMemoryAlign (const CMemoryAlign& kcMa);
CMemoryAlign& operator= (const CMemoryAlign& kcMa);

 protected:
uint32_t        m_nCacheLineSize;

#ifdef MEMORY_MONITOR
uint32_t        m_nMemoryUsageInBytes;
#endif//MEMORY_MONITOR
};

/*!
*************************************************************************************
* \brief        malloc with zero filled utilization in Wels
*
* \param        kuiSize     size of memory block required
*
* \return       allocated memory pointer exactly, failed in case of NULL return
*
* \note N/A
*************************************************************************************
*/
void* WelsMallocz (const uint32_t kuiSize, const char* kpTag);

/*!
*************************************************************************************
* \brief        free utilization in Wels
*
* \param        pPtr    data pointer to be free.
*                       i.e, uint8_t *pPtr = actual data to be free, argv = &pPtr.
*
* \return       NONE
*
* \note N/A
*************************************************************************************
*/
void WelsFree (void* pPtr, const char* kpTag);

#define WELS_SAFE_FREE(pPtr, pTag)              if (pPtr) { WelsFree(pPtr, pTag); pPtr = NULL; }

#define  WELS_NEW_OP(object, type)   \
  (type*)(new object);

#define  WELS_DELETE_OP(p) \
  if(p) delete p;            \
  p = NULL;

}

#endif//WELS_COMMON_MEMORY_ALIGN_H__
