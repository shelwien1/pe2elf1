
#ifndef _WELS_TASK_H_
#define _WELS_TASK_H_

#include "codec_def.h"

namespace WelsCommon {

class IWelsTaskSink {
 public:
  virtual int OnTaskExecuted() = 0;
  virtual int OnTaskCancelled() = 0;
};

class IWelsTask {
 public:
  IWelsTask (IWelsTaskSink* pSink) {
    m_pSink = pSink;
  };
  virtual ~IWelsTask() { }

  virtual int Execute() = 0;

  IWelsTaskSink* GetSink() {
    return m_pSink;
  };

 protected:
  IWelsTaskSink*   m_pSink;
};

}

#endif


