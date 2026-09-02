
#include "misc/intrin.inc"

#define EXTERN extern
#include "FSM.cpp"

#include "rc_config.inc"

// Ahead of rc_io.inc, which needs Counter for the header model -- see RCio.
#include "counter.inc"

#include "rc_io.inc"

// rc.inc is the scalar rangecoder of sh_v1xN_s.cpp, instantiated once here
// with carry propagation: the twin the vector encoder falls back to for a
// block where a lane lost a carry. Its carryless form is still built -- that
// is what mk_kernel.sh generates the vector kernel from -- but only as the
// generator's input, never as a class the compile instantiates. The class name
// comes in through a macro, so rc.inc itself stays the file it was moved from.

#define RC_CARRYLESS 0
#define Rangecoder Rangecoder_CY
#include "rc.inc"
#undef Rangecoder
#undef RC_CARRYLESS

// ...and a second time for the block header's own coder, which is the same
// scalar carry-propagating coder over a different cursor: forwards, straight
// to and from the coroutine, with a fixed-length flush. See RC_HDR_IO.
#define RC_CARRYLESS 0
#define RC_IO_BASE   RC_HDR_IO
#define Rangecoder   Rangecoder_HDR
#include "rc.inc"
#undef Rangecoder
#undef RC_IO_BASE
#undef RC_CARRYLESS

#include "rc_vec.inc"



//  Everything the generated kernel does is in rc.inc, the staging and the
//  scatter that commits it included: mk_kernel.sh preprocesses rc.inc with
//  the compile's own -march, so __AVX512F__ and friends are known there, and
//  RC_KERNEL_CONF fingerprints what they resolved to.

template< int f_DEC > struct Model;

#include "model0.inc"

#include "model1.inc"
