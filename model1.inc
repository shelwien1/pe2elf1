
template<>
struct Model<1> : Coroutine, Predictor {
  static const uint f_DEC=1;

  ALIGN(64) Counter stats[0x100];   // order0

  ALIGN(4096) RCio<BLKSIZE,RCNUM,1> rcio;

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

      for( i=0; i<blksize*8; i++ ) {
        j = i % RCNUM;

        P_S(state); p=pp[state];

        rc.rc_Process( j, p, bit );

        P_update( state, bit );

        if_e0( i%8==7 ) { P_byte(stats,c); put0(c); }
      }

      if( blksize<BLKSIZE ) break;

      chkout();
    }

    yield(this,0);
  }

};
