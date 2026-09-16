// Tangelo as an embeddable source of byte probabilities.
//
// The programs in this repository are their own compressors: main.inc codes
// what ByteModel predicts. This header is the other way round - it packages the
// same machinery for a *host* program that has its own arithmetic coder and
// just wants P(next byte) over 256 values, which is what a PPM or PPMD model
// hands a mixer. tf003/ is the first such host: the Tangelo model in place of
// the PPMD one that feeds its transformer and its mixer.
//
// Everything is inside namespace tangelo, because a host of that kind has its
// own SCALE, its own Rangecoder and its own idea of what "the model" is, and
// none of them should have to move. The host needs to have defined, before
// including this: the word/uint/byte/qword typedefs, INLINE, NOINLINE, ALIGN,
// <stdio.h>, <stdlib.h> and <math.h>.
//
//   tangelo::ByteSource bs;
//   bs.Init( cmap );          // cmap[256], nonzero where the byte can occur
//   bs.Predict( probs );      // P(next byte) into probs[256], sums to 1
//   ... the host codes a byte c ...
//   bs.Update( c );           // advance the model over c's eight bits
//   bs.Predict( probs );      // and so on
//
// -DTRACK_VERIFY=<n> builds a self-checking version, which is what to reach for
// when embedding this somewhere new: it compares every byte of model state
// across each walk, and cross-checks the walk's predicted code length against
// the model's real one. Neither failure breaks a round trip, so neither is
// visible without it.
//
// Cost: one speculative walk per byte. On English text that is about 73 model
// steps and 21 000 journal cells (SPEED.md), which is far more than a PPMD
// lookup - the point is what it buys, a distribution from a model that only
// predicts bits.

#ifndef TANGELO_BM_INC
#define TANGELO_BM_INC

namespace tangelo {

// The probability scale ByteSource reports frequencies on, before the caller
// normalises them. Named PSCALE because track.inc and bytemodel.inc use it.
enum{ PSCALE = 1<<16 };

// How deep the speculative walk goes before it gives up on a prefix; 0x80000
// is tangelo_s.cpp's value, chosen there because Tangelo's predictions are
// sharp enough that the default 0x90000 cuts into the distribution that
// matters. See section 4.3 of ALGORITHM.md for the trade curve.
#ifndef PRUNE_LOG
 #define PRUNE_LOG 0x80000
#endif

// The Coder in coder_tangelo.inc owns a range coder, because this repository's
// own programs code with it. Here the host does the coding, so this stands in
// for it: only the model half of Coder is ever used. It stays a member rather
// than being compiled out so that coder_tangelo.inc - which is the one
// definition of how a bit is fed to Tangelo - needs no variant.
struct Rangecoder {
  void StartEncode( FILE* ) {}
  void StartDecode( FILE* ) {}
  void rc_Quit( void ) {}
};

// The journal, and the model marking its own writes into it (write.inc): one
// translation unit, no assembly and no instrumenter, which is what makes the
// model embeddable in someone else's program at all.
#define TRACK_SRC 1
#include "log2lut.inc"
#include "track.inc"
#include "write.inc"

#include "tangelo/tangelo.inc"
#include "coder_tangelo.inc"

#include "bytemodel.inc"

// The globals the model shares. tangelo_w.cpp has to declare these in one
// translation unit and define them in another; here there is only one, and
// the namespace keeps them from colliding with the host's.
int y = 0;
int bpos = 0;

Random  rnd;
Ilog    ilog;
Ptable  pt;
Stretch stretch;

// One range of model state. The list below is the whole of it, and the journal
// needs it twice over: its extent is the address window a cell's 30-bit offset
// indexes (track.inc), and its contents are what -DTRACK_VERIFY compares.
// Same list as tangelo_s.cpp's TRACK_STATE_RANGES.
struct VRange { const char* name; byte* p; uint n; };

struct ByteSource {

  ByteModel<Coder> M;

  VRange vr_[4];
  uint   nvr_;

  void Declare( void ) {
    nvr_ = 0;
    vr_[nvr_].name="Coder"; vr_[nvr_].p=(byte*)&M.E; vr_[nvr_].n=sizeof(Coder);  nvr_++;
    vr_[nvr_].name="y";     vr_[nvr_].p=(byte*)&y;   vr_[nvr_].n=sizeof(y);      nvr_++;
    vr_[nvr_].name="bpos";  vr_[nvr_].p=(byte*)&bpos;vr_[nvr_].n=sizeof(bpos);   nvr_++;
    vr_[nvr_].name="rnd";   vr_[nvr_].p=(byte*)&rnd; vr_[nvr_].n=sizeof(rnd);    nvr_++;

    byte* lo = vr_[0].p;
    byte* hi = vr_[0].p + vr_[0].n;
    for( uint k=1; k<nvr_; k++ ) {
      if( vr_[k].p           < lo ) lo = vr_[k].p;
      if( vr_[k].p+vr_[k].n  > hi ) hi = vr_[k].p+vr_[k].n;
    }
    Track_Base( lo, hi );
  }

  // cmap[i] nonzero where byte i can occur in the input. Bytes it rules out
  // are dropped from the distribution exactly - the walk never enters a
  // subtree that leads only to them - which is worth both size and time.
  // Passing 0 means "every byte is possible".
  void Init( const char* cmap ) {
    Declare();
    Track_Init();
    M.Init( COMPRESS, 0 );
    if( cmap ) {
      byte m[256];
      for( uint i=0; i<256; i++ ) m[i] = cmap[i] ? 1 : 0;
      M.SetAlphabet( m );
    }
#ifdef TRACK_VERIFY
    vsnap_ = 0; vpos_ = 0;
#endif
  }

  // P(next byte) over all 256 values, normalised to sum to 1, and zero where
  // the alphabet map says the byte cannot occur.
  void Predict( float* probs ) {
    if( !M.AtByteBoundary() ) {
      fprintf( stderr, "tangelo: Predict() called mid-byte - the distribution\n"
                       "  it produces would be for the wrong bits.\n" );
      exit(8);
    }
#ifdef TRACK_VERIFY
    int checking = verify_this();
    if( checking ) verify_save();
#endif
    M.Predict();
#ifdef TRACK_VERIFY
    if( checking ) verify_check();
#endif
    // The walk must have journaled and rolled back something, or every
    // speculative branch has been leaking into the real model. That failure
    // is invisible in a round trip - both sides corrupt the model identically
    // - and shows up only as bad compression, so check it once.
    if( trk_undone==0 ) {
      fprintf( stderr, "tangelo: the speculative walk journaled nothing.\n" );
      exit(6);
    }
    float inv = 1.0f / float(M.Total());
    for( uint i=0; i<256; i++ ) probs[i] = float(M.Freq(i)) * inv;
  }

  // The eight real steps for the byte that was actually coded.
  void Update( uint c ) {
#ifdef TRACK_VERIFY
    // The other half of the self-check: the walk said this byte would cost
    // CodeLength(c), and the model is about to say what it really costs. They
    // must agree exactly, unless the byte's prefix was pruned. This is what
    // catches the simulated step and the real one drifting apart - a stale
    // arena, a layout the host and the model disagree about - which the state
    // snapshot alone would not see.
    uint sum = 0;
    for( int j=7; j>=0; --j ) {
      uint bit = (c>>j)&1;
      uint pp  = M.E.encode_sim( bit );
      sum += bit ? LOG2(pp) : LOG2(SCALE-pp);
    }
    if( (M.CodeLength(c)!=0xFFFFF) && (sum!=M.CodeLength(c)) ) {
      fprintf( stderr, "tangelo: at byte %u the walk predicted %u for symbol %u "
                       "but the model then produced %u\n", vpos_, M.CodeLength(c), c, sum );
      exit(7);
    }
    vpos_++;
#else
    M.Update( c );
#endif
  }

  // What the walk said this byte would cost, 2^16 * bits - the interface an
  // optimal parser wants, and free once Predict() has run.
  uint CodeLength( uint c ) const { return M.CodeLength(c); }

#ifdef TRACK_VERIFY
  // Built with -DTRACK_VERIFY=<n>: for the first n bytes (and, with
  // -DTRACK_VERIFY_EVERY=<k>, every k-th after that) every byte of the state
  // declared above is copied aside, the walk runs, and the state is compared.
  // If any write escaped the journal, UNDO() leaves the model changed and this
  // names the offset. Nothing else finds that: encoder and decoder corrupt the
  // model identically, so the round trip still passes and only the compressed
  // size suffers.
 #ifndef TRACK_VERIFY_EVERY
  #define TRACK_VERIFY_EVERY 0
 #endif
  byte* vsnap_;
  uint  vpos_;

  int verify_this( void ) {
    return (vpos_<TRACK_VERIFY) || (TRACK_VERIFY_EVERY && ((vpos_%TRACK_VERIFY_EVERY)==0));
  }

  void verify_save( void ) {
    if( vsnap_==0 ) {
      qword total=0; for( uint k=0; k<nvr_; k++ ) total += vr_[k].n;
      vsnap_ = (byte*)malloc(total);
      if( vsnap_==0 ) fprintf(stderr,"tangelo: verify buffer\n"), exit(7);
    }
    byte* q = vsnap_;
    for( uint k=0; k<nvr_; k++ ) memcpy( q, vr_[k].p, vr_[k].n ), q+=vr_[k].n;
  }

  void verify_check( void ) {
    byte* q = vsnap_;
    for( uint k=0; k<nvr_; k++ ) {
      for( uint b=0; b<vr_[k].n; b++ ) if( q[b]!=vr_[k].p[b] ) {
        fprintf( stderr, "tangelo: UNDO() did not restore %s+%u at byte %u: %02X -> %02X\n"
                         "  a memory write in the model step is escaping the journal\n",
                 vr_[k].name, b, vpos_, q[b], vr_[k].p[b] );
        exit(7);
      }
      q += vr_[k].n;
    }
  }
#endif
};

} // namespace tangelo

#endif // TANGELO_BM_INC
