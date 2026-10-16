
//bit_stream.h  -       bit-stream reading and / writing auxiliary data
#ifndef WELS_BIT_STREAM_H__
#define WELS_BIT_STREAM_H__

#include "typedefs.h"
#include "wels_common_defs.h"
#include "golomb_common.h"

using namespace WelsCommon;

namespace WelsDec {

/*!
 * \brief   input bits for decoder or initialize bitstream writing in encoder
 *
 * \param   pBitString  Bit string auxiliary pointer
 * \param   kpBuf       bit-stream buffer
 * \param   kiSize      size in bits for decoder; size in bytes for encoder
 *
 * \return  size of buffer data in byte; failed in -1 return
 */
int32_t DecInitBits (PBitStringAux pBitString, const uint8_t* kpBuf, const int32_t kiSize);

int32_t InitReadBits (PBitStringAux pBitString, intX_t iEndOffset);

void RBSP2EBSP (uint8_t* pDstBuf, uint8_t* pSrcBuf, const int32_t kiSize);

} // namespace WelsDec

#endif//WELS_BIT_STREAM_H__
