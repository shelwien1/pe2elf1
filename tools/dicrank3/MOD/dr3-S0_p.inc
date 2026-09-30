// dicrank3's SSE rows: stream x stage x small context; NB cells per row

static const byte S0_e[15+1]={ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };


static const int S0_NB = (7+2) * (1);
static const int S0_LIM = (1007+0) * (1);
static const int S0_T0 = (1593+0) * (1);
static const int S0_UPD = (3+0) * (1);
static const int S0_UPMIN = (9+0) * (1);
static const int S0_wr = (8350+1) * (1);
static const int S0_wr1 = (3588+0) * (1);
static const int S0_wr1b = (15165+1) * (1);
static const int S0_mw = (5024+1) * (1);
static const int S0_K = (48340+0) * (1);
static const int S0_NWk = (22428+1) * (1);
static const int S0_RKinc = (20471+1) * (1);
static const int S0_kStep = (8222+1) * (1);
static const int S0_kMin = (475152+1) * (1);
static const int S0_kMax = (16384+1) * (1);
static const int S0_M1_k = (1029491+1) * (1);
static const int S0_M2_k = (0+1) * (1);
static const int S0_G2_k = (0+0) * (1);
static const int S0_G3_k = (98309+0) * (1);
static const int S0_G4_k = (7+0) * (1);
static const int S0_NWm = (86346+1) * (1);
static const int S0_MWinc = (24708+1) * (1);
static const int S0_mwStep = (123904+1) * (1);
static const int S0_mwMin = (1234+1) * (1);
static const int S0_mwMax = (3970+1) * (1);
static const int S0_M1_m = (229380+1) * (1);
static const int S0_M2_m = (130+1) * (1);
static const int S0_G2_m = (10+0) * (1);
static const int S0_G3_m = (262144+0) * (1);
static const int S0_G4_m = (131490+0) * (1);
static const int S0_mwXlo = (377+0) * (1);
static const int S0_mwXhi = (11882+0) * (1);
static const int S0_mwP0 = (17791+1) * (1);
static const int S0_M1_u = (214360+0) * (1);
static const int S0_M2_u = (0+0) * (1);
static const int S0_M2_x = (1104+0) * (1);
static const int S0_NWu = (519634+0) * (1);
static const int S0_RUinc = (62510+0) * (1);
static const int S0_uStep = (74106+0) * (1);
static const int S0_uMin = (896+0) * (1);
static const int S0_uMax = (89625+0) * (1);
static const int S0_vMin = (48+0) * (1);
static const int S0_vMax = (788036+0) * (1);
static const int S0_G2_u = (2+0) * (1);
static const int S0_G3_u = (0+0) * (1);
static const int S0_G4_u = (17+0) * (1);
static const int S0_R0_u = (2+0) * (1);
static const int S0_M1_v = (32768+0) * (1);
static const int S0_M2_v = (51+0) * (1);
static const int S0_NWv = (842032+0) * (1);
static const int S0_RVinc = (689057+0) * (1);
static const int S0_vStep = (1040+0) * (1);
static const int S0_G2_v = (96+0) * (1);
static const int S0_G3_v = (8+0) * (1);
static const int S0_G4_v = (0+0) * (1);
static const int S0_R0_v = (254+0) * (1);
static const int S0_UVH = (256+0) * (1);
static const int S0_CXW = (441+0) * (1);
static const int S0_XHW = (1424+0) * (1);
static const int S0_UVDET = (356+0) * (1);
static const int S0_XHC = (66+0) * (1);
static const int S0_leak1 = (61940+1) * (1);
static const int S0_leak2 = (60705+1) * (1);
static const int S0_stP_min = (5122+1) * (1);
static const int S0_AGAu = (11+0) * (1);
static const int S0_AGAv = (511+0) * (1);
static const int S0_AGAm = (1160+0) * (1);
static const int S0_AGAk = (3592+0) * (1);
static const int S0_AGB = (323+0) * (1);
static const int S0_AGBuv = (155+0) * (1);
static const int S0_E2E = (511+0) * (1);
static const int S0_E2Euv = (362+0) * (1);



static uint S0_MakeCx( uint sid, uint st, uint e ) {
  int Cx;

  Cx = 0;
  Cx = (Cx<<2) + (((sid)&3)); // 11
  Cx = (Cx<<2) + (((st)&3)); // 11
  Cx = Cx*16 + S0_e[__min(15,__max(0,e-(1-1)))];
  return uint(Cx);
}

