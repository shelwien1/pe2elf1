// alt_p2_encode.inc -- the predictor-2 model, encoding.

// `P2Coef` in alt_p2_block.inc is the coefficient-table borrow both directions
// do for the length of one image.  This file's header used to say the encoder
// did it and the decoder did not; the decoder does, in the same six lines, and
// the two are one object now.
int32_t alt_model_p2_encode(BmfImage* p_i, uint8_t* a2) {
  AltP2Block* plane[4];
  uint32_t i, src1;
  int16_t seed1, l7c, seed2;
  int32_t nplanes;
  uint8_t* out = a2;
  P2Coef p2_coef;
  p2_coef.fold();
  int32_t row_i = p_i->width;
  int32_t height = p_i->height;
  alt_p2_planes_alloc(plane, row_i);
  const bool ref1_mixed = ref_transformed(1);
  const bool ref2_mixed = ref_transformed(2);
  const bool ref3_mixed = ref_transformed(3);
  rc_begin_encode();
  if( height>0 ) {
    src1 = 0;
    nplanes = plane_count;
    do {
      if( nplanes>0 ) {
        alt_p2_start_row(plane, src1, row_i);
        nplanes = plane_count;
      }
      if( row_i<=0 ) {
        for( int32_t j = 0; j<4; ++j ) ctx_bias[j] = 0;
      } else {
        uint32_t xf4 = src1;
        // Each row starts the bias accumulators at zero.  They were four
        // locals here -- `at0`, `seed0`, `off0`, `want0` -- and `ctx_bias` was
        // loaded from them at the head of every sample and written back at the
        // end of the row.
        for( int32_t j = 0; j<4; ++j ) ctx_bias[j] = 0;
        for( i = 0; i<(uint32_t)row_i; ++i ) {
          int32_t off0b = plane_desc[0].src_plane;
          for( int32_t j = 0; j<4; ++j ) ctx_bias[j] >>= 3;
          AltP2Block *blk0 = (AltP2Block*)(plane[0]);
          blk0->encode_sample(out, off0b, plane[2], plane[1]);
          blk0->add_bias(ctx_bias);
          if( ref1_mixed )
            seed1 = 16*out[plane_desc[0].src_plane];
          else
            seed1 = 0;
          AltP2Block *blk1 = (AltP2Block*)(plane[1]);
          plane[1]->cursor[0]->dval = seed1;
          int32_t off1 = plane_desc[1].src_plane;
          blk1->encode_sample(out, off1, plane[0], plane[2]);
          blk1->add_bias(ctx_bias);
          if( ref2_mixed )
            seed2 = (plane_desc[plane_desc[2].src_plane].weight1*out[plane_desc[1].src_plane]+plane_desc[plane_desc[2].src_plane].weight0*out[plane_desc[0].src_plane])>>3;
          else
            seed2 = 0;
          AltP2Block *blk2 = (AltP2Block*)(plane[2]);
          plane[2]->cursor[0]->dval = seed2;
          int32_t off2 = plane_desc[2].src_plane;
          blk2->encode_sample(out, off2, plane[0], plane[1]);
          blk2->add_bias(ctx_bias);
          nplanes = plane_count;
          if( plane_count>=4 ) {
            if( ref3_mixed )
              l7c = (plane_desc[plane_desc[3].src_plane].weight2*out[2]+plane_desc[plane_desc[3].src_plane].weight1*out[1]+plane_desc[plane_desc[3].src_plane].weight0**out)>>3;
            else
              l7c = 0;
            AltP2Block *l4c = (AltP2Block*)(plane[3]);
            plane[3]->cursor[0]->dval = l7c;
            int32_t alpha_src = plane_desc[3].src_plane;
            l4c->encode_sample(out, alpha_src, plane[2], plane[0]);
            l4c->add_bias(ctx_bias);
            nplanes = plane_count;
          }
          out += nplanes;
        }
        src1 = xf4;
      }
      ++src1;
    } while( src1<(uint32_t)height );
  }
  rc_end_encode();
  p2_coef.restore();
  return alt_p2_planes_free(plane);
}
