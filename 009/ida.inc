// ida.inc -- what is left of the Hex-Rays vocabulary.
//
// The decompiler writes a byte or a word inside a wider value as a macro over
// a pointer cast -- `BYTE4(x)` for `*((uint8_t *)&x + 4)`, `LOWORD(x) = y` for
// a sixteen-bit store into the low half of a register.  Every one of those is
// gone from this tree now, and what replaced each is at the site: an absolute
// value written as `abs32`, a signed comparison written as a comparison, and a
// partial store written as the assignment it amounts to -- in every case the
// half it preserved turned out to be dead, which the types say and the streams
// agree with.  The macros were how MSVC's register allocation looked from the
// outside; none of them was a decision the program made.
//
// `__OFSUB__` went the same way.  It is the x86 overflow flag after a compare,
// and Hex-Rays emits it wherever the original tested a signed inequality --
// signed less-than *is* `SF != OF`, so `(x-y<0) ^ __OFSUB__(x,y)` is `x < y`
// and nothing more.  Its one site said `e_top > deadzone_hi`, which is what
// the five sibling lines around it already said.
//
// One is kept, because it says something the code means rather than something
// the compiler did.  `__PAIR64__` was the other, and its one remaining site was
// a register split after all -- two 32-bit stores MSVC had both halves for.

// Four bytes of a recovered struct that exist only at the wider pointer width:
// a member that is a pointer in one build and four bytes of padding in the
// other.  The struct's layout is the program's, and this is how it stays the
// same size at both.
#define BMF_SPILL_PAD(n) uint8_t _spill_pad ## n[sizeof(void*)-4]

// The two helpers the decompiler calls by name.  `abs32` is a function and not
// a macro, and it is what the branchless absolute values above became: the
// original computed `(m ^ x) - m` with `m` the sign broadcast into a byte or a
// word, which is `|x|` in as many bits as the destination keeps.
//
// `0u-(uint32_t)x` rather than `(uint32_t)-x`: the second negates before it
// converts, and negating `INT32_MIN` is signed overflow.  No residual in this
// program reaches it -- they are pixel differences -- but the two spellings
// have the same codegen, so the total one costs nothing and is one less thing
// UBSan has to be told to ignore.
inline uint32_t abs32(int32_t x) {
  return x<0 ? 0u-(uint32_t)x : (uint32_t)x;
}

// An invariant the source depends on, checked in the `BMF_ASSERTS=1` build and
// absent from the shipping one.
//
// Every use of this stands where a comment would otherwise have to say "and
// this cannot happen because" -- `p2_pred`'s shift range, `p2_bump`'s.  The
// reasoning is written out at each; what this adds is that the reasoning can be
// run.  Not `assert` directly, because the default build passes `-DNDEBUG` and
// a reader would have to know that to know these are off.
#ifdef BMF_ASSERTS
#  include <cassert>
#  define BMF_ASSUME(x) assert(x)
#else
#  define BMF_ASSUME(x) ((void)0)
#endif

// Named-width min/max/clamp, and the width in the name is the point.
//
// Not `std::min` / `std::clamp`: the call sites mix `int32_t`, `uint32_t` and
// `uint16_t` operands, and template deduction would pick one and hide the
// conversion -- in a program where a conversion is how the fold tables work,
// that is exactly the wrong thing to make invisible.  Spelling the width means
// every narrowing at a call site is still written down.  No `<algorithm>`
// either, which keeps this header free-standing.
//
// At `-O2` these are the same `cmov` the two-line `if` forms compiled to; the
// gate says so, since none of the eleven sites they replace moved a stream.
static inline int32_t min32(int32_t a, int32_t b) {
  return a<b ? a : b;
}

static inline int32_t max32(int32_t a, int32_t b) {
  return a>b ? a : b;
}

static inline uint32_t umin32(uint32_t a, uint32_t b) {
  return a<b ? a : b;
}

static inline int32_t clamp32(int32_t v, int32_t lo, int32_t hi) {
  return v<lo ? lo : v>hi ? hi : v;
}

// The reference weight's range, and the one clamp that is written five times.
//
// `>= 191 -> 191` is `min(w, 191)` exactly -- the boundary case assigns the
// value it already holds -- so fusing the two `if`s loses nothing.  Both bounds
// are model parameters: a weight is a 128th, so 191 is 1.49x and -64 is -0.5x.
constexpr int32_t kWeightMin = -64, kWeightMax = 191;

// The ceiling on `choose_plane_coding`'s slack -- how much worse than the best
// a candidate may cost and still be tried.  0x4000 occurs in at least five
// unrelated roles in this program and this is one of them; it is *not*
// `BitCtr::kMaxLimit`, nor either rescale trigger, and naming it here rather
// than sharing one header constant is deliberate.  Two constants that happen to
// share a value are not one constant.
constexpr uint32_t kSlackMax = 0x4000;

// Exchange two values.  Not `std::swap`, for the same reason `min32` is not
// `std::min`: no `<utility>`, and the width is written down.
//
// Three of these sit in a row in `cost_candidate` when the two reference planes
// come out in the other order, and all three were spelled with a temporary of
// their own -- `tmp`, `swap`, `w1s`.  A swap written that way reads as two
// copies plus a spill, which is what `tools/abpair.py` reported it as the
// moment the clamps around it stopped hiding the shape.
template<class T>
static inline void swap32(T& a, T& b) {
  const T t = a;
  a = b;
  b = t;
}

static inline int32_t clamp_weight(int32_t w) {
  return clamp32(w, kWeightMin, kWeightMax);
}

// Halve a count, rounding *up*: `x - (x >> 1)`, which leaves 1 at 1 and 0 at 0.
// Every adaptive counter in the program is cut back this way, and rounding up
// is the point of it -- a symbol seen once must not be rescaled out of the
// alphabet, and a range coder cannot be handed a zero-weight slot it is still
// willing to code.
//
// Five models share the rule over runs of `uint16_t`: `FreqRec::rescale` over
// five weights, `P2Freq` over three in each of two places, `BitCtr::scale_rare`
// over two and `halve_pair` over a bit-tree pair.  `SymList::rescale` is *not*
// this and is left alone: it halves with `(bias + cnt) >> 1`, rounding down
// with a bias that depends on how far the list is from its rescale point, and
// it re-sorts as it goes.  Its two closing scalars are the shared rule and use
// `halve_up`.
static inline uint32_t halve_up(uint32_t x) {
  return x-(x>>1);
}

static inline void halve_counts(uint16_t* c, int32_t n) {
  for( int32_t k = 0; k<n; ++k )
    c[k] = (uint16_t)halve_up(c[k]);
}

// The exit codes, which are also the indices into the message table below --
// that table is what defines them, so there is nothing here to invent.  They
// are the program's exit status as well, and `test.sh` reads them, so the
// numbers are the interface and not just an ordering.
//
// `bmf_no_option` and `bmf_user_break` are in the table and never raised: the
// original had an option parser and a keyboard-interrupt check, and this build
// has neither.  They stay because removing them would renumber the five that
// are raised.
enum BmfExit {
  bmf_no_file    = 1,
  bmf_no_option  = 2,
  bmf_bad_file   = 3,
  bmf_read_error = 4,
  bmf_write_error= 5,
  bmf_no_open    = 6,
  bmf_no_memory  = 7,
  bmf_user_break = 8,
};

__attribute__((noreturn)) void bmf_fatal(int32_t Code, ...);
