
// Generated into MOD/sh_model-S0_p.inc: the S0 knobs and context masks
// (MakeTables), the row-context builder (MakeIndex).  The SSE cells are a
// Table(), NB per row context: a fixed array member of S0_T in the shipping
// build (NB folded), a pointer allocated in S0_Init() in the tuning build.

static const byte S0_cx[256]={ 0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254 };


static const int S0_NB = (7+2) * (1);
static const int S0_LIM = (1007+0) * (1);
static const int S0_T0 = (8121+0) * (1);
static const int S0_UPD = (3+0) * (1);
static const int S0_UPMIN = (33+0) * (1);
static const int S0_wr = (8350+1) * (1);
static const int S0_wr1 = (3588+0) * (1);
static const int S0_wr1b = (15165+1) * (1);
static const int S0_mw = (1986+1) * (1);
static const int S0_K = (32814+0) * (1);
static const int S0_NWk = (22428+1) * (1);
static const int S0_RKinc = (20471+1) * (1);
static const int S0_kStep = (8222+1) * (1);
static const int S0_kMin = (475152+1) * (1);
static const int S0_kMax = (19356+1) * (1);
static const int S0_M1_k = (1029491+1) * (1);
static const int S0_M2_k = (0+1) * (1);
static const int S0_G2_k = (1+0) * (1);
static const int S0_G3_k = (98309+0) * (1);
static const int S0_G4_k = (7+0) * (1);
static const int S0_NWm = (284016+1) * (1);
static const int S0_MWinc = (12215+1) * (1);
static const int S0_mwStep = (123904+1) * (1);
static const int S0_mwMin = (1234+1) * (1);
static const int S0_mwMax = (3970+1) * (1);
static const int S0_M1_m = (254164+1) * (1);
static const int S0_M2_m = (130+1) * (1);
static const int S0_G2_m = (10+0) * (1);
static const int S0_G3_m = (262144+0) * (1);
static const int S0_G4_m = (131490+0) * (1);
static const int S0_mwXlo = (377+0) * (1);
static const int S0_mwXhi = (14303+0) * (1);
static const int S0_mwP0 = (17791+1) * (1);
static const int S0_M1_u = (214360+0) * (1);
static const int S0_M2_u = (0+0) * (1);
static const int S0_M2_x = (1104+0) * (1);
static const int S0_NWu = (519634+0) * (1);
static const int S0_RUinc = (62510+0) * (1);
static const int S0_uStep = (74106+0) * (1);
static const int S0_uMin = (13+0) * (1);
static const int S0_uMax = (89625+0) * (1);
static const int S0_vMin = (48+0) * (1);
static const int S0_vMax = (2895360+0) * (1);
static const int S0_G2_u = (3+0) * (1);
static const int S0_G3_u = (262144+0) * (1);
static const int S0_G4_u = (17+0) * (1);
static const int S0_R0_u = (2+0) * (1);
static const int S0_M1_v = (136505+0) * (1);
static const int S0_M2_v = (51+0) * (1);
static const int S0_NWv = (286004+0) * (1);
static const int S0_RVinc = (689057+0) * (1);
static const int S0_vStep = (2177+0) * (1);
static const int S0_G2_v = (96+0) * (1);
static const int S0_G3_v = (8+0) * (1);
static const int S0_G4_v = (611+0) * (1);
static const int S0_R0_v = (254+0) * (1);
static const int S0_UVH = (256+0) * (1);
static const int S0_CXW = (441+0) * (1);
static const int S0_XHW = (1424+0) * (1);
static const int S0_UVDET = (356+0) * (1);
static const int S0_XHC = (66+0) * (1);
static const int S0_leak1 = (61940+1) * (1);
static const int S0_leak2 = (60705+1) * (1);
static const int S0_stP_min = (31997+1) * (1);
static const int S0_AGAu = (11+0) * (1);
static const int S0_AGAv = (511+0) * (1);
static const int S0_AGAm = (1715+0) * (1);
static const int S0_AGAk = (0+0) * (1);
static const int S0_AGB = (323+0) * (1);
static const int S0_AGBuv = (155+0) * (1);
static const int S0_E2E = (511+0) * (1);
static const int S0_E2Euv = (362+0) * (1);



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

