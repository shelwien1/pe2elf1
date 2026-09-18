
// Row context of the SSE stage, expanded by IDX/idx2inc.pl from the Index
// blocks of sh_model-S0.idx (MOD/sh_model-S0_p.inc).  c3/c2/c1 = the three
// last bytes, cxt = the bit-tree node (1..255).  The coder combines the two
// results in 64 bits: row = Cx * S0_Cx3_Volume + Cx3.
static uint S0_MakeCx( uint c2, uint c1, uint cxt ) {
  int Cx;

  Cx = 0;
  Cx = (Cx<<8) + (((c2)&255)); // 11111111
  Cx = (Cx<<5) + (((c1)&31)); // 00011111
  Cx = (Cx*255) + S0_cx[(cxt)<1?1:((cxt)>255?255:(cxt))]; // 00000000
  return uint(Cx);
}
static uint S0_MakeCx3( uint c3 ) {
  int Cx3;

  Cx3 = 0;
  Cx3 = (Cx3<<6) + (((((c3)>>6)&1)<<5) + ((c3)&31)); // 01011111
  return uint(Cx3);
}
