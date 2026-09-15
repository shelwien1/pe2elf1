
typedef enum {COMPRESS, DECOMPRESS} Mode;

// Glue between the Tangelo model and the bytewise coder.
//
// Tangelo's step is inverted relative to fpaq0mw's. Its Model::predictNext()
// first folds the bit that was just coded (the global y) into c0/bpos and the
// contexts, and then returns the probability of the NEXT bit; its own Encoder
// seeds p=2048 before the first bit and calls predictNext() after coding each
// one. The walk in track.inc wants the fpaq0mw shape instead - "tell me the
// prediction you are about to use, then consume this bit" - so the pending
// prediction is held in a member:
//
//   P()             the probability the next coded bit will use, in [1,SCALE-1]
//   encode_sim(b)   returns P(), then advances the model past bit b
//
// Both run inside the speculative walk (instrumented, every store journaled)
// and in the real update in main.inc (persistent), so p is model state like any
// other: it is written through this object and rolled back with the rest.

struct Coder {

  Mode mode;
  uint p;              // pending prediction, already clamped for the coder
  Rangecoder rc;
  Model M;

  void rc_Init( Mode _mode, FILE* f ) {
    mode = _mode;
    M.Init();
    y = 0;
    bpos = 0;
    p = hSCALE;        // what Tangelo's own Encoder starts with
    if( mode==DECOMPRESS ) {
      rc.StartDecode(f);
    } else {
      rc.StartEncode(f);
    }
  }

  uint P( void ) { return p; }

  // INLINE so that the -DSIM_FUNC translation unit defines exactly one symbol,
  // the extern "C" encode_sim below. Anything left out of line there is a weak
  // COMDAT symbol that the linker may resolve to the normal build's copy,
  // silently dropping the instrumentation; track.pl refuses such an input.
  INLINE
  uint encode_sim( int bit ) {
    uint p0 = p;
    y = bit;
    int pr = M.predictNext();
    // The coder needs 0 < p < SCALE: it takes both LOG2(p) and LOG2(SCALE-p),
    // and a zero-width interval cannot be coded. Clamping the pending value
    // rather than the model's own pr0 leaves the APM feedback untouched.
    p = (pr<1) ? 1 : ( (pr>SCALE-1) ? SCALE-1 : uint(pr) );
    return p0;
  }

  void flush( void ) {
    rc.rc_Quit();
  }

};


#ifdef SIM_FUNC

// The instrumented entry point. This is the function that gets compiled to
// assembly, journaled by track.pl and called from the speculative walk.
extern "C" uint encode_sim( void* _p, int bit ) {
  auto& c = *(Coder*)_p;
  return c.encode_sim(bit);
}

#endif
