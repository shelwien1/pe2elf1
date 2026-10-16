
#ifndef _WELS_THREAD_POOL_H_
#define _WELS_THREAD_POOL_H_

#include <stdio.h>
#include "WelsTask.h"
#include "WelsTaskThread.h"
#include "WelsList.h"

namespace WelsCommon {


class  CWelsThreadPool : public CWelsThread, public IWelsTaskThreadSink {
 public:
  enum {
    DEFAULT_THREAD_NUM = 4,
  };

  static WELS_THREAD_ERROR_CODE SetThreadNum (int32_t iMaxThreadNum);

  static CWelsThreadPool* AddReference();
  void RemoveInstance();

  static bool IsReferenced();

  //IWelsTaskThreadSink
  virtual WELS_THREAD_ERROR_CODE OnTaskStart (CWelsTaskThread* pThread,  IWelsTask* pTask);
  virtual WELS_THREAD_ERROR_CODE OnTaskStop (CWelsTaskThread* pThread,  IWelsTask* pTask);

  //  CWelsThread
  virtual void ExecuteTask();

  WELS_THREAD_ERROR_CODE  QueueTask (IWelsTask* pTask);
  int32_t        GetThreadNum() const {
    return m_iMaxThreadNum;
  }


 protected:
  WELS_THREAD_ERROR_CODE Init();
  WELS_THREAD_ERROR_CODE Uninit();

  WELS_THREAD_ERROR_CODE CreateIdleThread();
  void           DestroyThread (CWelsTaskThread* pThread);
  WELS_THREAD_ERROR_CODE AddThreadToIdleQueue (CWelsTaskThread* pThread);
  WELS_THREAD_ERROR_CODE AddThreadToBusyList (CWelsTaskThread* pThread);
  WELS_THREAD_ERROR_CODE RemoveThreadFromBusyList (CWelsTaskThread* pThread);
  bool           AddTaskToWaitedList (IWelsTask* pTask);
  CWelsTaskThread*   GetIdleThread();
  IWelsTask*         GetWaitedTask();
  int32_t            GetIdleThreadNum();
  int32_t            GetBusyThreadNum();
  int32_t            GetWaitedTaskNum();
  void               ClearWaitedTasks();

 private:
  CWelsThreadPool();
  virtual ~CWelsThreadPool();
  
  WELS_THREAD_ERROR_CODE StopAllRunning();

  static int32_t   m_iRefCount;
  static int32_t   m_iMaxThreadNum;
  static CWelsThreadPool* m_pThreadPoolSelf;

  CWelsNonDuplicatedList<IWelsTask>* m_cWaitedTasks;
  CWelsNonDuplicatedList<CWelsTaskThread>* m_cIdleThreads;
  CWelsList<CWelsTaskThread>* m_cBusyThreads;

  CWelsLock   m_cLockPool;
  CWelsLock   m_cLockWaitedTasks;
  CWelsLock   m_cLockIdleTasks;
  CWelsLock   m_cLockBusyTasks;

  DISALLOW_COPY_AND_ASSIGN (CWelsThreadPool);
};

}


#endif



