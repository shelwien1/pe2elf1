// The binary range coder of oggcomp (balrogg-P-hash rc.inc): 15-bit
// probabilities, 64-bit low with carry propagation -- oggcomp's options as
// h264p has them: no carryless mode, eight bytes of low. The flush writes
// all of low, so that the decoder reads exactly the bytes the coder wrote,
// to the last: none past them, none left.
//
// The byte I/O is Lib3's Coroutine itself (coro3b.inc), the coder's base:
// put() into the output pin, a yield 2 when its buffer is full; get() from
// the input pin, a yield 1 when it is empty. A stream cut short is the only
// one whose decoder reads past its end, where get() returns uint(-1) once
// the driver has set f_quit: the driver sees that it had to (ttfcomp's
// RcIo wrapper, which counted and checked every byte, is gone).
// Include after Lib3/common.inc and Lib3/coro3b.inc.

#ifndef LIKELY
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

namespace h264p {

enum { SCALElog = 15 };
#if defined(__GNUC__) || defined(__clang__)
#define RC_BSR64(x) ((uint)(63 - __builtin_clzll(x)))
#else
static inline uint RC_BSR64(uint64_t x) {
  uint i = 0;
  while(x >>= 1)
    i++;
  return i;
}
#endif
template <int f_DEC> struct Rangecoder : Coroutine {
  static const uint CODBYTES = 4;
  static const uint LOWBYTES = 8;
  static const uint LOWBITS = LOWBYTES * 8;
  static const uint sTOP = 1u << 24;
  static const uint gTOP = 1u << 16;
  uint lowl, lowh;
  uint rpre;
  uint Carry;
  uint code;
  uint range;
  uint64_t FFNum;  // (a run of 0xFF bytes can pass 2^32: 11 TB of bytes all 0xFF coded as they are)
  uint Cache;
  uint skip;
  uint low_Add() {
    const uint cf = (lowl + rpre < lowl);
    lowl += rpre;
    const uint c = (lowh + cf < lowh);
    lowh += cf;
    return c;
  }
  uint low_Top() const { return (lowh >> 24) & 0xFF; }
  void low_Shift8() {
    lowh = (lowh << 8) | (lowl >> 24);
    lowl <<= 8;
  }
  uint64_t low_Get() const { return (uint64_t(lowh) << 32) + lowl; }
  // (the first LOWBYTES - CODBYTES bytes of low are not output)
  void emit(byte c) {
    if(UNLIKELY(skip)) {
      skip--;
      return;
    }
    put(c);
  }
  void ShiftLow(uint n) {
    Carry |= low_Add();
    rpre = 0;
    for(; n; n--) {
      const uint cl = low_Top();
      low_Shift8();
      if(LIKELY(cl != 0xFF || Carry)) {
        if(Cache != uint(-1))
          emit((byte)(Cache + Carry));
        for(; FFNum; FFNum--)
          emit((byte)(Carry - 1));
        Cache = cl;
        Carry = 0;
      } else
        FFNum++;
    }
  }
  void ShiftCode(uint n) {
    code -= rpre;
    rpre = 0;
    for(; n; n--)
      code = (code << 8) + get();
  }
  void rc_Renorm() {
    const uint n = (range < sTOP) + (range < gTOP);
    if(f_DEC == 0)
      ShiftLow(n);
    else
      ShiftCode(n);
    range <<= n * 8;
  }
  INLINE void rc_Process(uint freq, uint &bit) {
    uint b = bit;
    rc_Renorm();
    rpre = range >> SCALElog;
    rpre *= freq;
    if(f_DEC)
      b = (code >= rpre);
    range -= rpre;
    range = b ? range : rpre;
    rpre &= -b;
    bit = b;
  }
  //  A byte as it is: each of its 256 values equally likely, in one step
  //  (range at least 2^24 first, so each value has 2^16 of it or more: next
  //  to nothing is lost to the rounding). Decoding, the byte; a code past
  //  the last value, which only a broken stream gives, reads as 255.
  INLINE uint rc_Byte(uint b) {
    rc_Renorm();
    if(range < sTOP)
      rc_Renorm();
    const uint r = range >> 8;
    if(f_DEC) {
      b = code / r;
      b = b > 255 ? 255 : b;
    }
    rpre = r * b;
    range = r;
    return b;
  }
  void rc_Init() {
    lowl = lowh = 0;
    rpre = 0;
    Carry = 0;
    FFNum = 0;
    Cache = uint(-1);
    range = 0xFFFFFFFFu;
    skip = LOWBYTES - CODBYTES;
    if(f_DEC) {
      code = 0;
      for(uint i = 0; i < CODBYTES; i++)
        code = (code << 8) + get();
    }
  }
  //  The flush: the last decision's low, then a value in [low, low +
  //  range), all LOWBYTES of it (no renormalisation: the decoder, which
  //  read CODBYTES ahead, has then read every byte written, and the coder
  //  skipped LOWBYTES - CODBYTES)
  void rc_Quit() {
    if(f_DEC)
      return;
    ShiftLow(0);
    uint64_t llow = low_Get();
    const uint64_t high = llow + range;
    const uint k = high < llow ? LOWBITS : RC_BSR64(llow ^ high);
    llow |= (k >= 64) ? ~uint64_t(0) : ((uint64_t(1) << k) - 1);
    if(Cache != uint(-1))
      emit((byte)(Cache + Carry));
    for(uint64_t i = 0; i < FFNum; i++)
      emit((byte)(0xFF + Carry));
    for(uint i = 0; i < LOWBYTES; i++)
      emit((byte)(llow >> (LOWBITS - 8 - i * 8)));
  }
};
#undef RC_BSR64

}  // namespace h264p
