
// Context of the mixer, expanded by IDX/idx2inc.pl from the Index Cx block
// of sh_model-M0.idx (MOD/sh_model-M0_p.inc).
static uint M0_MakeCx( uint c2, uint c1, uint cxt ) {
  int Cx;

  Cx = 0;
  Cx = (Cx<<4) + ((((c2)>>4)&15)); // 11110000
  Cx = (Cx<<8) + (((c1)&255)); // 11111111
  Cx = Cx*(256) + (cxt);
  return uint(Cx);
}
