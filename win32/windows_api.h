/*
 * <windows.h> for the ROSE sources whose Windows code uses the Windows API without including it
 * (Visual C++ builds get it through rose_msvc.h): the Makefile adds it with -include to the
 * compilation of those sources only.
 */
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
