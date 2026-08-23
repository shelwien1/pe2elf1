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
