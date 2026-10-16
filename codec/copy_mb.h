
#ifndef WELS_COPY_MB_H_
#define WELS_COPY_MB_H_

#include "typedefs.h"

/****************************************************************************
 * Copy functions
 ****************************************************************************/
void WelsCopy4x4_c (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy8x4_c (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy4x8_c (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy8x8_c (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy8x16_c (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS); //
void WelsCopy16x8_c (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS); //
void WelsCopy16x16_c (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);

#if defined(__cplusplus)
extern "C" {
#endif//__cplusplus

#if defined (X86_ASM)

void WelsCopy8x8_mmx (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy8x16_mmx (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy16x8NotAligned_sse2 (uint8_t* Dst, int32_t  iStrideD, uint8_t* Src, int32_t  iStrideS);
void WelsCopy16x16_sse2 (uint8_t* Dst, int32_t  iStrideD, uint8_t* Src, int32_t  iStrideS);
void WelsCopy16x16NotAligned_sse2 (uint8_t* Dst, int32_t  iStrideD, uint8_t* Src, int32_t  iStrideS);
#endif//X86_ASM

#if defined (HAVE_NEON)
void WelsCopy8x8_neon (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy16x16_neon (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy16x16NotAligned_neon (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy16x8NotAligned_neon (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy8x16_neon (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
#endif

#if defined (HAVE_NEON_AARCH64)
void WelsCopy8x8_AArch64_neon (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy16x16_AArch64_neon (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy16x16NotAligned_AArch64_neon (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy16x8NotAligned_AArch64_neon (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy8x16_AArch64_neon (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
#endif

#if defined (HAVE_MMI)
void WelsCopy8x8_mmi (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy8x16_mmi (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy16x8NotAligned_mmi (uint8_t* Dst, int32_t  iStrideD, uint8_t* Src, int32_t  iStrideS);
void WelsCopy16x16_mmi (uint8_t* Dst, int32_t  iStrideD, uint8_t* Src, int32_t  iStrideS);
void WelsCopy16x16NotAligned_mmi (uint8_t* Dst, int32_t  iStrideD, uint8_t* Src, int32_t  iStrideS);
#endif//HAVE_MMI

#if defined (HAVE_MSA)
void WelsCopy8x8_msa (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy8x16_msa (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy16x8_msa (uint8_t* Dst, int32_t  iStrideD, uint8_t* Src, int32_t  iStrideS);
void WelsCopy16x16_msa (uint8_t* Dst, int32_t  iStrideD, uint8_t* Src, int32_t  iStrideS);
#endif//HAVE_MSA

#if defined (HAVE_LSX)
void WelsCopy8x8_lsx (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy16x16_lsx (uint8_t* Dst, int32_t  iStrideD, uint8_t* Src, int32_t  iStrideS);
void WelsCopy16x16NotAligned_lsx (uint8_t* Dst, int32_t  iStrideD, uint8_t* Src, int32_t  iStrideS);
#endif

#if defined(__cplusplus)
}
#endif//__cplusplus

#endif //SAMPLE_H_
