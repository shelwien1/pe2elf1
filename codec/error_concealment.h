
#ifndef WELS_ERROR_CONCEALMENT_H__
#define WELS_ERROR_CONCEALMENT_H__
#include "typedefs.h"
#include "rec_mb.h"
#include "decoder_context.h"

namespace WelsDec {
//Initialize error concealment
void InitErrorCon (PWelsDecoderContext pCtx);
//Do error concealment using frame copy method
void DoErrorConFrameCopy (PWelsDecoderContext pCtx);
//Do error concealment using slice copy method
void DoErrorConSliceCopy (PWelsDecoderContext pCtx);
//Do error concealment using slice MV copy method
void DoMbECMvCopy (PWelsDecoderContext pCtx, PPicture pDec, PPicture pRef, int32_t iMbXy, int32_t iMbX, int32_t iMbY,
                   sMCRefMember* pMCRefMem, int32_t iCurrPoc);
void GetAvilInfoFromCorrectMb (PWelsDecoderContext pCtx);
void DoErrorConSliceMVCopy (PWelsDecoderContext pCtx);
//Mark erroneous frame as Ref Pic into DPB
int32_t MarkECFrameAsRef (PWelsDecoderContext pCtx);
//Judge if EC is needed to current frame
bool NeedErrorCon (PWelsDecoderContext pCtx);
// ImplementErrorConceal
// Do actual error concealment
void ImplementErrorCon (PWelsDecoderContext pCtx);

} // namespace WelsDec

#endif//WELS_ERROR_CONCEALMENT_H__

