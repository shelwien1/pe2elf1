// Generated into MOD/tc_model-M0_p.inc: the M0 knobs and context masks
// (MakeTables), the mixer-context builder (MakeIndex).

pmask( M0_k, "1" );
pdesc( M0_s, 1, "111" );
pdesc( M0_e, 1, "0001010100101000" );


pdesc( M0_W0_, 0, "010111001000001" );
static const int M0_W0 = M0_W0_.value * (1);
pdesc( M0_Wclip_, 0, "000100000010" );
static const int M0_Wclip = M0_Wclip_.value * (1);
pdesc( M0_Bclip_, 0, "00000001100" );
static const int M0_Bclip = M0_Bclip_.value * (1);
pdesc( M0_Pmin_, 0, "101001111100011" );
static const int M0_Pmin = M0_Pmin_.value * (1);
pdesc( M0_M1w_, 0, "1111111111000010000" );
static const int M0_M1w = M0_M1w_.value * (1);
pdesc( M0_M2w_, 0, "01010000000000000" );
static const int M0_M2w = M0_M2w_.value * (1);
pdesc( M0_NWw_, 0, "1000000000000100000" );
static const int M0_NWw = M0_NWw_.value * (1);
pdesc( M0_INCw_, 0, "000000100000000000" );
static const int M0_INCw = M0_INCw_.value * (1);
pdesc( M0_STEPw_, 0, "1110000100100000000" );
static const int M0_STEPw = M0_STEPw_.value * (1);
pdesc( M0_G2w_, 0, "0101011" );
static const int M0_G2w = M0_G2w_.value * (1);
pdesc( M0_DCw_, 0, "101111001" );
static const int M0_DCw = M0_DCw_.value * (1);
pdesc( M0_RCw_, 0, "001100001000010" );
static const int M0_RCw = M0_RCw_.value * (1);
pdesc( M0_NAGw_, 0, "1" );
static const int M0_NAGw = M0_NAGw_.value * (1);
pdesc( M0_M1b_, 0, "1010110000010110000" );
static const int M0_M1b = M0_M1b_.value * (1);
pdesc( M0_M2b_, 0, "11000000000000000" );
static const int M0_M2b = M0_M2b_.value * (1);
pdesc( M0_NWb_, 0, "10100010000101100000" );
static const int M0_NWb = M0_NWb_.value * (1);
pdesc( M0_INCb_, 0, "100100001000000001" );
static const int M0_INCb = M0_INCb_.value * (1);
pdesc( M0_STEPb_, 0, "1110110100010010001" );
static const int M0_STEPb = M0_STEPb_.value * (1);
pdesc( M0_G2b_, 0, "0000000" );
static const int M0_G2b = M0_G2b_.value * (1);
pdesc( M0_DCb_, 0, "001100001" );
static const int M0_DCb = M0_DCb_.value * (1);
pdesc( M0_RCb_, 0, "000000000000010" );
static const int M0_RCb = M0_RCb_.value * (1);
pdesc( M0_NAGb_, 0, "1" );
static const int M0_NAGb = M0_NAGb_.value * (1);
pdesc( M0_AGAw_, 0, "010011000100" );
static const int M0_AGAw = M0_AGAw_.value * (1);
pdesc( M0_AGAb_, 0, "001000010000" );
static const int M0_AGAb = M0_AGAb_.value * (1);
pdesc( M0_AGB_, 0, "0000001000" );
static const int M0_AGB = M0_AGB_.value * (1);
pdesc( M0_E2E_, 0, "100000000" );
static const int M0_E2E = M0_E2E_.value * (1);



static uint M0_MakeCx( uint k, uint st, uint e ) {
  int Cx;

  Cx = 0;
  M0_k.inc( Cx, k );
  M0_s.inc( Cx, st );
  M0_e.inc( Cx, e );
  return uint(Cx);
}

