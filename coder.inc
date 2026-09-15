
typedef enum {COMPRESS, DECOMPRESS} Mode;

struct Coder {

  Mode mode;

  Predictor predictor;

  Rangecoder rc;


  void rc_Init( Mode _mode, FILE* f ) {
    mode = _mode;
    if( mode==DECOMPRESS ) {
      rc.StartDecode(f);
    } else {
      rc.StartEncode(f);
    }
  }

  void encode_p( int y, int p ) {
    if( y==1 ) {
      rc.rc_Process( 0, p,       SCALE );
    } else {
      rc.rc_Process( p, SCALE-p, SCALE );
    }
  }

  int decode_p( int p ) {
#if 0
    uint r = rc.rc_GetFreq(SCALE);
    uint y = r<p;
#else 
    uint y = rc.code < rc.rc_Bound(p,SCALE);
#endif
    encode_p( y, p );
    return y;
  }

  void encode( int y ) {
    encode_p( y, predictor.P() );
    predictor.update(y);
  }

  // The two calls the walk in track.inc makes on a model (see main.inc):
  // P() is the prediction the next coded bit will use, encode_sim() returns it
  // and then advances the model past that bit.
  uint P( void ) { return predictor.P(); }

  // See coder_tangelo.inc: the walk is only meaningful from a byte boundary,
  // which for this model is the context reset to 1 after the eighth bit.
  bool at_byte_boundary( void ) { return predictor.cxt==1; }

  uint encode_sim( int y ) {
    uint p = predictor.P();
    predictor.update(y);
    return p;
  }

  int decode() {
    int y = decode_p( predictor.P() );
    predictor.update(y);
    return y;
  }

  void flush() {
    rc.rc_Quit();
  }

};


#ifdef SIM_FUNC

extern "C" uint encode_sim( void* _p, int y ) {
  auto& p = *(Coder*)_p;
  return p.encode_sim(y);
}

#endif


