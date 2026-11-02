
// 32-bit atomic store of a ring index (a or b).
// The original called _InterlockedExchange((long*)&a,..) directly; that is
// only correct where long is 32-bit (Win32/Win64). On LP64 POSIX long is
// 64-bit and the exchange would also clobber the neighbouring field (b, N).
#ifdef _WIN32
#if 1
//long _InterlockedExchange(long volatile *, long);
#ifndef __INTRIN_H_
extern "C" {
long _InterlockedExchange( volatile long*, long );
}
#else
long _InterlockedExchange1( volatile void* x, long y) {
  return _InterlockedExchange( (volatile long*)x, y );
}
#define _InterlockedExchange _InterlockedExchange1
#endif
#endif
static inline void bufring_xchg( volatile uint& x, uint v ) { _InterlockedExchange( (long*)&x, long(v) ); }
#else
static inline void bufring_xchg( volatile uint& x, uint v ) { __atomic_exchange_n( &x, v, __ATOMIC_SEQ_CST ); }
#endif

// compiler barrier after reading the other side's index, so the slot (len,
// contents) is not read -- or written, for out() -- ahead of it. x86 orders
// the loads themselves; this only stops the compiler from hoisting.
#if defined(__GNUC__) || defined(__clang__)
 #define bufring_acquire() __atomic_thread_fence(__ATOMIC_ACQUIRE)
#else
 #include <intrin.h>
 #define bufring_acquire() _ReadWriteBarrier()
#endif

template< int _bufsize, int _bufN >
struct buf_array {
  enum { bufsize=_bufsize, N=_bufN };
  byte buf[N][bufsize];
};

struct buf_ptr {
  byte* buf;
  uint size; // buffer size
  uint len;  // content size
  uint ofs;  // offset of the relevant part
  union {
    uint flags;
    uint f_eof;
//    struct {
//    uint f_eof:1;
//    };
  };

  void init( void* p, uint l ) {
    buf = (byte*)p;
    size = l;
    len = l;
    ofs = 0;
    flags = 0;
  }

};

template< int _N >
struct buf_ring {
  //enum { N = _N, N2=2*N };
  buf_ptr tbl[_N];

  volatile uint a;
  volatile uint b;
  volatile int N;
  volatile uint f_init;

  // optional wakeups (0 = none): auto-reset events set after every push()/pop(),
  // so the other side can WaitForSingleObject() instead of polling
  HANDLE ev_push; // consumer waits on it for data
  HANDLE ev_pop;  // producer waits on it for a free slot

  template < class base >
  void init( base& X, uint _ofs=0, uint _suf=0 ) {
    uint i;
    for( i=0; i<_N; i++ ) {
      buf_ptr& x = tbl[i];
      x.buf = &X.buf[i][0];
      x.size = X.bufsize - _suf;
      x.len = 0;
      x.ofs = _ofs;
      x.flags = 0;
    }
    a=0; b=0; f_init=1; N=_N;
    ev_push=0; ev_pop=0;
  }

  void init( byte* X, uint bufsize, uint _ofs=0, uint _suf=0, uint __N=0 ) {
    uint i; N=_N;
    if( __N!=0 ) N=__N;
    for( i=0; i<_N; i++ ) {
      buf_ptr& x = tbl[i];
      x.buf = X+i*bufsize; // &X.buf[i][0];
      x.size = bufsize - _suf;
      x.len = 0;
      x.ofs = _ofs;
      x.flags = 0;
    }
    a=0; b=0; f_init=1; 
    ev_push=0; ev_pop=0;
//printf( "bufsize=%i N=%i\n", bufsize, N );
  }

  void init( int X ) { f_init=0; N=_N; }

  void init( void ) { f_init=0; N=_N; }

//  NOINLINE
  void  pop( void ) { 
    uint _a = a; 
    //a = (_a+1)%(N+N);  
    bufring_xchg( a, (_a+1)%(N+N) );
    if( ev_pop ) SetEvent( ev_pop );
  }

/*
  NOINLINE
  void push0( uint _b, uint l, uint _ofs=-1 ) { 
    buf_ptr& p = tbl[_b%N]; 
    p.len = l; 
    if( _ofs!=-1 ) p.ofs=_ofs; 
  }
*/

//  NOINLINE
  void push( uint l, uint _ofs=-1 ) { 
    uint _b = b; 
    //push0( _b, l, _ofs );
    buf_ptr& p = tbl[_b%N]; 
    p.len = l; 
    if( _ofs!=-1 ) p.ofs=_ofs; 
    //b = (_b+1)%(N+N); 
    bufring_xchg( b, (_b+1)%(N+N) );
    if( ev_push ) SetEvent( ev_push );
  }

//  NOINLINE
  buf_ptr* inp( void ) { 
    uint _a = a%(N+N);
    uint _b = b%(N+N);
    bufring_acquire();
    return f_init ? ((_a==_b) ? 0 : &tbl[_a%N]) : 0; 
  }

//  NOINLINE
  buf_ptr* out( void ) { 
    uint _a = a%(N+N);
    uint _b = (b+N)%(N+N);
    bufring_acquire();
    return f_init ? ((_a==_b) ? 0 : &tbl[_b%N]) : 0; 
  }

//  NOINLINE
  uint getinpsize( void ) { return tbl[a%N].len; }

  // queued (pushed, not yet popped) buffers that carry data; only meaningful
  // once the consumer has stopped
  uint pending( void ) {
    uint k, _b = b%(N+N);
    bufring_acquire();
    for( k=a%(N+N); k!=_b; k=(k+1)%(N+N) ) if( tbl[k%N].len > tbl[k%N].ofs ) return 1;
    return 0;
  }
};
