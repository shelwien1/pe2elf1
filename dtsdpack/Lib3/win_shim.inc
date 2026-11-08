
// win_shim.inc -- the WinAPI subset used by MTwrap_v3 / rzwrap, on POSIX.
//
// _WIN32: just <windows.h>, exactly what the sources included before.
// POSIX : minimal emulation, same calling convention and semantics as far as
//         the callers depend on them:
//
//   VirtualAlloc  -> anonymous mmap: zero-filled and page-aligned, like
//                    MEM_COMMIT. The callers never run constructors and rely on
//                    the zero fill. One extra page in front of each block keeps
//                    the mapping size, so VirtualFree(p,0,MEM_RELEASE) can
//                    munmap the whole extent (munmap needs the length, Win32
//                    looks it up in the VAD).
//   CreateThread  -> pthread_create behind a small heap handle
//   CreateEventA  -> mutex + condvar + flag; auto-reset or manual-reset
//   SetEvent / ResetEvent
//   WaitForSingleObject(thread,INFINITE) -> pthread_join
//   WaitForSingleObject(event,ms)        -> condvar wait, consumes the signal
//                                           of an auto-reset event
//   CloseHandle   -> thread: detach if not joined; event: destroy; then free
//   Sleep         -> nanosleep with EINTR retry, Sleep(0) -> sched_yield
//   __debugbreak  -> __builtin_trap
//
// DWORD stays 32-bit on LP64 (unsigned int, not unsigned long).

#pragma once

#ifdef _WIN32

#include <windows.h>

#else

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sched.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/types.h>

typedef void*        HANDLE;
typedef void*        LPVOID;
typedef unsigned int DWORD;
typedef DWORD*       LPDWORD;
typedef int          BOOL;

#ifndef WINAPI
#define WINAPI
#endif

enum {
  MEM_COMMIT     = 0x00001000,
  MEM_RESERVE    = 0x00002000,
  MEM_RELEASE    = 0x00008000,
  PAGE_READWRITE = 0x04
};

#ifndef INFINITE
#define INFINITE 0xFFFFFFFFu
#endif
#ifndef WAIT_OBJECT_0
#define WAIT_OBJECT_0 0u
#endif
#ifndef WAIT_TIMEOUT
#define WAIT_TIMEOUT 0x102u
#endif
#ifndef WAIT_FAILED
#define WAIT_FAILED 0xFFFFFFFFu
#endif

#define __debugbreak() __builtin_trap()

enum { win_shim_pagesize = 4096 };

static inline void* VirtualAlloc( void* /*addr*/, size_t size, DWORD /*type*/, DWORD /*prot*/ ) {
  size_t total = ((size + win_shim_pagesize-1) & ~size_t(win_shim_pagesize-1)) + win_shim_pagesize;
  void* p = mmap( 0, total, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0 );
  if( p==MAP_FAILED ) return 0;
  *(size_t*)p = total;
  return (char*)p + win_shim_pagesize;
}

static inline BOOL VirtualFree( void* p, size_t /*size*/, DWORD /*type*/ ) {
  if( p==0 ) return 0;
  char* base = (char*)p - win_shim_pagesize;
  return munmap( base, *(size_t*)base )==0;
}

static inline void Sleep( DWORD ms ) {
  if( ms==0 ) { sched_yield(); return; }
  struct timespec ts, rem;
  ts.tv_sec  = ms/1000;
  ts.tv_nsec = long(ms%1000)*1000000L;
  while( nanosleep(&ts,&rem)!=0 && errno==EINTR ) ts = rem;
}

typedef DWORD (WINAPI *LPTHREAD_START_ROUTINE)( LPVOID );

enum { win_shim_THREAD=1, win_shim_EVENT=2 };

struct win_shim_thread {
  int type; // win_shim_THREAD
  pthread_t th;
  LPTHREAD_START_ROUTINE fn;
  LPVOID arg;
  int joined;
};

struct win_shim_event {
  int type; // win_shim_EVENT
  pthread_mutex_t m;
  pthread_cond_t c;
  int signaled;
  int manual;
};

static void* win_shim_thread_entry( void* p ) {
  win_shim_thread* t = (win_shim_thread*)p;
  t->fn( t->arg );
  return 0;
}

static inline HANDLE CreateThread( void* /*sec*/, size_t stack, LPTHREAD_START_ROUTINE fn, LPVOID arg, DWORD /*flags*/, LPDWORD tid ) {
  win_shim_thread* t = (win_shim_thread*)calloc( 1, sizeof(win_shim_thread) );
  if( t==0 ) return 0;
  t->type = win_shim_THREAD; t->fn = fn; t->arg = arg;
  pthread_attr_t attr; pthread_attr_init( &attr );
  if( stack ) pthread_attr_setstacksize( &attr, stack );
  int r = pthread_create( &t->th, &attr, win_shim_thread_entry, t );
  pthread_attr_destroy( &attr );
  if( r!=0 ) { free(t); return 0; }
  if( tid ) *tid = DWORD(uintptr_t(t));
  return (HANDLE)t;
}

static inline HANDLE CreateEventA( void* /*sec*/, BOOL manual, BOOL initial, const char* /*name*/ ) {
  win_shim_event* e = (win_shim_event*)calloc( 1, sizeof(win_shim_event) );
  if( e==0 ) return 0;
  e->type = win_shim_EVENT; e->manual = manual!=0; e->signaled = initial!=0;
  pthread_mutex_init( &e->m, 0 );
  pthread_cond_init( &e->c, 0 );
  return (HANDLE)e;
}

static inline BOOL SetEvent( HANDLE h ) {
  win_shim_event* e = (win_shim_event*)h;
  if( e==0 || e->type!=win_shim_EVENT ) return 0;
  pthread_mutex_lock( &e->m );
  e->signaled = 1;
  if( e->manual ) pthread_cond_broadcast( &e->c ); else pthread_cond_signal( &e->c );
  pthread_mutex_unlock( &e->m );
  return 1;
}

static inline BOOL ResetEvent( HANDLE h ) {
  win_shim_event* e = (win_shim_event*)h;
  if( e==0 || e->type!=win_shim_EVENT ) return 0;
  pthread_mutex_lock( &e->m );
  e->signaled = 0;
  pthread_mutex_unlock( &e->m );
  return 1;
}

static inline DWORD WaitForSingleObject( HANDLE h, DWORD ms ) {
  if( h==0 ) return WAIT_FAILED;

  if( *(int*)h==win_shim_THREAD ) {
    win_shim_thread* t = (win_shim_thread*)h;
    if( ms!=INFINITE ) return WAIT_FAILED; // only the INFINITE join is used here
    if( t->joined ) return WAIT_OBJECT_0;
    if( pthread_join(t->th,0)!=0 ) return WAIT_FAILED;
    t->joined = 1;
    return WAIT_OBJECT_0;
  }

  win_shim_event* e = (win_shim_event*)h;
  if( e->type!=win_shim_EVENT ) return WAIT_FAILED;
  struct timespec ts;
  if( ms!=INFINITE ) {
    clock_gettime( CLOCK_REALTIME, &ts );
    ts.tv_sec  += ms/1000;
    ts.tv_nsec += long(ms%1000)*1000000L;
    if( ts.tv_nsec>=1000000000L ) { ts.tv_sec++; ts.tv_nsec-=1000000000L; }
  }
  DWORD r = WAIT_OBJECT_0;
  pthread_mutex_lock( &e->m );
  while( !e->signaled ) {
    if( ms==INFINITE ) pthread_cond_wait( &e->c, &e->m );
    else if( pthread_cond_timedwait( &e->c, &e->m, &ts )==ETIMEDOUT ) { r = e->signaled ? WAIT_OBJECT_0 : WAIT_TIMEOUT; break; }
  }
  if( (r==WAIT_OBJECT_0) && !e->manual ) e->signaled = 0;
  pthread_mutex_unlock( &e->m );
  return r;
}

static inline BOOL CloseHandle( HANDLE h ) {
  if( h==0 ) return 0;
  if( *(int*)h==win_shim_THREAD ) {
    win_shim_thread* t = (win_shim_thread*)h;
    if( !t->joined ) pthread_detach( t->th );
    free( t );
    return 1;
  }
  if( *(int*)h==win_shim_EVENT ) {
    win_shim_event* e = (win_shim_event*)h;
    pthread_cond_destroy( &e->c );
    pthread_mutex_destroy( &e->m );
    free( e );
    return 1;
  }
  return 0;
}

#endif
