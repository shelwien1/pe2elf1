
#include "common.inc"

#ifdef _WIN32
#ifndef _WIN32_WINNT
  #define _WIN32_WINNT 0x0500
#endif
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN   
#include <windows.h>
#else
#include <dlfcn.h>
#endif

template< typename funcptr >
uint dll_load( const char* dllname, const char* funcname, funcptr* p_func ) {
#ifdef _WIN32
  HMODULE dll = LoadLibrary( dllname );
  if( dll==0 ) {
    return 1;
  }
  void* q = (int*)GetProcAddress( dll, funcname );
  if( q==0 ) {
    return 2;
  }
  *p_func = decltype(*p_func)(q);
  return 0;
#else
  void *handle = dlopen(dllname, RTLD_LAZY);
  if(handle == 0) return 25;
  void* q = (int*)dlsym(handle, funcname);
  if(q == 0) return 26;
  *p_func = decltype(*p_func)(q);
  return 0;
#endif
}

//extern "C" __declspec(dllimport) void __stdcall Sleep( uint );

//extern "C" __declspec(dllimport) uint __stdcall GetTickCount( void );
#if defined(__GNUC__) && !defined(_WIN32)

// 100ns ticks, the unit QueryUnbiasedInterruptTime uses on the Windows side.
// CLOCK_MONOTONIC rather than gettimeofday so the benchmark cannot be moved by
// a clock adjustment mid-run.
#include <time.h>
qword GetTickCount100ns(void) {
  timespec t;
  clock_gettime( CLOCK_MONOTONIC, &t );
  return qword(t.tv_sec)*10000000ULL + qword(t.tv_nsec)/100;
}

#else

//#ifndef _INC_WINDOWS
#ifndef _WINDOWS_
extern "C" __declspec(dllimport) unsigned __stdcall GetTickCount( void );
#endif
//#endif

#endif

#ifdef _WIN32
typedef ULONGLONG (__stdcall* t_QueryUnbiasedInterruptTimePrecise)( PULONGLONG );
t_QueryUnbiasedInterruptTimePrecise p_QueryUnbiasedInterruptTimePrecise = 0;

typedef BOOL (__stdcall* t_QueryUnbiasedInterruptTime)( PULONGLONG UnbiasedTime );
t_QueryUnbiasedInterruptTime p_QueryUnbiasedInterruptTime = 0;

extern uint g_LoadTimeFunctions;

uint LoadTimeFunctions() {
  g_LoadTimeFunctions = -1;
  if( dll_load( "ntdll.dll", "RtlGetInterruptTimePrecise", &p_QueryUnbiasedInterruptTimePrecise ) ) {
  if( dll_load("kernel32.dll", "QueryUnbiasedInterruptTime", &p_QueryUnbiasedInterruptTime) ) {
    return 0;
  }
  }
  return 1;
}

uint g_LoadTimeFunctions = LoadTimeFunctions();

#endif

qword GetTickCount1( void ) {
#ifdef _WIN32
  qword r = 0;
  if( p_QueryUnbiasedInterruptTimePrecise ) {
    qword r1=0;
    r = (*p_QueryUnbiasedInterruptTimePrecise)(&r1);
  } else if( p_QueryUnbiasedInterruptTime ) {
    (*p_QueryUnbiasedInterruptTime)(&r);
  } else {
    r = qword(GetTickCount() * 10000);
  }
  return r;
#else
  return GetTickCount100ns();
#endif
}

qword starttick,lasttick,curtick,fintick;

#define StartTimer() (starttick=lasttick=GetTickCount1())
//#define StartTimer(xxx) (starttick=GetTickCount(),lasttick=starttick+xxx)

#define CheckTimer(xxx) (curtick=GetTickCount1(),(lasttick<=curtick)?lasttick=curtick+(xxx),1:0)

#define BreakTimer() (fintick=GetTickCount1()-starttick)

#define PrintTimer() (printf("%i.%03is",fintick/10000000,(fintick%10000000)/10000)

