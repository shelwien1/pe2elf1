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
  template<int Pos>
  constexpr CtxIdx&bit(bool b) {
    val += (uint32_t)b<<Pos;
    return *this;
  }

  // `W` bits of `v`, moved to `Pos`.
  template<int Pos, int W>
  constexpr CtxIdx&bits(uint32_t v) {
    val += (v&((1u<<W)-1))<<Pos;
    return *this;
  }

  // A term that is not a bit field -- a stride that is not a power of two, or
  // an index already built.
  constexpr CtxIdx&raw(uint32_t v) {
    val += v;
    return *this;
  }

  constexpr operator uint32_t() const {
    return val;
  }
};
