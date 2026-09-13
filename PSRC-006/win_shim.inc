
#pragma once

// -------------------------------------------------------------------------
// Portable shims for the handful of Win32 entry points used by the MT layer.
//
//   VirtualAlloc / VirtualFree
//   CreateThread / WaitForSingleObject / CloseHandle / Sleep
//   GetCurrentThread / SetThreadAffinityMask
//
// _WIN32 : the prototypes are declared here directly, so <windows.h> is never
//          included.  Signatures and calling convention match <windows.h>
//          exactly, so linking against kernel32 is unchanged.
// else   : POSIX implementation on mmap + pthreads.
//
// Include AFTER common.inc (needs byte/uint/qword, ALIGN, INLINE).
// -------------------------------------------------------------------------

#if defined(_WINDOWS_) || defined(_WINDOWS_H)
  // <windows.h> got included after all - nothing to do, everything below is
  // already declared and would collide.
#else

#ifndef WIN_SHIM_TYPES
#define WIN_SHIM_TYPES

typedef void*          HANDLE;
typedef void*          LPVOID;
typedef const void*    LPCVOID;
typedef int            BOOL;

#if defined(_WIN32)
typedef unsigned long  DWORD;   // <windows.h> spelling; 32 bits on Win32/Win64
#else
typedef unsigned int   DWORD;   // 32 bits on LP64 too
#endif

typedef DWORD*         LPDWORD;
typedef size_t         SIZE_T;
typedef size_t         DWORD_PTR;
typedef size_t         ULONG_PTR;

#endif // WIN_SHIM_TYPES

#ifndef WINAPI
 #if defined(_WIN32)
  #define WINAPI __stdcall
 #else
  #define WINAPI
 #endif
#endif

typedef DWORD (WINAPI *LPTHREAD_START_ROUTINE)( LPVOID );

#ifndef INFINITE
#define INFINITE        0xFFFFFFFFu
#endif
#ifndef WAIT_OBJECT_0
#define WAIT_OBJECT_0   0x00000000u
#define WAIT_TIMEOUT    0x00000102u
#define WAIT_FAILED     0xFFFFFFFFu
#endif

#ifndef MEM_COMMIT
#define MEM_COMMIT      0x00001000u
#define MEM_RESERVE     0x00002000u
#define MEM_DECOMMIT    0x00004000u
#define MEM_RELEASE     0x00008000u
#endif
#ifndef PAGE_READWRITE
#define PAGE_NOACCESS   0x01u
#define PAGE_READONLY   0x02u
#define PAGE_READWRITE  0x04u
#define PAGE_WRITECOPY  0x08u
#endif


// =========================================================================
#if defined(_WIN32)
// =========================================================================

extern "C" {

__declspec(dllimport) LPVOID    WINAPI VirtualAlloc( LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect );
__declspec(dllimport) BOOL      WINAPI VirtualFree( LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType );

__declspec(dllimport) HANDLE    WINAPI CreateThread( LPVOID lpThreadAttributes, SIZE_T dwStackSize,
                                                     LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter,
                                                     DWORD dwCreationFlags, LPDWORD lpThreadId );
__declspec(dllimport) DWORD     WINAPI WaitForSingleObject( HANDLE hHandle, DWORD dwMilliseconds );
__declspec(dllimport) BOOL      WINAPI CloseHandle( HANDLE hObject );
__declspec(dllimport) void      WINAPI Sleep( DWORD dwMilliseconds );

__declspec(dllimport) HANDLE    WINAPI GetCurrentThread( void );
__declspec(dllimport) DWORD_PTR WINAPI SetThreadAffinityMask( HANDLE hThread, DWORD_PTR dwThreadAffinityMask );

}

// =========================================================================
#else // POSIX
// =========================================================================

#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <time.h>
#include <errno.h>

#ifndef MAP_ANONYMOUS
 #ifdef MAP_ANON
  #define MAP_ANONYMOUS MAP_ANON
 #endif
#endif

// --- memory --------------------------------------------------------------
// VirtualAlloc(0,n,MEM_COMMIT,PAGE_READWRITE) -> anonymous zero-filled mmap.
// Both round the size up to a page and both hand back zeroed pages, which is
// what the callers rely on (the ring buffers and the model tables are used
// without a constructor run).  lpAddress is treated as a hint, never MAP_FIXED.

static void* VirtualAlloc( void* lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect ) {
  if( dwSize==0 ) return 0;
  if( (flAllocationType&(MEM_COMMIT|MEM_RESERVE))==0 ) return 0;

  int prot;
  switch( flProtect ) {
    case PAGE_NOACCESS : prot = PROT_NONE;            break;
    case PAGE_READONLY : prot = PROT_READ;            break;
    default            : prot = PROT_READ|PROT_WRITE; break;
  }

  void* p = mmap( lpAddress, dwSize, prot, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0 );
  return (p==MAP_FAILED) ? 0 : p;
}

// NOTE: Win32 takes dwSize==0 with MEM_RELEASE and looks the region size up in
// the VAD.  munmap has no such table, so the POSIX build needs the real size.
static BOOL VirtualFree( void* lpAddress, SIZE_T dwSize, DWORD dwFreeType ) {
  if( (dwFreeType&MEM_RELEASE) && dwSize==0 ) return 0; // unsupported here
  return munmap( lpAddress, dwSize )==0;
}

// --- threads -------------------------------------------------------------

struct win_shim_thread {
  pthread_t              th;
  LPTHREAD_START_ROUTINE fn;
  LPVOID                 arg;
  DWORD                  ret;
  int                    joined;
};

// GetCurrentThread() returns a pseudo-handle on Win32; mirror that so
// SetThreadAffinityMask(GetCurrentThread(),..) keeps working unchanged.
#define WIN_SHIM_CURTHREAD ((HANDLE)(~(ptrdiff_t)1))

static void* win_shim_thread_entry( void* p ) {
  win_shim_thread* h = (win_shim_thread*)p;
  h->ret = h->fn( h->arg );
  return 0;
}

// dwCreationFlags: only 0 is supported (no CREATE_SUSPENDED).
static HANDLE CreateThread( LPVOID, SIZE_T dwStackSize,
                            LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter,
                            DWORD dwCreationFlags, LPDWORD lpThreadId ) {
  if( dwCreationFlags!=0 ) return 0;

  win_shim_thread* h = (win_shim_thread*)malloc( sizeof(win_shim_thread) );
  if( h==0 ) return 0;
  h->fn = lpStartAddress; h->arg = lpParameter; h->ret = 0; h->joined = 0;

  pthread_attr_t at; pthread_attr_t* pat = 0;
  if( dwStackSize!=0 ) {
    pthread_attr_init( &at );
    size_t sz = dwStackSize;
#ifdef PTHREAD_STACK_MIN
    if( sz<(size_t)PTHREAD_STACK_MIN ) sz = PTHREAD_STACK_MIN;
#endif
    pthread_attr_setstacksize( &at, sz );
    pat = &at;
  }

  int r = pthread_create( &h->th, pat, win_shim_thread_entry, h );
  if( pat ) pthread_attr_destroy( pat );
  if( r!=0 ) { free(h); return 0; }

  if( lpThreadId ) *lpThreadId = (DWORD)(size_t)h;
  return (HANDLE)h;
}

static DWORD WaitForSingleObject( HANDLE hHandle, DWORD dwMilliseconds ) {
  win_shim_thread* h = (win_shim_thread*)hHandle;
  if( h==0 || hHandle==WIN_SHIM_CURTHREAD ) return WAIT_FAILED;
  if( h->joined ) return WAIT_OBJECT_0;

  if( dwMilliseconds==INFINITE ) {
    if( pthread_join(h->th,0)!=0 ) return WAIT_FAILED;
    h->joined = 1;
    return WAIT_OBJECT_0;
  }

#if defined(__GLIBC__)
  struct timespec ts;
  clock_gettime( CLOCK_REALTIME, &ts );
  ts.tv_sec  += dwMilliseconds/1000;
  ts.tv_nsec += (long)(dwMilliseconds%1000)*1000000L;
  if( ts.tv_nsec>=1000000000L ) { ts.tv_nsec-=1000000000L; ts.tv_sec++; }
  int r = pthread_timedjoin_np( h->th, 0, &ts );
  if( r==ETIMEDOUT ) return WAIT_TIMEOUT;
  if( r!=0 ) return WAIT_FAILED;
  h->joined = 1;
  return WAIT_OBJECT_0;
#else
  // No timed join available - degrade to a blocking join.
  if( pthread_join(h->th,0)!=0 ) return WAIT_FAILED;
  h->joined = 1;
  return WAIT_OBJECT_0;
#endif
}

static BOOL CloseHandle( HANDLE hObject ) {
  win_shim_thread* h = (win_shim_thread*)hObject;
  if( h==0 || hObject==WIN_SHIM_CURTHREAD ) return 0;
  if( h->joined==0 ) pthread_detach( h->th );
  free( h );
  return 1;
}

static void Sleep( DWORD dwMilliseconds ) {
  if( dwMilliseconds==0 ) { sched_yield(); return; }
  struct timespec ts;
  ts.tv_sec  = dwMilliseconds/1000;
  ts.tv_nsec = (long)(dwMilliseconds%1000)*1000000L;
  while( nanosleep(&ts,&ts)==-1 && errno==EINTR ) {}
}

// --- affinity ------------------------------------------------------------

static HANDLE GetCurrentThread( void ) { return WIN_SHIM_CURTHREAD; }

static DWORD_PTR SetThreadAffinityMask( HANDLE hThread, DWORD_PTR dwThreadAffinityMask ) {
#if defined(CPU_SETSIZE) && defined(__linux__)
  pthread_t t = (hThread==WIN_SHIM_CURTHREAD) ? pthread_self()
                                              : ((win_shim_thread*)hThread)->th;
  cpu_set_t set; CPU_ZERO( &set );
  unsigned i, n = 8*sizeof(DWORD_PTR);
  if( n>CPU_SETSIZE ) n = CPU_SETSIZE;
  for( i=0; i<n; i++ ) if( (dwThreadAffinityMask>>i)&1 ) CPU_SET( i, &set );
  // Win32 returns the PREVIOUS mask, 0 on failure.  The previous mask is not
  // retrievable portably, so a success is reported as the mask just set.
  return pthread_setaffinity_np( t, sizeof(set), &set )==0 ? dwThreadAffinityMask : 0;
#else
  (void)hThread; (void)dwThreadAffinityMask;
  return 0; // no portable equivalent
#endif
}

#endif // _WIN32 / POSIX

#endif // _WINDOWS_
