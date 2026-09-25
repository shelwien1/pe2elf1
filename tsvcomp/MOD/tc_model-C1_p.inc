// Generated into MOD/tc_model-C1_p.inc: the C1 knobs and context masks
// (MakeTables), the cell-context builder (MakeIndex).  tsvcomp's C1 cells:
// jump sizes and the other column (tsvcomp.cpp).



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



static uint C1_MakeCx( uint h ) {
  int Cx;

  Cx = 0;
  Cx = (Cx<<19) + (((h)&524287)); // 1111111111111111111
  return uint(Cx);
}

