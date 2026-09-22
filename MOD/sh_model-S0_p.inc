
// Generated into MOD/sh_model-S0_p.inc: the S0 knobs and context masks
// (MakeTables), the row-context builders (MakeIndex).  The rows are declared
// below through def_Data (SSE-MIX2-REDESIGN.md sec.2.4): a fixed array of
// SSE_Row<NB> in the shipping build, NB folded; in the tuning build NB is a
// load-time value, so the table is an SSE_Tbl<CP_S0> that instantiates the
// row for every NB and dispatches on the one in force -- allocated in
// S0_Init() and freed in S0_Quit() like a Table() member.

static const byte S0_cx[256]={ 0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254 };


static const int S0_NB = (2+2) * (1);
static const int S0_LIM = (587+0) * (1);
static const int S0_T0 = (187+0) * (1);
static const int S0_UPD = (1+0) * (1);
static const int S0_UPMIN = (0+0) * (1);
static const int S0_wr = (4634+1) * (1);
static const int S0_wr1 = (3522+0) * (1);
static const int S0_mw = (1490+1) * (1);
static const int S0_K = (36995+0) * (1);
static const int S0_NWk = (64259+1) * (1);
static const int S0_RKinc = (3220+1) * (1);
static const int S0_kStep = (94785+1) * (1);
static const int S0_kMin = (4079617+1) * (1);
static const int S0_kMax = (12302+1) * (1);
static const int S0_M1_k = (732192+1) * (1);
static const int S0_M2_k = (98560+1) * (1);
static const int S0_G2_k = (1+0) * (1);
static const int S0_G3_k = (74503+0) * (1);
static const int S0_G4_k = (6+0) * (1);
static const int S0_NWm = (163910+1) * (1);
static const int S0_MWinc = (256+1) * (1);
static const int S0_mwStep = (262088+1) * (1);
static const int S0_mwMin = (2113+1) * (1);
static const int S0_mwMax = (32767+1) * (1);
static const int S0_M1_m = (209984+1) * (1);
static const int S0_M2_m = (11080+1) * (1);
static const int S0_G2_m = (33+0) * (1);
static const int S0_G3_m = (262144+0) * (1);
static const int S0_G4_m = (131490+0) * (1);
static const int S0_mwXlo = (260+0) * (1);
static const int S0_mwXhi = (12063+0) * (1);
static const int S0_mwP0 = (14856+1) * (1);
static const int S0_M1_u = (116480+0) * (1);
static const int S0_M2_u = (28+0) * (1);
static const int S0_NWu = (519634+0) * (1);
static const int S0_RUinc = (49280+0) * (1);
static const int S0_uStep = (118568+0) * (1);
static const int S0_uMin = (48+0) * (1);
static const int S0_uMax = (2895360+0) * (1);
static const int S0_G2_u = (1+0) * (1);
static const int S0_G3_u = (262144+0) * (1);
static const int S0_G4_u = (80+0) * (1);
static const int S0_R0_u = (131+0) * (1);
static const int S0_M1_v = (184384+0) * (1);
static const int S0_M2_v = (163+0) * (1);
static const int S0_NWv = (475968+0) * (1);
static const int S0_RVinc = (32+0) * (1);
static const int S0_vStep = (65217+0) * (1);
static const int S0_G2_v = (0+0) * (1);
static const int S0_G3_v = (262144+0) * (1);
static const int S0_G4_v = (140+0) * (1);
static const int S0_R0_v = (66+0) * (1);
static const int S0_UVH = (906+0) * (1);
static const int S0_CXW = (1023+0) * (1);
static const int S0_XHW = (1328+0) * (1);
static const int S0_UVDET = (16+0) * (1);
static const int S0_XHC = (0+0) * (1);
static const int S0_leak1 = (32208+1) * (1);
static const int S0_leak2 = (26912+1) * (1);
static const int S0_stP_min = (0+1) * (1);
static const int S0_AGAu = (923+0) * (1);
static const int S0_AGAm = (1889+0) * (1);
static const int S0_AGAk = (1521+0) * (1);
static const int S0_AGB = (256+0) * (1);
static const int S0_E2E = (219+0) * (1);



// Row context of the SSE stage: c3/c2/c1 = the three last bytes, cxt = the
// bit-tree node (1..255); one row per context, S0_tbl[S0_MakeCx(..)].
static uint S0_MakeCx( uint c3, uint c2, uint c1, uint cxt ) {
  int Cx;

  Cx = 0;
  Cx = (Cx<<5) + (((c3)&31)); // 11111
  Cx = (Cx<<5) + (((c2)&31)); // 11111
  Cx = (Cx<<4) + (((c1)&15)); // 1111
  Cx = (Cx*255) + S0_cx[(cxt)<1?1:((cxt)>255?255:(cxt))]; // 00000000
  return uint(Cx);
}

