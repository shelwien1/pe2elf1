// ByteModel - a bitwise model presented as a source of byte distributions.
//
// This is the whole point of the machinery: given a model that predicts one bit
// at a time and updates itself as it goes, produce P(next byte) over all 256
// values, by running the model's compiled step speculatively down the prefix
// tree and rolling its memory writes back through the journal in track.inc.
//
//   SetAlphabet(cmap)  restrict the alphabet; dead subtrees are then skipped
//   Predict()          walk the tree, fill freq[0..255] and the total
//   Update(c)          advance the model for real, over c's eight bits
//   CodeLength(c)      what the walk said c would cost, 2^16 * bits
//
// Predict()/Update() is the coding interface; Predict()/CodeLength() is the
// interface an optimal parser wants, and it is the same walk either way.
//
// Everything here is model-agnostic. A Coder supplies:
//   P()                 the probability the next coded bit will use, 12-bit
//   encode_sim(bit)     returns P(), then advances the model past that bit
//   at_byte_boundary()  true when the next bit is a byte's first
// and the instrumented copy of encode_sim is reached through the extern "C"
// declaration in track.inc.

//////////////////////////// the walk ////////////////////////////

// WALK<ctx,depth>::go - the model is positioned at tree node ctx, at depth.
// Fill the code length of both children, then descend into the ones worth
// visiting. ctx and depth are template parameters so the whole walk unrolls
// into straight-line code with no tree bookkeeping at run time.
//
// Note what does NOT need the model advanced: both children's code lengths come
// from the prediction AT this node, which P() gives without any side effect. So
// a child that is dead or too improbable costs nothing at all - no step, no
// journaling, no rollback. (The earlier shape of this walk advanced into a child
// and tested afterwards, which paid a full model step for every child of every
// visited node, including the ones it then declined to explore.)
template< class Coder, uint ctx, int depth >
struct WALK {
  INLINE
  inline static void go( Coder& E, uint (&clen)[0x200], const byte (&live)[0x200] ) {
    uint p  = E.P();
    uint c0 = clen[ctx] + LOG2(SCALE-p);   // this bit is a 0
    uint c1 = clen[ctx] + LOG2(p);         // this bit is a 1
    clen[ctx*2+0] = c0;
    clen[ctx*2+1] = c1;

    // A prefix is explored while its accumulated code length stays above
    // PRUNE_LOG per bit - see §4.3 of ALGORITHM.md. live[] is the exact half of
    // the same test: a subtree with no reachable symbol under it is never worth
    // entering, whatever its probability.
    if( live[ctx*2+0] && (c0>PRUNE_LOG*(depth+1)) ) {
      NEST();
      encode_sim( &E, 0 );
      WALK<Coder,ctx*2+0,depth+1>::go( E, clen, live );
      UNDO();
    }

    if( live[ctx*2+1] && (c1>PRUNE_LOG*(depth+1)) ) {
      NEST();
      encode_sim( &E, 1 );
      WALK<Coder,ctx*2+1,depth+1>::go( E, clen, live );
      UNDO();
    }
  }
};

// Depth 7: the children are the 256 leaves. Nothing is predicted after the
// eighth bit inside this byte, so the model is not advanced here at all - which
// is what keeps half of the tree's nodes out of the journal entirely.
template< class Coder, uint ctx >
struct WALK<Coder,ctx,7> {
  INLINE
  inline static void go( Coder& E, uint (&clen)[0x200], const byte (&live)[0x200] ) {
    uint p = E.P();
    clen[ctx*2+0] = clen[ctx] + LOG2(SCALE-p);
    clen[ctx*2+1] = clen[ctx] + LOG2(p);
  }
};

//////////////////////////// the model ////////////////////////////

template< class Coder >
struct ByteModel {

  Coder E;                 // the bitwise model, and the range coder with it
  uint  clen[0x200];       // per tree node: 2^16 * sum of log2(p) along its path
  uint  freq[1+256];       // the distribution; freq[256] is the total
  byte  live[0x200];       // 1 where a subtree can still contain a symbol

  void Init( Mode mode, FILE* f ) {
    E.rc_Init( mode, f );
    for( uint j=0; j<0x200; j++ ) live[j] = 1;
  }

  // Bytes whose flag is 0 never occur in this input, so their leaves get no
  // frequency and the walk never enters a subtree that leads only to them.
  // Unlike the code-length pruning this costs nothing in compression: it
  // removes probability mass that was pure waste.
  void SetAlphabet( const byte cmap[256] ) {
    uint j;
    for( j=0; j<256; j++ ) live[0x100+j] = cmap[j] ? 1 : 0;
    for( j=0xFF; j>=1; j-- ) live[j] = live[2*j+0] | live[2*j+1];
  }

  void Predict( void ) {
    uint j;

    for( j=0; j<0x200; j++ ) clen[j] = 0xFFFFF;   // "not evaluated": ~16 bits
    clen[1] = 0;

    // The one place journaling is on. Outside it the model's writes are
    // permanent: Update() below, and everything before the first byte. The
    // track.pl route ignores this - its two copies of the step differ in the
    // code, not in a flag - see write.inc.
    trk_on = true;
    WALK<Coder,1,0>::go( E, clen, live );
    trk_on = false;

    uint total = 0;
    for( j=0; j<256; j++ ) {
      uint f = 0;
      if( live[0x100+j] ) {
        uint r = LOG2(SCALE)*8 - clen[0x100+j];   // 2^16 * cost of j, in bits
        if( r>0xFFFFF ) r = 0xFFFFF;              // floor the frequency at 1
        f = unlog[r];
      }
      freq[j] = f;
      total  += f;
    }
    freq[256] = total;
  }

  // The eight real steps, whose writes are permanent. This is the model's own
  // copy of encode_sim, not the instrumented one the walk calls.
  void Update( uint c ) {
    for( int j=7; j>=0; --j ) E.encode_sim( (c>>j)&1 );
  }

  uint Total( void )                const { return freq[256]; }
  uint Freq( uint c )               const { return freq[c]; }
  uint CodeLength( uint c )         const { return clen[0x100+c]; }
  int  AtByteBoundary( void )             { return E.at_byte_boundary(); }

  // Cumulative frequency below c, for the encoder.
  uint Cum( uint c ) const {
    uint low = 0;
    for( uint j=0; j<c; j++ ) low += freq[j];
    return low;
  }

  // The inverse, for the decoder: the symbol whose interval holds code, and the
  // cumulative frequency below it. The c<255 bound is only a guard - for a
  // well-formed stream rc_GetFreq() returns a value below the total, so the scan
  // stops on its own; a corrupt one can hand it anything.
  uint Symbol( uint code, uint& low ) const {
    uint c;
    for( c=0,low=0; (c<255) && (low+freq[c]<=code); c++ ) low += freq[c];
    return c;
  }

};
