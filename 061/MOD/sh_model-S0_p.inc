
// Generated into MOD/sh_model-S0_p.inc: the S0 knobs and context masks
// (MakeTables), the row-context builders (MakeIndex).  The cell storage is
// a Table(): a fixed array member of S0_T in the shipping build, a pointer
// allocated in S0_Init() in the tuning build (IDX-FORMAT.md sec.9); the
// size helper is defined by the coder before MOD/sh_model-S0_h.inc.

static const byte S0_cx[256]={ 0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254 };



static const int S0_HBITS = (22+0) * (1);
static const int S0_NB = (2+2) * (1);
static const int S0_LIM = (1444+0) * (1);
static const int S0_T0 = (308+0) * (1);
static const int S0_ON = (1+0) * (1);
static const int S0_UPD = (1+0) * (1);
static const int S0_UPMIN = (0+0) * (1);
static const int S0_P0 = (18952+1) * (1);
static const int S0_P1 = (5137+0) * (1);
static const int S0_wr = (4777+1) * (1);
static const int S0_wr1 = (2356+0) * (1);
static const int S0_mw = (2112+1) * (1);
static const int S0_K = (35677+0) * (1);
static const int S0_NWk = (69564+1) * (1);
static const int S0_RKinc = (3797+1) * (1);
static const int S0_kStep = (130123+1) * (1);
static const int S0_kMin = (4079617+1) * (1);
static const int S0_kMax = (15039+1) * (1);
static const int S0_M1_k = (864072+1) * (1);
static const int S0_M2_k = (6368+1) * (1);
static const int S0_G1_k = (5000+0) * (1);
static const int S0_G2_k = (1+0) * (1);
static const int S0_G3_k = (74503+0) * (1);
static const int S0_G4_k = (34740+0) * (1);
static const int S0_NWm = (133737+1) * (1);
static const int S0_MWinc = (256+1) * (1);
static const int S0_mwStep = (261953+1) * (1);
static const int S0_mwMin = (1600+1) * (1);
static const int S0_mwMax = (32767+1) * (1);
static const int S0_M1_m = (138225+1) * (1);
static const int S0_M2_m = (16448+1) * (1);
static const int S0_G1_m = (5000+0) * (1);
static const int S0_G2_m = (33+0) * (1);
static const int S0_G3_m = (262144+0) * (1);
static const int S0_G4_m = (131490+0) * (1);
static const int S0_mwXlo = (448+0) * (1);
static const int S0_mwXhi = (27473+0) * (1);
static const int S0_mwP0 = (14856+1) * (1);
static const int S0_M1_u = (155232+0) * (1);
static const int S0_M2_u = (1908+0) * (1);
static const int S0_NWu = (451109+0) * (1);
static const int S0_RUinc = (640+0) * (1);
static const int S0_uStep = (115720+0) * (1);
static const int S0_uMin = (1+0) * (1);
static const int S0_uMax = (4247560+0) * (1);
static const int S0_G1_u = (5000+0) * (1);
static const int S0_G2_u = (1+0) * (1);
static const int S0_G3_u = (262144+0) * (1);
static const int S0_G4_u = (132+0) * (1);
static const int S0_R0_u = (145+0) * (1);
static const int S0_M1_v = (178304+0) * (1);
static const int S0_M2_v = (165+0) * (1);
static const int S0_NWv = (288375+0) * (1);
static const int S0_RVinc = (32+0) * (1);
static const int S0_vStep = (54994+0) * (1);
static const int S0_G1_v = (5000+0) * (1);
static const int S0_G2_v = (0+0) * (1);
static const int S0_G3_v = (262144+0) * (1);
static const int S0_G4_v = (1024+0) * (1);
static const int S0_R0_v = (128+0) * (1);
static const int S0_UVH = (906+0) * (1);
static const int S0_CXW = (1020+0) * (1);
static const int S0_XHW = (1328+0) * (1);
static const int S0_UVDET = (65+0) * (1);
static const int S0_XHC = (0+0) * (1);
static const int S0_leak1 = (32208+1) * (1);
static const int S0_leak2 = (28634+1) * (1);
static const int S0_stP_min = (0+1) * (1);
static const int S0_AGAu = (504+0) * (1);
static const int S0_AGAm = (1263+0) * (1);
static const int S0_AGAk = (768+0) * (1);
static const int S0_AGB = (256+0) * (1);
static const int S0_E2E = (104+0) * (1);



// Row context of the SSE stage: c3/c2/c1 = the three last bytes, cxt = the
// bit-tree node (1..255).  The coder combines the two results in 64 bits:
// row = Cx * S0_Cx3_Volume + Cx3.
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
  Cx3 = (Cx3<<7) + (((c3)&127)); // 01111111
  return uint(Cx3);
}

