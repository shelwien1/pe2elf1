#ifndef WELS_MANAGE_DEC_REF_H__
#define WELS_MANAGE_DEC_REF_H__


#include "typedefs.h"
#include "decoder_context.h"

namespace WelsDec {

void  WelsResetRefPic (PWelsDecoderContext pCtx);
void  WelsResetRefPicWithoutUnRef (PWelsDecoderContext pCtx);
int32_t WelsInitRefList (PWelsDecoderContext pCtx, int32_t iPoc);
int32_t WelsInitBSliceRefList (PWelsDecoderContext pCtx, int32_t iPoc);
int32_t WelsReorderRefList (PWelsDecoderContext pCtx);
int32_t WelsReorderRefList2 (PWelsDecoderContext pCtx);
int32_t WelsMarkAsRef (PWelsDecoderContext pCtx, PPicture pLastDec = NULL);

} // namespace WelsDec

#endif//WELS_MANAGE_DEC_REF_H__


