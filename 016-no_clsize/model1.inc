
template<>
struct Model<1> : Coroutine, PredictorD {
  static const uint f_DEC=1;

  ALIGN(64) t_Counter stats[0x200];   // order0

  ALIGN(4096) RCio<BLKSIZE,RCNUM> rcio;

#if !RC_VEC
  // the scalar reference decoder; the vector build carries its lane state as
  // do_process locals instead, from the same rc.inc via rc_vecD.inc
  ALIGN(64) RangecoderN<BLKSIZE,RCNUM, Rangecoder_CY<1> > rc;
#endif

  // outbuf has to hold a whole block: the decode loop writes blksize bytes with
  // the unchecked put0 and only calls chkout() once, at the block boundary.
  static_assert( int(outbufsize)>=int(BLKSIZE), "outbufsize must be at least BLKSIZE" );

  // RC_DEC_PUTW's paired output store. Defined here rather than in Lib3
  // because Lib3 is shared with other projects and syncs separately -- a knob
  // in this directory must not need a matching Lib3. Both coroutine headers
  // expose outptr (coro3b.inc has it in the union pin[] aliases, so this is
  // the same cursor put0 advances), and put0 is *outptr++ = c with the one
  // chkout per block done by the caller, so the pair is unchecked the same way.
  void put0pair( uint w ) { (word&)*outptr = w; outptr += 2; }

  NOINLINE
  void do_process() {
    uint c,i,j,bit=0,p,blksize,state;

#if RC_VEC
    // The generated coder: lane state as local arrays, methods as macros --
    // sh_v1xN's arrangement, from the same rc.inc as the scalar reference.
    // Locals, not members, so nothing the loop stores through can alias them.
// RC_DEC_ALIGN=0 drops the vector alignment here. The decoder touches the lane
// state one scalar at a time and never as a vector, and ALIGN(VECSIZE) on a
// local array forces `and rsp,-32` in the prologue -- which forces a frame
// pointer and takes rbp out of the allocator for the whole function, the
// documented reason model_pass is NOINLINE on the encoder side. Dropping it
// does remove both, and measures 1.4% slower anyway; see rc_config.inc.
#if RC_DEC_ALIGN
#define RC_KALIGN ALIGN(VECSIZE)
#else
#define RC_KALIGN
#endif
#include "rc_vecD.inc"
#undef  RC_KALIGN
    static_assert( KERNEL_CONF == RC_KERNEL_CONF,
      "rc_vecD.inc was generated against a different RC_* configuration than "
      "this build. Re-run mk_kernel.sh with the same -D set -- build.sh takes "
      "it as arguments and gc.bat has %defs% for it." );
#endif

    P_Init(stats,DIM(stats));

    while(1) {

      rcio.rc_Read( (*this) );

#if RC_VEC
      tmpbase = rcio.rows();
      // each cursor starts at the top of its lane's chunk and reads DOWN,
      // running into the 0xFF pad below it past the end of the substream
      for( j=0; j<RCNUM; j++ ) { tmpptr[j] = j*rcio.rowsize + rcio.FF_PADSIZE-1 + rcio.rclen[j]; rc_Init(j); }
#else
      rc.rc_Init( rcio );
#endif

#if RC_VEC
      rc_Process( 0, hSCALE, bit );
#else
      rc.rc_Process( 0, hSCALE, bit );
#endif
      RC_REFILL_ONE( 0 );

      if( bit ) {
        for( j=0,c=0; j<16; j++ ) {
#if RC_VEC
          rc_Process( j%RCNUM, hSCALE, bit );
#else
          rc.rc_Process( j%RCNUM, hSCALE, bit );
#endif
          RC_REFILL_ONE( j%RCNUM );
          c = c*2 + (bit>0);
        } blksize=c;
      } else blksize=BLKSIZE;

      // The group form of the loop: RCNUM is a compile-time constant, so the
      // fully unrolled inner loop makes j a constant in every body -- the
      // lane arrays become fixed addresses and (j&7)==7 a compile-time test,
      // the shape (and the pragma) of the original sh_v1xN decoder. Needs
      // RCNUM%8==0 so byte boundaries land on constant j; other geometries
      // take the plain loop below.
#if (RC_RCNUM%8)==0
      {
        uint nb1 = blksize*8 - (blksize*8)%RCNUM;
#if   RC_DEC_UNROLL==0
#define RC_UNROLL_DEC RC_UNROLL
#elif RC_DEC_UNROLL==1
#define RC_UNROLL_DEC _Pragma("nounroll")
#elif RC_DEC_UNROLL==2
#define RC_UNROLL_DEC _Pragma("unroll 2")
#elif RC_DEC_UNROLL==4
#define RC_UNROLL_DEC _Pragma("unroll 4")
#elif RC_DEC_UNROLL==8
#define RC_UNROLL_DEC _Pragma("unroll 8")
#else
#error "RC_DEC_UNROLL must be 0, 1, 2, 4 or 8"
#endif
#if RC_DEC_WAVE==2
        // The wavefront (speed_plan_next.md 1.1). A group is RCNUM/8 output
        // bytes -- lanes 0..7 are one, 8..15 the next -- and the bit-context
        // chain restarts at every byte and carries nothing across: P_byte is
        // `c = ctx&0xFF; ctx = 1`. The only cross-byte edge is a RAW through
        // cty[], and at bit k the context is in [2^k, 2^(k+1)), so depth-k
        // nodes are touched at bit k and nowhere else.
        //
        // So byte n's step k only has to stay before byte n+1's step k, and
        // stepping the bytes together turns one 8*RCNUM/8-step chain into
        // RCNUM/8 chains of 8 that overlap. Same loads, same updates, same
        // rc_Process calls with j still a compile-time constant: the
        // instruction count does not move, only the dependency graph.
        {
          enum { NB = RCNUM/8 };
          uint lctx[NB], st[NB], bt[NB];
          for( i=0; i<nb1; i+=RCNUM ) {
            RC_UNROLL for( uint m=0; m<NB; m++ ) lctx[m] = 1;
            RC_UNROLL
            for( j=0; j<8; j++ ) {
              RC_UNROLL
              for( uint m=0; m<NB; m++ ) {
                P_S_L( lctx[m], st[m] ); p = P_P(st[m]);
                rc_Process( m*8+j, p, bt[m] );
                P_update_L( lctx[m], st[m], bt[m] );
              }
            }
#if RC_DEC_PUTW
            RC_UNROLL
            for( uint m=0; m<NB; m+=2 ) put0pair( (lctx[m]&0xFF) + ((lctx[m+1]&0xFF)<<8) );
#else
            RC_UNROLL for( uint m=0; m<NB; m++ ) put0( lctx[m]&0xFF );
#endif
            RC_REFILLALL();
          }
          ctx = 1;                      // the tail loops below use the member
        }
#elif RC_DEC_WAVE==1
        // Caller-held context (speed_plan_next.md 1.2), same order as the
        // default: the group's bytes one after another. The member ctx costs
        // 10 memory references per group -- two loads of cty, two of ctx, four
        // stores of ctx and two of the reset constant -- because the store to
        // cty[ctx] may-alias this->ctx and nothing bounds a loaded index. The
        // only difference from RC_DEC_WAVE=2 below is which of the two loops
        // is inner.
        {
          enum { NB = RCNUM/8 };
          for( i=0; i<nb1; i+=RCNUM ) {
            RC_UNROLL
            for( uint m=0; m<NB; m++ ) {
              uint lctx = 1;
              RC_UNROLL
              for( j=0; j<8; j++ ) {
                P_S_L( lctx, state ); p = P_P(state);
                rc_Process( m*8+j, p, bit );
                P_update_L( lctx, state, bit );
              }
              put0( lctx&0xFF );
            }
            RC_REFILLALL();
          }
          ctx = 1;                      // the tail loops below use the member
        }
#elif RC_EAGER_CTY
        // Same loop, with the counter fetched a bit ahead: `state` for bit j+1
        // is one half of the pair loaded at bit j, so the load is off the
        // critical chain and only a select remains on it. j is a compile-time
        // constant here (RC_UNROLL over a constant RCNUM), so both (j&7) tests
        // vanish.
        qword kids = 0; uint state1 = 0;
        for( i=0; i<nb1; i+=RCNUM ) {
          RC_UNROLL_DEC
          for( j=0; j<RCNUM; j++ ) {
            if( (j&7)==0 ) { ctx=1; state1=cty[1].Get(); kids=P_kids(1); }
            state = state1; p=P_P(state);
            rc_Process( j, p, bit );
            cty[ctx].Update( state, bit );
            (ctx*=2)+=bit;
            if( (j&7)!=7 ) { state1 = P_sel(kids,bit); kids = P_kids(ctx); }
            else { P_byte(stats,c); put0(c); }
          }
          RC_REFILLALL();
        }
#else
        {
#if RC_DEC_PUTW
        uint cpair = 0;
#endif
        for( i=0; i<nb1; i+=RCNUM ) {
          RC_UNROLL_DEC
          for( j=0; j<RCNUM; j++ ) {
            P_S(state); p=P_P(state);
#if RC_VEC
            rc_Process( j, p, bit );
#else
            rc.rc_Process( j, p, bit );
#endif
            P_update( state, bit );
#if RC_DEC_PUTW
            // The group's two output bytes go to consecutive addresses, so
            // hold the first across its eight bits and store the pair once.
            // j is constant here, so the (j&8) test folds away.
            if( (j&7)==7 ) { P_byte(stats,c);
              if( (j&8)==0 ) cpair = c; else put0pair( cpair + (c<<8) ); }
#else
            if( (j&7)==7 ) { P_byte(stats,c); put0(c); }
#endif
          }
          RC_REFILLALL();
        }
        }
#endif
        for( i=nb1; i<blksize*8; i++ ) {
          j = i % RCNUM;
          P_S(state); p=P_P(state);
#if RC_VEC
          rc_Process( j, p, bit );
#else
          rc.rc_Process( j, p, bit );
#endif
          RC_REFILL_ONE( j );
          P_update( state, bit );
          if_e0( i%8==7 ) { P_byte(stats,c); put0(c); }
        }
      }
#else
      for( i=0; i<blksize*8; i++ ) {
        j = i % RCNUM;
        P_S(state); p=P_P(state);
#if RC_VEC
        rc_Process( j, p, bit );
#else
        rc.rc_Process( j, p, bit );
#endif
        RC_REFILL_ONE( j );
        P_update( state, bit );
        if_e0( i%8==7 ) { P_byte(stats,c); put0(c); }
      }
#endif

      if( blksize<BLKSIZE ) break;

      chkout();
    }

    yield(this,0);
  }

};
