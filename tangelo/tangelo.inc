// The Tangelo model (a paq/lpaq-class bit predictor), split back into the parts
// its own "#include" markers named. legacy/tangelo_orig.cpp is the single-file
// original this came from.
//
// The only changes from that original are the ones the two-translation-unit,
// journaled build needs; each is commented at the point it was made:
//   common.inc      mutable globals declared, not defined; locale-free ctype
//   hash.inc/stsq   free functions made static so both TUs can hold a copy
//   CM.inc          memset() spelled out so its stores can be journaled
//   mixer.inc       "using Base::" for the dependent base members (GCC/Clang)
//   CM_small.inc    template parameter renamed so it stops shadowing "m"
//
// The model predicts one bit at a time: Model::predictNext() folds the bit just
// coded (the global y) into the state and returns P(next bit = 1) as a 12-bit
// probability. ../coder_tangelo.inc adapts that to the walk's step interface.

#include "common.inc"
#include "window.inc"
#include "random.inc"
#include "ilog.inc"
#include "idiv.inc"
#include "hash.inc"
#include "table_fsm.inc"
#include "stsq.inc"
#include "mixer_train.inc"
#include "mixer1.inc"
#include "mixer.inc"
#include "APM.inc"
#include "counter.inc"
#include "CM_small.inc"
#include "CM.inc"
#include "CM_match.inc"
#include "CM_main.inc"
