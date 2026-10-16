
#ifndef WELS_CABAC_DECODER_H__
#define WELS_CABAC_DECODER_H__

#include "decoder_context.h"
#include "error_code.h"
#include "wels_common_defs.h"
namespace WelsDec {
static const uint8_t g_kRenormTable256[256] = {
  6, 6, 6, 6, 6, 6, 6, 6,
  5, 5, 5, 5, 5, 5, 5, 5,
  4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4,
  3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1
};


//1. CABAC context initialization
void WelsCabacGlobalInit (PWelsDecoderContext pCabacCtx);
void WelsCabacContextInit (PWelsDecoderContext  pCtx, uint8_t eSliceType, int32_t iCabacInitIdc, int32_t iQp);

//2. decoding Engine initialization
int32_t InitCabacDecEngineFromBS (PWelsCabacDecEngine pDecEngine, SBitStringAux* pBsAux);
void RestoreCabacDecEngineToBS (PWelsCabacDecEngine pDecEngine, SBitStringAux* pBsAux);

//3. actual decoding
int32_t Read32BitsCabac (PWelsCabacDecEngine pDecEngine, uint32_t& uiValue, int32_t& iNumBitsRead);
int32_t DecodeBinCabac (PWelsCabacDecEngine pDecEngine, PWelsCabacCtx pBinCtx, uint32_t& uiBit);
int32_t DecodeBypassCabac (PWelsCabacDecEngine pDecEngine, uint32_t& uiBinVal);
int32_t  DecodeTerminateCabac (PWelsCabacDecEngine pDecEngine, uint32_t& uiBinVal);

//4. unary parsing
int32_t DecodeUnaryBinCabac (PWelsCabacDecEngine pDecEngine, PWelsCabacCtx pBinCtx, int32_t iCtxOffset, uint32_t& uiSymVal);

//5. EXGk parsing
int32_t DecodeExpBypassCabac (PWelsCabacDecEngine pDecEngine, int32_t iCount, uint32_t& uiSymVal);
uint32_t DecodeUEGLevelCabac (PWelsCabacDecEngine pDecEngine, PWelsCabacCtx pBinCtx, uint32_t& uiBinVal);
int32_t DecodeUEGMvCabac (PWelsCabacDecEngine pDecEngine, PWelsCabacCtx pBinCtx, uint32_t iMaxC,  uint32_t& uiCode);

#define WELS_CABAC_HALF    0x01FE
#define WELS_CABAC_QUARTER 0x0100
#define WELS_CABAC_FALSE_RETURN(iErrorInfo) \
if(iErrorInfo) { \
  return iErrorInfo; \
}
}
#endif
