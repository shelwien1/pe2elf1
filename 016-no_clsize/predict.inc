
// The counter shape is a per-MODEL choice, not a per-build one: the encoder
// and the decoder are separate types with separate `stats` arrays and no
// shared state, so each can carry whatever counter its own loop likes. They
// disagree on this box -- see RC_FUSE_PP_ENC / RC_FUSE_PP_DEC in
// rc_config.inc -- which is the whole reason this is a template parameter.
template< int FUSE >
struct PredictorT {

  uint ctx,ctz;  // o0 aligned bit context

  typedef typename SelCounter<FUSE>::t t_Counter;

  t_Counter* __restrict cty;

  // What P_S hands back is what P_update takes and what P_P turns into a
  // probability -- the counter's payload, whatever shape it has. Unfused that
  // is the raw state and P_P is the pp[] load; fused it is the {p,state} dword
  // and P_P is a mask.
  static uint P_P( uint _s ) { return t_Counter::P(_s); }

  // cty is NOT biased by -1. The bias would cost nothing on its own, but it
  // makes cty[2*c] land at an odd index. Index 0 goes unused instead;
  // contexts run 1..255 and the models' stats[] is 0x200 entries.
  void P_Init( t_Counter* p0_, uint p_size ) { 
    t_Counter* __restrict P_p = p0_;
    for( uint i=0; i<p_size; i++ ) P_p[i].Reset();
    ctx=1; cty=P_p;
  }

  void P_S( uint& _s ) {  
    _s = cty[ctx].Get();
  }

  void P_byte( void* p_, uint& c ) { 
    c = ctx&0xFF; ctx=1; 
  }

  void P_ctxfix() {
  }

  void P_update( uint state, uint bit ) {
    cty[ctx].Update( state, bit );   
    (ctx*=2)+=bit;
  }

  // The same two, against a caller-held context. A pass that walks a whole
  // block wants ctx in a register, and cannot get it: the store to
  // cty[ctx].state may-alias this->ctx -- ctx is a loaded index, so nothing
  // bounds it -- and LICM gives up, leaving ctx written once per input byte.
  // Handing the context in by reference sidesteps the aliasing question
  // entirely. Identical arithmetic, identical traffic through cty[].
  void P_S_L( uint _c, uint& _s ) {
    _s = cty[_c].Get();
  }

  void P_update_L( uint& _c, uint state, uint bit ) {
    cty[_c].Update( state, bit );
    (_c*=2)+=bit;
  }

  uint Init( void* _p ) {
    uint r = 0;
    if( _p!=0 ) {
      char* fnam = (char*)_p;
      FILE* h = fopen( fnam, "rb" ); if( h==0 ) r=4; else {
        if( FSM[0].Load(h) ) r=5;
        fclose(h);
      }
    }
    return r;
  }

  void* Save( void* r0 = 0 ) { return 0; }

  uint Quit( void ) { return 0; }

};

typedef PredictorT< RC_FUSE_PP_ENC > PredictorE;   // Model<0>
typedef PredictorT< RC_FUSE_PP_DEC > PredictorD;   // Model<1>
