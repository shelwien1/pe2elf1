
//  The scalar rangecoder of sh_v1xN_s.cpp, moved here so it can be included
//  twice: once with RC_CARRYLESS=1 (the fast path, which drops carry
//  propagation and only raises a flag) and once with RC_CARRYLESS=0 (the
//  carry-propagating twin the encoder falls back to when the fast path loses a
//  carry). The includer sets RC_CARRYLESS and #defines Rangecoder to the class
//  name it wants -- see model.inc -- so this file is written against the bare
//  name and has no include guard on purpose.
//
//  Differences from sh_v1xN_s.cpp:
//   - f_DEC is a template parameter instead of a member, because model0.cpp and
//     model1.cpp are separate translation units specialised on it.
//   - RC_IO (rc_io.inc) writes into a per-lane byte buffer instead of a FILE*.
//   - a lost carry is reported through carry_lost() instead of being fatal:
//     here it is recoverable, model0.inc re-encodes the block with the twin.
//
//  Needs: SCALElog (FSM.cpp), the knobs in rc_config.inc, RC_IO (rc_io.inc).

#if RC_RANGE64
 typedef qword rangetype;
 #define RC_RANGE_INIT (1ULL<<32)
#else
 typedef uint  rangetype;
 #define RC_RANGE_INIT 0xFFFFFFFF
#endif

template< int f_DEC >
struct Rangecoder : RC_IO< Rangecoder<f_DEC> > {

  typedef RC_IO< Rangecoder<f_DEC> > t_IO;
  using t_IO::get;
  using t_IO::put;

  static const int  RC_f_DEC  = f_DEC;    // so RangecoderN can read it back

  static const uint CODBYTES = RC_CODBYTES;
  static const uint LOWBYTES = RC_LOWBYTES;
  static const uint LOWBITS  = LOWBYTES*8;
  static const uint sTOP     = 0x01000000U;
  static const uint gTOP     = 0x00010000U;
  // ~0>>(64-LOWBITS) rather than (1<<LOWBITS)-1: nothing to get wrong at 64.
  static constexpr qword LOW_MASK = qword(-1) >> (64-LOWBITS);
  static constexpr qword LOW_MAX  = LOW_MASK+1;

#if RC_LOWSPLIT
  uint  lowl, lowh;   // low accumulator, LOWBITS wide, as (lowh<<32)|lowl
#else
  qword lowc;    // low accumulator, LOWBITS wide
#endif
  uint  rpre;    // the unit out of rc_Renorm; scaled to the low delta in
                 // rc_Process, which is what the next ShiftLow applies
  uint  Carry;   // carry out of low, pending until the next byte is emitted
  uint  code;
  rangetype range;
  uint  FFNum;
  uint  Cache;
#if RC_STATS
  uint  n_carry; // isn't expected to be used on enough data to overflow
#endif
#if RC_VECOUT && RC_SCATTER
  // ShiftLow's store, staged rather than performed: the dword, the tmpbase
  // offset it belongs at, and the renorm count that says whether it is a
  // store at all. rc_Flush commits one; model0.inc commits a whole group
  // with one scatter. See RC_SCATTER in rc_config.inc.
  uint  stcl, stad, stn;
  // so the compile can tell it is looking at a kernel generated for the same
  // RC_SCATTER it was built with -- model0.inc static_asserts on it
  enum { SCATTER_GEN = 1 };
#elif RC_VECOUT
  enum { SCATTER_GEN = 0 };
#endif

#if RC_STATS
  #define RC_CARRY_COUNT(c) (n_carry+=(c))
#else
  #define RC_CARRY_COUNT(c) ((void)0)
#endif

  // Carries that escaped the low accumulator over this substream. Non-zero
  // means the output is not decodable and the block has to be re-coded with
  // the carry-propagating twin -- which is exactly what FFNum counts here,
  // since carryless ShiftLow has no FF run to track and reuses it as the flag.
#if RC_CARRYLESS
  uint carry_lost( void ) const { return FFNum; }
#else
  uint carry_lost( void ) const { return 0; }
#endif

  // low += rpre, returning the carry out of the LOWBITS-wide accumulator.
  // The LOWBYTES==8 / <8 split is sh_v1xN.inc's: at 8 there is no bit left to
  // hold the overflow, so the test has to be on the addition itself.
  uint low_Add( void ) {
    uint c = 0;
#if RC_LOWSPLIT
    uint cf = (lowl+rpre < lowl);
    lowl += rpre;
    if( LOWBYTES==8 ) c = (lowh+cf < lowh);
    lowh += cf;
    if( LOWBYTES< 8 ) c = (lowh >= uint(LOW_MAX>>32));
    lowh &= uint(LOW_MASK>>32);          // clears that bit (and is 0 at LB4)
#else
    if( LOWBYTES==8 ) c = (lowc+rpre < lowc);
    lowc += rpre;
    if( LOWBYTES< 8 ) c = (lowc>=LOW_MAX);
    lowc &= LOW_MASK;                    // clears that bit
#endif
    return c;
  }

  uint low_Top( void ) {                 // the byte about to leave
#if RC_LOWSPLIT
    // LOWBITS-40 is where the top byte sits inside lowh; at LOWBYTES==4 lowh
    // holds nothing and the top byte is still in lowl.
    enum { TOPSH = LOWBITS>32 ? LOWBITS-40 : 0 };
    return LOWBITS>32 ? ((lowh>>TOPSH)&0xFF) : ((lowl>>24)&0xFF);
#else
    return uint(lowc>>(LOWBITS-8)) & 0xFF;
#endif
  }

  void low_Shift8( void ) {
#if RC_LOWSPLIT
    lowh = (lowh<<8) | (lowl>>24);       // constant count: no 32-sh trap here
    lowl <<= 8;
    lowh &= uint(LOW_MASK>>32);
#else
    lowc = (lowc<<8) & LOW_MASK;
#endif
  }

  qword low_Get( void ) {
#if RC_LOWSPLIT
    return (qword(lowh)<<32) + lowl;
#else
    return lowc;
#endif
  }

  void low_Init( void ) {
#if RC_LOWSPLIT
    lowl = 0; lowh = 0;
#else
    lowc = 0;
#endif
  }

#if !RC_VECOUT

  void ShiftLow( uint n ) {

    uint c = low_Add();
    rpre = 0;
    RC_CARRY_COUNT(c);

#if RC_CARRYLESS

    FFNum += c;   // ShiftLowN: "use as flag"

    for( ; n!=0; n-- ) {
      put( byte(low_Top()) );
      low_Shift8();
    }

#else

    Carry |= c;

    // At most one carry can be pending: the sub-intervals nest, so everything
    // added between two emitted bytes sums to less than one range, i.e. < 2^32.

    for( ; n!=0; n-- ) {
      uint cl = low_Top();
      low_Shift8();

      if_e1( (cl!=0xFF) || Carry ) {
        if( Cache!=uint(-1) ) put( byte(Cache+Carry) );
        for( ; FFNum!=0; FFNum-- ) put( byte(Carry-1) );
        Cache = cl;
        Carry = 0;
      } else FFNum++;
    }

#endif
  }

  void ShiftCode( uint n ) {
    code -= rpre; rpre = 0;   // mirrors ShiftLow's deferred update
    for( ; n!=0; n-- ) code = (code<<8) + get();
  }

  // sh_v1xN_macro.inc's rc_Renorm2, minus its trailing rpre assignment.
  // The counted 0/1/2-byte shift is exact for the binary coder because its
  // unit is range>>15 >= 2^9, so a step cannot take range below 2^8. The tail
  // loop only matters for the multi-symbol entry points below, where the unit
  // is range/totFreq and can be much smaller.
  void rc_Renorm( void ) {
    uint n = (range<sTOP) + (range<gTOP);
    if( f_DEC==0 ) ShiftLow(n); else ShiftCode(n);
    range <<= n*8;
#if RC_RENORM_TAIL
    while( range<sTOP ) {
      if( f_DEC==0 ) ShiftLow(1); else ShiftCode(1);
      range <<= 8;
    }
#endif
  }

#else // RC_VECOUT ---------------------------------------------------------
  //
  //  The vector-shape twins. Reachable only through mk_kernel.sh, which
  //  resolves this file with RC_VECOUT=1 (and RC_CARRYLESS=1) and turns the
  //  lane state into [RCNUM] arrays -- see rc_soa.pl. They address the
  //  output through the flat cursor the generated prelude declares
  //  (tmpbase + tmpptr), never through RC_IO, and inline the low_* helpers
  //  because a value-returning function cannot become a macro. A scalar
  //  build never compiles this branch, and the arithmetic is line for line
  //  the scalar coder's -- t.sh checks the streams byte for byte.

  // sh_v1xN's ShiftLowN: the substream is written BACKWARDS (tmpptr
  // decrements), which is what makes the store free -- the plain
  // little-endian store of low's top bytes lands them in stream order
  // with no byte extraction or swapping. cl is low aligned so low's top
  // byte is cl's top byte (zero or one shift per config); the store
  // stages sizeof(cl) bytes ending at tmpptr+1 and the counted decrement
  // commits 0..2 of them, so a short shift is simply overwritten by the
  // next store. The per-byte loop is what scalarised the sweep in the
  // OpenCL and ispc ports; the branch was the cost, never the store.
  void ShiftLow( uint n ) {
    uint sh = n*8;
#if RC_LOWSPLIT
    uint _cf = (lowl+rpre < lowl);
    lowl += rpre;
#if RC_LOWBYTES==8
    uint _cy = (lowh+_cf < lowh);
    lowh += _cf;
#else
    lowh += _cf;
    uint _cy = (lowh > uint(LOW_MASK>>32));
    lowh &= uint(LOW_MASK>>32);
#endif
#else
#if RC_LOWBYTES==8
    qword _t = lowc + rpre;
    uint _cy = (_t < lowc);
    lowc = _t;
#else
    qword _t = lowc + rpre;
    uint _cy = (_t > LOW_MASK);
    lowc = _t & LOW_MASK;
#endif
#endif
    rpre = 0;
    RC_CARRY_COUNT(_cy);
    FFNum += _cy;   // ShiftLowN: "use as flag"
    // cl's top byte lands at tmpptr+1, the next at tmpptr, the rest
    // below as staging that later stores overwrite
#if RC_LOWSPLIT
#if RC_LOWBYTES==8
    uint cl = lowh;
#elif RC_LOWBYTES>=6
    uint cl = lowh << (64-LOWBITS);
#elif RC_LOWBYTES==5
    uint cl = (lowh<<24) | (lowl>>8);
#else
    uint cl = lowl;
#endif
#if RC_SCATTER
    // n==0 stages without committing: the cursor does not move, so the next
    // store of this lane writes this exact window again and every byte here
    // is overwritten before anything reads the row. RC_SCATTER_SKIP is what
    // acts on it -- and it is the common case, the coder emits about 0.08
    // bytes per coded bit.
    stcl = cl; stad = tmpptr-2; stn = n;
#else
    (uint&)tmpbase[tmpptr-2] = cl;
#endif
#else
#if RC_LOWBYTES==8
    qword cl = lowc;
    (qword&)tmpbase[tmpptr-6] = cl;
#else
    uint cl = uint(lowc >> (LOWBITS-32));
    (uint&)tmpbase[tmpptr-2] = cl;
#endif
#endif
    tmpptr -= n;
#if RC_LOWSPLIT
    // sh is 0, 8 or 16, so splitting the shift keeps both counts <= 16 and
    // the sh==0 case falls out as 0 on its own -- (x>>16)>>16 is a defined
    // pair of 32-bit shifts where x>>32 is not. That drops the vpcmpeqd and
    // vpandn the ternary compiled to in each half of the sweep.
    //
    // x86 SIMD and NEON would give the same answer from the single shift
    // `lowl >> (32-sh)`, because their variable shift saturates to zero past
    // the lane width -- but scalar x86 masks the count and OpenCL defines the
    // shift as `x >> (n & 31)`, so that form is target-specific and undefined
    // in C. RC_SHIFT_SAT in rc_config.inc has it, with the measurements: it
    // is slower here, because clang will not fold the poison into the one
    // instruction the hardware would allow.
#if RC_SHIFT_SAT
    uint _hi = lowl >> (32-sh);
#else
    uint _hi = (lowl >> 16) >> (16-sh);
#endif
    lowh = ((lowh<<sh) | _hi) & uint(LOW_MASK>>32);
    lowl <<= sh;
#else
    lowc = (lowc<<sh) & LOW_MASK;
#endif
  }

#if RC_SCATTER
  // Commit the store ShiftLow staged. One lane at a time -- for the paths that
  // code one lane at a time; the sweep uses model0.inc's scatter instead.
  void rc_Flush( void ) {
    (uint&)tmpbase[stad] = stcl;
  }
#endif

  // Encode: the counted shift. Decode: the branchy nested refill with the
  // cursor touched only inside it -- ~91% of bits pass two predicted tests
  // and do nothing else, the shape rc_split_ispc_v1.md section 9.5 measured.
  void rc_Renorm( void ) {
    if( f_DEC==0 ) {
      uint n = (range<sTOP) + (range<gTOP);
      ShiftLow( n );
      range <<= n*8;
    }
    if( f_DEC==1 ) {
      code -= rpre;
      rpre = 0;
      if_e0( range<sTOP ) {
        uint _q = tmpptr;
        if_e0( range<gTOP ) {
          range <<= 16;
          code = (code<<16) + (uint(tmpbase[_q])<<8) + tmpbase[_q-1];
          tmpptr = _q-2;
        } else {
          range <<= 8;
          code = (code<<8) + tmpbase[_q];
          tmpptr = _q-1;
        }
      }
    }
  }

#endif // RC_VECOUT

  // sh_v1xN_macro.inc's rc_Process, unchanged apart from dropping rcidx (the
  // lane index now selects the instance, not the array slot). freq is P(bit==0)
  // scaled to SCALE. This is the entry point the model uses.
  INLINE
  void rc_Process( uint freq, uint& _bit ) {
    uint _b = _bit;   // not `bit`: as a macro, callers pass a variable named
                      // bit, and `uint bit = bit;` would self-initialize

    rc_Renorm();
    rpre = range>>SCALElog;

    rpre *= freq;

    if( f_DEC ) _b = (code>=rpre);

    range -= rpre;

    range = _b ? range : rpre;
    rpre &= -_b;

    _bit = _b;
  }

  // Decode needs the unit before it can pick the symbol, so this is where the
  // decoder renormalises. No clamp is needed: the top symbol gets rpre*freq
  // like every other, so the leftover range above rpre*totFreq is never coded
  // into and code/rpre stays below totFreq.
  //
  // Unused by this coder's binary model -- kept because it is half of the
  // multi-symbol interface sh_v1xN_s.cpp built on top of the binary core, and
  // dropping it would make rc.inc no longer be that file's Rangecoder.
  uint rc_GetFreq( uint totFreq ) {
    rc_Renorm();
    rpre = range/totFreq;
    return code/rpre;
  }

  // The binary version, unrolled to a general alphabet:
  //
  //     rpre *= freq;                    rpre  = unit*cumFreq  (the low delta)
  //     range -= rpre;                   range = unit*freq, or the remainder
  //     range = bit ? range : rpre;      for the topmost symbol
  //     rpre &= -bit;
  //
  // Encode renormalises here; decode already did it in rc_GetFreq, and calling
  // it twice would apply the pending rpre twice.
  void rc_Process( uint cumFreq, uint freq, uint totFreq ) {

    if( f_DEC==0 ) { rc_Renorm(); rpre = range/totFreq; }

    uint tmp = rpre*cumFreq;

    range = rpre*freq;

    rpre = tmp;   // low += tmp / code -= tmp, deferred to the next renorm
  }

  void rc_Init( void ) {
    low_Init();
    rpre  = 0;
    Carry = 0;
    FFNum = 0;
    Cache = -1;
#if RC_STATS
    n_carry = 0;
#endif
    range = RC_RANGE_INIT;
#if RC_VECOUT && RC_SCATTER
    // where the first ShiftLow's store would land anyway: the row's zero
    // prefix, which rc_Write drops. Keeps a flush before the first ShiftLow
    // -- there is none today -- from writing somewhere it should not.
    stcl = 0; stad = tmpptr-2; stn = 0;
#endif
    if( f_DEC==1 ) { code = 0; for( uint i=0; i<CODBYTES; i++ ) code = (code<<8) + get(); }
  }

#if RC_VECOUT
  // the carryless minimal flush, low_Get inlined
  void rc_Quit( void ) {
    if( f_DEC==0 ) {
      rc_Renorm();
#if RC_SCATTER
      // before the put1 run below, which overwrites the top of the staged
      // dword -- exactly what it does to the direct store
      rc_Flush();
#endif

      uint i, n = LOWBYTES;
#if RC_LOWSPLIT
      qword llow = (qword(lowh)<<32) + lowl;
#else
      qword llow = lowc;
#endif
      qword high = llow + range;
      uint  hic  = (high<llow);

      for( i=0; i<LOWBYTES; i++ ) {
        qword mask = ((1ULL<<((i+1)*4))<<((i+1)*4))-1;
        if( hic || ((llow|mask)<high) ) llow|=mask, --n;
      }

      // the cursor sits below ShiftLow's staging pair; the next stream
      // byte belongs one up (sh_v1xN_macro.inc's ++tmpptr before put1)
      tmpptr += 1;
      for( i=0; i<n; i++ ) put( byte(llow>>(LOWBITS-8-i*8)) );
    }
  }
#else
  void rc_Quit( void ) {
    if( f_DEC==0 ) {
      rc_Renorm();   // applies the last pending rpre

      uint i, n = LOWBYTES;

      // Carry=0: cache   .. FF x FFNum .. low
      // Carry=1: cache+1 .. 00 x FFNum .. low
      qword llow = low_Get();
      qword high = llow + range;
      uint  hic  = (high<llow);   // llow+range can leave the qword at LOWBYTES==8

      for( i=0; i<LOWBYTES; i++ ) {
        qword mask = ((1ULL<<((i+1)*4))<<((i+1)*4))-1;
        if( hic || ((llow|mask)<high) ) llow|=mask, --n;
      }

#if RC_CARRYLESS

      // Cache and the FF run do not exist here; ShiftLow already wrote those
      // bytes, so only the tail of low is left. Which also means the trailing
      // FFs that the carry-mode flush below trims are already committed.
      for( i=0; i<n; i++ ) put( byte(llow>>(LOWBITS-8-i*8)) );

#if RC_CARRYLESS_WARN
      if( FFNum ) fprintf( stderr,
        "rc.inc: RC_CARRYLESS lost %u carr%s -- substream is NOT decodable.\n",
        FFNum, FFNum==1?"y":"ies" );
#endif

#else

      if( (n==0) && (Carry==0) ) {
        // Everything still to write is one run of 0xFF: the Cache byte, when
        // it is one, and FFNum more. The decoder reads 0xFF past the end of a
        // substream, so the tail of that run need not be written -- but
        // rc_Read's padding is RC_FF_PADSIZE and the run is as long as the
        // data makes it, so only RC_FF_TRIM of it may go. sh_v1xN_c.inc and
        // sh_v1xN_s.cpp drop the whole run; on a block whose flush leaves more
        // than the padding behind, that decodes to the wrong bytes.
        uint run  = FFNum + ((Cache!=uint(-1)) && (Cache==0xFF));
        uint keep = (run>RC_FF_TRIM) ? run-RC_FF_TRIM : 0;
        if( (Cache!=uint(-1)) && (Cache!=0xFF) ) put( byte(Cache) );
        for( i=0; i<keep; i++ ) put( byte(0xFF) );
      } else {
        if( Cache!=uint(-1) ) put( byte(Cache+Carry) );
        for( i=0; i<FFNum; i++ ) put( byte(0xFF+Carry) );
        for( i=0; i<n; i++ ) put( byte(llow>>(LOWBITS-8-i*8)) );
      }

#endif

#if RC_STATS
      fprintf( stderr, "LOWBYTES=%u  carries=%u\n", LOWBYTES, n_carry );
#endif
    }
  }
#endif // RC_VECOUT

};

#undef RC_CARRY_COUNT
#undef RC_RANGE_INIT
