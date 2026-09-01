// tf_all.inc - the transformer implementation, for coder0's single-translation-
// unit build.  Every system header these need is included at the top of
// coder0.cpp; this file only pulls in the pieces themselves, in dependency
// order, and only the ones the configured engine actually uses.
//
// Two anonymous-namespace names had to be renamed to survive being in one
// translation unit with their neighbours (see tf/PORTING.md): weights_io_
// compressed's die/dtype_size/Reader and arena_build's die.

#include "weights_io.h"
#include "weights_io.inc"
#include "weights_io_compressed.inc"
#include "weights_init.inc"

#if TF_FP32
#include "fp32_model.h"
#include "fp32_model.inc"
#else
#include "model_opt.h"
#include "qmat_dense.inc"
#include "qmat_sparse.inc"
#include "attn.inc"
#include "kda.inc"
#include "glue.inc"
#include "arena_build.inc"
#include "model_opt.inc"
#endif
