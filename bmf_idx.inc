// bmf_idx.inc -- the IDX bridge: everything the generated MOD/ headers need,
// then the headers themselves.
//
// Included from bmf.cpp; not a standalone translation unit.
//
// Every number the codec can be tuned on lives in IDX/*.idx and reaches the
// sources through here.  IDX-FORMAT.md is the format; the short version is that
// each parameter is declared once and the build decides what it becomes:
//
//   ./mk.sh           Const 0 -- a live `mapping` object whose pattern string
//                     carries a "!MAP!" marker, so IDX/opt.pl can find it in
//                     the executable and hill-climb it without rebuilding.
//   ./mk.sh release   Const 1 -- a literal the compiler folds, no indirection.
//
// Both builds must produce the same coded stream.  That is the contract, and
// t.sh plus the corpus manifest are the test.
//
// This file is the "before" half of the include-order split IDX-FORMAT.md
// sec.12 warns about: the generated headers need idx_f and the mapping runtime,
// and everything that consumes their values -- bmf_tables.inc and the models --
// has to come after them.  Merging the two halves is the obvious-looking
// simplification that cannot work.

// An IDX integer read back as the float it encodes: v * 2^-k.
//
// The port keeps every float32 the sources used to spell as a literal, so the
// integer in the .idx is that float's exact mantissa and k its binary exponent
// (IDX/f2idx.py computes the pair).  The result is exact rather than merely
// close: |v| < 2^24 makes float(v) exact, 2^k is a power of two so the division
// neither rounds nor loses a bit, and what comes back is the same float32 the
// literal named.  A round fixed-point scale -- v/32768, the usual IDX
// convention -- would have moved every NLMS rate and every seed weight by a
// fraction of a per cent and quietly changed the codec.
//
// It is also why the patterns are wide: a 24-bit mantissa is 24 bits for the
// optimizer to move.  Freeze what has little leverage (IDX-FORMAT.md sec.10)
// rather than narrowing the declaration.
static inline constexpr float idx_f(int64_t v, int32_t k) {
  return (float)((double)v/(double)((uint64_t)1<<k));
}

// The same, for a value the consumer wants at double precision.
static inline constexpr double idx_d(int64_t v, int32_t k) {
  return (double)v/(double)((uint64_t)1<<k);
}

// A parameter used as a divisor, a shift or a size is a crash when the
// optimizer sweeps it to zero, and sweeping it is the point.  The clamp belongs
// at the point of use, not in the .idx: the pattern is the search space, the
// clamp is the contract (IDX-FORMAT.md sec.5).
static inline constexpr int32_t idx_clamp(int32_t v, int32_t lo, int32_t hi) {
  return v<lo ? lo : v>hi ? hi : v;
}

#include "sh_mapping.inc"

#include "MOD/bmf-P2_h.inc"
#include "MOD/bmf-P1_h.inc"
#include "MOD/bmf-CN_h.inc"
#include "MOD/bmf-CM_h.inc"
#include "MOD/bmf-AP_h.inc"
#include "MOD/bmf-PA_h.inc"
#include "MOD/bmf-GL_h.inc"
#include "MOD/bmf-MB_h.inc"
#include "MOD/bmf-CD_h.inc"
#include "MOD/bmf-IX_h.inc"

// A parameter that has to be a template argument.
//
// In the shipping build an IDX value is a literal and can be a non-type
// template argument like any other integer.  In the tuning build it is a read
// from a patchable object -- not a constant expression, and therefore not a
// template argument at all.  A non-type template parameter of *reference* type
// takes both: `const int32_t&` binds an object with static storage duration
// however that object was initialized, so the same call site compiles under
// either build and only the shipping one folds.  That is IDX-FORMAT.md sec.8,
// and it is what lets the context indices' bit positions be parameters at all.
//
// USE_NEW comes from the generated headers above, so this has to follow them.
#if USE_NEW
 #define IDXP(name) const int32_t& name
#else
 #define IDXP(name) int32_t name
#endif

// Instantiating on an object with internal linkage gives the instantiated type
// internal linkage too, which GCC warns about under -Wsubobject-linkage.  The
// warning is right in general -- separate translation units would each get a
// different type under one name -- and vacuous here, because bmf is a single
// translation unit.  clang has no such warning.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wsubobject-linkage"
#endif
