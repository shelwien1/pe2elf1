// mix_layout.inc -- the mixer's input vector, as compile-time constants.
//
// The layout was always static.  Every m.add() in the model runs on every bit,
// in the same order, and the one thing that varies the count -- cxtfl, which
// drops one input per context -- is a property of WHICH MAP is mixing, not of
// the data.  So the 466 slots are the same 466 slots on every bit of every
// file, and nothing said so: the vector was rebuilt by `tx[nx++] = x` each
// time, and every consumer that needed to address it did so by counting.
//
// That cost more than the increment.  The `mul` rescale block in
// ContextModel::p addresses four of the core map's contexts POSITIONALLY, and
// had to be repaired when that map went from 7 contexts to 9; nothing could
// assert the total; and IDX-FORMAT-adjacent §10.5 of the model analysis notes
// that a feature-count change in mix2 walks off tx[] with no check anywhere.
//
// SHAPE
//
// One enum per submodel, each starting at 0 and naming the START of every
// block it contributes.  A block one slot wide needs no initialiser -- the
// enum's auto-increment is exactly right for it, which is why the ten
// SmallStationaryContextMaps below are a bare list and the trailing `_N`
// falls out of the same mechanism.  A block N slots wide spells `= prev + N`.
// The final `_N` of each enum is therefore that submodel's input count, by
// construction rather than by arithmetic somebody has to redo.
//
// A second enum lays the submodels end to end, each base being the previous
// base plus the previous submodel's `_N`, and MIX_INPUTS falls out of the last
// one.  That is the number Mixer is instantiated on.
//
// The context counts are declared HERE and the maps are declared FROM them, so
// there is one source of truth: change X_CM_CTX and the map, the layout, the
// mixer width and the `mul` block's stride all move together.  Getting that
// backwards -- counts on the maps, layout copied alongside -- is what made the
// 7->9 change a two-place edit with only one of them checked.

//---------------------------------------------------------------- widths

// What one ContextMap context contributes: the run input, plus mix2's five
// terms, or its four when cxtfl == 0 (the (p1-p0) term is dropped and st/4
// becomes st*9/32).  RunContextMap and SmallStationaryContextMap give one each.
enum {
  MIXIN_CM  = 6,   // cxtfl != 0
  MIXIN_CM0 = 5,   // cxtfl == 0
  MIXIN_RCM = 1,
  MIXIN_SCM = 1
};

//---------------------------------------------------------------- context counts

enum {
  X_CM_CTX = 9,    // ContextModel::cm   orders 3,4,5,6,8,13,0,1,2
  W_CM_CTX = 46,   // WordModel::cm
  S_CN_CTX = 5,    // SparseModel::cn
  S_SCM_N  = 10,   // SparseModel's scm1..scma
  R_CO_CTX = 4,    // RecordModel::co   } cxtfl == 3
  R_CP_CTX = 3,    //             ::cp  }
  R_CM_CTX = 2,    //             ::cm  }
  R_CN_CTX = 5,    //             ::cn  } cxtfl == 0
  R_CQ_CTX = 3     //             ::cq  }
};

//---------------------------------------------------------------- per submodel

// ContextModel's own inputs, in the order p() adds them: the constant bias,
// the three RunContextMaps, then the core map.
enum {
  X_bias,                                   // auto: 0
  X_rcm7,                                   // auto: 1
  X_rcm9,                                   // auto: 2
  X_rcm10,                                  // auto: 3
  X_cm,                                     // auto: 4
  X_N = X_cm + X_CM_CTX*MIXIN_CM            // 58
};

// WordModel: one map, 46 contexts.
enum {
  W_cm,                                     // auto: 0
  W_N = W_cm + W_CM_CTX*MIXIN_CM            // 276
};

// SparseModel: the hashed map, then ten direct-indexed tables one slot each --
// the case the auto-increment was made for, including the total.
enum {
  S_cn,                                     // auto: 0
  S_scm1 = S_cn + S_CN_CTX*MIXIN_CM,        // 30
  S_scm2, S_scm3, S_scm4, S_scm5,           // auto: 31..34
  S_scm6, S_scm7, S_scm8, S_scm9, S_scma,   // auto: 35..39
  S_N                                       // auto: 40
};

// RecordModel: co and cp mix at cxtfl 3, then cm/cn/cq at cxtfl 0.  The order
// is the order mix() calls them in, which is not the order they are declared
// in -- the layout follows the calls, because that is what fills the vector.
enum {
  R_co,                                     // auto: 0
  R_cp  = R_co  + R_CO_CTX*MIXIN_CM,        // 24
  R_cm0 = R_cp  + R_CP_CTX*MIXIN_CM,        // 42
  R_cn0 = R_cm0 + R_CM_CTX*MIXIN_CM0,       // 52
  R_cq0 = R_cn0 + R_CN_CTX*MIXIN_CM0,       // 77
  R_N   = R_cq0 + R_CQ_CTX*MIXIN_CM0        // 92
};

//---------------------------------------------------------------- bases

// The submodels laid end to end, in the order ContextModel::p runs them.
// MIX_INPUTS is what MainMixer is instantiated on; the Mixer template rounds it
// up to a multiple of 16 for the aligned SIMD loads, and the pad slots stay
// zero -- a zero input contributes zero to every dot product and trains a zero
// gradient, so they are inert rather than merely unused.
enum {
  MIXB_X,                                   // auto: 0   ContextModel
  MIXB_W = MIXB_X + X_N,                    // 58        WordModel
  MIXB_S = MIXB_W + W_N,                    // 334       SparseModel
  MIXB_R = MIXB_S + S_N,                    // 374       RecordModel
  MIX_INPUTS = MIXB_R + R_N                 // 466
};

// The property the whole file exists to make checkable.  It is not a
// restatement of the arithmetic above -- it is the claim that the four
// submodels tile the vector with no gap and no overlap, which is exactly what
// nothing could assert before.
static_assert( MIXB_W == X_N
            && MIXB_S == X_N + W_N
            && MIXB_R == X_N + W_N + S_N
            && MIX_INPUTS == X_N + W_N + S_N + R_N,
  "mix_layout.inc: the submodel bases do not tile the mixer input vector" );
