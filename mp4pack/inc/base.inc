// inc/base.inc -- the platform: <windows.h>, or winapi_posix.h on Linux; the
//  C++ headers, x86targets.h; the types
// (a part of the engine: inc/mp4pack.inc includes it, in its order)

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#define DS "\\"        // the directory separator, in literals: L"u" DS
#define EXE ".exe"     // the tools' extension
#else
#include "../winapi_posix.h"
#define DS "/"
#define EXE ""
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <algorithm>
#include "../x86targets.h"  // the CPU, and the -march targets whose code runs on it (cpuinfo's)

typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned long long qword;
using std::string;
using std::wstring;
using std::vector;

static const qword ALL = ~0ULL;

