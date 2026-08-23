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
  Ctx0Lo = Ctx0Lo*4 + ((filt>288+(1-1))+(filt>720+(1-1))+(filt>3536+(1-1)));
  Ctx0Lo = Ctx0Lo*4 + ((g3>240+(1-1))+(g3>400+(1-1))+(g3>752+(1-1)));
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
  Ctx1a = Ctx1a*4 + ((run>144+(1-1))+(run>1056+(1-1))+(run>2256+(1-1)));
  Ctx1a = Ctx1a*4 + ((sum>10+(1-1))+(sum>24+(1-1))+(sum>55+(1-1)));
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
  Ctx1b = Ctx1b*4 + ((run>240+(1-1))+(run>1024+(1-1))+(run>2400+(1-1)));
  Ctx1b = Ctx1b*4 + ((sum>11+(1-1))+(sum>24+(1-1))+(sum>39+(1-1)));
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
  Ctx2a = Ctx2a*4 + ((run>640+(1-1))+(run>1280+(1-1))+(run>2576+(1-1)));
  Ctx2a = Ctx2a*4 + ((sum>4+(1-1))+(sum>12+(1-1))+(sum>33+(1-1)));
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
  Ctx2b = Ctx2b*4 + ((run>688+(1-1))+(run>1216+(1-1))+(run>2464+(1-1)));
  Ctx2b = Ctx2b*4 + ((sum>13+(1-1))+(sum>25+(1-1))+(sum>58+(1-1)));
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
  Ctx3 = Ctx3*4 + ((run>592+(1-1))+(run>1568+(1-1))+(run>2896+(1-1)));
  Ctx3 = Ctx3*4 + ((sum>9+(1-1))+(sum>19+(1-1))+(sum>37+(1-1)));
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
  Ctx4 = Ctx4*4 + ((run>368+(1-1))+(run>1952+(1-1))+(run>3056+(1-1)));
  Ctx4 = Ctx4*4 + ((sum>10+(1-1))+(sum>21+(1-1))+(sum>39+(1-1)));
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
