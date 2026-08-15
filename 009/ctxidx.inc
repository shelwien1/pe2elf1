// ctxidx.inc -- build a context index out of named bit fields.
//
// Taken from `dummy.cpp`'s `SseIdx`, which does the same job for that
// program's secondary-estimation indices.  Renamed for what it indexes here:
// nothing in BMF is an SSE table, and every use below is a model context.
//
// The decompilation writes these as one multiplication chain --
//
//     nb = 32*W.match[2] + 16*W.match[4] + 2*W.match[0] + 8*NE.match[1]
//        + (N.match[1] + 4*(NE_sym == NW_sym))
//
// -- where every constant is a shift written as a multiply, the fields arrive
// in whatever order the register allocator wanted, and there is no way to see
// that the six terms are six *bits* of one six-bit word without adding up the
// powers of two by hand.  Hex-Rays writes them that way because MSVC emitted
// `lea` chains; the shape was never in the source.
//
//     nb = CtxIdx{}.bit<5>(W.match[2]).bit<4>(W.match[4])
//                  .bit<3>(NE.match[1]).bit<2>(NE_sym == NW_sym)
//                  .bit<1>(W.match[0]).bit<0>(N.match[1]);
//
// Same value, and now the width of the index is countable and a field cannot
// silently overlap its neighbour.
//
// It compiles to the same code: every method is `constexpr`, takes its
// position as a template parameter, and does one shift and one add.  The gate
// is what says so -- the fifteen streams are byte for byte what they were
// before this file existed.
struct CtxIdx {
  uint32_t val = 0;

  // One bit at `Pos`.
  template<int32_t Pos>
  constexpr CtxIdx&bit(bool b) {
    val += (uint32_t)b<<Pos;
    return *this;
  }

  // `W` bits of `v`, moved to `Pos`.
  template<int32_t Pos, int32_t W>
  constexpr CtxIdx&bits(uint32_t v) {
    val += (v&((1u<<W)-1))<<Pos;
    return *this;
  }

  // A mixed-radix digit: `v` counts in units of `Stride`, and `Radix` is how
  // many values it has.  Not a bit field -- `Stride` need not be a power of
  // two, and `Radix` need not be the next one up from it.
  //
  // `nb_slot` is the reason this exists.  `gA` strides 64 with five values and
  // `band` strides 320 -- which is 5*64 -- with six, so the pair is two digits
  // of a mixed-radix number rather than two fields, and writing either as
  // `bits<>` says a width where the truth is a radix.  That is not pedantry:
  // `bits<6, 2>(gA)` was tried, and the one neighbourhood in the corpus that
  // reaches `gA == 4` landed in slot 0 instead of 256, moving three streams.
  //
  // The radix is not enforced here -- it is a fact about the caller's value,
  // and this is a constant-folded add -- but it is written down, which is what
  // says `320 * 6 = 1920` is the size the table has to be.
  template<uint32_t Stride, uint32_t Radix>
  constexpr CtxIdx&digit(uint32_t v) {
    val += Stride*v;
    return *this;
  }

  // A term that is neither a bit field nor a bounded digit -- an index already
  // built, or one whose range the code cannot state.
  constexpr CtxIdx&raw(uint32_t v) {
    val += v;
    return *this;
  }

  constexpr operator uint32_t() const {
    return val;
  }
};
