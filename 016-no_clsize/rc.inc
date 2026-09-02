
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
//
//  In order: the lane state, the low accumulator and the renorm that drains
//  it (scalar and vector twins), rc_Process, rc_Init and rc_Quit, and the
//  decoder's vector renorm pass.
//
//  Two rules the whole file is written against, because mk_kernel.sh turns
//  it into the lane-array MACROS the models call (rc_soa.pl, rc_macro.pl):
//  a method that the generated kernel needs must be void -- a macro cannot
//  return a value, and rc_soa.pl drops the ones that do -- and it must be
//  defined before its callers, because a macro cannot be used before it is
//  defined. That, not taste, is what fixes the order below.

 // The CRTP base: RC_IO (rc_io.inc) unless the includer names another. The
 // header coder needs a cursor that runs FORWARDS, straight to and from the
 // coroutine, because its stream is the one whose length is not written down
 // anywhere -- see RC_HDR_IO. Everything below is written against the base's
 // get/put and does not care which it is.
#ifndef RC_IO_BASE
#define RC_IO_BASE RC_IO
#endif

 // A CODBYTES-wide range. At the default 4 this is the whole word; narrower,
 // it is what lets `code` fit a float32 mantissa -- see RC_CODBYTES.
 // as a right shift, not (1<<8*CODBYTES)-1: at CODBYTES==4 that shift is 32,
 // and clang rejects it in a constant expression even on the dead branch.
 #define RC_RANGE_INIT ( 0xFFFFFFFFu >> (8*(4-RC_CODBYTES)) )

 // low's top four bytes, aligned so low's own top byte is this dword's top
 // byte -- what the vector ShiftLow stores, and (through rc_config.inc's
 // RC_ENC_CL) what RC_ENC_RENORM>=2 writes out in place of a local. One
 // cascade, read from both places.
#if RC_LOWBYTES==8
 #define RC_LOW_CL  lowh
#elif RC_LOWBYTES>=6
 #define RC_LOW_CL  (lowh << (64-LOWBITS))
#elif RC_LOWBYTES==5
 #define RC_LOW_CL  ((lowh<<24) | (lowl>>8))
#else
 #define RC_LOW_CL  lowl
#endif

 // Index of the top set bit. A macro, not a function: rc_vecD.inc is this file
 // preprocessed, and its contents land inside do_process, where a definition
 // would be illegal. Never called with 0 -- rc_Quit's operand is low^high and
 // range>0 keeps them apart. (common.inc's log2i is 32-bit.)
#if defined(__GNUC__) || defined(__clang__)
 #define RC_BSR64( x ) uint( 63-__builtin_clzll(x) )
 #define RC_BSR32( x ) uint( 31-__builtin_clz(x) )
#elif defined(_MSC_VER)
 #define RC_BSR64( x ) [&]{ unsigned long i_; _BitScanReverse64(&i_,(x)); return uint(i_); }()
 #define RC_BSR32( x ) [&]{ unsigned long i_; _BitScanReverse  (&i_,(x)); return uint(i_); }()
#else
 #define RC_BSR64( x ) [&]{ qword v_=(x); uint i_=0; while( v_>>=1 ) i_++; return i_; }()
 #define RC_BSR32( x ) [&]{ uint  v_=(x); uint i_=0; while( v_>>=1 ) i_++; return i_; }()
#endif

 // Which array slot holds lane L. The lane number is the format's -- bit i of
 // a block goes to lane i%RCNUM, so lane m*8+j is bit j of byte m of a decode
 // group -- but where its state sits in the arrays is the includer's
 // business. The vector renorm pass (rc_RenormRun below) wants the lanes it
 // covers after step j to be a contiguous run, so under it the slots are
 // BIT-MAJOR: slot = j*NB + m, and the lanes finished by step j are slots
 // 0..(j+1)*NB-1. Without the pass, the identity. Defined here, where every
 // includer of the generated kernel sees it, and once: this file is included
 // twice.
#ifndef RC_SLOT
#if RC_DEC_VRENORM
#define RC_SLOT( L )  ( ((L)%8)*(RC_RCNUM/8) + (L)/8 )
#else
#define RC_SLOT( L )  (L)
#endif
#endif

template< int f_DEC >
struct Rangecoder : RC_IO_BASE< Rangecoder<f_DEC> > {

  typedef RC_IO_BASE< Rangecoder<f_DEC> > t_IO;
  using t_IO::get;
  using t_IO::put;

  static const int  RC_f_DEC  = f_DEC;    // so RangecoderN can read it back

  // rc_Quit's aligned flush rounds low up, which can carry out of the
  // accumulator, and only the carry-propagating ShiftLow can carry it.
  // Not for the kernel: rc_soa.pl lifts this region into do_process, where
  // there is no t_IO -- and no vector build sets QUIT_ALIGN in any case.
#if !RC_VECOUT
  static_assert( !(RC_CARRYLESS) || !t_IO::QUIT_ALIGN,
    "RC_CARRYLESS with QUIT_ALIGN: the aligned flush's round-up has nowhere "
    "to put a carry out of low" );
#endif

  static const uint CODBYTES = RC_CODBYTES;
  static const uint LOWBYTES = RC_LOWBYTES;
  static const uint LOWBITS  = LOWBYTES*8;
  // The renorm thresholds are the top two byte boundaries of a CODBYTES-wide
  // range: at CODBYTES=4 these are the 1<<24 and 1<<16 the shapes in
  // rc_config.inc are written against, and they track CODBYTES below that.
  static const uint sTOP     = 1u << (8*(CODBYTES-1));
  static const uint gTOP     = 1u << (8*(CODBYTES-2));
  // ~0>>(64-LOWBITS) rather than (1<<LOWBITS)-1: nothing to get wrong at 64.
  static constexpr qword LOW_MASK = qword(-1) >> (64-LOWBITS);
  static constexpr qword LOW_MAX  = LOW_MASK+1;

  uint  lowl, lowh;   // low accumulator, LOWBITS wide, as (lowh<<32)|lowl
  uint  rpre;    // the unit out of rc_Renorm; scaled to the low delta in
                 // rc_Process, which is what the next ShiftLow applies
  // The carry-propagating coder's two: the carry out of low, and the byte
  // held back so that carry can still reach it. The carryless coder has
  // neither -- its ShiftLow emits every byte as it goes -- so the vector
  // kernel, which is always carryless (rc_config.inc errors otherwise), does
  // not carry them. Their place in the declaration order is the scalar
  // coder's layout and does not move.
#if !RC_VECOUT
  uint  Carry;
#endif
  uint  code;
  uint  range;
  // Its only consumer is model0.inc's `ncarry &= FFNum[j]` over every lane at
  // block end, so the per-lane values are never read apart and the generated
  // kernel can give it fewer slots than lanes -- see RC_FF_LANES. The enum is
  // how that reaches rc_soa.pl: comments do not survive the preprocessor.
  enum { FFNum_SOA_FOLD = RC_FF_LANES };
  uint  FFNum;
#if !RC_VECOUT
  uint  Cache;   // see Carry above
#endif

#if RC_VECOUT
  // The renorm's shift count, kept in lane state rather than in a local,
  // which is the whole point of those shapes -- see rc_config.inc.
  uint  dsh;

  // Where ShiftLow's store goes when the build stages it instead of
  // performing it: the dword, the tmpbase offset it belongs at, and the
  // renorm count that says whether it is a store at all -- see rc_FlushAll.
#if RC_SCATTER
  uint  stcl, stad, stn;
#endif

  // The decoder's vector renorm pass (rc_RenormRun below): a lane's cached
  // refill window -- the dword ending at its cursor -- and the shift it took
  // in the last pass, which is what the refill selects lanes by. RC_DEC_RENORM_ZS
  // borrows the window slot for the dword it address-selects.
#if RC_DEC_VRENORM || RC_DEC_RENORM_ZS
  uint  vwin, vsh;
#endif

  // what this kernel was generated against; model0.inc checks it. Every knob
  // above reaches the generator, and RC_KERNEL_CONF carries every one of them
  // -- so a field set chosen by a knob is a build error when the two disagree,
  // not a silently different kernel.
  enum { KERNEL_CONF = RC_KERNEL_CONF };
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

#if !RC_VECOUT
//----------------------------------------------------------------------------
//  The low accumulator, and the scalar renorm that drains it. These three
//  helpers are here and not beside the state because the vector twins below
//  inline all of them -- they return values, so the generator drops them.
//----------------------------------------------------------------------------

  // low += rpre, returning the carry out of the LOWBITS-wide accumulator.
  // The LOWBYTES==8 / <8 split is sh_v1xN.inc's: at 8 there is no bit left to
  // hold the overflow, so the test has to be on the addition itself.
  uint low_Add( void ) {
    uint c = 0;
    const uint cf = (lowl+rpre < lowl);
    lowl += rpre;
    if( LOWBYTES==8 ) c = (lowh+cf < lowh);
    lowh += cf;
    if( LOWBYTES< 8 ) c = (lowh >= uint(LOW_MAX>>32));
    lowh &= uint(LOW_MASK>>32);          // clears that bit (and is 0 at LB4)
    return c;
  }

  uint low_Top( void ) {                 // the byte about to leave
    // LOWBITS-40 is where the top byte sits inside lowh; at LOWBYTES==4 lowh
    // holds nothing and the top byte is still in lowl.
    enum { TOPSH = LOWBITS>32 ? LOWBITS-40 : 0 };
    return LOWBITS>32 ? ((lowh>>TOPSH)&0xFF) : ((lowl>>24)&0xFF);
  }

  void low_Shift8( void ) {
    lowh = (lowh<<8) | (lowl>>24);       // constant count: no 32-sh trap here
    lowl <<= 8;
    lowh &= uint(LOW_MASK>>32);
  }

  void ShiftLow( uint n ) {

    const uint c = low_Add();
    rpre = 0;

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
      const uint cl = low_Top();
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
    const uint n = (range<sTOP) + (range<gTOP);
    if( f_DEC==0 ) ShiftLow(n); else ShiftCode(n);
    range <<= n*8;
  }

#else // RC_VECOUT ---------------------------------------------------------
  //
  //  The vector-shape twins. Reachable only through mk_kernel.sh, which
  //  resolves this file with RC_VECOUT=1 (and RC_CARRYLESS=1) and turns the
  //  lane state into [RCNUM] arrays -- see rc_soa.pl. They address the
  //  output through the flat cursor the generated prelude declares
  //  (tmpbase + tmpptr), never through RC_IO, and inline the low_* helpers
  //  above. A scalar build never compiles this branch, and the arithmetic is
  //  line for line the scalar coder's -- t.sh checks the streams byte for
  //  byte.

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
    // RC_ENC_RENORM decides what `n` IS and whether this body keeps any locals.
    // At 0 it is the byte count 0/1/2 and the shift is derived here; at >=1 it
    // is the SHIFT COUNT 0/8/16 and the byte count is derived instead. That
    // direction matters: everything downstream of the selection wants the
    // shift, and only tmpptr wants the count. See rc_config.inc.
#if RC_ENC_RENORM==0
    const uint sh = n*8;   // at >=1 the parameter IS the shift, so nothing to declare
#endif
#if RC_FOLD_RPRE
    // rc_Process already added rpre to low and folded the carry into FFNum,
    // so this whole block moved there -- see rc_config.inc
#else
    const uint _cf = (lowl+rpre < lowl);
    lowl += rpre;
#if RC_LOWBYTES==8
    const uint _cy = (lowh+_cf < lowh);
    lowh += _cf;
#else
    lowh += _cf;
    const uint _cy = (lowh > uint(LOW_MASK>>32));
    lowh &= uint(LOW_MASK>>32);
#endif
    rpre = 0;
    // "use as flag", literally. The generated kernel's only reader is
    // model0.inc's ncarry fold, which tests against zero, so carry the
    // NO-CARRY predicate as an all-ones mask instead of a count: `_cy-1` is
    // that mask, and the accumulate becomes one vpand of a compare result
    // clang already has in hand, with no +1 fixup at the fold.
    FFNum &= _cy - 1;
#endif
    // cl's top byte lands at tmpptr+1, the next at tmpptr, the rest
    // below as staging that later stores overwrite
#if RC_ENC_RENORM<2
    const uint cl = RC_LOW_CL;
#endif
    // The store -- performed here, or staged for rc_FlushAll to commit a
    // group at a time (RC_SCATTER, rc_config.inc). Staged, n==0 means the
    // store is dead: the cursor does not move, so this lane's next store
    // writes this exact window again and every byte here is overwritten
    // before anything reads the row. That is the common case -- the coder
    // emits about 0.08 bytes per coded bit -- and RC_SCATTER_SKIP is what
    // acts on it.
#if RC_SCATTER
    stcl = RC_ENC_CL; stad = tmpptr-2; stn = RC_ENC_NB;
#else
    (uint&)tmpbase[tmpptr-2] = RC_ENC_CL;
#endif
    tmpptr -= RC_ENC_NB;
    // sh is 0, 8 or 16, so splitting the shift keeps both counts <= 16 and
    // the sh==0 case falls out as 0 on its own -- (x>>16)>>16 is a defined
    // pair of 32-bit shifts where x>>32 is not. That drops the vpcmpeqd and
    // vpandn the ternary compiled to in each half of the sweep.
    //
    // x86 SIMD and NEON would give the same answer from the single shift
    // `lowl >> (32-sh)`, because their variable shift saturates to zero past
    // the lane width -- but scalar x86 masks the count and OpenCL defines the
    // shift as `x >> (n & 31)`, so that form is target-specific and undefined
#if RC_ENC_RENORM>=2
    // _hi and cl were each read exactly once; at 2 they are written out where
    // they were used and the body declares nothing at all.
    lowh = ((lowh<<RC_ENC_SH) | RC_ENC_HI) & uint(LOW_MASK>>32);
    lowl <<= RC_ENC_SH;
#else
    const uint _hi = (lowl >> 16) >> (16-RC_ENC_SH);
    lowh = ((lowh<<RC_ENC_SH) | _hi) & uint(LOW_MASK>>32);
    lowl <<= RC_ENC_SH;
#endif
  }

  // ---- committing the staged store (RC_SCATTER) ---------------------------
  //
  // ShiftLow stages its dword in stcl/stad/stn instead of storing it, and the
  // model commits a whole group of them at once: the staged arrays are RCNUM
  // lanes of dword and dword-offset, which is exactly a vpscatterdd -- the
  // one instruction SLP will not produce on its own -- or, where there is no
  // scatter, a block of scalar stores that at least needs no address
  // extraction. Both are nothing at all when the build is not staging.

  // this lane's staged store, now: the block-length bits and the tail run
  // one lane at a time and come back to lane 0, so they commit per bit
  void rc_FlushOne( void ) {
#if RC_SCATTER
    (uint&)tmpbase[stad] = stcl;
#endif
  }

  // the RC_FLUSH_LANES lanes from this one, as one commit. RC_SCATTER_W lanes
  // to a scatter, and the width has to match what the sweep's arithmetic came
  // out at (rc_config.inc); unaligned loads, because the lane arrays carry
  // VECSIZE alignment, which is not the scatter's width. RC_SCATTER_SKIP=1
  // scatters under a mask of the lanes that owe a store -- the compare gives
  // the k-mask directly, so testing them and masking them is the same one
  // instruction -- or, in the scalar block, branches past a group where no
  // lane owes one.
  void rc_FlushAll( void ) {
#if !RC_SCATTER
#elif RC_SCATTER_W==16
    const __m512i a = _mm512_loadu_si512( (const void*)&stad[rcidx] );
    const __m512i v = _mm512_loadu_si512( (const void*)&stcl[rcidx] );
#if RC_SCATTER_SKIP
    const __mmask16 m = _mm512_cmpneq_epi32_mask( _mm512_loadu_si512( (const void*)&stn[rcidx] ), _mm512_setzero_si512() );
    _mm512_mask_i32scatter_epi32( (void*)tmpbase, m, a, v, 1 );
#else
    _mm512_i32scatter_epi32( (void*)tmpbase, a, v, 1 );
#endif
#elif RC_SCATTER_W==8
    const __m256i a = _mm256_loadu_si256( (const __m256i*)&stad[rcidx] );
    const __m256i v = _mm256_loadu_si256( (const __m256i*)&stcl[rcidx] );
#if RC_SCATTER_SKIP
    const __mmask8 m = _mm256_cmpneq_epi32_mask( _mm256_loadu_si256( (const __m256i*)&stn[rcidx] ), _mm256_setzero_si256() );
    _mm256_mask_i32scatter_epi32( (void*)tmpbase, m, a, v, 1 );
#else
    _mm256_i32scatter_epi32( (void*)tmpbase, a, v, 1 );
#endif
#else
    // no scatter on this target: the whole group as a block of scalar stores
    // read back out of the staged arrays. Still the point -- the arrays got
    // there as contiguous vector stores, so the address extraction the inline
    // store needed (24 vpextrd plus 4 vextracti128 per group, all port 5) is
    // gone and what is left lands on the store port. Measured on AVX2
    // (skylake, RCNUM=16, 20 MB of enwik8): 103.1 -> 96.7 cycles per group.
#if RC_SCATTER_SKIP
    uint z = 0;
    RC_UNROLL
    for( uint i=0; i<RC_FLUSH_LANES; i++ ) z |= stn[rcidx+i];
    if_e0( z )
#endif
    {
      RC_UNROLL
      for( uint i=0; i<RC_FLUSH_LANES; i++ ) (uint&)tmpbase[stad[rcidx+i]] = stcl[rcidx+i];
    }
#endif
  }

  // Encode: the counted shift. Decode: the one-branch refill below.
  void rc_Renorm( void ) {
    if( f_DEC==0 ) {
      // RC_ENC_NSEL carries the decoder's two lessons across to this side:
      // pick the SHIFT COUNT with a nested ternary rather than summing two
      // compares into a byte count, and hold it in lane state rather than in a
      // local. See rc_config.inc for what each is worth.
#if RC_ENC_NSEL==2
      dsh = (range<sTOP) ? (range<gTOP) ? 16 : 8 : 0;
      ShiftLow( RC_ENC_ARG(dsh) );
      range <<= dsh;
#elif RC_ENC_NSEL==1
      // _sh, not sh: at RC_ENC_RENORM==0 ShiftLow's body declares its own `sh`,
      // and after the generator turns the function into a macro an argument
      // named sh would expand into that declaration's own initialiser.
      const uint _sh = (range<sTOP) ? (range<gTOP) ? 16 : 8 : 0;
      ShiftLow( RC_ENC_ARG(_sh) );
      range <<= _sh;
#else
      const uint n = (range<sTOP) + (range<gTOP);
      ShiftLow( RC_ENC_ARG(n*8) );
      range <<= n*8;
#endif
    }
    if( f_DEC==1 ) {
      // Shelwien's one-branch refill (rc_renorm_1br.txt). Every operation is
      // its own statement on lane state -- no locals -- because in the 16x
      // unrolled decode loop a local is a scalar clang has to allocate a
      // register for sixteen times over, while a lane slot it can re-read or
      // rematerialise. dsh holds the SHIFT COUNT, 0/8/16, not the byte count:
      // it is what both shifts want and the cursor's decrement is dsh>>3 for
      // free.
      //
      // The outer test is worth ~18% -- ~92% of bits renormalise nothing, and
      // the two unconditional shifts it skips land on the loop-carried range
      // and code chains. The rpre fold is DUPLICATED into both arms rather
      // than hoisted above it: hoisting measures 1.7% slower, because it makes
      // code and rpre live across the test in a body short of registers.
      //
      // misc/dec_renorm.md is the analysis, and the nine other shapes it
      // measured; this is the one that won.
      //
      // Under RC_DEC_VRENORM the group loop never calls this -- model1.inc
      // renormalises a run of lanes at once, after their bits, and rc_Process
      // applies rpre to code itself -- so what is left here serves the
      // block-length bits and the tail, and the fold has nothing to fold.
#if RC_DEC_RENORM_ZS
      // Shelwien's shape, as sent (rc_config.inc): branchless, the load's
      // ADDRESS selected between the row and this lane's zero slot in the
      // RCio, the slot rewritten every step. The offset is signed: the rows
      // sit below the slot in the RCio, so it is negative.
      code -= rpre;
      rpre  = 0;
      rcio.zslot[rcidx] = 0;
      dsh = (range<sTOP) ? (range<gTOP) ? 16 : 8 : 0;
      tmpptr -= dsh>>3;
      vwin = (range<sTOP) ? uint( ((char*)&tmpbase[tmpptr+1]) - ((char*)&rcio.zslot[rcidx]) ) : 0;
      vwin = *(const uint*)( ((char*)&rcio.zslot[rcidx]) + int(vwin) );
      code  <<= dsh;
      range <<= dsh;
      code  |= vwin & ((dsh>0) ? (dsh==8) ? 0xFF : 0xFFFF : 0);
#else
      if( __builtin_expect_with_probability( range<sTOP, 0, 0.99 ) ) {
        dsh     = (range<gTOP) ? 16 : 8;
        tmpptr -= dsh>>3;
        if( !RC_DEC_VRENORM ) { code -= rpre; rpre = 0; }
        code  <<= dsh;
        range <<= dsh;
        code   |= (uint&)tmpbase[tmpptr+1] & ((dsh==8) ? 0xFF : 0xFFFF);
      } else {
        if( !RC_DEC_VRENORM ) { code -= rpre; rpre = 0; }
      }
#endif
    }
  }

#endif // RC_VECOUT

//----------------------------------------------------------------------------
//  Coding one symbol.
//----------------------------------------------------------------------------

  // sh_v1xN_macro.inc's rc_Process, unchanged apart from dropping rcidx (the
  // lane index now selects the instance, not the array slot). freq is P(bit==0)
  // scaled to SCALE. This is the entry point the model uses.
  INLINE
  void rc_Process( uint freq, uint& _bit ) {
    uint _b = _bit;   // not `bit`: as a macro, callers pass a variable named
                      // bit, and `uint bit = bit;` would self-initialize

#if RC_VECOUT && RC_DEC_VRENORM
    // Decode renormalises AFTER the bit, a run of lanes at a time, in
    // model1.inc's vector pass -- see RC_DEC_VRENORM. The scalar paths that
    // still need it before the bit (the block-length bits, the tail) call
    // rc_Renorm themselves, through model1.inc's rc_ProcessR.
    if( f_DEC==0 ) rc_Renorm();
#else
    rc_Renorm();
#endif
    rpre = range>>SCALElog;

    rpre *= freq;

    if( f_DEC ) _b = (code>=rpre);

    range -= rpre;

    range = _b ? range : rpre;
    rpre &= -_b;

#if RC_VECOUT && RC_DEC_VRENORM
    // ...and the decoder applies it here rather than leaving it pending for
    // the next renorm, which no longer runs at the head of the step. rpre is
    // dead after this on the decode side; the slot is never read again.
    if( f_DEC==1 ) code -= rpre;
#endif

#if RC_VECOUT && RC_FOLD_RPRE
    // Apply rpre here instead of at the next ShiftLow -- see rc_config.inc.
    // f_DEC is a template parameter, so the decoder (which needs rpre for
    // ShiftCode's `code -= rpre`) never compiles this.
    if( f_DEC==0 ) {
      const uint _pf = (lowl+rpre < lowl);
      lowl += rpre;
      const uint _pc = (lowh+_pf < lowh);
      lowh += _pf;
      rpre = 0;
      FFNum &= _pc - 1;
    }
#endif

    _bit = _b;
  }

#if RC_VECOUT
  // rc_Process with the renorm BEFORE the bit, for the paths that need it
  // that way. Without the pass, rc_Process already renormalises.
  void rc_ProcessR( uint freq, uint& _bit ) {
    if( RC_DEC_VRENORM ) rc_Renorm();
    rc_Process( freq, _bit );
  }
#endif

//----------------------------------------------------------------------------
//  The multi-symbol interface, on top of the binary core. Unused by this
//  coder's model and kept only because dropping it would make rc.inc no
//  longer be sh_v1xN_s.cpp's Rangecoder: rc_soa.pl drops rc_GetFreq from the
//  generated kernel (it returns a value) and nothing calls the other one.
//----------------------------------------------------------------------------

  // Decode needs the unit before it can pick the symbol, so this is where the
  // decoder renormalises. No clamp is needed: the top symbol gets rpre*freq
  // like every other, so the leftover range above rpre*totFreq is never coded
  // into and code/rpre stays below totFreq.
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

    const uint tmp = rpre*cumFreq;

    range = rpre*freq;

    rpre = tmp;   // low += tmp / code -= tmp, deferred to the next renorm
  }

//----------------------------------------------------------------------------
//  Starting and finishing a substream.
//----------------------------------------------------------------------------

  void rc_Init( void ) {
    lowl  = 0;
    lowh  = 0;
    rpre  = 0;
#if RC_VECOUT
    FFNum = uint(-1);   // the no-carry mask of ShiftLow above
#else
    Carry = 0;
    FFNum = 0;
    Cache = -1;
#endif
    range = RC_RANGE_INIT;
#if RC_VECOUT
    // Seed the staging at where the first ShiftLow's store would land anyway:
    // the row's zero prefix, which rc_Write drops. Keeps a commit before the
    // first ShiftLow -- there is none today -- from writing somewhere it
    // should not. Nothing when the build is not staging, and encoder-only in
    // any case: in the decoder tmpptr indexes the INPUT row, so tmpptr-2 is
    // not an address anything should be seeded with. Dead either way, but
    // visibly so.
#if RC_SCATTER
    if( f_DEC==0 ) { stcl = 0; stad = tmpptr-2; stn = 0; }
#endif
#endif
    if( f_DEC==1 ) { code = 0; for( uint i=0; i<CODBYTES; i++ ) code = (code<<8) + get(); }
  }

  // The aligned flush's byte count, from `range` alone -- which is why the
  // reader can ask for it after mirroring rc_Quit's renorm and know how many
  // of the bytes it just read belong to whatever follows. See rc_Quit.
  uint quit_drop( void ) const { return (RC_BSR32(range)-1)/8; }

  // Flush the largest value in [low, low+range) whose bottom bits are all 1:
  // fills llow and takes the bytes that buys off n, the flush length.
  //
  //   low|(2^k-1) < high  <=>  (low>>k) < (high>>k)
  //
  // -- both sides say the top 64-k bits of low and high differ -- so the
  // longest fillable run is the position of the top set bit of low^high.
  // No search: one bit scan. And since a fill that fails at k fails at
  // every wider k, that same k is what the byte-wise loop this replaced
  // was finding, one byte at a time.
  //
  // Only the byte-aligned part of the run shortens the flush: the decoder
  // reads 0xFF past the end of a substream (rc_Read pads it -- see
  // rc_config.inc), so a trailing all-ones BYTE need not be written, and
  // that count is floor(k/8) either way. Filling the leftover k%8 bits is
  // free and cannot drop another byte; what it buys is the value of the
  // last byte we do write. See misc/flush_bits.md.
  //
  // range >= sTOP holds at every flush, so k >= 24: a flush always drops
  // at least 3 of the LOWBYTES it would otherwise emit.
  void flush_fill( qword& llow, uint& n ) {
    const qword high = llow + range;
    const uint  hic  = (high<llow);   // llow+range can leave the qword at LOWBYTES==8
    const uint  k    = hic ? LOWBITS : RC_BSR64( llow ^ high );
    llow |= (k>=64) ? ~qword(0) : ((qword(1)<<k)-1);
    n    -= k/8;
  }

#if RC_VECOUT
  // The carryless minimal flush. No Cache and no FF run to settle first:
  // this coder's ShiftLow wrote those bytes as it went, so only the tail of
  // low is left.
  void rc_Quit( void ) {
    if( f_DEC==0 ) {
      rc_Renorm();
      // commit the staged store before the put run below, which overwrites
      // the top of that dword -- exactly what it does to the direct store.
      rc_FlushOne();

      uint i, n = LOWBYTES;
      qword llow = (qword(lowh)<<32) + lowl;
      flush_fill( llow, n );

      // the cursor sits below ShiftLow's staging pair; the next stream
      // byte belongs one up (sh_v1xN_macro.inc's ++tmpptr before put1)
      tmpptr += 1;
      for( i=0; i<n; i++ ) put( byte(llow>>(LOWBITS-8-i*8)) );
    }
  }
#else
  // The same flush plus the two things the vector build never needs: the
  // ALIGNED variant, for a stream with no 0xFF pad under it (RC_HDR_IO), and
  // the carry coder's held-back Cache and 0xFF run.
  void rc_Quit( void ) {
    if( f_DEC==0 ) {
      rc_Renorm();   // applies the last pending rpre

      uint i, n = LOWBYTES;

      // Carry=0: cache   .. FF x FFNum .. low
      // Carry=1: cache+1 .. 00 x FFNum .. low
      qword llow = (qword(lowh)<<32) + lowl;

      if( t_IO::QUIT_ALIGN ) {

        // The aligned flush, for a stream that has no 0xFF pad under it: the
        // bytes it drops come back as whatever follows in the file, so they
        // have to be bytes ANY value can occupy. That means flushing an
        // ALIGNED BLOCK -- pick j, round low up to a multiple of 2^(8j) and
        // write the top LOWBYTES-j bytes of that. Whatever the reader picks up
        // for the bottom 8j bits, it lands in [A, A+2^(8j)), and that whole
        // block is inside the interval.
        //
        // A 2^m-aligned block fits in [low, low+range) exactly when
        //
        //     (low+2^m-1) | (2^m-1)  <  low+range
        //
        // -- the fill test with low rounded UP instead of down, which is the
        // whole difference between the two flushes. But j cannot be chosen
        // from that: `range` is the only thing the writer and the reader both
        // hold here (the writer has low, the reader has code), so j has to be
        // a function of range alone. Any interval of length >= 2^(m+1)
        // contains a 2^m-aligned block whatever low is, and that bound is
        // tight, so the best such function is
        //
        //     j = (bsr(range)-1)/8
        //
        // At CODBYTES=4 that is 3 whenever range >= 2^25 and 2 below it, so
        // the flush costs about one byte more per block than the FF one.
        const uint  j  = (RC_BSR32(range)-1)/8;
        const qword km = (qword(1)<<(8*j))-1;
        const qword A  = (llow + km) & ~km;
        // The round-up can leave the accumulator, which is a carry out of low
        // like any other. It cannot collide with one already pending: a
        // pending Carry means low just wrapped, so llow is under 2^32 and
        // nowhere near the top.
        Carry |= (A<llow);
        llow = A;
        n -= j;

      } else {

        flush_fill( llow, n );

      }

#if RC_CARRYLESS

      // Cache and the FF run do not exist here; ShiftLow already wrote those
      // bytes, so only the tail of low is left. Which also means the trailing
      // FFs that the carry-mode flush below trims are already committed.
      for( i=0; i<n; i++ ) put( byte(llow>>(LOWBITS-8-i*8)) );


#else

      if( (n==0) && (Carry==0) ) {
        // Everything still to write is one run of 0xFF: the Cache byte, when
        // it is one, and FFNum more. The decoder reads 0xFF past the end of a
        // substream, so the tail of that run need not be written -- but
        // rc_Read's padding is RC_FF_PADSIZE and the run is as long as the
        // data makes it, so only RC_FF_TRIM of it may go. sh_v1xN_c.inc and
        // sh_v1xN_s.cpp drop the whole run; on a block whose flush leaves more
        // than the padding behind, that decodes to the wrong bytes.
        const uint run  = FFNum + ((Cache!=uint(-1)) && (Cache==0xFF));
        const uint keep = (run>RC_FF_TRIM) ? run-RC_FF_TRIM : 0;
        if( (Cache!=uint(-1)) && (Cache!=0xFF) ) put( byte(Cache) );
        for( i=0; i<keep; i++ ) put( byte(0xFF) );
      } else {
        if( Cache!=uint(-1) ) put( byte(Cache+Carry) );
        for( i=0; i<FFNum; i++ ) put( byte(0xFF+Carry) );
        for( i=0; i<n; i++ ) put( byte(llow>>(LOWBITS-8-i*8)) );
      }

#endif

    }
  }
#endif // RC_VECOUT

//----------------------------------------------------------------------------
//  The decoder's renorm as a vector pass (RC_DEC_VRENORM).
//
//  model1.inc's group loop steps the NB=RCNUM/8 bytes of a group together,
//  one bit position at a time, so after step j the lanes of bit positions
//  <=j are done for the group -- and with the lane arrays bit-major
//  (RC_SLOT) they are a contiguous run. Under RC_DEC_VRENORM rc_Process does
//  not renormalise on the decode side: rc_RenormRun renormalises a run of
//  RC_DEC_VRENORM lanes AFTER their bits, in a loop the compiler vectorises,
//  and the block-length bits and the tail, which do not run through it, go
//  through rc_ProcessR. misc/vrenorm.md has the measurements and the two
//  mechanisms that shaped this; rc_config.inc has the knobs.
//----------------------------------------------------------------------------
#if RC_VECOUT && RC_DEC_VRENORM

  // One lane of the pass. Two compares give the shift count 0/8/16, the
  // cursor moves by the byte count, the two shifts, and the bytes shifted in
  // come from...
#if RC_DEC_VRENORM_GATHER
  // ...the row: every lane loads the dword at its new cursor, (1<<sh)-1 keeps
  // the bytes that came in -- none, for an idle lane -- and the compiler
  // makes the sixteen loads a gather. Under RC_DEC_ZSLOT an idle lane's
  // index is the zero slot at zofs (RCio::zslot) instead. The load is legal
  // for every lane: the window is inside the row by RC_SKIP>=4 above and the
  // 0xFF pad below (rc_io.inc). See RC_DEC_VRENORM_GATHER for why this is
  // not the default.
  void rc_RenormV( uint zofs ) {
    const uint r  = range;
    const uint sh = ( uint(r<sTOP) + uint(r<gTOP) )*8;
    const uint p  = tmpptr - (sh>>3);
    const int  ix = (RC_DEC_ZSLOT && sh==0) ? int(zofs) : int(p+1);
#if RC_DEC_VRENORM_PROBE==1
    const uint w  = p;   // the probe: no load at all
#else
    const uint w  = (uint&)tmpbase[ix];
#endif
    tmpptr = p;
    code   = (code<<sh) | (w & ((1u<<sh)-1));
    range  = r<<sh;
  }
#else
  // ...vwin, the lane's cached refill window: the dword ending at its cursor,
  // `vwin == (uint&)tmpbase[tmpptr-3]` on entry to every pass -- the
  // invariant misc/dec_renorm.md's shape 9 kept in scalar lane state. The
  // cursor's own byte is the window's top byte, so the top sh bits are what
  // an sh-bit shift wants, and (win>>16)>>(16-sh) is 0 for a lane that
  // shifted nothing. No memory but the lane arrays: the refill below reloads
  // the window afterwards, and only for the lanes that moved it.
  void rc_RenormV( uint zofs ) {
    const uint r  = range;
    const uint sh = ( uint(r<sTOP) + uint(r<gTOP) )*8;
    tmpptr -= sh>>3;
    code  = (code<<sh) | ((vwin>>16)>>(16-sh));
    range = r<<sh;
    vsh   = sh;
  }
#endif

  // One lane's window reload -- also how the invariant is seeded at block
  // start. tmpptr-3 is inside the row: rc_io.inc asserts the pad covers it.
  void rc_Refill( void ) {
#if RC_DEC_VRENORM_PROBE==1
    vwin = tmpptr;       // the probe: no load from the row at all
#else
    vwin = *(const uint*)(tmpbase + tmpptr - 3);
#endif
  }

  // One lane off the run's mask: the lowest set bit, or the run's first slot
  // when there is none -- whose window did not move, so the reload is the
  // same dword again. tzcnt of 0 is 32 on BMI and the & folds it to slot 0.
  void rc_RefillOne( uint& msk ) {
    const uint i = ( msk ? uint(__builtin_ctz(msk)) : 32u ) & (RC_DEC_VRENORM-1);
    msk &= msk-1;
    rc_Refill( rcidx+i );
  }

  // The refill of the run starting at this lane: the lanes with vsh != 0 as
  // a bit per lane, RC_DEC_VRENORM_SLOTS of them reloaded unconditionally --
  // ~1.2 of 16 shift in a group -- and the rest by RC_DEC_VRENORM_TAIL's
  // shape. This is the part that does NOT vectorise: on this box, or on any
  // core that serialises gathers, sixteen loads for 1.2 lanes cost more than
  // the pass buys. And it must write nothing the scalar steps read: a store
  // whose address is known only after the mask -- forty cycles after the
  // group's last decision -- stalls every later load of that array in the
  // memory disambiguation logic, whatever the slack; a refill into code[]
  // measured 50 cycles a group (misc/vrenorm.md 3). vwin[] is read by the
  // next pass, a group later, and by then the address is old.
  void rc_RefillRun( void ) {
    uint msk = 0;
#if RC_DEC_VRENORM_PROBE==3
    // the probe: no refill at all
#else
#if defined(__AVX512VL__) && RC_DEC_VRENORM>=8
    for( uint c=0; c<RC_DEC_VRENORM; c+=8 )
      msk |= uint(_mm256_cmpneq_epi32_mask( _mm256_loadu_si256( (const __m256i*)&vsh[rcidx+c] ), _mm256_setzero_si256() )) << c;
#elif defined(__AVX2__) && RC_DEC_VRENORM>=8
    for( uint c=0; c<RC_DEC_VRENORM; c+=8 )
      msk |= uint(_mm256_movemask_ps( _mm256_castsi256_ps( _mm256_cmpeq_epi32( _mm256_loadu_si256( (const __m256i*)&vsh[rcidx+c] ), _mm256_setzero_si256() ) ) ) ^ 0xFF) << c;
#else
    for( uint k=0; k<RC_DEC_VRENORM; k++ ) msk |= uint(vsh[rcidx+k]!=0) << k;
#endif
#if RC_DEC_VRENORM_PROBE==4
    vwin = msk;          // the probe: the mask alone, kept live
#else
    rc_RefillOne( msk );
    if( RC_DEC_VRENORM_SLOTS>=2 ) rc_RefillOne( msk );
    if( RC_DEC_VRENORM_SLOTS>=3 ) rc_RefillOne( msk );
    if( RC_DEC_VRENORM_SLOTS>=4 ) rc_RefillOne( msk );
#if RC_DEC_VRENORM_TAIL==0
    if( msk ) do { rc_RefillOne( msk ); } while( msk );
#elif RC_DEC_VRENORM_TAIL==1
    if( msk ) for( uint k=0; k<RC_DEC_VRENORM; k++ ) rc_Refill( rcidx+k );
#endif
#endif
#endif
  }

#if RC_DEC_VRENORM_GATHER && defined(__AVX2__) && (RC_DEC_VRENORM%8)==0
  // Eight lanes of the gather shape, from this one, with the gather spelled
  // out. The loop vectoriser only forms a gather on a core LLVM marks
  // fast-gather -- Skylake and up -- and on a Haswell-tuned build
  // (-march=haswell, or native on one) it scalarises the sixteen loads into
  // vpextrd / load / vpinsrd chunks around the vector arithmetic, which is
  // what "the AVX2 build looks mostly scalar" turned out to be. The
  // intrinsic gathers wherever AVX2 is, and rc_RenormRun uses this body
  // there in place of the loop.
  //
  // The mask is the lanes that shift, and RC_DEC_ZSLOT is which of the two
  // ways an idle lane loads nothing: the gather's own mask, which fills the
  // lane from the zero source and never touches memory (0), or its index
  // pointed at the zero slot under an unmasked gather (1).
  void rc_RenormV8( uint zofs ) {
    const __m256i r   = _mm256_loadu_si256( (const __m256i*)&range[rcidx] );
    const __m256i m1  = _mm256_cmpeq_epi32( _mm256_min_epu32( r, _mm256_set1_epi32( int(sTOP-1) ) ), r );   // r < sTOP
    const __m256i m2  = _mm256_cmpeq_epi32( _mm256_min_epu32( r, _mm256_set1_epi32( int(gTOP-1) ) ), r );   // r < gTOP
    const __m256i c8  = _mm256_set1_epi32( 8 );
    const __m256i sh  = _mm256_add_epi32( _mm256_and_si256( m1, c8 ), _mm256_and_si256( m2, c8 ) );
    const __m256i p   = _mm256_sub_epi32( _mm256_loadu_si256( (const __m256i*)&tmpptr[rcidx] ), _mm256_srli_epi32( sh, 3 ) );
    const __m256i ix  = _mm256_add_epi32( p, _mm256_set1_epi32( 1 ) );
#if RC_DEC_VRENORM_PROBE==1
    const __m256i w   = p;   // the probe: no load at all
#elif RC_DEC_ZSLOT
    const __m256i w   = _mm256_i32gather_epi32( (const int*)tmpbase, _mm256_blendv_epi8( _mm256_set1_epi32( int(zofs) ), ix, m1 ), 1 );
#else
    const __m256i w   = _mm256_mask_i32gather_epi32( _mm256_setzero_si256(), (const int*)tmpbase, ix, m1, 1 );
#endif
    const __m256i one = _mm256_set1_epi32( 1 );
    const __m256i msk = _mm256_sub_epi32( _mm256_sllv_epi32( one, sh ), one );
    const __m256i cd  = _mm256_loadu_si256( (const __m256i*)&code[rcidx] );
    _mm256_storeu_si256( (__m256i*)&tmpptr[rcidx], p );
    _mm256_storeu_si256( (__m256i*)&code[rcidx], _mm256_or_si256( _mm256_sllv_epi32( cd, sh ), _mm256_and_si256( w, msk ) ) );
    _mm256_storeu_si256( (__m256i*)&range[rcidx], _mm256_sllv_epi32( r, sh ) );
  }
#endif

  // One pass over the run of RC_DEC_VRENORM lanes starting at this one: the
  // spelled-out gather body where it applies, else a loop for the compiler
  // to vectorise -- RC_DEC_VRENORM_VF lanes wide, and kept away from the
  // unroller, which would otherwise scalarise it before the vectoriser ever
  // saw it. The knob names inside the pragma are expanded when the compiler
  // parses it, not here.
  void rc_RenormPass( uint zofs ) {
#if RC_DEC_VRENORM_GATHER && defined(__AVX2__) && (RC_DEC_VRENORM%8)==0
    for( uint k=0; k<RC_DEC_VRENORM; k+=8 ) rc_RenormV8( rcidx+k, zofs );
#else
#if RC_DEC_VRENORM_VF>1
    _Pragma("clang loop vectorize(enable) vectorize_width(RC_DEC_VRENORM_VF) interleave_count(RC_DEC_VRENORM/RC_DEC_VRENORM_VF) unroll(disable)")
#else
    _Pragma("clang loop vectorize(disable) unroll(full)")   // the probe: scalar, unrolled
#endif
    for( uint k=0; k<RC_DEC_VRENORM; k++ ) rc_RenormV( rcidx+k, zofs );
#endif
  }

  // The pass, then its refill.
  void rc_RenormRun( uint zofs ) {
    rc_RenormPass( zofs );
    // the probe: the pass twice over -- the second finds nothing to shift, so
    // what it adds is the cost of a pass with nothing to wait for
    if( RC_DEC_VRENORM_PROBE==2 ) rc_RenormPass( zofs );
#if !RC_DEC_VRENORM_GATHER
    rc_RefillRun();
#endif
  }

#endif // RC_VECOUT && RC_DEC_VRENORM

};

#undef RC_RANGE_INIT
#undef RC_LOW_CL
#undef RC_BSR64
#undef RC_BSR32
#undef RC_IO_BASE
