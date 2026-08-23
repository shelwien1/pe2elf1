// bmf-MB.inc -- the template idx2inc.pl expands into MOD/bmf-MB_p.inc.
// The declarations go to MOD/bmf-MB_h.inc; these are the index builders.

uint32_t MakeBucket( uint32_t prev, uint32_t last, uint32_t group ) {
  int32_t Bucket;
  MakeIndex Bucket
  return (uint32_t)Bucket;
}

uint32_t MakeRunCtr( uint32_t novel, uint32_t bucket ) {
  int32_t RunCtr;
  MakeIndex RunCtr
  return (uint32_t)RunCtr;
}

uint32_t MakeCandCtx( uint32_t exact, uint32_t near, uint32_t far,
                      uint32_t in_top, uint32_t in_prev, uint32_t in_sym, uint32_t in_sel ) {
  int32_t CandCtx;
  MakeIndex CandCtx
  return (uint32_t)CandCtx;
}

uint32_t MakeCandNode( uint32_t pos, uint32_t ctx ) {
  int32_t CandNode;
  MakeIndex CandNode
  return (uint32_t)CandNode;
}

uint32_t MakeCandFallback( uint32_t deep, uint32_t fallback ) {
  int32_t CandFallback;
  MakeIndex CandFallback
  return (uint32_t)CandFallback;
}

uint32_t MakeNB2( uint32_t a, uint32_t b, uint32_t c, uint32_t d ) {
  int32_t NB2;
  MakeIndex NB2
  return (uint32_t)NB2;
}

uint32_t MakeSig1( uint32_t bucket, uint32_t m0, uint32_t m1,
                   uint32_t g3, uint32_t g2, uint32_t g1, uint32_t g0, uint32_t nb2 ) {
  int32_t Sig1;
  MakeIndex Sig1
  return (uint32_t)Sig1;
}

uint32_t MakeSig2( uint32_t id1, uint32_t a, uint32_t b, uint32_t c ) {
  int32_t Sig2;
  MakeIndex Sig2
  return (uint32_t)Sig2;
}

uint32_t MakeSig3( uint32_t id2, uint32_t left ) {
  int32_t Sig3;
  MakeIndex Sig3
  return (uint32_t)Sig3;
}

uint32_t MakeMatchCtx( uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f ) {
  int32_t MatchCtx;
  MakeIndex MatchCtx
  return (uint32_t)MatchCtx;
}

uint32_t MakeSymCache( uint32_t key, uint32_t slot ) {
  int32_t SymCache;
  MakeIndex SymCache
  return (uint32_t)SymCache;
}
