// winapi_posix.h -- the Win32 functions mp4pack.cpp uses, on POSIX (Linux)
//
// Just what mp4pack needs of each, with Windows' behaviour where mp4pack
// depends on it:
// - wide strings are wchar_t (UTF-32 here), UTF-8 to the system; a byte that
//   is not valid UTF-8 becomes U+DC80..U+DCFF and goes back as it was, so any
//   file name gets through;
// - HANDLEs are objects of this file: files and pipes (ReadFile at the end of
//   a pipe fails with ERROR_BROKEN_PIPE, as on Windows), processes, threads,
//   events, searches; waits are on one mutex and condition;
// - CreateProcessW forks and execs the program with the STARTUPINFO's handles
//   as its stdin, stdout and stderr (no other descriptor gets through: all are
//   opened close-on-exec) and the command line split as the MS C runtime
//   splits it. The child is killed when the thread that made it ends
//   (PR_SET_PDEATHSIG): mp4pack starts them from its main thread, so they die
//   with it, as with its job object on Windows. A thread for each waits for
//   its end; one killed by SIGINT gets STATUS_CONTROL_C_EXIT, by another
//   signal 128+the signal, by TerminateProcess the code given there;
// - SIGINT, SIGQUIT, SIGHUP and SIGTERM go to the SetConsoleCtrlHandler
//   routine (as CTRL_C, _BREAK, _CLOSE and _SHUTDOWN), called on a thread of
//   their own: main blocks them before anything else runs (WinapiPosixInit),
//   and the other threads inherit that. SIGPIPE is ignored: writing to a pipe
//   nobody reads fails instead;
// - main converts the arguments and calls wmain (unless WINAPI_POSIX_NO_MAIN).

#ifndef WINAPI_POSIX_H
#define WINAPI_POSIX_H

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <algorithm>
#include <string>
#include <vector>

//---- types and constants

typedef uint32_t DWORD;
typedef int BOOL;
typedef unsigned char BOOLEAN;
typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef int32_t LONG;
typedef int64_t LONGLONG;
typedef uint64_t ULONGLONG;
typedef unsigned int UINT;
typedef wchar_t WCHAR;
typedef void* HANDLE;
typedef HANDLE HMODULE;
typedef void* FARPROC;
#define WINAPI
#define TRUE 1
#define FALSE 0

#define MAX_PATH 260
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define INFINITE 0xFFFFFFFFu
#define WAIT_OBJECT_0 0u
#define WAIT_TIMEOUT 258u
#define WAIT_FAILED 0xFFFFFFFFu
#define MAXIMUM_WAIT_OBJECTS 64
#define STILL_ACTIVE 259u

#define CP_ACP 0u
#define CP_UTF8 65001u
#define WC_NO_BEST_FIT_CHARS 0x400u

#define GENERIC_READ 0x80000000u
#define GENERIC_WRITE 0x40000000u
#define FILE_SHARE_READ 1u
#define FILE_SHARE_WRITE 2u
#define FILE_SHARE_DELETE 4u
#define CREATE_NEW 1u
#define CREATE_ALWAYS 2u
#define OPEN_EXISTING 3u
#define OPEN_ALWAYS 4u
#define TRUNCATE_EXISTING 5u
#define FILE_FLAG_SEQUENTIAL_SCAN 0x08000000u
#define FILE_BEGIN 0u
#define FILE_CURRENT 1u
#define FILE_END 2u
#define FILE_ATTRIBUTE_READONLY 0x1u
#define FILE_ATTRIBUTE_DIRECTORY 0x10u
#define FILE_ATTRIBUTE_ARCHIVE 0x20u
#define FILE_ATTRIBUTE_NORMAL 0x80u
#define FILE_ATTRIBUTE_REPARSE_POINT 0x400u
#define MOVEFILE_REPLACE_EXISTING 1u
#define MOVEFILE_COPY_ALLOWED 2u
#define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE 2u

#define MEM_COMMIT 0x1000u
#define MEM_RESERVE 0x2000u
#define MEM_RELEASE 0x8000u
#define PAGE_READWRITE 0x4u

#define HANDLE_FLAG_INHERIT 1u
#define STARTF_USESTDHANDLES 0x100u
#define CREATE_SUSPENDED 4u
#define DUPLICATE_SAME_ACCESS 2u
#define STD_INPUT_HANDLE ((DWORD)-10)
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define STD_ERROR_HANDLE ((DWORD)-12)
#define SEM_FAILCRITICALERRORS 0x1u
#define SEM_NOGPFAULTERRORBOX 0x2u
#define SEM_NOOPENFILEERRORBOX 0x8000u
#define CTRL_C_EVENT 0u
#define CTRL_BREAK_EVENT 1u
#define CTRL_CLOSE_EVENT 2u
#define CTRL_LOGOFF_EVENT 5u
#define CTRL_SHUTDOWN_EVENT 6u
#define JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE 0x2000u
#define STATUS_CONTROL_C_EXIT 0xC000013Au

#define ERROR_SUCCESS 0u
#define ERROR_INVALID_FUNCTION 1u
#define ERROR_FILE_NOT_FOUND 2u
#define ERROR_PATH_NOT_FOUND 3u
#define ERROR_TOO_MANY_OPEN_FILES 4u
#define ERROR_ACCESS_DENIED 5u
#define ERROR_INVALID_HANDLE 6u
#define ERROR_NOT_ENOUGH_MEMORY 8u
#define ERROR_BAD_FORMAT 11u
#define ERROR_NOT_SAME_DEVICE 17u
#define ERROR_NO_MORE_FILES 18u
#define ERROR_WRITE_PROTECT 19u
#define ERROR_GEN_FAILURE 31u
#define ERROR_NOT_SUPPORTED 50u
#define ERROR_FILE_EXISTS 80u
#define ERROR_INVALID_PARAMETER 87u
#define ERROR_BROKEN_PIPE 109u
#define ERROR_DISK_FULL 112u
#define ERROR_CALL_NOT_IMPLEMENTED 120u
#define ERROR_INSUFFICIENT_BUFFER 122u
#define ERROR_INVALID_NAME 123u
#define ERROR_DIR_NOT_EMPTY 145u
#define ERROR_ALREADY_EXISTS 183u
#define ERROR_FILENAME_EXCED_RANGE 206u

struct FILETIME { DWORD dwLowDateTime, dwHighDateTime; };
struct LARGE_INTEGER { LONGLONG QuadPart; };
struct ULARGE_INTEGER { ULONGLONG QuadPart; };
struct SECURITY_ATTRIBUTES { DWORD nLength; void* lpSecurityDescriptor; BOOL bInheritHandle; };
struct COORD { short X, Y; };
struct SMALL_RECT { short Left, Top, Right, Bottom; };
struct CONSOLE_SCREEN_BUFFER_INFO { COORD dwSize, dwCursorPosition; WORD wAttributes; SMALL_RECT srWindow; COORD dwMaximumWindowSize; };
struct STARTUPINFOW {
  DWORD cb; WCHAR *lpReserved, *lpDesktop, *lpTitle;
  DWORD dwX, dwY, dwXSize, dwYSize, dwXCountChars, dwYCountChars, dwFillAttribute, dwFlags;
  WORD wShowWindow, cbReserved2; BYTE* lpReserved2;
  HANDLE hStdInput, hStdOutput, hStdError;
};
struct PROCESS_INFORMATION { HANDLE hProcess, hThread; DWORD dwProcessId, dwThreadId; };
struct WIN32_FIND_DATAW {
  DWORD dwFileAttributes; FILETIME ftCreationTime, ftLastAccessTime, ftLastWriteTime;
  DWORD nFileSizeHigh, nFileSizeLow, dwReserved0, dwReserved1;
  WCHAR cFileName[MAX_PATH]; WCHAR cAlternateFileName[14];
};
enum GET_FILEEX_INFO_LEVELS { GetFileExInfoStandard };
struct WIN32_FILE_ATTRIBUTE_DATA { DWORD dwFileAttributes; FILETIME ftCreationTime, ftLastAccessTime, ftLastWriteTime; DWORD nFileSizeHigh, nFileSizeLow; };
struct BY_HANDLE_FILE_INFORMATION {
  DWORD dwFileAttributes; FILETIME ftCreationTime, ftLastAccessTime, ftLastWriteTime;
  DWORD dwVolumeSerialNumber, nFileSizeHigh, nFileSizeLow, nNumberOfLinks, nFileIndexHigh, nFileIndexLow;
};
struct SYSTEM_INFO {
  WORD wProcessorArchitecture, wReserved; DWORD dwPageSize; void *lpMinimumApplicationAddress, *lpMaximumApplicationAddress;
  uintptr_t dwActiveProcessorMask; DWORD dwNumberOfProcessors, dwProcessorType, dwAllocationGranularity; WORD wProcessorLevel, wProcessorRevision;
};
struct JOBOBJECT_BASIC_LIMIT_INFORMATION {
  LONGLONG PerProcessUserTimeLimit, PerJobUserTimeLimit; DWORD LimitFlags; size_t MinimumWorkingSetSize, MaximumWorkingSetSize;
  DWORD ActiveProcessLimit; uintptr_t Affinity; DWORD PriorityClass, SchedulingClass;
};
struct IO_COUNTERS { ULONGLONG ReadOperationCount, WriteOperationCount, OtherOperationCount, ReadTransferCount, WriteTransferCount, OtherTransferCount; };
struct JOBOBJECT_EXTENDED_LIMIT_INFORMATION {
  JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimitInformation; IO_COUNTERS IoInfo;
  size_t ProcessMemoryLimit, JobMemoryLimit, PeakProcessMemoryUsed, PeakJobMemoryUsed;
};
enum JOBOBJECTINFOCLASS { JobObjectExtendedLimitInformation = 9 };
typedef DWORD (*LPTHREAD_START_ROUTINE)( void* );
typedef BOOL (*PHANDLER_ROUTINE)( DWORD );

//---- errors

inline DWORD& WpLastError() { static thread_local DWORD e; return e; }
inline DWORD GetLastError() { return WpLastError(); }
inline void SetLastError( DWORD e ) { WpLastError() = e; }

inline DWORD WpError( int e ) {
  switch( e ) {
  case 0: return ERROR_SUCCESS;
  case ENOENT: return ERROR_FILE_NOT_FOUND;
  case ENOTDIR: return ERROR_PATH_NOT_FOUND;
  case EACCES: case EPERM: case EISDIR: case ETXTBSY: return ERROR_ACCESS_DENIED;
  case EEXIST: return ERROR_ALREADY_EXISTS;
  case ENOTEMPTY: return ERROR_DIR_NOT_EMPTY;
  case EXDEV: return ERROR_NOT_SAME_DEVICE;
  case ENOSPC: case EDQUOT: return ERROR_DISK_FULL;
  case EMFILE: case ENFILE: return ERROR_TOO_MANY_OPEN_FILES;
  case ENOMEM: return ERROR_NOT_ENOUGH_MEMORY;
  case EBADF: return ERROR_INVALID_HANDLE;
  case EINVAL: return ERROR_INVALID_PARAMETER;
  case EPIPE: return ERROR_BROKEN_PIPE;
  case ENAMETOOLONG: return ERROR_FILENAME_EXCED_RANGE;
  case EROFS: return ERROR_WRITE_PROTECT;
  case ENOEXEC: return ERROR_BAD_FORMAT;
  case ENOSYS: case EOPNOTSUPP: return ERROR_NOT_SUPPORTED;
  }
  return ERROR_GEN_FAILURE;
}

inline BOOL WpFail( int e ) { SetLastError( WpError(e) ); return FALSE; }

//---- strings: UTF-8 and wide

// a byte of no valid UTF-8 sequence becomes U+DC00+byte (U+DC80..U+DCFF)
inline std::wstring WpWide( const char* s, size_t n ) {
  std::wstring w;
  const unsigned char* p = (const unsigned char*)s;
  for( size_t i=0; i<n; ) {
    unsigned c = p[i], k = (c>=0xF0) && (c<=0xF4) ? 3 : (c>=0xE0) ? ((c<=0xEF) ? 2 : 9) : (c>=0xC2) ? 1 : (c<0x80) ? 0 : 9;
    uint32_t v = (k==0) ? c : (k==1) ? (c&0x1F) : (k==2) ? (c&0x0F) : (c&0x07);
    size_t j = 1;
    if( k==9 ) k = 0, v = 0xDC00+c;
    else for(; (j<=k) && (i+j<n) && ((p[i+j]&0xC0)==0x80); j++ ) v = (v<<6) | (p[i+j]&0x3F);
    if( (j<=k) || ((k==2) && (v<0x800)) || ((k==3) && ((v<0x10000) || (v>0x10FFFF))) || ((v>=0xD800) && (v<=0xDFFF) && (k>0)) ) {
      w += wchar_t(0xDC00+c); i++; continue; // (not UTF-8: the lead byte alone)
    }
    w += wchar_t(v); i += j;
  }
  return w;
}

inline std::wstring WpWide( const std::string& s ) { return WpWide( s.data(), s.size() ); }

// U+DC80..U+DCFF back to their bytes; other surrogates (not from here) as U+FFFD
inline std::string WpUtf8( const wchar_t* w, size_t n ) {
  std::string s;
  for( size_t i=0; i<n; i++ ) {
    uint32_t c = uint32_t(w[i]);
    if( (c>=0xDC80) && (c<=0xDCFF) ) { s += char(c-0xDC00); continue; }
    if( ((c>=0xD800) && (c<=0xDFFF)) || (c>0x10FFFF) ) c = 0xFFFD;
    if( c<0x80 ) s += char(c);
    else if( c<0x800 ) { s += char(0xC0|(c>>6)); s += char(0x80|(c&0x3F)); }
    else if( c<0x10000 ) { s += char(0xE0|(c>>12)); s += char(0x80|((c>>6)&0x3F)); s += char(0x80|(c&0x3F)); }
    else { s += char(0xF0|(c>>18)); s += char(0x80|((c>>12)&0x3F)); s += char(0x80|((c>>6)&0x3F)); s += char(0x80|(c&0x3F)); }
  }
  return s;
}

inline std::string WpUtf8( const wchar_t* w ) { return w ? WpUtf8( w, wcslen(w) ) : std::string(); }

// as on Windows: srclen -1 takes the NUL too; dstlen 0 asks for the size
inline int MultiByteToWideChar( UINT, DWORD, const char* s, int n, wchar_t* w, int m ) {
  if( !s || (n==0) || (m<0) ) { SetLastError( ERROR_INVALID_PARAMETER ); return 0; }
  std::wstring r = WpWide( s, (n<0) ? strlen(s)+1 : size_t(n) );
  if( m==0 ) return int(r.size());
  if( r.size()>size_t(m) ) { SetLastError( ERROR_INSUFFICIENT_BUFFER ); return 0; }
  if( r.size() ) memcpy( w, r.data(), r.size()*sizeof(wchar_t) );
  return int(r.size());
}

inline int WideCharToMultiByte( UINT, DWORD, const wchar_t* w, int n, char* s, int m, const char*, BOOL* used ) {
  if( used ) *used = FALSE;
  if( !w || (n==0) || (m<0) ) { SetLastError( ERROR_INVALID_PARAMETER ); return 0; }
  std::string r = WpUtf8( w, (n<0) ? wcslen(w)+1 : size_t(n) );
  if( m==0 ) return int(r.size());
  if( r.size()>size_t(m) ) { SetLastError( ERROR_INSUFFICIENT_BUFFER ); return 0; }
  if( r.size() ) memcpy( s, r.data(), r.size() );
  return int(r.size());
}

inline UINT GetACP() { return CP_UTF8; }

inline int lstrcmpiW( const wchar_t* a, const wchar_t* b ) {
  for(;; a++, b++ ) {
    wchar_t x = *a, y = *b;
    if( (x>='A') && (x<='Z') ) x += 'a'-'A';
    if( (y>='A') && (y<='Z') ) y += 'a'-'A';
    if( x!=y ) return (x<y) ? -1 : 1;
    if( !x ) return 0;
  }
}

//---- handles

enum { WP_FILE, WP_PROCESS, WP_THREAD, WP_EVENT, WP_FIND, WP_OTHER };

struct WpObj {
  int kind, refs;
  int fd, pipe;            // file: the descriptor; a pipe's end
  int done;                // process, thread: it has ended; event: it is set
  int manual;              // event: manual reset
  DWORD code;              // process, thread: the exit code
  pid_t pid;               // process
  int killed; DWORD kcode; // process: TerminateProcess's code
  DIR* dir; std::string dpath, pat; // search
  WpObj( int k ) : kind(k), refs(1), fd(-1), pipe(0), done(0), manual(0), code(0), pid(0), killed(0), kcode(0), dir(0) {}
};

inline pthread_mutex_t* WpMutex() { static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; return &m; }

// signaled when a process or thread ends or an event is set (monotonic clock)
inline pthread_cond_t* WpCond() {
  static pthread_cond_t* c = [] {
    static pthread_cond_t cv;
    pthread_condattr_t a;
    pthread_condattr_init( &a );
    pthread_condattr_setclock( &a, CLOCK_MONOTONIC );
    pthread_cond_init( &cv, &a );
    pthread_condattr_destroy( &a );
    return &cv;
  }();
  return c;
}

// one reference less (with the mutex held); the last one frees it
inline void WpRelease( WpObj* o ) {
  if( --o->refs ) return;
  if( o->fd>=0 ) close( o->fd );
  if( o->dir ) closedir( o->dir );
  delete o;
}

inline WpObj* WpFile( HANDLE h ) {
  WpObj* o = (WpObj*)h;
  if( !o || (h==INVALID_HANDLE_VALUE) || (o->kind!=WP_FILE) ) { SetLastError( ERROR_INVALID_HANDLE ); return 0; }
  return o;
}

inline HANDLE WpNewFile( int fd ) {
  struct stat st;
  WpObj* o = new WpObj( WP_FILE );
  o->fd = fd;
  o->pipe = (fstat( fd, &st )==0) && (S_ISFIFO(st.st_mode) || S_ISSOCK(st.st_mode));
  return o;
}

inline BOOL CloseHandle( HANDLE h ) {
  if( !h || (h==INVALID_HANDLE_VALUE) ) { SetLastError( ERROR_INVALID_HANDLE ); return FALSE; }
  pthread_mutex_lock( WpMutex() );
  WpRelease( (WpObj*)h );
  pthread_mutex_unlock( WpMutex() );
  return TRUE;
}

inline HANDLE GetStdHandle( DWORD n ) {
  static WpObj* h[3];
  int fd = (n==STD_INPUT_HANDLE) ? 0 : (n==STD_OUTPUT_HANDLE) ? 1 : (n==STD_ERROR_HANDLE) ? 2 : -1;
  if( fd<0 ) { SetLastError( ERROR_INVALID_PARAMETER ); return INVALID_HANDLE_VALUE; }
  pthread_mutex_lock( WpMutex() );
  if( !h[fd] ) { h[fd] = (WpObj*)WpNewFile( fd ); h[fd]->refs = 1<<30; } // (never closed)
  pthread_mutex_unlock( WpMutex() );
  return h[fd];
}

inline HANDLE GetCurrentProcess() { return (HANDLE)(intptr_t)-1; }
inline DWORD GetCurrentProcessId() { return DWORD( getpid() ); }

inline BOOL DuplicateHandle( HANDLE, HANDLE h, HANDLE, HANDLE* d, DWORD, BOOL, DWORD ) {
  WpObj* o = WpFile( h );
  if( !o ) return FALSE;
  int fd = fcntl( o->fd, F_DUPFD_CLOEXEC, 3 );
  if( fd<0 ) return WpFail( errno );
  *d = WpNewFile( fd );
  return TRUE;
}

// only the handles given as a child's stdin, stdout and stderr get to it
inline BOOL SetHandleInformation( HANDLE, DWORD, DWORD ) { return TRUE; }

inline BOOL CreatePipe( HANDLE* r, HANDLE* w, SECURITY_ATTRIBUTES*, DWORD ) {
  int p[2];
  if( pipe2( p, O_CLOEXEC ) ) return WpFail( errno );
  *r = WpNewFile( p[0] ); *w = WpNewFile( p[1] );
  return TRUE;
}

//---- files

inline std::string WpPath( const wchar_t* p ) { return WpUtf8( p ); }

inline DWORD WpAttrs( const struct stat& st ) {
  DWORD a = S_ISDIR(st.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_ARCHIVE;
  if( !(st.st_mode & (S_IWUSR|S_IWGRP|S_IWOTH)) ) a |= FILE_ATTRIBUTE_READONLY;
  return a;
}

inline FILETIME WpTime( time_t t ) {
  ULONGLONG v = ULONGLONG(t)*10000000u + 116444736000000000ull; // (since 1601)
  FILETIME f; f.dwLowDateTime = DWORD(v); f.dwHighDateTime = DWORD(v>>32);
  return f;
}

inline HANDLE CreateFileW( const wchar_t* name, DWORD access, DWORD, SECURITY_ATTRIBUTES*, DWORD disp, DWORD flags, HANDLE ) {
  std::string p = WpPath( name );
  if( p=="NUL" ) p = "/dev/null";
  int o = O_CLOEXEC | (((access&GENERIC_READ) && (access&GENERIC_WRITE)) ? O_RDWR : (access&GENERIC_WRITE) ? O_WRONLY : O_RDONLY);
  if( disp==CREATE_NEW ) o |= O_CREAT|O_EXCL;
  else if( disp==CREATE_ALWAYS ) o |= O_CREAT|O_TRUNC;
  else if( disp==OPEN_ALWAYS ) o |= O_CREAT;
  else if( disp==TRUNCATE_EXISTING ) o |= O_TRUNC;
  int fd;
  do fd = open( p.c_str(), o, 0666 ); while( (fd<0) && (errno==EINTR) );
  if( fd<0 ) {
    SetLastError( ((errno==EEXIST) && (disp==CREATE_NEW)) ? ERROR_FILE_EXISTS : WpError(errno) );
    return INVALID_HANDLE_VALUE;
  }
  struct stat st;
  if( (fstat( fd, &st )==0) && S_ISDIR(st.st_mode) ) { close( fd ); SetLastError( ERROR_ACCESS_DENIED ); return INVALID_HANDLE_VALUE; } // (as Windows)
  if( flags & FILE_FLAG_SEQUENTIAL_SCAN ) posix_fadvise( fd, 0, 0, POSIX_FADV_SEQUENTIAL );
  return WpNewFile( fd );
}

inline BOOL ReadFile( HANDLE h, void* buf, DWORD n, DWORD* got, void* ) {
  WpObj* o = WpFile( h );
  if( got ) *got = 0;
  if( !o ) return FALSE;
  ssize_t r;
  do r = read( o->fd, buf, n ); while( (r<0) && (errno==EINTR) );
  if( r<0 ) return WpFail( errno );
  if( got ) *got = DWORD(r);
  if( (r==0) && n && o->pipe ) { SetLastError( ERROR_BROKEN_PIPE ); return FALSE; } // (Windows: the other end is closed)
  return TRUE;
}

inline BOOL WriteFile( HANDLE h, const void* buf, DWORD n, DWORD* put, void* ) {
  WpObj* o = WpFile( h );
  if( put ) *put = 0;
  if( !o ) return FALSE;
  DWORD d = 0;
  while( d<n ) {
    ssize_t r = write( o->fd, (const char*)buf+d, n-d );
    if( r<0 ) { if( errno==EINTR ) continue; if( put ) *put = d; return WpFail( errno ); }
    d += DWORD(r);
  }
  if( put ) *put = d;
  return TRUE;
}

inline BOOL SetFilePointerEx( HANDLE h, LARGE_INTEGER to, LARGE_INTEGER* at, DWORD how ) {
  WpObj* o = WpFile( h );
  if( !o ) return FALSE;
  off_t r = lseek( o->fd, off_t(to.QuadPart), (how==FILE_BEGIN) ? SEEK_SET : (how==FILE_CURRENT) ? SEEK_CUR : SEEK_END );
  if( r<0 ) return WpFail( errno );
  if( at ) at->QuadPart = r;
  return TRUE;
}

inline BOOL SetEndOfFile( HANDLE h ) {
  WpObj* o = WpFile( h );
  if( !o ) return FALSE;
  off_t at = lseek( o->fd, 0, SEEK_CUR );
  if( (at<0) || ftruncate( o->fd, at ) ) return WpFail( errno );
  return TRUE;
}

inline BOOL GetFileSizeEx( HANDLE h, LARGE_INTEGER* sz ) {
  WpObj* o = WpFile( h );
  struct stat st;
  if( !o ) return FALSE;
  if( fstat( o->fd, &st ) ) return WpFail( errno );
  sz->QuadPart = st.st_size;
  return TRUE;
}

inline BOOL GetFileInformationByHandle( HANDLE h, BY_HANDLE_FILE_INFORMATION* i ) {
  WpObj* o = WpFile( h );
  struct stat st;
  if( !o ) return FALSE;
  if( fstat( o->fd, &st ) ) return WpFail( errno );
  memset( i, 0, sizeof(*i) );
  i->dwFileAttributes = WpAttrs( st );
  i->ftCreationTime = i->ftLastAccessTime = i->ftLastWriteTime = WpTime( st.st_mtime );
  ULONGLONG dev = ULONGLONG(st.st_dev), ino = ULONGLONG(st.st_ino), sz = ULONGLONG(st.st_size);
  i->dwVolumeSerialNumber = DWORD( dev ^ (dev>>32) );
  i->nFileIndexHigh = DWORD(ino>>32); i->nFileIndexLow = DWORD(ino);
  i->nFileSizeHigh = DWORD(sz>>32); i->nFileSizeLow = DWORD(sz);
  i->nNumberOfLinks = DWORD(st.st_nlink);
  return TRUE;
}

inline BOOL GetFileAttributesExW( const wchar_t* p, GET_FILEEX_INFO_LEVELS, void* out ) {
  struct stat st;
  if( stat( WpPath(p).c_str(), &st ) ) return WpFail( errno );
  WIN32_FILE_ATTRIBUTE_DATA* a = (WIN32_FILE_ATTRIBUTE_DATA*)out;
  ULONGLONG sz = S_ISDIR(st.st_mode) ? 0 : ULONGLONG(st.st_size);
  a->dwFileAttributes = WpAttrs( st );
  a->ftCreationTime = a->ftLastAccessTime = a->ftLastWriteTime = WpTime( st.st_mtime );
  a->nFileSizeHigh = DWORD(sz>>32); a->nFileSizeLow = DWORD(sz);
  return TRUE;
}

inline BOOL SetFileAttributesW( const wchar_t*, DWORD ) { return TRUE; }

inline BOOL CreateDirectoryW( const wchar_t* p, SECURITY_ATTRIBUTES* ) {
  if( mkdir( WpPath(p).c_str(), 0777 ) ) return WpFail( errno );
  return TRUE;
}

// a link to a directory goes, not what is in it (as a junction on Windows)
inline BOOL RemoveDirectoryW( const wchar_t* p ) {
  std::string s = WpPath( p );
  struct stat st;
  if( (lstat( s.c_str(), &st )==0) && S_ISLNK(st.st_mode) ) { if( unlink( s.c_str() ) ) return WpFail( errno ); return TRUE; }
  if( rmdir( s.c_str() ) ) return WpFail( errno );
  return TRUE;
}

inline BOOL DeleteFileW( const wchar_t* p ) {
  std::string s = WpPath( p );
  struct stat st;
  if( (lstat( s.c_str(), &st )==0) && S_ISDIR(st.st_mode) ) { SetLastError( ERROR_ACCESS_DENIED ); return FALSE; }
  if( unlink( s.c_str() ) ) return WpFail( errno );
  return TRUE;
}

inline BOOL WpCopy( const std::string& from, const std::string& to, int excl ) {
  int in = open( from.c_str(), O_RDONLY|O_CLOEXEC );
  struct stat st;
  if( in<0 ) return WpFail( errno );
  if( fstat( in, &st ) ) { int e = errno; close( in ); return WpFail( e ); }
  if( S_ISDIR(st.st_mode) ) { close( in ); return WpFail( EISDIR ); }
  int out = open( to.c_str(), O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC|(excl ? O_EXCL : 0), st.st_mode & 0777 );
  if( out<0 ) { int e = errno; close( in ); return WpFail( e ); }
  std::vector<char> b( 1<<20 );
  int e = 0;
  for(;;) {
    ssize_t n = read( in, &b[0], b.size() );
    if( n<0 ) { if( errno==EINTR ) continue; e = errno; break; }
    if( n==0 ) break;
    for( ssize_t d=0; d<n; ) {
      ssize_t w = write( out, &b[d], size_t(n-d) );
      if( w<0 ) { if( errno==EINTR ) continue; e = errno; break; }
      d += w;
    }
    if( e ) break;
  }
  close( in );
  if( close( out ) && !e ) e = errno;
  if( e ) { unlink( to.c_str() ); return WpFail( e ); }
  return TRUE;
}

inline BOOL CopyFileW( const wchar_t* from, const wchar_t* to, BOOL fail_if_there ) { return WpCopy( WpPath(from), WpPath(to), fail_if_there ); }

// rename(), but the target is replaced only if asked (MoveFileW: never), and
// across file systems a file is copied and removed (MoveFileW always does it)
inline BOOL MoveFileExW( const wchar_t* from, const wchar_t* to, DWORD flags ) {
  std::string f = WpPath( from ), t = WpPath( to );
  struct stat st;
  if( !(flags & MOVEFILE_REPLACE_EXISTING) && (lstat( t.c_str(), &st )==0) ) { SetLastError( ERROR_ALREADY_EXISTS ); return FALSE; }
  if( rename( f.c_str(), t.c_str() )==0 ) return TRUE;
  if( (errno!=EXDEV) || !(flags & MOVEFILE_COPY_ALLOWED) ) return WpFail( errno );
  if( !WpCopy( f, t, 0 ) ) return FALSE;
  if( unlink( f.c_str() ) ) return WpFail( errno );
  return TRUE;
}

inline BOOL MoveFileW( const wchar_t* from, const wchar_t* to ) { return MoveFileExW( from, to, MOVEFILE_COPY_ALLOWED ); }

inline BOOL CreateHardLinkW( const wchar_t* link, const wchar_t* target, SECURITY_ATTRIBUTES* ) {
  if( ::link( WpPath(target).c_str(), WpPath(link).c_str() ) ) return WpFail( errno );
  return TRUE;
}

inline BOOLEAN CreateSymbolicLinkW( const wchar_t* link, const wchar_t* target, DWORD ) {
  if( symlink( WpPath(target).c_str(), WpPath(link).c_str() ) ) return WpFail( errno );
  return TRUE;
}

// there are no 8.3 names
inline DWORD GetShortPathNameW( const wchar_t*, wchar_t*, DWORD ) { SetLastError( ERROR_NOT_SUPPORTED ); return 0; }

// the wide string into the buffer if it fits with its NUL (its length), else
// the size needed (NUL included)
inline DWORD WpPut( const std::wstring& s, wchar_t* buf, DWORD n ) {
  if( !buf || (n<=s.size()) ) return DWORD(s.size()+1);
  memcpy( buf, s.c_str(), (s.size()+1)*sizeof(wchar_t) );
  return DWORD(s.size());
}

// a relative path after the current directory; "./" and "//" dropped (".."
// is not folded: a link before it would go elsewhere)
inline DWORD GetFullPathNameW( const wchar_t* p, DWORD n, wchar_t* buf, wchar_t** file ) {
  if( !p || !*p ) { SetLastError( ERROR_INVALID_NAME ); return 0; }
  std::wstring s = p;
  if( s[0]!='/' ) {
    std::vector<char> cwd( 4096 );
    while( !getcwd( &cwd[0], cwd.size() ) ) { if( (errno!=ERANGE) || (cwd.size()>(1u<<20)) ) return WpFail( errno ); cwd.resize( cwd.size()*2 ); }
    s = WpWide( std::string( &cwd[0] ) ) + L"/" + s;
  }
  std::wstring r;
  for( size_t i=0; i<s.size(); ) {
    if( (s[i]=='/') && r.size() && (r[r.size()-1]=='/') ) { i++; continue; }
    if( (s[i]=='.') && r.size() && (r[r.size()-1]=='/') && ((i+1==s.size()) || (s[i+1]=='/')) ) { i += (i+1<s.size()) ? 2 : 1; continue; }
    r += s[i++];
  }
  if( (r.size()>1) && (r[r.size()-1]=='/') && (s[s.size()-1]!='/') ) r.erase( r.size()-1 ); // ("a/." is "a")
  DWORD k = WpPut( r, buf, n );
  if( file && (k==r.size()) ) { size_t q = r.rfind( '/' ); *file = (q+1<r.size()) ? buf+q+1 : 0; }
  return k;
}

inline DWORD GetTempPathW( DWORD n, wchar_t* buf ) {
  const char* t = getenv( "TMPDIR" );
  std::string s = (t && (t[0]=='/')) ? t : "/tmp";
  if( s[s.size()-1]!='/' ) s += '/';
  return WpPut( WpWide(s), buf, n );
}

inline DWORD GetModuleFileNameW( HMODULE, wchar_t* buf, DWORD n ) {
  std::vector<char> b( 4096 );
  ssize_t k;
  while( (k = readlink( "/proc/self/exe", &b[0], b.size() ))>=ssize_t(b.size()) ) b.resize( b.size()*2 );
  if( k<0 ) return WpFail( errno );
  std::wstring w = WpWide( &b[0], size_t(k) );
  if( n==0 ) { SetLastError( ERROR_INSUFFICIENT_BUFFER ); return 0; }
  DWORD m = DWORD( std::min<size_t>( w.size(), n-1 ) );
  memcpy( buf, w.data(), m*sizeof(wchar_t) ); buf[m] = 0;
  if( m<w.size() ) { SetLastError( ERROR_INSUFFICIENT_BUFFER ); return n; }
  return m;
}

inline BOOL GetDiskFreeSpaceExW( const wchar_t* dir, ULARGE_INTEGER* avail, ULARGE_INTEGER* total, ULARGE_INTEGER* all_free ) {
  struct statvfs v;
  if( statvfs( WpPath(dir).c_str(), &v ) ) return WpFail( errno );
  if( avail ) avail->QuadPart = ULONGLONG(v.f_bavail)*v.f_frsize;
  if( total ) total->QuadPart = ULONGLONG(v.f_blocks)*v.f_frsize;
  if( all_free ) all_free->QuadPart = ULONGLONG(v.f_bfree)*v.f_frsize;
  return TRUE;
}

//---- directory searches: "dir/pattern", '*' and '?' (case does not matter, as on Windows)

inline BOOL WpNext( WpObj* o, WIN32_FIND_DATAW* fd ) {
  while( struct dirent* e = readdir( o->dir ) ) {
    if( fnmatch( o->pat.c_str(), e->d_name, FNM_NOESCAPE|FNM_CASEFOLD ) ) continue;
    std::string p = o->dpath + "/" + e->d_name;
    struct stat st, ls;
    if( lstat( p.c_str(), &ls ) ) continue; // (gone)
    int link = S_ISLNK(ls.st_mode);
    if( link && stat( p.c_str(), &st ) ) st = ls; // (a link to nothing)
    else if( !link ) st = ls;
    std::wstring n = WpWide( std::string( e->d_name ) );
    if( n.size()>=MAX_PATH ) continue;
    memset( fd, 0, sizeof(*fd) );
    fd->dwFileAttributes = WpAttrs( st ) | (link ? FILE_ATTRIBUTE_REPARSE_POINT : 0);
    fd->ftCreationTime = fd->ftLastAccessTime = fd->ftLastWriteTime = WpTime( st.st_mtime );
    ULONGLONG sz = (S_ISDIR(st.st_mode) || link) ? 0 : ULONGLONG(st.st_size);
    fd->nFileSizeHigh = DWORD(sz>>32); fd->nFileSizeLow = DWORD(sz);
    memcpy( fd->cFileName, n.c_str(), (n.size()+1)*sizeof(wchar_t) );
    return TRUE;
  }
  SetLastError( ERROR_NO_MORE_FILES );
  return FALSE;
}

inline HANDLE FindFirstFileW( const wchar_t* spec, WIN32_FIND_DATAW* fd ) {
  std::string s = WpPath( spec );
  size_t q = s.rfind( '/' );
  WpObj* o = new WpObj( WP_FIND );
  o->dpath = (q==std::string::npos) ? "." : (q==0) ? "/" : s.substr( 0, q );
  o->pat = (q==std::string::npos) ? s : s.substr( q+1 );
  if( o->pat.find( '[' )!=std::string::npos ) { // (only * and ? are wildcards)
    std::string p;
    for( size_t i=0; i<o->pat.size(); i++ ) { if( o->pat[i]=='[' ) p += "[[]"; else p += o->pat[i]; }
    o->pat = p;
  }
  o->dir = opendir( o->dpath.c_str() );
  if( !o->dir ) { DWORD e = (errno==ENOENT) ? ERROR_PATH_NOT_FOUND : WpError(errno); delete o; SetLastError( e ); return INVALID_HANDLE_VALUE; }
  if( !WpNext( o, fd ) ) { closedir( o->dir ); o->dir = 0; delete o; SetLastError( ERROR_FILE_NOT_FOUND ); return INVALID_HANDLE_VALUE; }
  return o;
}

inline BOOL FindNextFileW( HANDLE h, WIN32_FIND_DATAW* fd ) {
  WpObj* o = (WpObj*)h;
  if( !o || (h==INVALID_HANDLE_VALUE) || (o->kind!=WP_FIND) ) { SetLastError( ERROR_INVALID_HANDLE ); return FALSE; }
  return WpNext( o, fd );
}

inline BOOL FindClose( HANDLE h ) { return CloseHandle( h ); }

//---- memory

// read/write blocks only; the size goes in a page before the block
inline void* VirtualAlloc( void*, size_t n, DWORD, DWORD ) {
  size_t pg = size_t( sysconf( _SC_PAGESIZE ) );
  void* m = mmap( 0, n+pg, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0 );
  if( m==MAP_FAILED ) { SetLastError( ERROR_NOT_ENOUGH_MEMORY ); return 0; }
  *(size_t*)m = n+pg;
  return (char*)m+pg;
}

inline BOOL VirtualFree( void* b, size_t, DWORD ) {
  size_t pg = size_t( sysconf( _SC_PAGESIZE ) );
  char* m = (char*)b-pg;
  if( munmap( m, *(size_t*)m ) ) return WpFail( errno );
  return TRUE;
}

//---- time

inline ULONGLONG GetTickCount64() {
  struct timespec t;
  clock_gettime( CLOCK_MONOTONIC, &t );
  return ULONGLONG(t.tv_sec)*1000u + ULONGLONG(t.tv_nsec)/1000000u;
}

inline void Sleep( DWORD ms ) {
  struct timespec t = { time_t(ms/1000), long(ms%1000)*1000000L };
  while( nanosleep( &t, &t ) && (errno==EINTR) ) {}
}

//---- the CPU

// the logical processors this process may run on
inline DWORD WpGetActiveProcessorCount( WORD ) {
  cpu_set_t s;
  if( sched_getaffinity( 0, sizeof(s), &s )==0 ) { int n = CPU_COUNT( &s ); if( n>0 ) return DWORD(n); }
  long n = sysconf( _SC_NPROCESSORS_ONLN );
  return (n>0) ? DWORD(n) : 1;
}

inline long WpReadNum( const std::string& path ) {
  long v = -1;
  if( FILE* f = fopen( path.c_str(), "r" ) ) { if( fscanf( f, "%ld", &v )!=1 ) v = -1; fclose( f ); }
  return v;
}

// RelationProcessorCore only: a record (48 bytes, as Windows makes them for
// one processor group) per core of the processors we may run on, from sysfs
inline BOOL WpGetLogicalProcessorInformationEx( int rel, void* buf, DWORD* len ) {
  cpu_set_t s;
  if( (rel!=0) || sched_getaffinity( 0, sizeof(s), &s ) ) { SetLastError( ERROR_NOT_SUPPORTED ); return FALSE; }
  std::vector< std::pair<long,long> > core; // (package, core id) of each
  std::vector<ULONGLONG> mask;
  for( int c=0; c<CPU_SETSIZE; c++ ) if( CPU_ISSET( c, &s ) ) {
    std::string t = "/sys/devices/system/cpu/cpu" + std::to_string(c) + "/topology/";
    long pk = WpReadNum( t+"physical_package_id" ), id = WpReadNum( t+"core_id" );
    if( id<0 ) { SetLastError( ERROR_NOT_SUPPORTED ); return FALSE; }
    size_t k = 0;
    while( (k<core.size()) && (core[k]!=std::make_pair( pk, id )) ) k++;
    if( k==core.size() ) { core.push_back( std::make_pair( pk, id ) ); mask.push_back( 0 ); }
    if( c<64 ) mask[k] |= 1ull<<c;
  }
  const DWORD R = 48;
  DWORD need = DWORD( core.size() )*R;
  if( !buf || (*len<need) ) { *len = need; SetLastError( ERROR_INSUFFICIENT_BUFFER ); return FALSE; }
  memset( buf, 0, need );
  for( size_t k=0; k<core.size(); k++ ) {
    BYTE* r = (BYTE*)buf+k*R;
    DWORD z = 0;
    memcpy( r, &z, 4 ); memcpy( r+4, &R, 4 );     // Relationship, Size
    r[8] = (mask[k]&(mask[k]-1)) ? 1 : 0;         // Flags: LTP_PC_SMT
    WORD one = 1; memcpy( r+8+22, &one, 2 );      // GroupCount
    memcpy( r+8+24, &mask[k], 8 );                // GroupMask[0].Mask (group 0)
  }
  *len = need;
  return TRUE;
}

inline HMODULE GetModuleHandleW( const wchar_t* name ) {
  static int kernel32;
  if( name && (lstrcmpiW( name, L"kernel32.dll" )==0) ) return &kernel32;
  SetLastError( ERROR_FILE_NOT_FOUND );
  return 0;
}

inline FARPROC GetProcAddress( HMODULE m, const char* name ) {
  if( m && !strcmp( name, "GetActiveProcessorCount" ) ) return (FARPROC)&WpGetActiveProcessorCount;
  if( m && !strcmp( name, "GetLogicalProcessorInformationEx" ) ) return (FARPROC)&WpGetLogicalProcessorInformationEx;
  SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
  return 0;
}

inline void GetSystemInfo( SYSTEM_INFO* si ) {
  memset( si, 0, sizeof(*si) );
  long n = sysconf( _SC_NPROCESSORS_ONLN );
  si->wProcessorArchitecture = 9; // (x64)
  si->dwPageSize = DWORD( sysconf( _SC_PAGESIZE ) );
  si->dwNumberOfProcessors = (n>0) ? DWORD(n) : 1;
  si->dwAllocationGranularity = 65536;
}

// the times of all CPUs together from /proc/stat, in 100ns; the kernel time
// includes the idle time (as on Windows)
inline BOOL GetSystemTimes( FILETIME* idle, FILETIME* kernel, FILETIME* user ) {
  unsigned long long v[8] = { 0 };
  FILE* f = fopen( "/proc/stat", "r" );
  if( !f ) return WpFail( errno );
  int n = fscanf( f, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", v, v+1, v+2, v+3, v+4, v+5, v+6, v+7 );
  fclose( f );
  if( n<4 ) { SetLastError( ERROR_NOT_SUPPORTED ); return FALSE; }
  ULONGLONG hz = ULONGLONG( sysconf( _SC_CLK_TCK ) ), u = 10000000u/(hz ? hz : 100);
  ULONGLONG i = (v[3]+v[4])*u, k = (v[2]+v[5]+v[6]+v[7])*u + i, s = (v[0]+v[1])*u;
  FILETIME* t[3] = { idle, kernel, user };
  ULONGLONG x[3] = { i, k, s };
  for( int j=0; j<3; j++ ) if( t[j] ) { t[j]->dwLowDateTime = DWORD(x[j]); t[j]->dwHighDateTime = DWORD(x[j]>>32); }
  return TRUE;
}

inline UINT SetErrorMode( UINT ) { return 0; }

//---- the console

inline BOOL GetConsoleMode( HANDLE h, DWORD* m ) {
  WpObj* o = WpFile( h );
  if( !o || !isatty( o->fd ) ) { SetLastError( ERROR_INVALID_HANDLE ); return FALSE; }
  if( m ) *m = 3;
  return TRUE;
}

inline BOOL GetConsoleScreenBufferInfo( HANDLE h, CONSOLE_SCREEN_BUFFER_INFO* ci ) {
  WpObj* o = WpFile( h );
  struct winsize ws;
  if( !o || ioctl( o->fd, TIOCGWINSZ, &ws ) || !ws.ws_col ) { SetLastError( ERROR_INVALID_HANDLE ); return FALSE; }
  memset( ci, 0, sizeof(*ci) );
  ci->dwSize.X = short( std::min<int>( ws.ws_col, 32767 ) ); ci->dwSize.Y = short( std::min<int>( ws.ws_row, 32767 ) );
  ci->srWindow.Right = short( ci->dwSize.X-1 ); ci->srWindow.Bottom = short( ci->dwSize.Y-1 );
  return TRUE;
}

inline BOOL WriteConsoleW( HANDLE h, const wchar_t* w, DWORD n, DWORD* put, void* ) {
  std::string s = WpUtf8( w, n );
  DWORD k = 0;
  BOOL r = WriteFile( h, s.data(), DWORD(s.size()), &k, 0 );
  if( put ) *put = r ? n : 0;
  return r;
}

//---- events, threads, waits

inline HANDLE CreateEventW( SECURITY_ATTRIBUTES*, BOOL manual, BOOL set, const wchar_t* ) {
  WpObj* o = new WpObj( WP_EVENT );
  o->manual = manual; o->done = set;
  return o;
}

inline BOOL SetEvent( HANDLE h ) {
  WpObj* o = (WpObj*)h;
  if( !o || (o->kind!=WP_EVENT) ) { SetLastError( ERROR_INVALID_HANDLE ); return FALSE; }
  pthread_mutex_lock( WpMutex() );
  o->done = 1;
  pthread_cond_broadcast( WpCond() );
  pthread_mutex_unlock( WpMutex() );
  return TRUE;
}

inline BOOL ResetEvent( HANDLE h ) {
  WpObj* o = (WpObj*)h;
  if( !o || (o->kind!=WP_EVENT) ) { SetLastError( ERROR_INVALID_HANDLE ); return FALSE; }
  pthread_mutex_lock( WpMutex() );
  o->done = 0;
  pthread_mutex_unlock( WpMutex() );
  return TRUE;
}

struct WpStart { LPTHREAD_START_ROUTINE fn; void* arg; WpObj* o; };

inline void* WpThread( void* a ) {
  WpStart s = *(WpStart*)a;
  delete (WpStart*)a;
  DWORD r = s.fn( s.arg );
  pthread_mutex_lock( WpMutex() );
  s.o->done = 1; s.o->code = r;
  pthread_cond_broadcast( WpCond() );
  WpRelease( s.o );
  pthread_mutex_unlock( WpMutex() );
  return 0;
}

inline HANDLE CreateThread( SECURITY_ATTRIBUTES*, size_t, LPTHREAD_START_ROUTINE fn, void* arg, DWORD, DWORD* id ) {
  WpObj* o = new WpObj( WP_THREAD );
  o->refs = 2; // the handle's and the thread's
  WpStart* s = new WpStart;
  s->fn = fn; s->arg = arg; s->o = o;
  pthread_t t;
  pthread_attr_t a;
  pthread_attr_init( &a );
  pthread_attr_setdetachstate( &a, PTHREAD_CREATE_DETACHED );
  int e = pthread_create( &t, &a, WpThread, s );
  pthread_attr_destroy( &a );
  if( e ) { delete s; delete o; WpFail( e ); return 0; }
  if( id ) *id = 0;
  return o;
}

inline int WpSignaled( WpObj* o ) {
  if( (o->kind==WP_PROCESS) || (o->kind==WP_THREAD) || (o->kind==WP_EVENT) ) return o->done;
  return 1;
}

// any one of them (the first signaled); bWaitAll is not supported
inline DWORD WaitForMultipleObjects( DWORD n, const HANDLE* h, BOOL, DWORD ms ) {
  struct timespec dl;
  clock_gettime( CLOCK_MONOTONIC, &dl );
  dl.tv_sec += ms/1000; dl.tv_nsec += long(ms%1000)*1000000L;
  if( dl.tv_nsec>=1000000000L ) { dl.tv_sec++; dl.tv_nsec -= 1000000000L; }
  pthread_mutex_lock( WpMutex() );
  for(;;) {
    for( DWORD i=0; i<n; i++ ) {
      WpObj* o = (WpObj*)h[i];
      if( !o || (h[i]==INVALID_HANDLE_VALUE) ) { pthread_mutex_unlock( WpMutex() ); SetLastError( ERROR_INVALID_HANDLE ); return WAIT_FAILED; }
      if( WpSignaled( o ) ) {
        if( (o->kind==WP_EVENT) && !o->manual ) o->done = 0;
        pthread_mutex_unlock( WpMutex() );
        return WAIT_OBJECT_0+i;
      }
    }
    if( ms==0 ) break;
    int e = (ms==INFINITE) ? pthread_cond_wait( WpCond(), WpMutex() ) : pthread_cond_timedwait( WpCond(), WpMutex(), &dl );
    if( e==ETIMEDOUT ) ms = 0; // (one more look)
  }
  pthread_mutex_unlock( WpMutex() );
  return WAIT_TIMEOUT;
}

inline DWORD WaitForSingleObject( HANDLE h, DWORD ms ) { return WaitForMultipleObjects( 1, &h, FALSE, ms ); }

//---- processes

// a command line into arguments, as CommandLineToArgvW and the MS C runtime
// split it: argv[0] to the first blank, or between the first two quotes;
// after it 2n backslashes and a quote are n and a quote that starts or ends
// quoting, 2n+1 and a quote n and a quote; "" in quotes is a quote
inline std::vector<std::wstring> WpSplit( const wchar_t* p ) {
  std::vector<std::wstring> a;
  std::wstring s;
  if( *p=='"' ) { for( p++; *p && (*p!='"'); p++ ) s += *p; if( *p ) p++; }
  else for(; *p && (*p!=' ') && (*p!='\t'); p++ ) s += *p;
  a.push_back( s );
  for(;;) {
    while( (*p==' ') || (*p=='\t') ) p++;
    if( !*p ) break;
    s.clear();
    int q = 0;
    while( *p && (q || ((*p!=' ') && (*p!='\t'))) ) {
      if( *p=='\\' ) {
        size_t b = 0;
        while( *p=='\\' ) { b++; p++; }
        if( *p=='"' ) { s.append( b/2, '\\' ); if( b&1 ) s += *p++; }
        else s.append( b, '\\' );
      } else if( *p=='"' ) {
        if( q && (p[1]=='"') ) { s += '"'; p += 2; }
        else { q = !q; p++; }
      } else s += *p++;
    }
    a.push_back( s );
  }
  return a;
}

// waits for the process to end, without reaping it first, so that the pid is
// not reused while TerminateProcess may still kill it
inline void* WpReaper( void* a ) {
  WpObj* o = (WpObj*)a;
  siginfo_t si;
  while( waitid( P_PID, id_t(o->pid), &si, WEXITED|WNOWAIT ) && (errno==EINTR) ) {}
  int st = 0;
  pthread_mutex_lock( WpMutex() );
  while( (waitpid( o->pid, &st, 0 )<0) && (errno==EINTR) ) {}
  if( WIFEXITED(st) ) o->code = DWORD( WEXITSTATUS(st) );
  else if( WIFSIGNALED(st) ) {
    int sig = WTERMSIG(st);
    o->code = (o->killed && (sig==SIGKILL)) ? o->kcode : (sig==SIGINT) ? STATUS_CONTROL_C_EXIT : DWORD(128+sig);
  } else o->code = 1;
  o->done = 1;
  pthread_cond_broadcast( WpCond() );
  WpRelease( o );
  pthread_mutex_unlock( WpMutex() );
  return 0;
}

// app: the program; cmd: its command line (split for its argv); with
// STARTF_USESTDHANDLES the three handles are its stdin, stdout and stderr
// (else ours); it does not wait for ResumeThread
inline BOOL CreateProcessW( const wchar_t* app, wchar_t* cmd, SECURITY_ATTRIBUTES*, SECURITY_ATTRIBUTES*, BOOL, DWORD, void*,
                            const wchar_t* cwd, STARTUPINFOW* si, PROCESS_INFORMATION* pi ) {
  std::vector<std::wstring> wa = WpSplit( cmd ? cmd : L"" );
  std::vector<std::string> args;
  for( size_t i=0; i<wa.size(); i++ ) args.push_back( WpUtf8( wa[i].c_str(), wa[i].size() ) );
  std::string exe = app ? WpPath( app ) : args[0], dir = cwd ? WpPath( cwd ) : std::string();
  std::vector<char*> argv;
  for( size_t i=0; i<args.size(); i++ ) argv.push_back( &args[i][0] );
  argv.push_back( 0 );
  int fds[3] = { 0, 1, 2 };
  if( si && (si->dwFlags & STARTF_USESTDHANDLES) ) {
    HANDLE hs[3] = { si->hStdInput, si->hStdOutput, si->hStdError };
    for( int i=0; i<3; i++ ) { WpObj* o = (WpObj*)hs[i]; fds[i] = (o && (hs[i]!=INVALID_HANDLE_VALUE) && (o->kind==WP_FILE)) ? o->fd : -1; }
  }
  int ep[2]; // the child's errno, if it does not get to exec
  if( pipe2( ep, O_CLOEXEC ) ) return WpFail( errno );
  sigset_t none;
  sigemptyset( &none );
  pid_t parent = getpid();
  pid_t pid = fork();
  if( pid==0 ) { // only async-signal-safe calls from here
    int a[3], e = 0;
    for( int i=0; (i<3) && !e; i++ ) { // (all above 2 first, then on 0, 1, 2)
      int t = (fds[i]>=0) ? fds[i] : open( "/dev/null", (i ? O_WRONLY : O_RDONLY)|O_CLOEXEC );
      a[i] = (t<0) ? -1 : fcntl( t, F_DUPFD_CLOEXEC, 3 );
      if( a[i]<0 ) e = errno;
      if( (fds[i]<0) && (t>=0) ) close( t );
    }
    for( int i=0; (i<3) && !e; i++ ) if( dup2( a[i], i )<0 ) e = errno;
    if( !e && (prctl( PR_SET_PDEATHSIG, SIGKILL ) || (getppid()!=parent)) ) e = ECHILD;
    if( !e ) sigprocmask( SIG_SETMASK, &none, 0 );
    if( !e && dir.size() && chdir( dir.c_str() ) ) e = errno;
    if( !e ) { execv( exe.c_str(), &argv[0] ); e = errno; }
    while( (write( ep[1], &e, sizeof(e) )<0) && (errno==EINTR) ) {}
    _exit( 127 );
  }
  close( ep[1] );
  if( pid<0 ) { int e = errno; close( ep[0] ); return WpFail( e ); }
  int e = 0;
  ssize_t r;
  while( ((r = read( ep[0], &e, sizeof(e) ))<0) && (errno==EINTR) ) {}
  close( ep[0] );
  if( r==ssize_t(sizeof(e)) ) { while( (waitpid( pid, 0, 0 )<0) && (errno==EINTR) ) {} return WpFail( e ); }
  WpObj* o = new WpObj( WP_PROCESS );
  o->pid = pid; o->refs = 2; // the handle's and the reaper's
  pthread_t t;
  pthread_attr_t at;
  pthread_attr_init( &at );
  pthread_attr_setdetachstate( &at, PTHREAD_CREATE_DETACHED );
  if( pthread_create( &t, &at, WpReaper, o ) ) WpReaper( o ); // (could not wait for it apart: now)
  pthread_attr_destroy( &at );
  pi->hProcess = o;
  pi->hThread = new WpObj( WP_OTHER );
  pi->dwProcessId = DWORD(pid); pi->dwThreadId = 0;
  return TRUE;
}

inline DWORD ResumeThread( HANDLE ) { return 0; }

inline BOOL GetExitCodeProcess( HANDLE h, DWORD* code ) {
  WpObj* o = (WpObj*)h;
  if( !o || (o->kind!=WP_PROCESS) ) { SetLastError( ERROR_INVALID_HANDLE ); return FALSE; }
  pthread_mutex_lock( WpMutex() );
  *code = o->done ? o->code : STILL_ACTIVE;
  pthread_mutex_unlock( WpMutex() );
  return TRUE;
}

inline BOOL TerminateProcess( HANDLE h, DWORD code ) {
  if( h==GetCurrentProcess() ) _exit( int(code) );
  WpObj* o = (WpObj*)h;
  if( !o || (o->kind!=WP_PROCESS) ) { SetLastError( ERROR_INVALID_HANDLE ); return FALSE; }
  pthread_mutex_lock( WpMutex() );
  if( !o->done && !o->killed ) { o->killed = 1; o->kcode = code; kill( o->pid, SIGKILL ); }
  pthread_mutex_unlock( WpMutex() );
  return TRUE;
}

// job objects: the children die with us anyway (PR_SET_PDEATHSIG)
inline HANDLE CreateJobObjectW( SECURITY_ATTRIBUTES*, const wchar_t* ) { return new WpObj( WP_OTHER ); }
inline BOOL SetInformationJobObject( HANDLE, JOBOBJECTINFOCLASS, void*, DWORD ) { return TRUE; }
inline BOOL AssignProcessToJobObject( HANDLE, HANDLE ) { return TRUE; }

//---- Ctrl+C and the like: signals

inline PHANDLER_ROUTINE& WpHandler() { static PHANDLER_ROUTINE h; return h; }

inline sigset_t WpSignals() {
  sigset_t s;
  sigemptyset( &s );
  sigaddset( &s, SIGINT ); sigaddset( &s, SIGQUIT ); sigaddset( &s, SIGHUP ); sigaddset( &s, SIGTERM );
  return s;
}

// the routine gets them; without one, or when it returns FALSE, they do what
// they do by default
inline void* WpSignalThread( void* ) {
  sigset_t s = WpSignals();
  for(;;) {
    int sig;
    if( sigwait( &s, &sig ) ) continue;
    PHANDLER_ROUTINE f;
    pthread_mutex_lock( WpMutex() );
    f = WpHandler();
    pthread_mutex_unlock( WpMutex() );
    DWORD t = (sig==SIGINT) ? CTRL_C_EVENT : (sig==SIGQUIT) ? CTRL_BREAK_EVENT : (sig==SIGHUP) ? CTRL_CLOSE_EVENT : CTRL_SHUTDOWN_EVENT;
    if( f && f( t ) ) continue;
    signal( sig, SIG_DFL );
    sigset_t one;
    sigemptyset( &one ); sigaddset( &one, sig );
    pthread_sigmask( SIG_UNBLOCK, &one, 0 );
    raise( sig );
  }
  return 0;
}

// one routine (the last one set; remove: none)
inline BOOL SetConsoleCtrlHandler( PHANDLER_ROUTINE f, BOOL add ) {
  pthread_mutex_lock( WpMutex() );
  WpHandler() = add ? f : 0;
  pthread_mutex_unlock( WpMutex() );
  return TRUE;
}

// before any other thread: the signals blocked, for the thread that takes them
inline void WinapiPosixInit() {
  signal( SIGPIPE, SIG_IGN );
  sigset_t s = WpSignals();
  pthread_sigmask( SIG_BLOCK, &s, 0 );
  pthread_t t;
  if( pthread_create( &t, 0, WpSignalThread, 0 )==0 ) pthread_detach( t );
  WpCond();
}

#ifndef WINAPI_POSIX_NO_MAIN
int wmain( int argc, wchar_t** argv );

int main( int argc, char** argv ) {
  WinapiPosixInit();
  std::vector<std::wstring> a;
  for( int i=0; i<argc; i++ ) a.push_back( WpWide( std::string( argv[i] ) ) );
  std::vector<wchar_t*> w;
  for( int i=0; i<argc; i++ ) w.push_back( &a[i][0] );
  w.push_back( 0 );
  return wmain( argc, &w[0] );
}
#endif

#endif
