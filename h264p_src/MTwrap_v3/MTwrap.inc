
#include "win_shim.inc"
//#include "valloc.inc"

#include "bufring2.inc"
#include "dyn_buff.inc"
#include "thread.inc"
#include "thread_pipe.inc"

template< int f_DEC, class Enc_class, typename Args_t = uint >
struct MTwrap : Coroutine {

  //---------------
  void* MidAlloc( size_t s, uint aln=4096 ) { // size_t: th_N*(n1+1)*64K exceeds 4 GiB from 64 threads x 64 MB chunks on
    s = (s+0xFFF) & (~size_t(0xFFF));
    void* r = VirtualAlloc(0, s, MEM_COMMIT, PAGE_READWRITE );
    return r;
  }

  void MidFree( void* p ) {
    VirtualFree( p, 0, MEM_RELEASE );
  }
  //---------------


  template <class T> T Min( T x, T y ) { return (x<y) ? x : y; }
  template <class T> T Max( T x, T y ) { return (x>y) ? x : y; }

  enum{ buf_N=16384 /*, buf1_N=256*4, buf2_N=buf1_N+1, buf3_N=64*4*/ };
  // ring capacities are clamped to buf_N and then incremented, so they reach
  // buf_N+1: the tables need that many entries (with buf_N, chunk_MB >= 1024
  // indexed tbl[16384] = the ring's own a/b/N fields -> wild memcpy)
  enum{ ring_N=buf_N+1 };

  typedef thread_pipe<ring_N,ring_N,Enc_class> codec1_t;

  typedef buf_ring<ring_N> que1_t;
  typedef buf_ring<ring_N> que2_t;

  uint th_N;
  uint chunksize;

  uint InpBufsize;
  uint OutBufsize;

  uint buf1_N, buf1_N1;
  uint buf2_N;

  que1_t* que1;
  que2_t* que2;

  codec1_t* C1; // encwrap

  byte* buf1;
  byte* buf2;

  byte* InpBuf;

  qword c_inp;
  qword c_out;
  qword c_goal;

  dyn_buff B; // buffer queues for loop_dec deinterleaving

  // set by the workers: input slot freed (que1 pop), output pushed (que2 push),
  // thread exited -- every event the main loop can be idle waiting for
  HANDLE ev_main;

  void main_wait( void ) {
    if( ev_main ) WaitForSingleObject( ev_main, INFINITE ); else thread_wait();
  }

  // f_parts: input/output in rounds of variable size (see loop_enc.inc/loop_dec.inc)
  //   encode: the input is a sequence of frames [u64 L][L bytes]; each whole
  //           frame (header included) goes to the next worker in turn
  //   decode: a worker ends a round by yielding 4 (thread_pipe flags that
  //           output buffer); the main loop then moves to the next worker
  // instead of fixed R-byte rounds. Not recorded in the container: the decoder
  // has to be told, like th_N and chunksize.
  uint f_parts;

  uint Init( uint _th_N, uint _chunksize, Args_t MArgs1, uint _f_parts=0 ) {
    uint i,r = 0;
    coro_init();

    B.Init();

    // fixed 64k buffers
    InpBufsize = 1<<16; 
    OutBufsize = 1<<16;

    th_N = _th_N; chunksize = _chunksize; // AlignUp(_chunksize,OutBufsize);
    f_parts = _f_parts;

    // inp/out processed data size
    c_inp=0; c_out=0;

    buf1_N = Max<qword>(1,Min<qword>(buf_N,chunksize/InpBufsize)); // chunksize for input buffers
    buf1_N1= Max<qword>(1,Min<qword>(buf_N,  (2<<20)/InpBufsize)); // min input chunksize
    buf2_N = Max<qword>(1,Min<qword>(buf_N,chunksize/OutBufsize)); // chunksize for output buffers
    chunksize = InpBufsize*buf1_N;

    if( buf1_N1<buf1_N ) buf1_N1=buf1_N;
/*
#define MUL 4
#define DIV 5
if( f_DEC ) buf1_N1=(buf1_N1*MUL)/DIV, buf2_N=(buf2_N*MUL)/DIV;
#undef MUL
#undef DIV
*/
    buf1_N++; buf2_N++; 
    buf1_N1++; 

//printf( "th_N=%i   chunksize=%i f_DEC=%i\n", th_N, chunksize, f_DEC );
//printf( "buf1_N=%i InpBufSize=%i buf1_N1=%i\n", buf1_N, InpBufsize, buf1_N1 );
//printf( "buf2_N=%i OutBufSize=%i\n", buf2_N, OutBufsize );

    // enc/dec thread_pipe sets
    C1 = 0; if( r==0 ) C1 = (codec1_t*)MidAlloc( AlignUp(sizeof(codec1_t),4096)*th_N ); if( C1==0 ) r=1;

    que1=0; if( r==0 ) que1 = (que1_t*)MidAlloc( AlignUp(sizeof(que1_t),32)*th_N, 32 ); if( que1==0 ) r=2;

    que2=0; if( r==0 ) que2 = (que2_t*)MidAlloc( AlignUp(sizeof(que2_t),32)*th_N, 32 ); if( que2==0 ) r=3;

    buf1=0; if( r==0 ) buf1 = (byte*)MidAlloc( size_t(InpBufsize)*buf1_N1*th_N ); if( buf1==0 ) r=3;

    buf2=0; if( r==0 ) buf2 = (byte*)MidAlloc( size_t(OutBufsize)*buf2_N*th_N ); if( buf2==0 ) r=4;

    InpBuf=0; if( f_DEC==1 ) { if( r==0 ) InpBuf = (byte*)MidAlloc( AlignUp(InpBufsize*2+4096,4096) ); if( InpBuf==0 ) r=5; }

    for( i=0; i<th_N; i++ ) { 
      if( r==0 ) que1[i].init(buf1+size_t(InpBufsize)*buf1_N1*i, InpBufsize, 0, 0, buf1_N1 ); 
      if( r==0 ) que2[i].init(buf2+size_t(OutBufsize)*buf2_N*i, OutBufsize, 1-f_DEC, 0, buf2_N );
      if( r==0 ) if( C1[i].Init(MArgs1) ) r=6; 
    }

    ev_main = 0; if( r==0 ) ev_main = CreateEventA( 0, 0, 0, 0 ); // auto-reset; 0 -> Sleep() polling

    if( r==0 )
    for( i=0; i<th_N; i++ ) {
      que1[i].ev_pop  = ev_main;
      que2[i].ev_push = ev_main;
      C1[i].ev_exit   = ev_main;
      // a worker that never starts would never set f_done: the main loop would wait
      // forever. Fail Init instead (threads already started just die with the process)
      if( C1[i].thread_start( que1[i], que2[i] )==0 ) { r=7; break; }
    }

    return r;
  }

  void Quit( void ) {
    uint i;

    if( C1!=0 ) {
      for( i=0; i<th_N; i++ ) {
        C1[i].thread_quit();
        C1[i].Quit();
      }
      MidFree( C1 );
    }

    if( ev_main ) CloseHandle( ev_main ); // after the joins: no worker can still set it
    ev_main = 0;

    if( que1 ) MidFree( que1 );
    if( que2 ) MidFree( que2 );

    if( buf1 ) MidFree( buf1 );
    if( buf2 ) MidFree( buf2 );

    if( InpBuf ) MidFree( InpBuf );

//printf( "!this=%I64X th_N=%i c=%iM dyn_buf=%iM!\n", this, th_N, chunksize>>20, B.c_alloc0>>(20-16) );
    B.Quit();
  }

  qword CalcTotalSize( void ) {
    qword r = c_inp;
    if( (f_DEC==0) && (C1!=0) ) {
      uint i; r=0;
      for( i=0; i<th_N; i++ ) r+=C1[i].c_inp;
    }
    return r;
  }

  void l_memcpy( byte* p, byte* q, uint l ) {
    uint i;
    for( i=0; i<l; i++ ) p[i]=q[i];
  }

  void do_process( void ) {
    buf_ptr* p; buf_ptr tmp;
    qword f_eof=0; 
    qword part_left = 0; // parts mode, encode: bytes of the current frame still to deal out
    byte phdr[8];        // parts mode, encode: frame header (may straddle input buffers)
    qword f_eof_all = (th_N>=64) ? ~0ULL : qword(qword(1)<<th_N)-1ULL; // 1<<64 is UB (x86 shl gives 1 -> 0 -> loop never runs)
    uint f_busy, f_next_j, j=0, n, l, i;
    int InpBuf_l = 0;
    uint f_err = 0; // container error (decode) or failed worker (encode)
    qword f_seen = 0;   // decode: workers that had at least one block
    qword f_eofblk = 0; // decode: workers whose empty (EOF) block was seen

    //StartTimer();

    if( f_DEC==0 ) {
      #include "loop_enc.inc"
    } else {
      #include "loop_dec.inc"
    }

    //outptr=outbeg; // no output on r0

    yield(this,f_err?3:0); // CoroFileProc::processfile() returns it
  }

};
