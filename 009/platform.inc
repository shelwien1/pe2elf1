// platform.inc -- the two things the codec needs that Windows does not have.
//
// The codec's two in-memory entry points hand `compress_image` and
// `expand_image` a `FILE*` over a buffer instead of over a path, which is what
// lets the file layer stay exactly as it is and the codec gain an entry point
// rather than a fork.  On a POSIX host that is `open_memstream` and `fmemopen`,
// one line each.  Windows has neither -- not in MSVC's CRT and not in
// mingw-w64, which is msvcrt underneath -- and they are the *only* two things
// in the program that a Windows compiler rejects.  Measured, not assumed:
// `x86_64-w64-mingw32-g++ -c bmf.cpp` reports exactly two errors and they are
// these two names.
//
// **Named `bmf_*` rather than defined as the POSIX names.**  Two reasons, and
// the second is the real one:
//
//   * a `#define open_memstream ...` would claim to be the POSIX function and
//     is not -- what is below supports the one shape this program uses and not,
//     say, a stream reopened or seeked past its end;
//   * `open_memstream` cannot be shimmed under its own name at all.  Its
//     contract is that `*bufp` and `*lenp` become valid when the stream is
//     *closed*, so a faithful stand-in would have to intercept `fclose`, and
//     nothing portable can.  So the close is a named operation here, and the
//     POSIX side of it is a one-line `fclose` that ignores its out-parameters
//     because the real function has already written them.
//
// The read side needs no such thing: the buffer is input, the caller owns it,
// and plain `fclose` ends the stream on both platforms.
//
// **What the Windows half actually does.**  A temporary file, which is the only
// thing Windows offers that is a `FILE*` and is not a named file the user has
// to clean up.  Deliberately *not* `tmpfile()`: the CRT's version creates its
// file in the root of the current drive, so it fails for an unelevated process
// on the system drive -- a documented defect that has outlived several CRTs.
// `GetTempFileNameA` puts it where the environment says temporary files go, and
// `FILE_FLAG_DELETE_ON_CLOSE` makes the deletion the kernel's job rather than
// something an abnormal exit can skip.
//
// So the Windows build touches the disk where the POSIX one does not.  That is
// a real difference and it is the price of the entry points existing at all
// there; it is not on any path `bmf c` or `bmf d` takes, because those open a
// path and never come through here.
#ifdef _WIN32

// **Two Windows implementations, and the header decides which.**  The good one
// wants `<windows.h>`, which is the Windows SDK and not the CRT: a toolchain
// can have a complete UCRT and no SDK headers at all, which is what a
// bring-your-own-compiler clang setup often looks like.  `__has_include` is
// standard C++17 and both clang and MSVC honour it, so the choice needs no
// configuration -- and `BMF_NO_WINDOWS_H` forces the fallback, which is how the
// second path is tested on a machine that has both.
//
// The difference is worth stating rather than hiding.  With the SDK the file is
// created by `GetTempFileNameA`, which is atomic and cannot collide, and it is
// deleted by the *kernel*.  Without it the name has to be built here and the
// deletion is the CRT's, which means an `abort()` can leave one behind where the
// SDK path would not.  Neither difference is visible to a caller; both are
// visible to somebody wondering why a temp directory has a file in it.
#if __has_include(<windows.h>)&&!defined(BMF_NO_WINDOWS_H)
#define BMF_WIN_SDK 1
#endif

#ifdef BMF_WIN_SDK

// `WIN32_LEAN_AND_MEAN` and `NOMINMAX` before the include, and three `#undef`s
// after it.  This header is included first, so anything it defines is in force
// over the whole program -- and `windows.h` defines macros named `min`, `max`,
// `near`, `far` and `small`, any of which would silently rewrite ordinary code
// in a tree that spells `min32`/`max32` deliberately.  The three below are the
// ones a modern SDK still defines through `WIN32_LEAN_AND_MEAN`; the list is
// short because it was measured by compiling, not copied from a wiki.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#undef near
#undef far
#undef small

// One temporary file as a `FILE*`, opened read/write and binary, deleted by the
// kernel when the last handle closes.
//
// The handle goes through `_open_osfhandle` and `_fdopen` so that what comes
// back is an ordinary CRT stream: `fread`, `fseek` and `fclose` on it are the
// same calls the POSIX side makes, and `fclose` is what drops the handle and
// takes the file with it.  After `_open_osfhandle` succeeds the descriptor owns
// the handle, which is why the failure path below closes the descriptor and not
// the handle -- closing both would be a double close.
static FILE* bmf_win_tempfile(void) {
  char dir[MAX_PATH+1], path[MAX_PATH+1];
  const uint32_t n = GetTempPathA((uint32_t)sizeof dir, dir);
  if( n==0||n>MAX_PATH )
    return nullptr;
  if( GetTempFileNameA(dir, "bmf", 0, path)==0 )
    return nullptr;
  // `CREATE_ALWAYS`: `GetTempFileNameA` has already created the file, so this
  // truncates the one it made rather than racing to make another.
  const HANDLE h = CreateFileA(path, GENERIC_READ|GENERIC_WRITE,
                               FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                               nullptr, CREATE_ALWAYS,
                               FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,
                               nullptr);
  if( h==INVALID_HANDLE_VALUE )
    return nullptr;
  const int32_t fd = _open_osfhandle((intptr_t)h, _O_BINARY|_O_RDWR);
  if( fd==-1 ) {
    CloseHandle(h);
    return nullptr;
  }
  FILE* fp = _fdopen(fd, "w+b");
  if( !fp )
    _close(fd);
  return fp;
}

#else   // _WIN32 without the SDK headers -- the CRT on its own

#include <process.h>

// The same thing with nothing but the CRT: a name built here, and `"D"` in the
// mode string, which is the MSVC extension that makes the CRT delete the file
// when the stream closes.  Confirmed by running it -- the file is gone after
// `fclose` -- rather than taken from the documentation.
//
// **The name has to be unique by construction**, because eight codecs in eight
// threads come through here and there is no atomic create to fall back on.
// `"x"` in the mode string would have given one, and this CRT ignores it: a
// second `fopen(path, "w+bxD")` on a path the first still holds *succeeds*,
// which was measured and is why the obvious retry loop is not what is here.
//
// So three things that cannot repeat: the process id, the address of a
// thread-local, and a counter inside that thread.  The middle one is the part
// that does the work -- a `thread_local` has a distinct address in every thread
// that touches it -- and it costs no header and no atomic, in a file whose
// point is to need as little as possible from the platform.
static thread_local uint32_t bmf_tmp_seq;

static FILE* bmf_win_tempfile(void) {
  const char* dir = getenv("TMP");
  if( !dir||!*dir )
    dir = getenv("TEMP");
  if( !dir||!*dir )
    dir = ".";
  char path[1024];
  const uintptr_t here = (uintptr_t)&bmf_tmp_seq;
  ++bmf_tmp_seq;
  // `%llx` against a `uint64_t`, which is exact rather than lucky: this arm is
  // inside `_WIN32`, and there `uint64_t` *is* `unsigned long long` for both
  // MSVC and mingw-w64.  Spelled with the named width because that is what this
  // tree spells and what `unify_types.py` enforces.
  const int32_t n = snprintf(path, sizeof path, "%s\\bmf%x_%llx_%x.tmp", dir,
                             (uint32_t)_getpid(), (uint64_t)here,
                             bmf_tmp_seq);
  if( n<=0||(size_t)n>=sizeof path )
    return nullptr;
  return fopen(path, "w+bD");
}

#endif  // BMF_WIN_SDK

static FILE* bmf_memopen_read(const void* data, size_t n) {
  FILE* fp = bmf_win_tempfile();
  if( !fp )
    return nullptr;
  if( n&&fwrite(data, 1, n, fp)!=n ) {
    fclose(fp);
    return nullptr;
  }
  rewind(fp);
  return fp;
}

static FILE* bmf_memstream_open(char** bufp, size_t* lenp) {
  *bufp = nullptr;
  *lenp = 0;
  return bmf_win_tempfile();
}

// The length is taken by seeking to the end and not from the write position,
// which is what `open_memstream` reports: its length is the furthest the stream
// has reached, not where it happens to be.  This program only appends, so the
// two agree today -- the seek is here so they keep agreeing if it stops.
//
// `_fseeki64`/`_ftelli64` because `long` is 32 bits on Windows even at 64, and
// the coded output of a large enough image would pass 2 GB and come back
// negative from `ftell`.
//
// The buffer gets a NUL past the reported length, as `open_memstream`'s does.
// Nothing here reads it; it is there so that this is not the difference that
// bites a future caller which does.
static int32_t bmf_memstream_close(FILE* fp, char** bufp, size_t* lenp) {
  *bufp = nullptr;
  *lenp = 0;
  if( fflush(fp)!=0||_fseeki64(fp, 0, SEEK_END)!=0 ) {
    fclose(fp);
    return EOF;
  }
  const int64_t end = _ftelli64(fp);
  if( end<0||(uint64_t)end>=(uint64_t)SIZE_MAX ) {
    fclose(fp);
    return EOF;
  }
  const size_t len = (size_t)end;
  rewind(fp);
  char* buf = (char*)malloc(len+1);
  if( !buf ) {
    fclose(fp);
    return EOF;
  }
  if( len&&fread(buf, 1, len, fp)!=len ) {
    free(buf);
    fclose(fp);
    return EOF;
  }
  buf[len] = '\0';
  *bufp = buf;
  *lenp = len;
  return fclose(fp);
}

#else

static FILE* bmf_memopen_read(const void* data, size_t n) {
  // The cast drops `const`, and the stream is opened `"rb"`, so nothing writes
  // through it.  `fmemopen` takes `void*` for the one signature it has.
  return fmemopen((void*)data, n, "rb");
}

static FILE* bmf_memstream_open(char** bufp, size_t* lenp) {
  return open_memstream(bufp, lenp);
}

// `open_memstream` has already written both out-parameters by the time the
// stream is closed, so this is `fclose` and the arguments are here to make the
// two platforms one signature.
static int32_t bmf_memstream_close(FILE* fp, char** bufp, size_t* lenp) {
  (void)bufp;
  (void)lenp;
  return fclose(fp);
}

#endif
