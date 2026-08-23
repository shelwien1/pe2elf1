// bmf-P2.inc -- the template idx2inc.pl expands into MOD/bmf-P2_p.inc.
// The declarations go to MOD/bmf-P2_h.inc; these are the index builders.

uint32_t MakeNbSlot( uint32_t band, uint32_t gA, uint32_t gB, uint32_t gC, uint32_t gD ) {
  int32_t NbSlot;
  MakeIndex NbSlot
  return (uint32_t)NbSlot;
}

uint32_t MakeBankSlot( uint32_t bank, uint32_t ctx ) {
  int32_t BankSlot;
  MakeIndex BankSlot
  return (uint32_t)BankSlot;
}

uint32_t MakeCtx0Lo( uint32_t f0, uint32_t f1, uint32_t f2, uint32_t f3, uint32_t f4, int32_t filt, int32_t g3 ) {
  int32_t Ctx0Lo;
  MakeIndex Ctx0Lo
  return (uint32_t)Ctx0Lo;
}

uint32_t MakeCtx0a( uint32_t f0, uint32_t f1, uint32_t f2, uint32_t f3, uint32_t f4, uint32_t f5, uint32_t lo ) {
  int32_t Ctx0a;
  MakeIndex Ctx0a
  return (uint32_t)Ctx0a;
}

uint32_t MakeCtx0b( uint32_t f0, uint32_t f1, uint32_t f2, uint32_t f3, uint32_t f4, uint32_t f5, uint32_t lo ) {
  int32_t Ctx0b;
  MakeIndex Ctx0b
  return (uint32_t)Ctx0b;
}

uint32_t MakeCtx1a( uint32_t f0, uint32_t f1, uint32_t f2, uint32_t f3, uint32_t f4, uint32_t f5, uint32_t f6, uint32_t f7, uint32_t f8, uint32_t f9, uint32_t f10, int32_t run, int32_t sum ) {
  int32_t Ctx1a;
  MakeIndex Ctx1a
  return (uint32_t)Ctx1a;
}

uint32_t MakeCtx1b( uint32_t f0, uint32_t f1, uint32_t f2, uint32_t f3, uint32_t f4, uint32_t f5, uint32_t f6, uint32_t f7, uint32_t f8, uint32_t f9, uint32_t f10, int32_t run, int32_t sum ) {
  int32_t Ctx1b;
  MakeIndex Ctx1b
  return (uint32_t)Ctx1b;
}

uint32_t MakeCtx2a( uint32_t f0, uint32_t f1, uint32_t f2, uint32_t f3, uint32_t f4, uint32_t f5, uint32_t f6, uint32_t f7, uint32_t f8, uint32_t f9, uint32_t f10, int32_t run, int32_t sum ) {
  int32_t Ctx2a;
  MakeIndex Ctx2a
  return (uint32_t)Ctx2a;
}

uint32_t MakeCtx2b( uint32_t f0, uint32_t f1, uint32_t f2, uint32_t f3, uint32_t f4, uint32_t f5, uint32_t f6, uint32_t f7, uint32_t f8, uint32_t f9, uint32_t f10, int32_t run, int32_t sum ) {
  int32_t Ctx2b;
  MakeIndex Ctx2b
  return (uint32_t)Ctx2b;
}

uint32_t MakeCtx3( uint32_t f0, uint32_t f1, uint32_t f2, uint32_t f3, uint32_t f4, uint32_t f5, uint32_t f6, uint32_t f7, uint32_t f8, uint32_t f9, uint32_t f10, int32_t run, int32_t sum ) {
  int32_t Ctx3;
  MakeIndex Ctx3
  return (uint32_t)Ctx3;
}

uint32_t MakeCtx4( uint32_t f0, uint32_t f1, uint32_t f2, uint32_t f3, uint32_t f4, uint32_t f5, uint32_t f6, uint32_t f7, uint32_t f8, uint32_t f9, uint32_t f10, int32_t run, int32_t sum ) {
  int32_t Ctx4;
  MakeIndex Ctx4
  return (uint32_t)Ctx4;
}

uint32_t MakeP2Ctx( uint32_t msum, uint32_t flat_b, uint32_t flat_a, uint32_t ctx15 ) {
  int32_t P2Ctx;
  MakeIndex P2Ctx
  return (uint32_t)P2Ctx;
}

uint32_t MakeP2Step( uint32_t msum, uint32_t ctx ) {
  int32_t P2Step;
  MakeIndex P2Step
  return (uint32_t)P2Step;
}

uint32_t MakeCtxDelta( uint32_t plane, uint32_t lvl ) {
  int32_t CtxDelta;
  MakeIndex CtxDelta
  return (uint32_t)CtxDelta;
}

uint32_t MakeCtxPair( uint32_t delta, uint32_t ctx15 ) {
  int32_t CtxPair;
  MakeIndex CtxPair
  return (uint32_t)CtxPair;
}

uint32_t MakeRun4Sel( int32_t nrun4 ) {
  int32_t Run4Sel;
  MakeIndex Run4Sel
  return (uint32_t)Run4Sel;
}

uint32_t MakeQ2Sel( int32_t nq2 ) {
  int32_t Q2Sel;
  MakeIndex Q2Sel
  return (uint32_t)Q2Sel;
}
