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
