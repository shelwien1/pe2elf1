/* d5c_rc.inc -- part of dxt5comp_core.inc; not standalone.
 *
 * RC<MD> -- the binary range coder, as a Coroutine layer. */

//------------------------------------------------------------------- range coder (LZMA-style binary)

/* mode 0 = encode, 1 = decode.  `enc` is a class constant with the same name
 * the runtime flag had, so every `if(enc)` / `enc ? :` in this file and in
 * Codec folds at compile time and the bodies did not need editing.  The decode
 * instantiation carries no shift_low / cache / carry code at all, and the
 * encode one no code/rd path. */
/* mode 0 = encode, 1 = decode.  `enc` is a class constant with the same name
 * the runtime flag had, so every `if(enc)` / `enc ? :` in this file and in
 * Codec folds at compile time and the bodies did not need editing.  The decode
 * instantiation carries no shift_low / cache / carry code at all, and the
 * encode one no code/rd path.
 *
 * RC derives from Coroutine and is meant to be a BASE of the frontend that
 * drives it (ddsdet<mode>, dxt5comp's d5cM<mode>), not a separate coroutine
 * instance: one Coroutine, one stack, RC as a layer in the chain.  The
 * compressed bytes move through `rcio`, one of that coroutine's pins, so when
 * the window runs dry deep inside the codec the pin's yield suspends the whole
 * carver+codec call stack and returns to the driver for a refill/flush.
 *
 * rcio defaults to the inherited put()/get() streams (pin[1] when encoding,
 * pin[0] when decoding), which is what a frontend whose only output IS the
 * coded stream wants.  Frontends with a second stream -- ddsdet's cs writes
 * the restfile on pin[1] and the archive on pin[2] -- point it elsewhere. */
template<int MD>
struct RC : Coroutine {
  enum { enc = (MD == 0) };
  coro3_pin* rcio = 0;
  u32 range, code; u64 low; u8 cache; u64 cachesz;
  /* Replaces the old `rcbuf.size()` deltas: with the stream going straight out
   * through a pin there is no buffer to measure. */
  u64 emitted = 0, consumed = 0;

  coro3_pin* rc_defpin() { return &pin[enc ? 1 : 0]; }

  void init_enc(coro3_pin* p) {
    rcio = p ? p : rc_defpin();
    low = 0; range = 0xFFFFFFFFu; cache = 0; cachesz = 1; emitted = 0;
  }
  void init_dec(coro3_pin* p) {
    rcio = p ? p : rc_defpin();
    range = 0xFFFFFFFFu; code = 0; consumed = 0;
    for(int i = 0; i < 5; i++) code = code << 8 | rd();
  }
  /* pin get() returns uint(-1) at EOF+f_quit; mapping that to 0 keeps the old
   * behaviour exactly -- a truncated stream decodes to zeros and is caught by
   * the entry crc rather than overrunning. */
  u8 rd() { uint v = rcio->get(); consumed++; return v > 255 ? 0 : u8(v); }
  void shift_low() {
    if(u32(low) < 0xFF000000u || (low >> 32) != 0) {
      u8 c = cache;
      do { rcio->put(u8(c + u8(low >> 32))); emitted++; c = 0xFF; } while(--cachesz);
      cache = u8(low >> 24);
    }
    cachesz++; low = u32(low) << 8;
  }
  // p1 = P(bit==1), P_BITS wide, in [P_MIN, P_MAX]
  int bit(int p1, int b) {
    u32 bound = (range >> P_BITS) * u32(p1);
    if(enc) {
      if(b) range = bound; else { low += bound; range -= bound; }
      while(range < (1u << 24)) { range <<= 8; shift_low(); }
    } else {
      b = code < bound;
      if(b) range = bound; else { code -= bound; range -= bound; }
      while(range < (1u << 24)) { code = code << 8 | rd(); range <<= 8; }
    }
    return b;
  }
  void flush() { for(int i = 0; i < 5; i++) shift_low(); }

  /* Raw bytes straight onto the coded stream, outside any model.
   * At a flush boundary the encoder's byte position and the decoder's are the
   * same (see the production==consumption note on Solid), so a blob written
   * here between flush() and a fresh init is read back byte for byte. */
  void raw_put(cu8P d, size_t n) { for(size_t i = 0; i < n; i++) { rcio->put(d[i]); emitted++; } }
  void raw_get(u8P d, size_t n)  { for(size_t i = 0; i < n; i++) d[i] = rd(); }
};
