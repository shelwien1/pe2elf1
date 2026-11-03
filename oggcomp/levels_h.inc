//  levels_h.inc -- the levels of this build, and the first half of each: its
//  families' generated headers (MOD/l<L>/tsvcomp-<family>_h.inc), which are
//  the level's parameters, mapping objects in the tuning build, and the
//  struct of each family's tables.  levels.inc is the other half: the index
//  builders, and the class (Lv9, Lv7, ...) the model takes for the level.
//
//  A level is a model: the six families' context indices and parameters as
//  IDX/l<L>/tsvcomp-<family>.idx declares them, generated into MOD/l<L>/
//  under names of the level's own (L9_dig_..., L7_dig_...), so that every
//  level is in one program.  They are the four models the tree had, by how
//  well they code:
//
//    -9  IDX/l9  the strongest, the one the tree had built in (model 0)
//    -7  IDX/l7  nearly as strong, and the fastest and smallest of the
//                four (it was oggcomp2.dll)
//    -3  IDX/l3  (oggcomp3.dll)
//    -1  IDX/l1  (oggcomp1.dll)
//
//  -3 and -1 kept their numbers, and the header byte that names a level is
//  the byte that named a model, so a stream made with any of the four as it
//  was decodes at its level (oggcomp.cpp, oc_level_of).
//
//  A build with -DOC_ONLY=L has level -L alone: ./mk.sh tuning L builds that
//  way, so that IDX/opt.pl finds that level's knobs and no others, and so
//  that the tuning build compiles in the time one level takes.
#ifndef OC_ONLY
#define OC_LV(n) 1
#else
#if OC_ONLY != 9 && OC_ONLY != 7 && OC_ONLY != 3 && OC_ONLY != 1
#error "OC_ONLY is a level: 9, 7, 3 or 1"
#endif
#define OC_LV(n) ((n) == OC_ONLY)
#endif

//  The rows per context of each family's tables, which the templates size
//  them by (%M%ND, %M%MND): a node per decision of the value coder's tree,
//  but for the signs, which are one decision each.
#define OC_LV_NODES(n)                                                \
  constexpr int L##n##_dig_ND = TC_NODE, L##n##_dig_MND = TC_MNODE; \
  constexpr int L##n##_sgn_ND = 1, L##n##_sgn_MND = 1;               \
  constexpr int L##n##_flr_ND = TC_NODE, L##n##_flr_MND = TC_MNODE; \
  constexpr int L##n##_cls_ND = TC_NODE, L##n##_cls_MND = TC_MNODE; \
  constexpr int L##n##_aux_ND = TC_NODE, L##n##_aux_MND = TC_MNODE; \
  constexpr int L##n##_hdr_ND = TC_NODE, L##n##_hdr_MND = TC_MNODE;

#if OC_LV(9)
OC_LV_NODES(9)
#include "MOD/l9/tsvcomp-aux_h.inc"
#include "MOD/l9/tsvcomp-cls_h.inc"
#include "MOD/l9/tsvcomp-dig_h.inc"
#include "MOD/l9/tsvcomp-flr_h.inc"
#include "MOD/l9/tsvcomp-hdr_h.inc"
#include "MOD/l9/tsvcomp-sgn_h.inc"
#endif
#if OC_LV(7)
OC_LV_NODES(7)
#include "MOD/l7/tsvcomp-aux_h.inc"
#include "MOD/l7/tsvcomp-cls_h.inc"
#include "MOD/l7/tsvcomp-dig_h.inc"
#include "MOD/l7/tsvcomp-flr_h.inc"
#include "MOD/l7/tsvcomp-hdr_h.inc"
#include "MOD/l7/tsvcomp-sgn_h.inc"
#endif
#if OC_LV(3)
OC_LV_NODES(3)
#include "MOD/l3/tsvcomp-aux_h.inc"
#include "MOD/l3/tsvcomp-cls_h.inc"
#include "MOD/l3/tsvcomp-dig_h.inc"
#include "MOD/l3/tsvcomp-flr_h.inc"
#include "MOD/l3/tsvcomp-hdr_h.inc"
#include "MOD/l3/tsvcomp-sgn_h.inc"
#endif
#if OC_LV(1)
OC_LV_NODES(1)
#include "MOD/l1/tsvcomp-aux_h.inc"
#include "MOD/l1/tsvcomp-cls_h.inc"
#include "MOD/l1/tsvcomp-dig_h.inc"
#include "MOD/l1/tsvcomp-flr_h.inc"
#include "MOD/l1/tsvcomp-hdr_h.inc"
#include "MOD/l1/tsvcomp-sgn_h.inc"
#endif
#undef OC_LV_NODES
