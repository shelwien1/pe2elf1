
//nalu.h:       NAL Unit definition
#ifndef WELS_NAL_UNIT_H__
#define WELS_NAL_UNIT_H__

#include "typedefs.h"
#include "wels_common_basis.h"
#include "nal_prefix.h"
#include "bit_stream.h"

namespace WelsDec {

///////////////////////////////////NAL UNIT level///////////////////////////////////

/* NAL Unit Structure */
typedef struct TagNalUnit {
  SNalUnitHeaderExt       sNalHeaderExt;

  union {
    struct SVclNal {
      SSliceHeaderExt     sSliceHeaderExt;
      SBitStringAux       sSliceBitsRead;
      uint8_t*            pNalPos;         // save the address of slice nal for GPU function
      int32_t             iNalLength;   // save the nal length for GPU function
      bool                bSliceHeaderExtFlag;
    } sVclNal;
    SPrefixNalUnit        sPrefixNal;
  } sNalData;
  unsigned long long uiTimeStamp;
} SNalUnit, *PNalUnit;

///////////////////////////////////ACCESS Unit level///////////////////////////////////

/* Access Unit structure */
typedef struct TagAccessUnits {
  PNalUnit*               pNalUnitsList;  // list of NAL Units pointer in this AU
  uint32_t                uiAvailUnitsNum;   // Number of NAL Units available in each AU list based current bitstream,
  uint32_t                uiActualUnitsNum;       // actual number of NAL units belong to current au
// While available number exceeds count size below, need realloc extra NAL Units for list space.
  uint32_t                uiCountUnitsNum;        // Count size number of malloced NAL Units in each AU list
  uint32_t                uiStartPos;
  uint32_t                uiEndPos;
  bool                    bCompletedAuFlag;       // Indicate whether it is a completed AU
} SAccessUnit, *PAccessUnit;

} // namespace WelsDec

#endif//WELS_NAL_UNIT_H__
