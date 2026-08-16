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
  uint32_t i_1, first;
  int16_t seed1;
  uint32_t src2;
  int16_t seed3, seed2;
  AltP2Block** planep;
  bool off;
  int32_t nplanes;
  P2Coef p2_coef;
  p2_coef.fold();
  uint32_t i = p_i->width;
  int32_t raw = p_i->height;
  alt_p2_planes_alloc(plane, i);
  int32_t xf0 = plane_desc[plane_desc[1].src_plane].flags&desc_has_refs;
  int32_t dc_flag = plane_desc[plane_desc[2].src_plane].flags&desc_has_refs;
  int32_t xf4 = (plane_desc[plane_desc[3].src_plane].flags)&desc_has_refs;
  rc_begin_decode();
  if( raw>0 ) {
    src2 = 0;
    int32_t back = -i;
    nplanes = plane_count;
    do {
      if( nplanes>0 ) {
        first = src2;
        int32_t pl2 = 0;
        i_1 = i;
        do {
          ++pl2;
          if( first ) {
            planep = &plane[pl2];
            if( first==1 ) {
              AltP2Block *blk_k = (AltP2Block*)(*(planep-1));
              blk_k->seed_history(-back);
            }
          } else {
            AltP2Block *src3 = (AltP2Block*)(plane[pl2-1]);
            planep = &plane[pl2];
            src3->seed_row0(i);
          }
          AltP2Block *blk_r = (AltP2Block*)(*(planep-1));
          blk_r->start_row();
          nplanes = plane_count;
        } while( plane_count>pl2 );
        src2 = first;
        i = i_1;
      }
      for( int32_t j = 0; j<4; ++j ) ctx_bias[3-j] = 0;
      if( i>0 ) {
        first = src2;
        uint32_t i_2 = 0;
        i_1 = i;
        do {
          for( int32_t j = 0; j<4; ++j ) ctx_bias[j] >>= 3;
          AltP2Block* blk0 = (AltP2Block*)(plane[0]);
          blk0->decode_sample(out, plane_desc[0].src_plane, plane[2], plane[1]);
          P2Ctx *cur0 = blk0->cursor[0];
          ctx_bias[0] += 32*cur0[-1].dupleft;
          ctx_bias[1] += 32*cur0[-1].dupright;
          ctx_bias[2] += 32*cur0[-1].dleft;
          ctx_bias[3] += 32*cur0[-1].dup;
          off = xf0==0;
          if( off )
            seed1 = 0;
          else
            seed1 = 16*out[plane_desc[0].src_plane];
          AltP2Block *blk1 = (AltP2Block*)(plane[1]);
          plane[1]->cursor[0]->dval = seed1;
          blk1->decode_sample(out, plane_desc[1].src_plane, plane[0], plane[2]);
          P2Ctx *cur1 = blk1->cursor[0];
          int32_t l7a = cur1[-1].dupright;
          int32_t l4a = cur1[-1].dleft;
          ctx_bias[0] += 32*cur1[-1].dupleft;
          int32_t l5a = cur1[-1].dup;
          ctx_bias[1] += 32*l7a;
          ctx_bias[2] += 32*l4a;
          ctx_bias[3] += 32*l5a;
          off = dc_flag==0;
          if( off )
            seed2 = 0;
          else
            seed2 = (plane_desc[plane_desc[2].src_plane].weight1*out[plane_desc[1].src_plane]+plane_desc[plane_desc[2].src_plane].weight0*out[plane_desc[0].src_plane])>>3;
          AltP2Block *blk2 = (AltP2Block*)(plane[2]);
          plane[2]->cursor[0]->dval = seed2;
          blk2->decode_sample(out, plane_desc[2].src_plane, plane[0], plane[1]);
          P2Ctx *cur2 = blk2->cursor[0];
          int32_t l7b = cur2[-1].dupright;
          int32_t l4b = cur2[-1].dleft;
          ctx_bias[0] += 32*cur2[-1].dupleft;
          int32_t l5b = cur2[-1].dup;
          ctx_bias[1] += 32*l7b;
          ctx_bias[2] += 32*l4b;
          ctx_bias[3] += 32*l5b;
          nplanes = plane_count;
          if( plane_count>=4 ) {
            if( xf4 )
              seed3 = (plane_desc[plane_desc[3].src_plane].weight2*out[2]+plane_desc[plane_desc[3].src_plane].weight1*out[1]+plane_desc[plane_desc[3].src_plane].weight0**out)>>3;
            else
              seed3 = 0;
            AltP2Block *blk3 = (AltP2Block*)(plane[3]);
            plane[3]->cursor[0]->dval = seed3;
            // `out[3]`, not `out[plane_desc[3].src_plane]`, which is what the
            // other three planes here and all four in the encoder use.  Kept
            // as the binary has it.
            blk3->decode_sample(out, 3, plane[2], plane[0]);
            P2Ctx *cur3 = blk3->cursor[0];
            int32_t l7c = cur3[-1].dupright;
            int32_t l4c = cur3[-1].dleft;
            int32_t l5c = cur3[-1].dup;
            ctx_bias[0] += 32*cur3[-1].dupleft;
            ctx_bias[1] += 32*l7c;
            ctx_bias[2] += 32*l4c;
            ctx_bias[3] += 32*l5c;
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
