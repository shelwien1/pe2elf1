
// Generated into MOD/sh_model-C0_p.inc: the C0 knobs and context masks
// (MakeTables), the cell-context builder (MakeIndex).  The order-1 cells
// are a Table(), one per context: a fixed array member of C0_T in the
// shipping build, a pointer allocated in C0_Init() in the tuning build.

static const byte C0_cx[256]={ 0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254 };


static const int C0_mwMax = (32767+1) * (1);
static const int C0_G3_m = (262144+0) * (1);
static const int C0_P0 = (19725+1) * (1);
static const int C0_P1 = (6161+0) * (1);
static const int C0_wr = (9434+1) * (1);
static const int C0_wr1 = (2506+0) * (1);
static const int C0_wr1b = (9434+1) * (1);
static const int C0_mw = (3519+1) * (1);
static const int C0_K = (18911+0) * (1);
static const int C0_NWk = (127436+1) * (1);
static const int C0_RKinc = (2673+1) * (1);
static const int C0_kStep = (94946+1) * (1);
static const int C0_kMin = (1384512+1) * (1);
static const int C0_kMax = (7177+1) * (1);
static const int C0_NWm = (708768+1) * (1);
static const int C0_MWinc = (26+1) * (1);
static const int C0_mwStep = (245524+1) * (1);
static const int C0_mwMin = (12+1) * (1);
static const int C0_M1_m = (260224+1) * (1);
static const int C0_M2_m = (8128+1) * (1);
static const int C0_M1_k = (822416+1) * (1);
static const int C0_M2_k = (1044+1) * (1);
static const int C0_G2_m = (8+0) * (1);
static const int C0_G2_k = (4+0) * (1);
static const int C0_stP_min = (73+1) * (1);
static const int C0_leak1 = (31789+1) * (1);
static const int C0_leak2 = (31618+1) * (1);
static const int C0_mwP0 = (18849+1) * (1);
static const int C0_G3_k = (74503+0) * (1);
static const int C0_G4_m = (131490+0) * (1);
static const int C0_G4_k = (5+0) * (1);
static const int C0_M1_u = (235648+0) * (1);
static const int C0_M2_u = (2014+0) * (1);
static const int C0_M2_x = (2014+0) * (1);
static const int C0_NWu = (400920+0) * (1);
static const int C0_RUinc = (184884+0) * (1);
static const int C0_uStep = (68580+0) * (1);
static const int C0_uMin = (1012+0) * (1);
static const int C0_uMax = (3328032+0) * (1);
static const int C0_vMin = (1012+0) * (1);
static const int C0_vMax = (3328032+0) * (1);
static const int C0_G2_u = (0+0) * (1);
static const int C0_G3_u = (262144+0) * (1);
static const int C0_G4_u = (63+0) * (1);
static const int C0_R0_u = (149+0) * (1);
static const int C0_M1_v = (253864+0) * (1);
static const int C0_M2_v = (15+0) * (1);
static const int C0_NWv = (411904+0) * (1);
static const int C0_RVinc = (749570+0) * (1);
static const int C0_vStep = (43392+0) * (1);
static const int C0_G2_v = (1+0) * (1);
static const int C0_G3_v = (262144+0) * (1);
static const int C0_G4_v = (205+0) * (1);
static const int C0_R0_v = (188+0) * (1);
static const int C0_UVH = (642+0) * (1);
static const int C0_CXW = (1057+0) * (1);
static const int C0_XHW = (920+0) * (1);
static const int C0_UVDET = (162+0) * (1);
static const int C0_XHC = (3200+0) * (1);
static const int C0_mwXlo = (449+0) * (1);
static const int C0_mwXhi = (17724+0) * (1);
static const int C0_AGAu = (96+0) * (1);
static const int C0_AGAv = (96+0) * (1);
static const int C0_AGAm = (2024+0) * (1);
static const int C0_AGAk = (1+0) * (1);
static const int C0_AGB = (24+0) * (1);
static const int C0_AGBuv = (24+0) * (1);
static const int C0_E2E = (256+0) * (1);
static const int C0_E2Euv = (256+0) * (1);



static uint C0_MakeCx( uint c1, uint cxt ) {
  int Cx;

  Cx = 0;
  Cx = (Cx<<8) + (((c1)&255)); // 11111111
  Cx = (Cx*255) + C0_cx[(cxt)<1?1:((cxt)>255?255:(cxt))]; // 00000000
  return uint(Cx);
}

