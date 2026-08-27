
#include "misc/intrin.inc"

#define EXTERN extern
#include "FSM.cpp"

#include "rc_config.inc"
#include "rc_io.inc"

// rc.inc is the scalar rangecoder of sh_v1xN_s.cpp, and it is included twice:
// once carryless (the encoder's fast path, which drops carry propagation and
// only flags a lost carry) and once with carry propagation (the twin the
// encoder falls back to for that block, and the decoder's coder -- the decode
// path is identical in both, RC_CARRYLESS only touches ShiftLow and rc_Quit).
// The class name comes in through a macro, so rc.inc itself stays the file it
// was moved from.

#define RC_CARRYLESS 1
#define Rangecoder Rangecoder_CL
#include "rc.inc"
#undef Rangecoder
#undef RC_CARRYLESS

#define RC_CARRYLESS 0
#define Rangecoder Rangecoder_CY
#include "rc.inc"
#undef Rangecoder
#undef RC_CARRYLESS

#include "rc_vec.inc"

#include "counter.inc"

#include "predict.inc"

template< int f_DEC > struct Model;

#include "model0.inc"

#include "model1.inc"
