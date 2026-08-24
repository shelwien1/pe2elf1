// bmf-QZ.inc -- the template idx2inc.pl expands into MOD/bmf-QZ_p.inc.
// The mapping declarations go to MOD/bmf-QZ_h.inc; these are the builders.

uint32_t MakeLevel( uint32_t x ) {
  int32_t Level;

  Level = 0;
  Level = Level*8 + ((x>1+(1-1))+(x>2+(1-1))+(x>4+(1-1))+(x>8+(1-1))+(x>14+(1-1))+(x>35+(1-1))+(x>103+(1-1)));
  return (uint32_t)Level;
}

uint32_t MakeGroup( uint32_t x ) {
  int32_t Group;

  Group = 0;
  Group = Group*8 + ((x>1+(1-1))+(x>3+(1-1))+(x>6+(1-1))+(x>10+(1-1))+(x>16+(1-1))+(x>27+(1-1))+(x>52+(1-1)));
  return (uint32_t)Group;
}

uint32_t MakeSlot( uint32_t x ) {
  int32_t Slot;

  Slot = 0;
  Slot = Slot*8 + ((x>5+(1-1))+(x>10+(1-1))+(x>36+(1-1))+(x>98+(1-1))+(x>154+(1-1))+(x>236+(1-1))+(x>248+(1-1)));
  return (uint32_t)Slot;
}

uint32_t MakeP2Ctx( uint32_t x ) {
  int32_t P2Ctx;

  P2Ctx = 0;
  P2Ctx = P2Ctx*31 + QZ_qc[__min(260,__max(0,x-(1-1)))];
  return (uint32_t)P2Ctx;
}

uint32_t MakeP2Len( uint32_t x ) {
  int32_t P2Len;

  P2Len = 0;
  P2Len = P2Len*10 + QZ_qn[__min(121,__max(0,x-(1-1)))];
  return (uint32_t)P2Len;
}
