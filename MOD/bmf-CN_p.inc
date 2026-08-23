// bmf-CN.inc -- the template idx2inc.pl expands into MOD/bmf-CN_p.inc.
// The declarations go to MOD/bmf-CN_h.inc; this is the index builder.

uint32_t MakeStrip( uint32_t plane, uint32_t parity, uint32_t escape_high, uint32_t low ) {
  int32_t Strip;

  Strip = 0;
  Strip = Strip*(4) + (plane);
  Strip = Strip*(2) + (parity);
  Strip = Strip*(2) + (escape_high);
  Strip = Strip*(64) + (low);
  return (uint32_t)Strip;
}

uint32_t MakeSlotMap( int32_t ctx ) {
  int32_t SlotMap;

  SlotMap = 0;
  CN_mslot.inc( SlotMap, ctx );
  return (uint32_t)SlotMap;
}
