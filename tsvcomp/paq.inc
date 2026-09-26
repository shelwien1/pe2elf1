// -------------------------------------------------------------
// paq.inc -- the paq-style block of tsvcomp, made of the framework's parts:
//
//   HashCtr    one hashed context model: 2^TB Counter<CP_H0> cells
//              (sh_counter.inc, knobs IDX/tc_model-H0.idx) and a 32-bit
//              check per slot.  The slot of (context hash, bit node) is
//              the cell that maps this context's history to a probability;
//              a slot whose check does not match is re-initialized
//              (Counter::Init, the prior) and taken over.
//   groups     the NCX model logits (hand-written contexts, then the
//              masked E0 ones) and the C0/C1/C2 logits, NGRP groups of
//              GN inputs, each mixed by a MixN<CP_X0,GN> cell (sh_mixN.inc)
//              selected by X0_MakeCx(group, k, stage, exponent step)
//   final      MixN<CP_X1,NGRP> over the group outputs, selected by
//              X1_MakeCx(k, stage, rows since the last change, change flags)
//
// Every cell learns end to end, like the rest of the coder: the final
// error is chained back through the mixers (their Pred::d) into the
// counters' C_Update.
// -------------------------------------------------------------

#include <vector>

template<class CP> struct HashCtr {
  std::vector<uint32_t>    chk;
  std::vector<Counter<CP>> c;
  qword mask = 0;
  void init( int bits ) {
    chk.assign( size_t(1) << bits, 0 );
    c.resize( size_t(1) << bits );
    for( auto& x : c ) x.Init();
    mask = (qword(1) << bits) - 1;
  }
  Counter<CP>& get( qword h ) {
    size_t i = size_t( h & mask );
    uint32_t k = uint32_t( h >> 32 ) | 1;
    if( chk[i] != k ) { chk[i] = k; c[i].Init(); }
    return c[i];
  }
};

// NCX hashed context models: NHC hand-written contexts (H[] in
// tsvcomp.cpp's code_value), then the E0_NE masked ones (IDX/tc_model-E0.idx,
// every mask zero to start with: opt.pl picks the bits); then the three
// counter tables C0, C1, C2 as extra mixer inputs
static const int NHC = 22, NCX = NHC + E0_NE, NIN = NCX + 3;
static_assert( E0_NE == 10, "GRP below lays out 10 masked models" );
static const int NGRP = 7, GN = 5;
static const int GRP[NGRP][GN] = {
  {  0,  1,  2,  3,  4 },   // target history: order 0, rows since change, last jump(s), level
  {  5,  6,  7, 10, 11 },   // both levels, the other column, change flags, rows since a price change, trades/volumes
  {  8,  9, 18, 20, 21 },   // which price columns changed, and their directions
  { 12, 13, 14, 15, 17 },   // predictions, a0..a7
  { 16, 19, NCX, NCX+1, NCX+2 },   // i1 prices, i0 volumes, C0, C1, C2
  { 22, 23, 24, 25, 26 },   // masked models 0..4 (E0)
  { 27, 28, 29, 30, 31 },   // masked models 5..9 (E0)
};
