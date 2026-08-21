// alt_p2_code.inc -- the predictor-2 model at depths other than eight, both
// directions.
//
// `P2Coef` in alt_p2_block.inc is the coefficient-table borrow both directions
// do for the length of one image.  This file's header used to say the encoder
// did it and the decoder did not; the decoder does, in the same six lines, and
// the two are one object now.
//
// The two halves were 86 and 89 lines in two files and `pairshare.py` measured
// them at 34 shared of 153 -- the largest share of the five pairs bmf.cpp's
// table declined, and declined on the argument that what they share is
// scaffolding rather than algorithm.  Read line by line that turned out not to
// hold here: outside the four coding calls they are the same body, and the
// differences are eight spellings and one deliberate near-miss.
//
//   * `rc_begin_*` and `rc_end_*`, two lines;
//   * `encode_sample` against `decode_sample`, four lines, which is
//     `AltP2Block::code_sample<f_DEC>`;
//   * plane 3's output offset -- the decoder stores to `out[3]` where the
//     encoder and all three other planes use `plane_desc[3].src_plane`.  That
//     is what the binary does and it stays, as the only `f_DEC` in the body
//     that is not about direction;
//   * the rest were spellings.  The row-zero bias loop counted up in one and
//     down in the other over four slots it sets to zero; the sample loop was a
//     `for` in one and a `do`/`while` in the other; and both saved the row
//     index into a second local across a loop that does not touch it -- `xf4`
//     in the encoder, `first` and `i_1` in the decoder -- which is a register
//     spill and not a value.
//
// The gate is the one the other nine merges used: the fifteen streams byte for
// byte at both pointer widths, and both instantiations entered.
void BMFCodec::alt_p2_planes_alloc(AltP2Block** plane, int32_t width) {
  for( int32_t pl = 0; pl<plane_count; ++pl ) {
    AltP2Block* page = p2_blocks.take(true);
    plane[pl] = page->alt_p2_alloc(this, width, pl);
  }
}

template<int32_t f_DEC>
int32_t BMFCodec::alt_model_p2(BmfImage* p_i, uint8_t* out) {
  AltP2Block* plane[4];
  // `coef_session` and not `p2_coef`: the name the tables themselves carry is a
  // `BMFState` member now, and a local of that name shadows it.  `-Wshadow` is
  // in the warning leg for exactly this -- the tree has no `this->` left to
  // disambiguate one from the other, so a shadow here would read as the
  // member and compile.
  P2Coef coef_session;
  coef_session.fold(this);
  const int32_t width = p_i->width;
  const int32_t height = p_i->height;
  alt_p2_planes_alloc(plane, width);
  const bool ref1_mixed = ref_transformed(1);
  const bool ref2_mixed = ref_transformed(2);
  const bool ref3_mixed = ref_transformed(3);
  if constexpr( f_DEC )
    rc_begin_decode();
  else
    rc_begin_encode();
  int32_t nplanes = plane_count;
  for( int32_t y = 0; y<height; ++y ) {
    if( nplanes>0 ) {
      alt_p2_start_row(plane, y, width);
      nplanes = plane_count;
    }
    // Each row starts the bias accumulators at zero.  They were four locals in
    // the encoder -- `at0`, `seed0`, `off0`, `want0` -- with `ctx_bias` loaded
    // from them at the head of every sample and written back at the end of the
    // row.
    for( int32_t j = 0; j<4; ++j ) ctx_bias[j] = 0;
    for( int32_t x = 0; x<width; ++x ) {
      for( int32_t j = 0; j<4; ++j ) ctx_bias[j] >>= 3;
      plane[0]->code_sample<f_DEC>(out, plane_desc[0].src_plane,
                                   plane[2], plane[1]);
      plane[0]->add_bias(ctx_bias);
      // Every plane after the first seeds its prediction from the planes
      // already coded at this pixel, blended by the descriptor's weights -- or
      // from nothing, when the descriptor says this plane is not mixed.
      int16_t seed1 = 0;
      if( ref1_mixed )
        seed1 = 16*out[plane_desc[0].src_plane];
      plane[1]->cursor[0]->dval = seed1;
      plane[1]->code_sample<f_DEC>(out, plane_desc[1].src_plane,
                                   plane[0], plane[2]);
      plane[1]->add_bias(ctx_bias);
      int16_t seed2 = 0;
      if( ref2_mixed ) {
        // The weights belong to the plane *this* plane references, which is why
        // the subscript is a descriptor's `src_plane` and not `2`.  Named
        // because the expression reached it twice and the seed below three
        // times, and at that length the two `plane_desc` subscripts read as one
        // thing.
        const PlaneDesc& mix = plane_desc[plane_desc[2].src_plane];
        seed2 = (mix.weight1*out[plane_desc[1].src_plane]
                +mix.weight0*out[plane_desc[0].src_plane])>>3;
      }
      plane[2]->cursor[0]->dval = seed2;
      plane[2]->code_sample<f_DEC>(out, plane_desc[2].src_plane,
                                   plane[0], plane[1]);
      plane[2]->add_bias(ctx_bias);
      nplanes = plane_count;
      if( plane_count>=4 ) {
        int16_t seed3 = 0;
        if( ref3_mixed ) {
          // `out[0]` and not `*out`, which is what this said: the three
          // operands are planes 0, 1 and 2 of one pixel and the other two are
          // already subscripts.  It also read as `weight0 ** out` -- a multiply
          // against a dereference, spelled with no space, which is two stars a
          // reader has to take apart before the line means anything.
          const PlaneDesc& mix = plane_desc[plane_desc[3].src_plane];
          seed3 = (mix.weight2*out[2]+mix.weight1*out[1]+mix.weight0*out[0])>>3;
        }
        plane[3]->cursor[0]->dval = seed3;
        // The decoder stores to `out[3]` where the encoder uses
        // `plane_desc[3].src_plane`, as do all three planes above in both
        // directions.  Kept as the binary has it: this is the one `f_DEC` here
        // that is not about which way the coder is running.
        plane[3]->code_sample<f_DEC>(out, f_DEC ? 3 : plane_desc[3].src_plane,
                                     plane[2], plane[0]);
        plane[3]->add_bias(ctx_bias);
        nplanes = plane_count;
      }
      out += nplanes;
    }
  }
  if constexpr( f_DEC )
    rc_end_decode();
  else
    rc_end_encode();
  coef_session.restore(this);
  return planes_free(p2_blocks, plane);
}

// The two names `plane.inc` reaches for.  The decode half was a file of its own
// holding these two lines; `alt_p1_code.inc` keeps its pair together and this
// is the same shape one model over.
int32_t BMFCodec::alt_model_p2_encode(BmfImage* p_i, uint8_t* a2) {
  return alt_model_p2<0>(p_i, a2);
}

int32_t BMFCodec::alt_model_p2_decode(BmfImage* p_i, uint8_t* out) {
  return alt_model_p2<1>(p_i, out);
}
