
#ifndef WELS_DECODER_SYSTEM_ARCHITECTURE_H__
#define WELS_DECODER_SYSTEM_ARCHITECTURE_H__

#include "typedefs.h"
#include "decoder_context.h"

namespace WelsDec {

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/*!
 * \brief   configure decoder parameters
 */
int32_t DecoderConfigParam (PWelsDecoderContext pCtx, const SDecodingParam* kpParam);

/*!
 * \brief   fill in default values of decoder context
 */
void WelsDecoderDefaults (PWelsDecoderContext pCtx, SLogContext* pLogCtx);

/*
* fill last decoded picture info
*/
void WelsDecoderLastDecPicInfoDefaults (SWelsLastDecPicInfo& sLastDecPicInfo);

/*!
* \brief   fill data fields in SPS and PPS default for decoder context
*/
void WelsDecoderSpsPpsDefaults (SWelsDecoderSpsPpsCTX& sSpsPpsCtx);

/*!
* \brief   copy SpsPps from one Ctx to another ctx for threaded code
*/
void CopySpsPps (PWelsDecoderContext pFromCtx, PWelsDecoderContext pToCtx);

/*!
 *************************************************************************************
 * \brief   Initialize Wels decoder parameters and memory
 *
 * \param   pCtx            input context to be initialized at first stage
 * \param   pTraceHandle    handle for trace
 * \param   pLo             log info pointer
 *
 * \return  0 - successed
 * \return  1 - failed
 *
 * \note    N/A
 *************************************************************************************
 */
int32_t WelsInitDecoder (PWelsDecoderContext pCtx, SLogContext* pLogCtx);

/*!
 *************************************************************************************
 * \brief   Uninitialize Wels decoder parameters and memory
 *
 * \param   pCtx    input context to be uninitialized at release stage
 *
 * \return  NONE
 *
 * \note    N/A
 *************************************************************************************
 */
void WelsEndDecoder (PWelsDecoderContext pCtx);

/*!
 *************************************************************************************
 * \brief   First entrance to decoding core interface.
 *
 * \param   pCtx            decoder context
 * \param   pBufBs          bit streaming buffer
 * \param   kBsLen          size in bytes length of bit streaming buffer input
 * \param   ppDst           picture payload data to be output
 * \param   pDstBufInfo     buf information of ouput data
 *
 * \return  0 - successed
 * \return  1 - failed
 *
 * \note    N/A
 *************************************************************************************
 */

int32_t WelsDecodeBs (PWelsDecoderContext pCtx, const uint8_t* kpBsBuf, const int32_t kiBsLen,
                      uint8_t** ppDst, SBufferInfo* pDstBufInfo, SParserBsInfo* pDstBsInfo);

/*
 *  request memory blocks for decoder avc part
 */
int32_t WelsRequestMem (PWelsDecoderContext pCtx, const int32_t kiMbWidth, const int32_t kiMbHeight,
                        bool& bReallocFlag);


/*
 *  free memory dynamically allocated during decoder
 */
void WelsFreeDynamicMemory (PWelsDecoderContext pCtx);

/*!
 * \brief   make sure synchonozization picture resolution (get from slice header) among different parts (i.e, memory related and so on)
 *          over decoder internal
 * ( MB coordinate and parts of data within decoder context structure )
 * \param   pCtx        Wels decoder context
 * \param   iMbWidth    MB width
 * \pram    iMbHeight   MB height
 * \return  0 - successful; none 0 - something wrong
 */
int32_t SyncPictureResolutionExt (PWelsDecoderContext pCtx, const int32_t kiMbWidth, const int32_t kiMbHeight);

/*!
 * \brief   init decoder predictive function pointers including ASM functions during MB reconstruction
 * \param   pCtx        Wels decoder context
 * \param   uiCpuFlag   cpu assembly indication
 */
void InitPredFunc (PWelsDecoderContext pCtx, uint32_t uiCpuFlag);

/*!
 * \brief   init decoder internal function pointers including ASM functions
 * \param   pCtx        Wels decoder context
 * \param   uiCpuFlag   cpu assembly indication
 */
void InitDecFuncs (PWelsDecoderContext pCtx, uint32_t uiCpuFlag);

void GetVclNalTemporalId (PWelsDecoderContext pCtx); //get the info that whether or not have VCL NAL in current AU,
//and if YES, get the temporal ID

//reset decoder number related statistics info
void ResetDecStatNums (SDecoderStatistics* pDecStat);
//update information when freezing occurs, including IDR/non-IDR number
void UpdateDecStatFreezingInfo (const bool kbIdrFlag, SDecoderStatistics* pDecStat);
//update information when no freezing occurs, including QP, correct IDR number, ECed IDR number
void UpdateDecStatNoFreezingInfo (PWelsDecoderContext pCtx);
//update decoder statistics information
void UpdateDecStat (PWelsDecoderContext pCtx, const bool kbOutput);
//Destroy picutre buffer
void DestroyPicBuff (PWelsDecoderContext pCtx, PPicBuff* ppPicBuf, CMemoryAlign* pMa);
//reset picture reodering buffer list
void ResetReorderingPictureBuffers (PPictReoderingStatus pPictReoderingStatus, PPictInfo pPictInfo,
                                    const bool& bFullReset);

#ifdef __cplusplus
}
#endif//__cplusplus

} // namespace WelsDec

#endif//WELS_DECODER_SYSTEM_ARCHITECTURE_H__
