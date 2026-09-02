
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

  typedef RCio<BLKSIZE,RCNUM> t_RCio;
  ALIGN(4096) t_RCio rcio;

  // The vector renorm pass gathers an idle lane's dword from rcio.zslot at a
  // compile-time index -- which is only a constant if the slot sits where
  // RCio::ZOFS says it does.
  static_assert( offsetof(t_RCio,zslot) - offsetof(t_RCio,tmpbuf) == t_RCio::ZOFS,
                 "RCio::ZOFS does not match the layout" );


  // outbuf has to hold a whole block: the decode loop writes blksize bytes with
  // the unchecked put0 and only calls chkout() once, at the block boundary.
  static_assert( int(outbufsize)>=int(BLKSIZE), "outbufsize must be at least BLKSIZE" );

  // The group's two output bytes as one 16-bit store. Defined here rather than
  // in Lib3 because Lib3 is shared with other projects and syncs separately --
  // this directory must not need a matching Lib3. Both coroutine headers
  // expose outptr (coro3b.inc has it in the union pin[] aliases, so this is
  // the same cursor put0 advances), and put0 is *outptr++ = c with the one
  // chkout per block done by the caller, so the pair is unchecked the same way.
  void put0pair( uint w ) { (word&)*outptr = w; outptr += 2; }

  NOINLINE
  void do_process() {
    uint c,i,j,bit=0,p,blksize,state;

    // The generated coder: lane state as local arrays, methods as macros --
    // sh_v1xN's arrangement, generated from rc.inc by mk_kernel.sh.
    // Locals, not members, so nothing the loop stores through can alias them.
// No vector alignment on the decoder's lane arrays. The decoder touches the lane
// state one scalar at a time and never as a vector, and ALIGN(VECSIZE) on a
// local array forces `and rsp,-32` in the prologue -- which forces a frame
// pointer and takes rbp out of the allocator for the whole function, the
// documented reason model_pass is NOINLINE on the encoder side. Dropping it
// does remove both, and measures 1.4% slower anyway; see rc_config.inc.
#define RC_KALIGN
#include "rc_vecD.inc"
#undef  RC_KALIGN
#if RC_DEC_VRENORM
    uint vwin[RCNUM];   // the renorm pass's cached refill window per lane
    uint vsh[RCNUM];    // ...and the shift each lane took in the last pass
#endif
    static_assert( KERNEL_CONF == RC_KERNEL_CONF,
      "rc_vecD.inc was generated against a different RC_* configuration than "
      "this build. Re-run mk_kernel.sh with the same -D set -- build.sh takes "
      "it as arguments and gc.bat has %defs% for it." );

    // Which array slot holds lane L. The lane number is the format's -- bit i
    // of the block goes to lane i%RCNUM, so lane m*8+j is bit j of byte m of a
    // group -- but where its state sits in the arrays is this loop's own
    // business. The vector renorm pass (RC_DEC_VRENORM) wants the lanes it
    // covers after step j to be a contiguous run, so under it the slots are
    // BIT-MAJOR: slot = j*NB + m, and the lanes finished by step j are slots
    // 0..(j+1)*NB-1. Without the pass the identity, so that build's code is
    // untouched.
#if RC_DEC_VRENORM
#define RC_SLOT( L )  ( ((L)%8)*(RCNUM/8) + (L)/8 )
#else
#define RC_SLOT( L )  (L)
#endif

    // rc_Process with the renorm BEFORE the bit, for the paths that need it
    // that way: the block-length bits and the tail, which do not run through
    // the pass. Under RC_DEC_VRENORM the kernel's rc_Process no longer
    // renormalises on the decode side, so this puts the (branchy, scalar)
    // rc_Renorm back in front of it; otherwise it is rc_Process itself.
#if RC_DEC_VRENORM
#define rc_ProcessR( k, f, b )  { rc_Renorm(k); rc_Process( k, f, b ); }
#else
#define rc_ProcessR( k, f, b )  rc_Process( k, f, b )
#endif

#if RC_DEC_VRENORM
    // The renorm as a pass over W consecutive slots, written as a loop for the
    // compiler to vectorise -- forced to RC_DEC_VRENORM_VF lanes wide, and
    // kept away from the unroller, which would otherwise scalarise it before
    // the vectoriser ever saw it (the pragma's `unroll(disable)`). Every lane
    // does everything: two compares give the shift count 0/8/16, the cursor
    // moves by the byte count, the two shifts, and (1<<sh)-1 -- the mask of
    // the bytes that have to come in, none for an idle lane -- goes to vmsk[]
    // for the refill. rpre has already been applied by rc_Process.
    //
    // The refill is the part that does NOT vectorise, on this box or on any
    // core that serialises gathers (see RC_DEC_VRENORM_GATHER): only ~1.2 of
    // 16 lanes take bytes in a group, so it is done for those lanes alone.
    // The lane mask of the pass comes out as a bit per lane, and the first
    // two set bits are refilled UNCONDITIONALLY -- a zero mask lands on the
    // run's first slot with vmsk==0 and changes nothing -- with a loop for
    // the rare third; P(3 or more) is about 13%, and that is the one branch
    // in the whole renorm, against sixteen 8%-taken ones before.
    //
    // The refill's load at tmpptr+1 is legal for every lane, so the dummy
    // lane's is too: the window is inside the row by RC_SKIP>=4 above and
    // the 0xFF pad below (rc_io.inc).
#define RC_PRAGMA( x ) _Pragma( #x )
#if RC_DEC_VRENORM_VF>1
#define RC_VR_LOOP( W ) RC_PRAGMA( clang loop vectorize(enable) vectorize_width(RC_DEC_VRENORM_VF) \
                                   interleave_count((W)/RC_DEC_VRENORM_VF) unroll(disable) )
#else
    // the probe: the same pass, scalar and fully unrolled
#define RC_VR_LOOP( W ) RC_PRAGMA( clang loop vectorize(disable) unroll(full) )
#endif
#if RC_DEC_VRENORM_PROBE==1
    // the probe: no load from the row at all -- the refill takes the cursor
    // for data. Decodes to garbage; the block structure survives, so the
    // work is the same
#define RC_VR_LOAD( ix, p ) (p)
#define RC_VR_WIN( p )      (p)
#else
#define RC_VR_LOAD( ix, p ) ((uint&)tmpbase[ix])
#define RC_VR_WIN( p )      (*(const uint*)(tmpbase + (p) - 3))   /* the window ending at cursor p */
#endif

#if RC_DEC_VRENORM_GATHER
    // every lane loads, and the compiler gathers -- see RC_DEC_VRENORM_GATHER
#define RC_VRENORM1( k0, W ) {                                               \
      RC_VR_LOOP( W )                                                        \
      for( uint _k=(k0); _k<(k0)+(W); _k++ ) {                               \
        const uint _r  = range[_k];                                          \
        const uint _sh = ( uint(_r<sTOP) + uint(_r<gTOP) )*8;               \
        const uint _p  = tmpptr[_k] - (_sh>>3);                              \
        const int  _ix = (RC_DEC_ZSLOT && _sh==0) ? int(RC_ZOFS) : int(_p+1); \
        const uint _w  = RC_VR_LOAD( _ix, _p );                              \
        tmpptr[_k] = _p;                                                     \
        code[_k]   = (code[_k]<<_sh) | (_w & ((1u<<_sh)-1));                 \
        range[_k]  = _r<<_sh;                                                \
      } }
#else
    // The shift pass, with no memory but the lane arrays: the bytes a lane
    // shifts in come from vwin[], its cached refill window -- the dword
    // ending at its cursor, `vwin == (uint&)tmpbase[tmpptr-3]` on entry to
    // every pass, the invariant misc/dec_renorm.md's shape 9 kept in scalar
    // lane state. The cursor's own byte is the window's top byte, so the top
    // sh bits of the window are exactly what an sh-bit shift wants, and
    // `(win>>16)>>(16-sh)` is 0 for the lane that shifted nothing.
    //
    // Then the lanes that shifted reload their window, and that is the
    // whole refill: a scalar load off the mask, into an array NOTHING on the
    // scalar path reads. That last part is the point. A refill that wrote
    // into code[] wrote to an address known only after the mask -- some
    // forty cycles after the group's last decision -- and every load of
    // code[k] the next steps issue in the meantime sits behind it in the
    // memory disambiguation logic; measured, it cost the same ~50 cycles a
    // group whether the pass had four steps of slack or none. vwin[] is
    // read by the next pass, a group later, and by then the address is old.
#define RC_VRENORM1( k0, W ) {                                               \
      RC_VR_LOOP( W )                                                        \
      for( uint _k=(k0); _k<(k0)+(W); _k++ ) {                               \
        const uint _r  = range[_k];                                          \
        const uint _sh = ( uint(_r<sTOP) + uint(_r<gTOP) )*8;               \
        tmpptr[_k] -= (_sh>>3);                                              \
        code[_k]   = (code[_k]<<_sh) | ((vwin[_k]>>16)>>(16-_sh));           \
        range[_k]  = _r<<_sh;                                                \
        vsh[_k]    = _sh;                                                    \
      }                                                                      \
      RC_VREFILL( k0, W ) }
    // the refill, and the probes that take it apart (RC_DEC_VRENORM_PROBE
    // 3..6: nothing, the mask alone, one slot, two slots -- all garbage)
#if RC_DEC_VRENORM_PROBE==3
#define RC_VREFILL( k0, W ) {}
#elif RC_DEC_VRENORM_PROBE==4
#define RC_VREFILL( k0, W ) { vwin[k0] = RC_LANEMASK( k0, W ); }
#elif RC_DEC_VRENORM_PROBE==5
#define RC_VREFILL( k0, W ) { uint _m = RC_LANEMASK( k0, W ); RC_VREFILL1( k0, W, _m ); }
#elif RC_DEC_VRENORM_PROBE==6
#define RC_VREFILL( k0, W ) { uint _m = RC_LANEMASK( k0, W ); RC_VREFILL1( k0, W, _m ); RC_VREFILL1( k0, W, _m ); }
#else
#define RC_VREFILL( k0, W ) {                                                \
      uint _m = RC_LANEMASK( k0, W );                                        \
      RC_VREFILL1( k0, W, _m ); RC_VREFILL1( k0, W, _m );                    \
      if( _m ) do { RC_VREFILL1( k0, W, _m ); } while( _m ); }
#endif
    // one lane's window reload: the lowest set bit of m, or the run's first
    // slot when there is none -- a reload of a window that did not move,
    // which is the same dword again
    // (tzcnt of 0 is 32 on BMI, and &(W-1) folds that to the first slot;
    // the index into the row is a zero-extended uint, so the -3 lands in
    // the addressing mode rather than in a sign-extension of its own)
#define RC_VREFILL1( k0, W, m ) {                                            \
      const uint _i = (k0) + ( ( (m) ? uint(__builtin_ctz(m)) : 32u ) & ((W)-1) ); \
      (m) &= (m)-1;                                                          \
      vwin[_i] = RC_VR_WIN( tmpptr[_i] ); }
    // the bit-per-lane mask of vsh[k0..k0+W) != 0. One compare and a mask
    // move where the ISA has them; the generic loop is an or-reduction the
    // vectoriser handles less neatly.
#if defined(__AVX512VL__) && RC_DEC_VRENORM>=8
#define RC_LANEMASK( k0, W ) [&]{ uint _r=0;                                 \
      for( uint _c=0; _c<(W); _c+=8 )                                        \
        _r |= uint(_mm256_cmpneq_epi32_mask( _mm256_loadu_si256((const __m256i*)&vsh[(k0)+_c]), \
                                            _mm256_setzero_si256() )) << _c;  \
      return _r; }()
#elif defined(__AVX2__) && RC_DEC_VRENORM>=8
#define RC_LANEMASK( k0, W ) [&]{ uint _r=0;                                 \
      for( uint _c=0; _c<(W); _c+=8 )                                        \
        _r |= uint(_mm256_movemask_ps( _mm256_castsi256_ps( _mm256_cmpeq_epi32( \
                _mm256_loadu_si256((const __m256i*)&vsh[(k0)+_c]), _mm256_setzero_si256() ) ) ) ^ 0xFF) << _c; \
      return _r; }()
#else
#define RC_LANEMASK( k0, W ) [&]{ uint _r=0;                                 \
      for( uint _k=0; _k<(W); _k++ ) _r |= uint(vsh[(k0)+_k]!=0) << _k;      \
      return _r; }()
#endif
#endif  /* RC_DEC_VRENORM_GATHER */
#if RC_DEC_VRENORM_PROBE==2
    // the probe: the pass twice over -- the second finds nothing to shift.
    // What it costs is what a pass costs with no dependency to wait on
#define RC_VRENORM( k0, W )  { RC_VRENORM1( k0, W ) RC_VRENORM1( k0, W ) }
#else
#define RC_VRENORM( k0, W )  RC_VRENORM1( k0, W )
#endif
#define RC_ZOFS  (decltype(rcio)::ZOFS)
    // steps between passes: a pass covers W lanes = W/NB bit positions
    enum { RC_VR_STEPS = RC_DEC_VRENORM/(RCNUM/8) };
#endif

    for( i=0; i<DIM(stats); i++ ) stats[i].Reset();
    rcio.hdr_Init();
    ctx = 1; cty = stats;

    while(1) {

      rcio.rc_Read< Rangecoder_HDR<1> >( (*this) );

      tmpbase = rcio.rows();
      // each cursor starts at the top of its lane's chunk and reads DOWN,
      // running into the 0xFF pad below it past the end of the substream
      for( j=0; j<RCNUM; j++ ) { tmpptr[RC_SLOT(j)] = j*rcio.rowsize + rcio.FF_PADSIZE-1 + rcio.rclen[j]; rc_Init(RC_SLOT(j)); }

      rc_ProcessR( RC_SLOT(0), hSCALE, bit );

      if( bit ) {
        for( j=0,c=0; j<16; j++ ) {
          rc_ProcessR( RC_SLOT(j%RCNUM), hSCALE, bit );
          c = c*2 + (bit>0);
        } blksize=c;
      } else blksize=BLKSIZE;

#if RC_DEC_VRENORM
      // The group loop renormalises AFTER a lane's bit, so it wants every
      // lane renormalised on entry; the block-length bits above left theirs
      // pending. Once a block, scalar, the branchy shape. And the pass's
      // window invariant, seeded.
      for( j=0; j<RCNUM; j++ ) rc_Renorm(j);
#if !RC_DEC_VRENORM_GATHER
      for( j=0; j<RCNUM; j++ ) vwin[j] = RC_VR_WIN( tmpptr[j] );
#endif
#endif

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
                rc_Process( RC_SLOT(m*8+j), p, bt[m] );
                cty[lctx[m]].Update( st[m], bt[m] );
                (lctx[m]*=2) += bt[m];
              }
#if RC_DEC_VRENORM
              // the lanes of the last RC_VR_STEPS bit positions, done for
              // this group, renormalised in one pass; j is a constant here
              if( ((j+1)%RC_VR_STEPS)==0 ) RC_VRENORM( (j+1-RC_VR_STEPS)*NB, RC_DEC_VRENORM );
#endif
            }
            RC_UNROLL
            for( uint m=0; m<NB; m+=2 ) put0pair( (lctx[m]&0xFF) + ((lctx[m+1]&0xFF)<<8) );
          }
          ctx = 1;                      // the tail loops below use the member
        }
        for( i=nb1; i<blksize*8; i++ ) {
          j = RC_SLOT( i % RCNUM );
          state = cty[ctx].Get(); p = t_Counter::P(state);
          rc_ProcessR( j, p, bit );
          cty[ctx].Update( state, bit );
          (ctx*=2) += bit;
          if_e0( i%8==7 ) { c = ctx&0xFF; ctx = 1; put0(c); }
        }
      }
#else
      for( i=0; i<blksize*8; i++ ) {
        j = RC_SLOT( i % RCNUM );
        state = cty[ctx].Get(); p = t_Counter::P(state);
        rc_ProcessR( j, p, bit );
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
