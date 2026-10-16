
//dec_frame.h
#ifndef WELS_DEC_FRAME_H__
#define WELS_DEC_FRAME_H__

#include "typedefs.h"
#include "wels_const.h"
#include "wels_common_basis.h"
#include "parameter_sets.h"
#include "nal_prefix.h"
#include "slice.h"
#include "picture.h"
#include "bit_stream.h"
#include "fmo.h"

namespace WelsDec {

///////////////////////////////////DQ Layer level///////////////////////////////////
typedef struct TagDqLayer       SDqLayer;
typedef SDqLayer*               PDqLayer;
typedef struct TagLayerInfo {
  SNalUnitHeaderExt             sNalHeaderExt;
  SSlice                        sSliceInLayer;  // Here Slice identify to Frame on concept
  PSubsetSps                    pSubsetSps;     // current pSubsetSps used, memory alloc in external
  PSps                          pSps;           // current sps based avc used, memory alloc in external
  PPps                          pPps;           // current pps used
} SLayerInfo, *PLayerInfo;
/* Layer Representation */

struct TagDqLayer {
  SLayerInfo                    sLayerInfo;

  PBitStringAux                 pBitStringAux;  // pointer to SBitStringAux
  PFmo                          pFmo;           // Current fmo context pointer used
  uint32_t* pMbType;
  int32_t* pSliceIdc;                           // using int32_t for slice_idc
  int16_t (*pMv[LIST_A])[MB_BLOCK4x4_NUM][MV_A];
  int16_t (*pMvd[LIST_A])[MB_BLOCK4x4_NUM][MV_A];
  int8_t  (*pRefIndex[LIST_A])[MB_BLOCK4x4_NUM];
	int8_t	(*pDirect)[MB_BLOCK4x4_NUM];
  bool*    pNoSubMbPartSizeLessThan8x8Flag;
  bool*    pTransformSize8x8Flag;
  int8_t*  pLumaQp;
  int8_t (*pChromaQp)[2];
  int8_t*  pCbp;
  uint16_t *pCbfDc;
  int8_t (*pNzc)[24];
  int8_t (*pNzcRs)[24];
  int8_t*  pResidualPredFlag;
  int8_t*  pInterPredictionDoneFlag;
  bool*    pMbCorrectlyDecodedFlag;
  bool*    pMbRefConcealedFlag;
  int16_t (*pScaledTCoeff)[MB_COEFF_LIST_SIZE];
  int8_t (*pIntraPredMode)[8];  //0~3 top4x4 ; 4~6 left 4x4; 7 intra16x16
  int8_t (*pIntra4x4FinalMode)[MB_BLOCK4x4_NUM];
  uint8_t  *pIntraNxNAvailFlag;
  int8_t*  pChromaPredMode;
  //uint8_t (*motion_pred_flag[LIST_A])[MB_PARTITION_SIZE]; // 8x8
  uint32_t (*pSubMbType)[MB_SUB_PARTITION_SIZE];
  int32_t iLumaStride;
  int32_t iChromaStride;
  uint8_t* pPred[3];
  int32_t iMbX;
  int32_t iMbY;
  int32_t iMbXyIndex;
  int32_t iMbWidth;               // MB width of this picture, equal to sSps.iMbWidth
  int32_t iMbHeight;              // MB height of this picture, equal to sSps.iMbHeight;

  /* Common syntax elements across all slices of a DQLayer */
  int32_t                   iSliceIdcBackup;
  uint32_t                  uiSpsId;
  uint32_t                  uiPpsId;
  uint32_t                  uiDisableInterLayerDeblockingFilterIdc;
  int32_t                   iInterLayerSliceAlphaC0Offset;
  int32_t                   iInterLayerSliceBetaOffset;
  //SPosOffset              sScaledRefLayer;
  int32_t                   iSliceGroupChangeCycle;

  PRefPicListReorderSyn     pRefPicListReordering;
  PPredWeightTabSyn         pPredWeightTable;
  PRefPicMarking            pRefPicMarking; // Decoded reference picture marking syntaxs
  PRefBasePicMarking        pRefPicBaseMarking;

  PPicture                  pRef;                   // reference picture pointer
  PPicture                  pDec;                   // reconstruction picture pointer for layer

	int16_t										iColocMv[2][16][2];     //Colocated MV cache
	int8_t										iColocRefIndex[2][16];  //Colocated RefIndex cache
	int8_t										iColocIntra[16];			  //Colocated Intra cache

  bool                      bUseWeightPredictionFlag;
	bool                      bUseWeightedBiPredIdc;
	bool                      bStoreRefBasePicFlag;                           // iCurTid == 0 && iCurQid = 0 && bEncodeKeyPic = 1
  bool                      bTCoeffLevelPredFlag;
  bool                      bConstrainedIntraResamplingFlag;
  uint8_t                   uiRefLayerDqId;
  uint8_t                   uiRefLayerChromaPhaseXPlus1Flag;
  uint8_t                   uiRefLayerChromaPhaseYPlus1;
  uint8_t                   uiLayerDqId;                    // dq_id of current layer
  bool                      bUseRefBasePicFlag;     // whether reference pic or reference base pic is referred?
};

typedef struct TagGpuAvcLayer {
  SLayerInfo                sLayerInfo;
  PBitStringAux             pBitStringAux;  // pointer to SBitStringAux

	uint32_t*                  pMbType;
  int32_t*                  pSliceIdc;      // using int32_t for slice_idc
  int8_t*                   pLumaQp;
  int8_t*                   pCbp;
  int8_t                    (*pNzc)[24];
  int8_t                    (*pIntraPredMode)[8];     //0~3 top4x4 ; 4~6 left 4x4; 7 intra16x16
  int32_t                   iMbX;
  int32_t                   iMbY;
  int32_t                   iMbXyIndex;
  int32_t                   iMbWidth;               // MB width of this picture, equal to sSps.iMbWidth
  int32_t                   iMbHeight;              // MB height of this picture, equal to sSps.iMbHeight;

} SGpuAvcDqLayer, *PGpuAvcDqLayer;

///////////////////////////////////////////////////////////////////////

} // namespace WelsDec

#endif//WELS_DEC_FRAME_H__
