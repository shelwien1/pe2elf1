//  filehandle, both ways round.
//
//  Lib3/file_api.inc picks one of two implementations of the same struct:
//  file_api_std.inc over stdio, and file_api_win.inc over CreateFile and
//  ReadFile.  Code written against one and compiled against the other is
//  the bug this file exists to catch, and it has caught two: error(), which
//  the WinAPI side did not have at all, so oggcomp reached past it to
//  ferror(filehandle::f) and would not compile there; and close(), which
//  returned zero for success through stdio and NONZERO for success through
//  CloseHandle, so `if( h.close() ) failed();` -- how oggcomp reads it --
//  called every successful close a write error.
//
//  Neither shows up on Linux, where only one of the two ever compiles.
//
//      c++ -I.. -o t file_api_test.cpp && ./t                    # stdio
//      x86_64-w64-mingw32-g++ -I.. -static -o t.exe file_api_test.cpp
//      wine t.exe                                                # WinAPI
//
//  and on Windows, cl or clang++ with -DFILE_API_WIN for the second.  The
//  two runs must print the same ten lines.

#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t byte;
typedef uint32_t uint;
typedef uint64_t qword;
#ifdef _WIN32
#include <windows.h>
#else
#define FILE_API_STD 1
#endif
#include "Lib3/file_api.inc"

static int fails = 0;

static void check(const char *what, int got, int want) {
  printf("  %-46s %s (got %d, wanted %d)\n", what, got == want ? "ok" : "FAIL", got, want);
  if(got != want)
    fails++;
}

int main(void) {
  const char *a = "file_api_test.1.tmp", *b = "file_api_test.2.tmp";
  byte buf[4096];
  memset(buf, 0x5A, sizeof buf);

  {  //  A clean write of a whole file leaves nothing to report.
    filehandle g;
    g.make(a);
    check("writ of a full buffer returns its length", (int)g.writ(buf, sizeof buf), (int)sizeof buf);
    check("error() after a clean write", g.error(), 0);
    check("close() returns 0 on success", g.close(), 0);
  }
  {  //  Reading past the end is a short read, and a short read at the end of
     //  a file is not an error.  Get this wrong on the WinAPI side -- where
     //  a short transfer is the only hint there is that something failed --
     //  and every successful run ends in "read error".
    filehandle f;
    f.open(a);
    check("read of the whole file", (int)f.read(buf, sizeof buf), (int)sizeof buf);
    check("error() after reading it all", f.error(), 0);
    check("read at end of file returns 0", (int)f.read(buf, sizeof buf), 0);
    check("error() after a short read at EOF", f.error(), 0);
    check("close() returns 0 on success", f.close(), 0);
  }
  {  //  And a failure that is real has to be reported.
    filehandle f;
    f.open(a);
    f.writ(buf, sizeof buf);
    check("error() after writing to a read-only handle", f.error() != 0, 1);
    f.close();
  }
  {
    filehandle g;
    g.make(b);
    g.writ(buf, 16);
    g.seek(0);
    g.read(buf, sizeof buf);
    check("error() after reading a write-only handle", g.error() != 0, 1);
    g.close();
  }

  {  //  "-" is a standard stream on both backends, and close() must leave it
     //  standing: it belongs to whoever started the process, and taking
     //  stdout away would silence everything that runs after -- including
     //  the CRT's own flush on the way out.  Done last because opening a
     //  standard stream puts it into binary mode, after which the lines
     //  below end in LF rather than the CRLF a Windows run has been using.
    filehandle g;
    check("make(\"-\") gives a usable handle", g.make("-") != 0, 1);
    check("close() of a borrowed stream returns 0", g.close(), 0);
    check("stdout is still there afterwards", fflush(stdout) == 0, 1);
  }

  remove(a);
  remove(b);
  printf("%s\n", fails ? "file_api_test: FAILURES" : "file_api_test: all ok");
  return fails ? 1 : 0;
}
