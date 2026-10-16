
#ifndef _WELS_THREAD_H_
#define _WELS_THREAD_H_


#include "macros.h"
#include "WelsLock.h"
#include "WelsThreadLib.h"

namespace WelsCommon {

class CWelsThread {
 public:
  CWelsThread();
  virtual ~CWelsThread();

  virtual void Thread();
  virtual void ExecuteTask() = 0;
  virtual WELS_THREAD_ERROR_CODE Start();
  virtual void Kill();
  WELS_MUTEX          m_hMutex;
 protected:
  static WELS_THREAD_ROUTINE_TYPE  TheThread (void* pParam);

  void SetRunning (bool bRunning) {
    CWelsAutoLock  cLock (m_cLockStatus);

    m_bRunning = bRunning;
  }
  void SetEndFlag (bool bEndFlag) {
    CWelsAutoLock  cLock (m_cLockStatus);

    m_bEndFlag = bEndFlag;
  }

  bool GetRunning() const {
    return m_bRunning;
  }

  bool GetEndFlag() const {
    return m_bEndFlag;
  }

  void SignalThread() {
    WelsEventSignal (&m_hEvent, &m_hMutex, &m_iConVar);
  }

 private:
  WELS_THREAD_HANDLE  m_hThread;
  WELS_EVENT          m_hEvent;
  CWelsLock           m_cLockStatus;
  bool                m_bRunning;
  bool                m_bEndFlag;
  int                 m_iConVar;

  DISALLOW_COPY_AND_ASSIGN (CWelsThread);
};


}



#endif


