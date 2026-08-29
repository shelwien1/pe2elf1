
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

  NOINLINE
  void do_process() {
    uint c,i,j,bit=0,p,blksize,state;

#if RC_VEC
    // The generated coder: lane state as local arrays, methods as macros --
    // sh_v1xN's arrangement, from the same rc.inc as the scalar reference.
    // Locals, not members, so nothing the loop stores through can alias them.
#include "rc_vecD.inc"
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

      if( bit ) {
        for( j=0,c=0; j<16; j++ ) {
#if RC_VEC
          rc_Process( j%RCNUM, hSCALE, bit );
#else
          rc.rc_Process( j%RCNUM, hSCALE, bit );
#endif
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
#if RC_EAGER_CTY
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
        }
#else
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
            if( (j&7)==7 ) { P_byte(stats,c); put0(c); }
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
