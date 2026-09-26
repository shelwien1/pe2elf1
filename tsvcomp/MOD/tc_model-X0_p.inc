// Generated into MOD/tc_model-X0_p.inc: the X0 knobs and context masks
// (MakeTables), the mixer-context builder (MakeIndex).  MixN cells of the
// paq block (paq.inc).

pmask( X0_g, "111" );
pmask( X0_k, "1" );
pdesc( X0_s, 1, "111" );
pdesc( X0_e, 1, "111111" );


pdesc( X0_W0_, 0, "010111001000001" );
static const int X0_W0 = X0_W0_.value * (1);
pdesc( X0_Wclip_, 0, "000100000010" );
static const int X0_Wclip = X0_Wclip_.value * (1);
pdesc( X0_Bclip_, 0, "00000001100" );
static const int X0_Bclip = X0_Bclip_.value * (1);
pdesc( X0_Pmin_, 0, "101001111100011" );
static const int X0_Pmin = X0_Pmin_.value * (1);
pdesc( X0_M1w_, 0, "1111111111000010000" );
static const int X0_M1w = X0_M1w_.value * (1);
pdesc( X0_M2w_, 0, "01010000000000000" );
static const int X0_M2w = X0_M2w_.value * (1);
pdesc( X0_NWw_, 0, "1000000000000100000" );
static const int X0_NWw = X0_NWw_.value * (1);
pdesc( X0_INCw_, 0, "000000100000000000" );
static const int X0_INCw = X0_INCw_.value * (1);
pdesc( X0_STEPw_, 0, "1110000100100000000" );
static const int X0_STEPw = X0_STEPw_.value * (1);
pdesc( X0_G2w_, 0, "0101011" );
static const int X0_G2w = X0_G2w_.value * (1);
pdesc( X0_DCw_, 0, "101111001" );
static const int X0_DCw = X0_DCw_.value * (1);
pdesc( X0_RCw_, 0, "001100001000010" );
static const int X0_RCw = X0_RCw_.value * (1);
pdesc( X0_NAGw_, 0, "1" );
static const int X0_NAGw = X0_NAGw_.value * (1);
pdesc( X0_M1b_, 0, "1010110000010110000" );
static const int X0_M1b = X0_M1b_.value * (1);
pdesc( X0_M2b_, 0, "11000000000000000" );
static const int X0_M2b = X0_M2b_.value * (1);
pdesc( X0_NWb_, 0, "10100010000101100000" );
static const int X0_NWb = X0_NWb_.value * (1);
pdesc( X0_INCb_, 0, "100100001000000001" );
static const int X0_INCb = X0_INCb_.value * (1);
pdesc( X0_STEPb_, 0, "1110110100010010001" );
static const int X0_STEPb = X0_STEPb_.value * (1);
pdesc( X0_G2b_, 0, "0000000" );
static const int X0_G2b = X0_G2b_.value * (1);
pdesc( X0_DCb_, 0, "001100001" );
static const int X0_DCb = X0_DCb_.value * (1);
pdesc( X0_RCb_, 0, "000000000000010" );
static const int X0_RCb = X0_RCb_.value * (1);
pdesc( X0_NAGb_, 0, "1" );
static const int X0_NAGb = X0_NAGb_.value * (1);
pdesc( X0_AGAw_, 0, "010011000100" );
static const int X0_AGAw = X0_AGAw_.value * (1);
pdesc( X0_AGAb_, 0, "001000010000" );
static const int X0_AGAb = X0_AGAb_.value * (1);
pdesc( X0_AGB_, 0, "0000001000" );
static const int X0_AGB = X0_AGB_.value * (1);
pdesc( X0_E2E_, 0, "100000000" );
static const int X0_E2E = X0_E2E_.value * (1);
pdesc( X0_XW_, 0, "00000000000" );
static const int X0_XW = X0_XW_.value * (1);



static uint X0_MakeCx( uint g, uint k, uint st, uint e ) {
  int Cx;

  Cx = 0;
  X0_g.inc( Cx, g );
  X0_k.inc( Cx, k );
  X0_s.inc( Cx, st );
  X0_e.inc( Cx, e );
  return uint(Cx);
}

