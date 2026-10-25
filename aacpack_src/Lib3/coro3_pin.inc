
#include "coro3_pin_0.inc"

struct coro3_pin: coro3_pin_0 {
  typedef Coroutine wrap;

  void pin_init( wrap* that, uint _r_code ) {
    ptr=beg=end=0; f_EOF=0;
    base_offs = ((char*)this) - ((char*)that);
    r_code = _r_code;
  }

  void yield_r( void ) {
    wrap& W = *(wrap*)(((char*)this) - base_offs );
    yield( (void*)&W, r_code );
  }

  void chkinp( void ) { if_e0( ptr>=end ) yield_r(); }

  void chkout( uint d=0 ) { if_e0( ptr>=end-d ) yield_r(); }

};
