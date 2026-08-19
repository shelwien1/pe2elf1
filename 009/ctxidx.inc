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

  // Bit `Pos` of `v`, kept where it already is.
  //
  // Not `bit<Pos>`, and the difference is the whole reason this exists.
  // `bit<Pos>(b)` *puts* a boolean at `Pos` and is a claim: that the term is
  // one bit wide and this is its value.  `bit_of<Pos>(v)` *takes* the bit that
  // is already at `Pos` and claims nothing about the rest of `v` -- which is
  // what the six bank contexts in `alt_p2_context` need, because their terms
  // carry an accumulated `run` with no type bound and the masked bit is
  // demonstrably not always the sign.  `tools/ctxidx.py` measured four of them
  // reaching well past their bit.
  //
  // Before this, such a term had two spellings and both were bad.  Written as
  // the decompiler left it, `(expr) & 0x2000000`, the position is a hex
  // literal that has to be counted out by hand and a `// bit 25` comment
  // beside it that nothing checks.  Written as `raw((expr) & 0x2000000)` --
  // which is what `ctxidx.py` used to offer and why it refused all six lines --
  // it is the same literal with scaffolding around it.
  //
  // Here the position *is* the mask: `1u << Pos` is generated, so a mask that
  // disagrees with its label cannot be written, and the widths of a word's
  // fields can be read off the chain the way every other `CtxIdx` user's can.
  //
  // `|=` rather than `+=`, because that is what the source does and because a
  // term whose other bits this deliberately does not bound must not be able to
  // carry into its neighbour.  The fields are disjoint -- that is what the
  // type is for -- so the two agree; the one that cannot carry is the one to
  // write.
  template<int32_t Pos>
  constexpr CtxIdx&bit_of(uint32_t v) {
    val |= v&(1u<<Pos);
    return *this;
  }

  // `v` from bit `Pos` up, with no claim about how wide it is.
  //
  // The high end of an index, where `bits<Pos, W>` would be a lie and `raw` is
  // silence.  `sig1`'s bucket, `sig2`'s and `sig3`'s interned ids, the run
  // bucket in both escape contexts, the ladder position and the plane: seven
  // terms, each of which is a value that has to clear the fields under it and
  // whose own range is bounded by a table size somewhere else, not by anything
  // visible at the index.
  //
  // They were `raw(8*id1)`, `raw(16*id2)`, `raw(bucket_at<<10)`, `raw(pos<<7)`
  // -- the multiply-for-a-shift this whole struct was written to get rid of.
  // `raw` says "some number, added"; `above<3>` says "everything below bit 3
  // belongs to the fields you just read".  That is the fact a reader of an
  // index wants, and the one thing `8*` makes them work out by hand.
  //
  // Deliberately not masked.  A `bits<Pos, W>` here would quietly drop the top
  // of an id that outgrew `W`, which is the failure mode that moved three
  // streams when `gA` was written as a two-bit field; this cannot silently
  // truncate because it does not claim a width at all.  What it costs is that
  // an overlap with the fields below is not caught -- but those fields are
  // `bit<>` and `bits<>` and countable, which is the whole point of writing the
  // index this way.
  template<int32_t Pos>
  constexpr CtxIdx&above(uint32_t v) {
    val += v<<Pos;
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
