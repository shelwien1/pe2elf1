
#ifndef _WELS_TASK_THREAD_H_
#define _WELS_TASK_THREAD_H_


#include "WelsTask.h"
#include "WelsThread.h"

namespace WelsCommon {

class CWelsTaskThread;

class IWelsTaskThreadSink {
 public:
  virtual WELS_THREAD_ERROR_CODE OnTaskStart (CWelsTaskThread* pThread, IWelsTask* pTask) = 0;
  virtual WELS_THREAD_ERROR_CODE OnTaskStop (CWelsTaskThread* pThread, IWelsTask* pTask) = 0;
};

class CWelsTaskThread : public CWelsThread {
 public:
  CWelsTaskThread (IWelsTaskThreadSink* pSink);
  virtual ~CWelsTaskThread();

  WELS_THREAD_ERROR_CODE  SetTask (IWelsTask* pTask);
  virtual void ExecuteTask();

  uintptr_t    GetID() const {
    return m_uiID;
  }

 private:
  CWelsLock   m_cLockTask;
  IWelsTaskThreadSink*   m_pSink;
  IWelsTask*    m_pTask;
  uintptr_t    m_uiID;

  DISALLOW_COPY_AND_ASSIGN (CWelsTaskThread);
};

}

#endif

