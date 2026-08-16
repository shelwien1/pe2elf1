// alt_p2_decode.inc -- the predictor-2 model, decoding.

// The mirror of `alt_p2_encode.inc`, minus the near-lossless drift control,
// which has nothing to decide here because the code says which fold was used.
//
// It used to say "minus two things", the second being the coefficient-table
// borrow.  That was wrong: this function did the borrow too, in six lines
// identical to the encoder's, 260 lines from the six that undo them.  Both are
// `P2Coef::fold`/`restore` now, which is also how the claim got checked.
int32_t alt_model_p2_decode(BmfImage* p_i, uint8_t* out) {
  AltP2Block* plane[4];
  int16_t seed1;
  uint32_t src2;
  int16_t seed3, seed2;
  bool off;
  int32_t nplanes;
  P2Coef p2_coef;
  p2_coef.fold();
  uint32_t i = p_i->width;
  int32_t raw = p_i->height;
  alt_p2_planes_alloc(plane, i);
  const bool ref1_mixed = ref_transformed(1);
  const bool ref2_mixed = ref_transformed(2);
  const bool ref3_mixed = ref_transformed(3);
  rc_begin_decode();
  if( raw>0 ) {
    src2 = 0;
    nplanes = plane_count;
    do {
      if( nplanes>0 ) {
        alt_p2_start_row(plane, src2, i);
        nplanes = plane_count;
      }
      for( int32_t j = 0; j<4; ++j ) ctx_bias[3-j] = 0;
      if( i>0 ) {
        uint32_t first = src2;
        uint32_t i_2 = 0;
        uint32_t i_1 = i;
        do {
          for( int32_t j = 0; j<4; ++j ) ctx_bias[j] >>= 3;
          AltP2Block* blk0 = plane[0];
          blk0->decode_sample(out, plane_desc[0].src_plane, plane[2], plane[1]);
          blk0->add_bias(ctx_bias);
          off = ref1_mixed==0;
          if( off )
            seed1 = 0;
          else
            seed1 = 16*out[plane_desc[0].src_plane];
          AltP2Block *blk1 = plane[1];
          plane[1]->cursor[0]->dval = seed1;
          blk1->decode_sample(out, plane_desc[1].src_plane, plane[0], plane[2]);
          blk1->add_bias(ctx_bias);
          off = ref2_mixed==0;
          if( off )
            seed2 = 0;
          else
            seed2 = (plane_desc[plane_desc[2].src_plane].weight1*out[plane_desc[1].src_plane]+plane_desc[plane_desc[2].src_plane].weight0*out[plane_desc[0].src_plane])>>3;
          AltP2Block *blk2 = plane[2];
          plane[2]->cursor[0]->dval = seed2;
          blk2->decode_sample(out, plane_desc[2].src_plane, plane[0], plane[1]);
          blk2->add_bias(ctx_bias);
          nplanes = plane_count;
          if( plane_count>=4 ) {
            if( ref3_mixed )
              seed3 = (plane_desc[plane_desc[3].src_plane].weight2*out[2]+plane_desc[plane_desc[3].src_plane].weight1*out[1]+plane_desc[plane_desc[3].src_plane].weight0**out)>>3;
            else
              seed3 = 0;
            AltP2Block *blk3 = plane[3];
            plane[3]->cursor[0]->dval = seed3;
            // `out[3]`, not `out[plane_desc[3].src_plane]`, which is what the
            // other three planes here and all four in the encoder use.  Kept
            // as the binary has it.
            blk3->decode_sample(out, 3, plane[2], plane[0]);
            blk3->add_bias(ctx_bias);
            nplanes = plane_count;
          }
          out += nplanes;
          ++i_2;
        } while( i_2<i_1 );
        src2 = first;
        i = i_1;
      }
      ++src2;
    } while( src2<(uint32_t)raw );
  }
  rc_end_decode();
  p2_coef.restore();
  return alt_p2_planes_free(plane);
}
