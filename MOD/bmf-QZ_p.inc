// bmf-QZ.inc -- the template idx2inc.pl expands into MOD/bmf-QZ_p.inc.
// The mapping declarations go to MOD/bmf-QZ_h.inc; these are the builders.

uint32_t MakeLevel( uint32_t x ) {
  int32_t Level;

  Level = 0;
  QZ_ql.inc( Level, x );
  return (uint32_t)Level;
}

uint32_t MakeGroup( uint32_t x ) {
  int32_t Group;

  Group = 0;
  QZ_qg.inc( Group, x );
  return (uint32_t)Group;
}

uint32_t MakeSlot( uint32_t x ) {
  int32_t Slot;

  Slot = 0;
  QZ_qs.inc( Slot, x );
  return (uint32_t)Slot;
}

uint32_t MakeP2Ctx( uint32_t x ) {
  int32_t P2Ctx;

  P2Ctx = 0;
  QZ_qc.inc( P2Ctx, x );
  return (uint32_t)P2Ctx;
}

uint32_t MakeP2Len( uint32_t x ) {
  int32_t P2Len;

  P2Len = 0;
  QZ_qn.inc( P2Len, x );
  return (uint32_t)P2Len;
}
