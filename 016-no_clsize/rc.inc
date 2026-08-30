
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
  // Its only consumer is model0.inc's `ncarry &= FFNum[j]` over every lane at
  // block end, so the per-lane values are never read apart and the generated
  // kernel can give it fewer slots than lanes -- see RC_FF_LANES. The enum is
  // how that reaches rc_soa.pl: comments do not survive the preprocessor.
  enum { FFNum_SOA_FOLD = RC_FF_LANES };
  uint  FFNum;
  uint  Cache;
#if RC_STATS
  uint  n_carry; // isn't expected to be used on enough data to overflow
#endif
#if RC_VECOUT
  // Where ShiftLow's store goes when the build stages it instead of performing
  // it: the dword, the tmpbase offset it belongs at, and the renorm count that
  // says whether it is a store at all. Declared unconditionally, because the
  // generated kernel must not depend on anything but the -D set -- whether
  // they are used is RC_SCATTER's business, decided at COMPILE time by
  // RC_STAGE_CL / RC_FLUSH_ONE (model.inc). When it is off, nothing writes
  // them and they cost nothing.
  uint  stcl, stad, stn;

  // RC_DEC_RENORM 7 and 8 keep the renorm's shift count in lane state rather
  // than in a local, which is the whole point of those shapes -- see
  // rc_config.inc. Declared unconditionally for the same reason the staging
  // trio is: the generated kernel must not depend on anything but the -D set.
  // Nothing writes it in the other shapes and it costs nothing there.
  uint  dsh;

  // RC_DEC_RENORM 9's cached refill window: the four stream bytes ending at
  // the cursor, so the invariant is `win == (uint&)tmpbase[tmpptr-3]` at every
  // entry to rc_Renorm. rc_Init seeds it and rc_Renorm re-fills it AFTER using
  // it, which is the whole point -- see rc_config.inc.
  uint  win;

  // what this kernel was generated against; model0.inc checks it
  enum { KERNEL_CONF = RC_KERNEL_CONF };
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
    // RC_ENC_RENORM decides what `n` IS and whether this body keeps any locals.
    // At 0 it is the byte count 0/1/2 and the shift is derived here; at >=1 it
    // is the SHIFT COUNT 0/8/16 and the byte count is derived instead. That
    // direction matters: everything downstream of the selection wants the
    // shift, and only tmpptr wants the count. See rc_config.inc.
#if RC_ENC_RENORM>=1
    RC_ENC_SHDECL
#else
    uint sh = n*8;
#endif
#if RC_FOLD_RPRE
    // rc_Process already added rpre to low and folded the carry into FFNum,
    // so this whole block moved there -- see rc_config.inc
#else
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
    // "use as flag", literally. The generated kernel's only reader is
    // model0.inc's ncarry fold, which tests against zero, so carry the
    // NO-CARRY predicate as an all-ones mask instead of a count: `_cy-1` is
    // that mask, and the accumulate becomes one vpand of a compare result
    // clang already has in hand, with no +1 fixup at the fold.
    FFNum &= _cy - 1;
#endif
    // cl's top byte lands at tmpptr+1, the next at tmpptr, the rest
    // below as staging that later stores overwrite
#if RC_LOWSPLIT
#if RC_ENC_RENORM<2
#if RC_LOWBYTES==8
    uint cl = lowh;
#elif RC_LOWBYTES>=6
    uint cl = lowh << (64-LOWBITS);
#elif RC_LOWBYTES==5
    uint cl = (lowh<<24) | (lowl>>8);
#else
    uint cl = lowl;
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
#else
#if RC_LOWBYTES==8
    qword cl = lowc;
    (qword&)tmpbase[tmpptr-6] = cl;
#else
    uint cl = uint(lowc >> (LOWBITS-32));
    (uint&)tmpbase[tmpptr-2] = cl;
#endif
#endif
    tmpptr -= RC_ENC_NB;
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
#if RC_ENC_RENORM>=2
    // _hi and cl were each read exactly once; at 2 they are written out where
    // they were used and the body declares nothing at all.
    lowh = ((lowh<<RC_ENC_SH) | RC_ENC_HI) & uint(LOW_MASK>>32);
    lowl <<= RC_ENC_SH;
#else
#if RC_SHIFT_SAT
    uint _hi = lowl >> (32-RC_ENC_SH);
#else
    uint _hi = (lowl >> 16) >> (16-RC_ENC_SH);
#endif
    lowh = ((lowh<<RC_ENC_SH) | _hi) & uint(LOW_MASK>>32);
    lowl <<= RC_ENC_SH;
#endif
#else
    lowc = (lowc<<RC_ENC_SH) & LOW_MASK;
#endif
  }

  // Encode: the counted shift. Decode: whichever of RC_DEC_RENORM's nine
  // shapes is built -- by default the branchy nested refill with the cursor
  // touched only inside it, ~91% of bits passing two predicted tests and
  // doing nothing else, the shape rc_split_ispc_v1.md section 9.5 measured.
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
#if RC_DEC_RENORM<7
      code -= rpre;
      rpre = 0;
#endif
#if RC_DEC_RENORM>=7
      // Shelwien's shapes, and they are not written the way the others are.
      // Every operation is its own statement on lane state -- no locals at all
      // -- because in the 16x unrolled decode loop a local is a scalar with a
      // live range clang has to allocate a register for sixteen times over,
      // while a lane-array slot is something it can re-read or rematerialise
      // as it likes. See rc_config.inc for what that measured.
      //
      // dsh holds the SHIFT COUNT, 0/8/16, not the byte count: it is what
      // both shifts want, and the cursor's decrement is dsh>>3 for free. The
      // nested ternary produces it directly. Selecting the byte count that way
      // instead and deriving the shift from it measures 29% slower.
#if RC_DEC_RENORM==9
      // Branchless, and the refill read one iteration ahead of where it is
      // needed. 7 reloads the window every bit and consumes it in the same
      // bit, so the chain is range -> dsh -> tmpptr -> load -> code and the
      // load's latency is on it. Here `code` takes its bytes out of win, which
      // the PREVIOUS bit already loaded, and this bit's load only has to land
      // before the next one reads it. Same load count, a whole iteration of
      // slack -- the trick RC_FOLD_RPRE plays on the encoder.
      dsh     = (range<sTOP) ? (range<gTOP) ? 16 : 8 : 0;
      code   -= rpre;
      rpre    = 0;
      code  <<= dsh;
      range <<= dsh;
      code   |= (win>>16) >> (16-dsh);   // the top dsh bits of the cached word
      tmpptr -= dsh>>3;
      win     = (uint&)tmpbase[tmpptr-3];
#elif RC_DEC_RENORM==8
      // The outer test kept -- the same one shape 0 has, and worth ~18% here.
      // The rpre fold is DUPLICATED into both arms rather than hoisted above
      // the branch: hoisting it measures 1.7% slower, because it makes code
      // and rpre live across the test in a body that is short of registers.
#if RC_DEC_COLD
      if( __builtin_expect_with_probability( range<sTOP, 0, 0.99 ) ) {
#else
      if_e0( range<sTOP ) {
#endif
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
#else
      dsh     = (range<sTOP) ? (range<gTOP) ? 16 : 8 : 0;
      tmpptr -= dsh>>3;
      code   -= rpre;
      rpre    = 0;
      code  <<= dsh;
      range <<= dsh;
      code   |= (uint&)tmpbase[tmpptr+1] & ((dsh==0) ? 0 : (dsh==8) ? 0xFF : 0xFFFF);
#endif
#elif RC_DEC_RENORM>=2 && RC_DEC_RENORM<=4
      // No branch: the decoder's counted shift, the encoder's `n` expression
      // read the other way round. gTOP < sTOP, so range<gTOP implies
      // range<sTOP and n is 0, 1 or 2 -- n bytes leave the substream and n
      // enter code, and n==0 leaves all three of range, code and the cursor
      // exactly as they were.
      uint _q = tmpptr;
#if RC_DEC_RENORM==3
      // sTOP and gTOP are 1<<24 and 1<<16, so the pair of compares is a count
      // of leading zero BYTES. range is never below 1<<9 -- the model clamps p
      // to [1,SCALE-1], so one step divides range by at most SCALE=1<<15 and
      // range was at least 1<<24 -- so clz is never asked about zero and n
      // never comes out 3.
      uint sh = uint(__builtin_clz(range)) & ~7u;
      uint n  = sh>>3;
#else
      uint n  = (range<sTOP) + (range<gTOP);
      uint sh = n*8;
#endif
#if RC_DEC_RENORM==4
      // The window addressed by n instead of shifted: the n bytes the refill
      // wants are [_q-n+1 .. _q], and a little-endian dword loaded AT _q-n+1
      // already has them in its low n bytes, most significant at the top --
      // which is the order code wants them in. What is above them is stream
      // the cursor has already passed, so the mask is the whole cost.
      // (1<<sh)-1 is 0, 0xFF, 0xFFFF for n = 0, 1, 2; sh is at most 16, so the
      // shift is defined. Reads up to _q+4: see the pad assert in rc_io.inc.
      uint _c = (uint&)tmpbase[_q+1-n] & ((1u<<sh)-1);
#else
      // The window at a fixed offset and shifted: the dword at _q-3 has byte
      // _q at the top, so the top n bytes of it are the refill. sh is 0 here
      // too, and x>>32 is undefined, so it is the split shift ShiftLow uses.
      uint _c = ((uint&)tmpbase[_q-3] >> 16) >> (16-sh);
#endif
      range <<= sh;
      code    = (code<<sh) + _c;
      tmpptr  = _q - n;
#elif RC_DEC_RENORM==5
      // What the measured distribution actually asks for. Over 800M bits of
      // enwik8: n==0 92.195%, n==1 7.805%, n==2 FIFTEEN TIMES. So the refill
      // is a one-byte refill, branchless on the single compare that decides
      // it, and the second byte is a guard that is perfectly predicted because
      // it is taken once in 53 million. sh is 0 or 8, so the split shift is
      // still what keeps the n==0 case defined.
      {
        uint _q = tmpptr;
        uint sh = (range<sTOP)*8;
        range <<= sh;
        code    = (code<<sh) + (((uint&)tmpbase[_q-3] >> 16) >> (16-sh));
        tmpptr  = _q - (sh>>3);
      }
      // One byte took range to at least 1<<17, so at most one more is ever
      // owed, and after it range is at least 1<<25.
      if_e0( range<sTOP ) {
        uint _r = tmpptr;
        range <<= 8;
        code    = (code<<8) + tmpbase[_r];
        tmpptr  = _r - 1;
      }
#else
      // RC_DEC_COLD strengthens the hint: if_e0 is expect(x,0), which clang
      // reads as ~10% and is not enough to sink 16 unrolled copies of the
      // refill out of the loop body. ~91% of bits take neither branch.
#if RC_DEC_COLD
      if( __builtin_expect_with_probability( range<sTOP, 0, 0.99 ) ) {
#else
      if_e0( range<sTOP ) {
#endif
        uint _q = tmpptr;
#if RC_DEC_RENORM==1 || RC_DEC_RENORM==6
        // Inside the 8% that renormalise, the counted shift instead of the
        // second test. n is 1 or 2 here, never 0, so sh is 8 or 16 and 32-sh
        // is 24 or 16: one defined shift, no split and no branch.
        uint n  = 1 + (range<gTOP);
        uint sh = n*8;
        range <<= sh;
#if RC_DEC_RENORM==6
        // The window addressed by n rather than shifted: the n bytes wanted
        // are [_q-n+1 .. _q] and a dword loaded AT _q-n+1 already has them in
        // its low n bytes, most significant at the top. Same two extra
        // operations either way -- a shift and a subtract there, a shift and a
        // decrement here -- so this is a straight A/B of address arithmetic
        // against shift arithmetic. Reads up to _q+3: see rc_io.inc.
        code    = (code<<sh) + ((uint&)tmpbase[_q+1-n] & ((1u<<sh)-1));
#else
        code    = (code<<sh) + ((uint&)tmpbase[_q-3] >> (32-sh));
#endif
        tmpptr  = _q - n;
#else
#if RC_LOAD32
        // The substream runs backwards, so the two bytes the 2-byte case wants
        // are the top two of the little-endian dword at _q-3, and the one byte
        // the 1-byte case wants is its top byte. One load covers both, hoisted
        // above the branch, with no masking and no shift-or to assemble it.
        // See the pad assert in rc_io.inc for the three bytes below _q.
        //
        // NOT `(const uint&)`: a C-style cast to a CONST reference from a byte
        // lvalue is a static_cast to a temporary -- the byte widened to uint,
        // silently -- because binding a const lvalue reference to a converted
        // temporary is a valid conversion and is tried before reinterpret_cast.
        // A non-const uint& cannot bind to a temporary, so it reinterprets,
        // which is why the encoder's `(uint&)tmpbase[...] = cl` works.
#if RC_LOAD32==2
        // ... but only in the 2-byte branch. Hoisting the window above the
        // test makes the COMMON 1-byte case pay a shift it did not need, and
        // that case is the more frequent of the two.
        if_e0( range<gTOP ) {
          range <<= 16;
          code = (code<<16) + ((uint&)tmpbase[_q-3] >> 16);
          tmpptr = _q-2;
        } else {
          range <<= 8;
          code = (code<<8) + tmpbase[_q];
          tmpptr = _q-1;
        }
#else
        uint _w = (uint&)tmpbase[_q-3];
        if_e0( range<gTOP ) {
          range <<= 16;
          code = (code<<16) + (_w>>16);
          tmpptr = _q-2;
        } else {
          range <<= 8;
          code = (code<<8) + (_w>>24);
          tmpptr = _q-1;
        }
#endif
#else
        if_e0( range<gTOP ) {
          range <<= 16;
          code = (code<<16) + (uint(tmpbase[_q])<<8) + tmpbase[_q-1];
          tmpptr = _q-2;
        } else {
          range <<= 8;
          code = (code<<8) + tmpbase[_q];
          tmpptr = _q-1;
        }
#endif
#endif  // RC_DEC_RENORM 1 or 6
      }
#endif  // RC_DEC_RENORM shape dispatch
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
      uint _pf = (lowl+rpre < lowl);
      lowl += rpre;
      uint _pc = (lowh+_pf < lowh);
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

    uint tmp = rpre*cumFreq;

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
#if RC_STATS
    n_carry = 0;
#endif
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
#if RC_VECOUT && RC_DEC_RENORM==9
    // seed the cached window; get() above is the only other thing that moves
    // the decoder's cursor
    if( f_DEC==1 ) win = (uint&)tmpbase[tmpptr-3];
#endif
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
