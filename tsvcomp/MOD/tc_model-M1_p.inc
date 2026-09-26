// Generated into MOD/tc_model-M1_p.inc: the M1 knobs and context masks
// (MakeTables), the final-mixer context builder (MakeIndex).

pmask( M1_k, "0" );
pdesc( M1_s, 1, "111" );
pdesc( M1_e, 1, "0000000001110110" );


pdesc( M1_W0_, 0, "010101110010000" );
static const int M1_W0 = M1_W0_.value * (1);
pdesc( M1_Wclip_, 0, "011000000110" );
static const int M1_Wclip = M1_Wclip_.value * (1);
pdesc( M1_Bclip_, 0, "00110010110" );
static const int M1_Bclip = M1_Bclip_.value * (1);
pdesc( M1_Pmin_, 0, "101001111100011" );
static const int M1_Pmin = M1_Pmin_.value * (1);
pdesc( M1_M1w_, 0, "1111111111000010000" );
static const int M1_M1w = M1_M1w_.value * (1);
pdesc( M1_M2w_, 0, "01100000011000000" );
static const int M1_M2w = M1_M2w_.value * (1);
pdesc( M1_NWw_, 0, "1010000100000100000" );
static const int M1_NWw = M1_NWw_.value * (1);
pdesc( M1_INCw_, 0, "010100000000000100" );
static const int M1_INCw = M1_INCw_.value * (1);
pdesc( M1_STEPw_, 0, "1111000100000001000" );
static const int M1_STEPw = M1_STEPw_.value * (1);
pdesc( M1_G2w_, 0, "1010000" );
static const int M1_G2w = M1_G2w_.value * (1);
pdesc( M1_DCw_, 0, "101111001" );
static const int M1_DCw = M1_DCw_.value * (1);
pdesc( M1_RCw_, 0, "000000010000000" );
static const int M1_RCw = M1_RCw_.value * (1);
pdesc( M1_NAGw_, 0, "1" );
static const int M1_NAGw = M1_NAGw_.value * (1);
pdesc( M1_M1b_, 0, "1111111111110000000" );
static const int M1_M1b = M1_M1b_.value * (1);
pdesc( M1_M2b_, 0, "00001000100000000" );
static const int M1_M2b = M1_M2b_.value * (1);
pdesc( M1_NWb_, 0, "11000110010010000000" );
static const int M1_NWb = M1_NWb_.value * (1);
pdesc( M1_INCb_, 0, "100100001000000001" );
static const int M1_INCb = M1_INCb_.value * (1);
pdesc( M1_STEPb_, 0, "1110110100010010001" );
static const int M1_STEPb = M1_STEPb_.value * (1);
pdesc( M1_G2b_, 0, "0000000" );
static const int M1_G2b = M1_G2b_.value * (1);
pdesc( M1_DCb_, 0, "001100111" );
static const int M1_DCb = M1_DCb_.value * (1);
pdesc( M1_RCb_, 0, "100000000000000" );
static const int M1_RCb = M1_RCb_.value * (1);
pdesc( M1_NAGb_, 0, "1" );
static const int M1_NAGb = M1_NAGb_.value * (1);
pdesc( M1_AGAw_, 0, "010011000100" );
static const int M1_AGAw = M1_AGAw_.value * (1);
pdesc( M1_AGAb_, 0, "000000000000" );
static const int M1_AGAb = M1_AGAb_.value * (1);
pdesc( M1_AGB_, 0, "1011001000" );
static const int M1_AGB = M1_AGB_.value * (1);
pdesc( M1_E2E_, 0, "100000000" );
static const int M1_E2E = M1_E2E_.value * (1);



static uint M1_MakeCx( uint k, uint st, uint e ) {
  int Cx;

  Cx = 0;
  M1_k.inc( Cx, k );
  M1_s.inc( Cx, st );
  M1_e.inc( Cx, e );
  return uint(Cx);
}

