#ifndef RC_IO_BASE
#error "rc.inc: name the byte I/O as RC_IO_BASE before including it"
#endif
enum { SCALElog = 15 };
#ifndef RC_LOWBYTES
#define RC_LOWBYTES 8
#endif
#if RC_LOWBYTES < 4 || RC_LOWBYTES > 8
#error "RC_LOWBYTES is 4..8"
#endif
#ifndef RC_FF_TRIM
#define RC_FF_TRIM 32
#endif
#if defined(__GNUC__) || defined(__clang__)
#define RC_BSR64(x) ((uint)(63 - __builtin_clzll(x)))
#else
static inline uint RC_BSR64(qword x) {
  uint i = 0;
  while(x >>= 1)
    i++;
  return i;
}
#endif
template <int f_DEC, int CARRYLESS = 0> struct Rangecoder : RC_IO_BASE<Rangecoder<f_DEC, CARRYLESS>> {
  typedef RC_IO_BASE<Rangecoder<f_DEC, CARRYLESS>> t_IO;
  using t_IO::get;
  using t_IO::put;
  static const uint CODBYTES = 4;
  static const uint LOWBYTES = RC_LOWBYTES;
  static const uint LOWBITS = LOWBYTES * 8;
  static const uint sTOP = 1u << 24;
  static const uint gTOP = 1u << 16;
  static constexpr qword LOW_MASK = qword(-1) >> (64 - LOWBITS);
  static constexpr qword LOW_MAX = LOW_MASK + 1;
  uint lowl, lowh;
  uint rpre;
  uint Carry;
  uint code;
  uint range;
  uint FFNum;
  uint Cache;
  uint skip;
  uint carry_lost() const { return CARRYLESS ? FFNum : 0; }
  uint low_Add() {
    uint c = 0;
    const uint cf = (lowl + rpre < lowl);
    lowl += rpre;
    if(LOWBYTES == 8)
      c = (lowh + cf < lowh);
    lowh += cf;
    if(LOWBYTES < 8)
      c = (lowh >= uint(LOW_MAX >> 32));
    lowh &= uint(LOW_MASK >> 32);
    return c;
  }
  uint low_Top() const {
    enum { TOPSH = LOWBITS > 32 ? LOWBITS - 40 : 0 };
    return LOWBITS > 32 ? ((lowh >> TOPSH) & 0xFF) : ((lowl >> 24) & 0xFF);
  }
  void low_Shift8() {
    lowh = (lowh << 8) | (lowl >> 24);
    lowl <<= 8;
    lowh &= uint(LOW_MASK >> 32);
  }
  qword low_Get() const { return (qword(lowh) << 32) + lowl; }
  void emit(byte c) {
    if(LOWBYTES > CODBYTES) {
      if(UNLIKELY(skip)) {
        skip--;
        return;
      }
    }
    put(c);
  }
  void ShiftLow(uint n) {
    const uint c = low_Add();
    rpre = 0;
    if(CARRYLESS) {
      FFNum += c;
      for(; n; n--) {
        emit((byte)low_Top());
        low_Shift8();
      }
    } else {
      Carry |= c;
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
  void rc_Quit() {
    if(f_DEC)
      return;
    rc_Renorm();
    uint i, n = LOWBYTES;
    qword llow = low_Get();
    const qword high = llow + range;
    const uint hic = (high < llow);
    const uint k = hic ? LOWBITS : RC_BSR64(llow ^ high);
    llow |= (k >= 64) ? ~qword(0) : ((qword(1) << k) - 1);
    n -= k / 8;
    if(CARRYLESS) {
      for(i = 0; i < n; i++)
        emit((byte)(llow >> (LOWBITS - 8 - i * 8)));
    } else if(n == 0 && Carry == 0) {
      const uint run = FFNum + (Cache != uint(-1) && Cache == 0xFF);
      const uint keep = run > RC_FF_TRIM ? run - RC_FF_TRIM : 0;
      if(Cache != uint(-1) && Cache != 0xFF)
        emit((byte)Cache);
      for(i = 0; i < keep; i++)
        emit(0xFF);
    } else {
      if(Cache != uint(-1))
        emit((byte)(Cache + Carry));
      for(i = 0; i < FFNum; i++)
        emit((byte)(0xFF + Carry));
      for(i = 0; i < n; i++)
        emit((byte)(llow >> (LOWBITS - 8 - i * 8)));
    }
  }
};
#undef RC_BSR64
