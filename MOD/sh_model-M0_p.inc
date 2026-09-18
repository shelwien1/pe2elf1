
// Context of the mixer, expanded by IDX/idx2inc.pl from the Index Cx block
// of sh_model-M0.idx (MOD/sh_model-M0_p.inc).
static uint M0_MakeCx( uint c2, uint c1, uint cxt ) {
  int Cx;

  Cx = 0;
  Cx = (Cx<<6) + ((((c2)>>2)&63)); // 11111100
  Cx = (Cx<<8) + (((c1)&255)); // 11111111
  Cx = (Cx*255) + M0_cx[(cxt)<1?1:((cxt)>255?255:(cxt))]; // 00000000
  return uint(Cx);
}
