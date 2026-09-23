
// Generated into MOD/sh_model-S0_p.inc: the S0 knobs and context masks
// (MakeTables), the row-context builder (MakeIndex).  The SSE cells are a
// Table(), NB per row context: a fixed array member of S0_T in the shipping
// build (NB folded), a pointer allocated in S0_Init() in the tuning build.

static const byte S0_cx[256]={ 0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254 };


static const int S0_NB = (2+2) * (1);
static const int S0_LIM = (463+0) * (1);
static const int S0_T0 = (412+0) * (1);
static const int S0_UPD = (1+0) * (1);
static const int S0_UPMIN = (0+0) * (1);
static const int S0_wr = (8178+1) * (1);
static const int S0_wr1 = (4090+0) * (1);
static const int S0_wr1b = (8032+1) * (1);
static const int S0_mw = (1986+1) * (1);
static const int S0_K = (37239+0) * (1);
static const int S0_NWk = (169737+1) * (1);
static const int S0_RKinc = (11001+1) * (1);
static const int S0_kStep = (94785+1) * (1);
static const int S0_kMin = (475152+1) * (1);
static const int S0_kMax = (14218+1) * (1);
static const int S0_M1_k = (1033361+1) * (1);
static const int S0_M2_k = (258337+1) * (1);
static const int S0_G2_k = (1+0) * (1);
static const int S0_G3_k = (98309+0) * (1);
static const int S0_G4_k = (4+0) * (1);
static const int S0_NWm = (163910+1) * (1);
static const int S0_MWinc = (25744+1) * (1);
static const int S0_mwStep = (60945+1) * (1);
static const int S0_mwMin = (2145+1) * (1);
static const int S0_mwMax = (32728+1) * (1);
static const int S0_M1_m = (209984+1) * (1);
static const int S0_M2_m = (27011+1) * (1);
static const int S0_G2_m = (19+0) * (1);
static const int S0_G3_m = (262144+0) * (1);
static const int S0_G4_m = (131490+0) * (1);
static const int S0_mwXlo = (377+0) * (1);
static const int S0_mwXhi = (16393+0) * (1);
static const int S0_mwP0 = (13345+1) * (1);
static const int S0_M1_u = (223812+0) * (1);
static const int S0_M2_u = (11+0) * (1);
static const int S0_M2_x = (1914+0) * (1);
static const int S0_NWu = (519634+0) * (1);
static const int S0_RUinc = (196608+0) * (1);
static const int S0_uStep = (74106+0) * (1);
static const int S0_uMin = (48+0) * (1);
static const int S0_uMax = (4265850+0) * (1);
static const int S0_vMin = (48+0) * (1);
static const int S0_vMax = (2895360+0) * (1);
static const int S0_G2_u = (0+0) * (1);
static const int S0_G3_u = (262144+0) * (1);
static const int S0_G4_u = (17+0) * (1);
static const int S0_R0_u = (25+0) * (1);
static const int S0_M1_v = (184384+0) * (1);
static const int S0_M2_v = (484+0) * (1);
static const int S0_NWv = (485804+0) * (1);
static const int S0_RVinc = (557712+0) * (1);
static const int S0_vStep = (62656+0) * (1);
static const int S0_G2_v = (1+0) * (1);
static const int S0_G3_v = (4+0) * (1);
static const int S0_G4_v = (41+0) * (1);
static const int S0_R0_v = (62+0) * (1);
static const int S0_UVH = (906+0) * (1);
static const int S0_CXW = (473+0) * (1);
static const int S0_XHW = (1424+0) * (1);
static const int S0_UVDET = (121+0) * (1);
static const int S0_XHC = (0+0) * (1);
static const int S0_leak1 = (61940+1) * (1);
static const int S0_leak2 = (60705+1) * (1);
static const int S0_stP_min = (0+1) * (1);
static const int S0_AGAu = (11+0) * (1);
static const int S0_AGAv = (511+0) * (1);
static const int S0_AGAm = (1536+0) * (1);
static const int S0_AGAk = (26+0) * (1);
static const int S0_AGB = (168+0) * (1);
static const int S0_AGBuv = (154+0) * (1);
static const int S0_E2E = (248+0) * (1);
static const int S0_E2Euv = (474+0) * (1);



// Row context of the SSE stage: c1 = the last byte, cxt = the bit-tree node
// (1..255).  One row of NB cells per context, starting at cell
// S0_MakeCx(c1, cxt) * CP_S0::NB.
static uint S0_MakeCx( uint c1, uint cxt ) {
  int Cx;

  Cx = 0;
  Cx = (Cx<<5) + ((((c1)>>3)&31)); // 11111000
  Cx = (Cx*255) + S0_cx[(cxt)<1?1:((cxt)>255?255:(cxt))]; // 00000000
  return uint(Cx);
}

