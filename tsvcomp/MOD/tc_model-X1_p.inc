// Generated into MOD/tc_model-X1_p.inc: the X1 knobs and context masks
// (MakeTables), the mixer-context builder (MakeIndex).  MixN cells of the
// paq block (paq.inc).

pmask( X1_k, "1" );
pdesc( X1_s, 1, "111" );
pdesc( X1_z, 1, "111" );
pmask( X1_c, "111" );


pdesc( X1_W0_, 0, "010111001000001" );
static const int X1_W0 = X1_W0_.value * (1);
pdesc( X1_Wclip_, 0, "000100000010" );
static const int X1_Wclip = X1_Wclip_.value * (1);
pdesc( X1_Bclip_, 0, "00000001100" );
static const int X1_Bclip = X1_Bclip_.value * (1);
pdesc( X1_Pmin_, 0, "101001111100011" );
static const int X1_Pmin = X1_Pmin_.value * (1);
pdesc( X1_M1w_, 0, "1111111111000010000" );
static const int X1_M1w = X1_M1w_.value * (1);
pdesc( X1_M2w_, 0, "01010000000000000" );
static const int X1_M2w = X1_M2w_.value * (1);
pdesc( X1_NWw_, 0, "1000000000000100000" );
static const int X1_NWw = X1_NWw_.value * (1);
pdesc( X1_INCw_, 0, "000000100000000000" );
static const int X1_INCw = X1_INCw_.value * (1);
pdesc( X1_STEPw_, 0, "1110000100100000000" );
static const int X1_STEPw = X1_STEPw_.value * (1);
pdesc( X1_G2w_, 0, "0101011" );
static const int X1_G2w = X1_G2w_.value * (1);
pdesc( X1_DCw_, 0, "101111001" );
static const int X1_DCw = X1_DCw_.value * (1);
pdesc( X1_RCw_, 0, "001100001000010" );
static const int X1_RCw = X1_RCw_.value * (1);
pdesc( X1_NAGw_, 0, "1" );
static const int X1_NAGw = X1_NAGw_.value * (1);
pdesc( X1_M1b_, 0, "1010110000010110000" );
static const int X1_M1b = X1_M1b_.value * (1);
pdesc( X1_M2b_, 0, "11000000000000000" );
static const int X1_M2b = X1_M2b_.value * (1);
pdesc( X1_NWb_, 0, "10100010000101100000" );
static const int X1_NWb = X1_NWb_.value * (1);
pdesc( X1_INCb_, 0, "100100001000000001" );
static const int X1_INCb = X1_INCb_.value * (1);
pdesc( X1_STEPb_, 0, "1110110100010010001" );
static const int X1_STEPb = X1_STEPb_.value * (1);
pdesc( X1_G2b_, 0, "0000000" );
static const int X1_G2b = X1_G2b_.value * (1);
pdesc( X1_DCb_, 0, "001100001" );
static const int X1_DCb = X1_DCb_.value * (1);
pdesc( X1_RCb_, 0, "000000000000010" );
static const int X1_RCb = X1_RCb_.value * (1);
pdesc( X1_NAGb_, 0, "1" );
static const int X1_NAGb = X1_NAGb_.value * (1);
pdesc( X1_AGAw_, 0, "010011000100" );
static const int X1_AGAw = X1_AGAw_.value * (1);
pdesc( X1_AGAb_, 0, "001000010000" );
static const int X1_AGAb = X1_AGAb_.value * (1);
pdesc( X1_AGB_, 0, "0000001000" );
static const int X1_AGB = X1_AGB_.value * (1);
pdesc( X1_E2E_, 0, "100000000" );
static const int X1_E2E = X1_E2E_.value * (1);
pdesc( X1_XW_, 0, "00000000000" );
static const int X1_XW = X1_XW_.value * (1);



static uint X1_MakeCx( uint k, uint st, uint zrun, uint sch ) {
  int Cx;

  Cx = 0;
  X1_k.inc( Cx, k );
  X1_s.inc( Cx, st );
  X1_z.inc( Cx, zrun );
  X1_c.inc( Cx, sch );
  return uint(Cx);
}

