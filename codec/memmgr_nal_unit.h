
#ifndef WELS_MEMORY_MANAGER_NAL_UNIT_H__
#define WELS_MEMORY_MANAGER_NAL_UNIT_H__

#include "typedefs.h"
#include "wels_common_basis.h"
#include "nalu.h"
#include "memory_align.h"

namespace WelsDec {

int32_t MemInitNalList (PAccessUnit* ppAu, const uint32_t kuiSize, CMemoryAlign* pMa);

int32_t MemFreeNalList (PAccessUnit* ppAu, CMemoryAlign* pMa);

/*
 *  MemGetNextNal
 *  Get next NAL Unit for using.
 *  Need expand NAL Unit list if exceeding count number of available NAL Units withing an Access Unit
 */
PNalUnit MemGetNextNal (PAccessUnit* ppAu, CMemoryAlign* pMa);

} // namespace WelsDec

#endif//WELS_MEMORY_MANAGER_NAL_UNIT_H__


