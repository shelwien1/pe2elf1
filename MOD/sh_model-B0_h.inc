// -------------------------------------------------------------
// B0 -- constants of bmpc's pixel mix and residual mixer.
//
// GENERATED from IDX/sh_model-B0.idx by IDX/idx2inc.pl, then wrapped in
// #ifndef by tools/idxguard.py; regenerate with  GCIDX=1 ./gc.sh.
// Edit the .idx, not this file -- the comments live there.
//
// The guards let sweep.py pin a knob with -DB0_NW_w=1024; left alone each
// one is a live `mapping` object whose pattern carries the !MAP! marker, so
// IDX/opt.pl hill-climbs it straight in the binary.  mdesc lines are the
// frozen ones ('!' in the .idx) -- same value, no marker.
// -------------------------------------------------------------
#ifndef B0_M1_w
pdesc( B0_M1_w_, 0, "0000" );
static const int B0_M1_w = B0_M1_w_.value * (1);
#endif
#ifndef B0_M2_w
pdesc( B0_M2_w_, 0, "0000" );
static const int B0_M2_w = B0_M2_w_.value * (1);
#endif
#ifndef B0_NW_w
pdesc( B0_NW_w_, 1, "01111111111" );
static const int B0_NW_w = B0_NW_w_.value * (1);
#endif
#ifndef B0_INC_w
pdesc( B0_INC_w_, 1, "1111111100100110000" );
static const int B0_INC_w = B0_INC_w_.value * (1);
#endif
#ifndef B0_STEP_w
pdesc( B0_STEP_w_, 1, "0110011001100" );
static const int B0_STEP_w = B0_STEP_w_.value * (1);
#endif
#ifndef B0_WMAX_w
pdesc( B0_WMAX_w_, 1, "0111111111111" );
static const int B0_WMAX_w = B0_WMAX_w_.value * (1);
#endif
#ifndef B0_G2_w
pdesc( B0_G2_w_, 1, "01011111111100110" );
static const int B0_G2_w = B0_G2_w_.value * (1);
#endif
#ifndef B0_D_w
pdesc( B0_D_w_, 1, "0000000000000" );
static const int B0_D_w = B0_D_w_.value * (1);
#endif
#ifndef B0_R_w
pdesc( B0_R_w_, 1, "011110100001000111111" );
static const int B0_R_w = B0_R_w_.value * (1);
#endif
#ifndef B0_R0_w
pdesc( B0_R0_w_, 0, "0000" );
static const int B0_R0_w = B0_R0_w_.value * (1);
#endif
#ifndef B0_G1_w
mdesc( B0_G1_w_, 1, "0111111111111" );
static const int B0_G1_w = B0_G1_w_.value * (1);
#endif
#ifndef B0_HUB
pdesc( B0_HUB_, 1, "0001111" );
static const int B0_HUB = B0_HUB_.value * (1);
#endif
#ifndef B0_NW2_w
pdesc( B0_NW2_w_, 1, "0110110100111" );
static const int B0_NW2_w = B0_NW2_w_.value * (1);
#endif
#ifndef B0_INC2_w
pdesc( B0_INC2_w_, 1, "0101011001000000100" );
static const int B0_INC2_w = B0_INC2_w_.value * (1);
#endif
#ifndef B0_HUB2
pdesc( B0_HUB2_, 1, "100000" );
static const int B0_HUB2 = B0_HUB2_.value * (1);
#endif
#ifndef B0_NWL
pdesc( B0_NWL_, 1, "011111111111111" );
static const int B0_NWL = B0_NWL_.value * (1);
#endif
#ifndef B0_INCL
pdesc( B0_INCL_, 1, "000000000000010" );
static const int B0_INCL = B0_INCL_.value * (1);
#endif
#ifndef B0_STEPL
pdesc( B0_STEPL_, 1, "00100100101101111" );
static const int B0_STEPL = B0_STEPL_.value * (1);
#endif
#ifndef B0_MM2
pdesc( B0_MM2_, 0, "0000" );
static const int B0_MM2 = B0_MM2_.value * (1);
#endif
#ifndef B0_WLO
mdesc( B0_WLO_, -4096, "00000000000000" );
static const int B0_WLO = B0_WLO_.value * (1);
#endif
#ifndef B0_WHI
mdesc( B0_WHI_, 1, "0111111111111" );
static const int B0_WHI = B0_WHI_.value * (1);
#endif
#ifndef B0_W0
mdesc( B0_W0_, 1, "01111111111" );
static const int B0_W0 = B0_W0_.value * (1);
#endif
#ifndef B0_SQC
mdesc( B0_SQC_, 1, "01111111111111" );
static const int B0_SQC = B0_SQC_.value * (1);
#endif
#ifndef B0_HUBA
mdesc( B0_HUBA_, 1, "0111111111" );
static const int B0_HUBA = B0_HUBA_.value * (1);
#endif
#ifndef B0_MADR
mdesc( B0_MADR_, 1, "00001111" );
static const int B0_MADR = B0_MADR_.value * (1);
#endif
#ifndef B0_MAD0
mdesc( B0_MAD0_, 1, "00011111" );
static const int B0_MAD0 = B0_MAD0_.value * (1);
#endif
#ifndef B0_BT1
mdesc( B0_BT1_, 1, "0010" );
static const int B0_BT1 = B0_BT1_.value * (1);
#endif
#ifndef B0_BT2
mdesc( B0_BT2_, 1, "000110" );
static const int B0_BT2 = B0_BT2_.value * (1);
#endif
#ifndef B0_BT3
mdesc( B0_BT3_, 1, "0010100" );
static const int B0_BT3 = B0_BT3_.value * (1);
#endif
#ifndef B0_BR
mdesc( B0_BR_, 1, "00011111" );
static const int B0_BR = B0_BR_.value * (1);
#endif
#ifndef B0_NWB
pdesc( B0_NWB_, 1, "01111111111" );
static const int B0_NWB = B0_NWB_.value * (1);
#endif
#ifndef B0_INCB
pdesc( B0_INCB_, 1, "0001111111001110" );
static const int B0_INCB = B0_INCB_.value * (1);
#endif
#ifndef B0_STEPB
pdesc( B0_STEPB_, 1, "0110011001100" );
static const int B0_STEPB = B0_STEPB_.value * (1);
#endif
#ifndef B0_DB
pdesc( B0_DB_, 1, "1101111011100" );
static const int B0_DB = B0_DB_.value * (1);
#endif
#ifndef B0_BMW0
pdesc( B0_BMW0_, 1, "0100000000" );
static const int B0_BMW0 = B0_BMW0_.value * (1);
#endif
#ifndef B0_SSEW
mdesc( B0_SSEW_, 1, "01011111111" );
static const int B0_SSEW = B0_SSEW_.value * (1);
#endif
