//  A binary rANS coder in rc.inc's shape: one scalar coder per lane, written
//  so that mk_kernel.sh can push it through the same three perl passes and get
//  a lane-array kernel out (rc_soa.pl -> rc_macro.pl -> defines.pl). The
//  includer #defines Rangecoder to the class name it wants -- see model.inc --
//  so this file is written against the bare name and has no include guard, on
//  purpose, exactly like rc.inc.
//
//  What it is. rANS with a 32-bit state, byte renormalisation and the model's
//  own SCALE as the frequency denominator:
//
//      M = SCALE = 1<<SCALElog     the probability denominator (2^15)
//      L = M<<RC_RANS_KLOG         the state's lower bound     (2^23)
//      b = 256                     the renormalisation base
//      x in [L, b*L) = [2^23, 2^31)
//
//  Coding a bit whose P(bit==0) is `freq` (the model's p, in [1,M-1]) means
//  coding a symbol with
//
//      f     = bit ? M-freq : freq        its frequency
//      start = bit ? freq   : 0           its cumulative frequency
//
//  and the two halves are the textbook rANS pair:
//
//      encode   x' = (x/f)*M + (x%f) + start
//      decode   s = x&(M-1);  bit = (s>=freq);  x' = f*(x>>SCALElog) + s - start
//
//  Differences from rc.inc that the rest of the coder has to know about:
//
//   - rANS is LIFO. The decoder consumes bytes in the reverse of the order the
//     encoder emitted them, so model0.inc's coding pass walks the block
//     BACKWARDS: last bit first, and the block-length header last. The
//     decoder still runs forwards. See the byte order note below.
//   - there is no carry. The whole carryless/carry-twin arrangement, FFNum,
//     Cache, the lost-carry re-code and the 0xFF padding go with it, and the
//     only loop-carried lane state left is x -- one register per lane against
//     the range coder's lowl, lowh, rpre, FFNum and range. That is the point:
//     misc/model0_process_skl.asm blames 27 of the encode loop's ~158 uops on
//     spilling a live set that does not fit.
//   - the encoder divides. That is rANS's one real cost, and RANS_DIV below is
//     where it is paid.
//   - p in [1,M-1] stops being a nicety and becomes a precondition. The range
//     coder degrades gracefully at p=0 -- the interval simply does not move --
//     where rANS divides by f=0. FSM.cpp's loader clamps pp[] to [1,SCALE-1]
//     and the block header codes at hSCALE, so nothing in this coder can
//     produce it; a different model would have to keep that clamp.
//
//  Byte order, and which way the cursors run. rANS emits the low byte of x
//  first, so a group of two lands as a little-endian pair, and the encoder
//  writes ASCENDING from the base of its row -- the other way round from
//  rc.inc, and for a reason: the store is a dword of which only the low
//  0..2 bytes are committed, and writing upwards puts the three bytes it
//  overshoots into stream the encoder has not reached yet, where the next
//  store overwrites them. Writing downwards would put them on top of bytes
//  already emitted, so the value would have to be pre-shifted by the byte
//  count: a shift, a subtract and a variable shift per half group, and 3.7% of
//  encode measured on 20 MB of enwik8. The coder was written that way first.
//
//  rc_Write then copies that region ascending as it always did, so the file
//  holds the substream first-emitted-first; a LIFO decoder wants the last
//  emitted first, so the decoder's cursor starts at the TOP of the chunk and
//  DESCENDS -- the same direction rc.inc's decoder runs, and its refill is
//  the same shape: one 32-bit load at a fixed offset below the cursor, here
//  shifted down rather than masked.
//
//  Needs: SCALElog (FSM.cpp), the knobs in rc_config.inc, RC_IO (rc_io.inc).

template< int f_DEC >
struct Rangecoder : RC_IO< Rangecoder<f_DEC> > {

  typedef RC_IO< Rangecoder<f_DEC> > t_IO;
  using t_IO::get;
  using t_IO::put;

  static const int  RC_f_DEC  = f_DEC;    // so RangecoderN can read it back

  // Named the way rc.inc names them, because rc_io.inc and rc_vec.inc read
  // them back and must not care which coder is underneath. CODBYTES is the
  // flush width: rANS ends a substream by writing the whole state.
  static const uint CODBYTES = 4;
  static const uint LOWBYTES = 4;

  // log2(L/M). The state carries log2(x) bits, and the rounding in x/f costs
  // the stream about M/(4x) of a bit per symbol -- so the coding loss is what
  // this exponent buys: 2^-KLOG/4 bits per coded bit, worst case. At 8 that is
  // 1/1024 of a bit against the 0.625 the coder actually spends, and the
  // measured cost on enwik8 is under 0.01%.
  //
  // It cannot go higher: the state has to stay below 2^31, because the
  // conversion below is the signed one. See RC_RANS_KLOG.
  static const uint KLOG  = RC_RANS_KLOG;
  static const uint RANSL = SCALE << KLOG;
  // The tightest word in the coder is p = M-1 with bit set: f = 1, start =
  // M-1, q = (1<<XSH)-1, and the state comes out at exactly (1<<(SCALElog+XSH))
  // - 1. One more bit of SCALElog or of KLOG and it would not fit -- and the
  // signed conversion wants one bit more than the word does.
  static_assert( SCALElog + RC_RANS_KLOG + 8 <= 31,
    "the rANS state must stay below 2^31: SCALElog + RC_RANS_KLOG + 8 is its "
    "width, and rans.inc converts it to float through (int)" );
  // x_max = (L/M)*b*f = f << XSH: the renormalisation threshold for a symbol
  // of frequency f.
  static const uint XSH   = KLOG + 8;

  // The state. One uint per lane, and the only thing carried between bits.
  uint rx;

  // RC_RANS_SPLIT's model-free half of the decode step: s = x & (M-1), one per
  // lane, produced for a whole group before the serial walk starts and read by
  // it. Outside the RC_VECOUT block because both coders carry it -- the scalar
  // reference has the same split pair, so the shapes stay comparable. Nothing
  // writes it in the unsplit shape and it costs nothing there.
  uint sv;
#if RC_VECOUT
  // Where ShiftLow's store goes when the build stages it instead of performing
  // it -- see RC_SCATTER in rc_config.inc and RC_STAGE_CL in model.inc. The
  // trio is declared unconditionally because the generated kernel must not
  // depend on anything but the -D set; when the build is not staging, nothing
  // writes them and they cost nothing.
  uint  stcl, stad, stn;

  // The decoder's refill shift, 0/8/16, kept in lane state rather than in a
  // local: in the 16x unrolled decode loop a local is a scalar clang has to
  // allocate a register for sixteen times over. rc.inc's dsh, and the same
  // reason. Nothing writes it on the encode side.
  uint  dsh;

  // what this kernel was generated against; model0.inc checks it
  enum { KERNEL_CONF = RC_KERNEL_CONF };
#endif

  // rANS cannot lose a carry because it has none. The model still asks, so
  // that model0.inc does not have to know which coder it is driving; rc_soa.pl
  // drops one-line value-returning methods, and the generated kernel never
  // needs this one.
  uint carry_lost( void ) const { return 0; }

#if !RC_VECOUT
  //
  //  The scalar shape. Reachable in a normal compile, and the reference the
  //  generated kernel is checked against: this side divides with the integer
  //  `/` and the vector side divides in float32 with a fixup, so a build where
  //  the two agree byte for byte over 100 MB is a build where the float path
  //  is right. RC_VEC=0 puts the coding pass on this class; t_matrix.sh runs
  //  it against the reference stream.
  //

  // The 0..2 bytes this symbol's threshold asks the state to give up, lowest
  // byte first into ascending addresses -- the byte order the note at the top
  // of the file describes.
  void rc_Renorm( uint _f ) {
    uint i, n = 0;
    const uint x0 = rx;                  // the bytes to emit come from here
    const uint xm = _f << XSH;
    while( rx >= xm ) { n++; rx >>= 8; }
    for( i=0; i<n; i++ ) put( byte(x0>>(8*i)) );
  }

  INLINE
  void rc_Process( uint freq, uint& _bit ) {
    uint _b = _bit;

    if( f_DEC==0 ) {
      const uint _f = _b ? SCALE-freq : freq;
      const uint _c = _b ? freq : 0;
      rc_Renorm( _f );
      const uint _q = rx/_f;
      rx = (_q<<SCALElog) + (rx-_q*_f) + _c;
    }

    if( f_DEC==1 ) {
      // The refill is a little-endian WORD, not two bytes: the encoder put
      // x&0xFF below (x>>8)&0xFF, so shifting them in one at a time would
      // swap them. At most one refill is needed -- x is at least f*(L/M)
      // >= L>>KLOG after a step, so 16 bits always suffice.
      //
      // It sits BEFORE the step, not after it, which is the same place --
      // between two steps -- but reached one step earlier. See the vector
      // twin for why that is worth anything.
      if( rx<RANSL ) {
        const uint sh = (rx<(RANSL>>8)) ? 16 : 8;
        uint w = get(); if( sh==16 ) w = (w<<8) | get();
        rx = (rx<<sh) | w;
      }
      const uint _s = rx & mSCALE;
#if RC_RANS_DECS==1
      // The textbook form: pick f and the cumulative start from the bit, then
      // one multiply-add. Two selects instead of two arms.
      _b = (_s>=freq);
      const uint _f = _b ? SCALE-freq : freq;
      const uint _c = _b ? freq : 0;
      rx = _f*(rx>>SCALElog) + _s - _c;
#elif RC_RANS_DECS==2
      // The same, with the selects as mask arithmetic: m = -bit, and
      // f = freq + (m & (SCALE-2*freq)) is freq or SCALE-freq without a cmov.
      _b = (_s>=freq);
      const uint _m = uint(-int(_b));
      const uint _f = freq + (_m & (SCALE-2*freq));
      rx = _f*(rx>>SCALElog) + _s - (freq & _m);
#else
      const uint _a = freq * (rx>>SCALElog);
      _b = (_s>=freq);
      rx = _b ? (rx - freq - _a) : (_a + _s);
#endif
    }

    _bit = _b;
  }

  // RC_RANS_SPLIT cuts the above in two at the model, the way rc.inc's
  // RC_DEC_SPLIT cuts the range coder's.
  //
  // rc_Process does both halves of a bit: it decides the bit, which needs the
  // model's p, and it updates the state, which also needs p. Because p arrives
  // one lane at a time -- a byte's eight bits are one walk of the context tree
  // -- the whole thing runs at the model's pace, scalar, and the disassembly
  // says so: 130 vector instructions in 1099, nearly all of them vmovdqa and
  // vpextrd moving lane state across a boundary rather than computing on it.
  //
  // The cut is possible because the bit does NOT need the multiply:
  //
  //     bit = (x & (M-1)) >= p
  //
  // has no p in the left side. So `s` can be produced for every lane up front,
  // the walk becomes a load, a compare and the context step, and the multiply
  // and the select go to a vector pass afterwards. What makes it LEGAL is
  // model1.inc's wavefront: a group is RCNUM bit-slots and lane j takes
  // exactly one of them, so no lane's x moves between the top of the group and
  // its own step -- renormalising every lane at the top is the same
  // renormalisation, at the same point in that lane's sequence.
  //
  //   rc_DecPre   renormalise, s = x & (M-1)   -- every lane, vectorisable
  //   the model   bit = (s >= p), ctx = ctx*2+bit  -- scalar, serial
  //   rc_DecUpd   the state update              -- every lane, vectorisable
  void rc_DecPre( void ) {
    if( rx<RANSL ) {
      const uint sh = (rx<(RANSL>>8)) ? 16 : 8;
      uint w = get(); if( sh==16 ) w = (w<<8) | get();
      rx = (rx<<sh) | w;
    }
    sv = rx & mSCALE;
  }
  void rc_DecUpd( uint freq, uint _b ) {
    const uint _a = freq * (rx>>SCALElog);
    rx = _b ? (rx - freq - _a) : (_a + sv);
  }
  // RC_RANS_SPLIT=2: the whole step minus the renormalisation, which
  // rc_DecPre already did for every lane. Keeps the bit and the update
  // together -- so no per-lane p or bit has to be carried between passes --
  // and only takes the refill branch out of the unrolled walk.
  void rc_DecStep( uint freq, uint& _bit ) {
    const uint _a = freq * (rx>>SCALElog);
    const uint _b = (sv>=freq);
    rx = _b ? (rx - freq - _a) : (_a + sv);
    _bit = _b;
  }

  void rc_Init( void ) {
    if( f_DEC==0 ) rx = RANSL;
    if( f_DEC==1 ) { rx = 0; for( uint i=0; i<4; i++ ) rx = (rx<<8) | get(); }
  }

  // The flush is the whole state, low byte first like the renorm above, so
  // the decoder's four-byte init is one little-endian load.
  void rc_Quit( void ) {
    if( f_DEC==0 ) for( uint i=0; i<4; i++ ) put( byte(rx>>(8*i)) );
  }

#else // RC_VECOUT ---------------------------------------------------------
  //
  //  The vector-shape twin. Reachable only through mk_kernel.sh, which
  //  resolves this file with RC_VECOUT=1 and turns the lane state into
  //  [RCNUM] arrays -- see rc_soa.pl. It addresses the output through the
  //  flat cursor the generated prelude declares (tmpbase + tmpptr) rather
  //  than through RC_IO, and every method is void and single-statement-
  //  shaped, because a value-returning function cannot become a macro.
  //  A scalar build never compiles this branch, and the arithmetic is line
  //  for line the scalar coder's -- t_matrix.sh checks the streams byte for
  //  byte against RC_VEC=0.
  //

  // Encode. The renorm and the rANS step, in one body, because splitting them
  // would put the byte count in lane state for no reason.
  //
  // The store: the whole state as one dword at the cursor, of which the
  // counted increment commits the low 0..2 bytes. The rest is staging the
  // next store overwrites, which is what writing upwards buys -- see the byte
  // order note at the top. rc.inc's staging window with the sign flipped, so
  // the same RC_STAGE_CL/RC_FLUSHALL machinery commits it.
  void rc_Renorm( uint _f ) {
    // The SHIFT count, 0/8/16, from a nested ternary -- not a byte count
    // summed out of two compares. Everything downstream of the selection
    // wants the shift and only the cursor wants the count, and the ternary
    // is two masked moves against the sum's two mask extractions, a
    // sign-shift and a subtract. That is rc_config.inc's RC_ENC_NSEL, which
    // measured +2.3% on the range coder, applied here from the start.
    //
    // It goes into dsh, the lane-state slot the decoder keeps its shift in,
    // rather than into a local: rc_Process needs it back after this call, to
    // scale the quotient it started before the shift was known.
    const uint _m = _f << XSH;
    dsh = (rx>=_m) ? ((rx>>8)>=_m ? 16 : 8) : 0;
    // stn only ever gets compared against zero (model.inc), so it takes the
    // shift rather than the count and saves the fixup.
    RC_STAGE_CL( rcidx, tmpptr, rx, dsh );
    tmpptr += dsh>>3;
    rx >>= dsh;
  }

  INLINE
  void rc_Process( uint freq, uint& _bit ) {
    uint _b = _bit;

    if( f_DEC==0 ) {
      const uint _f = _b ? SCALE-freq : freq;
      const uint _c = _b ? freq : 0;
      // The division, in float32 and through RANS_DIV, which model.inc
      // defines because it is the compile's business and not the generator's.
      // It runs AFTER the renormalisation, on a state below f<<XSH, so the
      // quotient is below 1<<XSH and the relative error float32 carries over
      // it is what RANS_DIV's bias is sized against: the truncated quotient is
      // the true one or exactly one below it, never above, and one correction
      // is always enough. misc/rans_div.cpp checks that for every f.
      //
      // The divide can also be moved AHEAD of the renormalisation -- the
      // quotient is the same one, and the shift comes off the float's exponent
      // afterwards -- which takes the two compares that pick the shift off the
      // front of the chain. It measures slower; model.inc has the numbers.
      //
      // The correction does not need a branch or even a second quotient:
      // adding one to q and taking f off r adds exactly M-f to the result,
      // so it folds into the reconstruction as a masked add.
      //
      // RANS_DIV takes the two integers and returns the integer quotient:
      // whether it converts to float at all, and to what, is the compile's
      // business and not this file's. Any correct division gives the same
      // stream, which is what makes the alternatives measurable against each
      // other -- model.inc has the six that were.
      rc_Renorm( _f );
      const uint _q = RANS_DIV( rx, _f );
      const uint _r = rx - _q*_f;
      const uint _u = uint(-int(_r>=_f));
      rx = (_q<<SCALElog) + _r + _c + ((SCALE-_f)&_u);
    }

    if( f_DEC==1 ) {
      // The refill, BEFORE the step rather than after it. It is the same
      // place -- rANS renormalises between two steps -- but the branch then
      // tests a state the previous call finished with, instead of one this
      // call's multiply has just produced, so it resolves a whole step
      // earlier. That is where rc.inc puts its refill too.
      //
      // The outer test is what the shape is for, and its rate is not a
      // guess: a byte-renormalised rANS refills exactly once per byte of
      // output, so the branch fires bpc/8 of the time. Measured over 20 MB
      // of enwik8 at 5.01 bpc: 12520077 firings in 160000322 coded bits,
      // 7.83%, and 1.000 bytes per firing -- the 16-bit case, x below L>>8,
      // happened 19 times in 12.5 million. The inner select is there for
      // correctness, not for the common path.
      //
      // The 0.99 in the hint is deliberately NOT that measured rate: it is
      // what makes clang put the body out of line rather than in the fall-
      // through path, and the difference between 0.92 and 0.99 is 5.3% of
      // decode -- the loop is big enough that its footprint matters. Inside, dsh is the SHIFT count, 0/8/16, not a byte
      // count -- it is what both the shift and the cursor's increment want.
      // The load is the reason for the byte order at the top of this file:
      // one little-endian word at the cursor, masked, no swap and no second
      // load. rc.inc's decode renorm, mirrored; misc/dec_renorm.md is where
      // that shape was measured.
      if( __builtin_expect_with_probability( rx<RANSL, 0, 0.99 ) ) {
        dsh = (rx<(RANSL>>8)) ? 16 : 8;
        rx <<= dsh;
        rx |= (uint&)tmpbase[tmpptr-3] >> (32-dsh);
        tmpptr -= dsh>>3;
      }
      // One multiply, and it does not wait for the bit. The textbook step is
      //   f = bit ? M-p : p;  c = bit ? p : 0;  x = f*(x>>15) + s - c
      // which puts the multiply AFTER the comparison that picks f, and needs
      // M-p and two selects to get there. But (M-p)*(x>>15) + s - p is
      // x - p - p*(x>>15), because (x>>15)<<15 is x-s -- so both arms are the
      // one product p*(x>>15), which depends on nothing but the counter and
      // the old state and can issue while the compare is still in flight.
      // Two selects, a constant and a subtract go with it.
      const uint _s = rx & mSCALE;
#if RC_RANS_DECS==1
      // The textbook form: pick f and the cumulative start from the bit, then
      // one multiply-add. Two selects instead of two arms.
      _b = (_s>=freq);
      const uint _f = _b ? SCALE-freq : freq;
      const uint _c = _b ? freq : 0;
      rx = _f*(rx>>SCALElog) + _s - _c;
#elif RC_RANS_DECS==2
      // The same, with the selects as mask arithmetic: m = -bit, and
      // f = freq + (m & (SCALE-2*freq)) is freq or SCALE-freq without a cmov.
      _b = (_s>=freq);
      const uint _m = uint(-int(_b));
      const uint _f = freq + (_m & (SCALE-2*freq));
      rx = _f*(rx>>SCALElog) + _s - (freq & _m);
#else
      const uint _a = freq * (rx>>SCALElog);
      _b = (_s>=freq);
      rx = _b ? (rx - freq - _a) : (_a + _s);
#endif
    }

    _bit = _b;
  }

  // RC_RANS_SPLIT cuts the above in two at the model, the way rc.inc's
  // RC_DEC_SPLIT cuts the range coder's.
  //
  // rc_Process does both halves of a bit: it decides the bit, which needs the
  // model's p, and it updates the state, which also needs p. Because p arrives
  // one lane at a time -- a byte's eight bits are one walk of the context tree
  // -- the whole thing runs at the model's pace, scalar, and the disassembly
  // says so: 130 vector instructions in 1099, nearly all of them vmovdqa and
  // vpextrd moving lane state across a boundary rather than computing on it.
  //
  // The cut is possible because the bit does NOT need the multiply:
  //
  //     bit = (x & (M-1)) >= p
  //
  // has no p in the left side. So `s` can be produced for every lane up front,
  // the walk becomes a load, a compare and the context step, and the multiply
  // and the select go to a vector pass afterwards. What makes it LEGAL is
  // model1.inc's wavefront: a group is RCNUM bit-slots and lane j takes
  // exactly one of them, so no lane's x moves between the top of the group and
  // its own step -- renormalising every lane at the top is the same
  // renormalisation, at the same point in that lane's sequence.
  //
  //   rc_DecPre   renormalise, s = x & (M-1)   -- every lane, vectorisable
  //   the model   bit = (s >= p), ctx = ctx*2+bit  -- scalar, serial
  //   rc_DecUpd   the state update              -- every lane, vectorisable
  void rc_DecPre( void ) {
    if( __builtin_expect_with_probability( rx<RANSL, 0, 0.99 ) ) {
      dsh = (rx<(RANSL>>8)) ? 16 : 8;
      rx <<= dsh;
      rx |= (uint&)tmpbase[tmpptr-3] >> (32-dsh);
      tmpptr -= dsh>>3;
    }
    sv = rx & mSCALE;
  }
  void rc_DecUpd( uint freq, uint _b ) {
    const uint _a = freq * (rx>>SCALElog);
    rx = _b ? (rx - freq - _a) : (_a + sv);
  }
  // RC_RANS_SPLIT=2: the whole step minus the renormalisation, which
  // rc_DecPre already did for every lane. Keeps the bit and the update
  // together -- so no per-lane p or bit has to be carried between passes --
  // and only takes the refill branch out of the unrolled walk.
  void rc_DecStep( uint freq, uint& _bit ) {
    const uint _a = freq * (rx>>SCALElog);
    const uint _b = (sv>=freq);
    rx = _b ? (rx - freq - _a) : (_a + sv);
    _bit = _b;
  }

  void rc_Init( void ) {
    if( f_DEC==0 ) {
      rx = RANSL;
      // Seed the staging where the first store would land anyway, so a commit
      // before the first rc_Renorm -- there is none today -- cannot write
      // somewhere it should not. A no-op when the build is not staging, and
      // encoder-only in any case: in the decoder tmpptr indexes the INPUT row.
      RC_STAGE_INIT( rcidx, tmpptr );
    }
    if( f_DEC==1 ) {
      rx = (uint&)tmpbase[tmpptr-3];
      tmpptr -= 4;
    }
  }

  // The flush is the whole state: the same dword store the renorm makes, with
  // all four bytes committed instead of none.
  void rc_Quit( void ) {
    if( f_DEC==0 ) {
      RC_STAGE_CL( rcidx, tmpptr, rx, 4 );
      tmpptr += 4;
      RC_FLUSH_ONE( rcidx );
    }
  }

#endif // RC_VECOUT

};
