// bmf-P2.inc -- the template idx2inc.pl expands into MOD/bmf-P2_p.inc.
// The declarations go to MOD/bmf-P2_h.inc; these are the index builders.

uint32_t MakeNbSlot( uint32_t band, uint32_t gA, uint32_t gB, uint32_t gC, uint32_t gD ) {
  int32_t NbSlot;

  NbSlot = 0;
  NbSlot = NbSlot*(6) + (band);
  NbSlot = NbSlot*(5) + (gA);
  NbSlot = NbSlot*(4) + (gB);
  NbSlot = NbSlot*(4) + (gC);
  NbSlot = NbSlot*(4) + (gD);
  return (uint32_t)NbSlot;
}

uint32_t MakeBankSlot( uint32_t bank, uint32_t ctx ) {
  int32_t BankSlot;

  BankSlot = 0;
  BankSlot = BankSlot*(5) + (bank);
  BankSlot = BankSlot*(32768) + (ctx);
  return (uint32_t)BankSlot;
}

uint32_t MakeCtx0Lo( uint32_t f0, uint32_t f1, uint32_t f2, uint32_t f3, uint32_t f4, int32_t filt, int32_t g3 ) {
  int32_t Ctx0Lo;

  Ctx0Lo = 0;
  Ctx0Lo = Ctx0Lo*(2) + (f0);
  Ctx0Lo = Ctx0Lo*(2) + (f1);
  Ctx0Lo = Ctx0Lo*(2) + (f2);
  Ctx0Lo = Ctx0Lo*(2) + (f3);
  Ctx0Lo = Ctx0Lo*(2) + (f4);
  P2_qf.inc( Ctx0Lo, filt );
  P2_qg.inc( Ctx0Lo, g3 );
  return (uint32_t)Ctx0Lo;
}

uint32_t MakeCtx0a( uint32_t f0, uint32_t f1, uint32_t f2, uint32_t f3, uint32_t f4, uint32_t f5, uint32_t lo ) {
  int32_t Ctx0a;

  Ctx0a = 0;
  Ctx0a = Ctx0a*(2) + (f0);
  Ctx0a = Ctx0a*(2) + (f1);
  Ctx0a = Ctx0a*(2) + (f2);
  Ctx0a = Ctx0a*(2) + (f3);
  Ctx0a = Ctx0a*(2) + (f4);
  Ctx0a = Ctx0a*(2) + (f5);
  Ctx0a = Ctx0a*(P2_Ctx0Lo_Volume) + (lo);
  return (uint32_t)Ctx0a;
}

uint32_t MakeCtx0b( uint32_t f0, uint32_t f1, uint32_t f2, uint32_t f3, uint32_t f4, uint32_t f5, uint32_t lo ) {
  int32_t Ctx0b;

  Ctx0b = 0;
  Ctx0b = Ctx0b*(2) + (f0);
  Ctx0b = Ctx0b*(2) + (f1);
  Ctx0b = Ctx0b*(2) + (f2);
  Ctx0b = Ctx0b*(2) + (f3);
  Ctx0b = Ctx0b*(2) + (f4);
  Ctx0b = Ctx0b*(2) + (f5);
  Ctx0b = Ctx0b*(P2_Ctx0Lo_Volume) + (lo);
  return (uint32_t)Ctx0b;
}

uint32_t MakeCtx1a( uint32_t f0, uint32_t f1, uint32_t f2, uint32_t f3, uint32_t f4, uint32_t f5, uint32_t f6, uint32_t f7, uint32_t f8, uint32_t f9, uint32_t f10, int32_t run, int32_t sum ) {
  int32_t Ctx1a;

  Ctx1a = 0;
  Ctx1a = Ctx1a*(2) + (f0);
  Ctx1a = Ctx1a*(2) + (f1);
  Ctx1a = Ctx1a*(2) + (f2);
  Ctx1a = Ctx1a*(2) + (f3);
  Ctx1a = Ctx1a*(2) + (f4);
  Ctx1a = Ctx1a*(2) + (f5);
  Ctx1a = Ctx1a*(2) + (f6);
  Ctx1a = Ctx1a*(2) + (f7);
  Ctx1a = Ctx1a*(2) + (f8);
  Ctx1a = Ctx1a*(2) + (f9);
  Ctx1a = Ctx1a*(2) + (f10);
  P2_qr_ctx1a.inc( Ctx1a, run );
  P2_qs_ctx1a.inc( Ctx1a, sum );
  return (uint32_t)Ctx1a;
}

uint32_t MakeCtx1b( uint32_t f0, uint32_t f1, uint32_t f2, uint32_t f3, uint32_t f4, uint32_t f5, uint32_t f6, uint32_t f7, uint32_t f8, uint32_t f9, uint32_t f10, int32_t run, int32_t sum ) {
  int32_t Ctx1b;

  Ctx1b = 0;
  Ctx1b = Ctx1b*(2) + (f0);
  Ctx1b = Ctx1b*(2) + (f1);
  Ctx1b = Ctx1b*(2) + (f2);
  Ctx1b = Ctx1b*(2) + (f3);
  Ctx1b = Ctx1b*(2) + (f4);
  Ctx1b = Ctx1b*(2) + (f5);
  Ctx1b = Ctx1b*(2) + (f6);
  Ctx1b = Ctx1b*(2) + (f7);
  Ctx1b = Ctx1b*(2) + (f8);
  Ctx1b = Ctx1b*(2) + (f9);
  Ctx1b = Ctx1b*(2) + (f10);
  P2_qr_ctx1b.inc( Ctx1b, run );
  P2_qs_ctx1b.inc( Ctx1b, sum );
  return (uint32_t)Ctx1b;
}

uint32_t MakeCtx2a( uint32_t f0, uint32_t f1, uint32_t f2, uint32_t f3, uint32_t f4, uint32_t f5, uint32_t f6, uint32_t f7, uint32_t f8, uint32_t f9, uint32_t f10, int32_t run, int32_t sum ) {
  int32_t Ctx2a;

  Ctx2a = 0;
  Ctx2a = Ctx2a*(2) + (f0);
  Ctx2a = Ctx2a*(2) + (f1);
  Ctx2a = Ctx2a*(2) + (f2);
  Ctx2a = Ctx2a*(2) + (f3);
  Ctx2a = Ctx2a*(2) + (f4);
  Ctx2a = Ctx2a*(2) + (f5);
  Ctx2a = Ctx2a*(2) + (f6);
  Ctx2a = Ctx2a*(2) + (f7);
  Ctx2a = Ctx2a*(2) + (f8);
  Ctx2a = Ctx2a*(2) + (f9);
  Ctx2a = Ctx2a*(2) + (f10);
  P2_qr_ctx2a.inc( Ctx2a, run );
  P2_qs_ctx2a.inc( Ctx2a, sum );
  return (uint32_t)Ctx2a;
}

uint32_t MakeCtx2b( uint32_t f0, uint32_t f1, uint32_t f2, uint32_t f3, uint32_t f4, uint32_t f5, uint32_t f6, uint32_t f7, uint32_t f8, uint32_t f9, uint32_t f10, int32_t run, int32_t sum ) {
  int32_t Ctx2b;

  Ctx2b = 0;
  Ctx2b = Ctx2b*(2) + (f0);
  Ctx2b = Ctx2b*(2) + (f1);
  Ctx2b = Ctx2b*(2) + (f2);
  Ctx2b = Ctx2b*(2) + (f3);
  Ctx2b = Ctx2b*(2) + (f4);
  Ctx2b = Ctx2b*(2) + (f5);
  Ctx2b = Ctx2b*(2) + (f6);
  Ctx2b = Ctx2b*(2) + (f7);
  Ctx2b = Ctx2b*(2) + (f8);
  Ctx2b = Ctx2b*(2) + (f9);
  Ctx2b = Ctx2b*(2) + (f10);
  P2_qr_ctx2b.inc( Ctx2b, run );
  P2_qs_ctx2b.inc( Ctx2b, sum );
  return (uint32_t)Ctx2b;
}

uint32_t MakeCtx3( uint32_t f0, uint32_t f1, uint32_t f2, uint32_t f3, uint32_t f4, uint32_t f5, uint32_t f6, uint32_t f7, uint32_t f8, uint32_t f9, uint32_t f10, int32_t run, int32_t sum ) {
  int32_t Ctx3;

  Ctx3 = 0;
  Ctx3 = Ctx3*(2) + (f0);
  Ctx3 = Ctx3*(2) + (f1);
  Ctx3 = Ctx3*(2) + (f2);
  Ctx3 = Ctx3*(2) + (f3);
  Ctx3 = Ctx3*(2) + (f4);
  Ctx3 = Ctx3*(2) + (f5);
  Ctx3 = Ctx3*(2) + (f6);
  Ctx3 = Ctx3*(2) + (f7);
  Ctx3 = Ctx3*(2) + (f8);
  Ctx3 = Ctx3*(2) + (f9);
  Ctx3 = Ctx3*(2) + (f10);
  P2_qr_ctx3.inc( Ctx3, run );
  P2_qs_ctx3.inc( Ctx3, sum );
  return (uint32_t)Ctx3;
}

uint32_t MakeCtx4( uint32_t f0, uint32_t f1, uint32_t f2, uint32_t f3, uint32_t f4, uint32_t f5, uint32_t f6, uint32_t f7, uint32_t f8, uint32_t f9, uint32_t f10, int32_t run, int32_t sum ) {
  int32_t Ctx4;

  Ctx4 = 0;
  Ctx4 = Ctx4*(2) + (f0);
  Ctx4 = Ctx4*(2) + (f1);
  Ctx4 = Ctx4*(2) + (f2);
  Ctx4 = Ctx4*(2) + (f3);
  Ctx4 = Ctx4*(2) + (f4);
  Ctx4 = Ctx4*(2) + (f5);
  Ctx4 = Ctx4*(2) + (f6);
  Ctx4 = Ctx4*(2) + (f7);
  Ctx4 = Ctx4*(2) + (f8);
  Ctx4 = Ctx4*(2) + (f9);
  Ctx4 = Ctx4*(2) + (f10);
  P2_qr_ctx4.inc( Ctx4, run );
  P2_qs_ctx4.inc( Ctx4, sum );
  return (uint32_t)Ctx4;
}

uint32_t MakeP2Ctx( uint32_t msum, uint32_t flat_b, uint32_t flat_a, uint32_t ctx15 ) {
  int32_t P2Ctx;

  P2Ctx = 0;
  P2Ctx = P2Ctx*(P2_ctxw_range) + (msum);
  P2Ctx = P2Ctx*(2) + (flat_b);
  P2Ctx = P2Ctx*(2) + (flat_a);
  P2Ctx = P2Ctx*(16) + (ctx15);
  return (uint32_t)P2Ctx;
}

uint32_t MakeP2Step( uint32_t msum, uint32_t ctx ) {
  int32_t P2Step;

  P2Step = 0;
  P2Step = P2Step*(P2_ctxw_range) + (msum);
  P2Step = P2Step*(64) + (ctx);
  return (uint32_t)P2Step;
}

uint32_t MakeCtxDelta( uint32_t plane, uint32_t lvl ) {
  int32_t CtxDelta;

  CtxDelta = 0;
  CtxDelta = CtxDelta*(4) + (plane);
  CtxDelta = CtxDelta*(16) + (lvl);
  return (uint32_t)CtxDelta;
}

uint32_t MakeCtxPair( uint32_t delta, uint32_t ctx15 ) {
  int32_t CtxPair;

  CtxPair = 0;
  CtxPair = CtxPair*(P2_CtxDelta_Volume) + (delta);
  CtxPair = CtxPair*(16) + (ctx15);
  return (uint32_t)CtxPair;
}

uint32_t MakeRun4Sel( int32_t nrun4 ) {
  int32_t Run4Sel;

  Run4Sel = 0;
  P2_r.inc( Run4Sel, nrun4 );
  return (uint32_t)Run4Sel;
}

uint32_t MakeQ2Sel( int32_t nq2 ) {
  int32_t Q2Sel;

  Q2Sel = 0;
  P2_q.inc( Q2Sel, nq2 );
  return (uint32_t)Q2Sel;
}

uint32_t MakeSeat( int32_t ctx ) {
  int32_t Seat;

  Seat = 0;
  P2_mseat.inc( Seat, ctx );
  return (uint32_t)Seat;
}

uint32_t MakeCellMap( int32_t ctx ) {
  int32_t CellMap;

  CellMap = 0;
  P2_mcell.inc( CellMap, ctx );
  return (uint32_t)CellMap;
}
