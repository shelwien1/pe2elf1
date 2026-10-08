/* Definitions linked into the binaries of the Linux release (librose.so and the programs, see
   RELOCATABLE in the Makefile), so that they run on systems older than the one they were built
   on: with the GNU C library 2.34 or later (they were built with 2.39), and with the libgcc_s of
   GCC 4.3 or later (they were built with GCC 13).  For the C library:

   - glibc 2.38's headers declare strtol, sscanf and the like (with _GNU_SOURCE, which g++
     defines) as the C23 functions __isoc23_strtol, __isoc23_sscanf, ... (GLIBC_2.38), which
     only differ in also accepting "0b" binary numbers: these are defined here with the older
     functions;
   - libstdc++ (std::random_device) uses arc4random (GLIBC_2.36): defined here with getrandom
     (GLIBC_2.25).
   For libgcc_s: libstdc++ (to_chars and from_chars for _Float16) uses the conversions of
   _Float16 of GCC 12 (GCC_12.0.0), which are defined here.

   This file is compiled as C11 without _GNU_SOURCE, so that the names below are the older
   functions.  The definitions are hidden: each binary uses its own. */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/random.h>
#include <unistd.h>
#include <wchar.h>

#define HIDDEN __attribute__((visibility("hidden")))

HIDDEN long __isoc23_strtol(const char* s, char** end, int base) { return strtol(s, end, base); }
HIDDEN long long __isoc23_strtoll(const char* s, char** end, int base) { return strtoll(s, end, base); }
HIDDEN unsigned long __isoc23_strtoul(const char* s, char** end, int base) { return strtoul(s, end, base); }
HIDDEN unsigned long long __isoc23_strtoull(const char* s, char** end, int base) {
  return strtoull(s, end, base);
}
HIDDEN long __isoc23_wcstol(const wchar_t* s, wchar_t** end, int base) { return wcstol(s, end, base); }
HIDDEN long long __isoc23_wcstoll(const wchar_t* s, wchar_t** end, int base) { return wcstoll(s, end, base); }
HIDDEN unsigned long __isoc23_wcstoul(const wchar_t* s, wchar_t** end, int base) {
  return wcstoul(s, end, base);
}
HIDDEN unsigned long long __isoc23_wcstoull(const wchar_t* s, wchar_t** end, int base) {
  return wcstoull(s, end, base);
}

HIDDEN int __isoc23_vsscanf(const char* s, const char* format, va_list ap) { return vsscanf(s, format, ap); }
HIDDEN int __isoc23_vfscanf(FILE* f, const char* format, va_list ap) { return vfscanf(f, format, ap); }
HIDDEN int __isoc23_vscanf(const char* format, va_list ap) { return vscanf(format, ap); }
HIDDEN int __isoc23_sscanf(const char* s, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  int n = vsscanf(s, format, ap);
  va_end(ap);
  return n;
}
HIDDEN int __isoc23_fscanf(FILE* f, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  int n = vfscanf(f, format, ap);
  va_end(ap);
  return n;
}
HIDDEN int __isoc23_scanf(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  int n = vscanf(format, ap);
  va_end(ap);
  return n;
}
HIDDEN int __isoc23_vswscanf(const wchar_t* s, const wchar_t* format, va_list ap) { return vswscanf(s, format, ap); }
HIDDEN int __isoc23_swscanf(const wchar_t* s, const wchar_t* format, ...) {
  va_list ap;
  va_start(ap, format);
  int n = vswscanf(s, format, ap);
  va_end(ap);
  return n;
}

HIDDEN void arc4random_buf(void* buffer, size_t size) {
  unsigned char* p = (unsigned char*)buffer;
  int fd = -1;
  while (size > 0) {
    ssize_t n = fd < 0 ? getrandom(p, size, 0) : read(fd, p, size);
    if (n < 0 && errno == EINTR) continue;
    if (n <= 0) {
      /* No getrandom (a kernel older than 3.17): /dev/urandom; no randomness at all: abort, as
         glibc's arc4random does */
      if (fd >= 0 || (fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC)) < 0) abort();
      continue;
    }
    p += n;
    size -= (size_t)n;
  }
  if (fd >= 0) close(fd);
}
HIDDEN uint32_t arc4random(void) {
  uint32_t r;
  arc4random_buf(&r, sizeof r);
  return r;
}
HIDDEN uint32_t arc4random_uniform(uint32_t bound) {
  if (bound < 2) return 0;
  uint32_t min = -bound % bound; /* rejects the values that would bias the result */
  for (;;) {
    uint32_t r = arc4random();
    if (r >= min) return r % bound;
  }
}

#ifdef __FLT16_MANT_DIG__
/* The conversions between _Float16 and float (IEEE binary16 and binary32), on the bits: the
   compiler would implement a conversion written in C with these very functions */
HIDDEN float __extendhfsf2(_Float16 h) {
  uint16_t x;
  memcpy(&x, &h, sizeof x);
  uint32_t sign = (uint32_t)(x & 0x8000) << 16, exponent = (x >> 10) & 0x1f, mantissa = x & 0x3ff, bits;
  if (exponent == 0x1f) {
    bits = sign | 0x7f800000 | (mantissa << 13); /* infinity or NaN */
  } else if (exponent != 0) {
    bits = sign | ((exponent + 112) << 23) | (mantissa << 13);
  } else if (mantissa == 0) {
    bits = sign; /* zero */
  } else {
    /* subnormal: normalize */
    exponent = 113;
    while (!(mantissa & 0x400)) {
      mantissa <<= 1;
      --exponent;
    }
    bits = sign | (exponent << 23) | ((mantissa & 0x3ff) << 13);
  }
  float f;
  memcpy(&f, &bits, sizeof f);
  return f;
}

HIDDEN __float128 __extendhftf2(_Float16 h) { return (__float128)__extendhfsf2(h); }

HIDDEN _Float16 __truncsfhf2(float f) {
  uint32_t x;
  memcpy(&x, &f, sizeof x);
  uint16_t sign = (uint16_t)((x >> 16) & 0x8000), bits;
  uint32_t exponent = (x >> 23) & 0xff, mantissa = x & 0x7fffff;
  if (exponent == 0xff) {
    bits = sign | 0x7c00 | (mantissa ? 0x200 | (mantissa >> 13) : 0); /* infinity or NaN */
  } else {
    int e = (int)exponent - 127 + 15;
    if (e >= 0x1f) {
      bits = sign | 0x7c00; /* overflow: infinity */
    } else if (e <= 0) {
      /* subnormal or zero: shift the mantissa, with its implicit bit, rounding to nearest even */
      if (e < -10) {
        bits = sign;
      } else {
        uint32_t m = mantissa | 0x800000;
        int shift = 14 - e;
        uint32_t half = m >> shift, rest = m & ((1u << shift) - 1), halfway = 1u << (shift - 1);
        if (rest > halfway || (rest == halfway && (half & 1))) ++half;
        bits = sign | (uint16_t)half;
      }
    } else {
      uint32_t half = ((uint32_t)e << 10) | (mantissa >> 13), rest = mantissa & 0x1fff;
      if (rest > 0x1000 || (rest == 0x1000 && (half & 1))) ++half; /* may round up to infinity */
      bits = sign | (uint16_t)half;
    }
  }
  _Float16 h;
  memcpy(&h, &bits, sizeof h);
  return h;
}
#endif
