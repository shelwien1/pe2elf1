
template<>
struct Model<1> : Coroutine, Predictor {
  static const uint f_DEC=1;

  ALIGN(64) Counter stats[0x100];   // order0

  ALIGN(4096) RCio<BLKSIZE,RCNUM> rcio;

  // Either class out of rc.inc decodes: RC_CARRYLESS only changes ShiftLow and
  // rc_Quit, which are encode-only. The carry twin is used here to match what
  // the original decoded with.
  ALIGN(64) RangecoderN<BLKSIZE,RCNUM, Rangecoder_CY<1> > rc;

  // outbuf has to hold a whole block: the decode loop writes blksize bytes with
  // the unchecked put0 and only calls chkout() once, at the block boundary.
  static_assert( int(outbufsize)>=int(BLKSIZE), "outbufsize must be at least BLKSIZE" );

  NOINLINE
  void do_process() {
    uint c,i,j,bit=0,p,blksize,state;

    P_Init(stats,DIM(stats));

    while(1) {

      rcio.rc_Read( (*this) );
      rc.rc_Init( rcio );

      rc.rc_Process( 0, hSCALE, bit );

      if( bit ) {
        for( j=0,c=0; j<16; j++ ) {
          rc.rc_Process( j%RCNUM, hSCALE, bit );
          c = c*2 + (bit>0);
        } blksize=c;
      } else blksize=BLKSIZE;

#if RC_DECSPLIT_ON
      // header decoded; the model enters strict i%RCNUM order here, which is
      // what the split's group rotation needs -- see rc_vec.inc
      rc.rc_Bulk( rcio );
#endif

      // The group form of the loop: RCNUM is a compile-time constant, so the
      // fully unrolled inner loop makes j a constant in every body -- rc[j]
      // becomes fixed addresses and (j&7)==7 a compile-time test, the same
      // shape (and the same pragma) that made the original sh_v1xN decoder
      // fast. Needs RCNUM%8==0 so byte boundaries land on constant j; other
      // geometries take the plain loop below.
#if (RC_RCNUM%8)==0
      {
        uint nb1 = blksize*8 - (blksize*8)%RCNUM;
        for( i=0; i<nb1; i+=RCNUM ) {
          RC_UNROLL
          for( j=0; j<RCNUM; j++ ) {
            P_S(state); p=pp[state];
            rc.rc_Process( j, p, bit );
            P_update( state, bit );
            if( (j&7)==7 ) { P_byte(stats,c); put0(c); }
          }
        }
        for( i=nb1; i<blksize*8; i++ ) {
          j = i % RCNUM;
          P_S(state); p=pp[state];
          rc.rc_Process( j, p, bit );
          P_update( state, bit );
          if_e0( i%8==7 ) { P_byte(stats,c); put0(c); }
        }
      }
#else
      for( i=0; i<blksize*8; i++ ) {
        j = i % RCNUM;

        P_S(state); p=pp[state];

        rc.rc_Process( j, p, bit );

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
