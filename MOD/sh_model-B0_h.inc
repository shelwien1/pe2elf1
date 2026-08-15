// -------------------------------------------------------------
// B0 -- constants of bmpc's LPC mixer, in sh_model-C0_h.inc style.
// Source of record: IDX/sh_model-B0.idx.
//
// Every knob is wrapped in #ifndef so a sweep can pin it from the
// command line (-DB0_NW_w=900) without touching the file; left alone
// it is a live `mapping` object whose pattern carries the !MAP! marker,
// so IDX/auto_opt2.pl can hill-climb it in the binary.
// -------------------------------------------------------------

// ---- Config_LW: one updater per linear-mix weight ----

// momentum_D  = B0_M1_w/1024        gradient EMA
#ifndef B0_M1_w
pdesc( B0_M1_w_, 1, "01110011001" );
static const int B0_M1_w = B0_M1_w_.value * (1);
#endif
// momentum_R  = B0_M2_w/65536       curvature EMA
#ifndef B0_M2_w
pdesc( B0_M2_w_, 1, "01111110101110000" );
static const int B0_M2_w = B0_M2_w_.value * (1);
#endif
// NW          = B0_NW_w/4096        newton step gain
#ifndef B0_NW_w
pdesc( B0_NW_w_, 1, "0111111111111" );
static const int B0_NW_w = B0_NW_w_.value * (1);
#endif
// inc         = B0_INC_w/(1<<20)    denominator floor
#ifndef B0_INC_w
pdesc( B0_INC_w_, 1, "01111111111" );
static const int B0_INC_w = B0_INC_w_.value * (1);
#endif
// stepMax     = B0_STEP_w/65536     trust region
#ifndef B0_STEP_w
pdesc( B0_STEP_w_, 1, "0110011001100" );
static const int B0_STEP_w = B0_STEP_w_.value * (1);
#endif
// maxVal      = B0_WMAX_w/1024      |w| box (minVal = -maxVal)
#ifndef B0_WMAX_w
pdesc( B0_WMAX_w_, 1, "0111111111111" );
static const int B0_WMAX_w = B0_WMAX_w_.value * (1);
#endif
// grad1_clip  = B0_G1_w/1024        (ParamUpdater does not read it)
#ifndef B0_G1_w
pdesc( B0_G1_w_, 1, "0111111111111" );
static const int B0_G1_w = B0_G1_w_.value * (1);
#endif
// grad2_clip  = B0_G2_w/1024        per-input curvature clip
#ifndef B0_G2_w
pdesc( B0_G2_w_, 1, "01111111111111111" );
static const int B0_G2_w = B0_G2_w_.value * (1);
#endif
// D_clip      = B0_D_w/64
#ifndef B0_D_w
pdesc( B0_D_w_, 1, "0111111111111" );
static const int B0_D_w = B0_D_w_.value * (1);
#endif
// R_clip      = B0_R_w
#ifndef B0_R_w
pdesc( B0_R_w_, 1, "011110100001000111111" );
static const int B0_R_w = B0_R_w_.value * (1);
#endif
// R0          = B0_R0_w/(1<<20)     seed curvature
#ifndef B0_R0_w
pdesc( B0_R0_w_, 0, "0000" );
static const int B0_R0_w = B0_R0_w_.value * (1);
#endif

// ---- mixer shape ----

// WLO/WHI: the box the logistic weight parameterisation maps onto,
// w = WLO + (WHI-WLO)*sigma(z).  Only read when W_LOGIT=1.
#ifndef B0_WLO
pdesc( B0_WLO_, -4096, "00000000000000" );   // -4.0 * 1024
static const int B0_WLO = B0_WLO_.value * (1);
#endif
#ifndef B0_WHI
pdesc( B0_WHI_, 1, "0111111111111" );        //  4.0 * 1024
static const int B0_WHI = B0_WHI_.value * (1);
#endif
// W0: seed weight of the "same component, pixel to the left" input,
// scaled by 1024.  Every other weight seeds at 0.
#ifndef B0_W0
pdesc( B0_W0_, 1, "01111111111" );           //  1.0 * 1024
static const int B0_W0 = B0_W0_.value * (1);
#endif
// SQC: stretch() clamp for the logistic mix, scaled by 1024; the byte
// value v enters as stretch((v+0.5)/256) clipped to +-SQC/1024.
#ifndef B0_SQC
pdesc( B0_SQC_, 1, "01111111111111" );       //  8.0 * 1024
static const int B0_SQC = B0_SQC_.value * (1);
#endif
