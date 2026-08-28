
#define INC_LOG2I
#define INC_FLEN
#pragma pack(push)
#include "common.inc"
#pragma pack(pop)

// CORO_FAKE and the stream buffer sizes live here, so coder.cpp and the model
// cannot disagree about which coroutine is in use.
#include "../rc_config.inc"

#if CORO_FAKE==0
#include "coro3b.inc"
//#include "coro3c.inc"
#else
#include "coro3b_fake.inc"
#endif

#include "../model.inc"

template< int f_DEC >
struct ModelWrap {

  static const uint model_size = macro_Max( sizeof(Model<0>), sizeof(Model<1>) );

  byte buf[ model_size ];

  uint init( void* r0 ) {
    ((Coroutine*)buf)->coro_init();
    auto Q = ((Model<f_DEC>*)buf);
    return Q->Init(r0);
  }

  void quit( void ) {
    auto Q = ((Model<f_DEC>*)buf);
    Q->Quit();
  }

  void* save( void ) {
    auto Q = ((Model<f_DEC>*)buf);
    return Q->Save();
  }

  static uint get_object_size( void ) {
    return model_size;
  }

  uint process( void ) {
    auto Q = ((Model<f_DEC>*)buf);
    return Q->coro_call(Q);
  }

  uint getoutsize() { 
    Coroutine& Q = (*(Coroutine*)buf);
    return Q.outptr-Q.outbeg; 
  }

  void addinp( byte* __restrict inp,uint inplen ) { 
    Coroutine& Q = (*(Coroutine*)buf);
    Q.addinp(inp,inplen); 
    if_e0( inplen==0 ) Q.f_quit=1;
  }

  void addout( byte* __restrict out,uint outlen ) { 
    Coroutine& Q = (*(Coroutine*)buf);
    Q.addout(out,outlen); 
  }

};

#include "model.h"

template<> uint C_get_object_size<f_DEC_value>( void ) { return ModelWrap<f_DEC_value>::get_object_size(); }
template<> uint C_init<f_DEC_value>( void* p, void* r0 ) { return ((ModelWrap<f_DEC_value>*)p)->init(r0); }
template<> void C_quit<f_DEC_value>( void* p ) { ((ModelWrap<f_DEC_value>*)p)->quit(); }
template<> uint C_process<f_DEC_value>( void* p ) { return ((ModelWrap<f_DEC_value>*)p)->process(); }
template<> uint C_getoutsize<f_DEC_value>( void* p ) { return ((ModelWrap<f_DEC_value>*)p)->getoutsize(); }
template<> void C_addinp<f_DEC_value>( void* p, byte* inp,uint inplen ) { ((ModelWrap<f_DEC_value>*)p)->addinp(inp,inplen); }
template<> void C_addout<f_DEC_value>( void* p, byte* out,uint outlen ) { ((ModelWrap<f_DEC_value>*)p)->addout(out,outlen); }

template<> void* C_save<f_DEC_value>( void* p ) { return ((ModelWrap<f_DEC_value>*)p)->save(); }
