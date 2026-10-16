
//mb_cache.h
#ifndef WELS_MACROBLOCK_CACHE_H__
#define WELS_MACROBLOCK_CACHE_H__

#include "typedefs.h"

namespace WelsDec {

#define REF_NOT_AVAIL    -2
#define REF_NOT_IN_LIST  -1  //intra

/*
 *  MB Cache information, such one cache should be defined within a slice
 */
/*
 * Cache for Luma               Cache for Chroma(Cb, Cr)
 *
 *  TL T T T T                  TL T T
 *   L - - - -                   L - -
 *   L - - - -                   L - - TR
 *   L - - - -
 *   L - - - - TR
 *
 */

////////////////////////mapping scan index////////////////////////

extern const uint8_t g_kuiScan4[16];

typedef struct TagNeighborAvail {
int32_t iTopAvail;
int32_t iLeftAvail;
int32_t iRightTopAvail;
int32_t iLeftTopAvail;  //used for check intra_pred_mode avail or not   //1: avail; 0: unavail

int32_t iLeftType;
int32_t iTopType;
int32_t iLeftTopType;
int32_t iRightTopType;

int8_t  iTopCbp;
int8_t  iLeftCbp;
int8_t iDummy[2]; //for align
} SWelsNeighAvail, *PWelsNeighAvail;

} // namespace WelsDec

#endif//WELS_MACROBLOCK_CACHE_H__
