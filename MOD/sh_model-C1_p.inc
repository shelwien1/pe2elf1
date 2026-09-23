
// Generated into MOD/sh_model-C1_p.inc: the C1 knobs and context masks
// (MakeTables), the cell-context builder (MakeIndex).  The C1 cells are a
// Table(), one per context: a fixed array member of C1_T in the shipping
// build, a pointer allocated in C1_Init() in the tuning build.  Same cell
// type and bundle layout as C0 (config.hpp), its own knobs: the second
// counter table the mixer blends with C0.

static const byte C1_cx[256]={ 0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254 };


static const int C1_mwMax = (32760+1) * (1);
static const int C1_G3_m = (262144+0) * (1);
static const int C1_P0 = (19588+1) * (1);
static const int C1_P1 = (5633+0) * (1);
static const int C1_wr = (8259+1) * (1);
static const int C1_wr1 = (2593+0) * (1);
static const int C1_wr1b = (12241+1) * (1);
static const int C1_mw = (2212+1) * (1);
static const int C1_K = (21632+0) * (1);
static const int C1_NWk = (164520+1) * (1);
static const int C1_RKinc = (8475+1) * (1);
static const int C1_kStep = (111156+1) * (1);
static const int C1_kMin = (4189696+1) * (1);
static const int C1_kMax = (6920+1) * (1);
static const int C1_NWm = (346138+1) * (1);
static const int C1_MWinc = (31892+1) * (1);
static const int C1_mwStep = (260096+1) * (1);
static const int C1_mwMin = (0+1) * (1);
static const int C1_M1_m = (261888+1) * (1);
static const int C1_M2_m = (6072+1) * (1);
static const int C1_M1_k = (1021633+1) * (1);
static const int C1_M2_k = (193536+1) * (1);
static const int C1_G2_m = (6671+0) * (1);
static const int C1_G2_k = (16579+0) * (1);
static const int C1_stP_min = (6200+1) * (1);
static const int C1_leak1 = (32272+1) * (1);
static const int C1_leak2 = (63235+1) * (1);
static const int C1_mwP0 = (20419+1) * (1);
static const int C1_G3_k = (74503+0) * (1);
static const int C1_G4_m = (131490+0) * (1);
static const int C1_G4_k = (128+0) * (1);
static const int C1_M1_u = (261120+0) * (1);
static const int C1_M2_u = (2027+0) * (1);
static const int C1_M2_x = (1718+0) * (1);
static const int C1_NWu = (395520+0) * (1);
static const int C1_RUinc = (256546+0) * (1);
static const int C1_uStep = (493048+0) * (1);
static const int C1_uMin = (222+0) * (1);
static const int C1_uMax = (3162240+0) * (1);
static const int C1_vMin = (972+0) * (1);
static const int C1_vMax = (3328032+0) * (1);
static const int C1_G2_u = (0+0) * (1);
static const int C1_G3_u = (262144+0) * (1);
static const int C1_G4_u = (107+0) * (1);
static const int C1_R0_u = (156+0) * (1);
static const int C1_M1_v = (261728+0) * (1);
static const int C1_M2_v = (296+0) * (1);
static const int C1_NWv = (437536+0) * (1);
static const int C1_RVinc = (327680+0) * (1);
static const int C1_vStep = (40576+0) * (1);
static const int C1_G2_v = (2+0) * (1);
static const int C1_G3_v = (262144+0) * (1);
static const int C1_G4_v = (129+0) * (1);
static const int C1_R0_v = (0+0) * (1);
static const int C1_UVH = (1088+0) * (1);
static const int C1_CXW = (1015+0) * (1);
static const int C1_XHW = (920+0) * (1);
static const int C1_UVDET = (184+0) * (1);
static const int C1_XHC = (0+0) * (1);
static const int C1_mwXlo = (0+0) * (1);
static const int C1_mwXhi = (16384+0) * (1);
static const int C1_AGAu = (0+0) * (1);
static const int C1_AGAv = (1024+0) * (1);
static const int C1_AGAm = (126+0) * (1);
static const int C1_AGAk = (69+0) * (1);
static const int C1_AGB = (276+0) * (1);
static const int C1_AGBuv = (332+0) * (1);
static const int C1_E2E = (256+0) * (1);
static const int C1_E2Euv = (202+0) * (1);



static uint C1_MakeCx( uint c2, uint c1, uint cxt ) {
  int Cx;

  Cx = 0;
  Cx = (Cx<<8) + (((c1)&255)); // 11111111
  Cx = (Cx*255) + C1_cx[(cxt)<1?1:((cxt)>255?255:(cxt))]; // 00000000
  return uint(Cx);
}

