
typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned short word;
typedef unsigned long long qword;

#ifndef _MSC_VER
#define __int64 long long
#define __int32 int
#define __int16 short
#define __int8  char
#endif

// Some convenience macros to make partial accesses nicer
#define LAST_IND(x,part_type)    (sizeof(x)/sizeof(part_type) - 1)
#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN
#  define LOW_IND(x,part_type)   LAST_IND(x,part_type)
#  define HIGH_IND(x,part_type)  0
#else
#  define HIGH_IND(x,part_type)  LAST_IND(x,part_type)
#  define LOW_IND(x,part_type)   0
#endif
// first unsigned macros:
#define BYTEn(x, n)   (*((byte*)&(x)+n))
#define WORDn(x, n)   (*((word*)&(x)+n))
#define DWORDn(x, n)  (*((uint*)&(x)+n))

#define LOBYTE(x)  BYTEn(x,LOW_IND(x,byte))
#define LOWORD(x)  WORDn(x,LOW_IND(x,word))
#define LODWORD(x) ((qword&)x)
#define HIBYTE(x)  BYTEn(x,HIGH_IND(x,byte))
#define HIWORD(x)  WORDn(x,HIGH_IND(x,word))
#define BYTE1(x)   BYTEn(x,  1)         // byte 1 (counting from 0)
#define BYTE2(x)   BYTEn(x,  2)
#define BYTE4(x)   BYTEn(x,  4)
#define WORD2(x)   WORDn(x,  2)         // third word of the object, unsigned

uint abs32( int x ) { return x >= 0 ? x : -x; }
