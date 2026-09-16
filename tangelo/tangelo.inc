// The Tangelo model (a paq/lpaq-class bit predictor), split back into the parts
// its own "#include" markers named. legacy/tangelo_orig.cpp is the single-file
// original this came from.
//
// The only changes from that original are the ones this build needs. Each is
// commented where it was made; the complete list, taken from a mechanical diff
// against legacy/tangelo_orig.cpp:
//
//   common.inc      the program's own preamble (includes, typedefs, flen, the
//                   INLINE macros) moved to ../tangelo_w.cpp; the probability
//                   scale named; MEM made overridable; y and bpos declared
//                   rather than defined; min/max made static; the <ctype.h>
//                   calls replaced by open-coded C-locale tests
//   random, ilog,   rnd, ilog, pt and stretch declared rather than defined, so
//   idiv, stsq      that both copies of the model step share one of each
//   hash, stsq      hash() and squash() made static, being pure
//   CM.inc          the memset() in the eviction path spelled out, so that its
//                   writes reach the journal
//   CM_main.inc     the byte classifier calls those C-locale tests
//   CM, CM_match,   INLINE on E::get, ContextMap::mix, MatchModel::matchModel,
//   CM_main, mixer  Mixer::p and Model::predictNext. Load-bearing rather than an
//                   optimisation - see ALGORITHM.md section 5.4
//   mixer.inc       "using Base::" for the dependent base members, which GCC and
//                   Clang require and MSVC does not
//   CM_small.inc    a template parameter renamed so it stops shadowing "m"
//   all of them      every write to model state wrapped in W(...), which marks
//                   it for tangelo_s and is "#define W(x) (x)" for tangelo_w -
//                   see ../write.inc and ALGORITHM.md section 5.6. mixer1.inc
//                   also declares the mixer's arrays with W_block(), which is
//                   the one thing the source can say and the assembly cannot
//
// window.inc, table_fsm.inc and mixer_train.inc are byte for byte the original;
// the rest differ only by the write markers and the entries above. The last two
// of those are what the original needs merely to compile with GCC or Clang.
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
