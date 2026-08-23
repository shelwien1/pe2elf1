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

uint32_t MakeNB2( uint32_t a, uint32_t b, uint32_t c, uint32_t d ) {
  int32_t NB2;

  NB2 = 0;
  NB2 = NB2*(2) + (a);
  NB2 = NB2*(2) + (b);
  NB2 = NB2*(2) + (c);
  NB2 = NB2*(2) + (d);
  return (uint32_t)NB2;
}

uint32_t MakeSig1( uint32_t bucket, uint32_t m0, uint32_t m1,
                   uint32_t g3, uint32_t g2, uint32_t g1, uint32_t g0, uint32_t nb2 ) {
  int32_t Sig1;

  Sig1 = 0;
  Sig1 = Sig1*(MB_freq_table_offset) + (bucket);
  Sig1 = Sig1*(2) + (m0);
  Sig1 = Sig1*(2) + (m1);
  Sig1 = Sig1*(2) + (g3);
  Sig1 = Sig1*(2) + (g2);
  Sig1 = Sig1*(2) + (g1);
  Sig1 = Sig1*(2) + (g0);
  Sig1 = Sig1*(MB_NB2_Volume) + (nb2);
  return (uint32_t)Sig1;
}

uint32_t MakeSig2( uint32_t id1, uint32_t a, uint32_t b, uint32_t c ) {
  int32_t Sig2;

  Sig2 = 0;
  Sig2 = Sig2*(MB_id1_range) + (id1);
  Sig2 = Sig2*(2) + (a);
  Sig2 = Sig2*(2) + (b);
  Sig2 = Sig2*(2) + (c);
  return (uint32_t)Sig2;
}

uint32_t MakeSig3( uint32_t id2, uint32_t left ) {
  int32_t Sig3;

  Sig3 = 0;
  Sig3 = Sig3*(MB_id2_range) + (id2);
  Sig3 = Sig3*(16) + (left);
  return (uint32_t)Sig3;
}

uint32_t MakeMatchCtx( uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f ) {
  int32_t MatchCtx;

  MatchCtx = 0;
  MatchCtx = MatchCtx*(2) + (a);
  MatchCtx = MatchCtx*(2) + (b);
  MatchCtx = MatchCtx*(2) + (c);
  MatchCtx = MatchCtx*(2) + (d);
  MatchCtx = MatchCtx*(2) + (e);
  MatchCtx = MatchCtx*(2) + (f);
  return (uint32_t)MatchCtx;
}

uint32_t MakeSymCache( uint32_t key, uint32_t slot ) {
  int32_t SymCache;

  SymCache = 0;
  SymCache = SymCache*(65536) + (key);
  SymCache = SymCache*(8) + (slot);
  return (uint32_t)SymCache;
}

uint32_t MakeEscCtx( uint32_t bucket, uint32_t run_pair, uint32_t m_up0, uint32_t alpha ) {
  int32_t EscCtx;

  EscCtx = 0;
  EscCtx = EscCtx*(32) + (bucket);
  EscCtx = EscCtx*(2) + (run_pair);
  EscCtx = EscCtx*(2) + (m_up0);
  EscCtx = EscCtx*(2) + (alpha);
  return (uint32_t)EscCtx;
}
