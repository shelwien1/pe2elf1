// Generated into MOD/tc_model-M2_p.inc: the M2 knobs and context masks
// (MakeTables), the mixer-context builder (MakeIndex).

pmask( M2_k, "1" );
pdesc( M2_s, 1, "111" );
pmask( M2_p, "001101" );


pdesc( M2_W0_, 0, "110000001000000" );
static const int M2_W0 = M2_W0_.value * (1);
pdesc( M2_Wclip_, 0, "000101000110" );
static const int M2_Wclip = M2_Wclip_.value * (1);
pdesc( M2_Bclip_, 0, "00000100100" );
static const int M2_Bclip = M2_Bclip_.value * (1);
pdesc( M2_Pmin_, 0, "101001111100011" );
static const int M2_Pmin = M2_Pmin_.value * (1);
pdesc( M2_M1w_, 0, "1111111111000010000" );
static const int M2_M1w = M2_M1w_.value * (1);
pdesc( M2_M2w_, 0, "10100000100100000" );
static const int M2_M2w = M2_M2w_.value * (1);
pdesc( M2_NWw_, 0, "1010000100000100000" );
static const int M2_NWw = M2_NWw_.value * (1);
pdesc( M2_INCw_, 0, "010110000000110100" );
static const int M2_INCw = M2_INCw_.value * (1);
pdesc( M2_STEPw_, 0, "1110000100100000000" );
static const int M2_STEPw = M2_STEPw_.value * (1);
pdesc( M2_G2w_, 0, "0000010" );
static const int M2_G2w = M2_G2w_.value * (1);
pdesc( M2_DCw_, 0, "101111001" );
static const int M2_DCw = M2_DCw_.value * (1);
pdesc( M2_RCw_, 0, "001000100100100" );
static const int M2_RCw = M2_RCw_.value * (1);
pdesc( M2_NAGw_, 0, "1" );
static const int M2_NAGw = M2_NAGw_.value * (1);
pdesc( M2_M1b_, 0, "1111111111110000000" );
static const int M2_M1b = M2_M1b_.value * (1);
pdesc( M2_M2b_, 0, "00011100101000001" );
static const int M2_M2b = M2_M2b_.value * (1);
pdesc( M2_NWb_, 0, "10100000000010100010" );
static const int M2_NWb = M2_NWb_.value * (1);
pdesc( M2_INCb_, 0, "100100001000000001" );
static const int M2_INCb = M2_INCb_.value * (1);
pdesc( M2_STEPb_, 0, "0011011000000000000" );
static const int M2_STEPb = M2_STEPb_.value * (1);
pdesc( M2_G2b_, 0, "0100001" );
static const int M2_G2b = M2_G2b_.value * (1);
pdesc( M2_DCb_, 0, "001010000" );
static const int M2_DCb = M2_DCb_.value * (1);
pdesc( M2_RCb_, 0, "100000000000000" );
static const int M2_RCb = M2_RCb_.value * (1);
pdesc( M2_NAGb_, 0, "1" );
static const int M2_NAGb = M2_NAGb_.value * (1);
pdesc( M2_AGAw_, 0, "011111000100" );
static const int M2_AGAw = M2_AGAw_.value * (1);
pdesc( M2_AGAb_, 0, "100001000000" );
static const int M2_AGAb = M2_AGAb_.value * (1);
pdesc( M2_AGB_, 0, "1111000010" );
static const int M2_AGB = M2_AGB_.value * (1);
pdesc( M2_E2E_, 0, "100000000" );
static const int M2_E2E = M2_E2E_.value * (1);



static uint M2_MakeCx( uint k, uint st, uint pq ) {
  int Cx;

  Cx = 0;
  M2_k.inc( Cx, k );
  M2_s.inc( Cx, st );
  M2_p.inc( Cx, pq );
  return uint(Cx);
}

