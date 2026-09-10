/*  The range coder: the scalar coder of sh_v1xN_s.cpp as it stands in the
    psrc tree's rc.inc, cleaned -- Subbotin's carry-propagating byte-wise
    coder with a wide low accumulator, and the carryless twin it grew there.

    It replaces sh_v2f.inc, which renormalised one bit at a time and divided
    by the range two or three times per coded bit.  This one shifts by whole
    bytes, at most two per step and none on most, and multiplies once.  The
    stream is not the same, so the container's version went up with it --
    OGGCOMP-PLAN.md section 12.

    What was cleaned out: the vector kernel and all that served it (the
    staged store, the folded rpre, the shift-count shapes, the generator's
    fingerprint), the lane arrays, the header coder's aligned flush, the
    multi-symbol entry points nothing here calls, and a file included twice
    under two class names -- the carry mode is a template parameter now,
    beside the direction.

    Needs: byte, uint, qword; LIKELY and UNLIKELY; and RC_IO_BASE, a class
    template the includer names, which the coder derives from and gets its
    bytes through:

        byte get()          the next input byte; 0xFF past the end, forever
        void put(byte c)    the next output byte

    Reading 0xFF past the end is what lets the flush leave its tail out.  */

#ifndef RC_IO_BASE
#error "rc.inc: name the byte I/O as RC_IO_BASE before including it"
#endif

/*  Probabilities are P(bit == 0) in SCALE, and 0 < freq < SCALE.  */
enum { SCALElog = 15, SCALE = 1 << SCALElog };

/*  Width of the low accumulator, in bytes, 4..8.  4 is the classic coder.
    Wider only buys carry headroom: a carry has LOWBYTES - 4 bytes to travel
    inside the register before it can escape into emitted output, which is
    what lets the carryless twin drop propagation -- at 8 an escape is a
    2^-33 event per step.  The carrying twin needs none of it and minds none
    of it: the first LOWBYTES - 4 bytes it would emit are the zeros the
    delay line starts with, and emit() drops them.  */
#ifndef RC_LOWBYTES
#define RC_LOWBYTES 8
#endif
#if RC_LOWBYTES < 4 || RC_LOWBYTES > 8
#error "RC_LOWBYTES is 4..8"
#endif

/*  The most trailing 0xFF bytes the carrying flush may leave out.  The
    reader takes 0xFF past the end, so a trailing run need not be written;
    but the run is as long as the data makes it, and whatever follows the
    stream has to read as 0xFF that far.  Here the stream ends the file, or
    TC_MEMCOST's rent of 0xFF bytes follows it, so any cap is safe; 32 is
    what the coder came with.  */
#ifndef RC_FF_TRIM
#define RC_FF_TRIM 32
#endif

/*  Index of the top set bit; never called with 0.  */
#if defined(__GNUC__) || defined(__clang__)
#define RC_BSR64(x) ((uint) (63 - __builtin_clzll(x)))
#else
static inline uint RC_BSR64(qword x) { uint i = 0;  while (x >>= 1) i++;  return i; }
#endif

template <int f_DEC, int CARRYLESS = 0>
struct Rangecoder : RC_IO_BASE<Rangecoder<f_DEC, CARRYLESS> > {
  typedef RC_IO_BASE<Rangecoder<f_DEC, CARRYLESS> > t_IO;
  using t_IO::get;
  using t_IO::put;

  static const uint CODBYTES = 4;             /*  the width of code, and of range  */
  static const uint LOWBYTES = RC_LOWBYTES;
  static const uint LOWBITS  = LOWBYTES * 8;
  static const uint sTOP     = 1u << 24;      /*  below this, shift a byte ...  */
  static const uint gTOP     = 1u << 16;      /*  ... and below this, two  */
  static constexpr qword LOW_MASK = qword(-1) >> (64 - LOWBITS);
  static constexpr qword LOW_MAX  = LOW_MASK + 1;   /*  0 at 64 bits; only read narrower  */

  uint lowl, lowh;   /*  low, LOWBITS wide, as (lowh << 32) | lowl  */
  uint rpre;         /*  what the step added to low, applied at the next renorm  */
  uint Carry;        /*  carry out of low, pending until the next byte goes  */
  uint code;
  uint range;
  uint FFNum;        /*  carrying: the 0xFF run held back; carryless: the lost-carry flag  */
  uint Cache;        /*  carrying: the byte held back for a carry, uint(-1) before the first  */
  uint skip;         /*  bytes of the zero prefix still to drop  */

  /*  Carries that escaped low.  Non-zero means the stream is not decodable
      and has to be coded again by the carrying twin, which never loses one.  */
  uint carry_lost() const { return CARRYLESS ? FFNum : 0; }

  /*  low += rpre, returning the carry out of the accumulator.  At 8 bytes
      there is no bit left to hold the overflow, so it is read off the
      addition itself.  */
  uint low_Add() {
    uint c = 0;
    const uint cf = (lowl + rpre < lowl);
    lowl += rpre;
    if (LOWBYTES == 8) c = (lowh + cf < lowh);
    lowh += cf;
    if (LOWBYTES < 8) c = (lowh >= uint(LOW_MAX >> 32));
    lowh &= uint(LOW_MASK >> 32);             /*  clears that bit; nothing at 4  */
    return c;
  }

  uint low_Top() const {                      /*  the byte about to leave  */
    enum { TOPSH = LOWBITS > 32 ? LOWBITS - 40 : 0 };
    return LOWBITS > 32 ? ((lowh >> TOPSH) & 0xFF) : ((lowl >> 24) & 0xFF);
  }

  void low_Shift8() {
    lowh = (lowh << 8) | (lowl >> 24);
    lowl <<= 8;
    lowh &= uint(LOW_MASK >> 32);
  }

  qword low_Get() const { return (qword(lowh) << 32) + lowl; }

  /*  A byte to the stream, less the zero prefix: the first LOWBYTES - 4
      bytes out of the delay line, which nothing has reached yet.  The
      reader starts at the first real byte and knows nothing of them.  */
  void emit(byte c) {
    if (LOWBYTES > CODBYTES) { if (UNLIKELY(skip)) { skip--;  return; } }
    put(c);
  }

  void ShiftLow(uint n) {
    const uint c = low_Add();
    rpre = 0;
    if (CARRYLESS) {
      FFNum += c;                             /*  the flag  */
      for (; n; n--) { emit((byte) low_Top());  low_Shift8(); }
    } else {
      Carry |= c;
      /*  At most one carry can be pending: the sub-intervals nest, so
          everything added between two emitted bytes sums to less than one
          range.  */
      for (; n; n--) {
        const uint cl = low_Top();
        low_Shift8();
        if (LIKELY(cl != 0xFF || Carry)) {
          if (Cache != uint(-1)) emit((byte) (Cache + Carry));
          for (; FFNum; FFNum--) emit((byte) (Carry - 1));
          Cache = cl;  Carry = 0;
        } else FFNum++;
      }
    }
  }

  void ShiftCode(uint n) {
    code -= rpre;  rpre = 0;                  /*  mirrors ShiftLow's deferred add  */
    for (; n; n--) code = (code << 8) + get();
  }

  /*  The counted 0/1/2-byte shift is exact for the binary coder: its unit
      is range >> 15 >= 2^9, so a step cannot take range below 2^8.  */
  void rc_Renorm() {
    const uint n = (range < sTOP) + (range < gTOP);
    if (f_DEC == 0) ShiftLow(n); else ShiftCode(n);
    range <<= n * 8;
  }

  /*  One bit, at probability freq (in SCALE) that it is zero.  Encoding
      reads bit; decoding writes it.  */
  INLINE void rc_Process(uint freq, uint & bit) {
    uint b = bit;
    rc_Renorm();
    rpre = range >> SCALElog;
    rpre *= freq;
    if (f_DEC) b = (code >= rpre);
    range -= rpre;
    range = b ? range : rpre;
    rpre &= -b;
    bit = b;
  }

  void rc_Init() {
    lowl = lowh = 0;
    rpre = 0;  Carry = 0;  FFNum = 0;  Cache = uint(-1);
    range = 0xFFFFFFFFu;
    skip = LOWBYTES - CODBYTES;
    if (f_DEC) { code = 0;  for (uint i = 0; i < CODBYTES; i++) code = (code << 8) + get(); }
  }

  void rc_Quit() {
    if (f_DEC) return;
    rc_Renorm();                              /*  applies the last pending rpre  */
    uint i, n = LOWBYTES;
    qword llow = low_Get();
    const qword high = llow + range;
    const uint  hic  = (high < llow);         /*  past the qword, at 8 bytes  */
    /*  Flush the largest value in [low, high) whose bottom bits are all 1:

            low | (2^k - 1) < high   <=>   (low >> k) < (high >> k)

        so the longest fillable run is the position of the top set bit of
        low ^ high -- one bit scan.  Only whole bytes of it shorten the
        flush, since the reader takes 0xFF past the end; filling the rest
        is free and fixes the value of the last byte written.  range >= sTOP
        at every flush, so k >= 24 and at least three bytes are dropped.  */
    const uint k = hic ? LOWBITS : RC_BSR64(llow ^ high);
    llow |= (k >= 64) ? ~qword(0) : ((qword(1) << k) - 1);
    n -= k / 8;
    if (CARRYLESS) {
      /*  Cache and the run do not exist here: those bytes are written.  */
      for (i = 0; i < n; i++) emit((byte) (llow >> (LOWBITS - 8 - i * 8)));
    } else if (n == 0 && Carry == 0) {
      /*  Everything left is one run of 0xFF -- Cache when it is one, and
          FFNum more -- and up to RC_FF_TRIM of it need not be written.  */
      const uint run  = FFNum + (Cache != uint(-1) && Cache == 0xFF);
      const uint keep = run > RC_FF_TRIM ? run - RC_FF_TRIM : 0;
      if (Cache != uint(-1) && Cache != 0xFF) emit((byte) Cache);
      for (i = 0; i < keep; i++) emit(0xFF);
    } else {
      if (Cache != uint(-1)) emit((byte) (Cache + Carry));
      for (i = 0; i < FFNum; i++) emit((byte) (0xFF + Carry));
      for (i = 0; i < n; i++) emit((byte) (llow >> (LOWBITS - 8 - i * 8)));
    }
  }
};

#undef RC_BSR64
