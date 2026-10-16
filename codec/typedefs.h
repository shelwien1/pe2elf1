
// typedef.h
#ifndef WELS_TYPE_DEFINES_H__
#define WELS_TYPE_DEFINES_H__

#include <limits.h>
#include <stddef.h>

////////////////////////////////////////////////////////////////////////////
// NOTICE : ALL internal implement MUST use the data type defined as below
//          ONLY except with the interface file !!!!!
////////////////////////////////////////////////////////////////////////////

#ifndef  _MSC_VER

#define __STDC_FORMAT_MACROS
#include <stdint.h>
#include <inttypes.h>

#ifdef __LP64__
typedef int64_t intX_t;
#else
typedef int32_t intX_t;
#endif

#else

// FIXME:     all singed type should be declared explicit,  for example,  int8_t should be declared as signed char.
typedef signed char      int8_t  ;
typedef unsigned char    uint8_t ;
typedef short            int16_t ;
typedef unsigned short   uint16_t;
typedef int              int32_t ;
typedef unsigned int     uint32_t;
typedef __int64          int64_t ;
typedef unsigned __int64 uint64_t;
#define PRId64 "I64d"

#ifdef _WIN64
typedef int64_t intX_t;
#else
typedef int32_t intX_t;
#endif

#endif // _MSC_VER defined

// The 'float' type is portable and usable without any need for any extra typedefs.

#ifdef EPSN
#undef EPSN
#endif//EPSN
#define EPSN (0.000001f) // (1e-6) // desired float precision

#endif //WELS_TYPE_DEFINES_H__

