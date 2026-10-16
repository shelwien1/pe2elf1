
//picture.h     -       reconstruction picture/ reference picture/ residual picture are declared here
#ifndef WELS_PICTURE_H__
#define WELS_PICTURE_H__

#include "typedefs.h"
#include "wels_common_defs.h"
#include "wels_const_common.h"
#include "wels_decoder_thread.h"

using namespace WelsCommon;

namespace WelsDec {

/*
*  Reconstructed Picture definition
*  It is used to express reference picture, also consequent reconstruction picture for output
*/

struct SPicture {
  /************************************payload data*********************************/
  uint8_t*        pBuffer[4];             // pointer to the first allocated byte, basical offset of buffer, dimension:
  uint8_t*        pData[4];               // pointer to picture planes respectively
  int32_t         iLinesize[4];// linesize of picture planes respectively used currently
  int32_t         iPlanes;                        // How many planes are introduced due to color space format?
// picture information

  /*******************************from EC mv copy****************************/
  bool bIdrFlag;

  /*******************************from other standard syntax****************************/
  /*from sps*/
  int32_t         iWidthInPixel;  // picture width in pixel
  int32_t         iHeightInPixel;// picture height in pixel
  /*from slice header*/
  int32_t         iFramePoc;              // frame POC

  /*******************************sef_definition for misc use****************************/
  bool            bUsedAsRef;                                                     //for ref pic management
  bool            bIsLongRef;     // long term reference frame flag       //for ref pic management
  int8_t          iRefCount;

  bool            bIsComplete;    // indicate whether current picture is complete, not from EC
  /*******************************for future use****************************/
  uint8_t         uiTemporalId;
  uint8_t         uiSpatialId;
  uint8_t         uiQualityId;

  int32_t         iFrameNum;              // frame number                 //for ref pic management
  int32_t         iFrameWrapNum;          // frame wrap number            //for ref pic management
  int32_t         iLongTermFrameIdx;                                      //id for long term ref pic
  uint32_t        uiLongTermPicNum;       //long_term_pic_num

  int32_t     iSpsId; //against mosaic caused by cross-IDR interval reference.
  int32_t     iPpsId;
  unsigned long long uiTimeStamp;
  uint32_t    uiDecodingTimeStamp; //represent relative decoding time stamps
  int32_t     iPicBuffIdx;
  EWelsSliceType  eSliceType;
  bool        bIsUngroupedMultiSlice; //multi-slice picture with each each slice group contains one slice.
  bool bNewSeqBegin;
  int32_t iMbEcedNum;
  int32_t iMbEcedPropNum;
  int32_t iMbNum;

  bool*    pMbCorrectlyDecodedFlag;
  int8_t (*pNzc)[24];
  uint32_t*  pMbType; // mb type used for direct mode
  int16_t (*pMv[LIST_A])[MB_BLOCK4x4_NUM][MV_A]; // used for direct mode
  int8_t (*pRefIndex[LIST_A])[MB_BLOCK4x4_NUM]; //used for direct mode
  struct SPicture* pRefPic[LIST_A][17];  //ref pictures used for direct mode
  SWelsDecEvent* pReadyEvent;  //MB line ready event

};// "Picture" declaration is comflict with Mac system

typedef struct SPicture* PPicture;

} // namespace WelsDec

#endif//WELS_PICTURE_H__
