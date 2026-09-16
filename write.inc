#ifndef WRITE_INC
#define WRITE_INC

// W(x) - "this expression writes model state".
//
// The model's writes reach the journal in two different ways in this
// repository, and this header is the seam between them.
//
//   tangelo_w  compiles the model step twice and lets track.pl insert a
//              journaling call in front of every store in one of the two copies
//              (ALGORITHM.md section 5). It needs the model source to say
//              nothing about writes at all, so there W(x) is just (x).
//
//   tangelo_s  (-DTRACK_SRC) journals the writes from the source instead: every
//              write to model state is spelled W(x) = ... and appends its own
//              journal cell. One translation unit, one copy of the step, no
//              assembly and no perl.
//
// The rule the marked-up source has to keep is weaker than track.pl's, and
// worth stating exactly: a piece of state must be journaled at least once
// before its first modification inside a speculative step. Journaling it more
// often is merely wasteful - UNDO() walks the cells backwards, so the oldest
// value wins - which is what lets W_block() cover a whole array once per step
// and leave the writes into it unmarked (mixer1.inc).
//
// Missing a write is the failure this design can have and track.pl's cannot,
// and it is invisible in a round trip: encoder and decoder corrupt the model
// identically and still agree. -DTRACK_VERIFY is what catches it - it compares
// every byte of declared model state across each walk - so it is not optional
// here in the way it is for tangelo_w.

#ifdef TRACK_SRC

// One narrow cell, in track.inc's format: a 30-bit offset into the model state
// with the width in its top two bits, and the overwritten bytes.
INLINE inline void trk_cell( const void* p, uint n ) {
  Cell* c = trkptr;
  uint  o = uint( (const byte*)p - trk_base );
  if( n==4 ) { c->off = o|T4; c->val = *(const uint*)p; }
  else
  if( n==2 ) { c->off = o|T2; c->val = *(const word*)p; }
  else       { c->off = o|T1; c->val = *(const byte*)p; }
  trkptr = c+1;
}

// A block of n bytes as one record: n/8 cells of payload copied verbatim and a
// footer naming the offset and the width, with any odd tail as narrow cells.
// Eight bytes of state - a pointer, which the ContextMap keeps 66 of - is the
// smallest record and costs two cells, exactly what two narrow cells would.
INLINE inline void trk_block( const void* p, uint n ) {
  const byte* q = (const byte*)p;
  if( n>=8 ) {
    uint nb = n & ~7u, nc = nb>>3;
    Cell* c = trkptr;
    const qword* src = (const qword*)q;
    for( uint w=0; w<nc; w++ ) ((qword*)c)[w] = src[w];
    c[nc].off = uint( q - trk_base ) | TW;
    c[nc].val = nb;
    trkptr = c + nc + 1;
    q += nb; n -= nb;
  }
  while( n>=4 ) { trk_cell(q,4); q+=4; n-=4; }
  if( n>=2 )    { trk_cell(q,2); q+=2; n-=2; }
  if( n )         trk_cell(q,1);
}

// The switch is a global rather than a second copy of the step: outside the
// speculative walk the model's writes are permanent and must not be recorded.
// It is set in ByteModel::Predict() and nowhere else, so it changes twice per
// input byte and the branch below is as predictable as a branch gets.
INLINE inline void trk_save( const void* p, uint n ) {
  if( trk_on ) { if( n<8 ) trk_cell(p,n); else trk_block(p,n); }
}

template< class T > INLINE inline T& W( T& x ) { trk_save( &x, sizeof(T) ); return x; }

// "Everything in [p,p+n) is about to change." Saves it once; the writes into it
// are then plain writes. Worth it where a model rewrites a whole array in a
// step - the mixer's weight rows - and wrong anywhere the array is only partly
// written and the rest must keep the value a LATER step gave it.
INLINE inline void W_block( const void* p, uint n ) { if( trk_on ) trk_block(p,n); }

// The walk in bytemodel.inc calls encode_sim(&E,bit). For tangelo_w that is the
// extern "C" function in the instrumented translation unit; here it is the
// Coder's own step, which journals itself.
//
// NOINLINE for the same reason tangelo_w gets it for free: the walk is 254
// unrolled call sites, and the model step is a few thousand instructions. Let
// it inline and the compiler builds 254 copies of Tangelo - a five-minute
// compile, a 2.4 MB binary and an instruction cache with no chance. One
// out-of-line copy is also what makes the two programs comparable.
template< class Coder > NOINLINE uint encode_sim( Coder* E, int bit ) { return E->encode_sim(bit); }

#else

// Macros rather than an identity function, so that a marker this program does
// not use cannot change what it compiles to: with them, track.pl instruments
// exactly the same 146 stores in exactly the same places as it did before the
// model said anything about its writes. An inline T& W(T&) is a no-op in
// principle and was not in practice - binding the reference moved four stores
// and spilled thirty more.
#define W(x)         (x)
#define W_block(p,n) ((void)0)

#endif

#endif // WRITE_INC
