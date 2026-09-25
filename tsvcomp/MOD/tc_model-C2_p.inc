// Generated into MOD/tc_model-C2_p.inc: the C2 knobs and context masks
// (MakeTables), the cell-context builder (MakeIndex).  tsvcomp's C2 cells:
// the prediction columns (tsvcomp.cpp).



static const int C2_mwMax = (32760+1) * (1);
static const int C2_G3_m = (262144+0) * (1);
static const int C2_P0 = (19588+1) * (1);
static const int C2_P1 = (5633+0) * (1);
static const int C2_wr = (8259+1) * (1);
static const int C2_wr1 = (2593+0) * (1);
static const int C2_wr1b = (12241+1) * (1);
static const int C2_mw = (2212+1) * (1);
static const int C2_K = (21632+0) * (1);
static const int C2_NWk = (164520+1) * (1);
static const int C2_RKinc = (8475+1) * (1);
static const int C2_kStep = (111156+1) * (1);
static const int C2_kMin = (4189696+1) * (1);
static const int C2_kMax = (6920+1) * (1);
static const int C2_NWm = (346138+1) * (1);
static const int C2_MWinc = (31892+1) * (1);
static const int C2_mwStep = (260096+1) * (1);
static const int C2_mwMin = (0+1) * (1);
static const int C2_M1_m = (261888+1) * (1);
static const int C2_M2_m = (6072+1) * (1);
static const int C2_M1_k = (1021633+1) * (1);
static const int C2_M2_k = (193536+1) * (1);
static const int C2_G2_m = (6671+0) * (1);
static const int C2_G2_k = (16579+0) * (1);
static const int C2_stP_min = (6200+1) * (1);
static const int C2_leak1 = (32272+1) * (1);
static const int C2_leak2 = (63235+1) * (1);
static const int C2_mwP0 = (20419+1) * (1);
static const int C2_G3_k = (74503+0) * (1);
static const int C2_G4_m = (131490+0) * (1);
static const int C2_G4_k = (128+0) * (1);
static const int C2_M1_u = (261120+0) * (1);
static const int C2_M2_u = (2027+0) * (1);
static const int C2_M2_x = (1718+0) * (1);
static const int C2_NWu = (395520+0) * (1);
static const int C2_RUinc = (256546+0) * (1);
static const int C2_uStep = (493048+0) * (1);
static const int C2_uMin = (222+0) * (1);
static const int C2_uMax = (3162240+0) * (1);
static const int C2_vMin = (972+0) * (1);
static const int C2_vMax = (3328032+0) * (1);
static const int C2_G2_u = (0+0) * (1);
static const int C2_G3_u = (262144+0) * (1);
static const int C2_G4_u = (107+0) * (1);
static const int C2_R0_u = (156+0) * (1);
static const int C2_M1_v = (261728+0) * (1);
static const int C2_M2_v = (296+0) * (1);
static const int C2_NWv = (437536+0) * (1);
static const int C2_RVinc = (327680+0) * (1);
static const int C2_vStep = (40576+0) * (1);
static const int C2_G2_v = (2+0) * (1);
static const int C2_G3_v = (262144+0) * (1);
static const int C2_G4_v = (129+0) * (1);
static const int C2_R0_v = (0+0) * (1);
static const int C2_UVH = (1088+0) * (1);
static const int C2_CXW = (1015+0) * (1);
static const int C2_XHW = (920+0) * (1);
static const int C2_UVDET = (184+0) * (1);
static const int C2_XHC = (0+0) * (1);
static const int C2_mwXlo = (0+0) * (1);
static const int C2_mwXhi = (16384+0) * (1);
static const int C2_AGAu = (0+0) * (1);
static const int C2_AGAv = (1024+0) * (1);
static const int C2_AGAm = (126+0) * (1);
static const int C2_AGAk = (69+0) * (1);
static const int C2_AGB = (276+0) * (1);
static const int C2_AGBuv = (332+0) * (1);
static const int C2_E2E = (256+0) * (1);
static const int C2_E2Euv = (202+0) * (1);



static uint C2_MakeCx( uint h ) {
  int Cx;

  Cx = 0;
  Cx = (Cx<<19) + (((h)&524287)); // 1111111111111111111
  return uint(Cx);
}

