
//nal_prefix.h  -       definitions for NAL Unit Header(/Ext) and PrefixNALUnit
#ifndef WELS_NAL_UNIT_PREFIX_H__
#define WELS_NAL_UNIT_PREFIX_H__

#include "typedefs.h"
#include "wels_common_basis.h"
#include "slice.h"

namespace WelsDec {

///////////////////////////////////NAL Unit prefix/headers///////////////////////////////////

/* Prefix NAL Unix syntax, refer to Page 392 in JVT X201wcm */
typedef struct TagPrefixNalUnit {
  SRefBasePicMarking sRefPicBaseMarking;
  bool               bStoreRefBasePicFlag;
  bool               bPrefixNalUnitAdditionalExtFlag;
  bool               bPrefixNalUnitExtFlag;
  bool               bPrefixNalCorrectFlag;
} SPrefixNalUnit, *PPrefixNalUnit;

} // namespace WelsDec

#endif//WELS_NAL_UNIT_PREFIX_H__
