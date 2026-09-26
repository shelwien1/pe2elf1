// Generated into MOD/tc_model-M3_p.inc: the M3 knobs and context masks
// (MakeTables), the mixer-context builder (MakeIndex).

pmask( M3_k, "1" );
pdesc( M3_s, 1, "111" );
pmask( M3_p, "001101" );


pdesc( M3_W0_, 0, "110000001000000" );
static const int M3_W0 = M3_W0_.value * (1);
pdesc( M3_Wclip_, 0, "000101000110" );
static const int M3_Wclip = M3_Wclip_.value * (1);
pdesc( M3_Bclip_, 0, "00000100100" );
static const int M3_Bclip = M3_Bclip_.value * (1);
pdesc( M3_Pmin_, 0, "101001111100011" );
static const int M3_Pmin = M3_Pmin_.value * (1);
pdesc( M3_M1w_, 0, "1111111111000010000" );
static const int M3_M1w = M3_M1w_.value * (1);
pdesc( M3_M2w_, 0, "10100000100100000" );
static const int M3_M2w = M3_M2w_.value * (1);
pdesc( M3_NWw_, 0, "1010000100000100000" );
static const int M3_NWw = M3_NWw_.value * (1);
pdesc( M3_INCw_, 0, "010110000000110100" );
static const int M3_INCw = M3_INCw_.value * (1);
pdesc( M3_STEPw_, 0, "1110000100100000000" );
static const int M3_STEPw = M3_STEPw_.value * (1);
pdesc( M3_G2w_, 0, "0000010" );
static const int M3_G2w = M3_G2w_.value * (1);
pdesc( M3_DCw_, 0, "101111001" );
static const int M3_DCw = M3_DCw_.value * (1);
pdesc( M3_RCw_, 0, "001000100100100" );
static const int M3_RCw = M3_RCw_.value * (1);
pdesc( M3_NAGw_, 0, "1" );
static const int M3_NAGw = M3_NAGw_.value * (1);
pdesc( M3_M1b_, 0, "1111111111110000000" );
static const int M3_M1b = M3_M1b_.value * (1);
pdesc( M3_M2b_, 0, "00011100101000001" );
static const int M3_M2b = M3_M2b_.value * (1);
pdesc( M3_NWb_, 0, "10100000000010100010" );
static const int M3_NWb = M3_NWb_.value * (1);
pdesc( M3_INCb_, 0, "100100001000000001" );
static const int M3_INCb = M3_INCb_.value * (1);
pdesc( M3_STEPb_, 0, "0011011000000000000" );
static const int M3_STEPb = M3_STEPb_.value * (1);
pdesc( M3_G2b_, 0, "0100001" );
static const int M3_G2b = M3_G2b_.value * (1);
pdesc( M3_DCb_, 0, "001010000" );
static const int M3_DCb = M3_DCb_.value * (1);
pdesc( M3_RCb_, 0, "100000000000000" );
static const int M3_RCb = M3_RCb_.value * (1);
pdesc( M3_NAGb_, 0, "1" );
static const int M3_NAGb = M3_NAGb_.value * (1);
pdesc( M3_AGAw_, 0, "011111000100" );
static const int M3_AGAw = M3_AGAw_.value * (1);
pdesc( M3_AGAb_, 0, "100001000000" );
static const int M3_AGAb = M3_AGAb_.value * (1);
pdesc( M3_AGB_, 0, "1111000010" );
static const int M3_AGB = M3_AGB_.value * (1);
pdesc( M3_E2E_, 0, "100000000" );
static const int M3_E2E = M3_E2E_.value * (1);



static uint M3_MakeCx( uint k, uint st, uint pq ) {
  int Cx;

  Cx = 0;
  M3_k.inc( Cx, k );
  M3_s.inc( Cx, st );
  M3_p.inc( Cx, pq );
  return uint(Cx);
}

