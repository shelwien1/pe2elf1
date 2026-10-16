
#ifndef WELS_REC_MB_H__
#define WELS_REC_MB_H__

#include "typedefs.h"
#include "wels_common_basis.h"
#include "error_code.h"

#include "decoder_context.h"

namespace WelsDec {

#define WELS_B_MB_REC_VERIFY(uiRet) do{ \
  uint32_t uiRetTmp = (uint32_t)uiRet; \
  if( uiRetTmp != ERR_NONE ) \
    return uiRetTmp; \
}while(0)

typedef struct TagMCRefMember {
  uint8_t* pDstY;
  uint8_t* pDstU;
  uint8_t* pDstV;

  uint8_t* pSrcY;
  uint8_t* pSrcU;
  uint8_t* pSrcV;

  int32_t iSrcLineLuma;
  int32_t iSrcLineChroma;

  int32_t iDstLineLuma;
  int32_t iDstLineChroma;

  int32_t iPicWidth;
  int32_t iPicHeight;
} sMCRefMember;

void BaseMC (PWelsDecoderContext pCtx, sMCRefMember* pMCRefMem, const int32_t& listIdx, const int8_t& iRefIdx,
             int32_t iXOffset, int32_t iYOffset, SMcFunc* pMCFunc,
             int32_t iBlkWidth, int32_t iBlkHeight, int16_t iMVs[2]);

void WelsFillRecNeededMbInfo (PWelsDecoderContext pCtx, bool bOutput, PDqLayer pCurDqLayer);

int32_t RecI4x4Mb (int32_t iMBXY, PWelsDecoderContext pCtx, int16_t* pScoeffLevel, PDqLayer pDqLayer);

int32_t RecI4x4Luma (int32_t iMBXY, PWelsDecoderContext pCtx, int16_t* pScoeffLevel, PDqLayer pDqLayer);

int32_t RecI4x4Chroma (int32_t iMBXY, PWelsDecoderContext pCtx, int16_t* pScoeffLevel, PDqLayer pDqLayer);

int32_t RecI8x8Mb (int32_t iMbXy, PWelsDecoderContext pCtx, int16_t* pScoeffLevel, PDqLayer pDqLayer);

int32_t RecI8x8Luma (int32_t iMbXy, PWelsDecoderContext pCtx, int16_t* pScoeffLevel, PDqLayer pDqLayer);

int32_t RecI16x16Mb (int32_t iMBXY, PWelsDecoderContext pCtx, int16_t* pScoeffLevel, PDqLayer pDqLayer);

int32_t RecChroma (int32_t iMBXY, PWelsDecoderContext pCtx, int16_t* pScoeffLevel, PDqLayer pDqLayer);

int32_t GetInterPred (uint8_t* pPredY, uint8_t* pPredCb, uint8_t* pPredCr, PWelsDecoderContext pCtx);

int32_t GetInterBPred (uint8_t* pPredYCbCr[3], uint8_t* pTempPredYCbCr[3], PWelsDecoderContext pCtx);

} // namespace WelsDec

#endif //WELS_REC_MB_H__

