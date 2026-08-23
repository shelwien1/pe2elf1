// bmf-MB.inc -- the template idx2inc.pl expands into MOD/bmf-MB_p.inc.
// The declarations go to MOD/bmf-MB_h.inc; these are the index builders.

uint32_t MakeBucket( uint32_t prev, uint32_t last, uint32_t group ) {
  int32_t Bucket;

  Bucket = 0;
  Bucket = Bucket*(5) + (prev);
  Bucket = Bucket*(5) + (last);
  Bucket = Bucket*(15) + (group);
  return (uint32_t)Bucket;
}

uint32_t MakeRunCtr( uint32_t novel, uint32_t bucket ) {
  int32_t RunCtr;

  RunCtr = 0;
  RunCtr = RunCtr*(3) + (novel);
  RunCtr = RunCtr*(16) + (bucket);
  return (uint32_t)RunCtr;
}

uint32_t MakeCandCtx( uint32_t exact, uint32_t near, uint32_t far,
                      uint32_t in_top, uint32_t in_prev, uint32_t in_sym, uint32_t in_sel ) {
  int32_t CandCtx;

  CandCtx = 0;
  CandCtx = CandCtx*(2) + (exact);
  CandCtx = CandCtx*(2) + (near);
  CandCtx = CandCtx*(2) + (far);
  CandCtx = CandCtx*(2) + (in_top);
  CandCtx = CandCtx*(2) + (in_prev);
  CandCtx = CandCtx*(2) + (in_sym);
  CandCtx = CandCtx*(2) + (in_sel);
  return (uint32_t)CandCtx;
}

uint32_t MakeCandNode( uint32_t pos, uint32_t ctx ) {
  int32_t CandNode;

  CandNode = 0;
  CandNode = CandNode*(32) + (pos);
  CandNode = CandNode*(MB_CandCtx_Volume) + (ctx);
  return (uint32_t)CandNode;
}

uint32_t MakeCandFallback( uint32_t deep, uint32_t fallback ) {
  int32_t CandFallback;

  CandFallback = 0;
  CandFallback = CandFallback*(2) + (deep);
  CandFallback = CandFallback*(8) + (fallback);
  return (uint32_t)CandFallback;
}
