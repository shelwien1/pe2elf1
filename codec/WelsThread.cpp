
#include "WelsThread.h"

namespace WelsCommon {

CWelsThread::CWelsThread() :
  m_hThread (0),
  m_bRunning (false),
  m_bEndFlag (false) {

  WelsEventOpen (&m_hEvent);
  WelsMutexInit(&m_hMutex);
  m_iConVar = 1;
}

CWelsThread::~CWelsThread() {
  Kill();
  WelsEventClose (&m_hEvent);
  WelsMutexDestroy(&m_hMutex);
}

void CWelsThread::Thread() {
  while (true) {
    WelsEventWait (&m_hEvent,&m_hMutex,m_iConVar);

    if (GetEndFlag()) {
      break;
    }

    m_iConVar = 1;
    ExecuteTask();//in ExecuteTask there will be OnTaskStop which opens the potential new Signaling of next run, so the setting of m_iConVar = 1 should be before ExecuteTask()
  }

  SetRunning (false);
}

WELS_THREAD_ERROR_CODE CWelsThread::Start() {
#ifndef __APPLE__
  if (NULL == m_hEvent) {
    return WELS_THREAD_ERROR_GENERAL;
  }
#endif
  if (GetRunning()) {
    return WELS_THREAD_ERROR_OK;
  }

  SetEndFlag (false);

  WELS_THREAD_ERROR_CODE rc = WelsThreadCreate (&m_hThread,
                              (LPWELS_THREAD_ROUTINE)TheThread, this, 0);

  if (WELS_THREAD_ERROR_OK != rc) {
    return rc;
  }

  while (!GetRunning()) {
    WelsSleep (1);
  }

  return WELS_THREAD_ERROR_OK;
}

void CWelsThread::Kill() {
  if (!GetRunning()) {
    return;
  }

  SetEndFlag (true);

  WelsEventSignal (&m_hEvent,&m_hMutex,&m_iConVar);
  WelsThreadJoin (m_hThread);
  return;
}

WELS_THREAD_ROUTINE_TYPE  CWelsThread::TheThread (void* pParam) {
  CWelsThread* pThis = static_cast<CWelsThread*> (pParam);

  pThis->SetRunning (true);

  pThis->Thread();

  WELS_THREAD_ROUTINE_RETURN (NULL);
}

}


