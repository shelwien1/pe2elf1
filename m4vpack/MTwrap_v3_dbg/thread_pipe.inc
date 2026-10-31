
template< int bufN1, int bufN2, class coro_t >
struct thread_pipe : thread< thread_pipe<bufN1,bufN2,coro_t> >, coro_t {

  using coro_t::f_quit;
  using coro_t::coro_init;
  using coro_t::coro_call;
  using coro_t::addinp;
  using coro_t::addout;
  using coro_t::getoutsize;
  using coro_t::getinplen;
  using ::thread<thread_pipe>::f_done; // ::-qualified: member thread() below must not change the meaning of an unqualified "thread"
//  using coro_t::outptr;
//  using coro_t::outbeg;

  typedef ::thread< thread_pipe > base;
  typedef buf_ring<bufN1> bufring1_t;
  typedef buf_ring<bufN2> bufring2_t;

  bufring1_t* pinp;
  bufring2_t* pout;
  qword zc_inp; // input bytes
  qword zc_out; // output bytes
  uint inp_left; // input the coroutine left unconsumed when it finished (0 after an error)
  volatile uint f_fail; // the coroutine stopped with an error (r>=3); set before f_done

  // auto-reset events, which the worker waits on (it never polls):
  HANDLE ev_inp; // = finp.ev_push: set by the main thread after it pushes an input buffer
  HANDLE ev_out; // = fout.ev_pop:  set by the main thread after it frees an output slot

  mtdbg_log dlog; // the worker's events (mtdbg.inc), which MTwrap::Quit() writes out

  // 0 when the events or the thread cannot be made
  uint thread_start( bufring1_t& finp, bufring2_t& fout ) {
    coro_init(); f_quit=0; f_done=0; f_fail=0;
    dlog.init();
    pinp = &finp; 
    pout = &fout;
    zc_inp = 0; zc_out = 0; 
    ev_inp = CreateEventA( 0, 0, 0, 0 ); // auto-reset, initially clear
    ev_out = CreateEventA( 0, 0, 0, 0 );
    if( ev_inp==0 || ev_out==0 ) {
      if( ev_inp ) CloseHandle( ev_inp );
      if( ev_out ) CloseHandle( ev_out );
      ev_inp = 0; ev_out = 0;
      return 0;
    }
    finp.ev_push = ev_inp;
    fout.ev_pop  = ev_out;
    return base::thread_start();
  }

  void thread_quit( void ) {
    base::thread_quit();
    if( pinp ) pinp->ev_push = 0;
    if( pout ) pout->ev_pop = 0;
    if( ev_inp ) CloseHandle( ev_inp );
    if( ev_out ) CloseHandle( ev_out );
    ev_inp = 0; ev_out = 0;
  }

  // the ring condition is rechecked after every wakeup; a signal that comes
  // between the check and the wait stays set (auto-reset), so none is lost
  static void wait_ev( HANDLE ev ) {
    WaitForSingleObject( ev, INFINITE );
  }

  void thread( void ) {
    uint r,l,f_eof,d;
    qword t;
    buf_ptr* p_inp;
    buf_ptr* p_out;
    bufring1_t& finp = *pinp;
    bufring2_t& fout = *pout;

    p_inp=0; p_out=0; f_eof=0; r=2;

    do {
      if( r==1 ) {
        if( p_inp ) { InpNextBuf: finp.pop(); p_inp=finp.inp(); }

        if( (p_inp==0) && f_eof ) {

          Quit:
          f_quit=1; // input request after EOF, r0 should only appear after this
          addinp( (byte*)"\xFF", 0 );

        } else {

          if( (p_inp=finp.inp())==0 ) {
            t = mtdbg_now();
            while( (p_inp=finp.inp())==0 ) wait_ev( ev_inp );
            dlog.since( MTDBG_WAIT_IN, t, -1, 0, 0 );
          }
          l = p_inp->len;
          d = p_inp->ofs;
//printf( "inp_%08X: a=%i l=%i l-d=%i\n", this, finp.a, l, l-p_inp->ofs );
          if( p_inp->f_eof ) f_eof=1;
          dlog.at( MTDBG_POP, -1, l>d ? l-d : 0, p_inp->f_eof!=0 );
          if( l<=d ) goto Quit; // no more data, normal addinp won't work
          zc_inp += l-d;
          //if( d>=l ) goto InpNextBuf;
          addinp( p_inp->buf+d, l-d );
//printf( "!addinp %i!\n", l-d );
        }
      } else { // r==2: output full, r==4: end of round (flush, possibly partial), r==0: done
        if( r==0 ) f_quit=1;
        if( p_out ) {
          if( f_quit ) p_out->f_eof=1;
          if( r==4 ) p_out->flags |= 2; // round end: MTwrap's decoder (parts mode) switches workers after this buffer
//printf( "out: b=%i l=%i\n", fout.b, outptr-outbeg );
          l = getoutsize(); //outptr-outbeg;
          d = p_out->ofs;
          zc_out += l+d;
          dlog.at( MTDBG_PUSH, -1, l, p_out->flags );
          fout.push( l+d ); // out len should include the skipped part
        }
        if( r==0 ) break; // after f_quit there can be still output
        OutNextBuf:
        if( (p_out=fout.out())==0 ) {
          t = mtdbg_now();
          while( (p_out=fout.out())==0 ) wait_ev( ev_out );
          dlog.since( MTDBG_WAIT_OUT, t, -1, 0, 0 );
        }
        p_out->flags = 0; // slots are reused: no stale eof/round-end flags

        d = p_out->ofs;
//printf( "!%X: addout %i f_eof=%i f_quit=%i r=%i!\n", uint(this), p_out->size-d, p_out->f_eof, f_quit, r );
        addout( p_out->buf+d, p_out->size-d );
      }

      t = mtdbg_now();
      do r = coro_call(this); while( r==8 ); // c-api timeout thing
      dlog.since( MTDBG_CALL, t, -1, 0, r );

//printf( "!pipe: r=%i inpsize=%i outsize=%i this=%I64X f_quit=%i f_eof=%i !\n", r, inpend-inpptr, outptr-outbeg, this, f_quit, f_eof ); fflush(stdout);
    } while( (r<3) || (r==4) );

    inp_left = (r<3) ? getinplen() : 0;
    f_fail = (r>=3);
    if( p_inp ) finp.pop(); // release the current buffer, so only unread ones stay queued
    dlog.at( MTDBG_EXIT, -1, inp_left, r );
    dlog.add( MTDBG_CPU, mtdbg_now(), mtdbg_cpu(), -1, 0, 0 );

//    printf( "!thread_pipe done. this=%I64X\n", this );
  }

};

/*
struct dummy_processor : coroutine<1> {

  volatile uint f_quit;

  void do_process( void ) {

    uint c;

    while(1) {
      c = get();
      if( f_quit ) break;
      put(c);
    } 

    yield(this,0);
  }

};
*/
