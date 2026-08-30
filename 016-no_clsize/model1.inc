
template<>
struct Model<1> : Coroutine {
  static const uint f_DEC=1;

  // The counter shape is this model's own choice -- see model0.inc, which
  // picks a different one. The decoder is always fused: Counter2 carries
  // {p,state} in one dword, so Get() is the whole load and P() is a mask.
  typedef SelCounter< 1 >::t t_Counter;

  // o0 aligned bit context, and the counter table it indexes. cty is NOT
  // biased by -1: the bias would cost nothing on its own, but it makes
  // cty[2*c] land at an odd index. Index 0 goes unused instead; contexts run
  // 1..255, which is why stats[] is 0x200 entries.
  uint ctx;
  t_Counter* __restrict cty;

  // The model_api.inc entry points. Init loads the counter state machine
  // named on the command line; both ends need the same one.
  uint Init( void* p ) {
    uint r = 0;
    if( p!=0 ) {
      FILE* h = fopen( (char*)p, "rb" ); if( h==0 ) r=4; else {
        if( FSM[0].Load(h) ) r=5;
        fclose(h);
      }
    }
    return r;
  }
  void* Save( void* r0 = 0 ) { return 0; }
  uint  Quit( void ) { return 0; }

  ALIGN(64) t_Counter stats[0x200];   // order0

  ALIGN(4096) RCio<BLKSIZE,RCNUM> rcio;


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

    // The generated coder: lane state as local arrays, methods as macros --
    // sh_v1xN's arrangement, from the same rc.inc as the scalar reference.
    // Locals, not members, so nothing the loop stores through can alias them.
// RC_DEC_ALIGN=0 drops the vector alignment here. The decoder touches the lane
// state one scalar at a time and never as a vector, and ALIGN(VECSIZE) on a
// local array forces `and rsp,-32` in the prologue -- which forces a frame
// pointer and takes rbp out of the allocator for the whole function, the
// documented reason model_pass is NOINLINE on the encoder side. Dropping it
// does remove both, and measures 1.4% slower anyway; see rc_config.inc.
#define RC_KALIGN
#include "rc_vecD.inc"
#undef  RC_KALIGN
    static_assert( KERNEL_CONF == RC_KERNEL_CONF,
      "rc_vecD.inc was generated against a different RC_* configuration than "
      "this build. Re-run mk_kernel.sh with the same -D set -- build.sh takes "
      "it as arguments and gc.bat has %defs% for it." );

    for( i=0; i<DIM(stats); i++ ) stats[i].Reset();
    ctx = 1; cty = stats;

    while(1) {

      rcio.rc_Read( (*this) );

      tmpbase = rcio.rows();
      // each cursor starts at the top of its lane's chunk and reads DOWN,
      // running into the 0xFF pad below it past the end of the substream
      for( j=0; j<RCNUM; j++ ) { tmpptr[j] = j*rcio.rowsize + rcio.FF_PADSIZE-1 + rcio.rclen[j]; rc_Init(j); }

      rc_Process( 0, hSCALE, bit );

      if( bit ) {
        for( j=0,c=0; j<16; j++ ) {
          rc_Process( j%RCNUM, hSCALE, bit );
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
        // The wavefront (speed_plan_next.md 1.1). A group is RCNUM/8 output
        // bytes -- lanes 0..7 are one, 8..15 the next -- and the bit-context
        // chain restarts at every byte and carries nothing across -- the byte
        // is `c = ctx&0xFF` and the context resets to 1. The only cross-byte edge is a RAW through
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
                st[m] = cty[lctx[m]].Get(); p = t_Counter::P(st[m]);
                rc_Process( m*8+j, p, bt[m] );
                cty[lctx[m]].Update( st[m], bt[m] );
                (lctx[m]*=2) += bt[m];
              }
            }
            RC_UNROLL
            for( uint m=0; m<NB; m+=2 ) put0pair( (lctx[m]&0xFF) + ((lctx[m+1]&0xFF)<<8) );
          }
          ctx = 1;                      // the tail loops below use the member
        }
        for( i=nb1; i<blksize*8; i++ ) {
          j = i % RCNUM;
          state = cty[ctx].Get(); p = t_Counter::P(state);
          rc_Process( j, p, bit );
          cty[ctx].Update( state, bit );
          (ctx*=2) += bit;
          if_e0( i%8==7 ) { c = ctx&0xFF; ctx = 1; put0(c); }
        }
      }
#else
      for( i=0; i<blksize*8; i++ ) {
        j = i % RCNUM;
        state = cty[ctx].Get(); p = t_Counter::P(state);
        rc_Process( j, p, bit );
        cty[ctx].Update( state, bit );
        (ctx*=2) += bit;
        if_e0( i%8==7 ) { c = ctx&0xFF; ctx = 1; put0(c); }
      }
#endif

      if( blksize<BLKSIZE ) break;

      chkout();
    }

    yield(this,0);
  }

};
