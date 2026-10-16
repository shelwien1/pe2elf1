
#ifndef WELS_DEBLOCKING_H__
#define WELS_DEBLOCKING_H__

#include "decoder_context.h"
#include "deblocking_common.h"
namespace WelsDec {

/*!
 * \brief   deblocking module initialize
 *
 * \param   pf
 *          cpu
 *
 * \return  NONE
 */

void  DeblockingInit (PDeblockingFunc pDeblockingFunc,  int32_t iCpu);


/*!
 * \brief   deblocking filtering target slice
 *
 * \param   dec         Wels decoder context
 *
 * \return  NONE
 */
void WelsDeblockingFilterSlice (PWelsDecoderContext pCtx, PDeblockingFilterMbFunc pDeblockMb);

/*!
* \brief   AVC slice init deblocking filtering target layer
*
* \in and out param   SDeblockingFilter
* \in and out param   iFilterIdc
*
* \return  NONE
*/
void WelsDeblockingInitFilter (PWelsDecoderContext pCtx, SDeblockingFilter& pFilter, int32_t& iFilterIdc);

/*!
* \brief   AVC MB deblocking filtering target layer
*
* \param   DqLayer which has the current location of MB to be deblocked.
*
* \return  NONE
*/
void WelsDeblockingFilterMB (PDqLayer pCurDqLayer, SDeblockingFilter& pFilter, int32_t& iFilterIdc,
                             PDeblockingFilterMbFunc pDeblockMb);

/*!
 * \brief   pixel deblocking filtering
 *
 * \param   filter                deblocking filter
 * \param   pix                   pixel value
 * \param   stride                frame stride
 * \param   bs                    boundary strength
 *
 * \return  NONE
 */

uint32_t DeblockingBsMarginalMBAvcbase (PDeblockingFilter  pFilter, PDqLayer pCurDqLayer, int32_t iEdge,
                                        int32_t iNeighMb, int32_t iMbXy);
uint32_t DeblockingBSliceBsMarginalMBAvcbase (PDqLayer pCurDqLayer, int32_t iEdge, int32_t iNeighMb, int32_t iMbXy);

int32_t DeblockingAvailableNoInterlayer (PDqLayer pCurDqLayer, int32_t iFilterIdc);

void WelsDeblockingMb (PDqLayer pCurDqLayer, PDeblockingFilter  pFilter, int32_t iBoundryFlag);

inline int8_t* GetPNzc (PDqLayer pCurDqLayer, int32_t iMbXy) {
  if (pCurDqLayer->pDec != NULL && pCurDqLayer->pDec->pNzc != NULL) {
    return pCurDqLayer->pDec->pNzc[iMbXy];
  }
  return pCurDqLayer->pNzc[iMbXy];
}

} // namespace WelsDec

#endif //WELS_DEBLOCKING_H__

