// Generated into MOD/tc_model-C0_p.inc: the C0 knobs and context masks
// (MakeTables), the cell-context builder (MakeIndex).  tsvcomp's C0 cells:
// the coded column's own history (tsvcomp.cpp).



static const int C0_mwMax = (32760+1) * (1);
static const int C0_G3_m = (262144+0) * (1);
static const int C0_P0 = (8192+1) * (1);
static const int C0_P1 = (3657+0) * (1);
static const int C0_wr = (8259+1) * (1);
static const int C0_wr1 = (3072+0) * (1);
static const int C0_wr1b = (12241+1) * (1);
static const int C0_mw = (1792+1) * (1);
static const int C0_K = (28162+0) * (1);
static const int C0_NWk = (206080+1) * (1);
static const int C0_RKinc = (1+1) * (1);
static const int C0_kStep = (77892+1) * (1);
static const int C0_kMin = (3145728+1) * (1);
static const int C0_kMax = (7009+1) * (1);
static const int C0_NWm = (346138+1) * (1);
static const int C0_MWinc = (1600+1) * (1);
static const int C0_mwStep = (228096+1) * (1);
static const int C0_mwMin = (0+1) * (1);
static const int C0_M1_m = (261888+1) * (1);
static const int C0_M2_m = (1568+1) * (1);
static const int C0_M1_k = (506898+1) * (1);
static const int C0_M2_k = (7104+1) * (1);
static const int C0_G2_m = (6671+0) * (1);
static const int C0_G2_k = (16579+0) * (1);
static const int C0_stP_min = (32640+1) * (1);
static const int C0_leak1 = (32488+1) * (1);
static const int C0_leak2 = (63235+1) * (1);
static const int C0_mwP0 = (19137+1) * (1);
static const int C0_G3_k = (74503+0) * (1);
static const int C0_G4_m = (131490+0) * (1);
static const int C0_G4_k = (72203+0) * (1);
static const int C0_M1_u = (261120+0) * (1);
static const int C0_M2_u = (1366+0) * (1);
static const int C0_M2_x = (1602+0) * (1);
static const int C0_NWu = (459008+0) * (1);
static const int C0_RUinc = (78033+0) * (1);
static const int C0_uStep = (271360+0) * (1);
static const int C0_uMin = (304+0) * (1);
static const int C0_uMax = (2650112+0) * (1);
static const int C0_vMin = (909+0) * (1);
static const int C0_vMax = (3328032+0) * (1);
static const int C0_G2_u = (0+0) * (1);
static const int C0_G3_u = (262144+0) * (1);
static const int C0_G4_u = (107+0) * (1);
static const int C0_R0_u = (184+0) * (1);
static const int C0_M1_v = (262080+0) * (1);
static const int C0_M2_v = (15+0) * (1);
static const int C0_NWv = (331785+0) * (1);
static const int C0_RVinc = (66603+0) * (1);
static const int C0_vStep = (40576+0) * (1);
static const int C0_G2_v = (2+0) * (1);
static const int C0_G3_v = (262144+0) * (1);
static const int C0_G4_v = (289+0) * (1);
static const int C0_R0_v = (0+0) * (1);
static const int C0_UVH = (2558+0) * (1);
static const int C0_CXW = (1252+0) * (1);
static const int C0_XHW = (920+0) * (1);
static const int C0_UVDET = (160+0) * (1);
static const int C0_XHC = (0+0) * (1);
static const int C0_mwXlo = (164+0) * (1);
static const int C0_mwXhi = (16384+0) * (1);
static const int C0_AGAu = (200+0) * (1);
static const int C0_AGAv = (3714+0) * (1);
static const int C0_AGAm = (1836+0) * (1);
static const int C0_AGAk = (1632+0) * (1);
static const int C0_AGB = (57+0) * (1);
static const int C0_AGBuv = (496+0) * (1);
static const int C0_E2E = (256+0) * (1);
static const int C0_E2Euv = (202+0) * (1);



static uint C0_MakeCx( uint h, uint zrun, uint sch ) {
  int Cx;

  Cx = 0;
  Cx = (Cx<<14) + (((h)&16383)); // 11111111111111
  Cx = Cx*4 + ((zrun>0+(1-1))+(zrun>1+(1-1))+(zrun>2+(1-1)));
  Cx = (Cx<<3) + (((sch)&7)); // 111
  return uint(Cx);
}

