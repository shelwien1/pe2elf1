
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

 // The CRTP base: RC_IO (rc_io.inc) unless the includer names another. The
 // header coder needs a cursor that runs FORWARDS, straight to and from the
 // coroutine, because its stream is the one whose length is not written down
 // anywhere -- see RC_HDR_IO. Everything below is written against the base's
 // get/put and does not care which it is.
#ifndef RC_IO_BASE
#define RC_IO_BASE RC_IO
#endif

 typedef uint  rangetype;
 // A CODBYTES-wide range. At the default 4 this is the whole word; narrower,
 // it is what lets `code` fit a float32 mantissa -- see RC_CODBYTES.
 // as a right shift, not (1<<8*CODBYTES)-1: at CODBYTES==4 that shift is 32,
 // and clang rejects it in a constant expression even on the dead branch.
 #define RC_RANGE_INIT ( 0xFFFFFFFFu >> (8*(4-RC_CODBYTES)) )

 // Index of the top set bit. A macro, not a function: rc_vecD.inc is this file
 // preprocessed, and its contents land inside do_process, where a definition
 // would be illegal. Never called with 0 -- rc_Quit's operand is low^high and
 // range>0 keeps them apart. (common.inc's log2i is 32-bit.)
#if defined(__GNUC__) || defined(__clang__)
 #define RC_BSR64( x ) uint( 63-__builtin_clzll(x) )
#elif defined(_MSC_VER)
 #define RC_BSR64( x ) [&]{ unsigned long i_; _BitScanReverse64(&i_,(x)); return uint(i_); }()
#else
 #define RC_BSR64( x ) [&]{ qword v_=(x); uint i_=0; while( v_>>=1 ) i_++; return i_; }()
#endif

template< int f_DEC >
struct Rangecoder : RC_IO_BASE< Rangecoder<f_DEC> > {

  typedef RC_IO_BASE< Rangecoder<f_DEC> > t_IO;
  using t_IO::get;
  using t_IO::put;

  static const int  RC_f_DEC  = f_DEC;    // so RangecoderN can read it back

  static const uint CODBYTES = RC_CODBYTES;
  static const uint LOWBYTES = RC_LOWBYTES;
  static const uint LOWBITS  = LOWBYTES*8;
  // The renorm thresholds are the top two byte boundaries of a CODBYTES-wide
  // range: at CODBYTES=4 these are the 1<<24 and 1<<16 the shapes in
  // rc_config.inc are written against, and they track CODBYTES below that.
  static const uint sTOP     = 1u << (8*(CODBYTES-1));
  static const uint gTOP     = 1u << (8*(CODBYTES-2));
  // what clz(range) reads as zero bytes to shift: 0 at CODBYTES=4.
  static const uint CLZBIAS  = 8*(4-CODBYTES);
  // ~0>>(64-LOWBITS) rather than (1<<LOWBITS)-1: nothing to get wrong at 64.
  static constexpr qword LOW_MASK = qword(-1) >> (64-LOWBITS);
  static constexpr qword LOW_MAX  = LOW_MASK+1;

  uint  lowl, lowh;   // low accumulator, LOWBITS wide, as (lowh<<32)|lowl
  uint  rpre;    // the unit out of rc_Renorm; scaled to the low delta in
                 // rc_Process, which is what the next ShiftLow applies
  uint  Carry;   // carry out of low, pending until the next byte is emitted
  uint  code;
  rangetype range;
  // Its only consumer is model0.inc's `ncarry &= FFNum[j]` over every lane at
  // block end, so the per-lane values are never read apart and the generated
  // kernel can give it fewer slots than lanes -- see RC_FF_LANES. The enum is
  // how that reaches rc_soa.pl: comments do not survive the preprocessor.
  enum { FFNum_SOA_FOLD = RC_FF_LANES };
  uint  FFNum;
  uint  Cache;
#if RC_VECOUT
  // Where ShiftLow's store goes when the build stages it instead of performing
  // it: the dword, the tmpbase offset it belongs at, and the renorm count that
  // says whether it is a store at all. Declared unconditionally, because the
  // generated kernel must not depend on anything but the -D set -- whether
  // they are used is RC_SCATTER's business, decided at COMPILE time by
  // RC_STAGE_CL / RC_FLUSH_ONE (model.inc). When it is off, nothing writes
  // them and they cost nothing.
  uint  stcl, stad, stn;

  // The renorm's shift count, kept in lane state rather
  // than in a local, which is the whole point of those shapes -- see
  // rc_config.inc. Declared unconditionally for the same reason the staging
  // trio is: the generated kernel must not depend on anything but the -D set.
  // Nothing writes it in the other shapes and it costs nothing there.
  uint  dsh;

  // what this kernel was generated against; model0.inc checks it
  enum { KERNEL_CONF = RC_KERNEL_CONF };
#endif

  #define RC_CARRY_COUNT(c) ((void)0)

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

  qword low_Get( void ) {
    return (qword(lowh)<<32) + lowl;
  }

  void low_Init( void ) {
    lowl = 0; lowh = 0;
  }

#if !RC_VECOUT

  void ShiftLow( uint n ) {

    const uint c = low_Add();
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
    RC_CARRY_COUNT(_cy);
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
#if RC_LOWBYTES==8
    const uint cl = lowh;
#elif RC_LOWBYTES>=6
    const uint cl = lowh << (64-LOWBITS);
#elif RC_LOWBYTES==5
    const uint cl = (lowh<<24) | (lowl>>8);
#else
    const uint cl = lowl;
#endif
#endif
    // The store -- performed here, or staged for the group to commit, which
    // is RC_STAGE_CL's business and the compile's, not the generator's.
    // Staged, n==0 means the store is dead: the cursor does not move, so this
    // lane's next store writes this exact window again and every byte here is
    // overwritten before anything reads the row. That is the common case --
    // the coder emits about 0.08 bytes per coded bit -- and RC_SCATTER_SKIP
    // is what acts on it.
    RC_STAGE_CL( rcidx, tmpptr-2, RC_ENC_CL, RC_ENC_NB );
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
      if( __builtin_expect_with_probability( range<sTOP, 0, 0.99 ) ) {
        dsh     = (range<gTOP) ? 16 : 8;
        tmpptr -= dsh>>3;
        code   -= rpre;
        rpre    = 0;
        code  <<= dsh;
        range <<= dsh;
        code   |= (uint&)tmpbase[tmpptr+1] & ((dsh==8) ? 0xFF : 0xFFFF);
      } else {
        code   -= rpre;
        rpre    = 0;
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
      RC_CARRY_COUNT(_pc);
      FFNum &= _pc - 1;
    }
#endif

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

    const uint tmp = rpre*cumFreq;

    range = rpre*freq;

    rpre = tmp;   // low += tmp / code -= tmp, deferred to the next renorm
  }

  void rc_Init( void ) {
    low_Init();
    rpre  = 0;
    Carry = 0;
#if RC_VECOUT
    FFNum = uint(-1);   // the no-carry mask of ShiftLow above
#else
    FFNum = 0;
#endif
    Cache = -1;
    range = RC_RANGE_INIT;
#if RC_VECOUT
    // Seed the staging at where the first ShiftLow's store would land anyway:
    // the row's zero prefix, which rc_Write drops. Keeps a commit before the
    // first ShiftLow -- there is none today -- from writing somewhere it
    // should not. A no-op when the build is not staging, and encoder-only in
    // any case: in the decoder tmpptr indexes the INPUT row, so tmpptr-2 is
    // not an address anything should be seeded with. Dead either way, but
    // visibly so.
    if( f_DEC==0 ) RC_STAGE_INIT( rcidx, tmpptr-2 );
#endif
    if( f_DEC==1 ) { code = 0; for( uint i=0; i<CODBYTES; i++ ) code = (code<<8) + get(); }
  }

#if RC_VECOUT
  // the carryless minimal flush, low_Get inlined
  void rc_Quit( void ) {
    if( f_DEC==0 ) {
      rc_Renorm();
      // commit the staged store before the put1 run below, which overwrites
      // the top of that dword -- exactly what it does to the direct store.
      // A no-op when the build is not staging.
      RC_FLUSH_ONE( rcidx );

      uint i, n = LOWBYTES;
      qword llow = (qword(lowh)<<32) + lowl;
      const qword high = llow + range;
      const uint  hic  = (high<llow);

      // Flush the largest value in [low, high) whose bottom bits are all 1.
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
      const uint k = hic ? LOWBITS : RC_BSR64( llow ^ high );
      llow |= (k>=64) ? ~qword(0) : ((qword(1)<<k)-1);
      n -= k/8;

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
      const qword high = llow + range;
      const uint  hic  = (high<llow);   // llow+range can leave the qword at LOWBYTES==8

      // Flush the largest value in [low, high) whose bottom bits are all 1.
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
      const uint k = hic ? LOWBITS : RC_BSR64( llow ^ high );
      llow |= (k>=64) ? ~qword(0) : ((qword(1)<<k)-1);
      // t_IO::QUIT_FLUSH is 0 for a substream the block header gives a length:
      // drop every byte the fill allows, and let rc_Read's 0xFF padding stand
      // in for them. A stream with no length written down anywhere cannot --
      // its reader would run off the end into whatever follows, which is not
      // 0xFF -- so it names its flush length instead and drops nothing. Both
      // are compile-time constants; only one arm survives.
      if( t_IO::QUIT_FLUSH ) n = t_IO::QUIT_FLUSH; else n -= k/8;

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

};

#undef RC_CARRY_COUNT
#undef RC_RANGE_INIT
#undef RC_BSR64
#undef RC_IO_BASE
