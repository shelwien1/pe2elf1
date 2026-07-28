/* xad_rc.inc -- part of xadpcm.cpp; not standalone.
 *
 * RC<MD> -- the binary range coder, as a Coroutine layer. */

/* MD 0 = encode, 1 = decode.  `enc` is a class constant carrying the name the
   runtime flag had, so every `if(enc)` and `enc ? :` here and in Codec folds at
   compile time and the bodies did not need editing.  The decode instantiation
   carries no shift_low / cache / carry code at all, and the encode one no
   code/rd path.

   RC derives from Coroutine and is meant to be a BASE of the frontend that
   drives it (Xad<MD>), not a separate coroutine instance: one Coroutine, one
   stack, RC as a layer in the chain.  The compressed bytes move through `rcio`,
   one of that coroutine's pins, so when the window runs dry deep inside the bit
   tree the pin's yield suspends the whole codec call stack and returns to the
   driver for a refill or a flush.

   rcio defaults to the inherited put()/get() streams -- pin[1] when encoding,
   pin[0] when decoding -- which is exactly what xadpcm wants, since the coded
   stream IS its output (resp. input).  The argument exists for frontends with a
   second stream. */
template<int MD>
struct RC : Coroutine {
  enum { enc = (MD==0) };
  coro3_pin* rcio = nullptr;
  u32 range = 0, code = 0;
  u64 low = 0;
  u8 cache = 0;
  u64 cachesz = 0;
  /* Replaces the old out->size() deltas: with the stream going straight out
     through a pin there is no buffer to measure. */
  u64 emitted = 0, consumed = 0;

  /* yield() copies stkptrH-stkptrL bytes into Coroutine::stk[STKPAD] -- 64 KB --
     with no bound check, and an overflow there is an intra-object overwrite
     that ASan cannot see (the same shape as the MIX_MAXIN bug README-dual.txt
     describes).  It would present as a wild jump rather than a clean crash, so
     measure rather than assume.  The probe sits HERE, in the two functions that
     touch a pin, because these are the only places in the codec that can yield:
     anywhere higher and the reading misses the frames below it. */
  u64 stk_hi = 0;

  INLINE void stk_probe() {
    if( !verbose )
      return;
    char here;
    u64 d = u64((const char*)stkptrH-&here);
    if( d>stk_hi )
      stk_hi = d;
  }

  coro3_pin* rc_defpin() {
    return &pin[enc ? 1 : 0];
  }

  void init_enc(coro3_pin* p = nullptr) {
    rcio = p ? p : rc_defpin();
    low = 0;
    range = 0xFFFFFFFFu;
    cache = 0;
    cachesz = 1;
    emitted = 0;
  }

  void init_dec(coro3_pin* p = nullptr) {
    rcio = p ? p : rc_defpin();
    range = 0xFFFFFFFFu;
    code = 0;
    consumed = 0;
    for( int i = 0; i<RC_PRIME; i++ )
      code = code<<8|rd();
  }

  /* A pin get() returns uint(-1) at EOF+f_quit; mapping that to 0 keeps the old
     `inpos<insize ? in[inpos++] : 0` behaviour exactly -- a truncated stream
     decodes to zeros and is caught by the entry crc rather than overrunning. */
  u8 rd() {
    stk_probe();
    uint v = rcio->get();
    consumed++;
    return v>255 ? 0 : u8(v);
  }

  void shift_low() {
    stk_probe();
    if( u32(low)<0xFF000000u||(low>>32)!=0 ) {
      u8 c = cache;
      do {
        rcio->put(u8(c+u8(low>>32)));
        emitted++;
        c = 0xFF;
      } while( --cachesz );
      cache = u8(low>>24);
    }
    cachesz++;
    low = u32(low)<<8;
  }

  // p1 = P(bit==1), 12-bit, in [1,4095]
  INLINE int bit(int p1, int b) {
    u32 bound = (range>>P_BITS)*u32(p1);
    if( enc ) {
      if( b )
        range = bound;
      else {
        low += bound;
        range -= bound;
      }
      while( range<u32(RC_TOP) ) {
        range <<= 8;
        shift_low();
      }
    } else {
      b = code<bound;
      if( b )
        range = bound;
      else {
        code -= bound;
        range -= bound;
      }
      while( range<u32(RC_TOP) ) {
        code = code<<8|rd();
        range <<= 8;
      }
    }
    return b;
  }

  void flush() {
    for( int i = 0; i<RC_PRIME; i++ )
      shift_low();
  }

  // raw bytes straight onto the coded stream, outside any model
  void raw_put(const u8* d, size_t n) {
    for( size_t i = 0; i<n; i++ ) {
      rcio->put(d[i]);
      emitted++;
    }
  }

  void raw_get(u8* d, size_t n) {
    for( size_t i = 0; i<n; i++ )
      d[i] = rd();
  }
};
