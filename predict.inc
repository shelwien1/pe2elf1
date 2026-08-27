
struct Predictor {

  uint ctx,ctz;  // o0 aligned bit context

  Counter* __restrict cty;

  void P_Init( Counter* p0_, uint p_size ) { 
    Counter* __restrict P_p = p0_;
    for( uint i=0; i<p_size; i++ ) P_p[i].state=0; 
    ctx=1; cty=P_p-1;
  }

  void P_S( uint& _s ) {  
    _s = cty[ctx].state;   
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
