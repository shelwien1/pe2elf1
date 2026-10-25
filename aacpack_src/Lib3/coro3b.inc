
//#define CORO_NOASM 1

#if defined(_MSC_VER) && !defined(__clang__)
#pragma runtime_checks( "scu", off )
#pragma check_stack(off)
#pragma strict_gs_check(off)
#endif

#if ((defined __GNUC__) || (defined __INTEL_COMPILER) || (defined __clang__)) && (!defined CORO_NOASM)
  #define _INC_SETJMP
  #ifdef X64
    // (clang too: its inline-asm version, which kept neither rbp nor rbx,
    // crashed where frames keep their pointer -- -O0, -fno-omit-frame-pointer)
    #include "coro3_setjmp_x64.h"
  #elif defined(__i386__)
    #include "coro3_setjmp_x32.h"
  #else
    #error "Lib3's coroutines: x86-64 (or i386) only"
  #endif
#else 
  #ifndef CORO_NOASM
  #define CORO_NOASM 1
  #endif
  #include <setjmp.h>
#endif

struct Coroutine;
static void yield( void* p, int value );

#include "coro3_pin.inc"

static uint coro_call0( Coroutine* that );
static void call_do_process0( Coroutine* that );

struct Coroutine {

  union {
    struct {
    byte* inpptr;
    byte* inpbeg;
    byte* inpend;
    uint  inp_f_EOF;
    uint  inp_pad_x64_;

    byte* outptr;
    byte* outbeg;
    byte* outend;
    uint  out_f_EOF;
    uint  out_pad_x64_;
    };
    coro3_pin pin[4];
  };

  volatile uint  state;

  ALIGN(32) jmp_buf PointA;
  ALIGN(32) jmp_buf PointB;

  ALIGN(8)
  volatile char* stkptrH;
  volatile char* stkptrL; // remembered _sp value for this instance

  typedef void (Coroutine::*t_do_process)( void );
  t_do_process p_do_process;

  enum{ STKPAD=4096*64+24 }; // coroutine stack size (an -O0 build's yields copy out 100 KB and more)
  enum{ STKPAD0=1<<18 }; // stack padding from frontend to coroutine

  ALIGN(8) byte stk[STKPAD];

  void coro_init( void ) {
    state = 0; 
    for( int i=0; i<DIM(pin); i++ ) pin[i].pin_init( this, 1+i );
  }

  template <typename T> 
  INLINE
  uint coro_call( T* that ) {
    p_do_process = (t_do_process)&T::do_process;
    return coro_call0(that);
  }

//---------------------

  void chkinp( void ) { pin[0].chkinp(); }
  void chkout( uint d=0 ) { pin[1].chkout(d); }

  uint getoutsize() { return pin[1].getoutsize(); } //-V110

  void addinp( byte* inp,uint inplen ) { pin[0].addinp(inp,inplen); }
  void addout( byte* out,uint outlen ) { pin[1].addout(out,outlen); }

};


NOINLINE
static void yield( void* p, int value ) { 
  Coroutine& q = *(Coroutine*)p;
  char curtmp; q.stkptrL=(char*)((uintptr_t)&curtmp-16);  // (below this frame's locals)
  if( setjmp(q.PointB)==0 ) { 
    q.state=value; 
    memcpy( q.stk, (char*)q.stkptrL, q.stkptrH-q.stkptrL );
    longjmp(q.PointA,1); 
    __assume(0);
  }
}

NOINLINE
static uint coro_call0( Coroutine* that ) {
  if_e1( setjmp(that->PointA)==0 ) {
    if_e1( that->state ) { // calls usually take this path, since other runs only on init
      memcpy( (char*)that->stkptrL, that->stk, that->stkptrH-that->stkptrL );
      longjmp(that->PointB,1); 
      __assume(0);
    }
    call_do_process0(that);
    __assume(0);
  }
  return that->state;
}


NOINLINE
static void call_do_process0( Coroutine* that ) {
  // call_do_process0 needs to be an actual separate function to allocate stack pad in its frame
  byte stktmp[Coroutine::STKPAD0]; 
  that->stkptrH = ((char*)stktmp);

  // do_process also needs a separate stack frame, to avoid merging stktmp into it, but ptr call is ok
  (that->*(that->p_do_process))();

  // do_process ends with yield(0) (can't normally return to changed frontend stack)
  // so tell compiler that this point can't be reached
  __assume(0);
}


//#include "coro3_init.inc"

