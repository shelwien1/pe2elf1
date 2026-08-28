
template<>
struct Model<1> : Coroutine, Predictor {
  static const uint f_DEC=1;

  ALIGN(64) Counter stats[0x100];   // order0

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
        for( i=0; i<nb1; i+=RCNUM ) {
          RC_UNROLL
          for( j=0; j<RCNUM; j++ ) {
            P_S(state); p=pp[state];
#if RC_VEC
            rc_Process( j, p, bit );
#else
            rc.rc_Process( j, p, bit );
#endif
            P_update( state, bit );
            if( (j&7)==7 ) { P_byte(stats,c); put0(c); }
          }
        }
        for( i=nb1; i<blksize*8; i++ ) {
          j = i % RCNUM;
          P_S(state); p=pp[state];
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
        P_S(state); p=pp[state];
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
