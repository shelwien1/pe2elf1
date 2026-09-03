
#ifndef RC_BIJ
#define RC_BIJ 1
#endif

// ---- I/O layer over coro3b pins ---------------------------------------------
// Rangecoder-side: bij_get() = one codestream bit (decoder, pin[0]),
// bij_put() = one codestream bit (encoder, pin[1]).  f_EOF: the last bij_get()
// was an implied tail bit.
//
// [CDM port] sh_bits names these get()/put().  CDM's decode_block() calls a bare
// put(c) to emit a DECODED PLAINTEXT byte on pin[1], and that must not go through
// the one-bit-delay bijection below -- so the codestream accessors are spelled
// bij_get/bij_put here and plain get/put stay Coroutine's.  Nothing else changed.
template< class Rangecoder >
struct RC_IO : Coroutine {
  uint f_EOF;  // decoder: codestream exhausted, bij_get() returns the implied 1s

  // RC_BIJ state
  int  held;   // encoder: last codestream bit not yet written (-1 = none)
  uint all1;   // every bit written/read so far was 1 (vacuously true at start)
  uint nxt;    // decoder: lookahead bit; uint(-1)=EOF, uint(-2)=not primed
  uint cnt;    // decoder: real bits delivered so far
  uint zdone;  // decoder: the canonical trailing 0 has been delivered

  uint raw_get( void ) { return pin[0].get(); }  // uint(-1) at end of input
  void raw_put( uint c ) { pin[1].put(c); }

  byte bij_get( void ) {
    uint c;
    if( RC_BIJ==0 ) { c=raw_get(); f_EOF=(c==uint(-1)); return c&1; }
    if( nxt==uint(-2) ) nxt = raw_get();
    c = nxt;
    if( c!=uint(-1) ) {                       // real bit; peek at the one after it
      nxt = raw_get();
      if( (nxt==uint(-1)) && all1 && (c==1) ) c=0, zdone=1; // 1^(k+1) -> 1^k 0
      all1 &= c; cnt++;
      f_EOF = 0; return c;
    }
    if( !zdone ) { zdone=1; if( cnt ) { f_EOF=0; return 0; } } // x -> x0
    f_EOF = 1; return 1;                      // implied tail
  }

  void bij_put( byte c ) {
    if( RC_BIJ==0 ) { raw_put(c&1); return; }
    if( held>=0 ) { raw_put(held); all1 &= held; }
    held = c&1;
  }

  void put_flush( void ) {                    // held = the canonical final 0
    if( (RC_BIJ==0) || (held<0) ) return;
    if( held ) raw_put(held); else if( all1 ) raw_put(1); // 1^k 0 -> 1^(k+1)
  }

  void io_init( void ) { f_EOF=0; held=-1; all1=1; nxt=uint(-2); cnt=0; zdone=0; }
};
