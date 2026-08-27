
#include <windows.h>

#include "common.inc"

uint fix_thread() {
  uint pn = GetCurrentProcessorNumber();
  HANDLE curP = GetCurrentProcess();
  HANDLE curT = GetCurrentThread();
  qword proc_mask=1<<pn;
  SetProcessAffinityMask( curP, proc_mask );
  SetThreadPriority( curT, 15 );
//printf( "!fix_thread!\n" );
  return pn;
}

volatile uint g_fix_thread = fix_thread();
