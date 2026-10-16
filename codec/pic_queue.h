
//pic_queue.h
#ifndef WELS_PICTURE_QUEUE_H__
#define WELS_PICTURE_QUEUE_H__


#include "picture.h"

namespace WelsDec {

#define   PICTURE_RESOLUTION_ALIGNMENT      32


typedef struct TagPicBuff {
  PPicture*      ppPic;
  int32_t        iCapacity;  // capacity size of queue
  int32_t        iCurrentIdx;
} SPicBuff, *PPicBuff;

/*
 *  Interfaces
 */

PPicture PrefetchPic (PPicBuff pPicBuff);  // To get current node applicable
PPicture PrefetchPicForThread (PPicBuff pPicBuff); // To get current node applicable in the case of threaded mode
PPicture PrefetchLastPicForThread (PPicBuff pPicBuff,
                                   const int32_t& iLast); // To get last node applicable in the case of threaded mode

} // namespace WelsDec

#endif//WELS_PICTURE_QUEUE_H__
