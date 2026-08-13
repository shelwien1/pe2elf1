// ida.inc -- the Hex-Rays vocabulary -- the macros the decompiler writes, and the two
// helpers it calls by name.

#define BYTEn(x, n) (*((uint8_t*)&(x)+(n)))
#define WORDn(x, n) (*((uint16_t*)&(x)+(n)))
#define LOBYTE(x) BYTEn(x, 0)
#define LOWORD(x) WORDn(x, 0)
#define HIBYTE(x) BYTEn(x, sizeof(x)-1)
#define HIWORD(x) WORDn(x, sizeof(x)/2-1)
#define BYTE1(x) BYTEn(x, 1)
#define BYTE2(x) BYTEn(x, 2)
#define BYTE4(x) BYTEn(x, 4)
#define WORD2(x) WORDn(x, 2)
#define __PAIR64__(high, low) (((uint64_t)(high)<<32)|(uint32_t)(low))

#define BMF_SPILL_PAD(n) uint8_t _spill_pad ## n[sizeof(void*)-4]

inline uint32_t abs32(int32_t x) {
  return x>=0 ? (uint32_t)x : (uint32_t)-x;
}

inline int8_t __OFSUB__(int32_t x, int32_t y) {
  int8_t sx = (int8_t)(x<0);
  return (int8_t)((sx^(int8_t)(y<0))&(sx^(int8_t)((x-y)<0)));
}
