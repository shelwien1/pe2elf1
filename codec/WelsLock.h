
#ifndef _WELS_LOCK_H_
#define _WELS_LOCK_H_

#include "macros.h"
#include "typedefs.h"
#include "WelsThreadLib.h"

namespace WelsCommon {

class CWelsLock {
  DISALLOW_COPY_AND_ASSIGN (CWelsLock);
 public:
  CWelsLock() {
    WelsMutexInit (&m_cMutex);
  }

  virtual ~CWelsLock() {
    WelsMutexDestroy (&m_cMutex);
  }

  WELS_THREAD_ERROR_CODE  Lock() {
    return WelsMutexLock (&m_cMutex);
  }

  WELS_THREAD_ERROR_CODE Unlock() {
    return WelsMutexUnlock (&m_cMutex);
  }

 private:
  WELS_MUTEX   m_cMutex;
};

class CWelsAutoLock {
  DISALLOW_COPY_AND_ASSIGN (CWelsAutoLock);
 public:
  CWelsAutoLock (CWelsLock& cLock) : m_cLock (cLock) {
    m_cLock.Lock();
  }

  virtual ~CWelsAutoLock() {
    m_cLock.Unlock();
  }

 private:
  CWelsLock&    m_cLock;
};

}

#endif







